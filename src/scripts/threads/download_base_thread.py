import json
import os
import re
import string
from urllib.parse import urlparse, unquote
import uuid
import warnings

import cloudscraper
from fuzzywuzzy import process
from PyQt6.QtCore import QThread, pyqtSignal
import requests
from urllib3.exceptions import InsecureRequestWarning
import zhon

from config import *

warnings.simplefilter("ignore", InsecureRequestWarning)


class DownloadBaseThread(QThread):
    message = pyqtSignal(str, str)
    messageBox = pyqtSignal(str, str, str)
    progress = pyqtSignal(int, int)
    finished = pyqtSignal(int)

    trainer_urls = []  # [{"game_name": str, "trainer_name": str, "origin": str, "author": str, "custom_name": str, "url": download url, "version": YYYY.MM.DD},]
    headers = {
        "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.110 Safari/537.36",
        "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8",
    }

    def __init__(self, parent=None):
        super().__init__(parent)
        self.html_content = ""
        self.downloaded_file_path = ""

    def get_webpage_content(self, url, verify=True, use_cloudScraper=False):
        if not self.is_internet_connected():
            return ""

        try:
            if use_cloudScraper:
                scraper = cloudscraper.create_scraper()
                req = scraper.get(url, headers=self.headers, verify=verify)
                req.raise_for_status()
            else:
                req = requests.get(url, headers=self.headers, verify=verify)
                req.raise_for_status()
        except Exception as e:
            print(f"Error requesting {url}: {str(e)}")
            return ""

        self.html_content = req.text

        return self.html_content

    def request_download(self, url, download_path, verify=True, use_cloudScraper=False):
        try:
            if use_cloudScraper:
                scraper = cloudscraper.create_scraper()
                req = scraper.get(url, headers=self.headers, verify=verify, stream=True, timeout=30)
                req.raise_for_status()
            else:
                req = requests.get(url, headers=self.headers, verify=verify, stream=True, timeout=30)
                req.raise_for_status()
        except Exception as e:
            print(f"Error requesting {url}: {str(e)}")
            return ""

        file_path = os.path.join(download_path, self.find_download_fname(req))
        total_size = int(req.headers.get('content-length', 0))
        downloaded = 0
        try:
            with open(file_path, "wb") as f:
                for chunk in req.iter_content(chunk_size=8192):
                    if chunk:
                        f.write(chunk)
                        downloaded += len(chunk)
                        self.progress.emit(downloaded, total_size)
        except Exception as e:
            print(f"Error during download stream: {str(e)}")
            if os.path.exists(file_path):
                os.remove(file_path)
            return ""
        self.downloaded_file_path = file_path

        return self.downloaded_file_path

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

        return urlparse(response.url).path.split("/")[-1]

    @staticmethod
    def get_signed_download_url(file_path_on_s3):
        if not SIGNED_URL_DOWNLOAD_ENDPOINT or not CLIENT_API_KEY:
            print("Error: API endpoint or Client API Key is not configured.")
            return None

        headers = {
            'x-api-key': CLIENT_API_KEY
        }
        params = {
            'filePath': file_path_on_s3
        }

        try:
            response = requests.get(SIGNED_URL_DOWNLOAD_ENDPOINT, headers=headers, params=params, timeout=15)
            response.raise_for_status()

            data = response.json()
            signed_url = data.get('signedUrl')
            if signed_url:
                return signed_url
            else:
                print(f"Error: 'signedUrl' not found in response. Response: {data}")
                return None

        except Exception as e:
            print(f"Error retrieving signed URL: {str(e)}")
        return None

    @staticmethod
    def get_signed_upload_url(file_path_on_s3, metadata_json):
        if not SIGNED_URL_UPLOAD_ENDPOINT or not CLIENT_API_KEY:
            print("Error: API endpoint or Client API Key is not configured.")
            return None

        # add uniqueness to file
        file_path, file_ext = os.path.splitext(file_path_on_s3)
        file_path_on_s3 = os.path.join("trainers", f"{os.path.basename(file_path)}_{uuid.uuid4().hex}{file_ext}").replace("\\", "/")

        headers = {
            'x-api-key': CLIENT_API_KEY
        }
        params = {
            'filePath': file_path_on_s3,
            'metadata': metadata_json
        }

        try:
            response = requests.get(SIGNED_URL_UPLOAD_ENDPOINT, headers=headers, params=params, timeout=15)
            response.raise_for_status()
            return response.json()

        except Exception as e:
            print(f"Error retrieving signed URL: {str(e)}")
        return None

    @staticmethod
    def is_internet_connected(urls=None, timeout=5):
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

    @staticmethod
    def arabic_to_roman(num):
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

    @staticmethod
    def symbol_replacement(text):
        return text.replace(': ', ' - ').replace(':', '-').replace("/", "_").replace("?", "")

    def find_best_trainer_match(self, target_name, target_language, threshold=85):
        trainer_details = self.load_json_content("translations.json")
        if not trainer_details:
            raise ValueError("translations.json doesn't exist in the database path.")

        # Ignore cases where input trainer name and target language are the same
        if is_chinese(target_name) and target_language == 'zh':
            return None
        elif not is_chinese(target_name) and target_language == 'en':
            return None

        # Handle cases where input trainer name and target language are different
        sanitized_to_original_en = {}  # Key: sanitized English, Value: Chinese
        sanitized_to_original_zh = {}  # Key: sanitized Chinese, Value: English
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

        return None

    def translate_trainer(self, trainer):
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

            # 3. Translate Game Name
            best_match = self.find_best_trainer_match(game_name, lang_key)
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
    def save_html_content(content, file_name, overwrite=True):
        html_file = os.path.join(DATABASE_PATH, file_name)
        mode = 'w' if overwrite else 'a'
        with open(html_file, mode, encoding='utf-8') as file:
            file.write(content)

    @staticmethod
    def load_html_content(file_name):
        html_file = os.path.join(DATABASE_PATH, file_name)
        if os.path.exists(html_file):
            try:
                with open(html_file, 'r', encoding='utf-8') as file:
                    return file.read()
            except Exception as e:
                print(f"Error loading HTML content from {html_file}: {str(e)}")
                return ""
        return ""

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
