import os
import re
import shutil
import stat
import subprocess
import time
from urllib.parse import parse_qs, urlparse

from bs4 import BeautifulSoup
import cloudscraper
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
        os.makedirs(DOWNLOAD_TEMP_DIR, exist_ok=True)

        self.src_dst = []  # List content: { "src": source_path, "dst": destination_path }
        selected_trainer = None
        origin = "fling"
        if not self.update:
            selected_trainer = DownloadBaseThread.trainer_urls[self.index]
            if selected_trainer["origin"] == "xiaoxing":
                origin = "xiaoxing"

        result = True
        if origin == "fling" or self.update:
            result = self.download_fling(selected_trainer)
        elif origin == "xiaoxing":
            result = self.download_xiaoxing(selected_trainer)

        try:
            for item in self.src_dst:
                if os.path.exists(item["dst"]):
                    os.chmod(item["dst"], stat.S_IWRITE)
                shutil.move(item["src"], item["dst"])

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

        if result:
            self.message.emit(tr("Download success!"), "success")
            time.sleep(self.download_finish_delay)
            self.finished.emit(0)

    def modify_fling_settings(self, removeBgMusic):
        # replace bg music in Documents folder
        username = os.getlogin()
        flingSettings_path = f"C:/Users/{username}/Documents/FLiNGTrainer"
        bgMusic_path = os.path.join(flingSettings_path, "TrainerBGM.mid")
        if os.path.exists(bgMusic_path):
            if settings["removeFlingBgMusic"]:
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

    def download_fling(self, selected_trainer):
        antiUrl = ""

        if self.update:
            self.trainerName = os.path.splitext(os.path.basename(self.trainerPath))[0]
            self.message.emit(tr("Updating ") + self.trainerName + "...", None)

        if self.update or settings["flingDownloadServer"] == "intl":
            if not self.update:
                trainerName_display = self.symbol_replacement(selected_trainer["trainer_name"])

                # Trainer duplication check
                for trainerPath in self.trainers.keys():
                    if trainerName_display in trainerPath:
                        self.message.emit(tr("Trainer already exists, aborted download."), "failure")
                        time.sleep(self.download_finish_delay)
                        self.finished.emit(1)
                        return False
            else:
                trainerName_display = self.trainerName

            # Download trainer
            self.message.emit(tr("Downloading..."), None)
            try:
                # Additional trainer file extraction for trainers from main site
                if not self.update:
                    targetUrl = selected_trainer["url"]
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

                trainerTemp = self.request_download(targetUrl, DOWNLOAD_TEMP_DIR)

            except Exception as e:
                self.message.emit(tr("An error occurred while downloading trainer: ") + str(e), "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
                return False

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
                return False

            # Extract compressed file and rename
            if os.path.splitext(trainerTemp)[1] in [".zip", ".rar"]:
                self.message.emit(tr("Decompressing..."), None)
                try:
                    command = [unzip_path, "x", "-y", trainerTemp, f"-o{DOWNLOAD_TEMP_DIR}"]
                    subprocess.run(command, check=True, creationflags=subprocess.CREATE_NO_WINDOW)

                except Exception as e:
                    self.message.emit(tr("An error occurred while extracting downloaded trainer: ") + str(e), "failure")
                    time.sleep(self.download_finish_delay)
                    self.finished.emit(1)
                    return False

            # Locate extracted .exe file
            cnt = 0
            extractedTrainerNames = []
            for filename in os.listdir(DOWNLOAD_TEMP_DIR):
                if "trainer" in filename.lower() and filename.endswith(".exe"):
                    extractedTrainerNames.append(filename)
                elif "trainer" not in filename.lower() and filename != os.path.basename(trainerTemp):
                    cnt += 1

            # Warn user if anti-cheat files found
            if cnt > 0 and not self.update:
                self.messageBox.emit("info", tr("Attention"), tr("Please check folder for anti-cheat requirements!"))
                os.startfile(DOWNLOAD_TEMP_DIR)

            # Check if extracted trainer name is None
            if not extractedTrainerNames:
                self.message.emit(tr("Could not find the downloaded trainer file, please try turning your antivirus software off."), "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
                return False

            # Construct destination trainer name dict (may have multiple versions of a same game)
            os.makedirs(self.trainerDownloadPath, exist_ok=True)
            if len(extractedTrainerNames) > 1:
                if self.update:
                    match = re.search(r'^(.*?)(\s+v\d+|\s+Early Access)', extractedTrainerNames[0])
                    trainerName_display = self.symbol_replacement(self.translate_trainer(match.group(1)))

                for extractedTrainerName in extractedTrainerNames:
                    trainer_details = ""
                    if domain == "flingtrainer.com":
                        pattern = r'trainer(.*)'
                        match = re.search(pattern, extractedTrainerName, re.IGNORECASE)
                        if match:
                            trainer_details = match.group(1)
                    else:
                        pattern = r"\s+Update.*|\s+v\d+.*"
                        match = re.search(pattern, extractedTrainerName)
                        if match:
                            trainer_details = match.group().replace(" Trainer", "")

                    trainer_name = f"{trainerName_display}{trainer_details}"

                    source_file = os.path.join(DOWNLOAD_TEMP_DIR, extractedTrainerName)
                    destination_file = os.path.join(self.trainerDownloadPath, trainer_name)
                    self.src_dst.append({"src": source_file, "dst": destination_file})

            else:
                trainer_name = f"{trainerName_display}.exe"
                source_file = os.path.join(DOWNLOAD_TEMP_DIR, extractedTrainerNames[0])
                destination_file = os.path.join(self.trainerDownloadPath, trainer_name)
                self.src_dst.append({"src": source_file, "dst": destination_file})

        elif settings["flingDownloadServer"] == "china":
            trainerName_display = self.symbol_replacement(selected_trainer["trainer_name"])
            downloadUrl = selected_trainer["url"]
            antiUrl = selected_trainer["anti_url"]
            if os.path.splitext(urlparse(antiUrl).path)[1] == ".rar":
                antiUrl = ""

            for trainerPath in self.trainers.keys():
                if trainerName_display in trainerPath:
                    self.message.emit(tr("Trainer already exists, aborted download."), "failure")
                    time.sleep(self.download_finish_delay)
                    self.finished.emit(1)
                    return False

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
                return False

            if not download_successful:
                self.message.emit(tr("An error occurred while downloading trainer: ") + f"Status code {req.status_code}: {req.reason}", "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
                return False

            trainerTemp = os.path.join(DOWNLOAD_TEMP_DIR, filename)
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
                        return False
                except Exception as e:
                    print(f"Error requesting {antiUrl}: {str(e)}")
                    return False

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
                return False

            # Locate extracted .exe file
            extractedTrainerNames = []
            for filename in os.listdir(DOWNLOAD_TEMP_DIR):
                if filename.endswith(".exe"):
                    extractedTrainerNames.append(filename)

            # Warn user if anti-cheat files found
            if antiUrl:
                self.messageBox.emit("info", tr("Attention"), tr("Please check folder for anti-cheat requirements!"))
                os.startfile(anti_folder)

            if not extractedTrainerNames:
                self.message.emit(tr("Could not find the downloaded trainer file, please try turning your antivirus software off."), "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
                return False

            os.makedirs(self.trainerDownloadPath, exist_ok=True)
            source_file = os.path.join(DOWNLOAD_TEMP_DIR, extractedTrainerNames[0])
            destination_file = os.path.join(self.trainerDownloadPath, trainerName_display + ".exe")
            self.src_dst.append({"src": source_file, "dst": destination_file})

        # remove fling trainer bg music
        if settings["removeFlingBgMusic"]:
            self.message.emit(tr("Removing trainer background music..."), None)
            self.modify_fling_settings(True)
            for item in self.src_dst:
                self.remove_bgMusic(item["src"], ["MID", "MIDI"])
        else:
            self.modify_fling_settings(False)

        # Delete original trainer file (could not preserve original file name due to multiple versions when updating)
        if len(extractedTrainerNames) > 1 and self.update:
            original_trainer_name = f"{self.trainerName}.exe"
            original_trainer_file = os.path.join(self.trainerDownloadPath, original_trainer_name)
            os.chmod(original_trainer_file, stat.S_IWRITE)
            os.remove(original_trainer_file)

        if os.path.exists(trainerTemp) and os.path.basename(trainerTemp) not in extractedTrainerNames:
            os.remove(trainerTemp)
        if antiUrl:
            os.remove(antiTemp)

        return True

    def get_onedrive_download_url(self, url):
        # Construct api url from redirected OneDrive url
        parsed_url = urlparse(url)
        query_params = parse_qs(parsed_url.query)

        authkey = query_params.get('authkey', [None])[0]
        item_id = query_params.get('id', [None])[0]
        cid = query_params.get('cid', [None])[0]

        # Validate extracted parameters
        if not all([authkey, item_id, cid]):
            print("Missing required query parameters in OneDrive URL.")
            return ""

        item_id_formatted = item_id.replace('!', '%21')
        api_url = (
            f"https://api.onedrive.com/v1.0/drives/{cid}/items/{item_id_formatted}"
            f"?select=id%2C%40content.downloadUrl&authkey={authkey}"
        )

        # Get download url from api url
        scraper = cloudscraper.create_scraper()
        req = scraper.get(api_url, headers=self.headers)
        data = req.json()
        download_url = data.get("@content.downloadUrl")
        if not download_url:
            print("OneDrive download URL not found in the API response.")
            return ""

        return download_url

    def download_xiaoxing(self, selected_trainer):
        # Trainer duplication check
        for trainerPath in self.trainers.keys():
            if self.symbol_replacement(selected_trainer["trainer_name"]) in trainerPath:
                self.message.emit(tr("Trainer already exists, aborted download."), "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
                return False

        # Download trainer archive (either MediaFire or OneDrive)
        page_content = self.get_webpage_content(selected_trainer["url"], "小幸修改器官方网站")
        trainerPage = BeautifulSoup(page_content, 'html.parser')
        mediaFireTag = trainerPage.find('a', href=lambda href: href and "www.mediafire.com" in href)
        oneDriveTag = trainerPage.find('a', href=lambda href: href and "1drv.ms" in href)
        extractedContentPath = os.path.join(DOWNLOAD_TEMP_DIR, "extracted")

        mediaFire_failed = False
        oneDrive_failed = False
        trainerTemp = None

        self.message.emit(tr("Downloading..."), None)

        try:
            # Attempt MediaFire download
            if mediaFireTag:
                mediaFirePage = self.get_webpage_content(mediaFireTag.get("href"), None, True)
                mediaFireHTML = BeautifulSoup(mediaFirePage, "html.parser")
                mediaFireDownloadButton = mediaFireHTML.find(id="downloadButton")
                if mediaFireDownloadButton:
                    trainerTemp = self.request_download(mediaFireDownloadButton.get("href"), DOWNLOAD_TEMP_DIR, True)
                else:
                    mediaFire_failed = True
            else:
                mediaFire_failed = True

            # Attempt OneDrive download if MediaFire failed
            if mediaFire_failed and oneDriveTag:
                scraper = cloudscraper.create_scraper()
                req = scraper.get(oneDriveTag.get("href"), headers=self.headers)
                oneDriveLink = self.get_onedrive_download_url(req.url)
                if oneDriveLink:
                    trainerTemp = self.request_download(oneDriveLink, DOWNLOAD_TEMP_DIR, True)
                else:
                    oneDrive_failed = True
            elif not oneDriveTag:
                oneDrive_failed = True

            if mediaFire_failed and oneDrive_failed:
                raise Exception(tr("No download link available"))

        except Exception as e:
            self.message.emit(tr("An error occurred while downloading trainer: ") + str(e), "failure")
            time.sleep(self.download_finish_delay)
            self.finished.emit(1)
            return False

        # Extract compressed file
        self.message.emit(tr("Decompressing..."), None)
        try:
            command = [unzip_path, "x", "-y", trainerTemp, f"-o{extractedContentPath}"]
            subprocess.run(command, check=True, creationflags=subprocess.CREATE_NO_WINDOW)

        except Exception as e:
            self.message.emit(tr("An error occurred while extracting downloaded trainer: ") + str(e), "failure")
            time.sleep(self.download_finish_delay)
            self.finished.emit(1)
            return False

        os.remove(trainerTemp)

        if not self.handle_xiaoxing_special_cases(selected_trainer, extractedContentPath):
            destination_path = os.path.join(self.trainerDownloadPath, self.symbol_replacement(selected_trainer["trainer_name"]))
            self.src_dst.append({"src": extractedContentPath, "dst": destination_path})

        return True

    def handle_xiaoxing_special_cases(self, selected_trainer, extractedContentPath):
        # Specific game cases
        folder_keywords = {
            "轩辕剑6": "轩辕剑6",
            "轩辕剑外传：穹之扉": "穹之扉"
        }

        if selected_trainer["game_name"] in folder_keywords.keys():
            destination_path = os.path.join(self.trainerDownloadPath, self.symbol_replacement(selected_trainer["trainer_name"]))
            keyword = folder_keywords.get(selected_trainer["game_name"])
            for folder_name in os.listdir(extractedContentPath):
                source_path = os.path.join(extractedContentPath, folder_name)
                if os.path.isdir(source_path) and keyword in folder_name:
                    self.src_dst.append({"src": source_path, "dst": destination_path})
                    return True

        # Multiple version case (check if there are only folders and no `.exe` files)
        temp_contents = os.listdir(extractedContentPath)
        has_exe = any(file.lower().endswith(".exe") for file in temp_contents if os.path.isfile(os.path.join(extractedContentPath, file)))
        only_folders = all(os.path.isdir(os.path.join(extractedContentPath, item)) for item in temp_contents)

        if not has_exe and only_folders:
            for folder_name in temp_contents:
                source_path = os.path.join(extractedContentPath, folder_name)
                destination_path = os.path.join(self.trainerDownloadPath, self.symbol_replacement(f"{selected_trainer['trainer_name']} {folder_name}"))
                self.src_dst.append({"src": source_path, "dst": destination_path})
            return True

        # Double extraction case (check if there is no `.exe` and a `.rar` file exists)
        rar_file = next(
            (file for file in temp_contents if file.lower().endswith(".rar") and os.path.isfile(os.path.join(extractedContentPath, file))),
            None
        )

        if not has_exe and rar_file:
            destination_path = os.path.join(self.trainerDownloadPath, self.symbol_replacement(selected_trainer["trainer_name"]))
            file_path = os.path.join(extractedContentPath, rar_file)
            try:
                command = [unzip_path, "x", "-y", f"-p123", file_path, f"-o{extractedContentPath}"]
                subprocess.run(command, check=True, creationflags=subprocess.CREATE_NO_WINDOW)
                os.remove(file_path)
                self.src_dst.append({"src": extractedContentPath, "dst": destination_path})
            except Exception as e:
                self.message.emit(tr("An error occurred while extracting downloaded trainer: ") + str(e), "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
            return True

        return False
