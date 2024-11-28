from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtGui import QFont, QFontDatabase
from PyQt6.QtWidgets import QApplication, QHBoxLayout, QLabel, QListWidget, QListWidgetItem, QPushButton, QWidget
from zhon.cedict import simp, trad

from config import *


class CustomButton(QPushButton):
    def __init__(self, text, parent=None):
        super(CustomButton, self).__init__(text, parent)
        self.setCursor(Qt.CursorShape.PointingHandCursor)

    def setEnabled(self, enabled):
        super().setEnabled(enabled)
        if enabled:
            self.setCursor(Qt.CursorShape.PointingHandCursor)
        else:
            self.setCursor(Qt.CursorShape.ForbiddenCursor)

    def enterEvent(self, event):
        if not self.isEnabled():
            QApplication.setOverrideCursor(Qt.CursorShape.ForbiddenCursor)
        super().enterEvent(event)

    def leaveEvent(self, event):
        QApplication.restoreOverrideCursor()
        super().leaveEvent(event)


class StatusMessageWidget(QWidget):
    def __init__(self, widgetName, message):
        super().__init__()
        self.setObjectName(widgetName)

        self.layout = QHBoxLayout()
        self.layout.setSpacing(3)
        self.setLayout(self.layout)

        self.messageLabel = QLabel(message)
        self.layout.addWidget(self.messageLabel)

        self.loadingLabel = QLabel(".")
        self.loadingLabel.setFixedWidth(20)
        self.layout.addWidget(self.loadingLabel)

        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_loading_animation)
        self.timer.start(500)

    def update_loading_animation(self):
        current_text = self.loadingLabel.text()
        new_text = '.' * ((len(current_text) % 3) + 1)
        self.loadingLabel.setText(new_text)

    def update_message(self, newMessage, state="load"):
        self.messageLabel.setText(newMessage)
        if state == "load":
            if not self.loadingLabel.isVisible():
                self.loadingLabel.show()
            if not self.timer.isActive():
                self.timer.start(500)
            self.messageLabel.setStyleSheet("")
        elif state == "error":
            self.timer.stop()
            self.loadingLabel.hide()
            self.messageLabel.setStyleSheet("QLabel { color: red; }")


class MultilingualListWidget(QListWidget):
    def __init__(self):
        super().__init__()
        self.english_font = QFont(QFontDatabase.applicationFontFamilies(QFontDatabase.addApplicationFont(font_config['en_US']))[0], 10)
        self.chinese_simplified_font = QFont(QFontDatabase.applicationFontFamilies(QFontDatabase.addApplicationFont(font_config['zh_CN']))[0], 10)
        self.chinese_traditional_font = QFont(QFontDatabase.applicationFontFamilies(QFontDatabase.addApplicationFont(font_config['zh_TW']))[0], 10)

    def addItem(self, item):
        if isinstance(item, str):
            item = QListWidgetItem(item)

        text = item.text()
        if self.is_chinese_simplified(text):
            item.setFont(self.chinese_simplified_font)
        elif self.is_chinese_traditional(text):
            item.setFont(self.chinese_traditional_font)
        else:
            item.setFont(self.english_font)

        super().addItem(item)

    @staticmethod
    def is_chinese_simplified(text):
        return any(char in simp for char in text)

    @staticmethod
    def is_chinese_traditional(text):
        return any(char in trad for char in text)
