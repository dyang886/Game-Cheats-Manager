import os
import psutil
import re
import shutil
import stat
import subprocess
import time

from PyQt6.QtCore import QThread, pyqtSignal
import requests

from config import *
from threads.download_base_thread import DownloadBaseThread


class VersionFetchWorker(QThread):
    versionFetched = pyqtSignal(str)
    fetchFailed = pyqtSignal()

    def __init__(self, app_name, parent=None):
        super().__init__(parent)
        self.app_name = app_name

    def run(self):
        if not VERSION_CHECKER_API_GATEWAY_ENDPOINT or not CLIENT_API_KEY:
            print("Error: API Gateway endpoint or Client API Key is not configured.")
            self.fetchFailed.emit()

        headers = {
            'x-api-key': CLIENT_API_KEY
        }
        params = {
            'appName': self.app_name
        }

        try:
            response = requests.get(VERSION_CHECKER_API_GATEWAY_ENDPOINT, headers=headers, params=params, timeout=15)
            response.raise_for_status()

            data = response.json()
            latest_version = data.get('latest_version')
            if latest_version:
                self.versionFetched.emit(latest_version)
            else:
                print(f"Error: 'latest_version' not found in response. Response: {data}")
                self.fetchFailed.emit()

        except Exception as e:
            print(f"Error retrieving latest version: {str(e)}")
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
            self.error.emit(str(e))


class FetchFlingSite(DownloadBaseThread):
    message = pyqtSignal(str, str)
    update = pyqtSignal(str, str, str)
    finished = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)

    def run(self):
        statusWidgetName = "fling"
        update_message1 = tr("Updating data from FLiNG") + " (1/2)"
        update_failed1 = tr("Update from FLiGN failed") + " (1/2)"
        update_message2 = tr("Updating data from FLiNG") + " (2/2)"
        update_failed2 = tr("Update from FLiGN failed") + " (2/2)"

        # Fling archive
        self.message.emit(statusWidgetName, update_message1)
        if settings['flingDownloadServer'] == "official":
            url = "https://archive.flingtrainer.com/"
            content = self.get_webpage_content(url)
            if not content:
                self.update.emit(statusWidgetName, update_failed1, "error")
                time.sleep(2)
            else:
                self.save_html_content(content, "fling_archive.html")

        elif settings['flingDownloadServer'] == "gcm":
            url = "GCM/Data/fling_archive.json"
            signed_url = self.get_signed_download_url(url)
            file_path = self.request_download(signed_url, DATABASE_PATH)
            if not file_path:
                self.update.emit(statusWidgetName, update_failed1, "error")
                time.sleep(2)

        # Fling main
        self.update.emit(statusWidgetName, update_message2, "load")
        if settings['flingDownloadServer'] == "official":
            url = "https://flingtrainer.com/all-trainers-a-z/"
            content = self.get_webpage_content(url)
            if not content:
                self.update.emit(statusWidgetName, update_failed2, "error")
                time.sleep(2)
            else:
                self.save_html_content(content, "fling_main.html")

        elif settings['flingDownloadServer'] == "gcm":
            url = "GCM/Data/fling_main.json"
            signed_url = self.get_signed_download_url(url)
            file_path = self.request_download(signed_url, DATABASE_PATH)
            if not file_path:
                self.update.emit(statusWidgetName, update_failed2, "error")
                time.sleep(2)

        self.finished.emit(statusWidgetName)


class FetchXiaoXingSite(DownloadBaseThread):
    message = pyqtSignal(str, str)
    update = pyqtSignal(str, str, str)
    finished = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)

    def run(self):
        statusWidgetName = "xiaoxing"
        update_message = tr("Updating data from XiaoXing")
        update_failed = tr("Update from XiaoXing failed")

        self.message.emit(statusWidgetName, update_message)
        url = "GCM/Data/xiaoxing.json"
        signed_url = self.get_signed_download_url(url)
        file_path = self.request_download(signed_url, DATABASE_PATH)
        if not file_path:
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
        fetch_message = tr("Fetching trainer translations")
        fetch_error = tr("Fetch trainer translations failed")

        self.message.emit(statusWidgetName, fetch_message)
        url = "GCM/Data/translations.json"
        signed_url = self.get_signed_download_url(url)
        file_path = self.request_download(signed_url, DATABASE_PATH)
        if not file_path:
            self.update.emit(statusWidgetName, fetch_error, "error")
            time.sleep(2)

        self.finished.emit(statusWidgetName)


class WeModCustomization(QThread):
    # Latest WeMod download: https://api.wemod.com/client/download
    # Custom WeMod version download: https://storage-cdn.wemod.com/app/releases/stable/WeMod-9.10.3.exe
    message = pyqtSignal(str, str)
    finished = pyqtSignal()

    def __init__(self, weModVersions, weModInstallPath, selectedWeModVersion, parent=None):
        super().__init__(parent)
        self.weModVersions = weModVersions
        self.weModInstallPath = weModInstallPath
        self.selectedWeModVersion = selectedWeModVersion
        self.selectedWeModPath = os.path.join(weModInstallPath, f"app-{selectedWeModVersion}")

    def run(self):
        asar = os.path.join(self.selectedWeModPath, "resources", "app.asar")
        asar_copy = os.path.join(WEMOD_TEMP_DIR, "app.asar")
        asar_bak = os.path.join(self.selectedWeModPath, "resources", "app.asar.bak")
        weModExe = os.path.join(self.selectedWeModPath, "WeMod.exe")
        weModExe_bak = os.path.join(self.selectedWeModPath, "WeMod.exe.bak")

        # Terminate if WeMod is running
        if self.is_program_running("WeMod.exe"):
            self.message.emit(tr("WeMod is currently running,\nplease close the application first"), "error")
            self.finished.emit()
            return

        # ===========================================================================
        # Unlock WeMod Pro
        if self.parent().weModProCheckbox.isChecked():
            patch_success = True

            # 1. Remove asar integrity check
            shutil.copyfile(weModExe, weModExe_bak)
            self.replace_hex_in_file(weModExe_bak, weModExe, '00001101', '00000101')
            os.remove(weModExe_bak)

            # 2. Patch app.asar
            os.makedirs(WEMOD_TEMP_DIR, exist_ok=True)
            if os.path.exists(asar_bak):
                if os.path.exists(asar):
                    os.remove(asar)
                os.rename(asar_bak, asar)
            shutil.copyfile(asar, asar_copy)

            # Extract app.asar file
            try:
                command = [unzip_path, 'e', '-y', asar_copy, "app*bundle.js", "index.js", f"-o{WEMOD_TEMP_DIR}"]
                subprocess.run(command, check=True, creationflags=subprocess.CREATE_NO_WINDOW)
            except Exception as e:
                self.message.emit(tr("Failed to extract file:") + f"\n{asar_copy}", "error")
                patch_success = False

            # Patching logic
            patch_success = self.yearly_active_sub()
            if not patch_success:
                patch_success = self.gifted_sub()

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
                self.message.emit(tr("WeMod Pro activated"), "success")
            else:
                self.message.emit(tr("Failed to activate WeMod Pro"), "error")

        else:
            if os.path.exists(asar_bak):
                if os.path.exists(asar):
                    os.remove(asar)
                os.rename(asar_bak, asar)

            self.message.emit(tr("WeMod Pro disabled"), "success")

        # ===========================================================================
        # Disable auto update
        updateExe = os.path.join(self.weModInstallPath, "Update.exe")
        updateExe_backup = os.path.join(self.weModInstallPath, "Update.exe.bak")
        try:
            if self.parent().disableUpdateCheckbox.isChecked():
                if os.path.exists(updateExe):
                    os.rename(updateExe, updateExe_backup)
                    self.message.emit(tr("WeMod auto update disabled"), "success")
            else:
                if os.path.exists(updateExe_backup):
                    os.rename(updateExe_backup, updateExe)
                    self.message.emit(tr("WeMod auto update enabled"), "success")
                elif not os.path.exists(updateExe):
                    self.message.emit(tr("Failed to enable WeMod auto update,\nplease try reinstalling WeMod"), "error")
        except Exception as e:
            self.message.emit(tr("Failed to process WeMod update file:") + f"\n{str(e)}", "error")

        # ===========================================================================
        # Delete other version folders
        if self.parent().delOtherVersionsCheckbox.isChecked():
            for version in self.weModVersions:
                if version != self.selectedWeModVersion:
                    folder_path = os.path.join(self.weModInstallPath, f"app-{version}")
                    try:
                        shutil.rmtree(folder_path)
                        self.message.emit(tr("Deleted WeMod version: ") + version, "success")
                    except Exception as e:
                        self.message.emit(tr("Failed to delete WeMod version: ") + version, "error")

        self.finished.emit()

    def is_program_running(self, program_name):
        for proc in psutil.process_iter():
            try:
                if program_name == proc.name():
                    return True
            except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
                pass
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

    def replace_hex_in_file(self, input_file, output_file, search_hex, replace_hex):
        try:
            command = [binmay_path, '-i', input_file, '-o', output_file, '-s', f"t:{search_hex}", '-r', f"t:{replace_hex}"]
            subprocess.run(command, check=True, creationflags=subprocess.CREATE_NO_WINDOW)
        except Exception as e:
            self.message.emit(tr("Failed to patch file:") + f"\n{input_file}", "error")

    def yearly_active_sub(self):
        patterns = {
            r'(getUserAccount\()(.*)(}async getUserAccountFlags)': r'\1\2.then(function(response) {response.subscription={period:"yearly", state:"active"}; response.flags=78; return response;})\3',
            r'(getUserAccountFlags\()(.*)(\)\).flags)': r'\1\2\3.then(function(response) {if(response.mask==4) {response.flags=4}; return response;})',
            r'(changeAccountEmail\()(.*)(email:.?,currentPassword:.?}\))': r'\1\2\3.then(function(response) {response.subscription={period:"yearly", state:"active"}; response.flags=78; return response;})',
            r'(getPromotion\()(.*)(collectMetrics:!\d}\))': r'\1\2\3.then(function(response) {response.components.appBanner=null; response.flags=0; return response;})'
        }

        # Mapping of patterns to files where they were found: {pattern key: file path}
        lines = {key: None for key in patterns}

        # Check files for matching patterns
        for pattern in patterns:
            for filename in os.listdir(WEMOD_TEMP_DIR):
                if filename.endswith('.js'):
                    file_path = os.path.join(WEMOD_TEMP_DIR, filename)
                    try:
                        with open(file_path, 'r', encoding='utf-8') as file:
                            content = file.read()
                            if re.search(pattern, content):
                                print(f"Pattern {pattern} found in file {file_path}")
                                lines[pattern] = file_path
                                break
                    except UnicodeDecodeError:
                        continue

        # Process each file with matched patterns
        if all(lines.values()):
            print("js file patched using yearly_active_sub method")
            for pattern, file_path in lines.items():
                self.apply_patch(file_path, pattern, patterns[pattern])
        else:
            print("Not all 4 patterns found for yearly_active_sub patch")
            return False

        return True

    def gifted_sub(self):
        pattern = (
            '{return"application/json"===e.headers.get("Content-Type")?await e.json():await e.text()}'
        )
        replacement = (
            '{if("application/json"===e.headers.get("Content-Type")){var t=await e.json();'
            'if(void 0!==t.subscription){class e{constructor(){var e=(new Date).getFullYear();'
            'this.d=new Date(e)}plus(e,t){var i=Number(e);return"year"===t?this.d.setFullYear(this.d.getFullYear()+i):'
            '"day"===t&&this.d.setDate(this.d.getDate()+i),this}minus(e,t){return this.plus(-1*e,t)}toISOString(){'
            'return this.d.toISOString()}}var i=(new e).minus(1,"day"),n=i.toISOString(),s=i.plus(1,"year").toISOString(),'
            'o=((new e).minus(1,"day").plus(3,"day").toISOString(),{startedAt:n,endsAt:s,period:"yearly",state:"active",'
            'nextInvoice:{date:s,amount:0,currency:"USD"}});o={...o,gift:{senderName:"WeMod"}},t.subscription=o}'
            'return t}return await e.text()}'
        )

        # Search for the original line in JS files
        js_files = [f for f in os.listdir(WEMOD_TEMP_DIR) if f.endswith(".js")]
        for js_file in js_files:
            file_path = os.path.join(WEMOD_TEMP_DIR, js_file)
            try:
                with open(file_path, "r", encoding="utf-8") as file:
                    content = file.read()

                if pattern in content:
                    self.apply_patch(file_path, re.escape(pattern), replacement)
                    print(f"js file patched using gifted_sub method: {file_path}")
                    return True
            except UnicodeDecodeError:
                pass

        print("Pattern not found for gifted_sub patch")
        return False
