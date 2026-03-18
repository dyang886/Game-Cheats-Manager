from PyQt6.QtCore import QEasingCurve, QPropertyAnimation, QRect, Qt, QTimer, pyqtSignal
from PyQt6.QtGui import QColor, QFont, QFontDatabase, QPainter, QPixmap
from PyQt6.QtWidgets import QApplication, QFrame, QGraphicsDropShadowEffect, QHBoxLayout, QLabel, QListWidget, QListWidgetItem, QProxyStyle, QPushButton, QStyle, QVBoxLayout, QWidget
from zhon.cedict import simp, trad

from config import *


class LargerActionIconStyle(QProxyStyle):
    def pixelMetric(self, metric, option=None, widget=None):
        if metric == QStyle.PixelMetric.PM_SmallIconSize:
            return 20
        return super().pixelMetric(metric, option, widget)


class ToastNotification(QWidget):
    action_accepted = pyqtSignal()

    def __init__(self, title, message, notification_type="info"):
        super().__init__()
        theme = settings.get("theme", "dark")
        header_logo_path = resource_path("assets/logo.ico")
        body_logo_path = resource_path("assets/logo.png")

        self.setWindowFlags(
            Qt.WindowType.FramelessWindowHint |
            Qt.WindowType.WindowStaysOnTopHint |
            Qt.WindowType.Tool
        )
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)
        self.setFixedWidth(400)

        # Main Layout (Padding for the drop shadow)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(15, 15, 15, 15)

        # Background Frame
        self.frame = QFrame(self)
        self.frame.setObjectName("ToastFrame")
        frame_layout = QVBoxLayout(self.frame)
        frame_layout.setContentsMargins(16, 14, 16, 16)
        frame_layout.setSpacing(12)

        # Apply Drop Shadow
        shadow = QGraphicsDropShadowEffect(self)
        shadow.setBlurRadius(15)
        shadow.setXOffset(0)
        shadow.setYOffset(4)
        shadow_color = QColor(0, 0, 0, 100) if theme == "dark" else QColor(0, 0, 0, 50)
        shadow.setColor(shadow_color)
        self.frame.setGraphicsEffect(shadow)

        app_font = QApplication.font()

        # --- HEADER ROW ---
        header_layout = QHBoxLayout()
        header_layout.setSpacing(8)

        self.lbl_logo = QLabel()
        pixmap = QPixmap(header_logo_path).scaled(16, 16, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation)
        self.lbl_logo.setPixmap(pixmap)

        self.lbl_app_name = QLabel("Game Cheats Manager")
        self.lbl_app_name.setObjectName("AppNameText")
        self.lbl_app_name.setFont(app_font)

        header_layout.addWidget(self.lbl_logo)
        header_layout.addWidget(self.lbl_app_name)
        header_layout.addStretch()
        frame_layout.addLayout(header_layout)

        # --- BODY ROW ---
        body_layout = QHBoxLayout()
        body_layout.setSpacing(12)

        # Body Logo (Left)
        self.lbl_body_logo = QLabel()
        self.lbl_body_logo.setFixedSize(64, 64)
        body_pixmap = QPixmap(body_logo_path).scaled(64, 64, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation)
        self.lbl_body_logo.setPixmap(body_pixmap)
        self.lbl_body_logo.setAlignment(Qt.AlignmentFlag.AlignCenter)
        body_layout.addWidget(self.lbl_body_logo, alignment=Qt.AlignmentFlag.AlignVCenter)

        # Text Vertical Layout (Right)
        text_layout = QVBoxLayout()
        text_layout.setSpacing(4)

        # Title
        title_font = QFont(app_font)
        title_font.setPointSize(app_font.pointSize() + 3)

        self.lbl_title = QLabel(title)
        self.lbl_title.setObjectName("TitleText")
        self.lbl_title.setFont(title_font)
        self.lbl_title.setWordWrap(True)

        # Message
        msg_font = QFont(app_font)
        msg_font.setPointSize(app_font.pointSize() + 1)

        self.lbl_message = QLabel(message)
        self.lbl_message.setObjectName("MessageText")
        self.lbl_message.setFont(msg_font)
        self.lbl_message.setWordWrap(True)

        text_layout.addWidget(self.lbl_title)
        text_layout.addWidget(self.lbl_message)

        body_layout.addLayout(text_layout)
        frame_layout.addLayout(body_layout)

        # --- FOOTER ROW (Buttons) ---
        btn_layout = QHBoxLayout()
        btn_layout.setContentsMargins(0, 6, 0, 0)
        btn_layout.addStretch()

        if notification_type == "update":
            self.btn_yes = QPushButton(tr("Yes"))
            self.btn_yes.setObjectName("PrimaryButton")
            self.btn_yes.setCursor(Qt.CursorShape.PointingHandCursor)
            self.btn_yes.clicked.connect(self.on_accept)

            self.btn_no = QPushButton(tr("No"))
            self.btn_no.setCursor(Qt.CursorShape.PointingHandCursor)
            self.btn_no.clicked.connect(self.close)

            btn_layout.addWidget(self.btn_yes)
            btn_layout.addWidget(self.btn_no)
        else:
            self.btn_ok = QPushButton(tr("Close"))
            self.btn_ok.setCursor(Qt.CursorShape.PointingHandCursor)
            self.btn_ok.clicked.connect(self.close)
            btn_layout.addWidget(self.btn_ok)

        frame_layout.addLayout(btn_layout)
        layout.addWidget(self.frame)

        self.apply_theme(theme)

        # Show for exactly 7 seconds
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.close)
        self.timer.start(7000)

    def apply_theme(self, theme):
        if theme == "dark":
            style = """
                QFrame#ToastFrame {
                    background-color: #202020;
                    border: 1px solid #333333;
                    border-radius: 8px;
                }
                QLabel#AppNameText { color: #aaaaaa; }
                QLabel#TitleText { color: #ffffff; }
                QLabel#MessageText { color: #cccccc; }
                QPushButton {
                    padding: 8px 20px;
                    border-radius: 4px;
                    border: 1px solid #444444;
                    background-color: #2d2d2d;
                    color: #ffffff;
                }
                QPushButton:hover { background-color: #383838; }
                QPushButton:pressed { background-color: #1a1a1a; }
                QPushButton#PrimaryButton {
                    background-color: #0080e3;
                    border: 1px solid #0080e3;
                }
                QPushButton#PrimaryButton:hover { background-color: #1a90e8; border: 1px solid #1a90e8; }
            """
        else:
            style = """
                QFrame#ToastFrame {
                    background-color: #f3f3f3;
                    border: 1px solid #e5e5e5;
                    border-radius: 8px;
                }
                QLabel#AppNameText { color: #666666; }
                QLabel#TitleText { color: #000000; }
                QLabel#MessageText { color: #333333; }
                QPushButton {
                    padding: 8px 20px;
                    border-radius: 4px;
                    border: 1px solid #cccccc;
                    background-color: #ffffff;
                    color: #000000;
                }
                QPushButton:hover { background-color: #f0f0f0; }
                QPushButton:pressed { background-color: #e0e0e0; }
                QPushButton#PrimaryButton {
                    background-color: #0057b7;
                    color: #ffffff;
                    border: 1px solid #0057b7;
                }
                QPushButton#PrimaryButton:hover { background-color: #0065d1; border: 1px solid #0065d1; }
            """
        self.setStyleSheet(style)

    def show_notification(self):
        self.adjustSize()
        screen_geo = QApplication.primaryScreen().availableGeometry()
        offset = 20

        # Target resting position
        end_x = screen_geo.width() - self.width() - offset
        end_y = screen_geo.height() - self.height() - offset

        # Start completely off-screen to the right
        start_x = screen_geo.width()
        start_y = end_y

        self.setGeometry(start_x, start_y, self.width(), self.height())
        self.show()

        # Smooth slide-in animation
        self.animation = QPropertyAnimation(self, b"geometry")
        self.animation.setDuration(450)
        self.animation.setEasingCurve(QEasingCurve.Type.OutCubic)
        self.animation.setStartValue(QRect(start_x, start_y, self.width(), self.height()))
        self.animation.setEndValue(QRect(end_x, end_y, self.width(), self.height()))
        self.animation.start()

    def on_accept(self):
        self.action_accepted.emit()
        self.close()


class CustomButton(QPushButton):
    def __init__(self, text, parent=None):
        super(CustomButton, self).__init__(text, parent)
        self.setCursor(Qt.CursorShape.PointingHandCursor)
        self._cursor_overridden = False

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
        self._cursor_overridden = True

    def leaveEvent(self, event):
        super().leaveEvent(event)
        QApplication.restoreOverrideCursor()
        self._cursor_overridden = False

    def hideEvent(self, event):
        if self._cursor_overridden:
            QApplication.restoreOverrideCursor()
            self._cursor_overridden = False
        super().hideEvent(event)


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
        self.auto_close_timer = None
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
        """Override close to remove the alert from the parent's active list and cancel auto-close timer."""
        # Cancel any pending auto-close timer
        if self.auto_close_timer is not None:
            self.auto_close_timer.stop()
            self.auto_close_timer = None

        # Remove from parent's active alerts list
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
