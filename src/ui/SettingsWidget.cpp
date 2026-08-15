#include "SettingsWidget.h"
#include "Theme.h"
#include "../storage/KeyStore.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QMessageBox>

namespace AnshuBio {

SettingsWidget::SettingsWidget(QWidget* parent) : QWidget(parent) {
    SetupUi();
    RefreshData();
}

void SettingsWidget::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(28, 28, 28, 28);
    mainLayout->setSpacing(20);

    auto* titleLabel = new QLabel("Settings", this);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: 700; color: #f8fafc;");
    mainLayout->addWidget(titleLabel);

    // Section 1: Device Name
    auto* nameFrame = new QFrame(this);
    nameFrame->setStyleSheet("background-color: #151926; border: 1px solid #1e293b; border-radius: 10px; padding: 20px;");
    auto* nameLayout = new QVBoxLayout(nameFrame);

    auto* nameTitle = new QLabel("PC DISPLAY NAME", nameFrame);
    nameTitle->setStyleSheet("font-size: 11px; font-weight: 700; color: #64748b; letter-spacing: 0.5px;");
    auto* nameDesc = new QLabel("Custom name presented to Android companion apps during discovery and pairing (does not modify Windows computer name).", nameFrame);
    nameDesc->setStyleSheet("font-size: 12px; color: #94a3b8; margin-top: 4px; margin-bottom: 8px;");

    m_pcDisplayNameEdit = new QLineEdit(nameFrame);
    m_pcDisplayNameEdit->setPlaceholderText("Anshu-PC");

    nameLayout->addWidget(nameTitle);
    nameLayout->addWidget(nameDesc);
    nameLayout->addWidget(m_pcDisplayNameEdit);
    mainLayout->addWidget(nameFrame);

    // Section 2: General & Security Preferences
    auto* prefsFrame = new QFrame(this);
    prefsFrame->setStyleSheet("background-color: #151926; border: 1px solid #1e293b; border-radius: 10px; padding: 20px;");
    auto* prefsLayout = new QVBoxLayout(prefsFrame);

    auto* prefsTitle = new QLabel("SYSTEM & SECURITY PREFERENCES", prefsFrame);
    prefsTitle->setStyleSheet("font-size: 11px; font-weight: 700; color: #64748b; letter-spacing: 0.5px; margin-bottom: 12px;");
    prefsLayout->addWidget(prefsTitle);

    m_startWithWindowsCheck = new QCheckBox("Start AnshuBio Unlock automatically with Windows", prefsFrame);
    m_protectionEnabledCheck = new QCheckBox("Enable Phone Biometric Unlock Protection", prefsFrame);
    m_soundCheck = new QCheckBox("Play notification sound on phone authentication request", prefsFrame);
    m_vibrationCheck = new QCheckBox("Vibrate on phone authentication request", prefsFrame);

    prefsLayout->addWidget(m_startWithWindowsCheck);
    prefsLayout->addWidget(m_protectionEnabledCheck);
    prefsLayout->addWidget(m_soundCheck);
    prefsLayout->addWidget(m_vibrationCheck);

    mainLayout->addWidget(prefsFrame);

    // Save Button
    m_saveBtn = new QPushButton("Save Settings", this);
    m_saveBtn->setStyleSheet("background-color: #3b82f6; border-color: #2563eb; color: white; padding: 10px 24px; border-radius: 6px; font-weight: 600; width: 160px;");
    connect(m_saveBtn, &QPushButton::clicked, this, [this]() {
        auto settings = KeyStore::Instance().GetSettings();
        settings.pcDisplayName = m_pcDisplayNameEdit->text().toStdString();
        settings.startWithWindows = m_startWithWindowsCheck->isChecked();
        settings.protectionEnabled = m_protectionEnabledCheck->isChecked();
        settings.soundEnabled = m_soundCheck->isChecked();
        settings.vibrationEnabled = m_vibrationCheck->isChecked();

        KeyStore::Instance().UpdateSettings(settings);
        QMessageBox::information(this, "Settings", "Settings saved successfully.");
    });

    mainLayout->addWidget(m_saveBtn);
    mainLayout->addStretch();
}

void SettingsWidget::RefreshData() {
    auto settings = KeyStore::Instance().GetSettings();
    m_pcDisplayNameEdit->setText(QString::fromStdString(KeyStore::Instance().GetPcDisplayName()));
    m_startWithWindowsCheck->setChecked(settings.startWithWindows);
    m_protectionEnabledCheck->setChecked(settings.protectionEnabled);
    m_soundCheck->setChecked(settings.soundEnabled);
    m_vibrationCheck->setChecked(settings.vibrationEnabled);
}

} // namespace AnshuBio
