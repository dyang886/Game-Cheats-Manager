import ctypes
import gettext
import json
import locale
import os
import re
import sys
import tempfile

import polib
from packaging.version import InvalidVersion, Version
import zhon.cedict as chinese_characters
from pypinyin import lazy_pinyin

try:
    from secret_config import *
except ImportError:
    SIGNED_URL_DOWNLOAD_ENDPOINT = SIGNED_URL_UPLOAD_ENDPOINT = ''
    VERSION_CHECKER_ENDPOINT = PATCH_PATTERNS_ENDPOINT = CLIENT_API_KEY = ''

    def signed_get(url, params, timeout):
        raise RuntimeError('Request signing is unavailable in this build')


APP_VERSION = "2.5.1-beta.1"


def parse_version(version):
    if not isinstance(version, str):
        return None

    try:
        return Version(version.strip())
    except InvalidVersion:
        return None


def is_newer_version(latest_version, current_version=APP_VERSION):
    latest = parse_version(latest_version)
    current = parse_version(current_version)
    return latest is not None and current is not None and latest > current


# All resources in development mode are relative to `src` folder
def resource_path(relative_path):
    # Pyinstaller
    if hasattr(sys, "_MEIPASS"):
        full_path = os.path.join(sys._MEIPASS, relative_path)
    # Others like Nuitka
    elif sys.argv[0].endswith('.exe'):
        full_path = os.path.join(os.path.dirname(sys.executable), relative_path)
    # Development
    else:
        if relative_path == "Updater.exe":
            full_path = os.path.join(os.path.dirname(__file__), '../../../Updater/target/release', relative_path)
        else:
            full_path = os.path.join(os.path.dirname(__file__), '..', relative_path)

    if not os.path.exists(full_path):
        formatted_message = (
            f"Couldn't find {os.path.basename(full_path)}. Please try reinstalling the application.\n"
            f"无法找到 {os.path.basename(full_path)}. 请尝试重新安装应用程序。"
        )
        ctypes.windll.user32.MessageBoxW(0, formatted_message, "Failure", 0x10)
        sys.exit(1)

    return full_path


def apply_settings(settings):
    with open(SETTINGS_FILE, "w", encoding="utf-8") as f:
        json.dump(settings, f, indent=4)


def load_settings():
    # Bound early so helpers that resolve defaults (e.g. `findCEInstallPath`) can read stored settings
    global settings

    locale.setlocale(locale.LC_ALL, '')
    system_locale = locale.getlocale()[0]
    # print(f"System locale: {system_locale}")
    locale_mapping = {
        "English_United States": "en_US",
        "Chinese (Simplified)_China": "zh_CN",
        "Chinese (Simplified)_Hong Kong SAR": "zh_CN",
        "Chinese (Simplified)_Macao SAR": "zh_CN",
        "Chinese (Simplified)_Singapore": "zh_CN",
        "Chinese (Traditional)_Hong Kong SAR": "zh_TW",
        "Chinese (Traditional)_Macao SAR": "zh_TW",
        "Chinese (Traditional)_Taiwan": "zh_TW",
        "German_Austria": "de_DE",
        "German_Belgium": "de_DE",
        "de_DE": "de_DE",
        "German_Italy": "de_DE",
        "German_Liechtenstein": "de_DE",
        "German_Luxembourg": "de_DE",
        "German_Switzerland": "de_DE"
    }
    app_locale = locale_mapping.get(system_locale, 'en_US')

    try:
        with open(SETTINGS_FILE, "r", encoding="utf-8") as f:
            settings = json.load(f)
    except Exception as e:
        print("Error loading settings json" + str(e))
        settings = {}

    default_settings = {
        "downloadPath": os.path.join(os.environ["APPDATA"], "GCM Trainers"),
        "language": app_locale,
        "theme": "dark",
        "enSearchResults": False,
        "sortByOrigin": True,
        "checkAppUpdate": True,
        "launchAppOnStartup": False,
        "showWarning": True,
        "lastSeenAnnouncementId": "",

        # Trainer management configs
        "enableGCM": True,
        "autoUpdateGCMData": True,
        "autoUpdateGCMTrainers": True,
        "removeFlingBgMusic": True,
        "autoUpdateFlingData": True,
        "autoUpdateFlingTrainers": True,
        "enableXiaoXing": True,
        "unlockXiaoXing": False,
        "autoUpdateXiaoXingData": True,
        "autoUpdateXiaoXingTrainers": True,
        "weModPath": wemod_install_path,
        "cePath": findCEInstallPath(),
        "enableCT": True,
        "autoUpdateCTData": True,
        "autoUpdateCTTrainers": True,
        "launchCTAsAdmin": False,
        "cevoPath": "",

        # Global trainer configs
        "autoUpdateTranslations": True,
        "launchAsAdmin": True,
        "safePath": True,
    }

    for key, value in default_settings.items():
        settings.setdefault(key, value)

    with open(SETTINGS_FILE, "w", encoding="utf-8") as f:
        json.dump(settings, f, indent=4)

    return settings


def get_translator():
    if not sys.argv[0].endswith('.exe'):
        for root, dirs, files in os.walk(resource_path("locale/")):
            for file in files:
                if file.endswith(".po"):
                    po = polib.pofile(os.path.join(root, file))
                    po.save_as_mofile(os.path.join(root, os.path.splitext(file)[0] + ".mo"))

    lang = settings["language"]
    gettext.bindtextdomain("Game Cheats Manager", resource_path("locale/"))
    gettext.textdomain("Game Cheats Manager")
    lang = gettext.translation("Game Cheats Manager", resource_path("locale/"), languages=[lang])
    lang.install()
    return lang.gettext


def is_chinese(text):
    for char in text:
        if char in chinese_characters.all:
            return True
    return False


def sort_trainers_key(name):
    if is_chinese(name):
        return " ".join(lazy_pinyin(name))
    return name


def sort_trainers_key_ignore_prefix(name):
    prefix_regex = re.compile(r"^\[.*?\]\s*")
    real_name = prefix_regex.sub("", name)

    if is_chinese(real_name):
        return " ".join(lazy_pinyin(real_name))
    return real_name


def ensure_trainer_download_path_is_valid():
    try:
        os.makedirs(settings["downloadPath"], exist_ok=True)
    except Exception:
        settings["downloadPath"] = os.path.join(os.environ["APPDATA"], "GCM Trainers")
        apply_settings(settings)
        os.makedirs(settings["downloadPath"], exist_ok=True)


def findWeModInstallPath():
    install_path = os.path.join(os.environ["LOCALAPPDATA"], "Wand")
    if os.path.exists(install_path):
        return install_path
    return os.path.join(os.environ["LOCALAPPDATA"], "WeMod")


def findCEInstallPath(gcm=False):
    # Cheat Engine downloaded through GCM lives in the trainer download path like any other entry
    download_path = settings.get("downloadPath", "")
    if download_path and os.path.isdir(download_path):
        try:
            for entry in sorted(os.scandir(download_path), key=lambda dirent: dirent.name):
                if entry.is_dir() and os.path.isfile(os.path.join(entry.path, CE_EXECUTABLE)):
                    return os.path.normpath(entry.path)
        except OSError as e:
            print(f"Error scanning for Cheat Engine in download path: {str(e)}")

    if gcm:
        return ""

    # Fall back to a system installation, preferring the newest version
    base_path = r'C:\Program Files'
    latest_version = []
    latest_path = ""

    if os.path.exists(base_path):
        for folder in os.listdir(base_path):
            if folder.startswith("Cheat Engine"):
                match = re.search(r"Cheat Engine (\d+(?:\.\d+)*)", folder)
                if match:
                    # Parse version into a list of integers (e.g., '7.5.1' -> [7, 5, 1])
                    version = list(map(int, match.group(1).split('.')))
                    while len(version) < 3:
                        version.append(0)
                    if version > latest_version:
                        latest_version = version
                        latest_path = os.path.join(base_path, folder)
                else:
                    latest_path = os.path.join(base_path, folder)

    return latest_path


def getCEExecutable():
    cePath = settings.get("cePath", "")
    if cePath:
        ceExecutable = os.path.join(cePath, CE_EXECUTABLE)
        if os.path.isfile(ceExecutable):
            return ceExecutable

    return ""


setting_path = os.path.join(os.environ["APPDATA"], "GCM Settings")
os.makedirs(setting_path, exist_ok=True)

SETTINGS_FILE = os.path.join(setting_path, "settings.json")
DATABASE_PATH = os.path.join(setting_path, "db")
os.makedirs(DATABASE_PATH, exist_ok=True)
DOWNLOAD_TEMP_DIR = os.path.join(tempfile.gettempdir(), "GameCheatsManagerTemp", "download")
VERSION_TEMP_DIR = os.path.join(tempfile.gettempdir(), "GameCheatsManagerTemp", "version")
WEMOD_TEMP_DIR = os.path.join(tempfile.gettempdir(), "GameCheatsManagerTemp", "wemod")

wemod_install_path = findWeModInstallPath()
CE_EXECUTABLE = "Cheat Engine.exe"

settings = load_settings()
tr = get_translator()

language_options = {
    "English (US)": "en_US",
    "简体中文": "zh_CN",
    "繁體中文": "zh_TW",
    "Deutsch": "de_DE"
}

theme_options = {
    tr("Dark"): "dark",
    tr("Light"): "light"
}

patch_methods = {
    tr("Yearly Subscription"): "yearly_sub",
    tr("Gifted Subscription"): "gifted_sub"
}

font_config = {
    "en_US": resource_path("assets/NotoSans-Regular.ttf"),
    "zh_CN": resource_path("assets/NotoSansSC-Regular.ttf"),
    "zh_TW": resource_path("assets/NotoSansTC-Regular.ttf"),
    "de_DE": resource_path("assets/NotoSans-Regular.ttf")
}

ensure_trainer_download_path_is_valid()
if settings["theme"] not in theme_options.values():
    settings["theme"] = "dark"
    apply_settings(settings)

if settings["theme"] == "dark":
    dropDownArrow_path = resource_path("assets/dropdown-white.png").replace("\\", "/")
elif settings["theme"] == "light":
    dropDownArrow_path = resource_path("assets/dropdown-black.png").replace("\\", "/")
checkMark_path = resource_path("assets/check-mark.png").replace("\\", "/")
upArrow_path = resource_path("assets/up.png").replace("\\", "/")
downArrow_path = resource_path("assets/down.png").replace("\\", "/")
leftArrow_path = resource_path("assets/left.png").replace("\\", "/")
rightArrow_path = resource_path("assets/right.png").replace("\\", "/")
unzip_path = resource_path("dependency/7z/7z.exe")
emptyMidi_path = resource_path("dependency/TrainerBGM.mid")
elevator_path = resource_path("dependency/Elevate.exe")
updater_path = resource_path("Updater.exe")

ARCHIVE_EXTENSIONS = (".zip", ".rar", ".7z")
DEFAULT_TRAINER_EXTENSIONS = (".exe", ".ct", ".cetrainer")
API_TIMEOUT = (5, 15)  # (connect, read) for every API call
