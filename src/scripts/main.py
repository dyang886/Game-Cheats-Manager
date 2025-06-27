import asyncio
import ctypes
import os
from pathlib import Path
from queue import Queue
import shutil
import stat
import subprocess
import sys

from desktop_notifier import Button, DesktopNotifier, Urgency
import qasync
from PyQt6.QtCore import QTimer
from PyQt6.QtGui import QAction, QColor, QFont, QFontDatabase, QIcon
from PyQt6.QtWidgets import QApplication, QFileDialog, QGridLayout, QHBoxLayout, QLabel, QLineEdit, QListWidgetItem, QMainWindow, QMessageBox, QStatusBar, QVBoxLayout, QWidget
from tendo import singleton
from PyQt6.QtSvgWidgets import QSvgWidget

import style_sheet
from widgets.custom_dialogs import *
from widgets.custom_widgets import *
from widgets.trainer_management import *
from threads.download_display_thread import *
from threads.download_trainers_thread import *
from threads.other_threads import *
from threads.update_trainers_thread import *


class GameCheatsManager(QMainWindow):

    def __init__(self):
        super().__init__()

        # Single instance check and basic UI setup
        try:
            self.single_instance_checker = singleton.SingleInstance()
        except singleton.SingleInstanceException:
            sys.exit(1)
        except Exception as e:
            print(str(e))

        self.setWindowTitle("Game Cheats Manager")
        self.setWindowIcon(QIcon(resource_path("assets/logo.ico")))
        self.setMinimumSize(680, 520)

        # Version and links
        self.appVersion = "1.0.0"
        self.githubLink = "https://github.com/dyang886/Game-Cheats-Manager"
        self.bilibiliLink = "https://space.bilibili.com/256673766"

        # Variable management
        self.trainerSearchEntryPrompt = tr("Search for installed")
        self.downloadSearchEntryPrompt = tr("Search to download")
        self.trainerDownloadPath = os.path.normpath(settings["downloadPath"])

        self.trainers = {}  # Store installed trainers: {trainer name: trainer path}
        self.searchable = True  # able to search online trainers or not
        self.downloadable = False  # able to double click on download list or not
        self.downloadQueue = Queue()
        self.currentlyDownloading = False
        self.currentlyUpdatingTrainers = False
        self.currentlyUpdatingFling = False
        self.currentlyUpdatingXiaoXing = False
        self.currentlyUpdatingTrans = False

        # Window references
        self.settings_window = None
        self.about_window = None
        self.trainer_manage_window = None

        # Main widget group
        centralWidget = QWidget(self)
        self.setCentralWidget(centralWidget)
        mainLayout = QGridLayout(centralWidget)
        mainLayout.setSpacing(15)
        mainLayout.setContentsMargins(30, 20, 30, 10)
        centralWidget.setLayout(mainLayout)
        self.init_settings()

        # Menu setup
        menu = self.menuBar()

        # Options menu
        optionMenu = menu.addMenu(tr("Options"))

        settingsAction = QAction(tr("Settings"), self)
        settingsAction.triggered.connect(self.open_settings)
        optionMenu.addAction(settingsAction)

        importAction = QAction(tr("Import Trainers"), self)
        importAction.triggered.connect(self.import_files)
        optionMenu.addAction(importAction)

        openDirectoryAction = QAction(tr("Open Trainer Download Path"), self)
        openDirectoryAction.triggered.connect(self.open_trainer_directory)
        optionMenu.addAction(openDirectoryAction)

        whiteListAction = QAction(tr("Add Paths to Whitelist"), self)
        whiteListAction.triggered.connect(self.add_whitelist)
        optionMenu.addAction(whiteListAction)

        aboutAction = QAction(tr("About"), self)
        aboutAction.triggered.connect(self.open_about)
        optionMenu.addAction(aboutAction)

        # Data update menu
        dataMenu = menu.addMenu(tr("Data Update"))

        updateTranslations = QAction(tr("Update Translation Data"), self)
        updateTranslations.triggered.connect(self.fetch_trainer_translations)
        dataMenu.addAction(updateTranslations)

        updateTrainerSearchData = QAction(tr("Update Trainer Search Data"), self)
        updateTrainerSearchData.triggered.connect(self.fetch_trainer_search_data)
        dataMenu.addAction(updateTrainerSearchData)

        updateTrainers = QAction(tr("Update Trainers"), self)
        updateTrainers.triggered.connect(self.update_trainers)
        dataMenu.addAction(updateTrainers)

        # Trainer management menu
        trainerManagement = QAction(tr("Trainer Management"), self)
        trainerManagement.triggered.connect(self.open_trainer_management)
        menu.addAction(trainerManagement)

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

        searchSvg = '<svg xmlns="http://www.w3.org/2000/svg" height="10" viewBox="0 0 512 512"><path fill="gray" d="M416 208c0 45.9-14.9 88.3-40 122.7L502.6 457.4c12.5 12.5 12.5 32.8 0 45.3s-32.8 12.5-45.3 0L330.7 376c-34.4 25.2-76.8 40-122.7 40C93.1 416 0 322.9 0 208S93.1 0 208 0S416 93.1 416 208zM208 352a144 144 0 1 0 0-288 144 144 0 1 0 0 288z"/></svg>'
        searchSvgWidget = QSvgWidget()
        searchSvgWidget.renderer().load(searchSvg.encode('utf-8'))
        searchSvgWidget.setFixedSize(22, 22)
        trainerSearchLayout.addWidget(searchSvgWidget)

        self.trainerSearchEntry = QLineEdit()
        trainerSearchLayout.addWidget(self.trainerSearchEntry)
        self.trainerSearchEntry.setPlaceholderText(self.trainerSearchEntryPrompt)
        self.trainerSearchEntry.textChanged.connect(self.update_list)

        # Display installed trainers
        self.flingListBox = MultilingualListWidget()
        self.flingListBox.itemActivated.connect(self.launch_trainer)
        trainersLayout.addWidget(self.flingListBox)

        # Launch and delete buttons
        bottomLayout = QHBoxLayout()
        bottomLayout.setSpacing(6)
        trainersLayout.addLayout(bottomLayout)

        self.launchButton = CustomButton(tr("Launch"))
        bottomLayout.addWidget(self.launchButton)
        self.launchButton.clicked.connect(self.launch_trainer)

        self.deleteButton = CustomButton(tr("Delete"))
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

        downloadSearchSvgWidget = QSvgWidget()
        downloadSearchSvgWidget.renderer().load(searchSvg.encode('utf-8'))
        downloadSearchSvgWidget.setFixedSize(22, 22)
        downloadSearchLayout.addWidget(downloadSearchSvgWidget)

        self.downloadSearchEntry = QLineEdit()
        self.downloadSearchEntry.setPlaceholderText(self.downloadSearchEntryPrompt)
        self.downloadSearchEntry.returnPressed.connect(self.on_enter_press)
        downloadSearchLayout.addWidget(self.downloadSearchEntry)

        # Display trainer search results
        self.downloadListBox = MultilingualListWidget()
        self.downloadListBox.itemActivated.connect(self.on_download_start)
        downloadsLayout.addWidget(self.downloadListBox)

        # Change trainer download path
        changeDownloadPathLayout = QVBoxLayout()
        changeDownloadPathLayout.setSpacing(2)
        downloadsLayout.addLayout(changeDownloadPathLayout)

        changeDownloadPathLayout.addWidget(QLabel(tr("Trainer download path:")))

        downloadPathLayout = QHBoxLayout()
        downloadPathLayout.setSpacing(5)
        changeDownloadPathLayout.addLayout(downloadPathLayout)

        self.downloadPathEntry = QLineEdit()
        self.downloadPathEntry.setReadOnly(True)
        self.downloadPathEntry.setText(self.trainerDownloadPath)
        downloadPathLayout.addWidget(self.downloadPathEntry)

        self.fileDialogButton = CustomButton("...")
        downloadPathLayout.addWidget(self.fileDialogButton)
        self.fileDialogButton.clicked.connect(self.change_path)

        self.show_cheats()

        # Show warning pop up
        if settings["showWarning"]:
            dialog = CopyRightWarning(self)
            dialog.show()

        # Check for software update
        if settings['checkAppUpdate']:
            self.versionFetcher = VersionFetchWorker('GCM')
            self.versionFetcher.versionFetched.connect(lambda latest_version: asyncio.get_event_loop().create_task(self.send_notification(True, latest_version)))
            self.versionFetcher.fetchFailed.connect(lambda: asyncio.get_event_loop().create_task(self.send_notification(False)))
            self.versionFetcher.start()

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
        os._exit(0)

    def start_update(self, version):
        try:
            pid = str(os.getpid())
            s3_path = f'GCM/Game Cheats Manager Setup {version}.exe'
            subprocess.Popen(
                [updater_path, '--pid', pid, '--s3-path', s3_path, '--theme', settings["theme"], '--language', settings["language"]],
                creationflags=subprocess.DETACHED_PROCESS | subprocess.CREATE_NEW_PROCESS_GROUP,
                start_new_session=True
            )

        except subprocess.SubprocessError:
            QMessageBox.critical(self, tr("Failure"), tr("Failed to update application."))

    async def send_notification(self, success, latest_version=0):
        notifier = DesktopNotifier(app_name="Game Cheats Manager", app_icon=Path(resource_path("assets/logo.ico")))

        if success and latest_version > self.appVersion:
            await notifier.send(
                title=tr('Update Available'),
                message=tr('New version found: {old_version} ➜ {new_version}').format(
                    old_version=self.appVersion,
                    new_version=latest_version
                ) + '\n' + tr('Would you like to update now?'),
                urgency=Urgency.Normal,
                buttons=[
                    Button(title=tr("Yes"), on_pressed=lambda: self.start_update(latest_version)),
                    Button(title=tr("No"))
                ],
            )

        elif not success:
            await notifier.send(
                title=tr('Update Check Failed'),
                message=tr('Failed to check for software update. You can navigate to `Options` ➜ `About` to check for updates manually.'),
                urgency=Urgency.Normal
            )

        self.versionFetcher.quit()

    def init_settings(self):
        if settings["theme"] == "dark":
            style = style_sheet.black
        elif settings["theme"] == "light":
            style = style_sheet.white

        style = style.format(
            check_mark=checkMark_path,
            drop_down_arrow=dropDownArrow_path,
            scroll_bar_top=upArrow_path,
            scroll_bar_bottom=downArrow_path,
            scroll_bar_left=leftArrow_path,
            scroll_bar_right=rightArrow_path,
        )
        self.setStyleSheet(style)

    def on_enter_press(self):
        keyword = self.downloadSearchEntry.text()
        if keyword and self.searchable:
            self.download_display(keyword)

    def on_download_start(self, item):
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
            if os.path.isfile(trainerPath):
                trainerName, trainerExt = os.path.splitext(os.path.basename(trainerPath))
                if trainerExt.lower() == ".exe" and os.path.getsize(trainerPath) != 0:
                    self.flingListBox.addItem(trainerName)
                    self.trainers[trainerName] = trainerPath
            else:
                exe_exclusions = ["flashplayer_22.0.0.210_ax_debug.exe"]
                trainerName = os.path.basename(trainerPath)
                exe_file_path = None
                for file in os.scandir(trainerPath):
                    filePath = os.path.normpath(file.path)
                    fileExt = os.path.splitext(file.name)[1]
                    if file.is_file() and fileExt.lower() == ".exe" and file.name not in exe_exclusions:
                        exe_file_path = filePath
                        break
                if exe_file_path:
                    self.flingListBox.addItem(trainerName)
                    self.trainers[trainerName] = exe_file_path

    def launch_trainer(self):
        try:
            selection = self.flingListBox.currentRow()
            if selection != -1:
                trainerName = self.flingListBox.item(selection).text()
                trainerPath = os.path.normpath(self.trainers[trainerName])
                trainerDir = os.path.dirname(trainerPath)

                ctypes.windll.shell32.ShellExecuteW(
                    None, "runas", trainerPath, None, trainerDir, 1
                )
        except Exception as e:
            print(str(e))

    def delete_trainer(self):
        index = self.flingListBox.currentRow()
        if index != -1:
            trainerName = self.flingListBox.item(index).text()
            trainerPath = self.trainers[trainerName]

            msg_box = QMessageBox(
                QMessageBox.Icon.Question,
                tr('Delete trainer'),
                tr('Are you sure you want to delete ') + f"{trainerName}?",
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
                self
            )

            yes_button = msg_box.button(QMessageBox.StandardButton.Yes)
            yes_button.setText(tr("Confirm"))
            no_button = msg_box.button(QMessageBox.StandardButton.No)
            no_button.setText(tr("Cancel"))
            reply = msg_box.exec()

            if reply == QMessageBox.StandardButton.Yes:
                try:
                    os.chmod(trainerPath, stat.S_IWRITE)
                    parent_dir = os.path.dirname(trainerPath)
                    if os.path.basename(parent_dir) == trainerName:
                        shutil.rmtree(parent_dir)
                    else:
                        os.remove(trainerPath)
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
            changedPath = os.path.normpath(os.path.join(folder, "GCM Trainers"))
            if self.downloadPathEntry.text() == changedPath:
                QMessageBox.critical(self, tr("Error"), tr("Please choose a new path."))
                self.on_message(tr("Failed to change trainer download path."), "failure")
                self.enable_all_widgets()
                return

            self.downloadListBox.addItem(tr("Migrating existing trainers..."))
            migration_thread = PathChangeThread(self.trainerDownloadPath, changedPath, self)
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

    def fetch_trainer_translations(self):
        if not self.currentlyUpdatingTrans:
            self.currentlyUpdatingTrans = True
            fetch_trainer_details_thread = FetchTrainerTranslations(self)
            fetch_trainer_details_thread.message.connect(self.on_status_load)
            fetch_trainer_details_thread.update.connect(self.on_status_update)
            fetch_trainer_details_thread.finished.connect(self.on_interval_finished)
            fetch_trainer_details_thread.start()

    def fetch_fling_data(self):
        if not self.currentlyUpdatingFling:
            self.currentlyUpdatingFling = True
            fetch_fling_site_thread = FetchFlingSite(self)
            fetch_fling_site_thread.message.connect(self.on_status_load)
            fetch_fling_site_thread.update.connect(self.on_status_update)
            fetch_fling_site_thread.finished.connect(self.on_interval_finished)
            fetch_fling_site_thread.start()

    def update_trainers(self, auto_check=False):
        if not self.currentlyUpdatingTrainers:
            self.currentlyUpdatingTrainers = True
            trainer_update_thread = UpdateTrainers(self.trainers, auto_check, self)
            trainer_update_thread.message.connect(self.on_status_load)
            trainer_update_thread.update.connect(self.on_status_update)
            trainer_update_thread.updateTrainer.connect(self.on_trainer_update)
            trainer_update_thread.finished.connect(self.on_interval_finished)
            trainer_update_thread.start()

    def fetch_xiaoxing_data(self):
        if not self.currentlyUpdatingXiaoXing:
            self.currentlyUpdatingXiaoXing = True
            fetch_xiaoxing_site_thread = FetchXiaoXingSite(self)
            fetch_xiaoxing_site_thread.message.connect(self.on_status_load)
            fetch_xiaoxing_site_thread.update.connect(self.on_status_update)
            fetch_xiaoxing_site_thread.finished.connect(self.on_interval_finished)
            fetch_xiaoxing_site_thread.start()

    def fetch_trainer_search_data(self):
        self.fetch_fling_data()
        if settings["enableXiaoXing"]:
            self.fetch_xiaoxing_data()

    def on_main_interval(self):
        if settings["autoUpdateTranslations"]:
            self.fetch_trainer_translations()
        if settings["autoUpdateFlingData"]:
            self.fetch_fling_data()
        if settings["autoUpdateXiaoXingData"]:
            self.fetch_xiaoxing_data()
        if settings["autoUpdateFlingTrainers"] or settings["autoUpdateXiaoXingTrainers"]:
            self.update_trainers(True)

    def download_trainers(self, index):
        self.enqueue_download(index, self.trainers, self.trainerDownloadPath, None)

    def on_trainer_update(self, update_entry):
        self.enqueue_download(None, None, self.trainerDownloadPath, update_entry)

    def enqueue_download(self, index, trainers, trainerDownloadPath, update_entry):
        self.downloadQueue.put((index, trainers, trainerDownloadPath, update_entry))
        if not self.currentlyDownloading:
            self.start_next_download()

    def start_next_download(self):
        if not self.downloadQueue.empty():
            self.currentlyDownloading = True
            self.disable_download_widgets()
            self.downloadListBox.clear()
            self.downloadable = False
            self.searchable = False

            index, trainers, trainerDownloadPath, update_entry = self.downloadQueue.get()
            download_thread = DownloadTrainersThread(index, trainers, trainerDownloadPath, update_entry, self)
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
            # item.setBackground(QColor(0, 255, 0, 20))
            self.downloadListBox.addItem(item)
        elif type == "failure":
            item.setForeground(QColor('red'))
            # item.setBackground(QColor(255, 0, 0, 20))
            self.downloadListBox.addItem(item)
        else:
            self.downloadListBox.addItem(item)

    def on_message_box(self, type, title, text):
        if type == "info":
            QMessageBox.information(self, title, text)
        elif type == "error":
            QMessageBox.critical(self, title, text)

    def on_migration_error(self, error_message):
        QMessageBox.critical(self, tr("Error"), tr("Error migrating trainers: ") + error_message)
        self.on_message(tr("Failed to change trainer download path."), "failure")
        self.show_cheats()
        self.enable_all_widgets()

    def on_migration_finished(self, new_path):
        self.trainerDownloadPath = new_path
        settings["downloadPath"] = self.trainerDownloadPath
        apply_settings(settings)
        self.show_cheats()
        self.on_message(tr("Migration complete!"), "success")
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
        elif widgetName == "xiaoxing":
            self.currentlyUpdatingXiaoXing = False
        elif widgetName == "translations":
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
                try:
                    dst = os.path.join(self.trainerDownloadPath, os.path.basename(file_name))
                    if os.path.exists(dst):
                        os.chmod(dst, stat.S_IWRITE)
                    shutil.copy(file_name, dst)
                    print("Trainer copied: ", file_name)
                except Exception as e:
                    QMessageBox.critical(self, tr("Failure"), tr("Failed to import trainer: ") + f"{file_name}\n{str(e)}")
                self.show_cheats()

            msg_box = QMessageBox(
                QMessageBox.Icon.Question,
                tr("Delete original trainers"),
                tr("Do you want to delete the original trainer files?"),
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
                self
            )

            yes_button = msg_box.button(QMessageBox.StandardButton.Yes)
            yes_button.setText(tr("Yes"))
            no_button = msg_box.button(QMessageBox.StandardButton.No)
            no_button.setText(tr("No"))
            reply = msg_box.exec()

            if reply == QMessageBox.StandardButton.Yes:
                for file_name in file_names:
                    try:
                        os.remove(file_name)
                    except Exception as e:
                        QMessageBox.critical(self, tr("Failure"), tr("Failed to delete original trainer: ") + f"{file_name}\n{str(e)}")

    def open_trainer_directory(self):
        os.startfile(self.trainerDownloadPath)

    def add_whitelist(self):
        msg_box = QMessageBox(
            QMessageBox.Icon.Question,
            tr("Administrator Access Required"),
            tr("To proceed with adding the trainer download paths to the Windows Defender whitelist, administrator rights are needed. A User Account Control (UAC) prompt will appear for permission.") +
            "\n\n" + tr("Would you like to continue?"),
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            self
        )

        yes_button = msg_box.button(QMessageBox.StandardButton.Yes)
        yes_button.setText(tr("Yes"))
        no_button = msg_box.button(QMessageBox.StandardButton.No)
        no_button.setText(tr("No"))
        reply = msg_box.exec()

        if reply == QMessageBox.StandardButton.Yes:
            paths = [DOWNLOAD_TEMP_DIR, settings["downloadPath"]]

            try:
                subprocess.run([elevator_path, 'whitelist'] + paths, check=True, shell=True)
                QMessageBox.information(self, tr("Success"), tr("Successfully added paths to Windows Defender whitelist."))

            except subprocess.CalledProcessError:
                QMessageBox.critical(self, tr("Failure"), tr("Failed to add paths to Windows Defender whitelist."))

    def open_about(self):
        if self.about_window is not None and self.about_window.isVisible():
            self.about_window.raise_()
            self.about_window.activateWindow()
        else:
            self.about_window = AboutDialog(self)
            self.about_window.show()

    def open_trainer_management(self):
        if self.trainer_manage_window is not None and self.trainer_manage_window.isVisible():
            self.trainer_manage_window.raise_()
            self.trainer_manage_window.activateWindow()
        else:
            self.trainer_manage_window = TrainerManagementDialog(self)
            self.trainer_manage_window.show()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    loop = qasync.QEventLoop(app)
    asyncio.set_event_loop(loop)

    # Load the selected font
    primary_font_path = font_config[settings["language"]]
    primary_font_id = QFontDatabase.addApplicationFont(primary_font_path)
    primary_font_families = QFontDatabase.applicationFontFamilies(primary_font_id)
    custom_font = QFont(primary_font_families[0], 10)
    custom_font.setHintingPreference(QFont.HintingPreference.PreferNoHinting)
    app.setFont(custom_font)

    mainWin = GameCheatsManager()
    mainWin.show()

    # Center window
    qr = mainWin.frameGeometry()
    cp = mainWin.screen().availableGeometry().center()
    qr.moveCenter(cp)
    mainWin.move(qr.topLeft())

    with loop:
        sys.exit(loop.run_forever())
