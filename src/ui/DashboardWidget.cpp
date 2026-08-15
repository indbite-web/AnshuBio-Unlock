#include "DashboardWidget.h"
#include "Theme.h"
#include "../storage/KeyStore.h"
#include "../session/SessionMonitor.h"
#include "../storage/SecurityLogger.h"
#include "../networking/bluetooth/BluetoothHelper.h"
#include <QFrame>
#include <QGridLayout>
#include <windows.h>

namespace AnshuBio {

DashboardWidget::DashboardWidget(QWidget* parent) : QWidget(parent) {
    SetupUi();
    RefreshData();
}

void DashboardWidget::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(28, 28, 28, 28);
    mainLayout->setSpacing(20);

    // Header section
    auto* headerLayout = new QHBoxLayout();
    auto* titleLabel = new QLabel("Dashboard", this);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: 700; color: #f8fafc;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    m_protectionBadge = new QLabel("Protected", this);
    m_protectionBadge->setStyleSheet("background-color: rgba(16, 185, 129, 0.15); color: #10b981; border: 1px solid rgba(16, 185, 129, 0.3); border-radius: 12px; padding: 4px 12px; font-weight: 600;");
    headerLayout->addWidget(m_protectionBadge);

    mainLayout->addLayout(headerLayout);

    // Status Cards Grid
    auto* gridLayout = new QGridLayout();
    gridLayout->setSpacing(16);

    // Card 1: PC Identity
    auto* pcCard = new QFrame(this);
    pcCard->setStyleSheet("background-color: #151926; border: 1px solid #1e293b; border-radius: 10px; padding: 16px;");
    auto* pcCardLayout = new QVBoxLayout(pcCard);
    auto* pcTitle = new QLabel("PC DISPLAY NAME", pcCard);
    pcTitle->setStyleSheet("font-size: 11px; font-weight: 700; color: #64748b; letter-spacing: 0.5px;");
    m_pcNameLabel = new QLabel("Anshu-PC", pcCard);
    m_pcNameLabel->setStyleSheet("font-size: 20px; font-weight: 700; color: #f8fafc; margin-top: 4px;");
    auto* pcDesc = new QLabel("Windows 11 Companion", pcCard);
    pcDesc->setStyleSheet("font-size: 12px; color: #94a3b8;");
    pcCardLayout->addWidget(pcTitle);
    pcCardLayout->addWidget(m_pcNameLabel);
    pcCardLayout->addWidget(pcDesc);
    gridLayout->addWidget(pcCard, 0, 0);

    // Card 2: Windows State
    auto* stateCard = new QFrame(this);
    stateCard->setStyleSheet("background-color: #151926; border: 1px solid #1e293b; border-radius: 10px; padding: 16px;");
    auto* stateCardLayout = new QVBoxLayout(stateCard);
    auto* stateTitle = new QLabel("WINDOWS STATE", stateCard);
    stateTitle->setStyleSheet("font-size: 11px; font-weight: 700; color: #64748b; letter-spacing: 0.5px;");
    m_osStateLabel = new QLabel("Unlocked", stateCard);
    m_osStateLabel->setStyleSheet("font-size: 20px; font-weight: 700; color: #10b981; margin-top: 4px;");
    auto* stateDesc = new QLabel("Native WTS session verified", stateCard);
    stateDesc->setStyleSheet("font-size: 12px; color: #94a3b8;");
    stateCardLayout->addWidget(stateTitle);
    stateCardLayout->addWidget(m_osStateLabel);
    stateCardLayout->addWidget(stateDesc);
    gridLayout->addWidget(stateCard, 0, 1);

    // Card 3: Network & Bluetooth
    auto* netCard = new QFrame(this);
    netCard->setStyleSheet("background-color: #151926; border: 1px solid #1e293b; border-radius: 10px; padding: 16px;");
    auto* netCardLayout = new QVBoxLayout(netCard);
    auto* netTitle = new QLabel("OFFLINE TRANSPORTS", netCard);
    netTitle->setStyleSheet("font-size: 11px; font-weight: 700; color: #64748b; letter-spacing: 0.5px;");
    m_wifiStatusLabel = new QLabel("Wi-Fi: LAN Active (Port 42425)", netCard);
    m_wifiStatusLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #3b82f6; margin-top: 4px;");
    m_btStatusLabel = new QLabel("Bluetooth: Standby", netCard);
    m_btStatusLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #94a3b8;");
    netCardLayout->addWidget(netTitle);
    netCardLayout->addWidget(m_wifiStatusLabel);
    netCardLayout->addWidget(m_btStatusLabel);
    gridLayout->addWidget(netCard, 1, 0);

    // Card 4: Trusted Phones & Power
    auto* phonesCard = new QFrame(this);
    phonesCard->setStyleSheet("background-color: #151926; border: 1px solid #1e293b; border-radius: 10px; padding: 16px;");
    auto* phonesCardLayout = new QVBoxLayout(phonesCard);
    auto* phonesTitle = new QLabel("TRUSTED PHONES & POWER", phonesCard);
    phonesTitle->setStyleSheet("font-size: 11px; font-weight: 700; color: #64748b; letter-spacing: 0.5px;");
    m_phonesCountLabel = new QLabel("0 / 2 Paired", phonesCard);
    m_phonesCountLabel->setStyleSheet("font-size: 20px; font-weight: 700; color: #f8fafc; margin-top: 4px;");
    m_powerStatusLabel = new QLabel("Power: AC Power", phonesCard);
    m_powerStatusLabel->setStyleSheet("font-size: 12px; color: #94a3b8;");
    phonesCardLayout->addWidget(phonesTitle);
    phonesCardLayout->addWidget(m_phonesCountLabel);
    phonesCardLayout->addWidget(m_powerStatusLabel);
    gridLayout->addWidget(phonesCard, 1, 1);

    mainLayout->addLayout(gridLayout);

    // Action buttons section
    auto* actionsLayout = new QHBoxLayout();
    m_manualLockBtn = new QPushButton("Lock PC Now", this);
    m_manualLockBtn->setStyleSheet("background-color: #ef4444; border-color: #dc2626; color: white; padding: 10px 20px; border-radius: 6px; font-weight: 600;");
    connect(m_manualLockBtn, &QPushButton::clicked, this, []() {
        SessionMonitor::Instance().LockWorkStation();
    });

    m_toggleProtectionBtn = new QPushButton("Pause Protection", this);
    m_toggleProtectionBtn->setStyleSheet("background-color: #1e293b; border-color: #334155; color: #f8fafc; padding: 10px 20px; border-radius: 6px; font-weight: 600;");
    connect(m_toggleProtectionBtn, &QPushButton::clicked, this, [this]() {
        auto settings = KeyStore::Instance().GetSettings();
        settings.protectionEnabled = !settings.protectionEnabled;
        KeyStore::Instance().UpdateSettings(settings);
        RefreshData();
    });

    actionsLayout->addWidget(m_manualLockBtn);
    actionsLayout->addWidget(m_toggleProtectionBtn);
    actionsLayout->addStretch();

    mainLayout->addLayout(actionsLayout);
    mainLayout->addStretch();
}

void DashboardWidget::RefreshData() {
    auto displayName = KeyStore::Instance().GetPcDisplayName();
    m_pcNameLabel->setText(QString::fromStdString(displayName));

    auto settings = KeyStore::Instance().GetSettings();
    if (settings.protectionEnabled) {
        m_protectionBadge->setText("Protected");
        m_protectionBadge->setStyleSheet("background-color: rgba(16, 185, 129, 0.15); color: #10b981; border: 1px solid rgba(16, 185, 129, 0.3); border-radius: 12px; padding: 4px 12px; font-weight: 600;");
        m_toggleProtectionBtn->setText("Pause Protection");
    } else {
        m_protectionBadge->setText("Disabled");
        m_protectionBadge->setStyleSheet("background-color: rgba(239, 68, 68, 0.15); color: #ef4444; border: 1px solid rgba(239, 68, 68, 0.3); border-radius: 12px; padding: 4px 12px; font-weight: 600;");
        m_toggleProtectionBtn->setText("Enable Protection");
    }

    bool isLocked = SessionMonitor::Instance().IsLocked();
    if (isLocked) {
        m_osStateLabel->setText("Locked");
        m_osStateLabel->setStyleSheet("font-size: 20px; font-weight: 700; color: #f59e0b; margin-top: 4px;");
    } else {
        m_osStateLabel->setText("Unlocked");
        m_osStateLabel->setStyleSheet("font-size: 20px; font-weight: 700; color: #10b981; margin-top: 4px;");
    }

    // Transports status
    m_wifiStatusLabel->setText("Wi-Fi: LAN Active (Port 42425)");
    bool btAvail = BluetoothHelper::IsBluetoothRadioAvailable();
    if (btAvail) {
        m_btStatusLabel->setText("Bluetooth: Available (Standby)");
        m_btStatusLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #3b82f6;");
    } else {
        m_btStatusLabel->setText("Bluetooth: Radio Off / Unavailable");
        m_btStatusLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #64748b;");
    }

    // Power status
    SYSTEM_POWER_STATUS sps;
    if (GetSystemPowerStatus(&sps)) {
        if (sps.ACLineStatus == 1) {
            m_powerStatusLabel->setText("Power: AC Power Online");
        } else if (sps.BatteryLifePercent != 255) {
            m_powerStatusLabel->setText(QString("Power: Battery %1%").arg(sps.BatteryLifePercent));
        } else {
            m_powerStatusLabel->setText("Power: Battery Operating");
        }
    }

    auto phones = KeyStore::Instance().GetTrustedPhones();
    m_phonesCountLabel->setText(QString("%1 / 2 Paired").arg(phones.size()));
}

} // namespace AnshuBio
