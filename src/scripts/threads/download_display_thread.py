import concurrent.futures
import re
import time
from urllib.parse import urljoin, urlparse

from bs4 import BeautifulSoup
import cn2an
from fuzzywuzzy import fuzz
ts = None

from config import *
from threads.download_base_thread import DownloadBaseThread


class DownloadDisplayThread(DownloadBaseThread):
    def __init__(self, keyword, parent=None):
        super().__init__(parent)
        self.keyword = keyword

    def run(self):
        DownloadBaseThread.trainer_urls = {}

        if settings["downloadServer"] == "intl":
            self.translator_warnings_displayed = False
            keywordList = self.translate_keyword(self.keyword)

            self.message.emit(tr("Searching..."), None)
            status = self.search_from_archive(keywordList)
            if not status:
                self.finished.emit(1)
                return 
            status = self.search_from_main_site(keywordList)
            if not status:
                self.finished.emit(1)
                return 

            # Remove duplicates and filter 
            new_trainer_urls = {}
            for original_name, url in DownloadBaseThread.trainer_urls.items():
                # Special cases
                ignored_trainers = [
                    "Dying Light The Following Enhanced Edition Trainer",
                    "World War Z Trainer",
                    "Street Fighter V Trainer",
                ]
                if original_name in ignored_trainers:
                    continue

                # Normalize the name
                norm_name = self.sanitize(original_name)
                domain = urlparse(url).netloc

                # Add or update the entry based on domain preference
                if norm_name not in new_trainer_urls:
                    new_trainer_urls[norm_name] = (original_name, url)
                else:
                    # Check if the new URL is from the main site and should replace the existing one
                    if domain == "flingtrainer.com":
                        new_trainer_urls[norm_name] = (original_name, url)

            # Convert the dictionary to use original names as keys
            DownloadBaseThread.trainer_urls = {original: url for _, (original, url) in new_trainer_urls.items()}

            if len(DownloadBaseThread.trainer_urls) == 0:
                self.message.emit(tr("No results found."), "failure")
                self.finished.emit(1)
                return

            # Translate search results
            self.message.emit(tr("Translating search results..."), None)
            trainer_names = list(DownloadBaseThread.trainer_urls.keys())

            with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
                future_to_trainerName = {executor.submit(self.translate_trainer, trainerName): trainerName for trainerName in trainer_names}

            translated_names = {}  # {original_en_name: translated_name}
            for future in concurrent.futures.as_completed(future_to_trainerName):
                original_trainerName = future_to_trainerName[future]
                translated_trainerName = future.result()
                translated_names[original_trainerName] = translated_trainerName

            # Sort based on translated names considering pinyin
            sorted_pairs = sorted(translated_names.items(), key=lambda item: sort_trainers_key(item[1]))

            # Reconstruct `DownloadBaseThread.trainer_urls` to match the sorted order of translated names
            DownloadBaseThread.trainer_urls = {original: DownloadBaseThread.trainer_urls[original] for original, _ in sorted_pairs}
            print("\nTrainer results in original name: ", [f"{index}. {trainerName}" for index, trainerName in enumerate(DownloadBaseThread.trainer_urls.keys(), start=1)])

            # Display sorted results
            self.message.emit("", "clear")
            for count, (original_name, translated_name) in enumerate(sorted_pairs, start=1):
                self.message.emit(f"{count}. {translated_name}", None)
        
        elif settings["downloadServer"] == "china":
            self.message.emit(tr("Searching..."), None)
            status = self.search_from_xgqdetail(self.keyword)
            if not status:
                self.finished.emit(1)
                return
            
            if len(DownloadBaseThread.trainer_urls) == 0:
                self.message.emit(tr("No results found."), "failure")
                self.finished.emit(1)
                return
            
            self.message.emit("", "clear")
            DownloadBaseThread.trainer_urls = dict(sorted(DownloadBaseThread.trainer_urls.items(), key=lambda item: sort_trainers_key(item[0])))
            print("\nTrainer results with download urls:")
            for count, (trainer_name, download_urls) in enumerate(DownloadBaseThread.trainer_urls.items(), start=1):
                self.message.emit(f"{count}. {trainer_name}", None)
                print(f"{count}. {trainer_name} | {download_urls}")
        
        self.finished.emit(0)

    def translate_keyword(self, keyword):
        translations = []
        if is_chinese(keyword):
            self.message.emit(tr("Translating keywords..."), None)

            # Using 3dm api to match cn_names
            trainer_details = self.load_json_content("xgqdetail.json")
            if trainer_details:
                for trainer in trainer_details:
                    if self.sanitize(keyword) in self.sanitize(trainer.get("keyw", "")):
                        translations.append(trainer.get("en_name", ""))

            elif self.initialize_translator():
                # Direct translation
                services = ["bing"]
                for service in services:
                    try:
                        translated_keyword = self.ts.translate_text(
                            keyword,
                            from_language='zh',
                            to_language='en',
                            translator=service
                        )
                        translations.append(translated_keyword)
                        print(
                            f"Translated keyword using {service}: {translated_keyword}")
                    except Exception as e:
                        print(f"Translation failed with {service}: {str(e)}")

                if not translations:
                    self.message.emit(tr("No translations found."), "failure")

            print("\nKeyword translations:", translations)
            return translations

        return [keyword]
    
    def search_from_archive(self, keywordList):
        # Search for results from fling archive
        page_content = self.load_html_content("fling_archive.html")
        archiveHTML = BeautifulSoup(page_content, 'html.parser')

        # Check if the request was successful
        if archiveHTML.find():
            self.message.emit(tr("Search success!") + " 1/2", "success")
        else:
            self.message.emit(tr("Search failed, please wait until all data is updated from FLiNG."), "failure")
            return False

        for link in archiveHTML.find_all(target="_self"):
            # parse trainer name
            rawTrainerName = link.get_text()
            parsedTrainerName = re.sub(
                r' v[\d.]+.*|\.\bv.*| \d+\.\d+\.\d+.*| Plus\s\d+.*|Build\s\d+.*|(\d+\.\d+-Update.*)|Update\s\d+.*|\(Update\s.*| Early Access .*|\.Early.Access.*', '', rawTrainerName).replace("_", ": ")
            trainerName = parsedTrainerName.strip() + " Trainer"

            # search algorithm
            url = "https://archive.flingtrainer.com/"
            if self.keyword_match(keywordList, trainerName):
                DownloadBaseThread.trainer_urls[trainerName] = urljoin(
                    url, link.get("href"))
        
        return True
    
    def search_from_main_site(self, keywordList):
        # Search for results from fling main site, prioritized, will replace same trainer from archive
        page_content = self.load_html_content("fling_main.html")
        mainSiteHTML = BeautifulSoup(page_content, 'html.parser')

        if mainSiteHTML.find():
            self.message.emit(tr("Search success!") + " 2/2", "success")
        else:
            self.message.emit(tr("Search failed, please wait until all data is updated from FLiNG."), "failure")
            return False
        time.sleep(0.5)

        for ul in mainSiteHTML.find_all('ul'):
            for li in ul.find_all('li'):
                for link in li.find_all('a'):
                    trainerName = link.get_text().strip()
                    if trainerName in ["Home", "Trainers", "Log In", "All Trainers (A-Z)", "Privacy Policy"]:
                        continue

                    # search algorithm
                    if trainerName and self.keyword_match(keywordList, trainerName):
                        DownloadBaseThread.trainer_urls[trainerName] = link.get("href")
        
        return True
    
    def search_from_xgqdetail(self, keyword):
        trainer_details = self.load_json_content("xgqdetail.json")
        if trainer_details:
            for entry in trainer_details:
                if "id" in entry and (keyword in entry["keyw"] or (len(keyword) >= 2 and keyword.lower() in entry["en_name"].lower())):
                    full_url = ""
                    anti_url = ""
                    if settings["language"] == "en_US" or settings["enSearchResults"]:
                        trainerDisplayName = f"{entry['en_name']} Trainer"
                    elif settings["language"] == "zh_CN" or settings["language"] == "zh_TW":
                        pattern = r'\s(v[\d\.v\-]+.*|Early ?Access.*)'
                        trainerDisplayName = f"《{re.sub(pattern, '', entry['title'])}》修改器"

                    try:
                        # Construct download url, example: https://down.fucnm.com/Story.of.Seasons.A.Wonderful.Life.v1.0.Plus.24.Trainer-FLiNG.zip
                        base_url = "https://down.fucnm.com/"
                        trainer_name = entry["en_name"].replace(": ", ".").replace("：", ".").replace(",", "").replace("'", "").replace("’", "").replace("?", "").replace("/", ".").replace(" - ", ".").replace(" ", ".")
                        version = entry["version"]
                        if self.sanitize(version) == "earlyaccess":
                            version = "Early.Access"

                        countMatch = re.search(r"(\w+?)项", entry["keyv"])
                        if countMatch:
                            chinese_count = countMatch.group(1)
                            count = cn2an.cn2an(chinese_count, "smart")
                        count = f"Plus.{count}"

                        full_url = f"{base_url}{trainer_name}.{version}.{count}.Trainer-FLiNG.zip"

                        if "anti_url" in entry:
                            anti_url = entry["anti_url"]
                    except Exception as e:
                        self.message.emit(tr("Failed to get trainer url: ") + trainerDisplayName, "failure")
                        print(f"Constructing download url for {entry['keyw']} failed: {str(e)}")
                    
                    if trainerDisplayName and full_url:
                        DownloadBaseThread.trainer_urls[trainerDisplayName] = [full_url, anti_url]
        
        self.message.emit(tr("Search success!"), "success")
        time.sleep(0.5)
        return True

    def keyword_match(self, keywordList, targetString):
        def is_match(sanitized_keyword, sanitized_targetString):
            similarity_threshold = 80
            similarity = fuzz.partial_ratio(
                sanitized_keyword, sanitized_targetString)
            return similarity >= similarity_threshold

        sanitized_targetString = self.sanitize(
            targetString.rsplit(" Trainer", 1)[0])

        return any(is_match(self.sanitize(kw), sanitized_targetString) for kw in keywordList if len(kw) >= 2 and len(sanitized_targetString) >= 2)
