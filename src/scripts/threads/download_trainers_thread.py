import os
import re
import shutil
import stat
import subprocess
import time
from urllib.parse import urlparse

from bs4 import BeautifulSoup
import requests

from config import *
from threads.download_base_thread import DownloadBaseThread


class DownloadTrainersThread(DownloadBaseThread):
    def __init__(self, index, trainers, trainerDownloadPath, update, trainerPath, updateUrl, parent=None):
        super().__init__(parent)
        self.index = index
        self.trainers = trainers
        self.trainerDownloadPath = trainerDownloadPath
        self.update = update
        self.trainerPath = trainerPath
        self.updateUrl = updateUrl
        self.download_finish_delay = 0.5
        self.update_error_delay = 3

    def run(self):
        self.message.emit(tr("Checking for internet connection..."), None)
        if not self.is_internet_connected():
            self.message.emit(tr("No internet connection, download failed."), "failure")
            time.sleep(self.download_finish_delay)
            self.finished.emit(1)
            return

        if os.path.exists(DOWNLOAD_TEMP_DIR):
            shutil.rmtree(DOWNLOAD_TEMP_DIR)
        antiUrl = ""
        
        if self.update:
            self.trainerName = os.path.splitext(os.path.basename(self.trainerPath))[0]
            self.message.emit(tr("Updating ") + self.trainerName + "...", None)

        if self.update or settings["downloadServer"] == "intl":
            # Trainer name check
            if not self.update:
                trainer_list = list(DownloadBaseThread.trainer_urls.items())
                filename = trainer_list[self.index][0]
                trainerName_download = self.symbol_replacement(filename)
                self.message.emit(tr("Translating trainer name..."), None)
                trainerName_final = self.symbol_replacement(self.translate_trainer(trainerName_download))

                for trainerPath in self.trainers.keys():
                    if trainerName_download in trainerPath or trainerName_final in trainerPath:
                        self.message.emit(tr("Trainer already exists, aborted download."), "failure")
                        time.sleep(self.download_finish_delay)
                        self.finished.emit(1)
                        return
            else:
                trainerName_download = self.trainerName
                trainerName_final = self.trainerName

            # Download trainer
            self.message.emit(tr("Downloading..."), None)
            try:
                # Additional trainer file extraction for trainers from main site
                if not self.update:
                    targetUrl = DownloadBaseThread.trainer_urls[filename]
                else:
                    targetUrl = self.updateUrl

                domain = urlparse(targetUrl).netloc
                if domain == "flingtrainer.com":
                    page_content = self.get_webpage_content(targetUrl, "FLiNG Trainer")
                    trainerPage = BeautifulSoup(page_content, 'html.parser')
                    targetObj = trainerPage.find(target="_self")
                    if targetObj:
                        targetUrl = targetObj.get("href")
                    else:
                        raise Exception(tr("Internet request failed."))
                
                os.makedirs(DOWNLOAD_TEMP_DIR, exist_ok=True)
                trainerTemp = self.request_download(targetUrl, DOWNLOAD_TEMP_DIR, trainerName_download)

            except Exception as e:
                self.message.emit(tr("An error occurred while downloading trainer: ") + str(e), "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
                return
            
            # Ensure file is successfully downloaded
            found_trainer = False
            for i in range(30):
                if os.path.exists(trainerTemp):
                    found_trainer = True
                    break
                time.sleep(1)
            if not found_trainer:
                self.message.emit(tr("Downloaded file not found."), "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
                return

            # Extract compressed file and rename
            self.message.emit(tr("Decompressing..."), None)
            try:
                command = [unzip_path, "x", "-y", trainerTemp, f"-o{DOWNLOAD_TEMP_DIR}"]
                subprocess.run(command, check=True, creationflags=subprocess.CREATE_NO_WINDOW)

            except Exception as e:
                self.message.emit(tr("An error occurred while extracting downloaded trainer: ") + str(e), "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
                return

            # Locate extracted .exe file
            cnt = 0
            exeRawName = []
            for filename in os.listdir(DOWNLOAD_TEMP_DIR):
                if "trainer" in filename.lower() and filename.endswith(".exe"):
                    exeRawName.append(filename)
                elif "trainer" not in filename.lower():
                    cnt += 1

            # Warn user if anti-cheat files found
            if cnt > 0 and not self.update:
                self.messageBox.emit("info", tr("Attention"), tr("Please check folder for anti-cheat requirements!"))
                os.startfile(DOWNLOAD_TEMP_DIR)

            # Check if exeRawName is None
            if not exeRawName:
                self.message.emit(tr("Could not find the downloaded trainer file, please try turning your antivirus software off."), "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
                return

            # Construct destination trainer name dict (may have multiple versions of a same game)
            os.makedirs(self.trainerDownloadPath, exist_ok=True)
            src_dst = {}  # {source file: destination file, ...}
            if len(exeRawName) > 1:
                if self.update:
                    match = re.search(r'^(.*?)(\s+v\d+|\s+Early Access)', exeRawName[0])
                    trainerName = match.group(1) + " Trainer"
                    trainerName_final = self.symbol_replacement(self.translate_trainer(trainerName))

                for source_exe in exeRawName:
                    trainer_details = ""
                    if domain == "flingtrainer.com":
                        pattern = r'trainer(.*)'
                        match = re.search(pattern, source_exe, re.IGNORECASE)
                        if match:
                            trainer_details = match.group(1)
                    else:
                        pattern = r"\s+Update.*|\s+v\d+.*"
                        match = re.search(pattern, source_exe)
                        if match:
                            trainer_details = match.group().replace(" Trainer", "")

                    trainer_name = f"{trainerName_final}{trainer_details}"
                
                    source_file = os.path.join(DOWNLOAD_TEMP_DIR, source_exe)
                    destination_file = os.path.join(self.trainerDownloadPath, trainer_name)
                    src_dst[source_file] = destination_file

            else:
                trainer_name = f"{trainerName_final}.exe"
                source_file = os.path.join(DOWNLOAD_TEMP_DIR, exeRawName[0])
                destination_file = os.path.join(self.trainerDownloadPath, trainer_name)
                src_dst[source_file] = destination_file

        elif settings["downloadServer"] == "china":
            trainer_list = list(DownloadBaseThread.trainer_urls.items())
            trainerName = self.symbol_replacement(trainer_list[self.index][0])
            downloadUrl = trainer_list[self.index][1][0]
            antiUrl = trainer_list[self.index][1][1]
            if os.path.splitext(urlparse(antiUrl).path)[1] == ".rar":
                antiUrl = ""

            for trainerPath in self.trainers.keys():
                if trainerName in trainerPath:
                    self.message.emit(tr("Trainer already exists, aborted download."), "failure")
                    time.sleep(self.download_finish_delay)
                    self.finished.emit(1)
                    return
            
            # Download trainer
            self.message.emit(tr("Downloading..."), None)
            base_url, filename = downloadUrl.rsplit('/', 1)
            modified_url = f"{base_url}/3DMGAME-{filename}"
            urls_to_try = [downloadUrl, modified_url]

            download_successful = False
            try:
                for url in urls_to_try:
                    req = requests.get(url, headers=self.headers)
                    if req.status_code == 200:
                        download_successful = True
                        break
            except Exception as e:
                print(f"Error requesting {urls_to_try}: {str(e)}")
                return

            if not download_successful:
                self.message.emit(tr("An error occurred while downloading trainer: ") + f"Status code {req.status_code}: {req.reason}", "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
                return
            
            os.makedirs(DOWNLOAD_TEMP_DIR, exist_ok=True)
            trainerTemp = os.path.join(DOWNLOAD_TEMP_DIR, trainerName + ".zip")
            with open(trainerTemp, "wb") as f:
                f.write(req.content)

            # Download anti-cheat files
            anti_folder = os.path.join(DOWNLOAD_TEMP_DIR, "anti")
            if antiUrl:
                try:
                    req = requests.get(antiUrl, headers=self.headers)
                    if req.status_code != 200:
                        self.message.emit(tr("An error occurred while downloading trainer: ") + f"Status code {req.status_code}: {req.reason}", "failure")
                        time.sleep(self.download_finish_delay)
                        self.finished.emit(1)
                        return
                except Exception as e:
                    print(f"Error requesting {antiUrl}: {str(e)}")
                    return

                os.makedirs(anti_folder, exist_ok=True)
                antiFileName = os.path.basename(urlparse(antiUrl).path)
                antiTemp = os.path.join(anti_folder, antiFileName)
                with open(antiTemp, "wb") as f:
                    f.write(req.content)
            
            # Decompress downloaded zip
            self.message.emit(tr("Decompressing..."), None)
            try:
                command = [unzip_path, "x", "-y", trainerTemp, f"-o{DOWNLOAD_TEMP_DIR}"]
                subprocess.run(command, check=True, creationflags=subprocess.CREATE_NO_WINDOW)
                if antiUrl:
                    command = [unzip_path, "x", "-y", antiTemp, f"-o{anti_folder}"]
                    subprocess.run(command, check=True, creationflags=subprocess.CREATE_NO_WINDOW)

            except Exception as e:
                self.message.emit(tr("An error occurred while extracting downloaded trainer: ") + str(e), "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
                return

            # Locate extracted .exe file
            cnt = 0
            exeRawName = None
            for filename in os.listdir(DOWNLOAD_TEMP_DIR):
                if filename.endswith(".exe"):
                    exeRawName = filename

            # Warn user if anti-cheat files found
            if antiUrl:
                self.messageBox.emit("info", tr("Attention"), tr("Please check folder for anti-cheat requirements!"))
                os.startfile(anti_folder)
            
            if not exeRawName:
                self.message.emit(tr("Could not find the downloaded trainer file, please try turning your antivirus software off."), "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
                return

            os.makedirs(self.trainerDownloadPath, exist_ok=True)
            src_dst = {}
            source_file = os.path.join(DOWNLOAD_TEMP_DIR, exeRawName)
            destination_file = os.path.join(self.trainerDownloadPath, trainerName + ".exe")
            src_dst[source_file] = destination_file

        # remove fling trainer bg music
        if settings["removeBgMusic"]:
            self.message.emit(tr("Removing trainer background music..."), None)
            self.modify_fling_settings(True)
            for source_file in src_dst.keys():
                self.remove_bgMusic(source_file, ["MID", "MIDI"])
        else:
            self.modify_fling_settings(False)

        try:
            # Delete original trainer file (could not preserve original file name due to multiple versions when updating)
            if len(exeRawName) > 1 and self.update:
                original_trainer_name = f"{self.trainerName}.exe"
                original_trainer_file = os.path.join(self.trainerDownloadPath, original_trainer_name)
                os.chmod(original_trainer_file, stat.S_IWRITE)
                os.remove(original_trainer_file)

            for src, dst in src_dst.items():
                if os.path.exists(dst):
                    os.chmod(dst, stat.S_IWRITE)
                shutil.move(src, dst)

            os.remove(trainerTemp)
            if antiUrl:
                os.remove(antiTemp)
            rhLog = os.path.join(DOWNLOAD_TEMP_DIR, "rh.log")
            if os.path.exists(rhLog):
                os.remove(rhLog)

        except PermissionError as e:
            self.message.emit(tr("Trainer is currently in use, please close any programs using the file and try again."), "failure")
            time.sleep(self.update_error_delay)
            self.finished.emit(1)
            return
        except Exception as e:
            self.message.emit(tr("Could not find the downloaded trainer file, please try turning your antivirus software off."), "failure")
            time.sleep(self.download_finish_delay)
            self.finished.emit(1)
            return
        
        self.message.emit(tr("Download success!"), "success")
        time.sleep(self.download_finish_delay)
        self.finished.emit(0)
    
    def modify_fling_settings(self, removeBgMusic):
        # replace bg music in Documents folder
        username = os.getlogin()
        flingSettings_path = f"C:/Users/{username}/Documents/FLiNGTrainer"
        bgMusic_path = os.path.join(flingSettings_path, "TrainerBGM.mid")
        if os.path.exists(bgMusic_path):
            if settings["removeBgMusic"]:
                shutil.copyfile(emptyMidi_path, bgMusic_path)
            else:
                os.remove(bgMusic_path)

        # change fling settings to disable startup music
        settingFiles = [
            os.path.join(flingSettings_path, "FLiNGTSettings.ini"),
            os.path.join(flingSettings_path, "TrainerSettings.ini")
        ]

        for settingFile in settingFiles:
            if not os.path.exists(settingFile):
                continue
            with open(settingFile, 'r', encoding='utf-8') as file:
                lines = file.readlines()

            # Update the OnLoadMusic setting
            with open(settingFile, 'w', encoding='utf-8') as file:
                for line in lines:
                    if line.strip().startswith('OnLoadMusic'):
                        if os.path.basename(settingFile) == "FLiNGTSettings.ini":
                            if removeBgMusic:
                                file.write('OnLoadMusic = False\n')
                            else:
                                file.write('OnLoadMusic = True\n')
                        elif os.path.basename(settingFile) == "TrainerSettings.ini":
                            if removeBgMusic:
                                file.write('OnLoadMusic=False\n')
                            else:
                                file.write('OnLoadMusic=True\n')
                    else:
                        file.write(line)

    def remove_bgMusic(self, source_exe, resource_type_list):
        # Base case
        if not resource_type_list:
            return

        resource_type = resource_type_list.pop(0)

        # Define paths and files
        tempLog = os.path.join(DOWNLOAD_TEMP_DIR, "rh.log")

        # Remove background music from executable
        command = [resourceHacker_path, "-open", source_exe, "-save", source_exe,
                   "-action", "delete", "-mask", f"{resource_type},,", "-log", tempLog]
        subprocess.run(command, creationflags=subprocess.CREATE_NO_WINDOW)

        # Read the log file
        with open(tempLog, 'r', encoding='utf-16-le') as file:
            log_content = file.read()

        # Check for deleted resource in log
        pattern = r"Deleted:\s*(\w+),(\d+),(\d+)"
        match = re.search(pattern, log_content)

        if match:
            # Resource was deleted; prepare to add the empty midi
            resource_id = match.group(2)
            locale_id = match.group(3)
            resource = ",".join([resource_type, resource_id, locale_id])
            command = [resourceHacker_path, "-open", source_exe, "-save", source_exe,
                       "-action", "addoverwrite", "-res", emptyMidi_path, "-mask", resource]
            subprocess.run(command, creationflags=subprocess.CREATE_NO_WINDOW)
        else:
            # Try the next resource type if any remain
            self.remove_bgMusic(source_exe, resource_type_list)
