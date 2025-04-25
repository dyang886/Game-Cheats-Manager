import json
import os
import re
import string
from urllib.parse import urlparse, unquote
from urllib3.exceptions import InsecureRequestWarning
import warnings

import cloudscraper
from fuzzywuzzy import process
from PyQt6.QtCore import QEventLoop, QThread, pyqtSignal
import requests
import zhon

from config import *
from widgets.browser_dialog import BrowserDialog

warnings.simplefilter("ignore", InsecureRequestWarning)


class DownloadBaseThread(QThread):
    message = pyqtSignal(str, str)
    messageBox = pyqtSignal(str, str, str)
    finished = pyqtSignal(int)
    loadUrl = pyqtSignal(str, str)
    downloadFile = pyqtSignal(str, str)

    trainer_urls = []
    headers = {
        "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.110 Safari/537.36",
        "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8",
    }

    def __init__(self, parent=None):
        super().__init__(parent)
        self.html_content = ""
        self.downloaded_file_path = ""
        self.browser_dialog = BrowserDialog()
        self.loadUrl.connect(self.browser_dialog.load_url)
        self.browser_dialog.content_ready.connect(self.handle_content_ready)
        self.downloadFile.connect(self.browser_dialog.handle_download)
        self.browser_dialog.download_completed.connect(self.handle_download_completed)

    def get_webpage_content(self, url, target_text, verify, use_cloudScraper=False):
        if not self.is_internet_connected():
            return ""

        try:
            if use_cloudScraper:
                scraper = cloudscraper.create_scraper()
                req = scraper.get(url, headers=self.headers, verify=verify)
                req.raise_for_status()
            else:
                req = requests.get(url, headers=self.headers, verify=verify)
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

    def request_download(self, url, download_path, verify, use_cloudScraper=False):
        try:
            if use_cloudScraper:
                scraper = cloudscraper.create_scraper()
                req = scraper.get(url, headers=self.headers, verify=verify)
                req.raise_for_status()
            else:
                req = requests.get(url, headers=self.headers, verify=verify)
        except Exception as e:
            print(f"Error requesting {url}: {str(e)}")
            return ""

        if req.status_code != 200:
            self.loop = QEventLoop()
            self.downloadFile.emit(url, download_path)
            self.loop.exec()
        else:
            file_path = os.path.join(download_path, self.find_download_fname(req))
            with open(file_path, "wb") as f:
                f.write(req.content)
            self.downloaded_file_path = file_path

        return self.downloaded_file_path

    def find_download_fname(self, response):
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

        return urlparse(response.url).path.split("/")[-1]

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

    def find_best_trainer_match(self, target_name, target_language, threshold=85):
        trainer_details = self.load_json_content("translations.json")
        if not trainer_details:
            return None

        sanitized_to_original_en = {}
        sanitized_to_original_zh = {}
        for trainer in trainer_details:
            if 'en_US' in trainer and 'zh_CN' in trainer:
                sanitized_en = self.sanitize(trainer['en_US'])
                sanitized_zh = self.sanitize(trainer['zh_CN'])
                sanitized_to_original_en[sanitized_en] = trainer['zh_CN']
                sanitized_to_original_zh[sanitized_zh] = trainer['en_US']

        # Determine the mapping and target based on the desired language
        sanitized_target = self.sanitize(target_name)
        if target_language == "zh":
            sanitized_to_original = sanitized_to_original_en
        elif target_language == "en":
            sanitized_to_original = sanitized_to_original_zh

        best_match, score = process.extractOne(sanitized_target, sanitized_to_original.keys())
        if score >= threshold:
            return sanitized_to_original[best_match]

        # Ignore cases where input trainer name and target language are the same language
        if is_chinese(target_name) and target_language == 'zh':
            pass
        elif not is_chinese(target_name) and target_language == 'en':
            pass
        else:
            print(f"No matchings found for {target_name}")

        return None

    def translate_trainer(self, trainerName, origin):
        # Special cases
        if trainerName == "Bright.Memory.Episode.1":
            trainerName = "Bright Memory: Episode 1"
        if trainerName == "轩辕剑柒 / 轩辕剑7":
            trainerName = "轩辕剑7"

        if settings["language"] in ["zh_CN", "zh_TW"] and not settings["enSearchResults"]:
            # Target language is Chinese
            try:
                prefix = "[小幸]" if origin == "xiaoxing" else ""
                best_match = self.find_best_trainer_match(trainerName, "zh")
                trainerName = f"{prefix}《{best_match or trainerName}》修改器"
            except Exception as e:
                print(f"An error occurred while translating trainer name: {str(e)}")

        elif settings["language"] == "en_US" or settings["enSearchResults"]:
            # Target language is English
            try:
                prefix = "[XiaoXing]" if origin == "xiaoxing" else ""
                best_match = self.find_best_trainer_match(trainerName, "en")
                trainerName = f"{prefix} {best_match or trainerName} Trainer"
            except Exception as e:
                print(f"An error occurred while translating trainer name: {str(e)}")

        return trainerName

    def save_html_content(self, content, file_name, overwrite=True):
        html_file = os.path.join(DATABASE_PATH, file_name)
        mode = 'w' if overwrite else 'a'
        with open(html_file, mode, encoding='utf-8') as file:
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
