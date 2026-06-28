import json
import os
import queue
import threading
import time
from concurrent.futures import ThreadPoolExecutor
from urllib.parse import urlparse, unquote
import uuid
import warnings

import cloudscraper
from PyQt6.QtCore import QThread, pyqtSignal
import requests
from urllib3.exceptions import InsecureRequestWarning

from config import *
from search_index import translation_index

_PARALLEL_THRESHOLD = 2 * 1024 * 1024  # skip parallel for files < 2 MB

warnings.simplefilter("ignore", InsecureRequestWarning)


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
        self.html_content = ""
        self.downloaded_file_path = ""

    def get_webpage_content(self, url, verify=True, use_cloudScraper=False):
        if not self.is_internet_connected():
            return ""

        try:
            if use_cloudScraper:
                scraper = cloudscraper.create_scraper()
                req = scraper.get(url, headers=self.headers, verify=verify)
            else:
                req = requests.get(url, headers=self.headers, verify=verify)
            req.raise_for_status()
        except Exception as e:
            print(f"Error requesting {url}: {str(e)}")
            return ""

        self.html_content = req.text

        return self.html_content

    def request_download(self, url, download_path, verify=True, use_cloudScraper=False):
        try:
            if use_cloudScraper:
                scraper = cloudscraper.create_scraper()
                req = scraper.get(url, headers=self.headers, verify=verify, stream=True, timeout=30)
            else:
                req = requests.get(url, headers=self.headers, verify=verify, stream=True, timeout=30)
            req.raise_for_status()
        except Exception as e:
            print(f"Error requesting {url}: {str(e)}")
            return ""

        file_path = os.path.join(download_path, self.find_download_fname(req))
        total_size = int(req.headers.get('content-length', 0))
        supports_ranges = req.headers.get('accept-ranges', '').lower() == 'bytes'

        if not use_cloudScraper and supports_ranges and total_size >= _PARALLEL_THRESHOLD:
            num_workers = min(total_size // (1024 * 1024), 6)
        else:
            num_workers = 1
        return self.download_queued(req, url, file_path, total_size, verify, num_workers)

    def download_queued(self, initial_req, url, file_path, total_size, verify, num_workers):
        total_units = min(total_size // (1024 * 1024), 24) if num_workers > 1 else 1
        print(f"[Queue] starting: {total_units} units, {num_workers} workers, {total_size / (1024 * 1024):.1f} MB", flush=True)

        unit_queue = queue.Queue()
        if num_workers > 1:
            unit_size = total_size // total_units
            for i in range(total_units):
                start = i * unit_size
                end = (i + 1) * unit_size - 1 if i < total_units - 1 else total_size - 1
                unit_queue.put((i, start, end))
            initial_req.close()
        else:
            unit_queue.put((0, None, None))

        # Pre-allocate file so workers can seek and write directly at their byte offsets
        try:
            with open(file_path, 'wb') as f:
                f.truncate(total_size)
        except Exception as e:
            print(f"[Queue] failed to create file: {e}", flush=True)
            return ""

        total_downloaded = [0]
        completed_units = [0]
        total_failures = [0]
        MAX_FAILURES = total_units + 10
        stop_event = threading.Event()
        downloaded_lock = threading.Lock()
        last_emit = [0.0]
        EMIT_INTERVAL = 0.05

        def emit_progress():
            self.progress.emit([(total_downloaded[0], total_size)])

        emit_progress()

        def worker(worker_idx):
            session = requests.Session() if num_workers > 1 else None
            f = open(file_path, 'r+b')
            try:
                while not stop_event.is_set():
                    try:
                        unit_idx, start, end = unit_queue.get_nowait()
                    except queue.Empty:
                        break

                    bytes_this_unit = 0
                    try:
                        if start is not None:
                            resp = session.get(url, headers={**self.headers, 'Range': f'bytes={start}-{end}'}, verify=verify, stream=True, timeout=60)
                            resp.raise_for_status()
                        else:
                            resp = initial_req
                        f.seek(start if start is not None else 0)
                        for piece in resp.iter_content(chunk_size=65536):
                            if piece:
                                f.write(piece)
                                bytes_this_unit += len(piece)
                                with downloaded_lock:
                                    total_downloaded[0] += len(piece)
                                    now = time.monotonic()
                                    if now - last_emit[0] >= EMIT_INTERVAL:
                                        last_emit[0] = now
                                        emit_progress()
                        with downloaded_lock:
                            completed_units[0] += 1
                    except Exception as e:
                        with downloaded_lock:
                            total_downloaded[0] -= bytes_this_unit
                            total_failures[0] += 1
                            failures = total_failures[0]
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
            for i in range(num_workers):
                executor.submit(worker, i)

        if stop_event.is_set() or completed_units[0] < total_units:
            print(f"[Queue] download failed: {total_failures[0]} total failures", flush=True)
            if os.path.exists(file_path):
                os.remove(file_path)
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
    def get_signed_download_url(file_path_on_s3):
        if not SIGNED_URL_DOWNLOAD_ENDPOINT or not CLIENT_API_KEY:
            print("Error: API endpoint or Client API Key is not configured.")
            return None

        headers = {
            'x-api-key': CLIENT_API_KEY
        }
        params = {
            'filePath': file_path_on_s3
        }

        try:
            response = requests.get(SIGNED_URL_DOWNLOAD_ENDPOINT, headers=headers, params=params, timeout=15)
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
            'metadata': metadata_json
        }

        try:
            response = requests.get(SIGNED_URL_UPLOAD_ENDPOINT, headers=headers, params=params, timeout=15)
            response.raise_for_status()
            return response.json()

        except Exception as e:
            print(f"Error retrieving signed URL: {str(e)}")
        return None

    @staticmethod
    def is_internet_connected(urls=None, timeout=5):
        if urls is None:
            urls = [
                "https://www.bing.com/",
                "https://www.baidu.com/",
                "http://www.google.com/",
                "https://www.apple.com/",
                "https://www.wechat.com/"
            ]

        for url in urls:
            try:
                response = requests.head(url, timeout=timeout)
                response.raise_for_status()
                return True
            except requests.RequestException:
                continue
        return False

    @staticmethod
    def symbol_replacement(text):
        return text.replace(': ', ' - ').replace(':', '-').replace("/", "_").replace("?", "")

    def find_best_trainer_match(self, target_name, target_language, threshold=85):
        # Ignore cases where input trainer name and target language are the same
        if is_chinese(target_name) and target_language == 'zh':
            return None
        elif not is_chinese(target_name) and target_language == 'en':
            return None

        # Exact (cached dict) lookup first, fuzzy fallback within the index
        return translation_index.translate(target_name, target_language, threshold)

    def translate_trainer(self, trainer):
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
                best_match = self.find_best_trainer_match(game_name, lang_key)
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
    def save_html_content(content, file_name, overwrite=True):
        html_file = os.path.join(DATABASE_PATH, file_name)
        mode = 'w' if overwrite else 'a'
        with open(html_file, mode, encoding='utf-8') as file:
            file.write(content)

    @staticmethod
    def load_html_content(file_name):
        html_file = os.path.join(DATABASE_PATH, file_name)
        if os.path.exists(html_file):
            try:
                with open(html_file, 'r', encoding='utf-8') as file:
                    return file.read()
            except Exception as e:
                print(f"Error loading HTML content from {html_file}: {str(e)}")
                return ""
        return ""

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
