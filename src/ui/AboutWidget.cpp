#include "AboutWidget.h"
#include "Theme.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QMessageBox>

namespace AnshuBio {

AboutWidget::AboutWidget(QWidget* parent) : QWidget(parent) {
    SetupUi();
}

void AboutWidget::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(28, 28, 28, 28);
    mainLayout->setSpacing(20);

    auto* titleLabel = new QLabel("About AnshuBio Unlock", this);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: 700; color: #f8fafc;");
    mainLayout->addWidget(titleLabel);

    auto* card = new QFrame(this);
    card->setStyleSheet("background-color: #151926; border: 1px solid #1e293b; border-radius: 10px; padding: 24px;");
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setSpacing(12);

    auto* nameLabel = new QLabel("AnshuBio Unlock", card);
    nameLabel->setStyleSheet("font-size: 20px; font-weight: 700; color: #3b82f6;");

    auto* versionLabel = new QLabel("Version 1.0.0 (Native C++ / Qt 6 Release)", card);
    versionLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #f8fafc;");

    auto* publisherLabel = new QLabel("Publisher: AnshuCore\nApplication Identity: com.anshucore.bio\nPlatform: Windows 10 & 11 (x64) Native", card);
    publisherLabel->setStyleSheet("font-size: 13px; color: #94a3b8; line-height: 1.6;");

    auto* descLabel = new QLabel(
        "AnshuBio Unlock is an offline biometric PC authentication and Windows unlock suite. "
        "It pairs your Windows workstation with trusted Android phones using local Wi-Fi LAN and Bluetooth RFCOMM. "
        "Biometric templates and passwords never leave their native secure subsystems.", card);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("font-size: 13px; color: #94a3b8; line-height: 1.5; margin-top: 8px;");

    auto* updateBtn = new QPushButton("Check for Updates", card);
    updateBtn->setStyleSheet("background-color: #1e293b; border: 1px solid #334155; color: #f8fafc; padding: 8px 16px; border-radius: 6px; font-weight: 600; width: 160px; margin-top: 12px;");
    connect(updateBtn, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, "Update", "You are using the latest version of AnshuBio Unlock (v1.0.0).");
    });

    cardLayout->addWidget(nameLabel);
    cardLayout->addWidget(versionLabel);
    cardLayout->addWidget(publisherLabel);
    cardLayout->addWidget(descLabel);
    cardLayout->addWidget(updateBtn);

    mainLayout->addWidget(card);
    mainLayout->addStretch();
}

} // namespace AnshuBio
