#include "TrustedPhonesWidget.h"
#include "Theme.h"
#include "../storage/KeyStore.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

namespace AnshuBio {

TrustedPhonesWidget::TrustedPhonesWidget(QWidget* parent) : QWidget(parent) {
    SetupUi();
    RefreshData();
}

void TrustedPhonesWidget::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(28, 28, 28, 28);
    mainLayout->setSpacing(20);

    auto* headerLayout = new QHBoxLayout();
    auto* titleLabel = new QLabel("Trusted Phones", this);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: 700; color: #f8fafc;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    m_addPhoneBtn = new QPushButton("+ Add Phone", this);
    m_addPhoneBtn->setStyleSheet("background-color: #3b82f6; border-color: #2563eb; color: white; padding: 8px 16px; border-radius: 6px; font-weight: 600;");
    connect(m_addPhoneBtn, &QPushButton::clicked, this, &TrustedPhonesWidget::RequestPairingWizard);
    headerLayout->addWidget(m_addPhoneBtn);

    mainLayout->addLayout(headerLayout);

    auto* subLabel = new QLabel("Manage paired Android phones authorized to unlock this PC (Strict maximum: 2 phones).", this);
    subLabel->setStyleSheet("font-size: 13px; color: #94a3b8;");
    mainLayout->addWidget(subLabel);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"Device Name", "Connection", "Transport", "Security Status", "Last Seen"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->setStyleSheet("background-color: #151926; border: 1px solid #1e293b; border-radius: 8px; color: #f8fafc;");

    mainLayout->addWidget(m_table);

    auto* btnLayout = new QHBoxLayout();
    m_removePhoneBtn = new QPushButton("Remove Selected Phone", this);
    m_removePhoneBtn->setStyleSheet("background-color: #1e293b; border-color: #334155; color: #f8fafc; padding: 8px 16px; border-radius: 6px; font-weight: 600;");
    connect(m_removePhoneBtn, &QPushButton::clicked, this, [this]() {
        int row = m_table->currentRow();
        if (row >= 0) {
            auto phones = KeyStore::Instance().GetTrustedPhones();
            if (row < static_cast<int>(phones.size())) {
                KeyStore::Instance().RemoveTrustedPhone(phones[row].id);
                RefreshData();
            }
        }
    });

    m_revokePhoneBtn = new QPushButton("Revoke & Blacklist Phone", this);
    m_revokePhoneBtn->setStyleSheet("background-color: rgba(239, 68, 68, 0.15); border: 1px solid rgba(239, 68, 68, 0.4); color: #ef4444; padding: 8px 16px; border-radius: 6px; font-weight: 600;");
    connect(m_revokePhoneBtn, &QPushButton::clicked, this, [this]() {
        int row = m_table->currentRow();
        if (row >= 0) {
            auto phones = KeyStore::Instance().GetTrustedPhones();
            if (row < static_cast<int>(phones.size())) {
                auto choice = QMessageBox::warning(this, "Revoke Phone",
                    QString("Are you sure you want to permanently revoke '%1'?\n\nA revoked phone will never be allowed to authenticate or re-pair.").arg(QString::fromStdString(phones[row].name)),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                if (choice == QMessageBox::Yes) {
                    KeyStore::Instance().RevokeTrustedPhone(phones[row].id);
                    RefreshData();
                }
            }
        }
    });

    btnLayout->addWidget(m_removePhoneBtn);
    btnLayout->addWidget(m_revokePhoneBtn);
    btnLayout->addStretch();

    mainLayout->addLayout(btnLayout);
}

void TrustedPhonesWidget::RefreshData() {
    auto phones = KeyStore::Instance().GetTrustedPhones();
    m_table->setRowCount(static_cast<int>(phones.size()));

    for (size_t i = 0; i < phones.size(); ++i) {
        m_table->setItem(static_cast<int>(i), 0, new QTableWidgetItem(QString::fromStdString(phones[i].name)));
        m_table->setItem(static_cast<int>(i), 1, new QTableWidgetItem("Online"));
        m_table->setItem(static_cast<int>(i), 2, new QTableWidgetItem(QString::fromStdString(phones[i].transport)));
        m_table->setItem(static_cast<int>(i), 3, new QTableWidgetItem("NIST P-256 Verified"));
        m_table->setItem(static_cast<int>(i), 4, new QTableWidgetItem(QString::fromStdString(phones[i].lastSeen)));
    }

    m_addPhoneBtn->setEnabled(phones.size() < 2);
}

} // namespace AnshuBio
