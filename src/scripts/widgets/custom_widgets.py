from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtGui import QColor, QFont, QFontDatabase, QPainter
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

        if self.underMouse():
            QApplication.restoreOverrideCursor()
            QApplication.setOverrideCursor(self.cursor().shape())
    
    def setDisabled(self, disabled):
        super().setDisabled(disabled)
        if disabled:
            self.setCursor(Qt.CursorShape.ForbiddenCursor)
        else:
            self.setCursor(Qt.CursorShape.PointingHandCursor)

        if self.underMouse():
            QApplication.restoreOverrideCursor()
            QApplication.setOverrideCursor(self.cursor().shape())

    def enterEvent(self, event):
        super().enterEvent(event)
        QApplication.restoreOverrideCursor()
        QApplication.setOverrideCursor(self.cursor().shape())

    def leaveEvent(self, event):
        super().leaveEvent(event)
        QApplication.restoreOverrideCursor()


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


class AlertWidget(QWidget):
    def __init__(self, parent, message, alert_type):
        super().__init__(parent)
        self.message = message
        self.alert_type = alert_type
        self.margin = 10
        self.colors = {
            "info": "#3498db",     # Blue
            "success": "#007110",  # Green
            "warning": "#f1c40f",  # Yellow
            "error": "#e74c3c",    # Red
        }
        self.init_ui()

    def init_ui(self):
        self.setWindowFlags(Qt.WindowType.FramelessWindowHint | Qt.WindowType.Tool)
        self.setAttribute(Qt.WidgetAttribute.WA_DeleteOnClose)

        self.label = QLabel(self.message, self)
        self.label.setStyleSheet("""
            color: white;
            padding: 10px;
            font-weight: bold;
        """)
        self.label.adjustSize()

        # Adjust the size of the custom widget
        alert_width = self.label.width()
        alert_height = self.label.height()
        self.setFixedSize(alert_width, alert_height)
        self.label.move(0, 0)

        # Add to the parent window's active alerts list
        if hasattr(self.parent(), "active_alerts"):
            self.parent().active_alerts.append(self)
            self.enforce_alert_limit()
            self.move_to_top_right()

    def enforce_alert_limit(self):
        """Ensure the total height of alerts doesn't exceed the parent window height."""
        if self.parent():
            max_alerts = (self.parent().height() - self.margin) // (self.height() + self.margin)
            while len(self.parent().active_alerts) > max_alerts:
                self.parent().active_alerts[0].close()

    def move_to_top_right(self):
        """Position the alert at the top-right corner below the previous alerts."""
        if self.parent():
            dialog_rect = self.parent().geometry()
            for index, alert in enumerate(self.parent().active_alerts):
                alert_x = dialog_rect.topRight().x() - alert.width() - self.margin
                alert_y = dialog_rect.topRight().y() + self.margin + index * (alert.height() + self.margin)
                alert.move(alert_x, alert_y)

    def close(self):
        """Override close to remove the alert from the parent's active list."""
        if hasattr(self.parent(), "active_alerts") and self in self.parent().active_alerts:
            self.parent().active_alerts.remove(self)
            self.move_to_top_right()
        super().close()

    def paintEvent(self, event):
        """Draw the rounded rectangle background."""
        painter = QPainter(self)
        painter.setBrush(QColor(self.colors[self.alert_type]))
        painter.setPen(Qt.PenStyle.NoPen)
        painter.drawRoundedRect(self.rect(), 5, 5)
