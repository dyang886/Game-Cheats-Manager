import os
import sys
import winreg as reg

from PyQt6.QtCore import Qt
from PyQt6.QtGui import QIcon, QPixmap
from PyQt6.QtWidgets import QCheckBox, QComboBox, QDialog, QFileDialog, QHBoxLayout, QLabel, QLineEdit, QMessageBox, QProgressBar, QSizePolicy, QTextEdit, QVBoxLayout

from config import *
from widgets.custom_widgets import CustomButton
from threads.other_threads import VersionFetchWorker, TrainerUploadWorker


class AnnouncementDialog(QDialog):
    def __init__(self, announcement_data, parent=None):
        super().__init__(parent)
        self.announcement_id = announcement_data.get("id", "")

        self.setWindowTitle(tr("Announcement"))
        self.setWindowIcon(QIcon(resource_path("assets/logo.ico")))
        self.setMinimumWidth(850)

        layout = QVBoxLayout()
        layout.setSpacing(15)
        layout.setContentsMargins(25, 20, 25, 20)
        self.setLayout(layout)

        # Determine display language
        lang = settings.get("language")
        if lang in ["zh_CN", "zh_TW"]:
            title = announcement_data.get("title_zh", announcement_data.get("title_en", ""))
            message = announcement_data.get("message_zh", announcement_data.get("message_en", ""))
        else:
            title = announcement_data.get("title_en", announcement_data.get("title_zh", ""))
            message = announcement_data.get("message_en", announcement_data.get("message_zh", ""))

        # Title
        titleLabel = QLabel(title)
        titleFont = self.font()
        titleFont.setPointSize(18)
        titleFont.setBold(True)
        titleLabel.setFont(titleFont)
        titleLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(titleLabel)

        # Message body
        messageLabel = QLabel(message)
        messageLabel.setWordWrap(True)
        messageLabel.setTextFormat(Qt.TextFormat.RichText)
        messageLabel.setOpenExternalLinks(True)
        messageFont = self.font()
        messageFont.setPointSize(11)
        messageLabel.setFont(messageFont)
        layout.addWidget(messageLabel)

        # OK button
        okButton = CustomButton("OK")
        okButton.clicked.connect(self.accept)
        buttonLayout = QHBoxLayout()
        buttonLayout.addStretch(1)
        buttonLayout.addWidget(okButton)
        buttonLayout.addStretch(1)
        layout.addLayout(buttonLayout)

        self.setFixedSize(self.sizeHint())

    def _mark_seen(self):
        settings["lastSeenAnnouncementId"] = self.announcement_id
        apply_settings(settings)

    def accept(self):
        self._mark_seen()
        super().accept()

    def closeEvent(self, event):
        self._mark_seen()
        super().closeEvent(event)


class CopyRightWarning(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle(tr("Warning"))
        self.setWindowIcon(QIcon(resource_path("assets/logo.ico")))
        layout = QVBoxLayout()
        layout.setSpacing(30)
        layout.setContentsMargins(20, 30, 20, 20)
        self.setLayout(layout)

        warningLayout = QHBoxLayout()
        layout.addLayout(warningLayout)

        WarningPixmap = QPixmap(resource_path("assets/warning.png"))
        scaledWarningPixmap = WarningPixmap.scaled(120, 120, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation)
        warningSign = QLabel()
        warningSign.setPixmap(scaledWarningPixmap)
        warningLayout.addWidget(warningSign)

        warningFont = self.font()
        warningFont.setPointSize(11)
        warningText = QLabel(tr("This software is open source and provided free of charge.\nResale is strictly prohibited, and developers reserve the right to pursue legal responsibility against the violators.\nIf you have paid for this software unofficially, please report the seller immediately.\nBelow are official links."))
        warningText.setFont(warningFont)
        warningText.setStyleSheet("color: red;")
        warningLayout.addWidget(warningText)

        linksLayout = QVBoxLayout()
        linksLayout.setSpacing(10)
        layout.addLayout(linksLayout)

        websiteUrl = self.parent().websiteLink
        websiteText = tr('Official Website:')
        websiteText = f'{websiteText} <a href="{websiteUrl}" style="text-decoration: none; color: #305CDE;">{websiteUrl}</a>'
        websiteLabel = QLabel(websiteText)
        websiteLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        websiteLabel.setTextFormat(Qt.TextFormat.RichText)
        websiteLabel.setOpenExternalLinks(True)
        linksLayout.addWidget(websiteLabel)

        githubUrl = self.parent().githubLink
        githubText = f'GitHub: <a href="{githubUrl}" style="text-decoration: none; color: #305CDE;">{githubUrl}</a>'
        githubLabel = QLabel(githubText)
        githubLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        githubLabel.setTextFormat(Qt.TextFormat.RichText)
        githubLabel.setOpenExternalLinks(True)
        linksLayout.addWidget(githubLabel)

        bilibiliUrl = self.parent().bilibiliLink
        text = tr("Bilibili:")
        bilibiliText = f'{text} <a href="{bilibiliUrl}" style="text-decoration: none; color: #305CDE;">{bilibiliUrl}</a>'
        bilibiliLabel = QLabel(bilibiliText)
        bilibiliLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        bilibiliLabel.setTextFormat(Qt.TextFormat.RichText)
        bilibiliLabel.setOpenExternalLinks(True)
        linksLayout.addWidget(bilibiliLabel)

        dontShowLayout = QHBoxLayout()
        dontShowLayout.setAlignment(Qt.AlignmentFlag.AlignRight)
        self.dontShowCheckbox = QCheckBox(tr("Don't show again"))
        dontShowLayout.addWidget(self.dontShowCheckbox)
        layout.addLayout(dontShowLayout)

        self.setFixedSize(self.sizeHint())

    def closeEvent(self, event):
        if self.dontShowCheckbox.isChecked():
            settings["showWarning"] = False
            apply_settings(settings)
        super().closeEvent(event)


class SettingsDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle(tr("Settings"))
        self.setWindowIcon(QIcon(resource_path("assets/setting.ico")))
        settingsLayout = QVBoxLayout()
        settingsLayout.setSpacing(15)
        self.setLayout(settingsLayout)
        self.setMinimumWidth(400)

        settingsWidgetsLayout = QVBoxLayout()
        settingsWidgetsLayout.setContentsMargins(50, 30, 50, 20)
        settingsLayout.addLayout(settingsWidgetsLayout)

        # Theme selection
        themeLayout = QVBoxLayout()
        themeLayout.setSpacing(2)
        settingsWidgetsLayout.addLayout(themeLayout)
        themeLabel = QLabel(tr("Theme:"))
        themeLabel.setSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Fixed)
        themeLayout.addWidget(themeLabel)
        self.themeCombo = QComboBox()
        self.themeCombo.addItems(theme_options.keys())
        self.themeCombo.setCurrentText(self.find_settings_key(settings["theme"], theme_options))
        themeLayout.addWidget(self.themeCombo)

        # Language selection
        languageLayout = QVBoxLayout()
        languageLayout.setSpacing(2)
        settingsWidgetsLayout.addLayout(languageLayout)
        languageLabel = QLabel(tr("Language:"))
        languageLabel.setSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Fixed)
        languageLayout.addWidget(languageLabel)
        self.languageCombo = QComboBox()
        self.languageCombo.addItems(language_options.keys())
        self.languageCombo.setCurrentText(self.find_settings_key(settings["language"], language_options))
        languageLayout.addWidget(self.languageCombo)

        # Launch app on startup
        self.launchAppOnStarupCheckbox = QCheckBox(tr("Launch app on system startup"))
        self.launchAppOnStarupCheckbox.setChecked(settings["launchAppOnStartup"])
        settingsWidgetsLayout.addWidget(self.launchAppOnStarupCheckbox)

        # Check software update at startup
        self.checkAppUpdateCheckbox = QCheckBox(tr("Check for software update at startup"))
        self.checkAppUpdateCheckbox.setChecked(settings["checkAppUpdate"])
        settingsWidgetsLayout.addWidget(self.checkAppUpdateCheckbox)

        # Legacy compatibility mode (Safe ASCII Launch)
        self.safePathCheckbox = QCheckBox(tr("Use safe launch path (may fix trainers unable to launch)"))
        self.safePathCheckbox.setChecked(settings["safePath"])
        settingsWidgetsLayout.addWidget(self.safePathCheckbox)

        # Always show english
        self.alwaysEnCheckbox = QCheckBox(tr("Always show search results in English"))
        self.alwaysEnCheckbox.setChecked(settings["enSearchResults"])
        settingsWidgetsLayout.addWidget(self.alwaysEnCheckbox)

        # Sort by origin
        self.sortByOriginCheckbox = QCheckBox(tr("Sort search results by origin"))
        self.sortByOriginCheckbox.setChecked(settings["sortByOrigin"])
        settingsWidgetsLayout.addWidget(self.sortByOriginCheckbox)

        # Auto update translation json
        self.autoUpdateTranslationsCheckbox = QCheckBox(tr("Update trainer translations automatically"))
        self.autoUpdateTranslationsCheckbox.setChecked(settings["autoUpdateTranslations"])
        settingsWidgetsLayout.addWidget(self.autoUpdateTranslationsCheckbox)

        # Apply button
        applyButtonLayout = QHBoxLayout()
        applyButtonLayout.setContentsMargins(0, 0, 10, 10)
        applyButtonLayout.addStretch(1)
        settingsLayout.addLayout(applyButtonLayout)
        self.applyButton = CustomButton(tr("Apply"))
        self.applyButton.setFixedWidth(100)
        self.applyButton.clicked.connect(self.apply_settings_page)
        applyButtonLayout.addWidget(self.applyButton)

    @staticmethod
    def find_settings_key(value, dict):
        return next(key for key, val in dict.items() if val == value)

    @staticmethod
    def add_or_remove_startup(app_name, path_to_exe, add=True):
        key = reg.HKEY_CURRENT_USER
        key_path = "Software\\Microsoft\\Windows\\CurrentVersion\\Run"
        try:
            registry_key = reg.OpenKey(key, key_path, 0, reg.KEY_WRITE)
            if add:
                reg.SetValueEx(registry_key, app_name, 0, reg.REG_SZ, path_to_exe)
            else:
                reg.DeleteValue(registry_key, app_name)
            reg.CloseKey(registry_key)
        except WindowsError as e:
            print(f"Registry modification failed: {str(e)}")

    def apply_settings_page(self):
        original_theme = settings["theme"]
        original_language = settings["language"]
        original_sortByOrigin = settings["sortByOrigin"]

        settings["theme"] = theme_options[self.themeCombo.currentText()]
        settings["language"] = language_options[self.languageCombo.currentText()]
        settings["safePath"] = self.safePathCheckbox.isChecked()
        settings["enSearchResults"] = self.alwaysEnCheckbox.isChecked()
        settings["sortByOrigin"] = self.sortByOriginCheckbox.isChecked()
        settings["checkAppUpdate"] = self.checkAppUpdateCheckbox.isChecked()
        settings["launchAppOnStartup"] = self.launchAppOnStarupCheckbox.isChecked()
        settings["autoUpdateTranslations"] = self.autoUpdateTranslationsCheckbox.isChecked()
        apply_settings(settings)

        if sys.argv[0].endswith('.exe'):
            app_name = "Game Cheats Manager"
            app_path = sys.argv[0]
            if self.launchAppOnStarupCheckbox.isChecked():
                self.add_or_remove_startup(app_name, app_path, True)
            else:
                self.add_or_remove_startup(app_name, app_path, False)

        if original_sortByOrigin != settings["sortByOrigin"]:
            self.parent().show_cheats()

        if original_theme != settings["theme"] or original_language != settings["language"]:
            msg_box = QMessageBox(
                QMessageBox.Icon.Question,
                tr("Attention"),
                tr("Do you want to restart the application now to apply theme or language settings?"),
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
                self
            )

            yes_button = msg_box.button(QMessageBox.StandardButton.Yes)
            yes_button.setText(tr("Yes"))
            no_button = msg_box.button(QMessageBox.StandardButton.No)
            no_button.setText(tr("No"))
            reply = msg_box.exec()

            if reply == QMessageBox.StandardButton.Yes:
                os.execl(sys.argv[0], sys.argv[0])

        else:
            QMessageBox.information(self, tr("Success"), tr("Settings saved."))


class AboutDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle(tr("About"))
        self.setWindowIcon(QIcon(resource_path("assets/logo.ico")))
        aboutLayout = QVBoxLayout()
        aboutLayout.setSpacing(30)
        aboutLayout.setContentsMargins(40, 20, 40, 30)
        self.setLayout(aboutLayout)

        appLayout = QHBoxLayout()
        appLayout.setSpacing(20)
        aboutLayout.addLayout(appLayout)

        # App logo
        logoPixmap = QPixmap(resource_path("assets/logo.png"))
        scaledLogoPixmap = logoPixmap.scaled(120, 120, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation)
        logoLabel = QLabel()
        logoLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        logoLabel.setPixmap(scaledLogoPixmap)
        if settings["theme"] == "light":
            logoLabel.setStyleSheet("""
                border: 2px solid black;
                border-radius: 20px;
                padding: -2px;
            """)
        appLayout.addWidget(logoLabel)

        # App name
        appNameFont = self.font()
        appNameFont.setPointSize(18)
        appInfoLayout = QVBoxLayout()
        appInfoLayout.setAlignment(Qt.AlignmentFlag.AlignVCenter)
        appLayout.addLayout(appInfoLayout)

        appNameLabel = QLabel("Game Cheats Manager")
        appNameLabel.setFont(appNameFont)
        appNameLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        appInfoLayout.addWidget(appNameLabel)

        # App version and update button
        versionLayout = QHBoxLayout()
        versionLayout.setSpacing(20)
        versionLayout.addStretch(1)
        appInfoLayout.addLayout(versionLayout)

        versionNumberLayout = QVBoxLayout()
        versionNumberLayout.setSpacing(2)
        versionLayout.addLayout(versionNumberLayout)

        currentVersionTextLabel = QLabel(tr("Current version: "))
        self.currentVersionNumberLabel = QLabel(self.parent().appVersion)
        currentVersionLayout = QHBoxLayout()
        currentVersionLayout.setSpacing(3)
        currentVersionLayout.addWidget(currentVersionTextLabel)
        currentVersionLayout.addWidget(self.currentVersionNumberLabel)
        versionNumberLayout.addLayout(currentVersionLayout)

        newestVersionTextLabel = QLabel(tr("Newest version: "))
        self.newestVersionNumberLabel = QLabel(tr("Loading..."))
        newestVersionLayout = QHBoxLayout()
        newestVersionLayout.setSpacing(3)
        newestVersionLayout.addWidget(newestVersionTextLabel)
        newestVersionLayout.addWidget(self.newestVersionNumberLabel)
        versionNumberLayout.addLayout(newestVersionLayout)

        self.updateButton = CustomButton(tr("Update Now"))
        self.updateButton.setStyleSheet("padding: 5;")
        self.updateButton.setSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Fixed)
        self.updateButton.setVisible(0)
        self.updateButton.clicked.connect(lambda: self.parent().start_update(self.newestVersionNumberLabel.text()))
        font = self.updateButton.font()
        font.setPointSize(8)
        versionLayout.addWidget(self.updateButton)
        versionLayout.addStretch(1)

        # Links
        linksLayout = QVBoxLayout()
        linksLayout.setSpacing(10)
        aboutLayout.addLayout(linksLayout)

        websiteUrl = self.parent().websiteLink
        websiteText = tr('Official Website:')
        websiteText = f'{websiteText} <a href="{websiteUrl}" style="text-decoration: none; color: #305CDE;">{websiteUrl}</a>'
        websiteLabel = QLabel(websiteText)
        websiteLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        websiteLabel.setTextFormat(Qt.TextFormat.RichText)
        websiteLabel.setOpenExternalLinks(True)
        linksLayout.addWidget(websiteLabel)

        githubUrl = self.parent().githubLink
        githubText = f'GitHub: <a href="{githubUrl}" style="text-decoration: none; color: #305CDE;">{githubUrl}</a>'
        githubLabel = QLabel(githubText)
        githubLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        githubLabel.setTextFormat(Qt.TextFormat.RichText)
        githubLabel.setOpenExternalLinks(True)
        linksLayout.addWidget(githubLabel)

        bilibiliUrl = self.parent().bilibiliLink
        text = tr("Bilibili:")
        bilibiliText = f'{text} <a href="{bilibiliUrl}" style="text-decoration: none; color: #305CDE;">{bilibiliUrl}</a>'
        bilibiliLabel = QLabel(bilibiliText)
        bilibiliLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        bilibiliLabel.setTextFormat(Qt.TextFormat.RichText)
        bilibiliLabel.setOpenExternalLinks(True)
        linksLayout.addWidget(bilibiliLabel)

        self.setFixedSize(self.sizeHint())
        self.start_version_fetch()

    def start_version_fetch(self):
        self.worker = VersionFetchWorker('GCM')
        self.worker.versionFetched.connect(self.update_version_labels)
        self.worker.fetchFailed.connect(self.handle_version_load_failure)
        self.worker.start()

    def update_version_labels(self, latest_version):
        current_version = self.parent().appVersion

        if latest_version > current_version:
            self.currentVersionNumberLabel.setStyleSheet("color: red;")
            self.newestVersionNumberLabel.setStyleSheet("color: green;")
            self.updateButton.setVisible(1)

        self.newestVersionNumberLabel.setText(latest_version)
        self.worker.quit()

    def handle_version_load_failure(self):
        self.newestVersionNumberLabel.setText(tr("Failed to load"))
        self.newestVersionNumberLabel.setStyleSheet("color: red;")
        self.worker.quit()


class TrainerUploadDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.worker = None
        self.setWindowTitle(tr("Upload Trainer"))
        self.setWindowIcon(QIcon(resource_path("assets/logo.ico")))
        self.setMinimumWidth(500)

        layout = QVBoxLayout()
        layout.setSpacing(20)
        layout.setContentsMargins(30, 30, 30, 30)
        self.setLayout(layout)

        # Contact Info
        contactLayout = QVBoxLayout()
        contactLayout.setSpacing(5)
        contactLayout.addWidget(QLabel(tr("Contact Info (Optional):")))
        self.contactEdit = QLineEdit()
        self.contactEdit.setPlaceholderText(tr("Email"))
        contactLayout.addWidget(self.contactEdit)

        contact_info_label = QLabel(tr("If you provide your Game-Zone Labs account email and your trainer is approved, a badge will be added to your account"))
        contact_info_font = contact_info_label.font()
        contact_info_font.setPointSize(9)
        contact_info_label.setFont(contact_info_font)
        contact_info_label.setStyleSheet("color: gray;")
        contact_info_label.setWordWrap(True)
        contactLayout.addWidget(contact_info_label)
        layout.addLayout(contactLayout)

        # Trainer Name
        nameLayout = QVBoxLayout()
        nameLayout.setSpacing(5)
        nameLayout.addWidget(QLabel(tr("Trainer Name:")))
        self.nameEdit = QLineEdit()
        self.nameEdit.setPlaceholderText(tr("What game does it work for"))
        nameLayout.addWidget(self.nameEdit)
        layout.addLayout(nameLayout)

        # Trainer Source
        sourceLayout = QVBoxLayout()
        sourceLayout.setSpacing(5)
        sourceLayout.addWidget(QLabel(tr("Trainer Source (Optional):")))
        self.sourceEdit = QLineEdit()
        self.sourceEdit.setPlaceholderText(tr("Original URL or author"))
        sourceLayout.addWidget(self.sourceEdit)

        source_info_label = QLabel(tr("If you made the trainer, please provide your author name"))
        source_info_font = source_info_label.font()
        source_info_font.setPointSize(9)
        source_info_label.setFont(source_info_font)
        source_info_label.setStyleSheet("color: gray;")
        source_info_label.setWordWrap(True)
        sourceLayout.addWidget(source_info_label)
        layout.addLayout(sourceLayout)

        # Trainer File Selection
        fileLabelLayout = QVBoxLayout()
        fileLabelLayout.setSpacing(5)
        fileLabelLayout.addWidget(QLabel(tr("Trainer File:")))

        fileSelectLayout = QHBoxLayout()
        self.fileEdit = QLineEdit()
        self.fileEdit.setReadOnly(True)
        self.fileEdit.setPlaceholderText(tr("Select a trainer file") + "...")

        self.browseButton = CustomButton("...")
        self.browseButton.clicked.connect(self.browse_file)

        fileSelectLayout.addWidget(self.fileEdit)
        fileSelectLayout.addWidget(self.browseButton)
        fileLabelLayout.addLayout(fileSelectLayout)

        info_label = QLabel(tr("If the trainer consists of multiple files, please compress them into a single archive (zip/rar/7z) before uploading"))
        info_font = info_label.font()
        info_font.setPointSize(9)
        info_label.setFont(info_font)
        info_label.setStyleSheet("color: gray;")
        info_label.setWordWrap(True)
        fileLabelLayout.addWidget(info_label)
        layout.addLayout(fileLabelLayout)

        # Notes
        notesLayout = QVBoxLayout()
        notesLayout.setSpacing(5)
        notesLayout.addWidget(QLabel(tr("Additional Notes:")))
        self.notesEdit = QTextEdit()
        self.notesEdit.setPlaceholderText(tr("Anything else to add..."))
        self.notesEdit.setMaximumHeight(100)
        self.notesEdit.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        notesLayout.addWidget(self.notesEdit)

        notes_info_label = QLabel(
            tr("Please make sure the trainer you are about to upload is tested to be safe and functional.") + "\n" +
            tr("For more info about trainer uploading, please join the QQ group to discuss: 186859946.")
        )
        notes_info_font = notes_info_label.font()
        notes_info_font.setPointSize(9)
        notes_info_label.setFont(notes_info_font)
        notes_info_label.setStyleSheet("color: gray;")
        notes_info_label.setWordWrap(True)
        notesLayout.addWidget(notes_info_label)
        layout.addLayout(notesLayout)

        # Progress Bar
        self.progressBar = QProgressBar()
        self.progressBar.setRange(0, 100)
        self.progressBar.setValue(0)
        self.progressBar.setVisible(False)
        layout.addWidget(self.progressBar)

        layout.addStretch()

        # Buttons
        buttonLayout = QHBoxLayout()
        buttonLayout.addStretch()

        self.uploadButton = CustomButton(tr("Upload"))
        self.uploadButton.setFixedWidth(100)
        self.uploadButton.clicked.connect(self.start_upload)

        buttonLayout.addWidget(self.uploadButton)
        layout.addLayout(buttonLayout)

    def browse_file(self):
        default_dir = settings.get("downloadPath", "")
        file_path, _ = QFileDialog.getOpenFileName(
            self,
            tr("Select a trainer file"),
            default_dir,
            tr("All Files (*)")
        )
        if file_path:
            self.fileEdit.setText(file_path)
            if not self.nameEdit.text():
                self.nameEdit.setText(os.path.basename(file_path))

    def start_upload(self):
        file_path = self.fileEdit.text()
        name = self.nameEdit.text()

        if not file_path or not os.path.exists(file_path):
            QMessageBox.warning(self, tr("Error"), tr("Please select a valid file."))
            return

        if not name:
            QMessageBox.warning(self, tr("Error"), tr("Please provide a trainer name."))
            return

        # Prepare UI for upload
        self.set_ui_locked(True)
        self.progressBar.setValue(0)
        self.progressBar.setVisible(True)

        self.worker = TrainerUploadWorker(
            file_path,
            name,
            self.contactEdit.text(),
            self.sourceEdit.text(),
            self.notesEdit.toPlainText()
        )
        self.worker.progress.connect(self.update_progress)
        self.worker.finished.connect(self.handle_upload_finished)
        self.worker.start()

    def update_progress(self, percent):
        self.progressBar.setValue(percent)

    def handle_upload_finished(self, success, message):
        self.progressBar.setVisible(False)
        self.set_ui_locked(False)

        if success:
            QMessageBox.information(self, tr("Success"), message)
            self.accept()
        else:
            QMessageBox.critical(self, tr("Error"), message)

    def closeEvent(self, event):
        if self.worker and self.worker.isRunning():
            self.worker.stop()
            self.worker.wait()
        super().closeEvent(event)

    def set_ui_locked(self, locked):
        self.contactEdit.setDisabled(locked)
        self.nameEdit.setDisabled(locked)
        self.sourceEdit.setDisabled(locked)
        self.notesEdit.setDisabled(locked)
        self.browseButton.setDisabled(locked)
        self.uploadButton.setDisabled(locked)
