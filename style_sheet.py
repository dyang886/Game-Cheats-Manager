white = """
    QMainWindow {{
        background-color: #ffffff;
    }}

    QStatusBar::item {{
        border: none;
    }}

    QMenuBar {{
        background-color: #f0f0f0;
    }}

    QMenuBar::item {{
        background-color: #f0f0f0;
        color: #000000;
        padding: 5px;
    }}

    QMenuBar::item:selected {{
        background-color: #cccccc;
    }}

    QMenu {{
        background-color: #ffffff;
        border: 2px solid #000000;
    }}

    QMenu::item {{
        background-color: #ffffff;
        color: #000000;
    }}

    QMenu::item:selected {{
        background-color: #cccccc;
    }}

    QStatusBar {{
        color: black;
    }}

    QCheckBox {{
        color: black;
    }}

    QPushButton {{
        padding: 9px;
        border-radius: 3px;
        background-color: #e6e6e6;
        color: #000000;
        outline: none;
    }}

    QPushButton:hover {{
        background-color: #f2f2f2;
    }}

    QPushButton:pressed {{
        background-color: #d9d9d9;
    }}

    QComboBox {{
        padding: 7px;
        border-radius: 3px;
        background-color: #e6e6e6;
        color: #000000;
    }}

    QComboBox::drop-down {{
        border: 0px;
        padding-right: 10px;
    }}

    QComboBox::down-arrow {{
        image: url({drop_down_arrow});
        width: 10px;
    }}

    QComboBox QAbstractItemView {{
        background-color: #e6e6e6;
        color: #000000;
    }}

    QDialog {{
        background-color: #ffffff;
    }}

    QLabel {{
        color: #000000;
    }}

    QTabWidget::pane {{
        border-top: 2px solid #f0f0f0;
    }}

    QTabBar::tab {{
        background-color: #e6e6e6;
        color: #000000;
        padding: 10px;
        border-radius: 3px;
    }}

    QTabBar::tab:hover {{
        background-color: #f2f2f2;
    }}

    QTabBar::tab:selected {{
        background-color: #d9d9d9;
    }}

    QLineEdit {{
        background-color: #e6e6e6;
        color: #000000;
        border: 1px solid #aaaaaa;
        border-radius: 3px;
        padding: 6px;
    }}

    QLineEdit:focus {{
        border-bottom: 2px solid #0057b7;
    }}

    QListWidget {{
        border: 1px solid #8c8c8c;
        border-radius: 3px;
        background-color: #e6e6e6;
        color: #000000;
    }}

    QScrollBar:vertical {{
        background-color: #f2f2f2;
        width: 15px;
        margin: 15px 0 15px 0;
    }}

    QScrollBar::handle:vertical {{
        background-color: #bfbfbf;
        min-height: 20px;
        border-radius: 3px;
        margin: 0 4px 0 4px;
    }}

    QScrollBar::handle:vertical:hover {{
        background-color: #c6c6c6;
    }}

    QScrollBar::handle:vertical:pressed {{
        background-color: #b1b1b1;
    }}

    QScrollBar::sub-line:vertical {{
        image: url({scroll_bar_top});
        background-color: #f2f2f2;
        height: 15px;
        border-top-left-radius: 7px;
        border-top-right-radius: 7px;
        subcontrol-position: top;
        subcontrol-origin: margin;
    }}

    QScrollBar::add-line:vertical {{
        image: url({scroll_bar_bottom});
        background-color: #f2f2f2;
        height: 15px;
        border-bottom-left-radius: 7px;
        border-bottom-right-radius: 7px;
        subcontrol-position: bottom;
        subcontrol-origin: margin;
    }}

    QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {{
        background: none;
    }}

    QScrollBar:horizontal {{
        background-color: #f2f2f2;
        height: 15px;
        margin: 0 15px 0 15px;
    }}

    QScrollBar::handle:horizontal {{
        background-color: #bfbfbf;
        min-width: 20px;
        border-radius: 3px;
        margin: 4px 0 4px 0;
    }}

    QScrollBar::handle:horizontal:hover {{
        background-color: #c6c6c6;
    }}

    QScrollBar::handle:horizontal:pressed {{
        background-color: #b1b1b1;
    }}

    QScrollBar::sub-line:horizontal {{
        image: url({scroll_bar_left});
        background-color: #f2f2f2;
        width: 15px;
        border-top-left-radius: 7px;
        border-bottom-left-radius: 7px;
        subcontrol-position: left;
        subcontrol-origin: margin;
    }}

    QScrollBar::add-line:horizontal {{
        image: url({scroll_bar_right});
        background-color: #f2f2f2;
        width: 15px;
        border-top-right-radius: 7px;
        border-bottom-right-radius: 7px;
        subcontrol-position: right;
        subcontrol-origin: margin;
    }}

    QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {{
        background: none;
    }}
"""

black = """
    QMainWindow {{
        background-color: #1c1c1c;
    }}

    QStatusBar::item {{
        border: none;
    }}

    QMenuBar {{
        background-color: #2e2e2e;
    }}

    QMenuBar::item {{
        background-color: #2e2e2e;
        color: #FFFFFF;
        padding: 5px;
    }}

    QMenuBar::item:selected {{
        background-color: #484848;
    }}

    QMenu {{
        background-color: #1c1c1c;
        border: 2px solid #ffffff;
    }}

    QMenu::item {{
        background-color: #1c1c1c;
        color: #FFFFFF;
    }}

    QMenu::item:selected {{
        background-color: #484848;
    }}

    QStatusBar {{
        color: white;
    }}

    QCheckBox {{
        color: white;
    }}

    QPushButton {{
        padding: 9px;
        border-radius: 3px;
        background-color: #2a2a2a;
        color: #FFFFFF;
        outline: none;
    }}

    QPushButton:hover {{
        background-color: #2f2f2f;
    }}

    QPushButton:pressed {{
        background-color: #232323;
    }}

    QComboBox {{
        padding: 7px;
        border-radius: 3px;
        background-color: #2a2a2a;
        color: #FFFFFF;
    }}

    QComboBox::drop-down {{
        border: 0px;
        padding-right: 10px;
    }}

    QComboBox::down-arrow {{
        image: url({drop_down_arrow});
        width: 10px;
    }}

    QComboBox QAbstractItemView {{
        background-color: #2a2a2a;
        color: #FFFFFF;
    }}

    QDialog {{
        background-color: #1c1c1c;
    }}

    QLabel {{
        color: #FFFFFF;
    }}

    QTabWidget::pane {{
        border-top: 2px solid #2e2e2e;
    }}

    QTabBar::tab {{
        background-color: #2a2a2a;
        color: #FFFFFF;
        padding: 10px;
        border-radius: 3px;
    }}

    QTabBar::tab:hover {{
        background-color: #2f2f2f;
    }}

    QTabBar::tab:selected {{
        background-color: #232323;
    }}

    QLineEdit {{
        background-color: #2a2a2a;
        color: #FFFFFF;
        border: 1px solid #555555;
        border-radius: 3px;
        padding: 6px;
    }}

    QLineEdit:focus {{
        border-bottom: 2px solid #007ad9;
    }}

    QListWidget {{
        border: 1px solid #a8a8a8;
        border-radius: 3px;
        background-color: #2a2a2a;
        color: #ffffff;
    }}

    QScrollBar:vertical {{
        background-color: #2f2f2f;
        width: 15px;
        margin: 15px 0 15px 0;
    }}

    QScrollBar::handle:vertical {{
        background-color: #636363;
        min-height: 20px;
        border-radius: 3px;
        margin: 0 4px 0 4px;
    }}

    QScrollBar::handle:vertical:hover {{
        background-color: #6f6f6f;
    }}

    QScrollBar::handle:vertical:pressed {{
        background-color: #5c5c5c;
    }}

    QScrollBar::sub-line:vertical {{
        image: url({scroll_bar_top});
        background-color: #2f2f2f;
        height: 15px;
        border-top-left-radius: 7px;
        border-top-right-radius: 7px;
        subcontrol-position: top;
        subcontrol-origin: margin;
    }}

    QScrollBar::add-line:vertical {{
        image: url({scroll_bar_bottom});
        background-color: #2f2f2f;
        height: 15px;
        border-bottom-left-radius: 7px;
        border-bottom-right-radius: 7px;
        subcontrol-position: bottom;
        subcontrol-origin: margin;
    }}

    QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {{
        background: none;
    }}

    QScrollBar:horizontal {{
        background-color: #2f2f2f;
        height: 15px;
        margin: 0 15px 0 15px;
    }}

    QScrollBar::handle:horizontal {{
        background-color: #636363;
        min-width: 20px;
        border-radius: 3px;
        margin: 4px 0 4px 0;
    }}

    QScrollBar::handle:horizontal:hover {{
        background-color: #6f6f6f;
    }}

    QScrollBar::handle:horizontal:pressed {{
        background-color: #5c5c5c;
    }}

    QScrollBar::sub-line:horizontal {{
        image: url({scroll_bar_left});
        background-color: #2f2f2f;
        width: 15px;
        border-top-left-radius: 7px;
        border-bottom-left-radius: 7px;
        subcontrol-position: left;
        subcontrol-origin: margin;
    }}

    QScrollBar::add-line:horizontal {{
        image: url({scroll_bar_right});
        background-color: #2f2f2f;
        width: 15px;
        border-top-right-radius: 7px;
        border-bottom-right-radius: 7px;
        subcontrol-position: right;
        subcontrol-origin: margin;
    }}

    QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {{
        background: none;
    }}
"""
