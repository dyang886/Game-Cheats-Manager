import json
import os
import shutil
import subprocess
import sys
import tempfile
import threading
import time
import tkinter as tk
from tkinter import filedialog, messagebox, ttk
import zipfile
import re

from bs4 import BeautifulSoup
import gettext
import polib
import requests
import sv_ttk
from tendo import singleton
from urllib.parse import urljoin, urlparse
import locale


def resource_path(relative_path):
    if hasattr(sys, "_MEIPASS"):
        return os.path.join(sys._MEIPASS, relative_path)
    return os.path.join(os.path.abspath("."), relative_path)


def apply_settings(settings):
    with open(SETTINGS_FILE, "w") as f:
        json.dump(settings, f)


def load_settings():
    try:
        with open(SETTINGS_FILE, "r") as f:
            return json.load(f)
    except FileNotFoundError:
        system_locale = locale.getlocale()[0]
        locale_mapping = {
            "English_United States": "en_US",
            "Chinese (Simplified)_China": "zh_CN",
            "Chinese (Simplified)_Hong Kong SAR": "zh_CN",
            "Chinese (Simplified)_Macao SAR": "zh_CN"
        }
        app_locale = locale_mapping.get(system_locale, 'en_US')

        # Default settings
        default_settings = {
            "downloadPath": "Trainers/",
            "language": app_locale
        }
        with open(SETTINGS_FILE, "w") as f:
            json.dump(default_settings, f)
        return default_settings


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

settings = load_settings()
_ = get_translator()

language_options = {
    "English (US)": "en_US",
    "简体中文": "zh_CN"
}


class GameCheatsManager(tk.Tk):

    def __init__(self):
        super().__init__()

        try:
            self.single_instance_checker = singleton.SingleInstance()
        except singleton.SingleInstanceException:
            sys.exit(1)

        self.title("Game Cheats Manager")
        self.iconbitmap(resource_path("assets/logo.ico"))
        sv_ttk.set_theme("dark")
        # {trainer name, trainer path}
        self.trainers = {}  # store installed trainers
        # {trainer name, download link}
        self.trainer_urls = {}  # store search results
        self.trainerPath = os.path.normpath(
            os.path.abspath(load_settings()["downloadPath"]))
        os.makedirs(self.trainerPath, exist_ok=True)
        self.tempDir = os.path.join(
            tempfile.gettempdir(), "GameCheatsManagerTemp")
        self.trainerSearchEntryPrompt = _("Search for installed")
        self.downloadSearchEntryPrompt = _("Search to download")
        self.headers = {
            'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3'}
        self.settings_window = None
        self.crackFilePath = resource_path("dependency/app.asar")
        self.wemodPath = os.path.join(os.getenv("LOCALAPPDATA"), "WeMod/")

        # Menu bar
        self.menuBar = tk.Frame(self, background="#2e2e2e")
        self.settingMenuBtn = tk.Menubutton(
            self.menuBar, text=_("Options"), background="#2e2e2e")
        self.settingsMenu = tk.Menu(self.settingMenuBtn, tearoff=0)
        self.settingsMenu.add_command(
            label=_("Settings"), command=self.open_settings)
        self.settingsMenu.add_command(
            label=("WeMod Pro"), command=self.wemod_pro)
        self.settingMenuBtn.config(menu=self.settingsMenu)
        self.settingMenuBtn.pack(side="left")
        self.menuBar.grid(row=0, column=0, sticky="ew")

        # Main frame
        self.frame = ttk.Frame(self)
        self.frame.grid(row=1, column=0, padx=30, pady=(20, 30))

        # ================================================================
        # column 1 - trainers
        # ================================================================
        self.flingFrame = ttk.Frame(self.frame)
        self.flingFrame.grid(row=0, column=0)

        self.trainerSearchFrame = ttk.Frame(self.flingFrame)
        self.trainerSearchFrame.grid(row=0, column=0, pady=(0, 20))

        self.searchPhoto = tk.PhotoImage(
            file=resource_path("assets/search.png"))
        search_width = 19
        search_height = 19
        search_width_ratio = self.searchPhoto.width() // search_width
        search_height_ratio = self.searchPhoto.height() // search_height
        self.searchPhoto = self.searchPhoto.subsample(
            search_width_ratio, search_height_ratio)
        self.searchLabel = ttk.Label(
            self.trainerSearchFrame, image=self.searchPhoto)
        self.searchLabel.pack(side=tk.LEFT, padx=(0, 10))

        self.trainerSearch_var = tk.StringVar()
        self.trainerSearchEntry = ttk.Entry(
            self.trainerSearchFrame, textvariable=self.trainerSearch_var)
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
                                       activestyle="none", width=40, height=15,
                                       yscrollcommand=self.flingVScrollbar.set,
                                       xscrollcommand=self.flingHScrollbar.set)
        self.flingListBox.grid(row=1, column=0)

        self.flingVScrollbar.config(command=self.flingListBox.yview)
        self.flingHScrollbar.config(command=self.flingListBox.xview)

        # bottom buttons
        self.bottomFrame = ttk.Frame(self.flingFrame)
        self.bottomFrame.grid(row=3, column=0, pady=(10, 0))

        self.launchButton = ttk.Button(
            self.bottomFrame, text=_("Launch"), width=11, command=self.launch_trainer)
        self.launchButton.grid(row=0, column=0, padx=(0, 9))

        self.deleteButton = ttk.Button(
            self.bottomFrame, text=_("Delete"), width=11, command=self.delete_trainer)
        self.deleteButton.grid(row=0, column=1, padx=(9, 0))

        # ================================================================
        # column 2 - downloads
        # ================================================================
        self.downloadFrame = ttk.Frame(self.frame)
        self.downloadFrame.grid(row=0, column=1, padx=(30, 0))

        self.downloadSearchFrame = ttk.Frame(self.downloadFrame)
        self.downloadSearchFrame.grid(row=0, column=0, pady=(0, 20))

        self.downloadSearchLabel = ttk.Label(
            self.downloadSearchFrame, image=self.searchPhoto)
        self.downloadSearchLabel.pack(side=tk.LEFT, padx=(0, 10))

        self.downloadSearch_var = tk.StringVar()
        self.downloadSearchEntry = ttk.Entry(
            self.downloadSearchFrame, textvariable=self.downloadSearch_var)
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
                                          activestyle="none", width=40, height=15,
                                          yscrollcommand=self.downloadVScrollbar.set,
                                          xscrollcommand=self.downloadHScrollbar.set)
        self.downloadListBox.grid(row=1, column=0)

        self.downloadVScrollbar.config(command=self.downloadListBox.yview)
        self.downloadHScrollbar.config(command=self.downloadListBox.xview)

        # bottom change download path
        self.changeDownloadPath = ttk.Frame(self.downloadFrame)
        self.changeDownloadPath.grid(row=3, column=0, pady=(10, 0))

        self.downloadPathText = tk.StringVar()
        self.downloadPathText.set(self.trainerPath)
        self.downloadPathEntry = ttk.Entry(
            self.changeDownloadPath, width=21, state="readonly", textvariable=self.downloadPathText)
        self.downloadPathEntry.pack(side=tk.LEFT, padx=(0, 15))

        self.fileDialogButton = ttk.Button(
            self.changeDownloadPath, text="...", width=2, command=self.create_migration_thread)
        self.fileDialogButton.pack(side=tk.LEFT)

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

    def open_settings(self):
        if self.settings_window is None or not self.settings_window.winfo_exists():
            self.settings_window = tk.Toplevel(self)
            self.settings_window.title(_("Settings"))
            self.settings_window.iconbitmap(
                resource_path("assets/setting.ico"))
            self.settings_window.columnconfigure(0, weight=1)
            self.settings_window.rowconfigure(0, weight=1)

            window_width, window_height = 300, 200
            self.settings_window.geometry(f"{window_width}x{window_height}")
            self.settings_window.resizable(False, False)

            # Center the settings window to be inside of main app window
            main_x = self.winfo_x()
            main_y = self.winfo_y()
            main_width = self.winfo_width()
            main_height = self.winfo_height()
            center_x = int(main_x + (main_width - window_width) / 2)
            center_y = int(main_y + (main_height - window_height) / 2)
            self.settings_window.geometry(f"+{center_x}+{center_y}")

            # languages frame
            self.languages_frame = ttk.Frame(self.settings_window)

            # languages label
            self.languages_label = ttk.Label(
                self.languages_frame, text=_("Language:"))
            self.languages_label.pack(anchor="w")

            # languages combobox
            self.languages_var = tk.StringVar()
            for key, value in language_options.items():
                if value == settings["language"]:
                    self.languages_var.set(key)

            self.languages_combobox = ttk.Combobox(
                self.languages_frame, textvariable=self.languages_var, values=list(language_options.keys()), state="readonly", width=20)
            self.languages_combobox.pack(side=tk.LEFT)

            # apply button
            apply_button = ttk.Button(
                self.settings_window, text=_("Apply"), command=self.apply_settings_page)

            self.languages_frame.grid(row=0, column=0, pady=(20, 0))
            apply_button.grid(row=2, column=0, padx=(
                0, 20), pady=(20, 20), sticky=tk.E)
        else:
            self.settings_window.lift()
            self.settings_window.focus_force()

    def apply_settings_page(self):
        settings["language"] = language_options[self.languages_var.get()]
        apply_settings(settings)
        messagebox.showinfo(_("Attention"), _(
            "Please restart the application to apply settings"))

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

    def update_list(self, *args):
        search_text = self.trainerSearch_var.get().lower()
        if search_text == self.trainerSearchEntryPrompt.lower() or search_text == "":
            self.show_cheats()
            return

        self.flingListBox.delete(0, tk.END)
        for trainerName in self.trainers.keys():
            if search_text in trainerName.lower():
                self.flingListBox.insert(tk.END, trainerName)

    def change_path(self, event=None):
        self.disable_all_widgets()
        self.downloadListBox.unbind('<Double-Button-1>')
        folder = filedialog.askdirectory(
            title=_("Change trainer download path"))

        if folder:
            changedPath = os.path.normpath(
                os.path.join(folder, "Trainers/"))
            if self.downloadPathText.get() == changedPath:
                messagebox.showerror(
                    _("Error"), _("Please choose a new path."))
                self.enable_all_widgets()
                return

            self.downloadPathText.set(changedPath)
            self.downloadListBox.delete(0, tk.END)
            self.downloadListBox.insert(
                tk.END, _("Trainer download directory changed!"))
            self.downloadListBox.insert(
                tk.END, _("Migrating existing trainers..."))

            dst = os.path.join(folder, "Trainers/")
            if not os.path.exists(dst):
                os.makedirs(dst)
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

    def show_cheats(self):
        self.flingListBox.delete(0, tk.END)
        self.trainers = {}
        for trainer in sorted(os.scandir(self.trainerPath), key=lambda dirent: dirent.name):
            trainerName = os.path.splitext(os.path.basename(trainer.path))[0]
            if trainer.is_file() and os.path.getsize(trainer) != 0:
                if "Trainer" in trainerName:
                    self.flingListBox.insert(tk.END, trainerName)
                    self.trainers[trainerName] = trainer.path
            # else:
            #     shutil.rmtree(trainer.path)

    def download_display(self, keyword):
        self.disable_download_widgets()
        self.downloadListBox.delete(0, tk.END)
        self.downloadListBox.unbind('<Double-Button-1>')
        self.downloadListBox.insert(tk.END, _("Searching..."))
        self.trainer_urls = {}

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
                r' v.*| Plus.*', '', rawTrainerName).replace("_", ": ")
            trainerName = parsedTrainerName.strip() + " Trainer"

            # search algorithm
            if len(keyword) >= 3:
                if re.search(re.escape(keyword), trainerName, re.IGNORECASE):
                    self.trainer_urls[trainerName] = urljoin(
                        url, link.get("href"))

        # Search for results from main website, prioritized, will replace same trainer from archive
        url = "https://flingtrainer.com/?s=" + keyword.replace(" ", "+")
        reqs = requests.get(url, headers=self.headers)
        mainSiteHTML = BeautifulSoup(reqs.text, 'html.parser')

        if not reqs.status_code == 200:
            self.downloadListBox.insert(
                tk.END, _("Request failed with status code: ") + str(reqs.status_code))

        # Generate and store main site search results
        for link in mainSiteHTML.find_all(rel="bookmark"):
            trainerName = link.get_text()
            if "My Trainers Archive" in trainerName:
                continue
            self.trainer_urls[trainerName] = link.get("href")

        # Remove duplicates
        new_trainer_urls = {}
        for original_name, url in self.trainer_urls.items():
            # Normalize the name
            norm_name = original_name.replace(":", "").replace(
                "'", "").replace("’", "").replace(" ", "").lower()
            domain = urlparse(url).netloc

            # Add or update the entry based on domain preference
            if norm_name not in new_trainer_urls:
                new_trainer_urls[norm_name] = (original_name, url)
            else:
                # Check if the new URL is from the main site and should replace the existing one
                existing_url = new_trainer_urls[norm_name][1]
                existing_domain = urlparse(existing_url).netloc
                if domain == "flingtrainer.com":
                    new_trainer_urls[norm_name] = (original_name, url)

        # Convert the dictionary to use original names as keys
        self.trainer_urls = {original: url for _,
                             (original, url) in new_trainer_urls.items()}

        # Sort the dict alphabetically
        self.trainer_urls = dict(sorted(self.trainer_urls.items()))

        count = 0
        for trainerName in self.trainer_urls.keys():
            if count == 0:
                self.enable_download_widgets()
                self.downloadListBox.bind(
                    '<Double-Button-1>', self.on_download_double_click)
                self.downloadListBox.delete(0, tk.END)

            self.downloadListBox.insert(tk.END, f"{str(count)}. {trainerName}")
            count += 1

        if len(self.trainer_urls) == 0:
            self.downloadListBox.unbind('<Double-Button-1>')
            self.downloadListBox.insert(tk.END, _("No results found."))
            self.enable_download_widgets()
            return

    def download_trainer(self, index):
        self.disable_download_widgets()
        self.downloadListBox.unbind('<Double-Button-1>')
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

        for trainerPath in self.trainers.keys():
            if mFilename in trainerPath:
                self.downloadListBox.delete(0, tk.END)
                self.downloadListBox.insert(
                    tk.END, _("Trainer already exists, aborted download."))
                self.enable_download_widgets()
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
        trainer_name = mFilename + ".exe"
        source_file = os.path.join(self.tempDir, gameRawName)
        destination_file = os.path.join(self.trainerPath, trainer_name)

        # remove fling trainer bg music
        self.modify_fling_settings()  # by changing trainer settings
        # by removing directly from exe
        self.remove_bgMusic(source_file, ["MID", "MIDI"])

        shutil.move(source_file, destination_file)
        os.remove(trainerTemp)
        rhLog = os.path.join(self.tempDir, "rh.log")
        if os.path.exists(rhLog):
            os.remove(rhLog)

        self.downloadListBox.insert(tk.END, _("Download success!"))
        self.enable_download_widgets()
        self.show_cheats()

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

    def wemod_pro(self):
        newest_version_folder = self.clean_old_versions(self.wemodPath)
        if not newest_version_folder:
            messagebox.showerror(_("Error"), _("WeMod not installed."))
            return

        newest_version_path = os.path.join(
            self.wemodPath, newest_version_folder)
        print("Newest Version Folder" + newest_version_path)

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

    def identify_and_sort_versions(self, directory):
        if not os.path.exists(directory):
            return None

        version_folders = []
        for item in os.listdir(directory):
            if os.path.isdir(os.path.join(directory, item)):
                match = re.match(r'app-(\d+\.\d+\.\d+)', item)
                if match:
                    version_info = tuple(
                        map(int, match.group(1).split('.')))  # (8, 13, 3)
                    # ((8, 13, 3), 'app-8.13.3')
                    version_folders.append((version_info, item))

        if not version_folders:
            return None

        # Sort based on version numbers (major, minor, patch)
        version_folders.sort(reverse=True)  # Newest first
        return version_folders

    def clean_old_versions(self, directory):
        version_folders = self.identify_and_sort_versions(directory)
        if not version_folders:
            return None

        # Skip the deletion if there's only one folder found
        if not len(version_folders) <= 1:
            # Skip the first one as it's the newest
            for version_info, folder_name in version_folders[1:]:
                folder_path = os.path.join(directory, folder_name)
                shutil.rmtree(folder_path)
                print(f"Deleted old version folder: {folder_path}")

        return version_folders[0][1]


if __name__ == "__main__":
    GameCheatsManager()
