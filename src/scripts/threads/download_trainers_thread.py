import os
import re
import shutil
import stat
import subprocess
import time

from bs4 import BeautifulSoup

from config import *
from threads.download_base_thread import DownloadBaseThread


class DownloadTrainersThread(DownloadBaseThread):
    def __init__(self, index, trainers, trainerDownloadPath, update_entry, parent=None):
        super().__init__(parent)
        self.index = index
        self.trainers = trainers
        self.trainerDownloadPath = trainerDownloadPath
        self.update_entry = update_entry
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

        self.src_dst = []  # List content: { "src": source_path, "dst": destination_path, "version": YYYY.MM.DD }
        self.antiCheatDst = ""
        selected_trainer = None
        if not self.update_entry:
            selected_trainer = DownloadBaseThread.trainer_urls[self.index]
        else:
            selected_trainer = self.update_entry
        origin = selected_trainer["origin"]

        result = True
        if origin == "fling_main" or origin == "fling_archive":
            result = self.download_fling(selected_trainer)
        elif origin == "xiaoxing":
            result = self.download_xiaoxing(selected_trainer)

        try:
            for item in self.src_dst:
                if os.path.exists(item["dst"]):
                    os.chmod(item["dst"], stat.S_IWRITE)
                if os.path.splitext(item['src'])[1]:
                    dst_dir = os.path.dirname(item["dst"])
                    os.makedirs(dst_dir, exist_ok=True)
                else:
                    dst_dir = item["dst"]
                shutil.move(item["src"], item["dst"])

                if dst_dir != self.antiCheatDst:
                    info_dict = {
                        "game_name": selected_trainer["game_name"],
                        "origin": selected_trainer["origin"]
                    }
                    if selected_trainer.get("version"):
                        info_dict["version"] = selected_trainer["version"]

                    with open(os.path.join(dst_dir, "gcm_info.json"), "w", encoding="utf-8") as info_file:
                        json.dump(info_dict, info_file, ensure_ascii=False, indent=4)

            rhLog = os.path.join(DOWNLOAD_TEMP_DIR, "rh.log")
            if os.path.exists(rhLog):
                os.remove(rhLog)

            if self.antiCheatDst and not self.update_entry:
                self.messageBox.emit("info", tr("Attention"), tr("Please check folder for anti-cheat requirements!"))
                os.startfile(self.antiCheatDst)

        except PermissionError as e:
            self.message.emit(tr("Trainer is currently in use, please close any programs using the file and try again."), "failure")
            time.sleep(self.update_error_delay)
            self.finished.emit(1)
            return
        except Exception as e:
            self.message.emit(tr("An error occurred when moving trainer: ") + str(e), "failure")
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
        if self.update_entry:
            trainerName_display = selected_trainer["trainer_name"]
            self.message.emit(tr("Updating ") + trainerName_display + "...", None)
        else:
            trainerName_display = self.symbol_replacement(selected_trainer["trainer_name"])
            # Trainer duplication check
            for trainerPath in self.trainers.keys():
                if trainerName_display == os.path.splitext(os.path.basename(trainerPath))[0]:
                    self.message.emit(tr("Trainer already exists, aborted download."), "failure")
                    time.sleep(self.download_finish_delay)
                    self.finished.emit(1)
                    return False

        # Download trainer
        self.message.emit(tr("Downloading..."), None)
        try:
            targetUrl = selected_trainer["url"]

            # Additional trainer file extraction for trainers from main site
            if settings["flingDownloadServer"] == "official" and not self.update_entry:
                if selected_trainer['origin'] == "fling_main":
                    page_content = self.get_webpage_content(targetUrl)
                    trainerPage = BeautifulSoup(page_content, 'html.parser')
                    targetObj = trainerPage.find(lambda tag: tag.get('target') == '_self' and 'flingtrainer.com' in tag.get('href'))
                    if not targetObj:
                        raise Exception(tr("Failed to find trainer download link."))
                    targetUrl = targetObj.get("href")
                    div_entry = trainerPage.find('div', class_='entry')
                    if div_entry:
                        pattern = r'options.*game\s*version.*last\s*updated:\s*(\d{4}\.[0-1]?\d\.[0-3]?\d)'
                        match = re.search(pattern, div_entry.get_text(separator=' ', strip=True), re.IGNORECASE)
                        if match:
                            version = match.group(1)
                    if not version:
                        raise Exception(tr("Failed to find trainer version."))
                    selected_trainer["version"] = version

            # Download trainers from gcm server
            elif settings["flingDownloadServer"] == "gcm" or self.update_entry:
                targetUrl = self.get_signed_download_url(targetUrl)

            trainerTemp = self.request_download(targetUrl, DOWNLOAD_TEMP_DIR)
            if not trainerTemp:
                raise Exception(tr("Internet request failed."))

        except Exception as e:
            self.message.emit(tr("An error occurred while downloading trainer: ") + str(e), "failure")
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
        extractedAntiCheatNames = []
        for filename in os.listdir(DOWNLOAD_TEMP_DIR):
            if "trainer" in filename.lower() and filename.endswith(".exe"):
                extractedTrainerNames.append(filename)
            # Count anti-cheat files
            elif ("trainer" not in filename.lower() and filename != os.path.basename(trainerTemp)) and filename.lower() != "info.txt":
                extractedAntiCheatNames.append(filename)
                cnt += 1

        # Warn user if anti-cheat files found
        if cnt > 0:
            self.antiCheatDst = os.path.join(self.trainerDownloadPath, trainerName_display, "anti-cheat")
            for antiCheatFile in extractedAntiCheatNames:
                self.src_dst.append({"src": os.path.join(DOWNLOAD_TEMP_DIR, antiCheatFile), "dst": os.path.join(self.antiCheatDst, antiCheatFile)})

        # Check if extracted trainer name is None
        if not extractedTrainerNames:
            self.message.emit(tr("Could not find the downloaded trainer file, please try turning your antivirus software off."), "failure")
            time.sleep(self.download_finish_delay)
            self.finished.emit(1)
            return False

        # Construct destination trainer name dict (may have multiple versions of a same game)
        os.makedirs(self.trainerDownloadPath, exist_ok=True)
        if len(extractedTrainerNames) > 1:
            if self.update_entry:
                trainerName_trans = self.translate_trainer(selected_trainer["game_name"], selected_trainer["origin"])
                if not trainerName_trans:
                    self.message.emit(tr("Failed to translate, please update translation data."), "failure")
                    self.finished.emit(1)
                    return
                trainerName_display = self.symbol_replacement(trainerName_trans)

            for extractedTrainerName in extractedTrainerNames:
                trainer_details = ""
                if selected_trainer['origin'] == "fling_main":
                    pattern = r'trainer(.*)\.exe'
                    match = re.search(pattern, extractedTrainerName, re.IGNORECASE)
                    if match:
                        trainer_details = match.group(1)
                else:
                    pattern = r"\s+Update.*|\s+v\d+.*"
                    match = re.search(pattern, extractedTrainerName)
                    if match:
                        trainer_details = match.group().replace(" Trainer", "").rstrip(".exe")

                trainer_name = f"{trainerName_display}{trainer_details}"

                source_file = os.path.join(DOWNLOAD_TEMP_DIR, extractedTrainerName)
                destination_file = os.path.join(self.trainerDownloadPath, trainer_name, extractedTrainerName)
                self.src_dst.insert(0, {"src": source_file, "dst": destination_file})

        else:
            source_file = os.path.join(DOWNLOAD_TEMP_DIR, extractedTrainerNames[0])
            destination_file = os.path.join(self.trainerDownloadPath, trainerName_display, extractedTrainerNames[0])
            self.src_dst.insert(0, {"src": source_file, "dst": destination_file})

        # remove fling trainer bg music
        if settings["removeFlingBgMusic"]:
            self.message.emit(tr("Removing trainer background music..."), None)
            self.modify_fling_settings(True)
            for item in self.src_dst:
                if item["src"].lower().endswith(".exe"):
                    self.remove_bgMusic(item["src"], ["MID", "MIDI"])
        else:
            self.modify_fling_settings(False)

        # Delete original trainer file (could not preserve original file name due to multiple versions when updating)
        if self.update_entry:
            shutil.rmtree(selected_trainer['trainer_dir'])

        if os.path.exists(trainerTemp) and os.path.basename(trainerTemp) not in extractedTrainerNames:
            os.remove(trainerTemp)

        return True

    def download_xiaoxing(self, selected_trainer):
        if self.update_entry:
            trainerName_display = selected_trainer["trainer_name"]
            self.message.emit(tr("Updating ") + trainerName_display + "...", None)
        else:
            trainerName_display = self.symbol_replacement(selected_trainer["trainer_name"])
            # Trainer duplication check
            for trainerPath in self.trainers.keys():
                if self.symbol_replacement(selected_trainer["trainer_name"]) == os.path.splitext(os.path.basename(trainerPath))[0]:
                    self.message.emit(tr("Trainer already exists, aborted download."), "failure")
                    time.sleep(self.download_finish_delay)
                    self.finished.emit(1)
                    return False

        self.message.emit(tr("Downloading..."), None)
        extractedContentPath = os.path.join(DOWNLOAD_TEMP_DIR, "extracted")
        try:
            signed_url = self.get_signed_download_url(selected_trainer['url'])
            trainerTemp = self.request_download(signed_url, DOWNLOAD_TEMP_DIR)
            if not trainerTemp:
                raise Exception(tr("Internet request failed."))

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
        if self.update_entry:
            shutil.rmtree(selected_trainer['trainer_dir'])

        if not self.handle_xiaoxing_special_cases(selected_trainer, extractedContentPath):
            destination_path = os.path.join(self.trainerDownloadPath, trainerName_display)
            self.src_dst.append({"src": extractedContentPath, "dst": destination_path})

        return True

    def handle_xiaoxing_special_cases(self, selected_trainer, extractedContentPath):
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
