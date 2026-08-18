import json
import os
import queue
import threading
import time
from concurrent.futures import ThreadPoolExecutor
from urllib.parse import urlparse, unquote
import uuid

from PyQt6.QtCore import QThread, pyqtSignal
import requests

from config import *
from search_index import translation_index

_PARALLEL_THRESHOLD = 2 * 1024 * 1024  # skip parallel for files < 2 MB
_API_TIMEOUT = (5, 15)
_DOWNLOAD_TIMEOUT = (5, 30)
_RANGE_DOWNLOAD_TIMEOUT = (5, 60)


class DownloadBaseThread(QThread):
    message = pyqtSignal(str, str)
    messageBox = pyqtSignal(str, str, str)
    progress = pyqtSignal(list)  # [(downloaded, total)]
    finished = pyqtSignal(int)

    trainer_urls = []  # [{"game_name": str, "trainer_name": str, "origin": str, "author": str, "custom_name": str, "url": download url, "version": YYYY.MM.DD},]
    headers = {
        "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.110 Safari/537.36",
        "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8",
    }

    def __init__(self, parent=None):
        super().__init__(parent)
        self.downloaded_file_path = ""

    def request_download(self, url, download_path, raise_errors=False, atomic=False):
        """Set atomic=True when overwriting a file that may be read concurrently (e.g. the
        database JSONs), so readers never observe a truncated or partially written file."""
        try:
            req = requests.get(url, headers=self.headers, stream=True, timeout=_DOWNLOAD_TIMEOUT)
            req.raise_for_status()
        except Exception as e:
            print(f"Error requesting {url}: {str(e)}")
            if raise_errors:
                raise
            return ""

        # Headers of this first response decide the file name, the size, and how it is fetched.
        file_path = os.path.join(download_path, os.path.basename(self.find_download_fname(req)))
        total_size = int(req.headers.get('content-length', 0))
        supports_ranges = req.headers.get('accept-ranges', '').lower() == 'bytes'

        # One worker per MB, capped, and only when the server claims to serve byte ranges
        if supports_ranges and total_size >= _PARALLEL_THRESHOLD:
            num_workers = min(total_size // (1024 * 1024), 6)
        else:
            num_workers = 1
        return self.download_queued(req, url, file_path, total_size, num_workers, raise_errors, atomic)

    def download_queued(self, initial_req, url, file_path, total_size, num_workers, raise_errors=False, atomic=False):
        total_units = min(total_size // (1024 * 1024), 24) if num_workers > 1 else 1
        print(f"[Queue] starting: {total_units} units, {num_workers} workers, {total_size / (1024 * 1024):.1f} MB", flush=True)

        # Split the file into byte ranges workers pull from, or one unit that streams it whole
        unit_queue = queue.Queue()
        if num_workers > 1:
            unit_size = total_size // total_units
            for i in range(total_units):
                start = i * unit_size
                end = (i + 1) * unit_size - 1 if i < total_units - 1 else total_size - 1
                unit_queue.put((i, start, end))
            if initial_req is not None:
                initial_req.close()  # its body is unused, every unit fetches its own range instead
        else:
            unit_queue.put((0, None, None))

        # When atomic, download into a sibling temp file
        write_path = os.path.join(os.path.dirname(file_path), f".gcm-{uuid.uuid4().hex[:8]}.part") if atomic else file_path

        # Pre-allocate file so workers can seek and write directly at their byte offsets
        try:
            with open(write_path, 'wb') as f:
                f.truncate(total_size)
        except Exception as e:
            print(f"[Queue] failed to create file: {e}", flush=True)
            return ""

        # Shared across workers, so every read and write of these goes through downloaded_lock
        total_downloaded = [0]
        completed_units = [0]
        total_failures = [0]
        last_error = [None]
        ranges_ignored = [False]
        MAX_FAILURES = total_units + 10
        stop_event = threading.Event()
        downloaded_lock = threading.Lock()
        last_emit = [0.0]
        EMIT_INTERVAL = 0.05

        # The initial response is good for one read, so a retry has to ask for the file again
        first_stream = [initial_req]

        def open_full_stream():
            resp = requests.get(url, headers=self.headers, stream=True, timeout=_DOWNLOAD_TIMEOUT)
            resp.raise_for_status()
            return resp

        def abandon_ranges():
            """Worth redoing the download without ranges only while nothing has succeeded yet."""
            if completed_units[0] == 0:
                ranges_ignored[0] = True
                stop_event.set()

        def emit_progress():
            self.progress.emit([(total_downloaded[0], total_size)])

        def discard_partial():
            try:
                os.remove(write_path)
            except OSError as e:
                print(f"[Queue] failed to remove partial file {write_path}: {e}", flush=True)

        emit_progress()

        def worker():
            session = requests.Session() if num_workers > 1 else None
            f = open(write_path, 'r+b')
            try:
                # Workers race for units rather than owning a fixed share, so a slow one holds nobody up
                while not stop_event.is_set():
                    try:
                        unit_idx, start, end = unit_queue.get_nowait()
                    except queue.Empty:
                        break

                    bytes_this_unit = 0
                    expected_bytes = (end - start + 1) if start is not None else total_size
                    try:
                        if start is not None:
                            resp = session.get(url, headers={**self.headers, 'Range': f'bytes={start}-{end}'}, stream=True, timeout=_RANGE_DOWNLOAD_TIMEOUT)
                            resp.raise_for_status()
                            # 200 means the range was ignored and the whole file is coming
                            if resp.status_code != 206:
                                abandon_ranges()
                                raise Exception(f"expected 206 for range {start}-{end}, got {resp.status_code}")
                        elif first_stream[0] is not None:
                            resp, first_stream[0] = first_stream[0], None
                        else:
                            resp = open_full_stream()

                        # Every unit writes at its own offset, which is why the file is pre-allocated
                        f.seek(start if start is not None else 0)
                        for piece in resp.iter_content(chunk_size=65536):
                            if piece:
                                # A status code is only a claim, so hold each unit to its byte count
                                if expected_bytes and bytes_this_unit + len(piece) > expected_bytes:
                                    if start is not None:
                                        abandon_ranges()
                                    raise Exception(f"unit {unit_idx} overran its {expected_bytes} byte slice")
                                f.write(piece)
                                bytes_this_unit += len(piece)
                                with downloaded_lock:
                                    total_downloaded[0] += len(piece)
                                    now = time.monotonic()
                                    if now - last_emit[0] >= EMIT_INTERVAL:  # throttle UI updates
                                        last_emit[0] = now
                                        emit_progress()

                        # A short slice would otherwise leave a silent hole in the file
                        if expected_bytes and bytes_this_unit != expected_bytes:
                            raise Exception(f"unit {unit_idx} got {bytes_this_unit} of {expected_bytes} bytes")
                        with downloaded_lock:
                            completed_units[0] += 1

                    except Exception as e:
                        with downloaded_lock:
                            total_downloaded[0] -= bytes_this_unit  # uncount it, the unit is redone
                            total_failures[0] += 1
                            last_error[0] = e
                            failures = total_failures[0]
                        # Failures are pooled, so one hopeless unit cannot retry forever
                        if failures >= MAX_FAILURES:
                            stop_event.set()
                            print(f"[Queue] unit {unit_idx} failed, aborting ({failures}/{MAX_FAILURES} total failures): {e}", flush=True)
                        else:
                            unit_queue.put((unit_idx, start, end))
                            print(f"[Queue] unit {unit_idx} failed, re-enqueued ({failures}/{MAX_FAILURES} total failures): {e}", flush=True)
            finally:
                f.close()
                if session:
                    session.close()

        with ThreadPoolExecutor(max_workers=num_workers) as executor:
            for _ in range(num_workers):
                executor.submit(worker)

        # Ranges were advertised but not honoured, so redo the whole thing in one stream
        if ranges_ignored[0]:
            print("[Queue] server ignored range requests, retrying as a single stream", flush=True)
            discard_partial()
            return self.download_queued(None, url, file_path, total_size, 1, raise_errors, atomic)

        if stop_event.is_set() or completed_units[0] < total_units:
            print(f"[Queue] download failed: {total_failures[0]} total failures", flush=True)
            discard_partial()
            if raise_errors and last_error[0] is not None:
                raise last_error[0]
            return ""

        # Same-directory rename, so readers see either the old file or the new one
        if atomic:
            try:
                os.replace(write_path, file_path)
            except Exception as e:
                print(f"[Queue] failed to replace {file_path}: {e}", flush=True)
                discard_partial()
                if raise_errors:
                    raise
                return ""

        emit_progress()
        self.downloaded_file_path = file_path
        return file_path

    @staticmethod
    def find_download_fname(response):
        content_disposition = response.headers.get('content-disposition')
        if content_disposition:
            if "filename*=" in content_disposition:
                filename_encoded = content_disposition.split("filename*=")[-1].strip('";')
                if filename_encoded.startswith("UTF-8''"):
                    filename_encoded = filename_encoded[len("UTF-8''"):]
                filename = unquote(filename_encoded)
                return filename

            if "filename=" in content_disposition:
                filename = content_disposition.split("filename=")[-1].strip('";')
                return filename

        return urlparse(str(response.url)).path.split("/")[-1]

    @staticmethod
    def get_signed_download_url(file_path_on_s3, raise_errors=False):
        if not SIGNED_URL_DOWNLOAD_ENDPOINT or not CLIENT_API_KEY:
            print("Error: API endpoint or Client API Key is not configured.")
            return None

        headers = {
            'x-api-key': CLIENT_API_KEY
        }
        params = {
            'filePath': file_path_on_s3,
            **get_client_params()
        }

        try:
            response = requests.get(SIGNED_URL_DOWNLOAD_ENDPOINT, headers=headers, params=params, timeout=_API_TIMEOUT)
            response.raise_for_status()

            data = response.json()
            signed_url = data.get('signedUrl')
            if signed_url:
                return signed_url
            else:
                print(f"Error: 'signedUrl' not found in response. Response: {data}")
                return None

        except Exception as e:
            print(f"Error retrieving signed URL: {str(e)}")
            if raise_errors:
                raise
        return None

    @staticmethod
    def get_signed_upload_url(file_path_on_s3, metadata_json):
        if not SIGNED_URL_UPLOAD_ENDPOINT or not CLIENT_API_KEY:
            print("Error: API endpoint or Client API Key is not configured.")
            return None

        # add uniqueness to file
        file_path, file_ext = os.path.splitext(file_path_on_s3)
        file_path_on_s3 = os.path.join("trainers", f"{os.path.basename(file_path)}_{uuid.uuid4().hex}{file_ext}").replace("\\", "/")

        headers = {
            'x-api-key': CLIENT_API_KEY
        }
        params = {
            'filePath': file_path_on_s3,
            'metadata': metadata_json,
            **get_client_params()
        }

        try:
            response = requests.get(SIGNED_URL_UPLOAD_ENDPOINT, headers=headers, params=params, timeout=_API_TIMEOUT)
            response.raise_for_status()
            return response.json()

        except Exception as e:
            print(f"Error retrieving signed URL: {str(e)}")
        return None

    @staticmethod
    def symbol_replacement(text):
        return text.replace(': ', ' - ').replace(':', '-').replace("/", "_").replace("?", "")

    @staticmethod
    def find_best_trainer_match(target_name, target_language, threshold=85):
        # Ignore cases where input trainer name and target language are the same
        if is_chinese(target_name) and target_language == 'zh':
            return None
        elif not is_chinese(target_name) and target_language == 'en':
            return None

        # Exact (cached dict) lookup first, fuzzy fallback within the index
        return translation_index.translate(target_name, target_language, threshold)

    @staticmethod
    def translate_trainer(trainer):
        """
        Dynamically builds the trainer name based on author, origin, and custom names.
        Expects a dictionary: {"game_name": ..., "origin": ..., "author": ..., "custom_name": ..., "custom_name_en": ..., "custom_name_zh": ...}
        """
        PREFIX_MAP = {
            "zh": {
                "fling_main": "风灵",
                "fling_archive": "风灵",
                "xiaoxing": "小幸",
                "the_cheat_script": "CT",
                "ct_other": "CT",
                "gcm": "GCM",
                "other": "其他"
            },
            "en": {
                "fling_main": "FL",
                "fling_archive": "FL",
                "xiaoxing": "XX",
                "the_cheat_script": "CT",
                "ct_other": "CT",
                "gcm": "GCM",
                "other": "OT"
            }
        }

        try:
            game_name = trainer.get("game_name", "")
            origin = trainer.get("origin", "other")
            author = trainer.get("author", "")
            custom_name = trainer.get("custom_name", "")
            custom_name_en = trainer.get("custom_name_en", "")
            custom_name_zh = trainer.get("custom_name_zh", "")

            # 1. Determine Target Language
            if settings["language"] in ["zh_CN", "zh_TW"] and not settings["enSearchResults"]:
                lang_key = "zh"
            else:
                lang_key = "en"

            # 2. Determine Prefix (Author > Built-in Origin Map)
            if author:
                prefix = f"[{author}]"
            else:
                source_str = PREFIX_MAP[lang_key].get(origin, PREFIX_MAP[lang_key]["other"])
                prefix = f"[{source_str}]" if source_str else ""

            # 3. Handle game_name="none" case: display custom name directly
            if game_name.lower() == "none":
                if lang_key == "zh":
                    display_name = custom_name_zh or custom_name or ""
                    trainerName = f"{prefix} {display_name}" if prefix else display_name
                else:
                    display_name = custom_name_en or custom_name or ""
                    trainerName = f"{prefix} {display_name}".strip() if prefix else display_name
            else:
                # 3a. Translate Game Name
                best_match = DownloadBaseThread.find_best_trainer_match(game_name, lang_key)
                translated_game_name = best_match or game_name

                # 4. Construct Final Name
                if lang_key == "zh":
                    # Prioritize Chinese custom name, fallback to generic custom name, finally generic modifier
                    suffix = custom_name_zh or custom_name or "修改器"
                    trainerName = f"{prefix}《{translated_game_name}》{suffix}"
                else:
                    # Prioritize English custom name, fallback to generic custom name, finally generic modifier
                    suffix = custom_name_en or custom_name or "Trainer"
                    trainerName = f"{prefix} {translated_game_name} {suffix}".strip()

        except Exception as e:
            print(f"An error occurred while translating trainer name: {str(e)}")
            return None

        return trainerName

    @staticmethod
    def is_cheat_engine_package(trainer):
        """Cheat Engine ships as a regular GCM entry, identified by its custom name."""
        if str(trainer.get("game_name", "")).lower() != "none":
            return False

        custom_name = trainer.get("custom_name") or ""
        return custom_name.strip().lower() == "cheat engine"

    @staticmethod
    def find_cheat_engine_entry():
        """Build a downloadable entry for Cheat Engine straight from the GCM database."""
        GCMData = DownloadBaseThread.load_json_content("gcm_trainers.json")
        if not GCMData:
            return None

        for trainer in GCMData:
            if DownloadBaseThread.is_cheat_engine_package(trainer):
                entry = {
                    "game_name": trainer.get("game_name"),
                    "trainer_name": None,
                    "origin": trainer.get("origin", "other"),
                    "author": trainer.get("author", ""),
                    "custom_name": trainer.get("custom_name", ""),
                    "custom_name_en": trainer.get("custom_name_en", ""),
                    "custom_name_zh": trainer.get("custom_name_zh", ""),
                    "url": trainer.get("gcm_url"),
                    "version": trainer.get("version", ""),
                    "extension": trainer.get("extension", "")
                }
                # Entries built outside a search still need their display name
                entry["trainer_name"] = DownloadBaseThread.translate_trainer(entry)
                return entry if entry["trainer_name"] else None

        return None

    @staticmethod
    def load_json_content(file_name, from_database=True):
        json_file = os.path.join(DATABASE_PATH, file_name) if from_database else file_name
        if os.path.exists(json_file):
            try:
                with open(json_file, 'r', encoding='utf-8') as file:
                    return json.load(file)
            except Exception as e:
                print(f"Error loading JSON content from {json_file}: {str(e)}")
                return ""
        return ""
