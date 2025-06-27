import os
import sys
import winreg as reg

from PyQt6.QtCore import Qt
from PyQt6.QtGui import QIcon, QPixmap
from PyQt6.QtWidgets import QCheckBox, QComboBox, QDialog, QHBoxLayout, QLabel, QMessageBox, QSizePolicy, QVBoxLayout

from config import *
from widgets.custom_widgets import CustomButton
from threads.other_threads import VersionFetchWorker


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

        githubUrl = self.parent().githubLink
        githubText = f'GitHub: <a href="{githubUrl}" style="text-decoration: none;">{githubUrl}</a>'
        githubLabel = QLabel(githubText)
        githubLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        githubLabel.setTextFormat(Qt.TextFormat.RichText)
        githubLabel.setOpenExternalLinks(True)
        linksLayout.addWidget(githubLabel)

        bilibiliUrl = self.parent().bilibiliLink
        text = tr("Bilibili author homepage:")
        bilibiliText = f'{text} <a href="{bilibiliUrl}" style="text-decoration: none;">{bilibiliUrl}</a>'
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

        # Always show english
        self.alwaysEnCheckbox = QCheckBox(tr("Always show search results in English"))
        self.alwaysEnCheckbox.setChecked(settings["enSearchResults"])
        settingsWidgetsLayout.addWidget(self.alwaysEnCheckbox)

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

        settings["theme"] = theme_options[self.themeCombo.currentText()]
        settings["language"] = language_options[self.languageCombo.currentText()]
        settings["enSearchResults"] = self.alwaysEnCheckbox.isChecked()
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

        githubUrl = self.parent().githubLink
        githubText = f'GitHub: <a href="{githubUrl}" style="text-decoration: none;">{githubUrl}</a>'
        githubLabel = QLabel(githubText)
        githubLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        githubLabel.setTextFormat(Qt.TextFormat.RichText)
        githubLabel.setOpenExternalLinks(True)
        linksLayout.addWidget(githubLabel)

        bilibiliUrl = self.parent().bilibiliLink
        text = tr("Bilibili author homepage:")
        bilibiliText = f'{text} <a href="{bilibiliUrl}" style="text-decoration: none;">{bilibiliUrl}</a>'
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
