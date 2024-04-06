import concurrent.futures
import datetime
import gettext
import json
import locale
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from urllib.parse import urljoin, urlparse
import uuid
import zipfile

from bs4 import BeautifulSoup
from fuzzywuzzy import fuzz, process
import polib
from PyQt6.QtCore import Qt, QEventLoop, QThread, QTimer, pyqtSignal, QUrl
from PyQt6.QtGui import QIcon, QPixmap
from PyQt6.QtWebEngineCore import QWebEngineDownloadRequest
from PyQt6.QtWebEngineWidgets import QWebEngineView
from PyQt6.QtWidgets import QDialog, QCheckBox, QVBoxLayout, QHBoxLayout, QPushButton, QComboBox, QLabel, QLineEdit, QMessageBox, QFileDialog, QWidget
import requests
ts = None


def resource_path(relative_path):
    if hasattr(sys, "_MEIPASS"):
        full_path = os.path.join(sys._MEIPASS, relative_path)
    else:
        full_path = os.path.join(os.path.abspath("."), relative_path)

    if not os.path.exists(full_path):
        resource_name = os.path.basename(relative_path)
        formatted_message = tr("Couldn't find {missing_resource}. Please try reinstalling the application.").format(
            missing_resource=resource_name)
        QMessageBox.critical(
            None, tr("Missing resource file"), formatted_message)
        sys.exit(1)

    return full_path


def apply_settings(settings):
    with open(SETTINGS_FILE, "w") as f:
        json.dump(settings, f)


def load_settings():
    locale.setlocale(locale.LC_ALL, '')
    system_locale = locale.getlocale()[0]
    locale_mapping = {
        "English_United States": "en_US",
        "Chinese (Simplified)_China": "zh_CN",
        "Chinese (Simplified)_Hong Kong SAR": "zh_CN",
        "Chinese (Simplified)_Macao SAR": "zh_CN",
        "Chinese (Simplified)_Singapore": "zh_CN",
        "Chinese (Traditional)_Hong Kong SAR": "zh_TW",
        "Chinese (Traditional)_Macao SAR": "zh_TW",
        "Chinese (Traditional)_Taiwan": "zh_TW"
    }
    app_locale = locale_mapping.get(system_locale, 'en_US')

    default_settings = {
        "downloadPath": os.path.join(os.environ["APPDATA"], "GCM Trainers"),
        "language": app_locale,
        "theme": "black",
        "enSearchResults": False,
        "WeModPath": os.path.join(os.environ["LOCALAPPDATA"], "WeMod")
    }

    try:
        with open(SETTINGS_FILE, "r") as f:
            settings = json.load(f)
    except FileNotFoundError:
        settings = default_settings
    else:
        for key, value in default_settings.items():
            settings.setdefault(key, value)

    with open(SETTINGS_FILE, "w") as f:
        json.dump(settings, f)

    return settings


def get_translator():
    if not hasattr(sys, 'frozen'):
        for root, dirs, files in os.walk(resource_path("locale/")):
            for file in files:
                if file.endswith(".po"):
                    po = polib.pofile(os.path.join(root, file))
                    po.save_as_mofile(os.path.join(
                        root, os.path.splitext(file)[0] + ".mo"))

    lang = settings["language"]
    gettext.bindtextdomain("Game Cheats Manager",
                           resource_path("locale/"))
    gettext.textdomain("Game Cheats Manager")
    lang = gettext.translation(
        "Game Cheats Manager", resource_path("locale/"), languages=[lang])
    lang.install()
    return lang.gettext


setting_path = os.path.join(
    os.environ["APPDATA"], "GCM Settings/")
if not os.path.exists(setting_path):
    os.makedirs(setting_path)

SETTINGS_FILE = os.path.join(setting_path, "settings.json")
DATABASE_PATH = os.path.join(setting_path, "db")
DOWNLOAD_TEMP_DIR = os.path.join(tempfile.gettempdir(), "GameCheatsManagerTemp", "download")
VERSION_TEMP_DIR = os.path.join(tempfile.gettempdir(), "GameCheatsManagerTemp", "version")
os.makedirs(DATABASE_PATH, exist_ok=True)

resourceHacker_path = resource_path("dependency/ResourceHacker.exe")
unrar_path = resource_path("dependency/UnRAR.exe")
emptyMidi_path = resource_path("dependency/TrainerBGM.mid")

settings = load_settings()
tr = get_translator()

language_options = {
    "English (US)": "en_US",
    "简体中文": "zh_CN",
    "繁體中文": "zh_TW"
}

theme_options = {
    tr("Black"): "black",
    tr("white"): "white"
}


class SettingsDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle(tr("Settings"))
        self.setWindowIcon(QIcon(resource_path("assets/setting.ico")))
        settingsLayout = QVBoxLayout()
        settingsLayout.setSpacing(15)
        self.setLayout(settingsLayout)
        self.setMinimumWidth(370)

        settingsWidgetsLayout = QVBoxLayout()
        settingsWidgetsLayout.setContentsMargins(50, 30, 50, 20)
        settingsLayout.addLayout(settingsWidgetsLayout)

        # Theme selection
        themeLayout = QVBoxLayout()
        themeLayout.setSpacing(2)
        settingsWidgetsLayout.addLayout(themeLayout)
        themeLayout.addWidget(QLabel(tr("Theme:")))
        self.themeCombo = QComboBox()
        self.themeCombo.addItems(theme_options.keys())
        self.themeCombo.setCurrentText(
            self.find_settings_key(settings["theme"], theme_options))
        themeLayout.addWidget(self.themeCombo)

        # Language selection
        languageLayout = QVBoxLayout()
        languageLayout.setSpacing(2)
        settingsWidgetsLayout.addLayout(languageLayout)
        languageLayout.addWidget(QLabel(tr("Language:")))
        self.languageCombo = QComboBox()
        self.languageCombo.addItems(language_options.keys())
        self.languageCombo.setCurrentText(
            self.find_settings_key(settings["language"], language_options))
        languageLayout.addWidget(self.languageCombo)

        # Always show english
        alwaysEnLayout = QHBoxLayout()
        alwaysEnLayout.setSpacing(10)
        settingsWidgetsLayout.addLayout(alwaysEnLayout)
        alwaysEnlabel = QLabel(tr("Always show search results in English:"))
        alwaysEnLayout.addWidget(alwaysEnlabel)
        self.alwaysEncheckbox = QCheckBox()
        self.alwaysEncheckbox.setChecked(settings["enSearchResults"])
        alwaysEnLayout.addWidget(self.alwaysEncheckbox)

        # Wemod path
        wemodLayout = QVBoxLayout()
        wemodLayout.setSpacing(2)
        settingsWidgetsLayout.addLayout(wemodLayout)
        wemodLayout.addWidget(QLabel(tr("WeMod path:")))
        wemodPathLayout = QHBoxLayout()
        wemodPathLayout.setSpacing(5)
        wemodLayout.addLayout(wemodPathLayout)
        self.wemodLineEdit = QLineEdit()
        self.wemodLineEdit.setText(settings["WeModPath"])
        wemodPathLayout.addWidget(self.wemodLineEdit)
        wemodPathButton = QPushButton("...")
        wemodPathButton.clicked.connect(self.selectWeModPath)
        wemodPathLayout.addWidget(wemodPathButton)

        # Apply button
        applyButtonLayout = QHBoxLayout()
        applyButtonLayout.setContentsMargins(0, 0, 10, 10)
        applyButtonLayout.addStretch(1)
        settingsLayout.addLayout(applyButtonLayout)
        self.applyButton = QPushButton(tr("Apply"))
        self.applyButton.setFixedWidth(100)
        self.applyButton.clicked.connect(self.apply_settings_page)
        applyButtonLayout.addWidget(self.applyButton)
    
    def find_settings_key(self, value, dict):
        return next(key for key, val in dict.items() if val == value)

    def selectWeModPath(self):
        initialPath = self.wemodLineEdit.text() or os.path.expanduser("~")
        directory = QFileDialog.getExistingDirectory(
            self, tr("Select WeMod Installation Path"), initialPath)
        if directory:
            self.wemodLineEdit.setText(os.path.normpath(directory))
    
    def apply_settings_page(self):
        settings["theme"] = theme_options[self.themeCombo.currentText()]
        settings["language"] = language_options[self.languageCombo.currentText()]
        settings["enSearchResults"] = self.alwaysEncheckbox.isChecked()
        settings["WeModPath"] = self.wemodLineEdit.text()
        apply_settings(settings)
        QMessageBox.information(self, tr("Attention"), tr(
            "Please restart the application to apply theme and language settings."))


class AboutDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle(tr("About"))
        self.setWindowIcon(QIcon(resource_path("assets/logo.ico")))
        aboutLayout = QVBoxLayout()
        aboutLayout.setSpacing(30)
        aboutLayout.setContentsMargins(40, 20, 40, 40)
        self.setLayout(aboutLayout)

        appLayout = QHBoxLayout()
        appLayout.setSpacing(0)
        aboutLayout.addLayout(appLayout)

        # App logo
        logoPixmap = QPixmap(resource_path("assets/logo.png")).scaled(120, 120, Qt.AspectRatioMode.KeepAspectRatio)
        logoLabel = QLabel()
        logoLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        logoLabel.setPixmap(logoPixmap)
        appLayout.addWidget(logoLabel)

        # App name and version
        appNameFont = self.font()
        appNameFont.setPointSize(18)
        appInfoLayout = QVBoxLayout()
        appLayout.addLayout(appInfoLayout)
        appNameLabel = QLabel("Game Cheats Manager")
        appNameLabel.setFont(appNameFont)
        appNameLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        appInfoLayout.addWidget(appNameLabel)
        appVersionLabel = QLabel(tr("Version: ") + self.parent().appVersion)
        appVersionLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        appInfoLayout.addWidget(appVersionLabel)

        # Links
        githubUrl = self.parent().githubLink
        githubText = f'GitHub: <a href="{githubUrl}" style="text-decoration: none; color: #284fff;">{githubUrl}</a>'
        githubLabel = QLabel(githubText)
        githubLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        githubLabel.setTextFormat(Qt.TextFormat.RichText)
        githubLabel.setOpenExternalLinks(True)
        aboutLayout.addWidget(githubLabel)


class PathChangeThread(QThread):
    finished = pyqtSignal(str)
    error = pyqtSignal(str)

    def __init__(self, source_path, destination_folder, parent=None):
        super().__init__(parent)
        self.source_path = source_path
        self.destination_folder = destination_folder

    def run(self):
        try:
            dst = os.path.normpath(os.path.join(self.destination_folder, "GCM Trainers/"))
            if not os.path.exists(dst):
                os.makedirs(dst)

            for filename in os.listdir(self.source_path):
                src_file = os.path.join(self.source_path, filename)
                dst_file = os.path.join(dst, filename)
                if os.path.exists(dst_file):
                    os.remove(dst_file)
                shutil.move(src_file, dst_file)
            shutil.rmtree(self.source_path)

            self.finished.emit(dst)
        except Exception as e:
            self.error.emit(str(e))


class StatusMessageWidget(QWidget):
    def __init__(self, widgetName, message):
        super().__init__()
        self.setObjectName(widgetName)

        self.layout = QHBoxLayout()
        self.layout.setSpacing(3)
        self.setLayout(self.layout)

        self.messageLabel = QLabel(message)
        self.layout.addWidget(self.messageLabel)

        self.loadingLabel = QLabel(".")
        self.loadingLabel.setFixedWidth(20)
        self.layout.addWidget(self.loadingLabel)

        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_loading_animation)
        self.timer.start(500)

    def update_loading_animation(self):
        current_text = self.loadingLabel.text()
        new_text = '.' * ((len(current_text) % 3) + 1)
        self.loadingLabel.setText(new_text)

    def update_message(self, newMessage, state="load"):
        self.messageLabel.setText(newMessage)
        if state == "load":
            if not self.loadingLabel.isVisible():
                self.loadingLabel.show()
            if not self.timer.isActive():
                self.timer.start(500)
            self.messageLabel.setStyleSheet("")
        elif state == "error":
            self.timer.stop()
            self.loadingLabel.hide()
            self.messageLabel.setStyleSheet("QLabel { color: red; }")


class BrowserDialog(QDialog):
    content_ready = pyqtSignal(str)
    download_completed = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.browser = QWebEngineView(self)
        layout = QVBoxLayout(self)
        layout.addWidget(self.browser)
        self.setLayout(layout)
        self.resize(800, 600)
        self.setWindowTitle(tr("Please complete any security checks..."))
        self.setWindowIcon(QIcon(resource_path("assets/logo.ico")))

        self.check_timer = QTimer(self)
        self.found_content = None
        self.check_count = 0
        self.download_path = ""
        self.file_name = ""

    def load_url(self, url, target_text):
        self.url = url
        self.target_text = target_text
        self.found_content = False
        self.check_count = 0
        self.browser.loadFinished.connect(self.on_load_finished)
        self.browser.load(QUrl(self.url))
        self.hide()

    def on_load_finished(self, success):
        self.check_timer.timeout.connect(self.check_content)
        self.check_timer.start(500)

    def check_content(self):
        if self.check_count >= 5:
            self.show()
        self.check_count += 1
        self.browser.page().toHtml(self.handle_html)

    def handle_html(self, html):
        if self.target_text in html:
            self.found_content = True
            self.check_timer.stop()
            self.content_ready.emit(html)
            self.close()
    
    def closeEvent(self, event):
        if not self.found_content == None and not self.found_content:
            self.check_timer.stop()
            self.browser.loadFinished.disconnect(self.on_load_finished)
            self.content_ready.emit("")
        event.accept()
    
    def handle_download(self, url, download_path, file_name):
        self.download_path = download_path
        self.file_name = file_name
        self.browser.page().profile().downloadRequested.connect(self.on_download_requested)
        self.browser.load(QUrl(url))

    def on_download_requested(self, download):
        suggested_filename = download.downloadFileName()
        extension = os.path.splitext(suggested_filename)[1]
        file_name = self.file_name + extension

        download.setDownloadDirectory(self.download_path)
        download.setDownloadFileName(file_name)
        download.accept()

        file_path = os.path.join(self.download_path, file_name)
        download.stateChanged.connect(lambda state: self.on_download_state_changed(state, file_path))
    
    def on_download_state_changed(self, state, file_path):
        if state == QWebEngineDownloadRequest.DownloadState.DownloadCompleted:
            self.browser.page().profile().downloadRequested.disconnect(self.on_download_requested)
            self.download_completed.emit(file_path)
            self.close()


class DownloadBaseThread(QThread):
    message = pyqtSignal(str, str)
    messageBox = pyqtSignal(str, str, str)
    finished = pyqtSignal(int)
    loadUrl = pyqtSignal(str, str)
    downloadFile = pyqtSignal(str, str, str)

    trainer_urls = {}  # Store search results: {trainer name: download link}
    headers = {
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3',
    }
    translator_initialized = False
    
    def __init__(self, parent=None):
        super().__init__(parent)
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
        
        req = requests.get(url, headers=self.headers)

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
        req = requests.get(url, headers=self.headers)
        
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
                response = requests.get(url, timeout=timeout)
                response.raise_for_status()
                return True
            except requests.RequestException:
                continue
        return False

    def is_chinese(self, keyword):
        for char in keyword:
            if '\u4e00' <= char <= '\u9fff':
                return True
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
        return re.sub(r"[\-\s\"'‘’“”:：.。,，()（）<>《》;；!！?？@#$%^&™®_+*=~`|]", "", text).lower()
    
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
        try:
            global ts
            if ts is None:
                if not self.translator_initialized:
                    self.message.emit(tr("Initializing translator..."), None)
                    self.translator_initialized = True
                import translators as trans
                ts = trans
            ts.translate_text("test")
        except Exception as e:
            print("import translators failed or error occurred while translating: " + str(e))

    def translate_trainer(self, trainerName):
        # =======================================================
        # Special cases
        if trainerName == "Bright.Memory.Episode.1 Trainer":
            trainerName = "Bright Memory: Episode 1 Trainer"

        if "/" in trainerName:
            trainerName = trainerName.split("/")[0]
        trans_trainerName = trainerName

        if (settings["language"] == "zh_CN" or settings["language"] == "zh_TW") and not settings["enSearchResults"]:
            original_trainerName = trainerName.rsplit(" Trainer", 1)[0]

            try:
                # Using 3dm api to match en_names
                best_match = self.find_best_trainer_match(original_trainerName)
                if best_match:
                    if "/" in best_match:
                        best_match = best_match.split("/")[0]
                    trans_trainerName = f"《{best_match}》修改器"
            
                else:
                    found_translation = False
                    # Using Xiao Hei He search to get list of chinese names
                    xhhSearchUrl = f"https://api.xiaoheihe.cn/bbs/app/api/general/search/v1?search_type=game&q={original_trainerName}"
                    reqs = requests.get(xhhSearchUrl, headers=self.headers)
                    xhhData = reqs.json()

                    if reqs.status_code != 200:
                        self.message.emit(tr("Translation request failed with status code: ") + str(reqs.status_code), "failure")

                    steam_appids = [item['info']['steam_appid']
                                    for item in xhhData['result']['items']
                                    if 'steam_appid' in item['info']]

                    if steam_appids:
                        with concurrent.futures.ThreadPoolExecutor() as executor:
                            future_to_appid = {executor.submit(
                                self.xhh_zhName, appid, original_trainerName): appid for appid in steam_appids[:3]}

                            for future in concurrent.futures.as_completed(future_to_appid):
                                result = future.result()
                                if result != None:
                                    trans_trainerName = result
                                    found_translation = True
                                    break

                    # Use direct translation if couldn't find a match
                    if not found_translation:
                        self.initialize_translator()
                        print(
                            "No matches found, using direct translation for: " + original_trainerName)
                        trans_trainerName = ts.translate_text(
                            original_trainerName, from_language='en', to_language='zh')

                    # strip any game names that have their english names
                    pattern = r'[A-Za-z0-9\s：&]+（([^\）]*)\）|\（[A-Za-z\s：&]+\）$'
                    trans_trainerName = re.sub(pattern, lambda m: m.group(
                        1) if m.group(1) else '', trans_trainerName)

                    # do not alter if game name ends with roman numerics
                    def is_roman_numeral(s):
                        return bool(re.match(r'^(?=[MDCLXVI])M?(CM|CD|D?C{0,3})(XC|XL|L?X{0,3})(IX|IV|V?I{0,3})$', s.strip()))

                    if not is_roman_numeral(trans_trainerName.split(" ")[-1]):
                        pattern = r'[A-Za-z\s：&]+$'
                        trans_trainerName = re.sub(pattern, '', trans_trainerName)

                    trans_trainerName = trans_trainerName.replace(
                        "《", "").replace("》", "")
                    trans_trainerName = f"《{trans_trainerName}》修改器"
            except Exception as e:
                print(f"An error occurred while translating trainer name: {str(e)}")

        return trans_trainerName
    
    def xhh_enName(self, appid):
        xhhGameDetailUrl = f"https://api.xiaoheihe.cn/game/get_game_detail/?h_src=game_rec_a&appid={appid}"
        reqs = requests.get(xhhGameDetailUrl, headers=self.headers)
        xhhData = reqs.json()

        if reqs.status_code != 200:
            self.message.emit(tr("Translation request failed with status code: ") + str(reqs.status_code), "failure")

        if 'name_en' in xhhData['result']:
            return xhhData['result']['name_en']
        else:
            return None

    def xhh_zhName(self, appid, trainerName):
        xhhGameDetailUrl = f"https://api.xiaoheihe.cn/game/get_game_detail/?h_src=game_rec_a&appid={appid}"
        reqs = requests.get(xhhGameDetailUrl, headers=self.headers)
        xhhData = reqs.json()

        if reqs.status_code != 200:
            self.message.emit(tr("Translation request failed with status code: ") + str(reqs.status_code), "failure")

        if 'name_en' in xhhData['result']:
            enName = xhhData['result']['name_en']
            sanitized_enName = self.sanitize(enName)
            sanitized_trainerName = self.sanitize(trainerName)
            match = sanitized_enName == sanitized_trainerName
            # print("Match:", sanitized_trainerName, "&", sanitized_enName, "=>", match)

            zhName = xhhData['result']['name']
            if match and self.is_chinese(zhName):
                return zhName
            else:
                return None
    
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

        
class UpdateTrainers(DownloadBaseThread):
    message = pyqtSignal(str, str)
    update = pyqtSignal(str, str)
    finished = pyqtSignal(str)

    def __init__(self, trainers, parent=None):
        super().__init__(parent)
        self.trainers = trainers

    def run(self):
        statusWidgetName = "trainerUpdate"
        self.message.emit(statusWidgetName, tr("Checking for trainer updates"))

        with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
            futures = [executor.submit(self.process_trainer, trainerPath) for trainerPath in self.trainers.values()]

            for future in concurrent.futures.as_completed(futures):
                result = future.result()
                if result:
                    trainerPath, update_url = result
                    self.update.emit(trainerPath, update_url)
        
        self.finished.emit(statusWidgetName)

    def process_trainer(self, trainerPath):
        # The binary pattern representing "FLiNGTrainerNamedPipe_" followed by some null bytes and the date
        pattern_hex = '46 00 4c 00 69 00 4E 00 47 00 54 00 72 00 61 00 69 00 6E 00 65 00 72 00 4E 00 61 00 6D 00 65 00 64 00 50 00 69 00 70 00 65 00 5F'
        pattern = bytes.fromhex(''.join(pattern_hex.split()))
        tagName = self.get_product_name(trainerPath)

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
                    trainerSrcDate = datetime.datetime.strptime(date_match.group().decode('utf-8'), '%b %d %Y')

                    page_content = self.get_webpage_content(f"https://flingtrainer.com/tag/{tagName}", "FLiNG Trainer")
                    tagPage = BeautifulSoup(page_content, 'html.parser')

                    trainerNamesMap = {}  # trainerName: gameContentEntryOnWeb
                    for game in tagPage.find_all('div', class_='post-content'):
                        trainerName = self.sanitize(game.find('a', rel='bookmark').text)
                        trainerNamesMap[trainerName] = game

                    if trainerNamesMap:
                        best_match, score = process.extractOne(self.sanitize(tagName + "-trainer"), trainerNamesMap.keys())
                        if score >= 85:
                            targetGameOgj = trainerNamesMap[best_match]
                            version_entry = targetGameOgj.find('div', class_='entry')
                            
                            if version_entry:
                                match = re.search(r'Last Updated:\s+(\d+\.\d+\.\d+)', version_entry.text)
                                if match:
                                    trainerDstDate = datetime.datetime.strptime(match.group(1), '%Y.%m.%d')
                                    print(f"{tagName}\nTrainer source date: {trainerSrcDate.strftime('%Y-%m-%d')}\nNewest build date: {trainerDstDate.strftime('%Y-%m-%d')}\n")
                                    
                                    if trainerDstDate > trainerSrcDate:
                                        update_url = targetGameOgj.find('a', href=True, rel='bookmark')['href']
                                        return trainerPath, update_url
        return None
    
    def get_product_name(self, trainerPath):
        os.makedirs(VERSION_TEMP_DIR, exist_ok=True)
        unique_id = uuid.uuid4().hex
        temp_version_info = os.path.join(VERSION_TEMP_DIR, f"version_info_{unique_id}.rc")

        # Command to extract version information using Resource Hacker
        command = [resourceHacker_path, "-open", trainerPath, "-save",
                   temp_version_info, "-action", "extract", "-mask", "VERSIONINFO,,"]
        subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, creationflags=subprocess.CREATE_NO_WINDOW)

        product_name = None
        with open(temp_version_info, 'r', encoding='utf-16') as file:
            file_content = file.read()

        match = re.search(r'VALUE "ProductName", "(.*?)"', file_content)
        if match:
            product_name = match.group(1)
        os.remove(temp_version_info)

        # Parse only the game name
        tag_name = None
        match = re.search(r'^(.*?)(\s+v\d+|\s+Early Access)', product_name)
        if match:
            tag_name = match.group(1).lower().replace("(", "").replace(")", "").replace(" ", "-")

        return tag_name


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
            total_pages_response = requests.get(index_page, headers=self.headers)
            if total_pages_response.status_code == 200:
                total_pages = total_pages_response.json().get("page", "")

        if total_pages:
            with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
                futures = [executor.submit(self.fetch_page, page) for page in range(1, total_pages + 1)]
                all_data = []
                for future in concurrent.futures.as_completed(futures):
                    result = future.result()
                    if result:
                        all_data.extend(result)
            
            # Special cases/additions
            additions = [
                {
                    "en_name": "Assassins Creed Libertation Remastered",
                    "keyw": "刺客信条：解放-重制版",
                },
                {
                    "en_name": "All Zombies Must Die!",
                    "keyw": "僵尸必须死",
                },
                {
                    "en_name": "Tale of Immortal",
                    "keyw": "鬼谷八荒",
                },
                {
                    "en_name": "Bright Memory: Episode 1",
                    "keyw": "光明记忆：第一章",
                },
                {
                    "en_name": "Monster Hunter World: Iceborne",
                    "keyw": "怪物猎人：世界-冰原",
                },
                {
                    "en_name": "Resident Evil 2",
                    "keyw": "生化危机2：重制版",
                },
                {
                    "en_name": "Resident Evil Operation Raccoon City",
                    "keyw": "生化危机：浣熊市行动",
                },
                {
                    "en_name": "Resident Evil: Biohazard HD Remaster",
                    "keyw": "生化危机HD重制版",
                },
                {
                    "en_name": "XCOM 2",
                    "keyw": "幽浮2",
                },
                {
                    "en_name": "Vampire’s Fall - Origins",
                    "keyw": "吸血鬼之殇：起源",
                },
                {
                    "en_name": "Dark Souls: Prepare To Die Edition",
                    "keyw": "黑暗之魂：受死版",
                },
                {
                    "en_name": "WARP",
                    "keyw": "弯曲",
                },
                {
                    "en_name": "Far Cry 3",
                    "keyw": "孤岛惊魂3",
                },
                {
                    "en_name": "Far Cry 5",
                    "keyw": "孤岛惊魂5",
                },
                {
                    "en_name": "Digimon Story Cyber Sleuth: Complete Edition",
                    "keyw": "数码宝贝物语：网路侦探骇客追忆",
                },
                {
                    "en_name": "The Legend of Heroes: Trails into Reverie",
                    "keyw": "英雄传说：创之轨迹",
                },
                {
                    "en_name": "Final Fantasy (Pixel Remaster)",
                    "keyw": "最终幻想 像素复刻版",
                },
                {
                    "en_name": "Final Fantasy II (Pixel Remaster)",
                    "keyw": "最终幻想2 像素复刻版",
                },
                {
                    "en_name": "Final Fantasy III (Pixel Remaster)",
                    "keyw": "最终幻想3 像素复刻版",
                },
                {
                    "en_name": "Final Fantasy IV (Pixel Remaster)",
                    "keyw": "最终幻想4 像素复刻版",
                },
                {
                    "en_name": "Final Fantasy V (Pixel Remaster)",
                    "keyw": "最终幻想5 像素复刻版",
                },
                {
                    "en_name": "Final Fantasy VI (Pixel Remaster)",
                    "keyw": "最终幻想6 像素复刻版",
                },
                {
                    "en_name": "Final Fantasy IV The After Years",
                    "keyw": "最终幻想4：月之归还",
                },
                {
                    "en_name": "Halo: The Master Chief Collection (Halo 2: Anniversary)",
                    "keyw": "光环：士官长合集（光环2：周年版）",
                },
                {
                    "en_name": "Halo: The Master Chief Collection (Halo: CE Anniversary)",
                    "keyw": "光环：士官长合集（光环：战斗进化周年版）",
                },
                {
                    "en_name": "Halo: The Master Chief Collection (Halo: Reach) Trainer",
                    "keyw": "光环：士官长合集（光环：致远星）",
                },
                {
                    "en_name": "Dishonored-The Knife of Dunwall",
                    "keyw": "耻辱 顿沃城之锋",
                },
                {
                    "en_name": "Mark of the Ninja",
                    "keyw": "忍者印记",
                },
                {
                    "en_name": "Risen 2 Dark Waters",
                    "keyw": "崛起2：黑暗水域",
                },
            ]
            all_data.extend(additions)

            filepath = os.path.join(DATABASE_PATH, "xgqdetail.json")
            with open(filepath, 'w', encoding='utf-8') as file:
                json.dump(all_data, file, ensure_ascii=False, indent=4)

        else:
            self.update.emit(statusWidgetName, fetch_error, "error")
            time.sleep(2)
        
        self.finished.emit(statusWidgetName)
    
    def fetch_page(self, page_number):
        trainer_detail_page = f"https://dl.fucnm.com/datafile/xgqdetail/list_{page_number}.txt"
        page_response = requests.get(trainer_detail_page, headers=self.headers)
        if page_response.status_code == 200:
            return page_response.json()
        else:
            print(f"Failed to fetch trainer detail page {page_number}")
            return None


class DownloadDisplayThread(DownloadBaseThread):
    def __init__(self, keyword, parent=None):
        super().__init__(parent)
        self.keyword = keyword

    def run(self):
        # Check for internet connection before search
        self.message.emit(tr("Checking for internet connection..."), None)
        if not self.is_internet_connected():
            self.message.emit(tr("No internet connection, search failed."), "failure")
            self.finished.emit(1)
            return
        
        DownloadBaseThread.trainer_urls = {}
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
            ]
            if original_name in ignored_trainers:
                continue

            # Normalize the name
            norm_name = original_name.replace(":", "").replace(".", "").replace(
                "'", "").replace("’", "").replace(" ", "").lower()
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

        # Sort the dict alphabetically
        DownloadBaseThread.trainer_urls = dict(sorted(DownloadBaseThread.trainer_urls.items()))

        # Display search results
        self.message.emit(tr("Translating search results..."), None)
        trainer_names = list(DownloadBaseThread.trainer_urls.keys())

        # Using a dictionary to store future and its corresponding original trainer name
        max_threads = 5
        with concurrent.futures.ThreadPoolExecutor(max_workers=max_threads) as executor:
            future_to_trainerName = {executor.submit(
                self.translate_trainer, trainerName): trainerName for trainerName in trainer_names}

        translated_names = {}
        for future in concurrent.futures.as_completed(future_to_trainerName):
            original_trainerName = future_to_trainerName[future]
            translated_trainerName = future.result()
            translated_names[original_trainerName] = translated_trainerName

        # Clear prior texts
        self.message.emit("", "clear")

        # Display results in the original order
        print("\nTrainer results in original name: ", trainer_names)
        for count, trainerName in enumerate(trainer_names, start=1):
            self.message.emit(f"{str(count)}. {translated_names[trainerName]}", None)

        if len(DownloadBaseThread.trainer_urls) == 0:
            self.message.emit(tr("No results found."), "failure")
            self.finished.emit(1)
            return
        
        self.finished.emit(0)

    def translate_keyword(self, keyword):
        translations = []
        if self.is_chinese(keyword):
            self.message.emit(tr("Translating keywords..."), None)

            # Using 3dm api to match cn_names
            trainer_details = self.load_json_content("xgqdetail.json")
            if trainer_details:
                for trainer in trainer_details:
                    if keyword in trainer.get("keyw", ""):
                        translations.append(trainer.get("en_name", ""))

            else:
                # Direct translation
                self.initialize_translator()
                services = ["bing"]
                for service in services:
                    try:
                        translated_keyword = ts.translate_text(
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

                # Using XiaoHeiHe search to get list of english names
                xhhSearchUrl = f"https://api.xiaoheihe.cn/bbs/app/api/general/search/v1?search_type=game&q={keyword}"
                reqs = requests.get(xhhSearchUrl, headers=self.headers)
                xhhData = reqs.json()
                steam_appids = []

                # XiaoHeiHe search
                if reqs.status_code != 200:
                    self.message.emit(tr("Translation request failed with status code: ") + str(reqs.status_code), "failure")
                    print("Xiao Hei He request failed")
                elif 'result' in xhhData and 'items' in xhhData['result']:
                    steam_appids = [item['info']['steam_appid']
                                    for item in xhhData['result']['items']
                                    if 'steam_appid' in item['info']]
                else:
                    print("No items found for Xiao Hei He request or incorrect JSON format")

                # Get every english game name from steam app id
                if steam_appids:
                    with concurrent.futures.ThreadPoolExecutor() as executor:
                        future_to_appid = {executor.submit(
                            self.xhh_enName, appid): appid for appid in steam_appids[:10]}

                        for future in concurrent.futures.as_completed(future_to_appid):
                            name_en = future.result()
                            if name_en:
                                translations.append(name_en)

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
            self.message.emit(tr("Search failed, please wait until all data is updated from FLiNG.") + " 1/2", "failure")
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
            self.message.emit(tr("Search failed, please wait until all data is updated from FLiNG.") + " 2/2", "failure")
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

    def keyword_match(self, keywordList, targetString):
        def is_match(sanitized_keyword, sanitized_targetString):
            similarity_threshold = 80
            similarity = fuzz.partial_ratio(
                sanitized_keyword, sanitized_targetString)
            return similarity >= similarity_threshold

        sanitized_targetString = self.sanitize(
            targetString.rsplit(" Trainer", 1)[0])

        return any(is_match(self.sanitize(kw), sanitized_targetString) for kw in keywordList if len(kw) >= 2 and len(sanitized_targetString) >= 2)


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

    def run(self):
        if os.path.exists(DOWNLOAD_TEMP_DIR):
            shutil.rmtree(DOWNLOAD_TEMP_DIR)
        
        if self.update:
            self.trainerName = os.path.splitext(os.path.basename(self.trainerPath))[0]
            self.message.emit(tr("Updating ") + self.trainerName + "...", None)

        # Trainer name check
        if not self.update:
            trainer_list = list(DownloadBaseThread.trainer_urls.items())
            filename = trainer_list[self.index][0]
            mFilename = filename.replace(':', ' -')
            if "/" in mFilename:
                mFilename = mFilename.split("/")[1]
            self.message.emit(tr("Translating trainer name..."), None)
            trans_mFilename = self.translate_trainer(mFilename)

            for trainerPath in self.trainers.keys():
                if mFilename in trainerPath or trans_mFilename in trainerPath:
                    self.message.emit(tr("Trainer already exists, aborted download."), "failure")
                    time.sleep(self.download_finish_delay)
                    self.finished.emit(1)
                    return
        else:
            mFilename = self.trainerName
            trans_mFilename = self.trainerName

        # Download trainer
        self.message.emit(tr("Downloading..."), None)
        try:
            # Additional trainer file extraction for trainers from main site
            if not self.update:
                targetUrl = DownloadBaseThread.trainer_urls[filename]
            else:
                targetUrl = self.updateUrl

            domain = urlparse(targetUrl).netloc
            if domain == "flingtrainer.com":
                page_content = self.get_webpage_content(targetUrl, "FLiNG Trainer")
                trainerPage = BeautifulSoup(page_content, 'html.parser')
                targetUrl = trainerPage.find(target="_self").get("href")
            
            os.makedirs(DOWNLOAD_TEMP_DIR, exist_ok=True)
            trainerTemp = self.request_download(targetUrl, DOWNLOAD_TEMP_DIR, mFilename)

        except Exception as e:
            self.message.emit(tr("An error occurred while downloading trainer: ") + str(e), "failure")
            time.sleep(self.download_finish_delay)
            self.finished.emit(1)
            return
        
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
            return

        # Extract compressed file and rename
        self.message.emit(tr("Decompressing..."), None)
        extension = os.path.splitext(trainerTemp)[1]
        try:
            if extension == ".rar":
                command = [unrar_path, "x", "-y", trainerTemp, DOWNLOAD_TEMP_DIR]
                subprocess.run(command, check=True,
                               creationflags=subprocess.CREATE_NO_WINDOW)
            elif extension == ".zip":
                with zipfile.ZipFile(trainerTemp, 'r') as zip_ref:
                    zip_ref.extractall(DOWNLOAD_TEMP_DIR)

        except Exception as e:
            self.message.emit(tr("An error occurred while extracting downloaded trainer: ") + str(e), "failure")
            time.sleep(self.download_finish_delay)
            self.finished.emit(1)
            return

        # Locate extracted .exe file
        cnt = 0
        gameRawName = None
        for filename in os.listdir(DOWNLOAD_TEMP_DIR):
            if "trainer" in filename.lower() and filename.endswith(".exe"):
                gameRawName = filename
            elif "trainer" not in filename.lower():
                cnt += 1

        # Warn user if extra files found
        if cnt > 0 and not self.update:
            self.messageBox.emit("info", tr("Attention"), tr("Please check folder for anti-cheat requirements!"))
            os.startfile(DOWNLOAD_TEMP_DIR)

        # Check if gameRawName is None
        if not gameRawName:
            self.message.emit(tr("Could not find the downloaded trainer file, please try turning your antivirus software off."), "failure")
            time.sleep(self.download_finish_delay)
            self.finished.emit(1)
            return

        os.makedirs(self.trainerDownloadPath, exist_ok=True)
        trainer_name = trans_mFilename + ".exe"
        source_file = os.path.join(DOWNLOAD_TEMP_DIR, gameRawName)
        destination_file = os.path.join(self.trainerDownloadPath, trainer_name)

        # remove fling trainer bg music
        self.message.emit(tr("Removing trainer background music..."), None)
        self.modify_fling_settings()  # by changing trainer settings
        # by removing directly from exe
        self.remove_bgMusic(source_file, ["MID", "MIDI"])

        try:
            shutil.move(source_file, destination_file)
            os.remove(trainerTemp)
            rhLog = os.path.join(DOWNLOAD_TEMP_DIR, "rh.log")
            if os.path.exists(rhLog):
                os.remove(rhLog)

        except PermissionError as e:
            self.message.emit(tr("Trainer is currently in use, please close any programs using the file and try again."), "failure")
            time.sleep(self.download_finish_delay)
            self.finished.emit(1)
            return
        except Exception as e:
            self.message.emit(tr("Could not find the downloaded trainer file, please try turning your antivirus software off."), "failure")
            time.sleep(self.download_finish_delay)
            self.finished.emit(1)
            return
        
        self.message.emit(tr("Download success!"), "success")
        time.sleep(self.download_finish_delay)
        self.finished.emit(0)
    
    def modify_fling_settings(self):
        # replace bg music in Documents folder
        username = os.getlogin()
        flingSettings_path = f"C:/Users/{username}/Documents/FLiNGTrainer"
        bgMusic_path = os.path.join(flingSettings_path, "TrainerBGM.mid")
        if os.path.exists(bgMusic_path):
            shutil.copyfile(emptyMidi_path, bgMusic_path)

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
                        file.write('OnLoadMusic=False\n')
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
