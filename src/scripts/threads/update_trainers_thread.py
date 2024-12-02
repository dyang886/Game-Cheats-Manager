import concurrent.futures
import datetime
import locale
import os
import re
import shutil
import subprocess
import threading
import time
import uuid

from bs4 import BeautifulSoup
from fuzzywuzzy import process
from PyQt6.QtCore import pyqtSignal

from config import *
from threads.download_base_thread import DownloadBaseThread


class UpdateTrainers(DownloadBaseThread):
    message = pyqtSignal(str, str)
    update = pyqtSignal(str, str, str)
    updateTrainer = pyqtSignal(str, str)
    finished = pyqtSignal(str)

    browser_condition = threading.Condition()

    def __init__(self, trainers, parent=None):
        super().__init__(parent)
        self.trainers = trainers

    def run(self):
        statusWidgetName = "trainerUpdate"
        self.message.emit(statusWidgetName, tr("Checking for trainer updates"))

        if os.path.exists(VERSION_TEMP_DIR):
            shutil.rmtree(VERSION_TEMP_DIR)

        if self.is_internet_connected():
            with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
                futures = [executor.submit(self.process_trainer, trainerPath) for trainerPath in self.trainers.values()]

                for future in concurrent.futures.as_completed(futures):
                    result = future.result()
                    if result:
                        trainerPath, update_url = result
                        self.updateTrainer.emit(trainerPath, update_url)
        else:
            self.update.emit(statusWidgetName, tr("Check trainer updates failed"), "error")
            time.sleep(2)

        self.finished.emit(statusWidgetName)

    def process_trainer(self, trainerPath):
        # The binary pattern representing "FLiNGTrainerNamedPipe_" followed by some null bytes and the date
        pattern_hex = '46 00 4c 00 69 00 4E 00 47 00 54 00 72 00 61 00 69 00 6E 00 65 00 72 00 4E 00 61 00 6D 00 65 00 64 00 50 00 69 00 70 00 65 00 5F'
        pattern = bytes.fromhex(''.join(pattern_hex.split()))
        tagName = self.get_product_name(trainerPath)

        if os.path.exists(trainerPath):
            with open(trainerPath, 'rb') as file:
                content = file.read()
                pattern_index = content.find(pattern)
                if pattern_index != -1:
                    start_index = pattern_index + len(pattern)
                    while content[start_index] == 0:
                        start_index += 1

                    # The date could be "Mar  8 2024" or "Dec 10 2022"
                    date_match = re.search(rb'\b(?:Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+\d{1,2}\s+\d{4}\b', content[start_index:])
                    if date_match and tagName:
                        locale.setlocale(locale.LC_TIME, 'English_United States')
                        trainerSrcDate = datetime.datetime.strptime(date_match.group().decode('utf-8'), '%b %d %Y')

                        page_content = self.get_webpage_content_with_lock(f"https://flingtrainer.com/tag/{tagName}", "FLiNG Trainer")
                        tagPage = BeautifulSoup(page_content, 'html.parser')

                        trainerNamesMap = {}  # trainerName: gameContentEntryOnWeb
                        for game in tagPage.find_all('div', class_='post-content'):
                            trainerName = self.sanitize(game.find('a', rel='bookmark').text)
                            trainerNamesMap[trainerName] = game

                        if trainerNamesMap:
                            if len(trainerNamesMap) == 1:
                                targetGameObj = next(iter(trainerNamesMap.values()))
                                version_entry = targetGameObj.find('div', class_='entry')
                            else:
                                best_match, score = process.extractOne(self.sanitize(tagName + "-trainer"), trainerNamesMap.keys())
                                if score >= 85:
                                    targetGameObj = trainerNamesMap[best_match]
                                    version_entry = targetGameObj.find('div', class_='entry')

                            if version_entry:
                                match = re.search(r'Last Updated:\s+(\d+\.\d+\.\d+)', version_entry.text)
                                if match:
                                    trainerDstDate = datetime.datetime.strptime(match.group(1), '%Y.%m.%d')
                                    print(f"{tagName}\nTrainer source date: {trainerSrcDate.strftime('%Y-%m-%d')}\nNewest build date: {trainerDstDate.strftime('%Y-%m-%d')}\n")

                                    if trainerDstDate > trainerSrcDate:
                                        # Special cases where trainers keep updating
                                        inconsistent_update_dates = ["dynasty-warriors-8-xtreme-legends-complete-edition", "dredge"]
                                        if tagName in inconsistent_update_dates and (trainerDstDate - trainerSrcDate < datetime.timedelta(days=2)):
                                            return None

                                        update_url = targetGameObj.find('a', href=True, rel='bookmark')['href']
                                        return trainerPath, update_url
        return None

    def get_webpage_content_with_lock(self, url, target_text):
        with self.browser_condition:
            self.browser_condition.wait_for(lambda: True)

            # Trigger the browser dialog and wait for its completion
            content = self.get_webpage_content(url, target_text)

            self.browser_condition.notify_all()

        return content

    def get_product_name(self, trainerPath):
        os.makedirs(VERSION_TEMP_DIR, exist_ok=True)
        unique_id = uuid.uuid4().hex
        temp_version_info = os.path.join(VERSION_TEMP_DIR, f"version_info_{unique_id}.rc")

        # Command to extract version information using Resource Hacker
        command = [resourceHacker_path, "-open", trainerPath, "-save",
                   temp_version_info, "-action", "extract", "-mask", "VERSIONINFO,,"]
        subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, creationflags=subprocess.CREATE_NO_WINDOW)

        file_content = None
        for _ in range(10):
            if os.path.exists(temp_version_info):
                with open(temp_version_info, 'r', encoding='utf-16') as file:
                    file_content = file.read()
                    break
            time.sleep(0.2)
        if not file_content:
            print("\nCould not find extracted .rc file for version info")
            return None

        product_name = None
        tag_name = None
        match = re.search(r'VALUE "ProductName", "(.*?)"', file_content)
        os.remove(temp_version_info)
        if match:
            product_name = match.group(1)
            # Parse only the game name
            match = re.search(r'^(.*?)(\s+v\d+|\s+Early Access)', product_name)
            if match:
                tag_name = match.group(1).lower().replace("(", "").replace(")", "").replace(" ", "-")

        return tag_name
