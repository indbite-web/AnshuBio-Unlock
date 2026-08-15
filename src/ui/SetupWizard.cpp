#include "SetupWizard.h"
#include "Theme.h"
#include "../storage/KeyStore.h"
#include "../core/AuthCoordinator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

namespace AnshuBio {

SetupWizard::SetupWizard(QWidget* parent) : QDialog(parent) {
    setWindowTitle("AnshuBio Unlock — Setup Wizard");
    setFixedSize(620, 520);
    setStyleSheet(Theme::GetMainStyleSheet());
    SetupUi();
}

void SetupWizard::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->addWidget(CreateWelcomeStep());
    m_stackedWidget->addWidget(CreateIdentityStep());
    m_stackedWidget->addWidget(CreatePairingStep());
    m_stackedWidget->addWidget(CreateConfirmationStep());
    m_stackedWidget->addWidget(CreateCompleteStep());

    mainLayout->addWidget(m_stackedWidget);
}

QWidget* SetupWizard::CreateWelcomeStep() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(16);

    auto* title = new QLabel("Welcome to AnshuBio Unlock", page);
    title->setStyleSheet("font-size: 22px; font-weight: 700; color: #3b82f6;");

    auto* desc = new QLabel(
        "AnshuBio Unlock allows you to unlock your Windows PC instantly using your trusted Android phone's native fingerprint or face biometric.\n\n"
        "• 100% Offline (Local Wi-Fi LAN & Bluetooth RFCOMM)\n"
        "• Zero biometric template or password transmission\n"
        "• Standard Windows Password and PIN login always remain available\n\n"
        "Let's get your PC set up in a few simple steps.", page);
    desc->setWordWrap(true);
    desc->setStyleSheet("font-size: 13px; color: #94a3b8; line-height: 1.6;");

    auto* nextBtn = new QPushButton("Get Started →", page);
    nextBtn->setStyleSheet("background-color: #3b82f6; border-color: #2563eb; color: white; padding: 10px 24px; border-radius: 6px; font-weight: 600; width: 140px;");
    connect(nextBtn, &QPushButton::clicked, this, [this]() {
        m_stackedWidget->setCurrentIndex(1);
    });

    layout->addWidget(title);
    layout->addWidget(desc);
    layout->addStretch();
    layout->addWidget(nextBtn, 0, Qt::AlignRight);
    return page;
}

QWidget* SetupWizard::CreateIdentityStep() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(16);

    auto* title = new QLabel("Step 1: Set PC Display Name", page);
    title->setStyleSheet("font-size: 20px; font-weight: 700; color: #f8fafc;");

    auto* desc = new QLabel("Choose a friendly name for this PC. This is how it will appear in the AnshuBio Android companion app.", page);
    desc->setWordWrap(true);
    desc->setStyleSheet("font-size: 13px; color: #94a3b8;");

    m_nameEdit = new QLineEdit(page);
    m_nameEdit->setText(QString::fromStdString(KeyStore::Instance().GetPcDisplayName()));

    auto* nextBtn = new QPushButton("Next: Pair Phone →", page);
    nextBtn->setStyleSheet("background-color: #3b82f6; border-color: #2563eb; color: white; padding: 10px 24px; border-radius: 6px; font-weight: 600;");
    connect(nextBtn, &QPushButton::clicked, this, [this]() {
        KeyStore::Instance().SetPcDisplayName(m_nameEdit->text().toStdString());
        auto session = AuthCoordinator::Instance().InitiatePairingSession();
        if (session.has_value()) {
            m_sessionId = session->sessionId;
            m_codeLabel->setText(QString::fromStdString(session->confirmCode));
        }
        m_stackedWidget->setCurrentIndex(2);
    });

    layout->addWidget(title);
    layout->addWidget(desc);
    layout->addWidget(m_nameEdit);
    layout->addStretch();
    layout->addWidget(nextBtn, 0, Qt::AlignRight);
    return page;
}

QWidget* SetupWizard::CreatePairingStep() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(16);

    auto* title = new QLabel("Step 2: Connect Phone", page);
    title->setStyleSheet("font-size: 20px; font-weight: 700; color: #f8fafc;");

    auto* desc = new QLabel("Open the AnshuBio Android app, tap '+ Add PC', and scan this PC on your local Wi-Fi or Bluetooth.", page);
    desc->setWordWrap(true);
    desc->setStyleSheet("font-size: 13px; color: #94a3b8;");

    auto* codeCard = new QFrame(page);
    codeCard->setStyleSheet("background-color: #151926; border: 1px solid #1e293b; border-radius: 10px; padding: 20px;");
    auto* codeLayout = new QVBoxLayout(codeCard);

    auto* codeTitle = new QLabel("PAIRING CONFIRMATION CODE", codeCard);
    codeTitle->setStyleSheet("font-size: 11px; font-weight: 700; color: #64748b; letter-spacing: 0.5px;");
    m_codeLabel = new QLabel("------", codeCard);
    m_codeLabel->setStyleSheet("font-size: 32px; font-weight: 800; color: #3b82f6; font-family: monospace; letter-spacing: 4px; margin-top: 8px;");

    codeLayout->addWidget(codeTitle);
    codeLayout->addWidget(m_codeLabel, 0, Qt::AlignCenter);

    auto* nextBtn = new QPushButton("I See the Code on Phone →", page);
    nextBtn->setStyleSheet("background-color: #3b82f6; border-color: #2563eb; color: white; padding: 10px 24px; border-radius: 6px; font-weight: 600;");
    connect(nextBtn, &QPushButton::clicked, this, [this]() {
        m_stackedWidget->setCurrentIndex(3);
    });

    layout->addWidget(title);
    layout->addWidget(desc);
    layout->addWidget(codeCard);
    layout->addStretch();
    layout->addWidget(nextBtn, 0, Qt::AlignRight);
    return page;
}

QWidget* SetupWizard::CreateConfirmationStep() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(16);

    auto* title = new QLabel("Step 3: Confirm Pairing", page);
    title->setStyleSheet("font-size: 20px; font-weight: 700; color: #f8fafc;");

    auto* desc = new QLabel("Verify that the 6-digit confirmation code matches on both your PC and your phone screen. Click 'Confirm Pairing' below to authorize the device.", page);
    desc->setWordWrap(true);
    desc->setStyleSheet("font-size: 13px; color: #94a3b8;");

    auto* confirmBtn = new QPushButton("Confirm Pairing", page);
    confirmBtn->setStyleSheet("background-color: #10b981; border-color: #059669; color: white; padding: 12px 28px; border-radius: 6px; font-weight: 700;");
    connect(confirmBtn, &QPushButton::clicked, this, [this]() {
        AuthCoordinator::Instance().ConfirmPairingFromPc(m_sessionId);
        m_stackedWidget->setCurrentIndex(4);
    });

    layout->addWidget(title);
    layout->addWidget(desc);
    layout->addStretch();
    layout->addWidget(confirmBtn, 0, Qt::AlignCenter);
    layout->addStretch();
    return page;
}

QWidget* SetupWizard::CreateCompleteStep() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(16);

    auto* title = new QLabel("Setup Complete!", page);
    title->setStyleSheet("font-size: 22px; font-weight: 700; color: #10b981;");

    auto* desc = new QLabel(
        "Your Android phone is now securely paired with AnshuBio Unlock.\n\n"
        "Next time you lock your PC (Win + L), simply unlock your phone with your fingerprint or face to automatically unlock Windows.", page);
    desc->setWordWrap(true);
    desc->setStyleSheet("font-size: 13px; color: #94a3b8; line-height: 1.6;");

    auto* finishBtn = new QPushButton("Finish & Open Dashboard", page);
    finishBtn->setStyleSheet("background-color: #3b82f6; border-color: #2563eb; color: white; padding: 10px 24px; border-radius: 6px; font-weight: 600;");
    connect(finishBtn, &QPushButton::clicked, this, [this]() {
        auto settings = KeyStore::Instance().GetSettings();
        settings.firstLaunchComplete = true;
        KeyStore::Instance().UpdateSettings(settings);
        emit WizardFinished();
        accept();
    });

    layout->addWidget(title);
    layout->addWidget(desc);
    layout->addStretch();
    layout->addWidget(finishBtn, 0, Qt::AlignRight);
    return page;
}

} // namespace AnshuBio
