import json
import os
import re
import string
import time
from urllib.parse import urlparse

from fuzzywuzzy import process
from PyQt6.QtCore import QEventLoop, QThread, pyqtSignal
import requests
import zhon

from config import *
from widgets.browser_dialog import BrowserDialog


class DownloadBaseThread(QThread):
    message = pyqtSignal(str, str)
    messageBox = pyqtSignal(str, str, str)
    finished = pyqtSignal(int)
    loadUrl = pyqtSignal(str, str)
    downloadFile = pyqtSignal(str, str, str)

    trainer_urls = {}  # For intl download server: {trainer name: download link}; for china download server: {trainer name: [download link, anti-cheats download link]}
    headers = {
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3',
    }
    translator_initializing = False
    translator_warnings_displayed = False  # make sure warning doesn't display more than once
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.ts = None
        self.html_content = ""
        self.downloaded_file_path = ""
        self.browser_dialog = BrowserDialog()
        self.loadUrl.connect(self.browser_dialog.load_url)
        self.browser_dialog.content_ready.connect(self.handle_content_ready)
        self.downloadFile.connect(self.browser_dialog.handle_download)
        self.browser_dialog.download_completed.connect(self.handle_download_completed)

    def get_webpage_content(self, url, target_text):
        if not self.is_internet_connected():
            return ""

        try:
            req = requests.get(url, headers=self.headers)
        except Exception as e:
            print(f"Error requesting {url}: {str(e)}")
            return ""

        if req.status_code != 200:
            self.loop = QEventLoop()
            self.loadUrl.emit(url, target_text)
            self.loop.exec()
        else:
            self.html_content = req.text

        return self.html_content

    def handle_content_ready(self, html_content):
        self.html_content = html_content
        if self.loop.isRunning():
            self.loop.quit()

    def request_download(self, url, download_path, file_name):
        try:
            req = requests.get(url, headers=self.headers)
        except Exception as e:
            print(f"Error requesting {url}: {str(e)}")
            return ""
        
        if req.status_code != 200:
            self.loop = QEventLoop()
            self.downloadFile.emit(url, download_path, file_name)
            self.loop.exec()
        else:
            extension = os.path.splitext(urlparse(req.url).path)[1]
            trainerTemp = os.path.join(download_path, file_name + extension)
            with open(trainerTemp, "wb") as f:
                f.write(req.content)
            self.downloaded_file_path = trainerTemp

        return self.downloaded_file_path

    def handle_download_completed(self, file_path):
        self.downloaded_file_path = file_path
        if self.loop.isRunning():
            self.loop.quit()

    def is_internet_connected(self, urls=None, timeout=5):
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
    
    def arabic_to_roman(self, num):
        if num == 0:
            return '0'

        numeral_map = [
            (1000, 'M'), (900, 'CM'), (500, 'D'), (400, 'CD'),
            (100, 'C'), (90, 'XC'), (50, 'L'), (40, 'XL'),
            (10, 'X'), (9, 'IX'), (5, 'V'), (4, 'IV'), (1, 'I')
        ]
        roman = ''

        while num > 0:
            for i, r in numeral_map:
                while num >= i:
                    roman += r
                    num -= i

        return roman
    
    def sanitize(self, text):
        text = re.sub(r'\d+', lambda x: self.arabic_to_roman(int(x.group())), text)
        all_punctuation = string.punctuation + zhon.hanzi.punctuation
        return ''.join(char for char in text if char not in all_punctuation and not char.isspace()).lower()
    
    def symbol_replacement(self, text):
        return text.replace(': ', ' - ').replace(':', '-').replace("/", "_").replace("?", "")
    
    def find_best_trainer_match(self, targetEnName, threshold=85):
        trainer_details = self.load_json_content("xgqdetail.json")
        if not trainer_details:
            return None

        # Create a mapping of sanitized English names to their original dictionary
        sanitized_to_original = {self.sanitize(trainer['en_name']): trainer['keyw'] for trainer in trainer_details}

        sanitized_target = self.sanitize(targetEnName)
        best_match, score = process.extractOne(sanitized_target, sanitized_to_original.keys())

        if score >= threshold:
            # Return the Chinese name corresponding to the best-matching English name
            return sanitized_to_original[best_match]
        return None
    
    def initialize_translator(self):
        if not self.is_internet_connected():
            if not self.translator_warnings_displayed:
                self.message.emit(tr("No internet connection, translation failed."), "failure")
                self.translator_warnings_displayed = True
                time.sleep(1)
            return False
        
        try:
            if self.ts is None and not self.translator_initializing:
                self.translator_initializing = True
                self.message.emit(tr("Initializing translator..."), None)
                import translators as trans
                self.ts = trans
                self.ts.translate_text("test")
            elif self.ts is None:
                # In process of initialization, wait until completed
                for _ in range(10):
                    if self.ts is not None:
                        break
                    time.sleep(1)
            return True
        
        except Exception as e:
            print("import translators failed or error occurred while translating: " + str(e))
            return False

    def translate_trainer(self, trainerName):
        """
        For displaying trainer name only.
        """

        # =======================================================
        # Special cases
        if trainerName == "Bright.Memory.Episode.1 Trainer":
            trainerName = "Bright Memory: Episode 1 Trainer"

        trans_trainerName = trainerName

        if (settings["language"] == "zh_CN" or settings["language"] == "zh_TW") and not settings["enSearchResults"]:
            original_trainerName = trainerName.rsplit(" Trainer", 1)[0]

            try:
                # Using 3dm api to match en_names
                best_match = self.find_best_trainer_match(original_trainerName)
                if best_match:
                    trans_trainerName = f"《{best_match}》修改器"
            
                elif self.initialize_translator():
                    # Use direct translation if couldn't find a match
                    print("No matches found, using direct translation for: " + original_trainerName)
                    trans_trainerName = self.ts.translate_text(original_trainerName, from_language='en', to_language='zh')

                    # strip any game names that have their english names
                    pattern = r'[A-Za-z0-9\s：&]+（([^\）]*)\）|\（[A-Za-z\s：&]+\）$'
                    trans_trainerName = re.sub(pattern, lambda m: m.group(1) if m.group(1) else '', trans_trainerName)

                    # do not alter if game name ends with roman numerics
                    def is_roman_numeral(s):
                        return bool(re.match(r'^(?=[MDCLXVI])M?(CM|CD|D?C{0,3})(XC|XL|L?X{0,3})(IX|IV|V?I{0,3})$', s.strip()))

                    if not is_roman_numeral(trans_trainerName.split(" ")[-1]):
                        pattern = r'[A-Za-z\s：&]+$'
                        trans_trainerName = re.sub(pattern, '', trans_trainerName)

                    trans_trainerName = trans_trainerName.replace("《", "").replace("》", "")
                    trans_trainerName = f"《{trans_trainerName}》修改器"

            except Exception as e:
                print(f"An error occurred while translating trainer name: {str(e)}")

        return trans_trainerName
    
    def save_html_content(self, content, file_name):
        html_file = os.path.join(DATABASE_PATH, file_name)
        with open(html_file, 'w', encoding='utf-8') as file:
            file.write(content)
    
    def load_html_content(self, file_name):
        html_file = os.path.join(DATABASE_PATH, file_name)
        if os.path.exists(html_file):
            with open(html_file, 'r', encoding='utf-8') as file:
                return file.read()
        return ""
    
    def load_json_content(self, file_name):
        json_file = os.path.join(DATABASE_PATH, file_name)
        if os.path.exists(json_file):
            with open(json_file, 'r', encoding='utf-8') as file:
                return json.load(file)
        return ""
