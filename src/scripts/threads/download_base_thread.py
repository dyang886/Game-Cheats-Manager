import hashlib
import json
import os
import queue
import threading
import time
from concurrent.futures import ThreadPoolExecutor
from urllib.parse import urlparse, unquote, unquote_plus
import uuid

from PyQt6.QtCore import QThread, pyqtSignal
import requests
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry

from config import *
import edge_routes
from search_index import translation_index

_PARALLEL_THRESHOLD = 2 * 1024 * 1024  # skip parallel for files < 2 MB
_DOWNLOAD_TIMEOUT = (10, 30)  # the read timeout is also what notices a connection that was dropped silently
_PINNED_TIMEOUT = (5, 30)  # a picked IP that doesn't answer fails fast, and another route takes over
# One connection and one range per MB, up to these
_MAX_WORKERS = 12
_MAX_UNITS = 48
_PIECE = 16 * 1024  # small pieces keep progress and handoffs responsive on slow lines
_STEAL_MIN_BYTES = 64 * 1024  # a range with less left finishes where it is


class _Handoff(Exception):
    """A slow range's rest taken over by a faster idle worker at the end."""


def create_http_session(pinned=False):
    """A session that retries transient failures. Use one per download: a connection left idle for
    later can be dropped silently by NATs and firewalls, and reusing it stalls for a whole read timeout.
    A pinned session only retries error statuses: for a connection problem, another route is the better retry."""
    retry = Retry(
        total=3, connect=0 if pinned else None, read=0 if pinned else None, backoff_factor=0.5, backoff_jitter=0.5,
        status_forcelist=RETRY_STATUSES, allowed_methods=frozenset({"GET"}), raise_on_status=False, respect_retry_after_header=False
    )
    adapter = HTTPAdapter(max_retries=retry, pool_maxsize=16)
    session = requests.Session()
    session.mount("https://", adapter)
    session.mount("http://", adapter)
    return session


def _timeout(route):
    return _PINNED_TIMEOUT if route != "dns" else _DOWNLOAD_TIMEOUT


def _etag(resp):
    """The response's ETag, weak or strong alike."""
    return (resp.headers.get('ETag') or '').removeprefix('W/')


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
        # The server's ETag is the file's MD5, so a local copy with the same MD5 is answered with an empty 304
        headers = dict(self.headers)
        cached_path = os.path.join(download_path, unquote_plus(os.path.basename(urlparse(url).path)))
        try:
            with open(cached_path, "rb") as cached_file:
                headers["If-None-Match"] = f'"{hashlib.file_digest(cached_file, "md5").hexdigest()}"'
        except OSError:
            pass  # no readable local copy, so the file is downloaded in full

        # Server-picked routes (China only); None downloads as before
        plan = edge_routes.plan_for(url)
        host = urlparse(url).hostname
        first_route = plan["exploit"][0] if plan else "dns"
        failed_routes = set()

        # Connections are reused across this download's ranges, but never left idle for the next download
        session = create_http_session(pinned=first_route != "dns")
        try:
            req = None
            try:
                edge_routes.pin(host, first_route)
                try:
                    req = session.get(url, headers=headers, stream=True, timeout=_timeout(first_route))
                    req.raise_for_status()
                except requests.RequestException as e:
                    if first_route == "dns":
                        raise
                    # The picked route failed (e.g. a 403 from an IP not serving this site): use the normal route
                    print(f"Route {first_route} failed ({e.__class__.__name__}), using the normal route")
                    if req is not None:
                        req.close()
                        req = None
                    session.close()
                    session = create_http_session()
                    edge_routes.unpin()
                    failed_routes.add(first_route)
                    first_route = "dns"
                    req = session.get(url, headers=headers, stream=True, timeout=_DOWNLOAD_TIMEOUT)
                    req.raise_for_status()
            except Exception as e:
                if req is not None:
                    req.close()
                print(f"Error requesting download: {e}")
                if raise_errors:
                    raise
                return ""
            finally:
                edge_routes.unpin()

            edge_routes.saw(req.headers.get('X-Edge-Seen'))
            if req.status_code == 304:
                req.close()
                print(f"Already up to date: {os.path.basename(cached_path)}")
                self.downloaded_file_path = cached_path
                return cached_path

            # Headers of this first response decide the file name, the size, and how it is fetched.
            file_path = os.path.join(download_path, os.path.basename(self.find_download_fname(req)))
            total_size = int(req.headers.get('content-length', 0))
            # Byte offsets only line up with what gets written when the body arrives uncompressed
            supports_ranges = req.headers.get('accept-ranges', '').lower() == 'bytes' and not req.headers.get('content-encoding')

            # One worker per MB, capped, and only when the server claims to serve byte ranges
            if supports_ranges and total_size >= _PARALLEL_THRESHOLD:
                num_workers = min(total_size // (1024 * 1024), _MAX_WORKERS)
            else:
                num_workers = 1
            return self.download_queued(
                session, req, url, file_path, total_size, num_workers, raise_errors, atomic,
                resumable=supports_ranges and total_size > 0, plan=plan, first_route=first_route, failed_routes=failed_routes,
                etag=_etag(req)
            )
        finally:
            session.close()

    def download_queued(
        self, session, initial_req, url, file_path, total_size, num_workers, raise_errors=False, atomic=False,
        resumable=False, plan=None, first_route="dns", failed_routes=(), etag=""
    ):
        total_units = min(total_size // (1024 * 1024), _MAX_UNITS) if num_workers > 1 else 1
        print(f"[Queue] starting: {total_units} units, {num_workers} workers, {total_size / (1024 * 1024):.1f} MB", flush=True)

        # Starting route per worker: one explore worker (two from 6 MB), the rest spread over the main routes
        host = urlparse(url).hostname
        if plan and num_workers > 1:
            explore = plan["explore"][:2 if num_workers >= 6 else 1]
            routes = [("e", r) for r in explore]
            routes += [("x", plan["exploit"][i % len(plan["exploit"])]) for i in range(num_workers - len(explore))]
        else:
            routes = [("x", first_route)] * num_workers
        fallback = (plan["exploit"] if plan else []) + ["dns"]
        recorder = edge_routes.RouteRecorder() if plan else None
        started = time.monotonic()

        # Split the file into byte ranges workers pull from. A single stream is one range covering the whole file
        # when the server serves ranges, so a dropped connection resumes where it stopped instead of starting over.
        unit_queue = queue.Queue()
        if num_workers > 1:
            unit_size = total_size // total_units
            for i in range(total_units):
                start = i * unit_size
                end = (i + 1) * unit_size - 1 if i < total_units - 1 else total_size - 1
                unit_queue.put((i, start, end))
            if initial_req is not None:
                initial_req.close()  # its body is unused, every unit fetches its own range instead
                initial_req = None
        elif resumable:
            unit_queue.put((0, 0, total_size - 1))
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
        ranges_confirmed = [False]
        ranges_ignored = [False]
        MAX_FAILURES = total_units + 10
        stop_event = threading.Event()
        downloaded_lock = threading.Lock()
        range_ended = threading.Condition(downloaded_lock)  # wakes workers waiting at the end
        last_emit = [0.0]
        EMIT_INTERVAL = 0.05

        # The initial response is good for one read, so a retry has to ask for the file again
        first_stream = [initial_req]

        # Guarded by downloaded_lock
        route_failures = {}
        dead_routes = set(failed_routes)  # skips a route the first request failed on
        inflight = {}  # worker id -> {"left", "done", "started", "handoff": None or the taker's slot}

        def request_handoff(speed):
            """(range, slot) of the slowest range still running if this idle worker is at least twice as fast, else
            None. On a line that is slow everywhere nobody is, so ranges stay where they are."""
            now = time.monotonic()
            with downloaded_lock:
                slowest, slowest_eta = None, 0.0
                for info in inflight.values():
                    if info["handoff"] or info["left"] < _STEAL_MIN_BYTES or now - info["started"] < 1.0:
                        continue
                    rate = info["done"] / (now - info["started"])
                    eta = info["left"] / max(rate, 1.0)
                    if rate * 2 < speed and eta > slowest_eta:
                        slowest, slowest_eta = info, eta
                if slowest is None:
                    return None
                slowest["handoff"] = queue.Queue(maxsize=1)
                return slowest, slowest["handoff"]

        def take_over(target, slot):
            """The rest of target's range, put in slot at its next piece; None if it finished first."""
            while not stop_event.is_set():
                try:
                    return slot.get(timeout=0.5)
                except queue.Empty:
                    with downloaded_lock:
                        if not any(info is target for info in inflight.values()):
                            break
            try:
                return slot.get_nowait()  # handed over just before it finished
            except queue.Empty:
                return None

        def next_unit_at_end(speed):
            """Once the queue is empty: a unit a failure put back, or the rest of a range much slower than this
            worker. None once nothing is left running."""
            while not stop_event.is_set():
                try:
                    return unit_queue.get_nowait()
                except queue.Empty:
                    pass
                handoff = request_handoff(speed) if speed else None
                if handoff:
                    unit = take_over(*handoff)
                    if unit is not None:
                        return unit
                    continue
                with range_ended:
                    if not inflight and unit_queue.empty():
                        return None
                    range_ended.wait(0.5)  # ranges become worth taking over after a second
            return None

        def replacement(route):
            """First working main route other than route, else the normal route."""
            with downloaded_lock:
                return next((r for r in fallback if r not in dead_routes and r != route), "dns")

        def open_full_stream(session, route):
            resp = session.get(url, headers=self.headers, stream=True, timeout=_timeout(route))
            try:
                resp.raise_for_status()
            except Exception:
                resp.close()
                raise
            return resp

        def abandon_ranges():
            """Worth redoing the download without ranges only until a range response has shown they work.
            After that, a bad response is one failed attempt, retried like any other."""
            with downloaded_lock:
                if not ranges_confirmed[0]:
                    ranges_ignored[0] = True
                    stop_event.set()

        def emit_progress():
            self.progress.emit([(total_downloaded[0], total_size)])

        def discard_partial():
            try:
                os.remove(write_path)
            except FileNotFoundError:
                pass
            except OSError as e:
                print(f"[Queue] failed to remove partial file: {e}", flush=True)

        emit_progress()

        def worker(worker_id):
            kind, route = routes[worker_id]
            # Own session per worker, so pinned connections aren't shared across routes
            wsession = create_http_session(pinned=route != "dns")
            edge_routes.pin(host, route)
            f = open(write_path, 'r+b')
            measured = False  # an explore route gets at least one range, even in a small download
            speed = 0.0  # of this worker's last finished range
            try:
                # Workers race for units rather than owning a fixed share, so a slow one holds nobody up.
                # Each keeps its connection alive across units, so it stays warmed up instead of restarting slowly.
                while not stop_event.is_set():
                    with downloaded_lock:
                        route_dead = route in dead_routes
                    # Leave a dead route, or an explore route near the end, and carry on on a main route
                    if route_dead or (kind == "e" and measured and unit_queue.qsize() <= num_workers):
                        kind, route = "x", replacement(route if route_dead else None)
                        wsession.close()
                        wsession = create_http_session(pinned=route != "dns")
                        edge_routes.pin(host, route)
                    try:
                        unit_idx, start, end = unit_queue.get_nowait()
                    except queue.Empty:
                        # Nothing left: stay to take over slow ranges, so one slow connection or route can't
                        # hold the end of the download
                        unit = next_unit_at_end(speed)
                        if unit is None:
                            break
                        unit_idx, start, end = unit

                    bytes_this_unit = 0
                    retire = False
                    expected_bytes = (end - start + 1) if start is not None else total_size
                    resp = None
                    unit_started = time.monotonic()
                    colo, failed = "", False
                    try:
                        # A single stream's first attempt reads the response that started the download
                        if first_stream[0] is not None:
                            resp, first_stream[0] = first_stream[0], None
                        elif start is not None:
                            resp = wsession.get(url, headers={**self.headers, 'Range': f'bytes={start}-{end}'}, stream=True, timeout=_timeout(route))
                            resp.raise_for_status()
                            # 200 means the range was ignored and the whole file is coming
                            if resp.status_code != 206:
                                abandon_ranges()
                                raise Exception(f"expected 206 for range {start}-{end}, got {resp.status_code}")
                            # A status code is only a claim, so the bytes must be the ones asked for
                            content_range = resp.headers.get('Content-Range', '')
                            if content_range != f"bytes {start}-{end}/{total_size}":
                                abandon_ranges()
                                raise Exception(f"expected range {start}-{end}/{total_size}, got '{content_range}'")
                            # Edges can briefly hold different versions of a file, and mixing them would corrupt it
                            if etag and _etag(resp) not in ("", etag):
                                with downloaded_lock:
                                    ranges_ignored[0] = True
                                    stop_event.set()
                                raise Exception(f"range {start}-{end} came from another version of the file")
                            with downloaded_lock:
                                ranges_confirmed[0] = True
                        else:
                            resp = open_full_stream(wsession, route)

                        colo = (resp.headers.get('cf-ray') or '').rpartition('-')[2]
                        if recorder:
                            recorder.saw(resp.headers.get('X-Edge-Seen'))
                        with downloaded_lock:
                            info = inflight[worker_id] = {"left": expected_bytes, "done": 0, "started": time.monotonic(), "handoff": None}

                        # Every unit writes at its own offset, which is why the file is pre-allocated
                        f.seek(start if start is not None else 0)
                        for piece in resp.iter_content(chunk_size=_PIECE):
                            if not piece:
                                continue
                            # Hold each unit to its byte count, so nothing is ever written outside its slice
                            if expected_bytes and bytes_this_unit + len(piece) > expected_bytes:
                                if start is not None:
                                    abandon_ranges()
                                raise Exception(f"unit {unit_idx} overran its {expected_bytes} byte slice")
                            f.write(piece)
                            bytes_this_unit += len(piece)
                            with downloaded_lock:
                                total_downloaded[0] += len(piece)
                                info["left"] = expected_bytes - bytes_this_unit
                                info["done"] = bytes_this_unit
                                handoff_slot = info["handoff"]
                                now = time.monotonic()
                                if now - last_emit[0] >= EMIT_INTERVAL:  # throttle UI updates
                                    last_emit[0] = now
                                    emit_progress()
                            if handoff_slot is not None and start is not None and bytes_this_unit < expected_bytes:
                                raise _Handoff()

                        # A short slice would otherwise leave a silent hole in the file
                        if expected_bytes and bytes_this_unit != expected_bytes:
                            raise Exception(f"unit {unit_idx} got {bytes_this_unit} of {expected_bytes} bytes")
                        with downloaded_lock:
                            completed_units[0] += 1
                        speed = bytes_this_unit / max(time.monotonic() - unit_started, 0.001)

                    except _Handoff:
                        # Not a failure: the faster worker that asked takes the rest, and this one bows out
                        handoff_slot.put((unit_idx, start + bytes_this_unit, end))
                        retire = True

                    except Exception as e:
                        failed = True
                        with downloaded_lock:
                            if start is None:
                                total_downloaded[0] -= bytes_this_unit  # uncount it, the stream is redone from the start
                            # A picked route failing without data is dropped below, so it doesn't use up the download's tries
                            if route == "dns" or bytes_this_unit:
                                total_failures[0] += 1
                            last_error[0] = e
                            failures = total_failures[0]
                            # Drop a picked route after two failures with no data, an optional explore route after one
                            if route != "dns" and bytes_this_unit == 0:
                                route_failures[route] = route_failures.get(route, 0) + 1
                                if route_failures[route] >= (1 if kind == "e" else 2):
                                    dead_routes.add(route)
                        # A range resumes where it stopped, since the bytes it already wrote stay in place
                        if start is not None:
                            start += bytes_this_unit
                            if start > end:
                                with downloaded_lock:
                                    completed_units[0] += 1
                                continue
                        # Failures are pooled, so one hopeless unit cannot retry forever
                        if failures >= MAX_FAILURES:
                            stop_event.set()
                            print(f"[Queue] unit {unit_idx} failed, aborting ({failures}/{MAX_FAILURES} total failures): {e}", flush=True)
                        else:
                            unit_queue.put((unit_idx, start, end))
                            print(f"[Queue] unit {unit_idx} failed, re-enqueued ({failures}/{MAX_FAILURES} total failures): {e}", flush=True)
                    finally:
                        if resp is not None:
                            resp.close()
                        with range_ended:
                            inflight.pop(worker_id, None)
                            range_ended.notify_all()
                        measured = measured or bytes_this_unit > 0
                        if recorder:
                            recorder.add(route, kind, colo, bytes_this_unit, time.monotonic() - unit_started, failed)
                    if retire:
                        break
            finally:
                f.close()
                wsession.close()
                edge_routes.unpin()

        with ThreadPoolExecutor(max_workers=num_workers) as executor:
            for worker_id in range(num_workers):
                executor.submit(worker, worker_id)

        # Ranges weren't honoured or came from different versions of the file, so redo the whole thing in one stream
        if ranges_ignored[0]:
            print("[Queue] ranges unusable, retrying as a single stream", flush=True)
            discard_partial()
            return self.download_queued(
                session, None, url, file_path, total_size, 1, raise_errors, atomic,
                plan=plan, first_route=first_route, failed_routes=dead_routes, etag=etag
            )

        if stop_event.is_set() or completed_units[0] < total_units:
            print(f"[Queue] download failed: {total_failures[0]} total failures", flush=True)
            discard_partial()
            if raise_errors and last_error[0] is not None:
                raise last_error[0]
            return ""

        # All writers are closed here; make sure antivirus or another process did not
        # remove or truncate the completed file before it is handed to the caller.
        expected_size = total_size or total_downloaded[0]
        try:
            if expected_size <= 0:
                raise OSError("downloaded file is empty")
            if not os.path.isfile(write_path):
                raise FileNotFoundError("downloaded file is missing")
            actual_size = os.path.getsize(write_path)
            if actual_size != expected_size:
                raise OSError(f"downloaded file has {actual_size} of {expected_size} bytes")
        except Exception as e:
            print(f"[Queue] completed file verification failed: {e}", flush=True)
            discard_partial()
            return ""

        # Same-directory rename, so readers see either the old file or the new one
        if atomic:
            replaced = False
            try:
                os.replace(write_path, file_path)
                replaced = True
                if not os.path.isfile(file_path) or os.path.getsize(file_path) != expected_size:
                    raise OSError("replaced file is missing or incomplete")
            except Exception as e:
                print(f"[Queue] failed to replace downloaded file: {e}", flush=True)
                if replaced:
                    try:
                        os.remove(file_path)
                    except FileNotFoundError:
                        pass
                    except OSError as cleanup_error:
                        print(f"[Queue] failed to remove invalid file: {cleanup_error}", flush=True)
                else:
                    discard_partial()
                if raise_errors:
                    raise
                return ""

        emit_progress()
        if recorder:
            recorder.finish(total_size, time.monotonic() - started)
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

        # CloudFront sends no Content-Disposition and its signed paths are encoded with quote_plus
        return unquote_plus(urlparse(str(response.url)).path.split("/")[-1])

    @staticmethod
    def get_signed_download_url(file_path_on_s3, raise_errors=False):
        if not SIGNED_URL_DOWNLOAD_ENDPOINT or not CLIENT_API_KEY:
            print("Error: API endpoint or Client API Key is not configured.")
            return None

        params = {
            'filePath': file_path_on_s3
        }
        # Pending speed reports ride along, kept until the server accepts them
        report_batch, report_param = edge_routes.take_reports()
        if report_param:
            params['edgeReport'] = report_param
        region = edge_routes.region()
        if region:
            params['edgeRegion'] = region

        try:
            response = signed_get(SIGNED_URL_DOWNLOAD_ENDPOINT, params, API_TIMEOUT)
            response.raise_for_status()
            if report_batch:
                edge_routes.ack(report_batch)
                report_batch = None

            data = response.json()
            edge_routes.update(data.get('edges'))
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
        finally:
            if report_batch:
                edge_routes.restore(report_batch)
        return None

    @staticmethod
    def get_signed_upload_url(file_path_on_s3, metadata_json):
        if not SIGNED_URL_UPLOAD_ENDPOINT or not CLIENT_API_KEY:
            print("Error: API endpoint or Client API Key is not configured.")
            return None

        # add uniqueness to file
        file_path, file_ext = os.path.splitext(file_path_on_s3)
        file_path_on_s3 = os.path.join("trainers", f"{os.path.basename(file_path)}_{uuid.uuid4().hex}{file_ext}").replace("\\", "/")

        params = {
            'filePath': file_path_on_s3,
            'metadata': metadata_json
        }

        try:
            response = signed_get(SIGNED_URL_UPLOAD_ENDPOINT, params, API_TIMEOUT)
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

        # Fail without translation data, so a name is never returned untranslated
        if not translation_index.has_data():
            raise ValueError("translations.json is missing or empty in the database path.")

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
