#include "LogsWidget.h"
#include "Theme.h"
#include "../storage/SecurityLogger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QDateTime>

namespace AnshuBio {

LogsWidget::LogsWidget(QWidget* parent) : QWidget(parent) {
    SetupUi();
    RefreshData();

    // Listen to real-time log events
    SecurityLogger::Instance().AddListener([this](const LogEntry&) {
        // Post update to UI
        QMetaObject::invokeMethod(this, [this]() {
            RefreshData();
        }, Qt::QueuedConnection);
    });
}

void LogsWidget::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(28, 28, 28, 28);
    mainLayout->setSpacing(20);

    auto* headerLayout = new QHBoxLayout();
    auto* titleLabel = new QLabel("Security Audit Logs", this);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: 700; color: #f8fafc;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    m_refreshBtn = new QPushButton("Refresh Logs", this);
    m_refreshBtn->setStyleSheet("background-color: #1e293b; border-color: #334155; color: #f8fafc; padding: 8px 16px; border-radius: 6px; font-weight: 600;");
    connect(m_refreshBtn, &QPushButton::clicked, this, &LogsWidget::RefreshData);
    headerLayout->addWidget(m_refreshBtn);

    mainLayout->addLayout(headerLayout);

    auto* subLabel = new QLabel("Real-time chronological log of authentication challenges, session transitions, and pairing events.", this);
    subLabel->setStyleSheet("font-size: 13px; color: #94a3b8;");
    mainLayout->addWidget(subLabel);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({"Timestamp", "Level", "Event Tag", "Message"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->setStyleSheet("background-color: #151926; border: 1px solid #1e293b; border-radius: 8px; color: #f8fafc; font-family: monospace; font-size: 12px;");

    mainLayout->addWidget(m_table);
}

void LogsWidget::RefreshData() {
    auto logs = SecurityLogger::Instance().GetRecentLogs(100);
    m_table->setRowCount(static_cast<int>(logs.size()));

    for (size_t i = 0; i < logs.size(); ++i) {
        size_t rowIdx = logs.size() - 1 - i; // Newest on top
        const auto& entry = logs[i];

        QDateTime dt = QDateTime::fromMSecsSinceEpoch(entry.timestamp);
        auto* timeItem = new QTableWidgetItem(dt.toString("yyyy-MM-dd HH:mm:ss"));
        auto* levelItem = new QTableWidgetItem(QString::fromStdString(entry.level));
        auto* tagItem = new QTableWidgetItem(QString::fromStdString(entry.tag));
        auto* msgItem = new QTableWidgetItem(QString::fromStdString(entry.message));

        if (entry.level == "SECURITY") {
            levelItem->setForeground(QColor("#10b981"));
        } else if (entry.level == "WARN") {
            levelItem->setForeground(QColor("#f59e0b"));
        } else if (entry.level == "ERROR") {
            levelItem->setForeground(QColor("#ef4444"));
        } else {
            levelItem->setForeground(QColor("#3b82f6"));
        }

        m_table->setItem(static_cast<int>(rowIdx), 0, timeItem);
        m_table->setItem(static_cast<int>(rowIdx), 1, levelItem);
        m_table->setItem(static_cast<int>(rowIdx), 2, tagItem);
        m_table->setItem(static_cast<int>(rowIdx), 3, msgItem);
    }
}

} // namespace AnshuBio
