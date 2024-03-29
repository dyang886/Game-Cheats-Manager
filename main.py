import os
import re
import shutil
import sys

from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtGui import QAction, QColor, QFont, QFontDatabase, QIcon, QPixmap
from PyQt6.QtWidgets import QApplication, QFileDialog, QGridLayout, QHBoxLayout, QLabel, QLineEdit, QListWidgetItem, QMainWindow, QMessageBox, QPushButton, QVBoxLayout, QWidget, QListWidget
from tendo import singleton

from helper import *
import style_sheet


class GameCheatsManager(QMainWindow):

    def __init__(self):
        super().__init__()

        # Single instance check and basic UI setup
        try:
            self.single_instance_checker = singleton.SingleInstance()
        except singleton.SingleInstanceException:
            sys.exit(1)
        self.setWindowTitle("Game Cheats Manager")
        self.setWindowIcon(QIcon(resource_path("assets/logo.ico")))
        self.setFixedSize(self.size())

        # Version, user prompts, and links
        self.appVersion = "2.0.0-beta2"
        self.githubLink = "https://github.com/dyang886/Game-Cheats-Manager"
        self.updateLink = "https://api.github.com/repos/dyang886/Game-Cheats-Manager/releases/latest"
        self.trainerSearchEntryPrompt = tr("Search for installed")
        self.downloadSearchEntryPrompt = tr("Search to download")

        # Paths and variable management
        self.trainerPath = os.path.normpath(
            os.path.abspath(settings["downloadPath"]))
        os.makedirs(self.trainerPath, exist_ok=True)
        
        self.dropDownArrow_path = resource_path("assets/dropdown.png").replace("\\", "/")
        
        self.crackFile_path = resource_path("dependency/app.asar")
        self.search_path = resource_path("assets/search.png")
        resource_path("dependency/ResourceHacker.exe")
        resource_path("dependency/UnRAR.exe")
        resource_path("dependency/TrainerBGM.mid")

        self.trainers = {}  # Store installed trainers: {trainer name: trainer path}
        self.searchable = True  # able to search online trainers or not
        self.downloadable = False  # able to double click on download list or not

        # Window references
        self.settings_window = None
        self.about_window = None

        # Main widget group
        centralWidget = QWidget(self)
        self.setCentralWidget(centralWidget)
        mainLayout = QGridLayout(centralWidget)
        mainLayout.setSpacing(15)
        mainLayout.setContentsMargins(30, 20, 30, 30)
        centralWidget.setLayout(mainLayout)
        self.init_settings()

        # Menu Setup
        menuFont = self.font()
        menuFont.setPointSize(9)
        menu = self.menuBar()
        menu.setFont(menuFont)

        optionMenu = menu.addMenu(tr("Options"))

        settingsAction = QAction(tr("Settings"), self)
        settingsAction.setFont(menuFont)
        settingsAction.triggered.connect(self.open_settings)
        optionMenu.addAction(settingsAction)

        importAction = QAction(tr("Import Trainers"), self)
        importAction.setFont(menuFont)
        importAction.triggered.connect(self.import_files)
        optionMenu.addAction(importAction)

        openDirectoryAction = QAction(tr("Open Trainer Download Path"), self)
        openDirectoryAction.setFont(menuFont)
        openDirectoryAction.triggered.connect(self.open_trainer_directory)
        optionMenu.addAction(openDirectoryAction)

        wemodAction = QAction(tr("Unlock WeMod Pro"), self)
        wemodAction.setFont(menuFont)
        wemodAction.triggered.connect(self.wemod_pro)
        optionMenu.addAction(wemodAction)

        aboutAction = QAction(tr("About"), self)
        aboutAction.setFont(menuFont)
        aboutAction.triggered.connect(self.open_about)
        optionMenu.addAction(aboutAction)

        # ===========================================================================
        # Column 1 - trainers
        # ===========================================================================
        trainersLayout = QVBoxLayout()
        trainersLayout.setSpacing(10)
        mainLayout.addLayout(trainersLayout, 0, 0)

        # Search installed trainers
        trainerSearchLayout = QHBoxLayout()
        trainerSearchLayout.setSpacing(10)
        trainerSearchLayout.setContentsMargins(20, 0, 20, 0)
        trainersLayout.addLayout(trainerSearchLayout)

        searchPixmap = QPixmap(self.search_path).scaled(25, 25, Qt.AspectRatioMode.KeepAspectRatio)
        searchLabel = QLabel()
        searchLabel.setPixmap(searchPixmap)
        trainerSearchLayout.addWidget(searchLabel)

        self.trainerSearchEntry = QLineEdit()
        trainerSearchLayout.addWidget(self.trainerSearchEntry)
        self.trainerSearchEntry.setPlaceholderText(self.trainerSearchEntryPrompt)
        self.trainerSearchEntry.textChanged.connect(self.update_list)

        # Display installed trainers
        self.flingListBox = QListWidget()
        self.flingListBox.itemDoubleClicked.connect(self.launch_trainer)
        trainersLayout.addWidget(self.flingListBox)

        # Launch and delete buttons
        bottomLayout = QHBoxLayout()
        bottomLayout.setSpacing(6)
        trainersLayout.addLayout(bottomLayout)

        self.launchButton = QPushButton(tr("Launch"))
        bottomLayout.addWidget(self.launchButton)
        self.launchButton.clicked.connect(self.launch_trainer)

        self.deleteButton = QPushButton(tr("Delete"))
        bottomLayout.addWidget(self.deleteButton)
        self.deleteButton.clicked.connect(self.delete_trainer)

        # ===========================================================================
        # Column 2 - downloads
        # ===========================================================================
        downloadsLayout = QVBoxLayout()
        downloadsLayout.setSpacing(10)
        mainLayout.addLayout(downloadsLayout, 0, 1)

        # Search online trainers
        downloadSearchLayout = QHBoxLayout()
        downloadSearchLayout.setSpacing(10)
        downloadSearchLayout.setContentsMargins(20, 0, 20, 0)
        downloadsLayout.addLayout(downloadSearchLayout)

        searchLabel = QLabel()
        searchLabel.setPixmap(QPixmap(self.search_path).scaled(25, 25, Qt.AspectRatioMode.KeepAspectRatio))
        downloadSearchLayout.addWidget(searchLabel)

        self.downloadSearchEntry = QLineEdit()
        self.downloadSearchEntry.setPlaceholderText(self.downloadSearchEntryPrompt)
        self.downloadSearchEntry.returnPressed.connect(self.on_enter_press)
        downloadSearchLayout.addWidget(self.downloadSearchEntry)

        # Display trainer search results
        self.downloadListBox = QListWidget()
        self.downloadListBox.itemDoubleClicked.connect(self.on_download_double_click)
        downloadsLayout.addWidget(self.downloadListBox)

        # Change trainer download path
        changeDownloadPathLayout = QHBoxLayout()
        changeDownloadPathLayout.setSpacing(5)
        downloadsLayout.addLayout(changeDownloadPathLayout)

        self.downloadPathEntry = QLineEdit()
        self.downloadPathEntry.setReadOnly(True)
        self.downloadPathEntry.setText(self.trainerPath)
        changeDownloadPathLayout.addWidget(self.downloadPathEntry)

        self.fileDialogButton = QPushButton("...")
        changeDownloadPathLayout.addWidget(self.fileDialogButton)
        self.fileDialogButton.clicked.connect(self.change_path)

        self.show_cheats()

        # Check for trainer update timer
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.on_check_update)
        self.on_check_update()
        self.timer.start(300000)

    # ===========================================================================
    # Core functions
    # ===========================================================================
    def init_settings(self):
        if settings["theme"] == "black":
            style = style_sheet.black
        elif settings["theme"] == "white":
            style = style_sheet.white

        style = style.format(
            drop_down_arrow=self.dropDownArrow_path,
        )
        self.setStyleSheet(style)

    def on_enter_press(self):
        keyword = self.downloadSearchEntry.text()
        if keyword and self.searchable:
            self.download_display(keyword)

    def on_download_double_click(self, item):
        index = self.downloadListBox.row(item)
        if index >= 0 and self.downloadable:
            self.download_trainers(index)

    def disable_download_widgets(self):
        self.downloadSearchEntry.setEnabled(False)
        self.fileDialogButton.setEnabled(False)

    def enable_download_widgets(self):
        self.downloadSearchEntry.setEnabled(True)
        self.fileDialogButton.setEnabled(True)

    def disable_all_widgets(self):
        self.downloadSearchEntry.setEnabled(False)
        self.fileDialogButton.setEnabled(False)
        self.trainerSearchEntry.setEnabled(False)
        self.launchButton.setEnabled(False)
        self.deleteButton.setEnabled(False)

    def enable_all_widgets(self):
        self.downloadSearchEntry.setEnabled(True)
        self.fileDialogButton.setEnabled(True)
        self.trainerSearchEntry.setEnabled(True)
        self.launchButton.setEnabled(True)
        self.deleteButton.setEnabled(True)

    def update_list(self):
        search_text = self.trainerSearchEntry.text().lower()
        if search_text == self.trainerSearchEntryPrompt.lower() or search_text == "":
            self.show_cheats()
            return

        self.flingListBox.clear()
        for trainerName in self.trainers.keys():
            if search_text in trainerName.lower():
                self.flingListBox.addItem(trainerName)

    def show_cheats(self):
        self.flingListBox.clear()
        self.trainers = {}
        for trainer in sorted(os.scandir(self.trainerPath), key=lambda dirent: dirent.name):
            trainerName, trainerExt = os.path.splitext(os.path.basename(trainer.path))
            if trainerExt.lower() == ".exe" and os.path.getsize(trainer.path) != 0:
                self.flingListBox.addItem(trainerName)
                self.trainers[trainerName] = trainer.path

    def launch_trainer(self):
        try:
            selection = self.flingListBox.currentRow()
            if selection != -1:
                trainerName = self.flingListBox.item(selection).text()
                os.startfile(os.path.normpath(self.trainers[trainerName]))
        except OSError as e:
            if e.winerror == 1223:
                print("[Launch Trainer] was canceled by the user.")
            else:
                raise

    def delete_trainer(self):
        index = self.flingListBox.currentRow()
        if index != -1:
            trainerName = self.flingListBox.item(index).text()
            os.remove(self.trainers[trainerName])
            self.flingListBox.takeItem(index)
            self.show_cheats()

    def change_path(self):
        self.downloadListBox.clear()
        self.disable_all_widgets()
        folder = QFileDialog.getExistingDirectory(self, tr("Change trainer download path"))

        if folder:
            changedPath = os.path.normpath(os.path.join(folder, "GCM Trainers/"))
            if self.downloadPathEntry.text() == changedPath:
                QMessageBox.critical(self, tr("Error"), tr("Please choose a new path."))
                self.enable_all_widgets()
                return

            self.downloadListBox.addItem(tr("Migrating existing trainers..."))
            self.migration_thread = PathChangeThread(self.trainerPath, folder, self)
            self.migration_thread.finished.connect(self.on_migration_finished)
            self.migration_thread.error.connect(self.on_migration_error)
            self.migration_thread.start()
        
        else:
            self.downloadListBox.addItem(tr("No path selected."))
            self.enable_all_widgets()
            return
        
    def on_check_update(self):
        # The binary pattern representing "FLiNGTrainerNamedPipe_" followed by some null bytes and the date
        pattern_hex = '46 00 4c 00 69 00 4E 00 47 00 54 00 72 00 61 00 69 00 6E 00 65 00 72 00 4E 00 61 00 6D 00 65 00 64 00 50 00 69 00 70 00 65 00 5F'
        # Convert the hex pattern to binary
        pattern = bytes.fromhex(''.join(pattern_hex.split()))

        for index, trainerPath in enumerate(self.trainers.values()):
            trainerBuildDate = ""
            checkUpdateUrl = "https://flingtrainer.com/tag/"

            with open(trainerPath, 'rb') as file:
                content = file.read()
                pattern_index = content.find(pattern)

                if pattern_index == -1:
                    continue

                # Find the start of the date string after the pattern and skip null bytes (00)
                start_index = pattern_index + len(pattern)
                while content[start_index] == 0:
                    start_index += 1

                # The date could be "Mar  8 2024" or "Dec 10 2022"
                date_match = re.search(rb'\b(?:Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+\d{1,2}\s+\d{4}\b', content[start_index:])
                
                if date_match:
                    trainerBuildDate = date_match.group().decode('utf-8')
                    # print(trainerBuildDate)

                    # extract trainer name


    def on_migration_finished(self, new_path):
        self.trainerPath = new_path
        settings["downloadPath"] = self.trainerPath
        apply_settings(settings)
        self.show_cheats()
        self.downloadListBox.addItem(tr("Migration complete!"))
        self.downloadPathEntry.setText(self.trainerPath)
        self.enable_all_widgets()

    def on_migration_error(self, error_message):
        QMessageBox.critical(self, tr("Error"), tr("Error creating the new path: ") + error_message)
        self.enable_all_widgets()

    def download_display(self, keyword):
        self.disable_download_widgets()
        self.downloadListBox.clear()
        self.downloadable = False
        self.searchable = False

        display_thread = DownloadDisplayThread(keyword, self)
        display_thread.message.connect(self.on_message)
        display_thread.finished.connect(self.on_display_finished)
        display_thread.start()

    def on_message(self, message, type=None):
        item = QListWidgetItem(message)

        if type == "clear":
            self.downloadListBox.clear()
        elif type == "success":
            item.setForeground(QColor('green'))
            self.downloadListBox.addItem(item)
        elif type == "failure":
            item.setForeground(QColor('red'))
            self.downloadListBox.addItem(item)
        else:
            self.downloadListBox.addItem(item)

    def on_display_finished(self, status):
        # 0: success; 1: failure
        if status:
            self.downloadable = False
        else:
            self.downloadable = True
            
        self.searchable = True
        self.enable_download_widgets()

    def download_trainers(self, index):
        self.disable_download_widgets()
        self.downloadListBox.clear()
        self.downloadable = False
        self.searchable = False

        download_thread = DownloadTrainersThread(index, self.trainerPath, self.trainers, self)
        download_thread.message.connect(self.on_message)
        download_thread.messageBox.connect(self.on_message_box)
        download_thread.finished.connect(self.on_download_finished)
        download_thread.start()

    def on_message_box(self, type, title, text):
        if type == "info":
            QMessageBox.information(self, title, text)
        elif type == "error":
            QMessageBox.critical(self, title, text)
    
    def on_download_finished(self, status):
        self.downloadable = False
        self.searchable = True
        self.enable_download_widgets()
        self.show_cheats()

    # ===========================================================================
    # Menu functions
    # ===========================================================================
    def open_settings(self):
        if self.settings_window is not None and self.settings_window.isVisible():
            self.settings_window.raise_()
            self.settings_window.activateWindow()
        else:
            self.settings_window = SettingsDialog(self)
            self.settings_window.show()

    def import_files(self):
        file_names, _ = QFileDialog.getOpenFileNames(self, tr("Import trainers"), "", tr("Executable Files (*.exe)"))
        if file_names:
            for file_name in file_names:
                dest_path = os.path.join(self.trainerPath, os.path.basename(file_name))
                shutil.move(file_name, dest_path)
                print("Trainer moved: ", file_name)
            self.show_cheats()

    def open_trainer_directory(self):
        os.startfile(self.trainerPath)

    def open_about(self):
        if self.about_window is not None and self.about_window.isVisible():
            self.about_window.raise_()
            self.about_window.activateWindow()
        else:
            self.about_window = AboutDialog(self)
            self.about_window.show()

    def wemod_pro(self):
        wemodPath = os.path.normpath(settings["WeModPath"])
        if not os.path.exists(wemodPath):
            QMessageBox.critical(self, tr("Error"), tr("WeMod not installed or invalid installation path.\nPlease choose a correct WeMod installation path in settings."))
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
            QMessageBox.critical(self, tr("Error"), tr("WeMod not installed or invalid installation path.\nPlease choose a correct WeMod installation path in settings."))
            return

        # Sort based on version numbers (major, minor, patch)
        version_folders.sort(reverse=True)  # Newest first

        # Skip the deletion if there's only one folder found
        if not len(version_folders) <= 1:
            # Skip the first one as it's the newest
            for version_info, folder_name in version_folders[1:]:
                folder_path = os.path.join(wemodPath, folder_name)
                try:
                    shutil.rmtree(folder_path)
                except Exception as e:
                    print(str(e))
                print(f"Deleted old version folder: {folder_path}")

        newest_version_folder = version_folders[0][1]

        newest_version_path = os.path.join(
            wemodPath, newest_version_folder)
        print("Newest Version Folder: " + newest_version_path)

        # Apply patch
        file_to_replace = os.path.join(
            newest_version_path, "resources/app.asar")
        try:
            os.remove(file_to_replace)
        except FileNotFoundError:
            pass
        except Exception:
            QMessageBox.critical(self, tr("Error"), tr("WeMod is currently running, please close the application first."))
            return
        shutil.copyfile(self.crackFile_path, file_to_replace)

        QMessageBox.information(self, tr("Success"), tr("You have now activated WeMod Pro!\nCurrent WeMod version: v") + newest_version_folder.strip('-app'))


if __name__ == "__main__":
    app = QApplication(sys.argv)

    # Language setting
    font_config = {
        "en_US": resource_path("assets/NotoSans-Regular.ttf"),
        "zh_CN": resource_path("assets/NotoSansSC-Regular.ttf"),
        "zh_TW": resource_path("assets/NotoSansTC-Regular.ttf")
    }
    fontId = QFontDatabase.addApplicationFont(
        font_config[settings["language"]])
    fontFamilies = QFontDatabase.applicationFontFamilies(fontId)
    customFont = QFont(fontFamilies[0], 10)
    app.setFont(customFont)

    mainWin = GameCheatsManager()
    mainWin.show()

    # Center window
    qr = mainWin.frameGeometry()
    cp = mainWin.screen().availableGeometry().center()
    qr.moveCenter(cp)
    mainWin.move(qr.topLeft())

    sys.exit(app.exec())
