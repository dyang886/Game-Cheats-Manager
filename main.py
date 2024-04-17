import os
from queue import Queue
import re
import shutil
import sys

from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtGui import QAction, QColor, QFont, QFontDatabase, QIcon, QPixmap
from PyQt6.QtWidgets import QApplication, QFileDialog, QGridLayout, QHBoxLayout, QLabel, QLineEdit, QListWidgetItem, QMainWindow, QMessageBox, QPushButton, QStatusBar, QVBoxLayout, QWidget, QListWidget
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
        self.setMinimumSize(670, 500)

        # Version, user prompts, and links
        self.appVersion = "2.0.0"
        self.githubLink = "https://github.com/dyang886/Game-Cheats-Manager"
        self.updateLink = "https://api.github.com/repos/dyang886/Game-Cheats-Manager/releases/latest"
        self.trainerSearchEntryPrompt = tr("Search for installed")
        self.downloadSearchEntryPrompt = tr("Search to download")

        # Paths and variable management
        self.trainerDownloadPath = os.path.normpath(
            os.path.abspath(settings["downloadPath"]))
        os.makedirs(self.trainerDownloadPath, exist_ok=True)
        
        self.dropDownArrow_path = resource_path("assets/dropdown.png").replace("\\", "/")
        self.upArrow_path = resource_path("assets/up.png").replace("\\", "/")
        self.downArrow_path = resource_path("assets/down.png").replace("\\", "/")
        self.leftArrow_path = resource_path("assets/left.png").replace("\\", "/")
        self.rightArrow_path = resource_path("assets/right.png").replace("\\", "/")
        
        self.crackFile_path = resource_path("dependency/app.asar")
        self.search_path = resource_path("assets/search.png")

        self.trainers = {}  # Store installed trainers: {trainer name: trainer path}
        self.searchable = True  # able to search online trainers or not
        self.downloadable = False  # able to double click on download list or not
        self.downloadQueue = Queue()
        self.currentlyDownloading = False
        self.currentlyUpdatingTrainers = False
        self.currentlyUpdatingFling = False
        self.currentlyUpdatingTrans = False

        # Window references
        self.settings_window = None
        self.about_window = None

        # Main widget group
        centralWidget = QWidget(self)
        self.setCentralWidget(centralWidget)
        mainLayout = QGridLayout(centralWidget)
        mainLayout.setSpacing(15)
        mainLayout.setContentsMargins(30, 20, 30, 10)
        centralWidget.setLayout(mainLayout)
        self.init_settings()

        # Menu setup
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

        aboutAction = QAction(tr("About"), self)
        aboutAction.setFont(menuFont)
        aboutAction.triggered.connect(self.open_about)
        optionMenu.addAction(aboutAction)

        # Below are standalone menu actions
        wemodAction = QAction(tr("Unlock WeMod Pro"), self)
        wemodAction.setFont(menuFont)
        wemodAction.triggered.connect(self.wemod_pro)
        menu.addAction(wemodAction)

        updateDatabaseAction = QAction(tr("Update Trainer Database"), self)
        updateDatabaseAction.setFont(menuFont)
        updateDatabaseAction.triggered.connect(self.fetch_database)
        menu.addAction(updateDatabaseAction)

        updateTrainersAction = QAction(tr("Update Trainers"), self)
        updateTrainersAction.setFont(menuFont)
        updateTrainersAction.triggered.connect(self.update_trainers)
        menu.addAction(updateTrainersAction)

        # Status bar setup
        self.statusbar = QStatusBar()
        self.setStatusBar(self.statusbar)

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
        self.downloadPathEntry.setText(self.trainerDownloadPath)
        changeDownloadPathLayout.addWidget(self.downloadPathEntry)

        self.fileDialogButton = QPushButton("...")
        changeDownloadPathLayout.addWidget(self.fileDialogButton)
        self.fileDialogButton.clicked.connect(self.change_path)

        self.show_cheats()

        # Update database, trainer update
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.on_main_interval)
        self.on_main_interval()
        self.timer.start(3600000)

    # ===========================================================================
    # Core functions
    # ===========================================================================
    def closeEvent(self, event):
        super().closeEvent(event)
        os._exit(1)

    def init_settings(self):
        if settings["theme"] == "black":
            style = style_sheet.black
        elif settings["theme"] == "white":
            style = style_sheet.white

        style = style.format(
            drop_down_arrow=self.dropDownArrow_path,
            scroll_bar_top=self.upArrow_path,
            scroll_bar_bottom=self.downArrow_path,
            scroll_bar_left=self.leftArrow_path,
            scroll_bar_right=self.rightArrow_path,
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
        if search_text == "":
            self.show_cheats()
            return

        self.flingListBox.clear()
        for trainerName in self.trainers.keys():
            if search_text in trainerName.lower():
                self.flingListBox.addItem(trainerName)

    def show_cheats(self):
        self.flingListBox.clear()
        self.trainers = {}
        entries = sorted(
            os.scandir(self.trainerDownloadPath),
            key=lambda dirent: sort_trainers_key(dirent.name)
        )

        for trainer in entries:
            trainerPath = os.path.normpath(trainer.path)
            trainerName, trainerExt = os.path.splitext(os.path.basename(trainerPath))
            if trainerExt.lower() == ".exe" and os.path.getsize(trainerPath) != 0:
                self.flingListBox.addItem(trainerName)
                self.trainers[trainerName] = trainerPath

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
            try:
                trainerName = self.flingListBox.item(index).text()
                os.remove(self.trainers[trainerName])
                self.flingListBox.takeItem(index)
                self.show_cheats()
            except PermissionError as e:
                QMessageBox.critical(self, tr("Error"), tr("Trainer is currently in use, please close any programs using the file and try again."))
    
    def findWidgetInStatusBar(self, statusbar, widgetName):
        for widget in statusbar.children():
            if widget.objectName() == widgetName:
                return widget
        return None

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
            migration_thread = PathChangeThread(self.trainerDownloadPath, folder, self)
            migration_thread.finished.connect(self.on_migration_finished)
            migration_thread.error.connect(self.on_migration_error)
            migration_thread.start()
        
        else:
            self.downloadListBox.addItem(tr("No path selected."))
            self.enable_all_widgets()
            return
    
    def download_display(self, keyword):
        self.disable_download_widgets()
        self.downloadListBox.clear()
        self.downloadable = False
        self.searchable = False

        display_thread = DownloadDisplayThread(keyword, self)
        display_thread.message.connect(self.on_message)
        display_thread.finished.connect(self.on_display_finished)
        display_thread.start()
    
    def fetch_database(self):
        if not self.currentlyUpdatingFling:
            self.currentlyUpdatingFling = True
            fetch_fling_site_thread = FetchFlingSite(self)
            fetch_fling_site_thread.message.connect(self.on_status_load)
            fetch_fling_site_thread.update.connect(self.on_status_update)
            fetch_fling_site_thread.finished.connect(self.on_interval_finished)
            fetch_fling_site_thread.start()

        if not self.currentlyUpdatingTrans:
            self.currentlyUpdatingTrans = True
            fetch_trainer_details_thread = FetchTrainerDetails(self)
            fetch_trainer_details_thread.message.connect(self.on_status_load)
            fetch_trainer_details_thread.update.connect(self.on_status_update)
            fetch_trainer_details_thread.finished.connect(self.on_interval_finished)
            fetch_trainer_details_thread.start()
    
    def update_trainers(self):
        if not self.currentlyUpdatingTrainers:
            self.currentlyUpdatingTrainers = True
            trainer_update_thread = UpdateTrainers(self.trainers, self)
            trainer_update_thread.message.connect(self.on_status_load)
            trainer_update_thread.update.connect(self.on_status_update)
            trainer_update_thread.updateTrainer.connect(self.on_trainer_update)
            trainer_update_thread.finished.connect(self.on_interval_finished)
            trainer_update_thread.start()
    
    def on_main_interval(self):
        if settings["autoUpdateDatabase"]:
            self.fetch_database()
        if settings["autoUpdate"]:
            self.update_trainers()

    def download_trainers(self, index):
        self.enqueue_download(index, self.trainers, self.trainerDownloadPath, False, None, None)
    
    def on_trainer_update(self, trainerPath, updateUrl):
        self.enqueue_download(None, None, self.trainerDownloadPath, True, trainerPath, updateUrl)
    
    def enqueue_download(self, index, trainers, trainerDownloadPath, update, trainerPath, updateUrl):
        self.downloadQueue.put((index, trainers, trainerDownloadPath, update, trainerPath, updateUrl))
        if not self.currentlyDownloading:
            self.start_next_download()

    def start_next_download(self):
        if not self.downloadQueue.empty():
            self.currentlyDownloading = True
            self.disable_download_widgets()
            self.downloadListBox.clear()
            self.downloadable = False
            self.searchable = False

            index, trainers, trainerDownloadPath, update, trainerPath, updateUrl = self.downloadQueue.get()
            download_thread = DownloadTrainersThread(index, trainers, trainerDownloadPath, update, trainerPath, updateUrl, self)
            download_thread.message.connect(self.on_message)
            download_thread.messageBox.connect(self.on_message_box)
            download_thread.finished.connect(self.on_download_finished)
            download_thread.start()
        else:
            self.currentlyDownloading = False

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
    
    def on_message_box(self, type, title, text):
        if type == "info":
            QMessageBox.information(self, title, text)
        elif type == "error":
            QMessageBox.critical(self, title, text)
    
    def on_migration_error(self, error_message):
        QMessageBox.critical(self, tr("Error"), tr("Error creating the new path: ") + error_message)
        self.enable_all_widgets()
    
    def on_migration_finished(self, new_path):
        self.trainerDownloadPath = new_path
        settings["downloadPath"] = self.trainerDownloadPath
        apply_settings(settings)
        self.show_cheats()
        self.downloadListBox.addItem(tr("Migration complete!"))
        self.downloadPathEntry.setText(self.trainerDownloadPath)
        self.enable_all_widgets()

    def on_display_finished(self, status):
        # 0: success; 1: failure
        if status:
            self.downloadable = False
        else:
            self.downloadable = True
            
        self.searchable = True
        self.enable_download_widgets()
    
    def on_download_finished(self, status):
        self.downloadable = False
        self.searchable = True
        self.enable_download_widgets()
        self.show_cheats()
        self.currentlyDownloading = False
        self.start_next_download()

    def on_status_load(self, widgetName, message):
        statusWidget = StatusMessageWidget(widgetName, message)
        self.statusbar.addWidget(statusWidget)

    def on_status_update(self, widgetName, newMessage, state):
        target = self.findWidgetInStatusBar(self.statusbar, widgetName)
        target.update_message(newMessage, state)

    def on_interval_finished(self, widgetName):
        target = self.findWidgetInStatusBar(self.statusbar, widgetName)
        if target:
            target.deleteLater()

        if widgetName == "fling":
            self.currentlyUpdatingFling = False
        elif widgetName == "details":
            self.currentlyUpdatingTrans = False
        elif widgetName == "trainerUpdate":
            self.currentlyUpdatingTrainers = False

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
        file_names, _ = QFileDialog.getOpenFileNames(self, tr("Select trainers you want to import"), "", "Executable Files (*.exe)")
        if file_names:
            for file_name in file_names:
                dest_path = os.path.join(self.trainerDownloadPath, os.path.basename(file_name))
                shutil.move(file_name, dest_path)
                print("Trainer moved: ", file_name)
            self.show_cheats()

    def open_trainer_directory(self):
        os.startfile(self.trainerDownloadPath)

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
