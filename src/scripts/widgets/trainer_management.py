import subprocess
import re

from PyQt6.QtWidgets import QDialog, QVBoxLayout, QTabWidget, QWidget, QHBoxLayout, QLabel, QComboBox, QCheckBox, QLineEdit, QFileDialog, QMessageBox
from PyQt6.QtGui import QIcon, QPixmap
from PyQt6.QtCore import Qt, QTimer

from config import *
from widgets.custom_widgets import AlertWidget, CustomButton
from threads.other_threads import WeModCustomization
import time

class TrainerManagementDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle(tr("Trainer Management"))
        self.setWindowIcon(QIcon(resource_path("assets/logo.ico")))
        self.setMinimumWidth(550)
        self.active_alerts = []

        # Main layout
        mainLayout = QVBoxLayout()
        mainLayout.setSpacing(15)
        self.setLayout(mainLayout)

        # Tab widget
        self.tabWidget = QTabWidget(self)
        self.tabWidget.setTabPosition(QTabWidget.TabPosition.North)
        mainLayout.addWidget(self.tabWidget)

        # Add tabs
        self.tabWidget.addTab(self.createFlingTab(), tr("FLiNG"))
        self.tabWidget.addTab(self.createXiaoxingTab(), tr("XiaoXing"))
        self.tabWidget.addTab(self.createWemodTab(), "WeMod")
        self.tabWidget.addTab(self.createCETab(), "Cheat Engine")
    
    def closeEvent(self, event):
        super().closeEvent(event)

        while self.active_alerts:
            self.active_alerts[0].close()

        settings["downloadServer"] = server_options[self.serverCombo.currentText()]
        settings["removeBgMusic"] = self.removeBgMusicCheckbox.isChecked()
        settings["autoUpdateDatabase"] = self.autoUpdateDatabaseCheckbox.isChecked()
        settings["autoUpdateFling"] = self.autoUpdateCheckbox.isChecked()
        settings["enableXiaoxing"] = self.enableXiaoxingCheckbox.isChecked()
        apply_settings(settings)

    @staticmethod
    def find_settings_key(value, dict):
        return next(key for key, val in dict.items() if val == value)
    
    def show_alert(self, message, alert_type):
        alert = AlertWidget(self, message, alert_type)
        alert.show()
        QTimer.singleShot(5000, alert.close)
   
    def moveEvent(self, event):
        super().moveEvent(event)
        for alert in self.active_alerts:
            alert.move_to_top_right()

    def createFlingTab(self):
        flingTab = QWidget()
        layout = QVBoxLayout()
        layout.setContentsMargins(0, 15, 0, 0)
        flingTab.setLayout(layout)
        officialLink = QLabel(tr("Official Website: ") + '<a href="https://flingtrainer.com" style="text-decoration: none;">https://flingtrainer.com</a>')
        officialLink.setOpenExternalLinks(True)
        layout.addWidget(officialLink)
        layout.addStretch(1)
        
        columns = QHBoxLayout()
        columns.setContentsMargins(30, 20, 30, 20)
        columns.setSpacing(40)
        layout.addLayout(columns)

        column1 = QVBoxLayout()
        columns.addLayout(column1)

        logoPixmap = QPixmap(resource_path("assets/fling.png"))
        scaledLogoPixmap = logoPixmap.scaled(130, 130, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation)
        logoLabel = QLabel()
        logoLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        logoLabel.setPixmap(scaledLogoPixmap)
        column1.addWidget(logoLabel)

        column2 = QVBoxLayout()
        column2.setSpacing(15)
        columns.addLayout(column2)

        # Server selection
        serverLayout = QVBoxLayout()
        serverLayout.setSpacing(2)
        column2.addLayout(serverLayout)
        serverLabel = QLabel(tr("Download Server:"))
        serverLayout.addWidget(serverLabel)
        self.serverCombo = QComboBox()
        self.serverCombo.addItems(server_options.keys())
        self.serverCombo.setCurrentText(self.find_settings_key(settings["downloadServer"], server_options))
        serverLayout.addWidget(self.serverCombo)

        # Remove trainer background music when downloading
        self.removeBgMusicCheckbox = QCheckBox(tr("Remove trainer background music"))
        self.removeBgMusicCheckbox.setChecked(settings["removeBgMusic"])
        column2.addWidget(self.removeBgMusicCheckbox)

        # Auto update trainer databases
        self.autoUpdateDatabaseCheckbox = QCheckBox(tr("Update trainer databases automatically"))
        self.autoUpdateDatabaseCheckbox.setChecked(settings["autoUpdateDatabase"])
        column2.addWidget(self.autoUpdateDatabaseCheckbox)

        # Auto trainer updates
        self.autoUpdateCheckbox = QCheckBox(tr("Update trainers automatically"))
        self.autoUpdateCheckbox.setChecked(settings["autoUpdate"])
        column2.addWidget(self.autoUpdateCheckbox)

        layout.addStretch(1)
        return flingTab

    def createXiaoxingTab(self):
        xiaoxingTab = QWidget()
        layout = QVBoxLayout()
        layout.setContentsMargins(0, 15, 0, 0)
        xiaoxingTab.setLayout(layout)
        officialLink = QLabel(tr("Official Website: ") + '<a href="https://www.xiaoxingjie.com" style="text-decoration: none;">https://www.xiaoxingjie.com</a>')
        officialLink.setOpenExternalLinks(True)
        layout.addWidget(officialLink)
        layout.addStretch(1)
        
        columns = QHBoxLayout()
        columns.setContentsMargins(30, 20, 30, 20)
        columns.setSpacing(40)
        layout.addLayout(columns)

        column1 = QVBoxLayout()
        columns.addLayout(column1)

        logoPixmap = QPixmap(resource_path("assets/xiaoxing.png"))
        scaledLogoPixmap = logoPixmap.scaled(130, 130, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation)
        logoLabel = QLabel()
        logoLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        logoLabel.setPixmap(scaledLogoPixmap)
        column1.addWidget(logoLabel)

        column2 = QVBoxLayout()
        column2.setSpacing(15)
        columns.addLayout(column2)

        # Enable Xiaoxing trainer
        self.enableXiaoxingCheckbox = QCheckBox(tr("Enable search for Xiao Xing trainers"))
        self.enableXiaoxingCheckbox.setChecked(settings["enableXiaoxing"])
        column2.addWidget(self.enableXiaoxingCheckbox)

        layout.addStretch(1)
        return xiaoxingTab

    def createWemodTab(self):
        wemodTab = QWidget()
        layout = QVBoxLayout()
        layout.setContentsMargins(0, 15, 0, 0)
        wemodTab.setLayout(layout)
        officialLink = QLabel(tr("Official Website: ") + '<a href="https://www.wemod.com" style="text-decoration: none;">https://www.wemod.com</a>')
        officialLink.setOpenExternalLinks(True)
        layout.addWidget(officialLink)
        layout.addStretch(1)
        
        columns = QHBoxLayout()
        columns.setContentsMargins(30, 20, 30, 20)
        columns.setSpacing(40)
        layout.addLayout(columns)

        column1 = QVBoxLayout()
        columns.addLayout(column1)

        logoPixmap = QPixmap(resource_path("assets/wemod.png"))
        scaledLogoPixmap = logoPixmap.scaled(130, 130, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation)
        logoLabel = QLabel()
        logoLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        logoLabel.setPixmap(scaledLogoPixmap)
        column1.addWidget(logoLabel)

        column2 = QVBoxLayout()
        column2.setSpacing(15)
        columns.addLayout(column2)

        # WeMod installation path
        installLayout = QVBoxLayout()
        installLayout.setSpacing(2)
        column2.addLayout(installLayout)

        installLabelLayout = QHBoxLayout()
        installLabelLayout.setSpacing(3)
        installLayout.addLayout(installLabelLayout)
        installLabelLayout.addWidget(QLabel(tr("WeMod installation path:")))
        self.weModResetButton = CustomButton(tr('Reset to default'))
        self.weModResetButton.setStyleSheet("padding: 3;")
        font = self.weModResetButton.font()
        font.setPointSize(8)
        self.weModResetButton.setFont(font)
        self.weModResetButton.clicked.connect(self.resetWemodPath)
        installLabelLayout.addWidget(self.weModResetButton)
        installLabelLayout.addStretch(1)

        installPathLayout = QHBoxLayout()
        installPathLayout.setSpacing(5)
        installLayout.addLayout(installPathLayout)
        self.weModInstallLineEdit = QLineEdit()
        self.weModInstallLineEdit.setReadOnly(True)
        installPathLayout.addWidget(self.weModInstallLineEdit)
        installPathButton = CustomButton("...")
        installPathButton.clicked.connect(self.selectWeModPath)
        installPathLayout.addWidget(installPathButton)

        # Version selection
        versionLayout = QVBoxLayout()
        versionLayout.setSpacing(2)
        column2.addLayout(versionLayout)
        versionLayout.addWidget(QLabel(tr("Installed WeMod Versions:")))
        self.versionCombo = QComboBox()
        versionLayout.addWidget(self.versionCombo)

        # Unlock WeMod pro
        self.weModProCheckbox = QCheckBox(tr("Activate WeMod Pro"))
        column2.addWidget(self.weModProCheckbox)

        # Disable auto update
        self.disableUpdateCheckbox = QCheckBox(tr("Disable WeMod Auto Update"))
        column2.addWidget(self.disableUpdateCheckbox)

        # Delete all other WeMod versions
        self.delOtherVersionsCheckbox = QCheckBox(tr("Delete All Other WeMod Versions"))
        column2.addWidget(self.delOtherVersionsCheckbox)

        # Apply button
        applyButtonLayout = QHBoxLayout()
        applyButtonLayout.setContentsMargins(0, 0, 10, 10)
        applyButtonLayout.addStretch(1)
        layout.addLayout(applyButtonLayout)
        self.weModApplyButton = CustomButton(tr("Apply"))
        self.weModApplyButton.setFixedWidth(100)
        self.weModApplyButton.clicked.connect(self.applyWeModCustomization)
        applyButtonLayout.addWidget(self.weModApplyButton)

        self.weModInstallLineEdit.setText(settings["weModPath"])
        self.findWeModVersions(settings["weModPath"])

        layout.addStretch(1)
        return wemodTab
    
    def createCETab(self):
        ceTab = QWidget()
        layout = QVBoxLayout()
        layout.setContentsMargins(0, 15, 0, 0)
        ceTab.setLayout(layout)
        officialLink = QLabel(tr("Official Website: ") + '<a href="https://www.cheatengine.org" style="text-decoration: none;">https://www.cheatengine.org</a>')
        officialLink.setOpenExternalLinks(True)
        layout.addWidget(officialLink)
        layout.addStretch(1)
        
        columns = QHBoxLayout()
        columns.setContentsMargins(30, 20, 30, 20)
        columns.setSpacing(40)
        layout.addLayout(columns)

        column1 = QVBoxLayout()
        columns.addLayout(column1)

        logoPixmap = QPixmap(resource_path("assets/ce.png"))
        scaledLogoPixmap = logoPixmap.scaled(130, 130, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation)
        logoLabel = QLabel()
        logoLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        logoLabel.setPixmap(scaledLogoPixmap)
        column1.addWidget(logoLabel)

        column2 = QVBoxLayout()
        column2.setSpacing(15)
        columns.addLayout(column2)

        # CE installation status
        self.installStatus = QLabel()
        column2.addWidget(self.installStatus)

        # CE installation path
        installLayout = QVBoxLayout()
        installLayout.setSpacing(2)
        column2.addLayout(installLayout)

        installLabelLayout = QHBoxLayout()
        installLabelLayout.setSpacing(3)
        installLayout.addLayout(installLabelLayout)
        installLabelLayout.addWidget(QLabel(tr("Cheat Engine installation path:")))
        self.ceResetButton = CustomButton(tr('Reset to default'))
        self.ceResetButton.setStyleSheet("padding: 3;")
        font = self.ceResetButton.font()
        font.setPointSize(8)
        self.ceResetButton.setFont(font)
        self.ceResetButton.clicked.connect(self.resetCEPath)
        installLabelLayout.addWidget(self.ceResetButton)
        installLabelLayout.addStretch(1)

        installPathLayout = QHBoxLayout()
        installPathLayout.setSpacing(5)
        installLayout.addLayout(installPathLayout)
        self.ceInstallLineEdit = QLineEdit()
        self.ceInstallLineEdit.setReadOnly(True)
        installPathLayout.addWidget(self.ceInstallLineEdit)
        installPathButton = CustomButton("...")
        installPathButton.clicked.connect(self.selectCEPath)
        installPathLayout.addWidget(installPathButton)

        # Add simplified chinese
        self.addzhCNCheckbox = QCheckBox(tr("Add Simplified Chinese"))
        column2.addWidget(self.addzhCNCheckbox)

        # Apply button
        applyButtonLayout = QHBoxLayout()
        applyButtonLayout.setContentsMargins(0, 0, 10, 10)
        applyButtonLayout.addStretch(1)
        layout.addLayout(applyButtonLayout)
        self.ceApplyButton = CustomButton(tr("Apply"))
        self.ceApplyButton.setFixedWidth(100)
        self.ceApplyButton.clicked.connect(self.applyCheatEngineCustomization)
        applyButtonLayout.addWidget(self.ceApplyButton)

        self.ceInstallLineEdit.setText(settings["cePath"])
        self.checkCEInstallStatus()

        layout.addStretch(1)
        return ceTab

    # ===========================================================================
    # Tab functions
    # ===========================================================================
    def selectWeModPath(self):
        initialPath = self.weModInstallLineEdit.text() or os.path.expanduser("~")
        directory = QFileDialog.getExistingDirectory(self, tr("Select WeMod installation path"), initialPath)
        if directory:
            settings["weModPath"] = os.path.normpath(directory)
            self.weModInstallLineEdit.setText(settings["weModPath"])
            apply_settings(settings)
            self.findWeModVersions(settings["weModPath"])
    
    def resetWemodPath(self):
        self.weModInstallLineEdit.setText(wemod_install_path)
        settings["weModPath"] = os.path.normpath(wemod_install_path)
        apply_settings(settings)
        self.findWeModVersions(settings["weModPath"])
    
    def selectCEPath(self):
        initialPath = self.ceInstallLineEdit.text() or os.path.expanduser("~")
        directory = QFileDialog.getExistingDirectory(self, tr("Select Cheat Engine installation path"), initialPath)
        if directory:
            settings["cePath"] = os.path.normpath(directory)
            self.ceInstallLineEdit.setText(settings["cePath"])
            apply_settings(settings)
            self.checkCEInstallStatus()
    
    def resetCEPath(self):
        self.ceInstallLineEdit.setText(ce_install_path)
        settings["cePath"] = os.path.normpath(ce_install_path)
        apply_settings(settings)
        self.checkCEInstallStatus()
    
    def checkCEInstallStatus(self):
        cePath = os.path.join(self.ceInstallLineEdit.text(), 'Cheat Engine.exe')
        if os.path.exists(cePath):
            self.installStatus.setText(tr("Cheat Engine is installed"))
            self.installStatus.setStyleSheet("color: green;")
            self.ceApplyButton.setEnabled(True)
        else:
            self.installStatus.setText(tr("Cheat Engine not installed"))
            self.installStatus.setStyleSheet("color: red;")
            self.ceApplyButton.setDisabled(True)
    
    def findWeModVersions(self, weModPath):
        self.weModVersions = []
        if not os.path.exists(weModPath):
            self.versionCombo.clear()
            self.versionCombo.addItem(tr("WeMod not installed"))
            self.weModApplyButton.setDisabled(True)
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
            self.weModApplyButton.setDisabled(True)
            return
        
        self.weModVersions.sort(key=lambda v: tuple(map(int, v.split('.'))), reverse=True)
        self.versionCombo.clear()
        self.versionCombo.addItems(self.weModVersions)
        self.weModApplyButton.setEnabled(True)
    
    def on_finished(self):
        self.weModApplyButton.setEnabled(True)
        self.weModResetButton.setEnabled(True)
        self.findWeModVersions(settings["weModPath"])
    
    def applyWeModCustomization(self):
        weModInstallPath = self.weModInstallLineEdit.text()
        selectedWeModVersion = self.versionCombo.currentText()
        self.weModApplyButton.setDisabled(True)
        self.weModResetButton.setDisabled(True)

        self.apply_thread = WeModCustomization(self.weModVersions, weModInstallPath, selectedWeModVersion, self)
        self.apply_thread.message.connect(self.show_alert)
        self.apply_thread.finished.connect(self.on_finished)
        self.apply_thread.start()
    
    def applyCheatEngineCustomization(self):
        self.ceApplyButton.setDisabled(True)
        self.ceResetButton.setDisabled(True)

        if self.addzhCNCheckbox.isChecked():
            msg_box = QMessageBox(
                QMessageBox.Icon.Question,
                tr("Administrator Access Required"),
                tr("To proceed with adding translation files, administrator rights are needed. A User Account Control (UAC) prompt will appear for permission.") +
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
                source_path = resource_path("dependency/zh_CN")
                destination_path = os.path.join(self.ceInstallLineEdit.text(), "languages", "zh_CN")

                try:
                    subprocess.run([elevator_path, 'translation', source_path, destination_path], check=True, shell=True)
                    self.show_alert(tr("Successfully added translation files"), 'success')
                except subprocess.CalledProcessError:
                    self.show_alert(tr("Failed to add translation files"), 'error')

        self.ceApplyButton.setEnabled(True)
        self.ceResetButton.setEnabled(True)
