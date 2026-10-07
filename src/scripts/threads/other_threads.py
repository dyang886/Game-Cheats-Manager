import mmap
import os
import psutil
import re
import shutil
import stat
import subprocess
import tempfile
import time
import traceback

from PyQt6.QtCore import QThread, pyqtSignal
import requests

from config import *
from threads.download_base_thread import DownloadBaseThread, http_session


class AnnouncementFetchWorker(QThread):
    announcementFetched = pyqtSignal(dict)
    fetchFailed = pyqtSignal()

    def run(self):
        try:
            signed_url = DownloadBaseThread.get_signed_download_url("GCM/Data/announcement.json")
            if not signed_url:
                print("Error: Failed to get signed URL for announcement.json")
                self.fetchFailed.emit()
                return

            response = http_session.get(signed_url, timeout=10)
            response.raise_for_status()
            data = response.json()

            announcements = data.get("announcements", [])
            if not announcements:
                return

            # Find the newest announcement (last in the list)
            newest = announcements[-1]
            announcement_id = newest.get("id")

            if announcement_id and announcement_id != settings.get("lastSeenAnnouncementId"):
                self.announcementFetched.emit(newest)

        except Exception:
            traceback.print_exc()
            self.fetchFailed.emit()


class UpdateWorker(DownloadBaseThread):
    message = pyqtSignal(str, str)
    update = pyqtSignal(str, str, str)
    urlFetched = pyqtSignal(str)
    finished = pyqtSignal(str)

    def __init__(self, s3_path, parent=None):
        super().__init__(parent)
        self.s3_path = s3_path

    def run(self):
        statusWidgetName = "appUpdate"
        update_failed = tr("Failed to update application")
        try:
            self.message.emit(statusWidgetName, tr("Preparing update"))
            signed_url = self.get_signed_download_url(self.s3_path)
            if signed_url:
                self.urlFetched.emit(signed_url)
            else:
                self.update.emit(statusWidgetName, update_failed, "error")
                time.sleep(2)

        except Exception:
            traceback.print_exc()
            self.update.emit(statusWidgetName, update_failed, "error")
            time.sleep(2)

        self.finished.emit(statusWidgetName)


class VersionFetchWorker(QThread):
    versionFetched = pyqtSignal(str)
    fetchFailed = pyqtSignal()

    def __init__(self, app_name, parent=None):
        super().__init__(parent)
        self.app_name = app_name

    def run(self):
        if not VERSION_CHECKER_ENDPOINT or not CLIENT_API_KEY:
            print("Error: API endpoint or Client API Key is not configured.")
            self.fetchFailed.emit()
            return

        params = {
            'appName': self.app_name
        }

        try:
            response = signed_get(VERSION_CHECKER_ENDPOINT, params, API_TIMEOUT)
            response.raise_for_status()

            data = response.json()
            latest_version = data.get('latest_version')
            if latest_version and parse_version(latest_version) is not None:
                self.versionFetched.emit(latest_version.strip())
            else:
                print(f"Error: Missing or invalid 'latest_version' in response. Response: {data}")
                self.fetchFailed.emit()

        except Exception:
            traceback.print_exc()
            self.fetchFailed.emit()


class PathChangeThread(QThread):
    finished = pyqtSignal(str)
    error = pyqtSignal(str)

    def __init__(self, source_path, destination_path, parent=None):
        super().__init__(parent)
        self.source_path = source_path
        self.destination_path = destination_path

    def run(self):
        try:
            if not os.path.exists(self.destination_path):
                os.makedirs(self.destination_path)

            for filename in os.listdir(self.source_path):
                src_file = os.path.join(self.source_path, filename)
                dst_file = os.path.join(self.destination_path, filename)
                os.chmod(src_file, stat.S_IWRITE)
                if os.path.exists(dst_file):
                    os.chmod(self.destination_path, stat.S_IWRITE)
                shutil.move(src_file, dst_file)
            shutil.rmtree(self.source_path)

            self.finished.emit(self.destination_path)

        except Exception as e:
            traceback.print_exc()
            self.error.emit(str(e))


class FetchGCMData(DownloadBaseThread):
    message = pyqtSignal(str, str)
    update = pyqtSignal(str, str, str)
    finished = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)

    def run(self):
        statusWidgetName = "gcm"
        update_failed = tr("Update from GCM failed")
        try:
            self.message.emit(statusWidgetName, tr("Updating data from GCM"))
            url = "GCM/Data/gcm_trainers.json"
            signed_url = self.get_signed_download_url(url)
            file_path = signed_url and self.request_download(signed_url, DATABASE_PATH, atomic=True)
            if not file_path:
                self.update.emit(statusWidgetName, update_failed, "error")
                time.sleep(2)

        except Exception:
            traceback.print_exc()
            self.update.emit(statusWidgetName, update_failed, "error")
            time.sleep(2)

        self.finished.emit(statusWidgetName)


class FetchFlingData(DownloadBaseThread):
    message = pyqtSignal(str, str)
    update = pyqtSignal(str, str, str)
    finished = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)

    def run(self):
        statusWidgetName = "fling"
        update_failed = tr("Update from FLiNG failed")
        try:
            update_message1 = tr("Updating data from FLiNG") + " (1/2)"
            update_failed1 = update_failed + " (1/2)"
            update_message2 = tr("Updating data from FLiNG") + " (2/2)"
            update_failed2 = update_failed + " (2/2)"

            self.message.emit(statusWidgetName, update_message1)
            url = "GCM/Data/fling_archive.json"
            signed_url = self.get_signed_download_url(url)
            file_path = signed_url and self.request_download(signed_url, DATABASE_PATH, atomic=True)
            if not file_path:
                self.update.emit(statusWidgetName, update_failed1, "error")
                time.sleep(2)

            self.update.emit(statusWidgetName, update_message2, "load")
            url = "GCM/Data/fling_main.json"
            signed_url = self.get_signed_download_url(url)
            file_path = signed_url and self.request_download(signed_url, DATABASE_PATH, atomic=True)
            if not file_path:
                self.update.emit(statusWidgetName, update_failed2, "error")
                time.sleep(2)

        except Exception:
            traceback.print_exc()
            self.update.emit(statusWidgetName, update_failed, "error")
            time.sleep(2)

        self.finished.emit(statusWidgetName)


class FetchXiaoXingData(DownloadBaseThread):
    message = pyqtSignal(str, str)
    update = pyqtSignal(str, str, str)
    finished = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)

    def run(self):
        statusWidgetName = "xiaoxing"
        update_failed = tr("Update from XiaoXing failed")
        try:
            self.message.emit(statusWidgetName, tr("Updating data from XiaoXing"))
            url = "GCM/Data/xiaoxing.json"
            signed_url = self.get_signed_download_url(url)
            file_path = signed_url and self.request_download(signed_url, DATABASE_PATH, atomic=True)
            if not file_path:
                self.update.emit(statusWidgetName, update_failed, "error")
                time.sleep(2)

        except Exception:
            traceback.print_exc()
            self.update.emit(statusWidgetName, update_failed, "error")
            time.sleep(2)

        self.finished.emit(statusWidgetName)


class FetchCTData(DownloadBaseThread):
    message = pyqtSignal(str, str)
    update = pyqtSignal(str, str, str)
    finished = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)

    def run(self):
        statusWidgetName = "ct"
        update_failed = tr("Update from CT failed")
        try:
            self.message.emit(statusWidgetName, tr("Updating data from CT"))
            url = "GCM/Data/cheat_table.json"
            signed_url = self.get_signed_download_url(url)
            file_path = signed_url and self.request_download(signed_url, DATABASE_PATH, atomic=True)
            if not file_path:
                self.update.emit(statusWidgetName, update_failed, "error")
                time.sleep(2)

        except Exception:
            traceback.print_exc()
            self.update.emit(statusWidgetName, update_failed, "error")
            time.sleep(2)

        self.finished.emit(statusWidgetName)


class FetchTrainerTranslations(DownloadBaseThread):
    message = pyqtSignal(str, str)
    update = pyqtSignal(str, str, str)
    finished = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)

    def run(self):
        statusWidgetName = "translations"
        fetch_error = tr("Fetch trainer translations failed")
        try:
            self.message.emit(statusWidgetName, tr("Fetching trainer translations"))
            url = "GCM/Data/translations.json"
            signed_url = self.get_signed_download_url(url)
            file_path = signed_url and self.request_download(signed_url, DATABASE_PATH, atomic=True)
            if not file_path:
                self.update.emit(statusWidgetName, fetch_error, "error")
                time.sleep(2)

        except Exception:
            traceback.print_exc()
            self.update.emit(statusWidgetName, fetch_error, "error")
            time.sleep(2)

        self.finished.emit(statusWidgetName)


class TrainerUploadWorker(QThread):
    finished = pyqtSignal(bool, str)
    progress = pyqtSignal(int)

    def __init__(self, file_path, trainer_name, contact_info, trainer_source, notes):
        super().__init__()
        self.file_path = file_path
        self.trainer_name = trainer_name
        self.contact_info = contact_info
        self.trainer_source = trainer_source
        self.notes = notes
        self._is_cancelled = False

    def stop(self):
        self._is_cancelled = True

    def run(self):
        try:
            meta_dict = {
                'uploader-contact': self.contact_info,
                'trainer-name': self.trainer_name,
                'trainer-source': self.trainer_source,
                'notes': self.notes
            }
            meta_json = json.dumps(meta_dict)

            upload_data = DownloadBaseThread.get_signed_upload_url(self.file_path, meta_json)
            if not upload_data:
                self.finished.emit(False, tr("Failed to retrieve upload authorization."))
                return

            if self._is_cancelled:
                return
            upload_url = upload_data.get('uploadUrl')
            required_headers = upload_data.get('requiredHeaders', {})
            if not upload_url:
                self.finished.emit(False, tr("Failed to retrieve upload url."))
                return

            class ProgressFileObject:
                def __init__(self, filepath, callback, cancel_check):
                    self.file = open(filepath, 'rb')
                    self.len = os.path.getsize(filepath)
                    self.read_so_far = 0
                    self.callback = callback
                    self.cancel_check = cancel_check

                def read(self, size=-1):
                    # Periodically check for cancellation
                    if self.cancel_check():
                        raise Exception("Upload cancelled by user.")

                    data = self.file.read(size)
                    self.read_so_far += len(data)
                    if self.len > 0:
                        percent = int(self.read_so_far * 100 / self.len)
                        self.callback(percent)
                    return data

                def close(self):
                    self.file.close()

                def __len__(self):
                    return self.len

            wrapped_file = ProgressFileObject(self.file_path, self.progress.emit, lambda: self._is_cancelled)

            # Send the PUT request
            response = requests.put(upload_url, data=wrapped_file, headers=required_headers, timeout=300)
            wrapped_file.close()
            response.raise_for_status()
            self.finished.emit(True, tr("Upload successful! Thank you for your contribution.\nYour trainer will be available for download after passing a manual review."))

        except Exception as e:
            traceback.print_exc()
            if not self._is_cancelled:
                self.finished.emit(False, tr("Upload failed: ") + str(e))


class WeModCustomization(QThread):
    # Latest WeMod download: https://api.wemod.com/client/download
    # Custom WeMod version download: https://storage-cdn.wemod.com/app/releases/stable/WeMod-11.6.0.exe
    # Custom Wand version download: https://storage-cdn.wemod.com/app/releases/stable/Wand-12.0.3.exe
    message = pyqtSignal(str, str)
    confirmClose = pyqtSignal()
    finished = pyqtSignal()

    PATCH_SCHEMA_VERSION = 2
    NATIVE_HELPER_PATH = "resources/app.asar.unpacked/static/unpacked/auxiliary/WandAuxiliaryService.exe"

    def __init__(self, weModVersions, weModInstallPath, selectedWeModVersion, patchMethod, parent=None):
        super().__init__(parent)
        self.close_confirmed = False  # set by the main thread while `confirmClose` blocks
        self.weModVersions = weModVersions
        self.weModInstallPath = weModInstallPath
        self.selectedWeModVersion = selectedWeModVersion
        self.selectedWeModPath = os.path.join(weModInstallPath, f"app-{selectedWeModVersion}")
        self.patchMethod = patchMethod

    def run(self):
        try:
            asar = os.path.join(self.selectedWeModPath, "resources", "app.asar")
            weModExeName = "Wand.exe" if os.path.exists(os.path.join(self.selectedWeModPath, "Wand.exe")) else "WeMod.exe"
            weModExe = os.path.join(self.selectedWeModPath, weModExeName)

            # Terminate if WeMod is running
            if self.is_program_running(weModExeName):
                self.confirmClose.emit()  # blocks until the user answers
                if not self.close_confirmed:
                    self.message.emit(tr("Wand is currently running,\nplease close the application first"), "error")
                    return

                if not self.close_program(weModExeName):
                    self.message.emit(tr("Could not close Wand,\nplease close the application manually"), "error")
                    return

            # ===========================================================================
            # Unlock WeMod Pro
            if self.parent().weModProCheckbox.isChecked():
                self.patch(asar, weModExe)
                self.message.emit(tr("Wand Pro activated"), "success")

            else:
                native_helper = os.path.join(self.selectedWeModPath, *self.NATIVE_HELPER_PATH.split("/"))
                self.restore_files([asar, weModExe, native_helper])
                self.message.emit(tr("Wand Pro disabled"), "success")

            # ===========================================================================
            # Disable auto update
            updateExe = os.path.join(self.weModInstallPath, "Update.exe")
            updateExe_backup = os.path.join(self.weModInstallPath, "Update.exe.bak")
            try:
                if self.parent().disableUpdateCheckbox.isChecked():
                    if os.path.exists(updateExe):
                        os.rename(updateExe, updateExe_backup)
                        self.message.emit(tr("Wand auto update disabled"), "success")
                else:
                    if os.path.exists(updateExe_backup):
                        os.rename(updateExe_backup, updateExe)
                        self.message.emit(tr("Wand auto update enabled"), "success")
                    elif not os.path.exists(updateExe):
                        self.message.emit(tr("Failed to enable Wand auto update,\nplease try reinstalling Wand"), "error")
            except Exception as e:
                self.message.emit(tr("Failed to process Wand update file:") + f"\n{str(e)}", "error")

            # ===========================================================================
            # Delete other version folders
            if self.parent().delOtherVersionsCheckbox.isChecked():
                for version in self.weModVersions:
                    if version != self.selectedWeModVersion:
                        folder_path = os.path.join(self.weModInstallPath, f"app-{version}")
                        try:
                            shutil.rmtree(folder_path)
                            self.message.emit(tr("Deleted Wand version: ") + version, "success")
                        except Exception as e:
                            self.message.emit(tr("Failed to delete Wand version: ") + version, "error")

        except Exception as e:
            traceback.print_exc()
            self.message.emit(tr("Failed to patch file:") + f"\n{str(e)}", "error")
        finally:
            self.finished.emit()

    def is_program_running(self, program_name):
        for proc in psutil.process_iter():
            try:
                if program_name == proc.name():
                    return True
            except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
                pass
        return False

    def close_program(self, program_name, timeout=10):
        for proc in psutil.process_iter():
            try:
                if program_name == proc.name():
                    proc.kill()
            except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
                pass

        deadline = time.time() + timeout
        while time.time() < deadline:
            if not self.is_program_running(program_name):
                return True
            time.sleep(0.2)

        return False

    def load_patterns(self, enable_dev):
        if not PATCH_PATTERNS_ENDPOINT or not CLIENT_API_KEY:
            raise RuntimeError("Patch-patterns endpoint or API key is not configured")

        params = {
            'patchMethod': self.patchMethod,
            'enableDev': 'true' if enable_dev else 'false',
            'schemaVersion': str(self.PATCH_SCHEMA_VERSION),
            'appVersion': self.selectedWeModVersion,
        }
        response = None
        try:
            response = signed_get(PATCH_PATTERNS_ENDPOINT, params, API_TIMEOUT)
            response.raise_for_status()
        except Exception as e:
            status_code = response.status_code if response is not None else -1
            raise RuntimeError(tr("Internet request failed.") + f" {status_code}") from e

        patterns = response.json()
        if not isinstance(patterns, dict) or patterns.get('schemaVersion') != self.PATCH_SCHEMA_VERSION:
            raise ValueError("Unsupported patch schema; update the patch-patterns service")
        if not isinstance(patterns.get('javascript'), list) or not isinstance(patterns.get('native'), list):
            raise ValueError("Patch schema requires javascript and native lists")
        return patterns

    def patch(self, asar, exe_path, enable_dev=False):
        """Stage and validate all patches before installing them."""
        patterns = self.load_patterns(enable_dev)
        native_targets = [entry['file'] for entry in patterns['native']]
        if len(native_targets) != len(set(native_targets)):
            raise ValueError("Native patch entries must target distinct files")

        os.makedirs(WEMOD_TEMP_DIR, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="patch-", dir=WEMOD_TEMP_DIR) as work_dir:
            files = [self.patch_native(entry, work_dir) for entry in patterns['native']]

            asar_copy = os.path.join(work_dir, "app.asar")
            shutil.copyfile(self.original_file(asar), asar_copy)
            command = [unzip_path, 'e', '-y', asar_copy, "app*bundle.js", "index.js", f"-o{work_dir}"]
            subprocess.run(command, check=True, creationflags=subprocess.CREATE_NO_WINDOW)
            self.patch_javascript(patterns['javascript'], work_dir)
            command = [unzip_path, 'a', '-y', asar_copy, os.path.join(work_dir, '*.js')]
            subprocess.run(command, check=True, creationflags=subprocess.CREATE_NO_WINDOW)

            exe_copy = os.path.join(work_dir, os.path.basename(exe_path))
            shutil.copyfile(self.original_file(exe_path), exe_copy)
            if self.disable_asar_integrity(exe_copy):
                files.append((exe_path, exe_copy))
            files.append((asar, asar_copy))
            self.install_files(files)

    def patch_javascript(self, entries, work_dir):
        contents = {}
        for filename in sorted(os.listdir(work_dir)):
            if filename.endswith('.js'):
                with open(os.path.join(work_dir, filename), encoding='utf-8') as file:
                    contents[filename] = file.read()

        changed = set()
        for index, entry in enumerate(entries):
            for filename, content in contents.items():
                patched, count = re.subn(entry['pattern'], entry['replacement'], content)
                if count:
                    contents[filename] = patched
                    changed.add(filename)
                    print(f"[{self.patchMethod}] patched JavaScript entry {index} in {filename}")
                    break
            else:
                if entry.get('required'):
                    raise ValueError(f"Required JavaScript patch {index} did not match")
                print(f"[{self.patchMethod}] optional JavaScript entry {index} not found")

        if not changed:
            raise ValueError("No JavaScript patches matched")
        for filename in sorted(changed):
            with open(os.path.join(work_dir, filename), 'w', encoding='utf-8') as file:
                file.write(contents[filename])

    def patch_native(self, entry, work_dir):
        # Native entries are always required and only target the auxiliary helper.
        if entry['file'] != self.NATIVE_HELPER_PATH:
            raise ValueError("Unsupported native patch target")
        target = os.path.join(self.selectedWeModPath, *self.NATIVE_HELPER_PATH.split('/'))
        with open(self.original_file(target), 'rb') as file:
            content = file.read()
        pattern = bytes.fromhex(entry['pattern'])
        replacement = bytes.fromhex(entry['replacement'])
        if not pattern or len(pattern) != len(replacement) or content.count(pattern) != 1:
            raise ValueError("Native patch must match exactly once without changing file size")
        staged = os.path.join(work_dir, os.path.basename(target))
        with open(staged, 'wb') as file:
            file.write(content.replace(pattern, replacement, 1))
        return target, staged

    def disable_asar_integrity(self, exe_path):
        """
        Turn off Electron's asar integrity fuse so a patched app.asar loads.
        Electron embeds its fuses as [32-byte sentinel][version][fuse count][N fuse chars], each char "0" or "1".
        """
        FUSE_SENTINEL = b"dL7pKGdnNz796PbbjQWNKmHXBZaB9tsX"
        FUSE_ASAR_INTEGRITY_INDEX = 4  # EnableEmbeddedAsarIntegrityValidation in the v1 fuse order
        FUSE_DISABLED = ord("0")

        with open(exe_path, 'r+b') as file:
            with mmap.mmap(file.fileno(), 0) as mapped:
                sentinel = mapped.find(FUSE_SENTINEL)
                if sentinel == -1:
                    raise Exception("Electron fuse sentinel not found")

                wire_start = sentinel + len(FUSE_SENTINEL) + 2
                wire_length = mapped[sentinel + len(FUSE_SENTINEL) + 1]
                if wire_length <= FUSE_ASAR_INTEGRITY_INDEX:
                    raise Exception(f"Electron fuse wire holds only {wire_length} fuses")

                position = wire_start + FUSE_ASAR_INTEGRITY_INDEX
                if mapped[position] == FUSE_DISABLED:
                    print("Asar integrity validation is already disabled")
                    return False

                mapped[position] = FUSE_DISABLED
                print("Disabled asar integrity validation")
                return True

    @staticmethod
    def original_file(path):
        return path + ".bak" if os.path.exists(path + ".bak") else path

    @staticmethod
    def install_files(files, *, create_backups=True):
        """Back up every destination first, then install the staged files."""
        if create_backups:
            for destination, _ in files:
                backup = destination + ".bak"
                if os.path.exists(backup):
                    continue
                try:
                    shutil.copyfile(destination, backup)
                except Exception:
                    if os.path.exists(backup):
                        os.remove(backup)
                    raise

        for destination, source in files:
            shutil.copyfile(source, destination)

    @classmethod
    def restore_files(cls, paths):
        files = [(path, path + ".bak") for path in paths if os.path.exists(path + ".bak")]
        cls.install_files(files, create_backups=False)
        for _, backup in files:
            os.remove(backup)
