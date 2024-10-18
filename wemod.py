import os
import psutil
import re
import shutil
import subprocess

from PyQt6.QtCore import QThread, pyqtSignal
from PyQt6.QtGui import QColor, QIcon
from PyQt6.QtWidgets import QCheckBox, QComboBox, QDialog, QFileDialog, QHBoxLayout, QLabel, QLineEdit, QListWidget, QListWidgetItem, QVBoxLayout

from config import *
from helper import CustomButton


class WeModDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle(tr("WeMod Customization"))
        self.setWindowIcon(QIcon(resource_path("assets/logo.ico")))
        weModLayout = QVBoxLayout()
        weModLayout.setSpacing(15)
        self.setLayout(weModLayout)
        self.setMinimumWidth(900)

        # Latest WeMod download: https://api.wemod.com/client/download
        # Custom WeMod version download: https://storage-cdn.wemod.com/app/releases/stable/WeMod-9.10.3.exe
        weModDownloadLink = QLabel(tr("Download WeMod: ") + '<a href="https://www.wemod.com" style="text-decoration: none;">https://www.wemod.com</a>')
        weModDownloadLink.setOpenExternalLinks(True)
        weModLayout.addWidget(weModDownloadLink)

        mainLayout = QHBoxLayout()
        mainLayout.setSpacing(30)
        mainLayout.setContentsMargins(30, 20, 30, 20)
        weModLayout.addLayout(mainLayout)

        column1Layout = QVBoxLayout()
        mainLayout.addLayout(column1Layout)

        column2Layout = QVBoxLayout()
        column2Layout.setSpacing(15)
        column2Layout.addStretch(1)
        mainLayout.addLayout(column2Layout)

        column3Layout = QVBoxLayout()
        column3Layout.setSpacing(15)
        mainLayout.addLayout(column3Layout)

        self.weModVersions = []

        # Prompt
        self.weModPrompt = QListWidget()
        column1Layout.addWidget(self.weModPrompt)

        # WeMod installation path
        installLayout = QVBoxLayout()
        installLayout.setSpacing(2)
        column2Layout.addLayout(installLayout)
        installLayout.addWidget(QLabel(tr("WeMod installation path:")))
        installPathLayout = QHBoxLayout()
        installPathLayout.setSpacing(5)
        installLayout.addLayout(installPathLayout)
        self.installLineEdit = QLineEdit()
        self.installLineEdit.setReadOnly(True)
        installPathLayout.addWidget(self.installLineEdit)
        installPathButton = CustomButton("...")
        installPathButton.clicked.connect(self.selectWeModPath)
        installPathLayout.addWidget(installPathButton)

        # Version selection
        versionLayout = QVBoxLayout()
        versionLayout.setSpacing(2)
        column2Layout.addStretch(1)
        column2Layout.addLayout(versionLayout)
        versionLayout.addWidget(QLabel(tr("Installed WeMod Versions:")))
        self.versionCombo = QComboBox()
        versionLayout.addWidget(self.versionCombo)
        column2Layout.addStretch(1)

        # Unlock WeMod pro
        self.weModProCheckbox = QCheckBox(tr("Activate WeMod Pro"))
        column3Layout.addWidget(self.weModProCheckbox)

        # Disable auto update
        self.disableUpdateCheckbox = QCheckBox(tr("Disable WeMod Auto Update"))
        column3Layout.addWidget(self.disableUpdateCheckbox)

        # Delete all other WeMod versions
        self.delOtherVersionsCheckbox = QCheckBox(tr("Delete All Other WeMod Versions"))
        column3Layout.addWidget(self.delOtherVersionsCheckbox)

        # Apply button
        applyButtonLayout = QHBoxLayout()
        applyButtonLayout.setContentsMargins(0, 0, 10, 10)
        applyButtonLayout.addStretch(1)
        weModLayout.addLayout(applyButtonLayout)
        self.applyButton = CustomButton(tr("Apply"))
        self.applyButton.setFixedWidth(100)
        self.applyButton.clicked.connect(self.applyWeModCustomization)
        self.applyButton.setDisabled(True)
        applyButtonLayout.addWidget(self.applyButton)

        self.installLineEdit.setText(settings["WeModPath"])
        self.findWeModVersions(settings["WeModPath"])
    
    def selectWeModPath(self):
        initialPath = self.installLineEdit.text() or os.path.expanduser("~")
        directory = QFileDialog.getExistingDirectory(self, tr("Select WeMod installation path"), initialPath)
        if directory:
            settings["WeModPath"] = os.path.normpath(directory)
            self.installLineEdit.setText(settings["WeModPath"])
            apply_settings(settings)
            self.findWeModVersions(settings["WeModPath"])
    
    def findWeModVersions(self, weModPath):
        self.weModVersions = []
        if not os.path.exists(weModPath):
            self.versionCombo.clear()
            self.versionCombo.addItem(tr("WeMod not installed"))
            self.applyButton.setDisabled(True)
            return

        for item in os.listdir(weModPath):
            if os.path.isdir(os.path.join(weModPath, item)):
                match = re.match(r'app-(\d+\.\d+\.\d+)', item)
                if match:
                    version_info = match.group(1)  # for instance: 9.3.0
                    self.weModVersions.append(version_info)

        if not self.weModVersions:
            self.versionCombo.clear()
            self.versionCombo.addItem(tr("WeMod not installed"))
            self.applyButton.setDisabled(True)
            return
        
        self.weModVersions.sort(key=lambda v: tuple(map(int, v.split('.'))), reverse=True)
        self.versionCombo.clear()
        self.versionCombo.addItems(self.weModVersions)
        self.applyButton.setEnabled(True)
    
    def on_message(self, message, type=None):
        item = QListWidgetItem(message)

        if type == "clear":
            self.weModPrompt.clear()
        elif type == "success":
            # item.setForeground(QColor('green'))
            item.setBackground(QColor(0, 255, 0, 20))
            self.weModPrompt.addItem(item)
        elif type == "failure":
            # item.setForeground(QColor('red'))
            item.setBackground(QColor(255, 0, 0, 20))
            self.weModPrompt.addItem(item)
        else:
            self.weModPrompt.addItem(item)
    
    def on_finished(self):
        self.applyButton.setEnabled(True)
        self.findWeModVersions(settings["WeModPath"])
    
    def applyWeModCustomization(self):
        weModInstallPath = self.installLineEdit.text()
        selectedWeModVersion = self.versionCombo.currentText()
        self.weModPrompt.clear()
        self.applyButton.setDisabled(True)

        self.apply_thread = ApplyCustomization(self.weModVersions, weModInstallPath, selectedWeModVersion, self)
        self.apply_thread.message.connect(self.on_message)
        self.apply_thread.finished.connect(self.on_finished)
        self.apply_thread.start()


class ApplyCustomization(QThread):
    message = pyqtSignal(str, str)
    finished = pyqtSignal()

    def __init__(self, weModVersions, weModInstallPath, selectedWeModVersion, parent=None):
        super().__init__(parent)
        self.weModVersions = weModVersions
        self.weModInstallPath = weModInstallPath
        self.selectedWeModVersion = selectedWeModVersion
        self.selectedWeModPath = os.path.join(weModInstallPath, f"app-{selectedWeModVersion}")
    
    def run(self):
        asar = os.path.join(self.selectedWeModPath, "resources", "app.asar")
        asar_copy = os.path.join(WEMOD_TEMP_DIR, "app.asar")
        asar_bak = os.path.join(self.selectedWeModPath, "resources", "app.asar.bak")
        weModExe = os.path.join(self.selectedWeModPath, "WeMod.exe")
        weModExe_bak = os.path.join(self.selectedWeModPath, "WeMod.exe.bak")

        # Terminate if WeMod is running
        if self.is_program_running("WeMod.exe"):
            self.message.emit(tr("WeMod is currently running,\nplease close the application first."), "failure")
            self.finished.emit()
            return

        # ===========================================================================
        # Unlock WeMod Pro
        if self.parent().weModProCheckbox.isChecked():
            patch_success = True

            # 1. Remove asar integrity check
            shutil.copyfile(weModExe, weModExe_bak)
            self.replace_hex_in_file(weModExe_bak, weModExe, '00001101', '00000101')
            os.remove(weModExe_bak)

            # 2. Patch app.asar
            os.makedirs(WEMOD_TEMP_DIR, exist_ok=True)
            if os.path.exists(asar_bak):
                if os.path.exists(asar):
                    os.remove(asar)
                os.rename(asar_bak, asar)
            shutil.copyfile(asar, asar_copy)

            # Extract app.asar file
            try:
                command = [unzip_path, 'e', '-y', asar_copy, "app*bundle.js", "index.js", f"-o{WEMOD_TEMP_DIR}"]
                subprocess.run(command, check=True, creationflags=subprocess.CREATE_NO_WINDOW)
            except Exception as e:
                self.message.emit(tr("Failed to extract file:") + f"\n{asar_copy}", "failure")
                patch_success = False
        
            patterns = {
                r'(getUserAccount\()(.*)(}async getUserAccountFlags)': r'\1\2.then(function(response) {response.subscription={period:"yearly",state:"active"}; response.flags=78; return response;})\3',
                r'(getUserAccountFlags\()(.*)(\)\).flags)': r'\1\2\3.then(function(response) {if (response.mask==4) {response.flags=4}; return response;})',
                r'(changeAccountEmail\()(.*)(email:.?,currentPassword:.?}\))': r'\1\2\3.then(function(response) {response.subscription={period:"yearly", state:"active"}; response.flags=78; return response;})',
                r'(getPromotion\()(.*)(collectMetrics:!0}\))': r'\1\2\3.then(function(response) {response.components.appBanner=null; response.flags=0; return response;})'
            }

            # Mapping of patterns to files where they were found: {pattern key: file path}
            lines = {key: None for key in patterns}

            # Check files for matching patterns
            for pattern in patterns:
                for filename in os.listdir(WEMOD_TEMP_DIR):
                    if filename.endswith('.js'):
                        file_path = os.path.join(WEMOD_TEMP_DIR, filename)
                        try:
                            with open(file_path, 'r', encoding='utf-8') as file:
                                content = file.read()
                                if re.search(pattern, content):
                                    lines[pattern] = file_path
                                    break
                        except UnicodeDecodeError:
                            continue

            # Process each file with matched patterns
            if all(lines.values()):
                print(f"js file patched: {list(lines.items())[0][1]}")
                for pattern, file_path in lines.items():
                    self.apply_patch(file_path, pattern, patterns[pattern])
            else:
                self.message.emit(tr("Unsupported WeMod version."), "failure")
                patch_success = False

            # pack patched js files back to app.asar
            try:
                shutil.copyfile(asar, asar_bak)
                command = [unzip_path, 'a', '-y', asar_copy, os.path.join(WEMOD_TEMP_DIR, '*.js')]
                subprocess.run(command, check=True, creationflags=subprocess.CREATE_NO_WINDOW)
                shutil.move(asar_copy, asar)
            except Exception as e:
                self.message.emit(tr("Failed to patch file:") + f"\n{asar}", "failure")
                patch_success = False

            # Clean up
            shutil.rmtree(WEMOD_TEMP_DIR)
            if patch_success:
                self.message.emit(tr("WeMod Pro activated."), "success")
            else:
                self.message.emit(tr("Failed to activate WeMod Pro."), "failure")

        else:
            if os.path.exists(asar_bak):
                if os.path.exists(asar):
                    os.remove(asar)
                os.rename(asar_bak, asar)

            self.message.emit(tr("WeMod Pro disabled."), "success")

        # ===========================================================================
        # Disable auto update
        updateExe = os.path.join(self.weModInstallPath, "Update.exe")
        updateExe_backup = os.path.join(self.weModInstallPath, "Update.exe.bak")
        try:
            if self.parent().disableUpdateCheckbox.isChecked():
                if os.path.exists(updateExe):
                    os.rename(updateExe, updateExe_backup)
                    self.message.emit(tr("WeMod auto update disabled."), "success")
            else:
                if os.path.exists(updateExe_backup):
                    os.rename(updateExe_backup, updateExe)
                    self.message.emit(tr("WeMod auto update enabled."), "success")
                elif not os.path.exists(updateExe):
                    self.message.emit(tr("Failed to enable WeMod auto update,\nplease try reinstalling WeMod."), "failure")
        except Exception as e:
            self.message.emit(tr("Failed to process WeMod update file:") + f"\n{str(e)}", "failure")

        # ===========================================================================
        # Delete other version folders
        if self.parent().delOtherVersionsCheckbox.isChecked():
            for version in self.weModVersions:
                if version != self.selectedWeModVersion:
                    folder_path = os.path.join(self.weModInstallPath, f"app-{version}")
                    try:
                        shutil.rmtree(folder_path)
                        self.message.emit(tr("Deleted WeMod version: ") + version, "success")
                    except Exception as e:
                        self.message.emit(tr("Failed to delete WeMod version: ") + version, "failure")
        
        self.finished.emit()
    
    def is_program_running(self, program_name):
        for proc in psutil.process_iter():
            try:
                if program_name == proc.name():
                    return True
            except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
                pass
        return False
    
    def apply_patch(self, file_path, pattern, replacement):
        try:
            with open(file_path, 'r+', encoding='utf-8') as file:
                content = file.read()
                modified_content = re.sub(pattern, replacement, content)
                file.seek(0)
                file.write(modified_content)
                file.truncate()
        except Exception as e:
            self.message.emit(tr("Failed to patch file:") + f"\n{file_path}", "failure")

    def replace_hex_in_file(self, input_file, output_file, search_hex, replace_hex):
        try:
            command = [binmay_path, '-i', input_file, '-o', output_file, '-s', f"t:{search_hex}", '-r', f"t:{replace_hex}"]
            subprocess.run(command, check=True, creationflags=subprocess.CREATE_NO_WINDOW)
        except Exception as e:
            self.message.emit(tr("Failed to patch file:") + f"\n{input_file}", "failure")
