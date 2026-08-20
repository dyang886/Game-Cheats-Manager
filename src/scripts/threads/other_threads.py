import mmap
import os
import psutil
import re
import shutil
import stat
import subprocess
import time
import traceback

from PyQt6.QtCore import QThread, pyqtSignal
import requests

from config import *
from threads.download_base_thread import DownloadBaseThread


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

            response = requests.get(signed_url, timeout=10)
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
            asar_copy = os.path.join(WEMOD_TEMP_DIR, "app.asar")
            asar_bak = os.path.join(self.selectedWeModPath, "resources", "app.asar.bak")

            weModExe_WeMod = os.path.join(self.selectedWeModPath, "WeMod.exe")  # ->  WeMod.exe latest is 11.6.0
            weModExe_Wand = os.path.join(self.selectedWeModPath, "Wand.exe")  # ->  Wand.exe newest is 12.0.3
            if os.path.exists(weModExe_Wand):
                weModExeName = "Wand.exe"
                weModExe = weModExe_Wand
            else:
                weModExeName = "WeMod.exe"
                weModExe = weModExe_WeMod

            # Terminate if WeMod is running
            if self.is_program_running(weModExeName):
                self.confirmClose.emit()  # blocks until the user answers
                if not self.close_confirmed:
                    self.message.emit(tr("Wand is currently running,\nplease close the application first"), "error")
                    self.finished.emit()
                    return

                if not self.close_program(weModExeName):
                    self.message.emit(tr("Could not close Wand,\nplease close the application manually"), "error")
                    self.finished.emit()
                    return

            # ===========================================================================
            # Unlock WeMod Pro
            if self.parent().weModProCheckbox.isChecked():
                patch_success = True

                try:
                    # 1. Remove asar integrity check
                    self.disable_asar_integrity(weModExe)

                    # 2. Patch app.asar
                    os.makedirs(WEMOD_TEMP_DIR, exist_ok=True)
                    if os.path.exists(asar_bak):
                        if os.path.exists(asar):
                            os.remove(asar)
                        os.rename(asar_bak, asar)
                    shutil.copyfile(asar, asar_copy)
                except Exception as e:
                    self.message.emit(tr("Failed to patch file:") + f"{str(e)}", "error")
                    self.finished.emit()
                    return

                # Extract app.asar file
                try:
                    command = [unzip_path, 'e', '-y', asar_copy, "app*bundle.js", "index.js", f"-o{WEMOD_TEMP_DIR}"]
                    subprocess.run(command, check=True, creationflags=subprocess.CREATE_NO_WINDOW)
                except Exception as e:
                    self.message.emit(tr("Failed to extract file:") + f"\n{asar_copy}", "error")
                    patch_success = False

                # Patching logic
                patch_success = self.patch()

                # pack patched js files back to app.asar
                try:
                    shutil.copyfile(asar, asar_bak)
                    command = [unzip_path, 'a', '-y', asar_copy, os.path.join(WEMOD_TEMP_DIR, '*.js')]
                    subprocess.run(command, check=True, creationflags=subprocess.CREATE_NO_WINDOW)
                    shutil.move(asar_copy, asar)
                except Exception as e:
                    self.message.emit(tr("Failed to patch file:") + f"\n{asar}", "error")
                    patch_success = False

                # Clean up
                shutil.rmtree(WEMOD_TEMP_DIR)
                if patch_success:
                    self.message.emit(tr("Wand Pro activated"), "success")
                else:
                    self.message.emit(tr("Failed to activate Wand Pro"), "error")

            else:
                if os.path.exists(asar_bak):
                    if os.path.exists(asar):
                        os.remove(asar)
                    os.rename(asar_bak, asar)

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

    def apply_patch(self, file_path, pattern, replacement):
        try:
            with open(file_path, 'r+', encoding='utf-8') as file:
                content = file.read()
                modified_content = re.sub(pattern, replacement, content)
                file.seek(0)
                file.write(modified_content)
                file.truncate()
        except Exception as e:
            self.message.emit(tr("Failed to patch file:") + f"\n{file_path}", "error")

    def disable_asar_integrity(self, exe_path):
        """
        Turn off Electron's asar integrity fuse so a patched app.asar loads.
        Electron embeds its fuses as [32-byte sentinel][version][fuse count][N fuse chars], each char "0" or "1
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

    def load_patterns(self, enable_dev):
        if not PATCH_PATTERNS_ENDPOINT or not CLIENT_API_KEY:
            print("Error: patch-patterns endpoint or API key is not configured.")
            return None

        params = {
            'patchMethod': self.patchMethod,
            'enableDev': 'true' if enable_dev else 'false'
        }
        response = None
        try:
            response = signed_get(PATCH_PATTERNS_ENDPOINT, params, API_TIMEOUT)
            response.raise_for_status()
            return response.json()
        except Exception as e:
            print(f"Error fetching patch patterns: {str(e)}")
            status_code = response.status_code if response is not None else -1
            self.message.emit(tr("Internet request failed.") + f" {status_code}", "error")
            return None

    def patch(self, enable_dev=False):
        entries = self.load_patterns(enable_dev)
        if not entries:
            print(f"No patch patterns for method '{self.patchMethod}'.")
            return False

        required_ok = True
        applied = False

        try:
            for i, entry in enumerate(entries):
                pattern, replacement = entry["pattern"], entry["replacement"]

                target = None
                for filename in os.listdir(WEMOD_TEMP_DIR):
                    if not filename.endswith('.js'):
                        continue
                    file_path = os.path.join(WEMOD_TEMP_DIR, filename)
                    try:
                        with open(file_path, 'r', encoding='utf-8') as file:
                            content = file.read()
                    except UnicodeDecodeError:
                        continue
                    if re.search(pattern, content):
                        target = file_path
                        break

                if target:
                    self.apply_patch(target, pattern, replacement)
                    applied = True
                    print(f"[{self.patchMethod}] patched entry {i} in {os.path.basename(target)}")
                else:
                    print(f"[{self.patchMethod}] entry {i} not found" + (" (required)" if entry.get("required") else ""))
                    if entry.get("required"):
                        required_ok = False
        except Exception as e:
            print(f"Error during {self.patchMethod} patching: {str(e)}")
            return False

        return required_ok and applied
