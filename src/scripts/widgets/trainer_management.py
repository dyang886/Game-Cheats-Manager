from packaging import version
import subprocess
import re

from PyQt6.QtWidgets import QCheckBox, QComboBox, QDialog, QFileDialog, QHBoxLayout, QLabel, QLineEdit, QMessageBox, QSizePolicy, QTabWidget, QVBoxLayout, QWidget
from PyQt6.QtGui import QIcon, QPixmap
from PyQt6.QtCore import Qt, QTimer

from config import *
from widgets.custom_widgets import AlertWidget, CustomButton, apply_native_title_bar_theme
from threads.other_threads import WeModCustomization


class TrainerManagementDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle(tr("Trainer Management"))
        self.setWindowIcon(QIcon(resource_path("assets/logo.ico")))
        apply_native_title_bar_theme(self)
        self.setMinimumWidth(650)
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
        self.tabWidget.addTab(self.createGCMTab(), "GCM")
        self.tabWidget.addTab(self.createFlingTab(), tr("FLiNG"))
        self.tabWidget.addTab(self.createXiaoXingTab(), tr("XiaoXing"))
        self.tabWidget.addTab(self.createCETab(), "Cheat Engine")
        self.tabWidget.addTab(self.createWemodTab(), "Wand")
        self.tabWidget.addTab(self.createCevoTab(), "Cheat Evolution")

    def closeEvent(self, event):
        super().closeEvent(event)

        while self.active_alerts:
            self.active_alerts[0].close()

        oldFlingServer = settings["flingDownloadServer"]
        newFlingServer = server_options[self.serverCombo.currentText()]
        if oldFlingServer != newFlingServer:
            self.parent().fetch_fling_data()
        oldEnableXiaoXing = settings["enableXiaoXing"]
        newEnableXiaoXing = self.enableXiaoXingCheckbox.isChecked()
        if not oldEnableXiaoXing and newEnableXiaoXing:
            self.parent().fetch_xiaoxing_data()

        settings["enableGCM"] = self.enableGCMCheckbox.isChecked()
        settings["autoUpdateGCMData"] = self.autoUpdateGCMDataCheckbox.isChecked()
        settings["autoUpdateGCMTrainers"] = self.autoUpdateGCMTrainersCheckbox.isChecked()
        settings["flingDownloadServer"] = newFlingServer
        settings["removeFlingBgMusic"] = self.removeFlingBgMusicCheckbox.isChecked()
        settings["autoUpdateFlingData"] = self.autoUpdateFlingDataCheckbox.isChecked()
        settings["autoUpdateFlingTrainers"] = self.autoUpdateFlingTrainersCheckbox.isChecked()
        settings["enableXiaoXing"] = self.enableXiaoXingCheckbox.isChecked()
        settings["unlockXiaoXing"] = self.unlockXiaoXingCheckbox.isChecked()
        settings["autoUpdateXiaoXingData"] = self.autoUpdateXiaoXingDataCheckbox.isChecked()
        settings["autoUpdateXiaoXingTrainers"] = self.autoUpdateXiaoXingTrainersCheckbox.isChecked()
        settings["enableCT"] = self.enableCTCheckbox.isChecked()
        settings["autoUpdateCTData"] = self.autoUpdateCTDataCheckbox.isChecked()
        settings["autoUpdateCTTrainers"] = self.autoUpdateCTTrainersCheckbox.isChecked()
        apply_settings(settings)

    @staticmethod
    def find_settings_key(value, dict):
        try:
            return next(key for key, val in dict.items() if val == value)
        except StopIteration:
            return next(iter(dict.values()))

    def show_alert(self, message, alert_type):
        alert = AlertWidget(self, message, alert_type)
        alert.show()
        alert.auto_close_timer = QTimer()
        alert.auto_close_timer.setSingleShot(True)
        alert.auto_close_timer.timeout.connect(alert.close)
        alert.auto_close_timer.start(5000)

    def moveEvent(self, event):
        super().moveEvent(event)
        for alert in self.active_alerts:
            alert.move_to_top_right()

    def createGCMTab(self):
        gcmTab = QWidget()
        layout = QVBoxLayout()
        layout.setContentsMargins(0, 15, 0, 0)
        gcmTab.setLayout(layout)

        columns = QHBoxLayout()
        columns.setContentsMargins(30, 20, 30, 20)
        columns.setSpacing(40)
        layout.addLayout(columns)

        column1 = QVBoxLayout()
        columns.addLayout(column1, stretch=2)

        logoPixmap = QPixmap(resource_path("assets/logo_trainer.png"))
        scaledLogoPixmap = logoPixmap.scaled(130, 130, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation)
        logoLabel = QLabel()
        logoLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        logoLabel.setPixmap(scaledLogoPixmap)
        column1.addWidget(logoLabel)

        column2 = QVBoxLayout()
        column2.setSpacing(15)
        column2.addStretch(1)
        columns.addLayout(column2, stretch=3)
        columns.setAlignment(column2, Qt.AlignmentFlag.AlignHCenter)

        # Enable GCM and Other trainer
        self.enableGCMCheckbox = QCheckBox(tr("Enable search for GCM and Other trainers"))
        self.enableGCMCheckbox.setChecked(settings["enableGCM"])
        column2.addWidget(self.enableGCMCheckbox)

        # Auto update GCM data
        self.autoUpdateGCMDataCheckbox = QCheckBox(tr("Update GCM data automatically"))
        self.autoUpdateGCMDataCheckbox.setChecked(settings["autoUpdateGCMData"])
        column2.addWidget(self.autoUpdateGCMDataCheckbox)

        # Auto update GCM trainers
        self.autoUpdateGCMTrainersCheckbox = QCheckBox(tr("Update GCM and Other trainers automatically"))
        self.autoUpdateGCMTrainersCheckbox.setChecked(settings["autoUpdateGCMTrainers"])
        column2.addWidget(self.autoUpdateGCMTrainersCheckbox)

        column2.addStretch(1)
        return gcmTab

    def createFlingTab(self):
        flingTab = QWidget()
        layout = QVBoxLayout()
        layout.setContentsMargins(0, 15, 0, 0)
        flingTab.setLayout(layout)
        officialLink = QLabel(tr("Official Website: ") + '<a href="https://flingtrainer.com" style="text-decoration: none; color: #0078D4;">https://flingtrainer.com</a>')
        officialLink.setOpenExternalLinks(True)
        layout.addWidget(officialLink)

        columns = QHBoxLayout()
        columns.setContentsMargins(30, 20, 30, 20)
        columns.setSpacing(40)
        layout.addLayout(columns)

        column1 = QVBoxLayout()
        columns.addLayout(column1, stretch=2)

        logoPixmap = QPixmap(resource_path("assets/fling.png"))
        scaledLogoPixmap = logoPixmap.scaled(130, 130, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation)
        logoLabel = QLabel()
        logoLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        logoLabel.setPixmap(scaledLogoPixmap)
        column1.addWidget(logoLabel)

        column2 = QVBoxLayout()
        column2.setSpacing(15)
        column2.addStretch(1)
        columns.addLayout(column2, stretch=3)
        columns.setAlignment(column2, Qt.AlignmentFlag.AlignHCenter)

        # Fling server selection
        serverLayout = QVBoxLayout()
        serverLayout.setSpacing(2)
        column2.addLayout(serverLayout)
        serverLayout.addWidget(QLabel(tr("Download Server:")))
        self.serverCombo = QComboBox()
        self.serverCombo.addItems(server_options.keys())
        self.serverCombo.setCurrentText(self.find_settings_key(settings["flingDownloadServer"], server_options))
        self.serverCombo.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        serverLayout.addWidget(self.serverCombo, stretch=1)

        # Remove Fling trainer background music when downloading
        self.removeFlingBgMusicCheckbox = QCheckBox(tr("Remove trainer background music"))
        self.removeFlingBgMusicCheckbox.setChecked(settings["removeFlingBgMusic"])
        column2.addWidget(self.removeFlingBgMusicCheckbox)

        # Auto update Fling data
        self.autoUpdateFlingDataCheckbox = QCheckBox(tr("Update FLiNG data automatically"))
        self.autoUpdateFlingDataCheckbox.setChecked(settings["autoUpdateFlingData"])
        column2.addWidget(self.autoUpdateFlingDataCheckbox)

        # Auto update Fling trainers
        self.autoUpdateFlingTrainersCheckbox = QCheckBox(tr("Update FLiNG trainers automatically"))
        self.autoUpdateFlingTrainersCheckbox.setChecked(settings["autoUpdateFlingTrainers"])
        column2.addWidget(self.autoUpdateFlingTrainersCheckbox)

        column2.addStretch(1)
        return flingTab

    def createXiaoXingTab(self):
        xiaoXingTab = QWidget()
        layout = QVBoxLayout()
        layout.setContentsMargins(0, 15, 0, 0)
        xiaoXingTab.setLayout(layout)
        officialLink = QLabel(tr("Official Website: ") + '<a href="https://www.xiaoxingjie.com" style="text-decoration: none; color: #0078D4;">https://www.xiaoxingjie.com</a>')
        officialLink.setOpenExternalLinks(True)
        layout.addWidget(officialLink)

        columns = QHBoxLayout()
        columns.setContentsMargins(30, 20, 30, 20)
        columns.setSpacing(40)
        layout.addLayout(columns)

        column1 = QVBoxLayout()
        columns.addLayout(column1, stretch=2)

        logoPixmap = QPixmap(resource_path("assets/xiaoxing.png"))
        scaledLogoPixmap = logoPixmap.scaled(130, 130, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation)
        logoLabel = QLabel()
        logoLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        logoLabel.setPixmap(scaledLogoPixmap)
        column1.addWidget(logoLabel)

        column2 = QVBoxLayout()
        column2.setSpacing(15)
        column2.addStretch(1)
        columns.addLayout(column2, stretch=3)
        columns.setAlignment(column2, Qt.AlignmentFlag.AlignHCenter)

        # Enable XiaoXing trainer
        self.enableXiaoXingCheckbox = QCheckBox(tr("Enable search for XiaoXing trainers"))
        self.enableXiaoXingCheckbox.setChecked(settings["enableXiaoXing"])
        column2.addWidget(self.enableXiaoXingCheckbox)

        # Unlock paid functions
        self.unlockXiaoXingCheckbox = QCheckBox(tr("Unlock all functions"))
        self.unlockXiaoXingCheckbox.setChecked(settings["unlockXiaoXing"])
        column2.addWidget(self.unlockXiaoXingCheckbox)

        # Auto update XiaoXing data
        self.autoUpdateXiaoXingDataCheckbox = QCheckBox(tr("Update XiaoXing data automatically"))
        self.autoUpdateXiaoXingDataCheckbox.setChecked(settings["autoUpdateXiaoXingData"])
        column2.addWidget(self.autoUpdateXiaoXingDataCheckbox)

        # Auto update XiaoXing trainers
        self.autoUpdateXiaoXingTrainersCheckbox = QCheckBox(tr("Update XiaoXing trainers automatically"))
        self.autoUpdateXiaoXingTrainersCheckbox.setChecked(settings["autoUpdateXiaoXingTrainers"])
        column2.addWidget(self.autoUpdateXiaoXingTrainersCheckbox)

        column2.addStretch(1)
        return xiaoXingTab

    def createCETab(self):
        ceTab = QWidget()
        layout = QVBoxLayout()
        layout.setContentsMargins(0, 15, 0, 0)
        ceTab.setLayout(layout)
        officialLink = QLabel(tr("Official Website: ") + '<a href="https://www.cheatengine.org" style="text-decoration: none; color: #0078D4;">https://www.cheatengine.org</a>')
        officialLink.setOpenExternalLinks(True)
        layout.addWidget(officialLink)
        ctLink = QLabel(tr("Cheat Table Sources: ") + '<a href="https://www.thecheatscript.com" style="text-decoration: none; color: #0078D4;">https://www.thecheatscript.com</a>')
        ctLink.setOpenExternalLinks(True)
        layout.addWidget(ctLink)

        columns = QHBoxLayout()
        columns.setContentsMargins(30, 20, 30, 20)
        columns.setSpacing(40)
        layout.addLayout(columns)

        column1 = QVBoxLayout()
        columns.addLayout(column1, stretch=2)

        logoPixmap = QPixmap(resource_path("assets/ce.png"))
        scaledLogoPixmap = logoPixmap.scaled(130, 130, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation)
        logoLabel = QLabel()
        logoLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        logoLabel.setPixmap(scaledLogoPixmap)
        column1.addWidget(logoLabel)

        column2 = QVBoxLayout()
        column2.setSpacing(15)
        column2.addStretch(1)
        columns.addLayout(column2, stretch=3)
        columns.setAlignment(column2, Qt.AlignmentFlag.AlignHCenter)

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

        # add a dashed line
        line = QLabel()
        line.setFixedHeight(1)
        line.setStyleSheet("background-color: #C0C0C0;")
        column2.addWidget(line)

        # Enable CT
        self.enableCTCheckbox = QCheckBox(tr("Enable Search for Cheat Tables"))
        self.enableCTCheckbox.setChecked(settings["enableCT"])
        column2.addWidget(self.enableCTCheckbox)

        # Auto update CT data
        self.autoUpdateCTDataCheckbox = QCheckBox(tr("Update Cheat Table data automatically"))
        self.autoUpdateCTDataCheckbox.setChecked(settings["autoUpdateCTData"])
        column2.addWidget(self.autoUpdateCTDataCheckbox)

        # Auto update CT trainers
        self.autoUpdateCTTrainersCheckbox = QCheckBox(tr("Update Cheat Tables automatically"))
        self.autoUpdateCTTrainersCheckbox.setChecked(settings["autoUpdateCTTrainers"])
        column2.addWidget(self.autoUpdateCTTrainersCheckbox)

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

        column2.addStretch(1)
        return ceTab

    def createWemodTab(self):
        wemodTab = QWidget()
        layout = QVBoxLayout()
        layout.setContentsMargins(0, 15, 0, 0)
        wemodTab.setLayout(layout)
        officialLink = QLabel(tr("Official Website: ") + '<a href="https://wand.com" style="text-decoration: none; color: #0078D4;">https://wand.com</a>')
        officialLink.setOpenExternalLinks(True)
        layout.addWidget(officialLink)

        columns = QHBoxLayout()
        columns.setContentsMargins(30, 20, 30, 20)
        columns.setSpacing(40)
        layout.addLayout(columns)

        column1 = QVBoxLayout()
        columns.addLayout(column1, stretch=2)

        logoPixmap = QPixmap(resource_path("assets/wand.png"))
        scaledLogoPixmap = logoPixmap.scaled(130, 130, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation)
        logoLabel = QLabel()
        logoLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        logoLabel.setPixmap(scaledLogoPixmap)
        column1.addWidget(logoLabel)

        column2 = QVBoxLayout()
        column2.setSpacing(15)
        column2.addStretch(1)
        columns.addLayout(column2, stretch=3)
        columns.setAlignment(column2, Qt.AlignmentFlag.AlignHCenter)

        # WeMod installation path
        installLayout = QVBoxLayout()
        installLayout.setSpacing(2)
        column2.addLayout(installLayout)

        installLabelLayout = QHBoxLayout()
        installLabelLayout.setSpacing(3)
        installLayout.addLayout(installLabelLayout)
        installLabelLayout.addWidget(QLabel(tr("Wand installation path:")))
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
        versionLayout.addWidget(QLabel(tr("Installed Wand Versions:")))
        self.versionCombo = QComboBox()
        versionLayout.addWidget(self.versionCombo)

        # Patch method selection
        patchLayout = QVBoxLayout()
        patchLayout.setSpacing(2)
        column2.addLayout(patchLayout)
        patchLayout.addWidget(QLabel(tr("Patch Method:")))
        self.patchCombo = QComboBox()
        self.patchCombo.addItems(patch_methods.keys())
        self.patchCombo.setCurrentText(list(patch_methods.keys())[0])
        patchLayout.addWidget(self.patchCombo)

        # Unlock WeMod pro
        self.weModProCheckbox = QCheckBox(tr("Activate Wand Pro"))
        column2.addWidget(self.weModProCheckbox)

        # Disable auto update
        self.disableUpdateCheckbox = QCheckBox(tr("Disable Wand Auto Update"))
        column2.addWidget(self.disableUpdateCheckbox)

        # Delete all other WeMod versions
        self.delOtherVersionsCheckbox = QCheckBox(tr("Delete All Other Wand Versions"))
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

        column2.addStretch(1)
        return wemodTab

    def createCevoTab(self):
        cevoTab = QWidget()
        layout = QVBoxLayout()
        layout.setContentsMargins(0, 15, 0, 0)
        cevoTab.setLayout(layout)
        officialLink = QLabel(tr("Official Website: ") + '<a href="https://cheatevolution.com" style="text-decoration: none; color: #0078D4;">https://cheatevolution.com</a>')
        officialLink.setOpenExternalLinks(True)
        layout.addWidget(officialLink)

        columns = QHBoxLayout()
        columns.setContentsMargins(30, 20, 30, 20)
        columns.setSpacing(40)
        layout.addLayout(columns)

        column1 = QVBoxLayout()
        columns.addLayout(column1, stretch=2)

        logoPixmap = QPixmap(resource_path("assets/cevo.png"))
        scaledLogoPixmap = logoPixmap.scaled(130, 130, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation)
        logoLabel = QLabel()
        logoLabel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        logoLabel.setPixmap(scaledLogoPixmap)
        column1.addWidget(logoLabel)

        column2 = QVBoxLayout()
        column2.setSpacing(15)
        column2.addStretch(1)
        columns.addLayout(column2, stretch=3)
        columns.setAlignment(column2, Qt.AlignmentFlag.AlignHCenter)

        # Cheat Evolution installation status
        self.cevoInstallStatus = QLabel()
        column2.addWidget(self.cevoInstallStatus)

        # Cheat Evolution installation path
        installLayout = QVBoxLayout()
        installLayout.setSpacing(2)
        column2.addLayout(installLayout)

        installLabelLayout = QHBoxLayout()
        installLabelLayout.setSpacing(3)
        installLayout.addLayout(installLabelLayout)
        installLabelLayout.addWidget(QLabel(tr("Cheat Evolution installation path:")))
        installLabelLayout.addStretch(1)

        installPathLayout = QHBoxLayout()
        installPathLayout.setSpacing(5)
        installLayout.addLayout(installPathLayout)
        self.cevoInstallLineEdit = QLineEdit()
        self.cevoInstallLineEdit.setReadOnly(True)
        installPathLayout.addWidget(self.cevoInstallLineEdit)
        cevoInstallPathButton = CustomButton("...")
        cevoInstallPathButton.clicked.connect(self.selectCevoPath)
        installPathLayout.addWidget(cevoInstallPathButton)

        # Activate PRO
        self.cevoProCheckbox = QCheckBox(tr("Activate PRO"))
        column2.addWidget(self.cevoProCheckbox)

        # Apply button
        applyButtonLayout = QHBoxLayout()
        applyButtonLayout.setContentsMargins(0, 0, 10, 10)
        applyButtonLayout.addStretch(1)
        layout.addLayout(applyButtonLayout)
        self.cevoApplyButton = CustomButton(tr("Apply"))
        self.cevoApplyButton.setFixedWidth(100)
        self.cevoApplyButton.clicked.connect(self.applyCevoCustomization)
        applyButtonLayout.addWidget(self.cevoApplyButton)

        self.cevoInstallLineEdit.setText(settings.get("cevoPath", ""))
        self.checkCevoInstallStatus()

        column2.addStretch(1)
        return cevoTab

    # ===========================================================================
    # Tab functions
    # ===========================================================================
    def selectWeModPath(self):
        initialPath = self.weModInstallLineEdit.text() or os.path.expanduser("~")
        directory = QFileDialog.getExistingDirectory(self, tr("Please select Wand installation path"), initialPath)
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
        directory = QFileDialog.getExistingDirectory(self, tr("Please select Cheat Engine installation path"), initialPath)
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
            self.installStatus.setText(tr("Please select Cheat Engine installation path"))
            self.installStatus.setStyleSheet("color: red;")
            self.ceApplyButton.setDisabled(True)

    def findWeModVersions(self, weModPath):
        self.weModVersions = []
        if not os.path.exists(weModPath):
            self.versionCombo.clear()
            self.versionCombo.addItem(tr("Wand not installed"))
            self.weModApplyButton.setDisabled(True)
            return

        for item in os.listdir(weModPath):
            if os.path.isdir(os.path.join(weModPath, item)):
                match = re.match(r'app-(\d+\.\d+\.\d+.*)', item)
                if match:
                    version_info = match.group(1)  # for instance: 9.3.0, 11.5.0-beta00
                    self.weModVersions.append(version_info)

        if not self.weModVersions:
            self.versionCombo.clear()
            self.versionCombo.addItem(tr("Wand not installed"))
            self.weModApplyButton.setDisabled(True)
            return

        self.weModVersions.sort(key=version.parse, reverse=True)
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
        patchMethod = patch_methods[self.patchCombo.currentText()]
        self.weModApplyButton.setDisabled(True)
        self.weModResetButton.setDisabled(True)

        self.apply_thread = WeModCustomization(self.weModVersions, weModInstallPath, selectedWeModVersion, patchMethod, self)
        self.apply_thread.message.connect(self.show_alert, Qt.ConnectionType.BlockingQueuedConnection)
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
                source_path = resource_path("dependency/CE Translations/zh_CN")
                destination_path = os.path.join(self.ceInstallLineEdit.text(), "languages", "zh_CN")

                try:
                    subprocess.run([elevator_path, 'copy', source_path, destination_path], check=True, shell=True)
                    self.show_alert(tr("Successfully added translation files"), 'success')
                except subprocess.CalledProcessError:
                    self.show_alert(tr("Failed to add translation files"), 'error')

        self.ceApplyButton.setEnabled(True)
        self.ceResetButton.setEnabled(True)

    def selectCevoPath(self):
        initialPath = self.cevoInstallLineEdit.text() or os.path.expanduser("~")
        directory = QFileDialog.getExistingDirectory(self, tr("Please select Cheat Evolution installation path"), initialPath)
        if directory:
            settings["cevoPath"] = os.path.normpath(directory)
            self.cevoInstallLineEdit.setText(settings["cevoPath"])
            apply_settings(settings)
            self.checkCevoInstallStatus()

    def checkCevoInstallStatus(self):
        cevoPatchPath = os.path.join(self.cevoInstallLineEdit.text(), 'CheatEvolution_patched.exe')
        cevoPath = os.path.join(self.cevoInstallLineEdit.text(), 'CheatEvolution.exe')
        if os.path.exists(cevoPatchPath):
            self.cevoInstallStatus.setText(tr("Please launch Cheat Evolution using CheatEvolution_patched.exe"))
            self.cevoInstallStatus.setStyleSheet("color: green;")
            self.cevoApplyButton.setEnabled(True)
        elif not os.path.exists(cevoPath):
            self.cevoInstallStatus.setText(tr("Invalid Cheat Evolution installation path"))
            self.cevoInstallStatus.setStyleSheet("color: red;")
            self.cevoApplyButton.setDisabled(True)
        else:
            self.cevoInstallStatus.setText(tr("Cheat Evolution is installed and ready to patch"))
            self.cevoInstallStatus.setStyleSheet("color: yellow;")
            self.cevoApplyButton.setEnabled(True)

    def applyCevoCustomization(self):
        self.cevoApplyButton.setDisabled(True)

        if self.cevoProCheckbox.isChecked():
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
                source_path = resource_path("dependency/CheatEvolution_patched.exe")
                destination_path = os.path.join(self.cevoInstallLineEdit.text(), "CheatEvolution_patched.exe")

                try:
                    subprocess.run([elevator_path, 'copy', source_path, destination_path], check=True, shell=True)
                    self.show_alert(tr("Patch successful"), 'success')
                except subprocess.CalledProcessError:
                    self.show_alert(tr("Failed to patch"), 'error')

        self.cevoApplyButton.setEnabled(True)
        self.checkCevoInstallStatus()
