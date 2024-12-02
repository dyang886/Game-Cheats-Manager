import os

from PyQt6.QtCore import QTimer, QUrl, pyqtSignal
from PyQt6.QtGui import QIcon
from PyQt6.QtWebEngineCore import QWebEngineDownloadRequest
from PyQt6.QtWebEngineWidgets import QWebEngineView
from PyQt6.QtWidgets import QDialog, QVBoxLayout

from config import *


class BrowserDialog(QDialog):
    content_ready = pyqtSignal(str)
    download_completed = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.browser = QWebEngineView(self)
        layout = QVBoxLayout(self)
        layout.addWidget(self.browser)
        self.setLayout(layout)
        self.resize(800, 600)
        self.setWindowTitle(tr("Please complete any security checks") + "/" + tr("Unable to access webpage"))
        self.setWindowIcon(QIcon(resource_path("assets/logo.ico")))

        self.check_timer = QTimer(self)
        self.found_content = None
        self.check_count = 0
        self.download_path = ""
        self.file_name = ""

    def load_url(self, url, target_text):
        self.url = url
        self.target_text = target_text
        self.found_content = False
        self.check_count = 0
        self.browser.loadFinished.connect(self.on_load_finished)
        self.browser.load(QUrl(self.url))
        self.hide()

    def on_load_finished(self, success):
        self.check_timer.timeout.connect(self.check_content)
        self.check_timer.start(500)

    def check_content(self):
        if self.check_count >= 5:
            self.show()
        self.check_count += 1
        self.browser.page().toHtml(self.handle_html)

    def handle_html(self, html):
        if self.target_text in html:
            self.found_content = True
            self.check_timer.stop()
            self.content_ready.emit(html)
            self.close()

    def closeEvent(self, event):
        if not self.found_content == None and not self.found_content:
            self.check_timer.stop()
            self.browser.loadFinished.disconnect(self.on_load_finished)
            self.content_ready.emit("")
        event.accept()

    def handle_download(self, url, download_path, file_name):
        self.download_path = download_path
        self.file_name = file_name
        self.browser.page().profile().downloadRequested.connect(self.on_download_requested)
        self.browser.load(QUrl(url))

    def on_download_requested(self, download):
        suggested_filename = download.downloadFileName()
        extension = os.path.splitext(suggested_filename)[1]
        file_name = self.file_name + extension

        download.setDownloadDirectory(self.download_path)
        download.setDownloadFileName(file_name)
        download.accept()

        file_path = os.path.join(self.download_path, file_name)
        download.stateChanged.connect(lambda state: self.on_download_state_changed(state, file_path))

    def on_download_state_changed(self, state, file_path):
        if state == QWebEngineDownloadRequest.DownloadState.DownloadCompleted:
            self.browser.page().profile().downloadRequested.disconnect(self.on_download_requested)
            self.download_completed.emit(file_path)
            self.close()
