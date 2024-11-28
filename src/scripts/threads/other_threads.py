import concurrent.futures
import json
import os
import shutil
import stat
import time

from PyQt6.QtCore import QThread, pyqtSignal
import requests

from config import *
import db_additions
from threads.download_base_thread import DownloadBaseThread


class VersionFetchWorker(QThread):
    versionFetched = pyqtSignal(str)
    fetchFailed = pyqtSignal()

    def __init__(self, update_link, parent=None):
        super().__init__(parent)
        self.update_link = update_link

    def run(self):
        try:
            response = requests.get(self.update_link)
            response.raise_for_status()
            data = response.json()
            latest_version = data.get("tag_name", "").lstrip("v")
            self.versionFetched.emit(latest_version)
        except Exception as e:
            print(f"Error fetching latest version: {e}")
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

        self.message.emit(statusWidgetName, update_message1)
        url = "https://archive.flingtrainer.com/"
        page_content = self.get_webpage_content(url, "FLiNG Trainers Archive")
        if not page_content:
            self.update.emit(statusWidgetName, update_failed1, "error")
            time.sleep(2)
        else:
            self.save_html_content(page_content, "fling_archive.html")

        self.update.emit(statusWidgetName, update_message2, "load")
        url = "https://flingtrainer.com/all-trainers-a-z/"
        page_content = self.get_webpage_content(url, "All Trainers (A-Z)")
        if not page_content:
            self.update.emit(statusWidgetName, update_failed2, "error")
            time.sleep(2)
        else:
            self.save_html_content(page_content, "fling_main.html")

        self.finished.emit(statusWidgetName)


class FetchTrainerDetails(DownloadBaseThread):
    message = pyqtSignal(str, str)
    update = pyqtSignal(str, str, str)
    finished = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)

    def run(self):
        statusWidgetName = "details"
        fetch_message = tr("Fetching trainer translations")
        fetch_error = tr("Fetch trainer translations failed")

        self.message.emit(statusWidgetName, fetch_message)
        total_pages = ""

        if self.is_internet_connected():
            index_page = "https://dl.fucnm.com/datafile/xgqdetail/index.txt"
            try:
                total_pages_response = requests.get(index_page, headers=self.headers)
                if total_pages_response.status_code == 200:
                    response = total_pages_response.json()
                    total_pages = response.get("page", "")
                    print(f"Total trainer translations fetched: {response.get('total', 'null')}\n")
            except Exception as e:
                print(f"Error requesting {index_page}: {str(e)}")

        if total_pages:
            completed_pages = 0
            error = False
            self.update.emit(statusWidgetName, f"{fetch_message} ({completed_pages}/{total_pages})", "load")

            with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
                futures = [executor.submit(self.fetch_page, page) for page in range(1, total_pages + 1)]
                all_data = []
                for future in concurrent.futures.as_completed(futures):
                    result = future.result()
                    if result:
                        all_data.extend(result)
                    else:
                        error = True
                    completed_pages += 1
                    self.update.emit(statusWidgetName, f"{fetch_message} ({completed_pages}/{total_pages})", "load")

            if error:
                self.update.emit(statusWidgetName, fetch_error, "error")
                time.sleep(2)
                self.finished.emit(statusWidgetName)
                return
            
            all_data.extend(db_additions.additions)

            filepath = os.path.join(DATABASE_PATH, "xgqdetail.json")
            with open(filepath, 'w', encoding='utf-8') as file:
                json.dump(all_data, file, ensure_ascii=False, indent=4)

        else:
            self.update.emit(statusWidgetName, fetch_error, "error")
            time.sleep(2)
        
        self.finished.emit(statusWidgetName)
    
    def fetch_page(self, page_number):
        trainer_detail_page = f"https://dl.fucnm.com/datafile/xgqdetail/list_{page_number}.txt"
        try:
            page_response = requests.get(trainer_detail_page, headers=self.headers)
            if page_response.status_code == 200:
                return page_response.json()
        except Exception as e:
            print(f"Error requesting {trainer_detail_page}: {str(e)}")
            return None

        print(f"Failed to fetch trainer detail page {page_number}")
        return None
