import concurrent.futures
import ctypes
import gettext
import json
import locale
import os
import re
import shutil
import subprocess
import sys
import tempfile
import threading
import time
import tkinter as tk
from tkinter import filedialog, font, messagebox, ttk
from urllib.parse import urljoin, urlparse
import zipfile

from bs4 import BeautifulSoup
from fuzzywuzzy import fuzz
from PIL import Image, ImageTk
import polib
import requests
import sv_ttk
from tendo import singleton
ts = None


def resource_path(relative_path):
    if hasattr(sys, "_MEIPASS"):
        return os.path.join(sys._MEIPASS, relative_path)
    return os.path.join(os.path.abspath("."), relative_path)


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
        "Chinese (Simplified)_Singapore": "zh_CN"
    }
    app_locale = locale_mapping.get(system_locale, 'en_US')

    default_settings = {
        "downloadPath": os.path.join(os.environ["APPDATA"], "GCM Trainers/"),
        "language": app_locale,
        "enSearchResults": False,
        "WeModPath": os.path.join(os.environ["LOCALAPPDATA"], "WeMod/")
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
    # compile .po files to .mo files
    for root, dirs, files in os.walk(resource_path("locale/")):
        for file in files:
            if file.endswith(".po"):
                po = polib.pofile(os.path.join(root, file))
                po.save_as_mofile(os.path.join(
                    root, os.path.splitext(file)[0] + ".mo"))

    # read settings and apply languages
    lang = settings["language"]
    gettext.bindtextdomain("Game Cheats Manager",
                           resource_path("locale/"))
    gettext.textdomain("Game Cheats Manager")
    lang = gettext.translation(
        "Game Cheats Manager", resource_path("locale/"), languages=[lang])
    lang.install()
    return lang.gettext


SETTINGS_FILE = os.path.join(
    os.environ["APPDATA"], "GCM Settings/", "settings.json")

setting_path = os.path.join(
    os.environ["APPDATA"], "GCM Settings/")
if not os.path.exists(setting_path):
    os.makedirs(setting_path)

language_options = {
    "English (US)": "en_US",
    "简体中文": "zh_CN"
}

ctypes.windll.shcore.SetProcessDpiAwareness(1)
settings = load_settings()
_ = get_translator()


class GameCheatsManager(tk.Tk):

    def __init__(self):
        super().__init__()

        # Single instance check and basic UI setup
        try:
            self.single_instance_checker = singleton.SingleInstance()
        except singleton.SingleInstanceException:
            sys.exit(1)
        self.title("Game Cheats Manager")
        self.iconbitmap(resource_path("assets/logo.ico"))
        sv_ttk.set_theme("dark")
        self.resizable(False, False)

        # Version, user prompts, and links
        self.appVersion = "1.2.7"
        self.githubLink = "https://github.com/dyang886/Game-Cheats-Manager"
        self.trainerSearchEntryPrompt = _("Search for installed")
        self.downloadSearchEntryPrompt = _("Search to download")

        # Paths and variable management
        settings = load_settings()  # Load settings once and use it throughout
        self.trainerPath = os.path.normpath(
            os.path.abspath(settings["downloadPath"]))
        os.makedirs(self.trainerPath, exist_ok=True)
        self.tempDir = os.path.join(
            tempfile.gettempdir(), "GameCheatsManagerTemp/")
        self.crackFilePath = resource_path("dependency/app.asar")
        self.trainers = {}  # Store installed trainers: {trainer name: trainer path}
        self.trainer_urls = {}  # Store search results: {trainer name: download link}

        # Networking
        self.headers = {
            'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3'
        }

        # Window references
        self.settings_window = None
        self.about_window = None

        # Widget fonts and styles
        font_config = {
            "en_US": ("Noto Sans", "assets/NotoSans-Regular.ttf"),
            "zh_CN": ("Noto Sans SC", "assets/NotoSansSC-Regular.ttf")
        }

        def is_font_installed(font_name):
            return font_name in font.families()

        font_name, font_path = font_config[settings["language"]]
        if font_path and not is_font_installed(font_name):
            ctypes.windll.gdi32.AddFontResourceW(resource_path(font_path))
            # Broadcasts a system-wide message if a new font has been added
            ctypes.windll.user32.SendMessageW(0xffff, 0x1D, 0, 0)

        self.default_font = (font_name, 10)
        self.menu_font = (font_name, 9)
        style = ttk.Style()
        style.configure("TButton", font=self.default_font, padding=8)
        style.configure("TEntry", padding=6)
        style.configure("TCombobox", padding=6)

        # Menu bar
        self.menuBar = tk.Frame(self, background="#2e2e2e")
        self.settingMenuBtn = tk.Menubutton(
            self.menuBar, text=_("Options"), background="#2e2e2e", font=self.menu_font)
        self.settingsMenu = tk.Menu(self.settingMenuBtn, tearoff=0, font=self.menu_font)
        self.settingsMenu.add_command(
            label=_("Settings"), command=self.open_settings)
        self.settingsMenu.add_command(
            label=("WeMod Pro"), command=self.wemod_pro)
        self.settingsMenu.add_command(
            label=_("About"), command=self.open_about)
        self.settingMenuBtn.config(menu=self.settingsMenu)
        self.settingMenuBtn.pack(side="left")
        self.menuBar.grid(row=0, column=0, sticky="ew")

        # Main frame
        self.frame = ttk.Frame(self)
        self.frame.grid(row=1, column=0, padx=50, pady=(30, 50))

        # ===========================================================================
        # column 1 - trainers
        # ===========================================================================
        self.flingFrame = ttk.Frame(self.frame)
        self.flingFrame.grid(row=0, column=0)

        self.trainerSearchFrame = ttk.Frame(self.flingFrame)
        self.trainerSearchFrame.grid(row=0, column=0, pady=(0, 20))

        self.searchPhoto = tk.PhotoImage(
            file=resource_path("assets/search.png"))
        search_width = 27
        search_height = 27
        search_width_ratio = self.searchPhoto.width() // search_width
        search_height_ratio = self.searchPhoto.height() // search_height
        self.searchPhoto = self.searchPhoto.subsample(
            search_width_ratio, search_height_ratio)
        self.searchLabel = ttk.Label(
            self.trainerSearchFrame, image=self.searchPhoto)
        self.searchLabel.pack(side=tk.LEFT, padx=(0, 10))

        self.trainerSearch_var = tk.StringVar()
        self.trainerSearchEntry = ttk.Entry(
            self.trainerSearchFrame, textvariable=self.trainerSearch_var, style="TEntry", font=self.default_font)
        self.trainerSearchEntry.pack()
        self.trainerSearchEntry.insert(0, self.trainerSearchEntryPrompt)
        self.trainerSearchEntry.config(foreground="grey")
        self.trainerSearch_var.trace_add('write', self.update_list)

        # scroll bar and list box
        self.flingVScrollbar = ttk.Scrollbar(
            self.flingFrame, orient="vertical")
        self.flingVScrollbar.grid(row=1, column=1, sticky='ns')

        self.flingHScrollbar = ttk.Scrollbar(
            self.flingFrame, orient="horizontal")
        self.flingHScrollbar.grid(row=2, column=0, sticky='ew')

        self.flingListBox = tk.Listbox(self.flingFrame, highlightthickness=0,
                                       activestyle="none", width=33, height=13,
                                       font=self.default_font,
                                       yscrollcommand=self.flingVScrollbar.set,
                                       xscrollcommand=self.flingHScrollbar.set)
        self.flingListBox.grid(row=1, column=0)

        self.flingVScrollbar.config(command=self.flingListBox.yview)
        self.flingHScrollbar.config(command=self.flingListBox.xview)

        # bottom buttons
        self.bottomFrame = ttk.Frame(self.flingFrame)
        self.bottomFrame.grid(row=3, column=0, pady=(10, 0), sticky="ew")
        self.bottomFrame.columnconfigure(0, weight=1)
        self.bottomFrame.columnconfigure(1, weight=1)

        self.launchButton = ttk.Button(
            self.bottomFrame, text=_("Launch"), width=11, command=self.launch_trainer, style="TButton")
        self.launchButton.grid(row=0, column=0, padx=(0, 9), sticky="ew")

        self.deleteButton = ttk.Button(
            self.bottomFrame, text=_("Delete"), width=11, command=self.delete_trainer, style="TButton")
        self.deleteButton.grid(row=0, column=1, padx=(9, 0), sticky="ew")

        # ===========================================================================
        # column 2 - downloads
        # ===========================================================================
        self.downloadFrame = ttk.Frame(self.frame)
        self.downloadFrame.grid(row=0, column=1, padx=(30, 0))

        self.downloadSearchFrame = ttk.Frame(self.downloadFrame)
        self.downloadSearchFrame.grid(row=0, column=0, pady=(0, 20))

        self.downloadSearchLabel = ttk.Label(
            self.downloadSearchFrame, image=self.searchPhoto)
        self.downloadSearchLabel.pack(side=tk.LEFT, padx=(0, 10))

        self.downloadSearch_var = tk.StringVar()
        self.downloadSearchEntry = ttk.Entry(
            self.downloadSearchFrame, textvariable=self.downloadSearch_var, style="TEntry", font=self.default_font)
        self.downloadSearchEntry.pack()
        self.downloadSearchEntry.insert(0, self.downloadSearchEntryPrompt)
        self.downloadSearchEntry.config(foreground="grey")

        # scroll bar and list box
        self.downloadVScrollbar = ttk.Scrollbar(
            self.downloadFrame, orient="vertical")
        self.downloadVScrollbar.grid(row=1, column=1, sticky='ns')

        self.downloadHScrollbar = ttk.Scrollbar(
            self.downloadFrame, orient="horizontal")
        self.downloadHScrollbar.grid(row=2, column=0, sticky='ew')

        self.downloadListBox = tk.Listbox(self.downloadFrame, highlightthickness=0,
                                          activestyle="none", width=33, height=13,
                                          font=self.default_font,
                                          yscrollcommand=self.downloadVScrollbar.set,
                                          xscrollcommand=self.downloadHScrollbar.set)
        self.downloadListBox.grid(row=1, column=0)

        self.downloadVScrollbar.config(command=self.downloadListBox.yview)
        self.downloadHScrollbar.config(command=self.downloadListBox.xview)

        # bottom change download path
        self.changeDownloadPath = ttk.Frame(self.downloadFrame)
        self.changeDownloadPath.grid(row=3, column=0, pady=(10, 0), sticky="ew")
        self.changeDownloadPath.columnconfigure(0, weight=3)
        self.changeDownloadPath.columnconfigure(1, weight=1)

        self.downloadPathText = tk.StringVar()
        self.downloadPathText.set(self.trainerPath)
        self.downloadPathEntry = ttk.Entry(
            self.changeDownloadPath, 
            state="readonly", 
            textvariable=self.downloadPathText, 
            style="TEntry", 
            font=self.default_font
        )
        self.downloadPathEntry.grid(row=0, column=0, sticky="ew", padx=(0, 15))

        self.fileDialogButton = ttk.Button(
            self.changeDownloadPath, 
            text="...", 
            command=self.create_migration_thread, 
            style="TButton"
        )
        self.fileDialogButton.grid(row=0, column=1, sticky="ew")

        self.trainerSearchEntry.bind('<FocusIn>', self.on_trainer_entry_click)
        self.trainerSearchEntry.bind(
            '<FocusOut>', self.on_trainer_entry_focusout)
        self.flingListBox.bind('<Double-Button-1>', self.launch_trainer)
        self.downloadSearchEntry.bind(
            '<FocusIn>', self.on_download_entry_click)
        self.downloadSearchEntry.bind(
            '<FocusOut>', self.on_download_entry_focusout)
        self.downloadSearchEntry.bind('<Return>', self.on_enter_press)
        self.downloadListBox.bind(
            '<Double-Button-1>', self.on_download_double_click)

        self.show_cheats()
        self.eval('tk::PlaceWindow . center')
        self.mainloop()

    # ===========================================================================
    # menu bar windows
    # ===========================================================================
    def center_window(self, window):
        window.update()
        main_x = self.winfo_x()
        main_y = self.winfo_y()
        main_width = self.winfo_width()
        main_height = self.winfo_height()
        window_width = window.winfo_width()
        window_height = window.winfo_height()
        center_x = int(main_x + (main_width - window_width) / 2)
        center_y = int(main_y + (main_height - window_height) / 2)
        window.geometry(f"+{center_x}+{center_y}")

    def apply_settings_page(self):
        settings["language"] = language_options[self.languages_var.get()]
        settings["enSearchResults"] = self.en_results_var.get()
        settings["WeModPath"] = self.wemod_path_var.get()
        apply_settings(settings)
        messagebox.showinfo(_("Attention"), _(
            "Please restart the application to apply settings"))

    def open_settings(self):
        if self.settings_window is None or not self.settings_window.winfo_exists():
            self.settings_window = tk.Toplevel(self)
            self.settings_window.title(_("Settings"))
            self.settings_window.iconbitmap(
                resource_path("assets/setting.ico"))
            self.settings_window.transient(self)

            settings_frame = ttk.Frame(self.settings_window)
            settings_frame.grid(row=0, column=0, sticky='nsew',
                                padx=(80, 80), pady=(50, 20))

            # ===========================================================================
            # languages frame
            languages_frame = ttk.Frame(settings_frame)
            languages_frame.grid(row=0, column=0, sticky="we")
            languages_frame.grid_columnconfigure(0, weight=1)

            languages_label = ttk.Label(
                languages_frame,
                text=_("Language:"),
                font=self.default_font
            )
            languages_label.grid(row=0, column=0, sticky="w")

            self.languages_var = tk.StringVar()
            for key, value in language_options.items():
                if value == settings["language"]:
                    self.languages_var.set(key)

            languages_combobox = ttk.Combobox(
                languages_frame,
                textvariable=self.languages_var,
                values=list(language_options.keys()),
                state="readonly",
                font=self.default_font,
                style="TCombobox"
            )
            languages_combobox.grid(row=1, column=0, sticky="we")

            # ===========================================================================
            # english search results frame
            en_results_frame = ttk.Frame(settings_frame)
            en_results_frame.grid(row=1, column=0, pady=(30, 0), sticky="we")
            en_results_frame.grid_columnconfigure(0, weight=1)

            en_results_label = ttk.Label(
                en_results_frame,
                text=_("Always show search results in English:"),
                wraplength=400,
                font=self.default_font
            )
            en_results_label.grid(row=0, column=0, sticky="w")

            self.en_results_var = tk.BooleanVar()
            self.en_results_var.set(settings["enSearchResults"])
            en_results_checkbox = ttk.Checkbutton(
                en_results_frame,
                variable=self.en_results_var
            )
            en_results_checkbox.grid(row=0, column=1, padx=(10, 0))

            # ===========================================================================
            # WeMod path frame
            wemod_path_frame = ttk.Frame(settings_frame)
            wemod_path_frame.grid(row=2, column=0, pady=(30, 0), sticky="we")
            wemod_path_frame.grid_columnconfigure(0, weight=1)

            wemod_path_label = ttk.Label(
                wemod_path_frame,
                text=_("WeMod installation path:"),
                font=self.default_font
            )
            wemod_path_label.grid(row=0, column=0, sticky="w")

            self.wemod_path_var = tk.StringVar()
            self.wemod_path_var.set(os.path.normpath(settings["WeModPath"]))
            wemod_path_entry = ttk.Entry(
                wemod_path_frame,
                state="readonly",
                style="TEntry",
                font=self.default_font,
                textvariable=self.wemod_path_var,
            )
            wemod_path_entry.grid(row=1, column=0, padx=(0, 10), sticky="we")

            wemod_path_button = ttk.Button(
                wemod_path_frame,
                text="...",
                command=self.change_wemod_path,
                style="TButton",
                width=2
            )
            wemod_path_button.grid(row=1, column=1)

            # ===========================================================================
            # apply button
            apply_button = ttk.Button(
                self.settings_window, text=_("Apply"),
                command=self.apply_settings_page,
                style="TButton",
                width=10
            )
            apply_button.grid(row=3, column=0, padx=(
                0, 30), pady=(30, 30), sticky="e")

            self.center_window(self.settings_window)
        else:
            self.settings_window.lift()
            self.settings_window.focus_force()

    def open_about(self):
        if self.about_window is None or not self.about_window.winfo_exists():
            self.about_window = tk.Toplevel(self)
            self.about_window.title(_("About"))
            self.about_window.iconbitmap(
                resource_path("assets/logo.ico"))
            self.about_window.transient(self)

            about_frame = ttk.Frame(self.about_window)
            about_frame.grid(row=0, column=0, sticky='nsew',
                             padx=(70, 70), pady=(50, 70))

            # ===========================================================================
            # app logo
            original_image = Image.open(resource_path("assets/logo.png"))
            resized_image = original_image.resize((150, 150))
            logo_image = ImageTk.PhotoImage(resized_image)
            logo_label = ttk.Label(about_frame, image=logo_image)
            logo_label.image = logo_image  # Keep a reference
            logo_label.grid(row=0, column=0)

            # ===========================================================================
            # app name and version
            appInfo_frame = ttk.Frame(about_frame)
            appInfo_frame.grid(row=0, column=1)

            app_name_label = ttk.Label(
                appInfo_frame, text="Game Cheats Manager", font=("Helvetica", 18))
            app_name_label.grid(row=0, column=0, pady=(0, 20))

            app_version_label = ttk.Label(
                appInfo_frame, text=_("Version: ") + self.appVersion, font=self.default_font)
            app_version_label.grid(row=1, column=0)

            # ===========================================================================
            # links
            def open_link(event, url):
                import webbrowser
                webbrowser.open_new(url)

            links_frame = ttk.Frame(about_frame)
            links_frame.grid(row=1, column=0, columnspan=2, pady=(40, 0))

            github_label = ttk.Label(links_frame, text="GitHub: ", font=self.default_font)
            github_label.pack(side=tk.LEFT)

            github_link = ttk.Label(
                links_frame, text=self.githubLink, cursor="hand2", foreground="#284fff", font=self.default_font)
            github_link.pack(side=tk.LEFT)
            github_link.bind(
                "<Button-1>", lambda e: open_link(e, self.githubLink))

            self.center_window(self.about_window)
        else:
            self.about_window.lift()
            self.about_window.focus_force()

    # ===========================================================================
    # event functions
    # ===========================================================================
    def on_trainer_entry_click(self, event):
        if self.trainerSearchEntry.get() == self.trainerSearchEntryPrompt:
            self.trainerSearchEntry.delete(0, tk.END)
            self.trainerSearchEntry.insert(0, "")
            self.trainerSearchEntry.config(foreground="white")

    def on_trainer_entry_focusout(self, event):
        if self.trainerSearchEntry.get() == "":
            self.trainerSearchEntry.insert(0, self.trainerSearchEntryPrompt)
            self.trainerSearchEntry.config(foreground="grey")

    def on_download_entry_click(self, event):
        if self.downloadSearchEntry.get() == self.downloadSearchEntryPrompt:
            self.downloadSearchEntry.delete(0, tk.END)
            self.downloadSearchEntry.insert(0, "")
            self.downloadSearchEntry.config(foreground="white")

    def on_download_entry_focusout(self, event):
        if self.downloadSearchEntry.get() == "":
            self.downloadSearchEntry.insert(0, self.downloadSearchEntryPrompt)
            self.downloadSearchEntry.config(foreground="grey")

    def on_enter_press(self, event=None):
        keyword = self.downloadSearchEntry.get()
        if keyword:
            self.create_download_thread1(keyword)

    def on_download_double_click(self, event=None):
        selection = self.downloadListBox.curselection()
        if selection:
            index = selection[0]
            self.create_download_thread2(index)

    def create_download_thread1(self, keyword):
        download_thread1 = threading.Thread(
            target=self.download_display, args=(keyword,))
        download_thread1.start()

    def create_download_thread2(self, index):
        download_thread2 = threading.Thread(
            target=self.download_trainer, args=(index,))
        download_thread2.start()

    def create_migration_thread(self):
        migration_thread = threading.Thread(target=self.change_path)
        migration_thread.start()

    # ===========================================================================
    # core helper functions
    # ===========================================================================
    def disable_download_widgets(self):
        self.downloadSearchEntry.config(state="disabled")
        self.fileDialogButton.config(state="disabled")

    def enable_download_widgets(self):
        self.downloadSearchEntry.config(state="enabled")
        self.fileDialogButton.config(state="enabled")

    def disable_all_widgets(self):
        self.downloadSearchEntry.config(state="disabled")
        self.fileDialogButton.config(state="disabled")
        self.trainerSearchEntry.config(state="disabled")
        self.launchButton.config(state="disabled")
        self.deleteButton.config(state="disabled")

    def enable_all_widgets(self):
        self.downloadSearchEntry.config(state="enabled")
        self.fileDialogButton.config(state="enabled")
        self.trainerSearchEntry.config(state="enabled")
        self.launchButton.config(state="enabled")
        self.deleteButton.config(state="enabled")
    
    def change_wemod_path(self):
        selected_path = filedialog.askdirectory(
            initialdir=self.wemod_path_var.get(),
            title=_("Select WeMod installation path")
        )

        if selected_path:
            self.wemod_path_var.set(os.path.normpath(selected_path))

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

    def xhh_enName(self, appid):
        xhhGameDetailUrl = f"https://api.xiaoheihe.cn/game/get_game_detail/?h_src=game_rec_a&appid={appid}"
        response = requests.get(xhhGameDetailUrl, headers=self.headers)
        xhhData = response.json()

        if 'name_en' in xhhData['result']:
            return xhhData['result']['name_en']
        else:
            return None

    def xhh_zhName(self, appid, trainerName):
        xhhGameDetailUrl = f"https://api.xiaoheihe.cn/game/get_game_detail/?h_src=game_rec_a&appid={appid}"
        response = requests.get(xhhGameDetailUrl, headers=self.headers)
        xhhData = response.json()

        # =======================================================
        # Special cases to match with Xiao Hei He game names
        if trainerName == "Assassin's Creed 3":
            trainerName = "Assassin's Creed III"
        elif trainerName == "Halo Infinite (Campaign)":
            trainerName = "Halo Infinite"

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

    def is_chinese(self, keyword):
        for char in keyword:
            if '\u4e00' <= char <= '\u9fff':
                return True
        return False

    def translate_keyword(self, keyword):
        translations = []
        if self.is_chinese(keyword):
            # Direct translation
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
            if not reqs.status_code == 200:
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
                self.downloadListBox.insert(
                    tk.END, _("No translations found."))

            print("Keyword translations:", translations)
            return translations

        return [keyword]

    def translate_trainer(self, trainerName):
        # =======================================================
        # Special cases
        if trainerName == "Bright.Memory.Episode.1 Trainer":
            trainerName = "Bright Memory: Episode 1 Trainer"

        if "/" in trainerName:
            trainerName = trainerName.split("/")[1]
        trans_trainerName = trainerName

        found_translation = False

        if settings["language"] == "zh_CN" and not settings["enSearchResults"]:
            original_trainerName = trainerName.rsplit(" Trainer", 1)[0]

            try:
                # Using Xiao Hei He search to get list of chinese names
                xhhSearchUrl = f"https://api.xiaoheihe.cn/bbs/app/api/general/search/v1?search_type=game&q={original_trainerName}"
                reqs = requests.get(xhhSearchUrl, headers=self.headers)
                xhhData = reqs.json()

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
                    print(
                        "No matches found, using direct translation for: " + original_trainerName)
                    trans_trainerName = ts.translate_text(
                        original_trainerName, from_language='en', to_language='zh')
                
                # strip any game names that have their english names
                pattern = r'[A-Za-z0-9\s：&]+（([^\）]*)\）|\（[A-Za-z\s：&]+\）$'
                trans_trainerName = re.sub(pattern, lambda m: m.group(1) if m.group(1) else '', trans_trainerName)

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
                print(f"An error occurred: {str(e)}")

        return trans_trainerName

    def sanitize(self, text):
        return re.sub(r"[\-\s\"'‘’“”:：.。,，()（）<>《》;；!！?？@#$%^&™®_+*=~`|]", "", text).lower()

    def keyword_match(self, keyword, targetString):
        def is_match(sanitized_keyword, sanitized_targetString):
            similarity_threshold = 80
            similarity = fuzz.partial_ratio(
                sanitized_keyword, sanitized_targetString)
            return similarity >= similarity_threshold

        sanitized_targetString = self.sanitize(targetString.rsplit(" Trainer", 1)[0])

        return any(is_match(self.sanitize(kw), sanitized_targetString) for kw in keyword if len(kw) >= 2 and len(sanitized_targetString) >= 2)

    def modify_fling_settings(self):
        # replace bg music in Documents folder
        empty_midi = resource_path("dependency/TrainerBGM.mid")
        username = os.getlogin()
        flingSettings_path = f"C:/Users/{username}/Documents/FLiNGTrainer"
        bgMusic_path = os.path.join(flingSettings_path, "TrainerBGM.mid")
        if os.path.exists(bgMusic_path):
            shutil.copyfile(empty_midi, bgMusic_path)

        # change fling settings to disable startup music
        settingFile_1 = os.path.join(flingSettings_path, "FLiNGTSettings.ini")
        settingFile_2 = os.path.join(flingSettings_path, "TrainerSettings.ini")

        for settingFile in [settingFile_1, settingFile_2]:
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
        empty_midi = resource_path("dependency/TrainerBGM.mid")
        resourceHackerPath = resource_path("dependency/ResourceHacker.exe")
        tempLog = os.path.join(self.tempDir, "rh.log")

        # Remove background music from executable
        command = [resourceHackerPath, "-open", source_exe, "-save", source_exe,
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
            command = [resourceHackerPath, "-open", source_exe, "-save", source_exe,
                       "-action", "addoverwrite", "-res", empty_midi, "-mask", resource]
            subprocess.run(command, creationflags=subprocess.CREATE_NO_WINDOW)
        else:
            # Try the next resource type if any remain
            self.remove_bgMusic(source_exe, resource_type_list)

    # ===========================================================================
    # core functions
    # ===========================================================================
    def update_list(self, *args):
        search_text = self.trainerSearch_var.get().lower()
        if search_text == self.trainerSearchEntryPrompt.lower() or search_text == "":
            self.show_cheats()
            return

        self.flingListBox.delete(0, tk.END)
        for trainerName in self.trainers.keys():
            if search_text in trainerName.lower():
                self.flingListBox.insert(tk.END, trainerName)

    def show_cheats(self):
        self.flingListBox.delete(0, tk.END)
        self.trainers = {}
        for trainer in sorted(os.scandir(self.trainerPath), key=lambda dirent: dirent.name):
            trainerName, trainerExt = os.path.splitext(
                os.path.basename(trainer.path))
            if trainerExt.lower() == ".exe" and os.path.getsize(trainer.path) != 0:
                # if "Trainer" in trainerName or "修改器" in trainerName:
                self.flingListBox.insert(tk.END, trainerName)
                self.trainers[trainerName] = trainer.path
            # else:
            #     shutil.rmtree(trainer.path)

    def launch_trainer(self, event=None):
        try:
            selection = self.flingListBox.curselection()
            if selection:
                index = selection[0]
                trainerName = self.flingListBox.get(index)
                os.startfile(os.path.normpath(self.trainers[trainerName]))
        except OSError as e:
            if e.winerror == 1223:
                print("Operation [Launch Trainer] was canceled by the user.")
            else:
                raise

    def delete_trainer(self):
        index = self.flingListBox.curselection()[0]
        trainerName = self.flingListBox.get(index)
        os.remove(self.trainers[trainerName])
        self.flingListBox.delete(index)
        self.show_cheats()

    def change_path(self, event=None):
        self.disable_all_widgets()
        self.downloadListBox.unbind('<Double-Button-1>')
        folder = filedialog.askdirectory(
            title=_("Change trainer download path"))

        if folder:
            changedPath = os.path.normpath(
                os.path.join(folder, "GCM Trainers/"))

            # Can't change to the same path
            if self.downloadPathText.get() == changedPath:
                messagebox.showerror(
                    _("Error"), _("Please choose a new path."))
                self.enable_all_widgets()
                return

            # check errors when creating the new path
            try:
                dst = os.path.join(folder, "GCM Trainers/")
                if not os.path.exists(dst):
                    os.makedirs(dst)
            except Exception as e:
                messagebox.showerror(_("Error"), _(
                    "Error creating the new path: ") + str(e))
                self.enable_all_widgets()
                return

            self.downloadPathText.set(changedPath)
            self.downloadListBox.delete(0, tk.END)
            self.downloadListBox.insert(
                tk.END, _("Trainer download directory changed!"))
            self.downloadListBox.insert(
                tk.END, _("Migrating existing trainers..."))

            for filename in os.listdir(self.trainerPath):
                src_file = os.path.join(self.trainerPath, filename)
                dst_file = os.path.join(dst, filename)
                if os.path.exists(dst_file):
                    os.remove(dst_file)
                shutil.move(src_file, dst_file)
            shutil.rmtree(self.trainerPath)
            self.trainerPath = dst
            settings["downloadPath"] = self.trainerPath
            apply_settings(settings)
            self.show_cheats()
            self.downloadListBox.insert(tk.END, _("Migration complete!"))

        self.enable_all_widgets()

    def download_display(self, keyword):
        self.disable_download_widgets()
        self.downloadListBox.delete(0, tk.END)
        self.downloadListBox.unbind('<Double-Button-1>')
        self.downloadSearchEntry.unbind('<Return>')
        self.trainer_urls = {}

        # Check for internet connection before search
        self.downloadListBox.insert(
            tk.END, _("Checking for internet connection..."))
        if not self.is_internet_connected():
            self.downloadListBox.insert(
                tk.END, _("No internet connection, search function is disabled."))
            self.enable_download_widgets()
            self.downloadSearchEntry.bind('<Return>', self.on_enter_press)
            return
        self.downloadListBox.insert(tk.END, _("Searching..."))

        # ts initialization will pop up a cmd window
        try:
            global ts
            if "translators" not in sys.modules:
                self.attributes('-topmost', True)
                import translators as ts
            ts.translate_text(keyword)
            self.attributes('-topmost', False)
        except Exception:
            pass

        keyword = self.translate_keyword(keyword)

        # Search for results from archive
        url = "https://archive.flingtrainer.com/"
        reqs = requests.get(url, headers=self.headers)
        archiveHTML = BeautifulSoup(reqs.text, 'html.parser')

        # Check if the request was successful
        if reqs.status_code == 200:
            self.downloadListBox.insert(tk.END, _("Request successful!"))
        else:
            self.downloadListBox.insert(
                tk.END, _("Request failed with status code: ") + str(reqs.status_code))

        for link in archiveHTML.find_all(target="_self"):
            # parse trainer name
            rawTrainerName = link.get_text()
            parsedTrainerName = re.sub(
                r' v.*|\.\bv.*| \d+\.\d+\.\d+.*| Plus\s\d+.*|Build\s\d+.*|(\d+\.\d+-Update.*)|Update\s\d+.*|\(Update\s.*| Early Access .*|\.Early.Access.*', '', rawTrainerName).replace("_", ": ")
            trainerName = parsedTrainerName.strip() + " Trainer"

            # search algorithm
            if self.keyword_match(keyword, trainerName):
                self.trainer_urls[trainerName] = urljoin(
                    url, link.get("href"))

        # Search for results from main website, prioritized, will replace same trainer from archive
        url = "https://flingtrainer.com/all-trainers-a-z/"
        reqs = requests.get(url, headers=self.headers)
        mainSiteHTML = BeautifulSoup(reqs.text, 'html.parser')

        if not reqs.status_code == 200:
            self.downloadListBox.insert(
                tk.END, _("Request failed with status code: ") + str(reqs.status_code))

        for ul in mainSiteHTML.find_all('ul'):
            for li in ul.find_all('li'):
                for link in li.find_all('a'):
                    trainerName = link.get_text().strip()
                    if trainerName in ["Home", "Trainers", "Log In", "All Trainers (A-Z)", "Privacy Policy"]:
                        continue

                    # search algorithm
                    if trainerName and self.keyword_match(keyword, trainerName):
                        self.trainer_urls[trainerName] = link.get("href")

        # Remove duplicates
        new_trainer_urls = {}
        for original_name, url in self.trainer_urls.items():
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
        self.trainer_urls = {original: url for _,
                             (original, url) in new_trainer_urls.items()}

        # Sort the dict alphabetically
        self.trainer_urls = dict(sorted(self.trainer_urls.items()))

        # Display search results
        self.downloadListBox.insert(tk.END, _("Translating..."))
        trainer_names = list(self.trainer_urls.keys())

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
        self.downloadListBox.delete(0, tk.END)

        # Display results in the original order
        for count, trainerName in enumerate(trainer_names):
            self.downloadListBox.insert(
                tk.END, f"{str(count)}. {translated_names[trainerName]}")

        if len(self.trainer_urls) == 0:
            self.downloadListBox.insert(tk.END, _("No results found."))
            self.enable_download_widgets()
            self.downloadSearchEntry.bind('<Return>', self.on_enter_press)
            return
        
        self.enable_download_widgets()
        self.downloadSearchEntry.bind('<Return>', self.on_enter_press)
        self.downloadListBox.bind(
            '<Double-Button-1>', self.on_download_double_click)

    def download_trainer(self, index):
        self.disable_download_widgets()
        self.downloadListBox.unbind('<Double-Button-1>')
        self.downloadSearchEntry.unbind('<Return>')
        self.downloadListBox.delete(0, tk.END)
        self.downloadListBox.insert(tk.END, _("Downloading..."))
        if os.path.exists(self.tempDir):
            shutil.rmtree(self.tempDir)

        # Extract final file link
        trainer_list = list(self.trainer_urls.items())
        filename = trainer_list[index][0]
        mFilename = filename.replace(':', ' -')
        if "/" in mFilename:
            mFilename = mFilename.split("/")[1]
        trans_mFilename = self.translate_trainer(mFilename)

        for trainerPath in self.trainers.keys():
            if mFilename in trainerPath or trans_mFilename in trainerPath:
                self.downloadListBox.delete(0, tk.END)
                self.downloadListBox.insert(
                    tk.END, _("Trainer already exists, aborted download."))
                self.enable_download_widgets()
                self.downloadSearchEntry.bind('<Return>', self.on_enter_press)
                return

        # Additional trainer file extraction for trainers from main site
        targeturl = self.trainer_urls[filename]
        domain = urlparse(targeturl).netloc
        if domain == "flingtrainer.com":
            gamereqs = requests.get(targeturl, headers=self.headers)
            soup2 = BeautifulSoup(gamereqs.text, 'html.parser')
            targeturl = soup2.find(target="_self").get("href")

        # Download file
        try:
            finalreqs = requests.get(targeturl, headers=self.headers)
        except Exception as e:
            messagebox.showerror(
                _("Error"), _("An error occurred while getting trainer url: ") + str(e))
            self.enable_download_widgets()
            self.downloadSearchEntry.bind('<Return>', self.on_enter_press)
            return

        # Find compressed file extension
        fileDownloadLink = finalreqs.url
        extension = fileDownloadLink[fileDownloadLink.rfind('.'):]
        zFilename = mFilename + extension

        trainerTemp = os.path.join(self.tempDir, zFilename)
        if not os.path.exists(self.tempDir):
            os.mkdir(self.tempDir)
        with open(trainerTemp, "wb") as f:
            f.write(finalreqs.content)
        time.sleep(2)

        # Extract compressed file and rename
        try:
            if extension == ".rar":
                unrar = resource_path("dependency/UnRAR.exe")
                command = [unrar, "x", "-y", trainerTemp, self.tempDir]
                subprocess.run(command, check=True,
                               creationflags=subprocess.CREATE_NO_WINDOW)
            elif extension == ".zip":
                with zipfile.ZipFile(trainerTemp, 'r') as zip_ref:
                    zip_ref.extractall(self.tempDir)
        except Exception as e:
            messagebox.showerror(
                _("Error"), _("An error occurred while extracting downloaded trainer: ") + str(e))
            self.enable_download_widgets()
            self.downloadSearchEntry.bind('<Return>', self.on_enter_press)
            return

        # Locate extracted .exe file
        cnt = 0
        for filename in os.listdir(self.tempDir):
            if "Trainer" in filename and filename.endswith(".exe"):
                gameRawName = filename
            elif "Trainer" not in filename:
                cnt += 1
        # Warn user if extra files found
        if cnt > 0:
            messagebox.showinfo(
                _("Attention"), _("Additional actions required\nPlease check folder for details!"))
            os.startfile(self.tempDir)

        os.makedirs(self.trainerPath, exist_ok=True)
        trainer_name = trans_mFilename + ".exe"
        source_file = os.path.join(self.tempDir, gameRawName)
        destination_file = os.path.join(self.trainerPath, trainer_name)

        # remove fling trainer bg music
        self.modify_fling_settings()  # by changing trainer settings
        # by removing directly from exe
        self.remove_bgMusic(source_file, ["MID", "MIDI"])

        try:
            shutil.move(source_file, destination_file)
            os.remove(trainerTemp)
            rhLog = os.path.join(self.tempDir, "rh.log")
            if os.path.exists(rhLog):
                os.remove(rhLog)
        except Exception as e:
            messagebox.showerror(_("Error"), _(
                "Could not find the downloaded trainer file, please try turning your antivirus software off."))
            self.enable_download_widgets()
            self.downloadSearchEntry.bind('<Return>', self.on_enter_press)
            return

        self.downloadListBox.insert(tk.END, _("Download success!"))
        self.enable_download_widgets()
        self.downloadSearchEntry.bind('<Return>', self.on_enter_press)
        self.show_cheats()

    def wemod_pro(self):
        wemodPath = os.path.normpath(settings["WeModPath"])
        if not os.path.exists(wemodPath):
            messagebox.showerror(_("Error"), _("WeMod not installed or invalid installation path.\nPlease choose a correct WeMod installation path in settings."))
            return

        version_folders = []
        for item in os.listdir(wemodPath):
            if os.path.isdir(os.path.join(wemodPath, item)):
                match = re.match(r'app-(\d+\.\d+\.\d+)', item)
                if match:
                    version_info = tuple(
                        map(int, match.group(1).split('.')))  # (8, 13, 3)
                    # ((8, 13, 3), 'app-8.13.3')
                    version_folders.append((version_info, item))

        if not version_folders:
            messagebox.showerror(_("Error"), _("WeMod not installed or invalid installation path.\nPlease choose a correct WeMod installation path in settings."))
            return

        # Sort based on version numbers (major, minor, patch)
        version_folders.sort(reverse=True)  # Newest first

        # Skip the deletion if there's only one folder found
        if not len(version_folders) <= 1:
            # Skip the first one as it's the newest
            for version_info, folder_name in version_folders[1:]:
                folder_path = os.path.join(wemodPath, folder_name)
                shutil.rmtree(folder_path)
                print(f"Deleted old version folder: {folder_path}")

        newest_version_folder = version_folders[0][1]

        newest_version_path = os.path.join(
            wemodPath, newest_version_folder)
        print("Newest Version Folder" + newest_version_path)

        # Apply patch
        file_to_replace = os.path.join(
            newest_version_path, "resources/app.asar")
        try:
            os.remove(file_to_replace)
        except Exception:
            messagebox.showerror(
                _("Error"), _("WeMod is currently running, please close the application first."))
            return
        shutil.copyfile(self.crackFilePath, file_to_replace)

        messagebox.showinfo(
            _("Success"), _("You have now activated WeMod Pro!\nCurrent WeMod version: v") + newest_version_folder.strip('-app'))


if __name__ == "__main__":
    GameCheatsManager()
