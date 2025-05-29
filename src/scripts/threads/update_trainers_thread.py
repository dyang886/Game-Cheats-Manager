import concurrent.futures
import os
import shutil
import time

from PyQt6.QtCore import pyqtSignal

from config import *
from threads.download_base_thread import DownloadBaseThread


class UpdateTrainers(DownloadBaseThread):
    message = pyqtSignal(str, str)
    update = pyqtSignal(str, str, str)
    updateTrainer = pyqtSignal(dict)
    finished = pyqtSignal(str)

    def __init__(self, trainers, parent=None):
        super().__init__(parent)
        self.trainers = trainers

    def check_trainer_update(self, trainer_path):
        """Check if a trainer needs an update and return update info if needed."""
        try:
            trainer_dir = os.path.dirname(trainer_path)
            trainer_name = os.path.basename(trainer_dir)
            info_path = os.path.join(trainer_dir, "gcm_info.json")
            if not os.path.exists(info_path):
                return None
            info = self.load_json_content(info_path, False)
            game_name = info['game_name']
            origin = info['origin']
            current_version = info.get('version')
            if not current_version:
                return None

            origin_to_file = {
                "xiaoxing": "xiaoxing.json",
                "fling_main": "fling_main.json",
            }
            database = self.load_json_content(origin_to_file[origin])

            for entry in database:
                if entry['game_name'] == game_name:
                    new_version = entry['version']
                    gcm_url = entry['gcm_url']
                    if current_version and new_version != current_version:
                        return {
                            "game_name": game_name,
                            "trainer_name": trainer_name,
                            "trainer_dir": trainer_dir,
                            "origin": origin,
                            "url": gcm_url,
                            "version": new_version
                        }
                    return None
            return None

        except Exception as e:
            print(f"Error checking update for {trainer_name}: {str(e)}")
            return None

    def run(self):
        statusWidgetName = "trainerUpdate"
        self.message.emit(statusWidgetName, tr("Checking for trainer updates"))

        try:
            if os.path.exists(VERSION_TEMP_DIR):
                shutil.rmtree(VERSION_TEMP_DIR)

            futures = []
            with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
                for _, trainer_path in self.trainers.items():
                    future = executor.submit(self.check_trainer_update, trainer_path)
                    futures.append(future)

                for future in concurrent.futures.as_completed(futures):
                    result = future.result()
                    if result:
                        self.updateTrainer.emit(result)

        except Exception as e:
            print(f"Error during trainer update check: {str(e)}")
            self.update.emit(statusWidgetName, tr("Check trainer updates failed"), "error")
            time.sleep(2)

        time.sleep(0.5)
        self.finished.emit(statusWidgetName)
