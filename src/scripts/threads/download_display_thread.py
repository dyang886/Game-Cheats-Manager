import concurrent.futures
import re
import time
from urllib.parse import urljoin, urlparse

from bs4 import BeautifulSoup
from fuzzywuzzy import fuzz

from config import *
from threads.download_base_thread import DownloadBaseThread


class DownloadDisplayThread(DownloadBaseThread):
    def __init__(self, keyword, parent=None):
        super().__init__(parent)
        self.keyword = keyword

    def run(self):
        DownloadBaseThread.trainer_urls = []
        keywordList = self.translate_keyword(self.keyword)
        self.message.emit(tr("Searching..."), None)

        # ======================================================
        # Fling
        status = self.search_from_fling_archive(keywordList)
        if not status:
            self.finished.emit(1)
            return
        status = self.search_from_fling_main(keywordList)
        if not status:
            self.finished.emit(1)
            return

        ignored_trainers = {
            "Dying Light The Following Enhanced Edition",
            "Monster Hunter World",
            "Street Fighter V",
            "World War Z"
        }

        # Filter and prioritize trainers from the main site
        filtered_trainer_urls = {}
        for trainer in DownloadBaseThread.trainer_urls:
            if trainer["game_name"] in ignored_trainers:
                continue

            norm_name = self.sanitize(trainer["game_name"])

            # Add trainer if not already present or replace only if from fling_main and current is from fling_archive
            if norm_name not in filtered_trainer_urls:
                filtered_trainer_urls[norm_name] = trainer
            elif trainer["origin"] == "fling_main" and filtered_trainer_urls[norm_name]["origin"] == "fling_archive":
                filtered_trainer_urls[norm_name] = trainer

        # Update trainer_urls with filtered and deduplicated results
        DownloadBaseThread.trainer_urls = list(filtered_trainer_urls.values())

        # ======================================================
        # XiaoXing
        if settings["enableXiaoXing"]:
            status = self.search_from_xiaoxing(keywordList)
            if not status:
                self.finished.emit(1)
                return

        # ======================================================
        # General
        self.message.emit(tr("Translating search results..."), None)
        with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
            futures = {
                executor.submit(self.translate_trainer, trainer["game_name"], trainer["origin"]): trainer
                for trainer in DownloadBaseThread.trainer_urls
            }

        # Update the translated names
        for future in concurrent.futures.as_completed(futures):
            trainer = futures[future]
            trainer["trainer_name"] = future.result()

        if not DownloadBaseThread.trainer_urls:
            self.message.emit(tr("No results found."), "failure")
            self.finished.emit(1)
            return

        # Sort based on translated names
        DownloadBaseThread.trainer_urls.sort(key=lambda trainer: sort_trainers_key(trainer["trainer_name"]))

        self.message.emit("", "clear")
        for count, trainer in enumerate(DownloadBaseThread.trainer_urls, start=1):
            self.message.emit(f"{count}. {trainer["trainer_name"]}", None)
            print(f"{count}. {trainer["game_name"]} | {trainer["trainer_name"]} | {trainer["url"]}")

        self.finished.emit(0)

    def translate_keyword(self, keyword):
        translations = [keyword]
        self.message.emit(tr("Translating keywords..."), None)

        # Load trainer translations from the JSON database
        trainer_details = self.load_json_content("translations.json")
        if trainer_details:
            for trainer in trainer_details:
                sanitized_keyword = self.sanitize(keyword)
                if is_chinese(keyword):
                    if sanitized_keyword in self.sanitize(trainer.get("zh_CN", "")):
                        translations.append(trainer.get("en_US", ""))
                else:
                    if sanitized_keyword in self.sanitize(trainer.get("en_US", "")):
                        translations.append(trainer.get("zh_CN", ""))

        translations = list(filter(None, set(translations)))
        print("\nKeyword translations:", translations, "\n")
        return translations

    def keyword_match(self, keywordList, targetString):
        def is_match(sanitized_keyword, sanitized_targetString):
            similarity_threshold = 80
            similarity = fuzz.partial_ratio(sanitized_keyword, sanitized_targetString)
            return similarity >= similarity_threshold

        sanitized_targetString = self.sanitize(targetString)

        return any(is_match(self.sanitize(kw), sanitized_targetString) for kw in keywordList if len(kw) >= 2 and len(sanitized_targetString) >= 2)

    def search_from_fling_archive(self, keywordList):
        page_content = self.load_html_content("fling_archive.html")
        archiveHTML = BeautifulSoup(page_content, 'html.parser')

        # Check if the request was successful
        if archiveHTML.find():
            self.message.emit(tr("Search from FLiNG success!") + " 1/2", "success")
        else:
            self.message.emit(tr("Search failed, please update FLiNG data."), "failure")
            return False

        for link in archiveHTML.find_all(target="_self"):
            # parse trainer name
            rawTrainerName = link.get_text()
            parsedTrainerName = re.sub(r' v[\d.]+.*|\.\bv.*| \d+\.\d+\.\d+.*| Plus\s\d+.*|Build\s\d+.*|(\d+\.\d+-Update.*)|Update\s\d+.*|\(Update\s.*| Early Access .*|\.Early.Access.*', '', rawTrainerName).replace("_", ": ")
            gameName = parsedTrainerName.strip()

            # search algorithm
            url = urljoin("https://archive.flingtrainer.com/", link.get("href")) if settings["flingDownloadServer"] == "official" else f"GCM/Fling Trainers/{os.path.basename(urlparse(link.get('href')).path)}"
            if self.keyword_match(keywordList, gameName):
                DownloadBaseThread.trainer_urls.append({
                    "game_name": gameName,
                    "trainer_name": None,
                    "origin": "fling_archive",
                    "url": url
                })

        return True

    def search_from_fling_main(self, keywordList):
        # Search for results from fling main site, prioritized, will replace same trainer from archive
        page_content = self.load_html_content("fling_main.html")
        mainSiteHTML = BeautifulSoup(page_content, 'html.parser')

        if mainSiteHTML.find():
            self.message.emit(tr("Search from FLiNG success!") + " 2/2", "success")
        else:
            self.message.emit(tr("Search failed, please update FLiNG data."), "failure")
            return False
        time.sleep(0.5)

        for ul in mainSiteHTML.find_all('ul'):
            for li in ul.find_all('li'):
                for link in li.find_all('a'):
                    rawTrainerName = link.get_text()
                    gameName = rawTrainerName.strip().rsplit(" Trainer", 1)[0]
                    if gameName in ["Home", "Trainers", "Log In", "All Trainers (A-Z)", "All", "Privacy Policy"]:
                        continue

                    # search algorithm
                    url = link.get("href") if settings["flingDownloadServer"] == "official" else f"GCM/Fling Trainers/{self.symbol_replacement(rawTrainerName)}"
                    if gameName and self.keyword_match(keywordList, gameName):
                        DownloadBaseThread.trainer_urls.append({
                            "game_name": gameName,
                            "trainer_name": None,
                            "origin": "fling_main",
                            "url": url
                        })

        return True

    def search_from_xiaoxing(self, keywordList):
        page_content = self.load_html_content("xiaoxing.html")
        xiaoXingHTML = BeautifulSoup(page_content, 'html.parser')

        if xiaoXingHTML.find():
            self.message.emit(tr("Search from XiaoXing success!"), "success")
        else:
            self.message.emit(tr("Search failed, please update XiaoXing data."), "failure")
            return False
        time.sleep(0.5)

        for article in xiaoXingHTML.find_all('article'):
            link = article.find('a')
            rawTrainerName = link.get_text()

            pattern = r'辅助器.*|多功能.*|全版本.*|全功能.*|\s+\S*修改器.*|十二项修改器.*'
            match = re.search(pattern, rawTrainerName)
            if not match:
                continue
            gameName = re.sub(pattern, '', rawTrainerName).strip()

            # search algorithm
            if gameName and self.keyword_match(keywordList, gameName):
                DownloadBaseThread.trainer_urls.append({
                    "game_name": gameName,
                    "trainer_name": None,
                    "origin": "xiaoxing",
                    "url": link.get("href")
                })

        return True
