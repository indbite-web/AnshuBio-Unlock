#pragma once
#include <QString>

namespace AnshuBio {
namespace Theme {

constexpr const char* BG_MAIN = "#0c0f17";
constexpr const char* BG_SIDEBAR = "#080a10";
constexpr const char* BG_CARD = "#151926";
constexpr const char* BG_CARD_HOVER = "#1c2233";
constexpr const char* ACCENT_BLUE = "#3b82f6";
constexpr const char* ACCENT_HOVER = "#2563eb";
constexpr const char* ACCENT_GREEN = "#10b981";
constexpr const char* ACCENT_RED = "#ef4444";
constexpr const char* ACCENT_YELLOW = "#f59e0b";
constexpr const char* TEXT_PRIMARY = "#f8fafc";
constexpr const char* TEXT_SECONDARY = "#94a3b8";
constexpr const char* TEXT_MUTED = "#64748b";
constexpr const char* BORDER_COLOR = "#1e293b";

inline QString GetMainStyleSheet() {
    return QString(R"(
        QWidget {
            background-color: %1;
            color: %2;
            font-family: 'Segoe UI', system-ui, -apple-system, sans-serif;
            font-size: 13px;
        }

        QScrollBar:vertical {
            background: %3;
            width: 8px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical {
            background: %4;
            border-radius: 4px;
            min-height: 20px;
        }
        QScrollBar::handle:vertical:hover {
            background: %5;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }

        QPushButton {
            background-color: %5;
            color: #ffffff;
            border: 1px solid %6;
            border-radius: 6px;
            padding: 8px 16px;
            font-weight: 600;
        }
        QPushButton:hover {
            background-color: %7;
        }
        QPushButton:pressed {
            background-color: #1d4ed8;
        }
        QPushButton:disabled {
            background-color: #1e293b;
            color: #64748b;
            border-color: #334155;
        }

        QLineEdit {
            background-color: %8;
            color: %2;
            border: 1px solid %6;
            border-radius: 6px;
            padding: 8px 12px;
        }
        QLineEdit:focus {
            border: 1px solid %5;
        }

        QTableWidget {
            background-color: %8;
            border: 1px solid %6;
            border-radius: 8px;
            gridline-color: %6;
        }
        QHeaderView::section {
            background-color: %3;
            color: %9;
            font-weight: 600;
            padding: 8px;
            border: none;
            border-bottom: 1px solid %6;
        }

        QCheckBox {
            color: %2;
            spacing: 8px;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border-radius: 4px;
            border: 1px solid %6;
            background-color: %8;
        }
        QCheckBox::indicator:checked {
            background-color: %5;
            border-color: %5;
        }
    )")
    .arg(BG_MAIN)
    .arg(TEXT_PRIMARY)
    .arg(BG_SIDEBAR)
    .arg(BORDER_COLOR)
    .arg(ACCENT_BLUE)
    .arg(BORDER_COLOR)
    .arg(ACCENT_HOVER)
    .arg(BG_CARD)
    .arg(TEXT_SECONDARY);
}

} // namespace Theme
} // namespace AnshuBio
