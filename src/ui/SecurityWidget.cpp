#include "SecurityWidget.h"
#include "Theme.h"
#include "../storage/KeyStore.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QMessageBox>
#include <windows.h>

namespace AnshuBio {

SecurityWidget::SecurityWidget(QWidget* parent) : QWidget(parent) {
    SetupUi();
    RefreshData();
}

void SecurityWidget::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(28, 28, 28, 28);
    mainLayout->setSpacing(20);

    auto* titleLabel = new QLabel("Security & Cryptography", this);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: 700; color: #f8fafc;");
    mainLayout->addWidget(titleLabel);

    // Cryptographic Status Frame
    auto* cryptoFrame = new QFrame(this);
    cryptoFrame->setStyleSheet("background-color: #151926; border: 1px solid #1e293b; border-radius: 10px; padding: 20px;");
    auto* cryptoLayout = new QVBoxLayout(cryptoFrame);

    auto* cryptoTitle = new QLabel("LOCAL CRYPTOGRAPHIC IDENTITY", cryptoFrame);
    cryptoTitle->setStyleSheet("font-size: 11px; font-weight: 700; color: #64748b; letter-spacing: 0.5px;");
    
    auto* algoLabel = new QLabel("Algorithm: NIST P-256 ECDSA / SHA-256 (Hardware CSPRNG Nonces)", cryptoFrame);
    algoLabel->setStyleSheet("font-size: 14px; font-weight: 600; color: #f8fafc; margin-top: 4px;");

    m_fingerprintLabel = new QLabel("Fingerprint: Loading...", cryptoFrame);
    m_fingerprintLabel->setStyleSheet("font-size: 13px; font-family: monospace; color: #3b82f6; margin-top: 2px;");

    m_vaultStatusLabel = new QLabel("Vault: Machine-bound AES-256-GCM (DPAPI Protected)", cryptoFrame);
    m_vaultStatusLabel->setStyleSheet("font-size: 13px; color: #10b981; margin-top: 2px;");

    cryptoLayout->addWidget(cryptoTitle);
    cryptoLayout->addWidget(algoLabel);
    cryptoLayout->addWidget(m_fingerprintLabel);
    cryptoLayout->addWidget(m_vaultStatusLabel);
    mainLayout->addWidget(cryptoFrame);

    // Windows Credential Vault Frame
    auto* credFrame = new QFrame(this);
    credFrame->setStyleSheet("background-color: #151926; border: 1px solid #1e293b; border-radius: 10px; padding: 20px;");
    auto* credLayout = new QVBoxLayout(credFrame);

    auto* credTitle = new QLabel("WINDOWS LOGON BRIDGING CREDENTIAL", credFrame);
    credTitle->setStyleSheet("font-size: 11px; font-weight: 700; color: #64748b; letter-spacing: 0.5px;");
    auto* credDesc = new QLabel("Store machine-bound encrypted credentials in DPAPI memory to authorize Windows LogonUI upon verified phone biometric confirmation.", credFrame);
    credDesc->setStyleSheet("font-size: 12px; color: #94a3b8; margin-top: 4px; margin-bottom: 12px;");

    credLayout->addWidget(credTitle);
    credLayout->addWidget(credDesc);

    auto* inputGrid = new QHBoxLayout();
    m_domainEdit = new QLineEdit(this);
    m_domainEdit->setPlaceholderText("Domain / Computer Name");
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("Windows Username");
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText("Windows Account Password");
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    inputGrid->addWidget(m_domainEdit);
    inputGrid->addWidget(m_usernameEdit);
    inputGrid->addWidget(m_passwordEdit);
    credLayout->addLayout(inputGrid);

    auto* credBtnLayout = new QHBoxLayout();
    m_saveCredBtn = new QPushButton("Save Machine Credential", this);
    m_saveCredBtn->setStyleSheet("background-color: #3b82f6; border-color: #2563eb; color: white; padding: 8px 16px; border-radius: 6px; font-weight: 600; margin-top: 8px;");
    connect(m_saveCredBtn, &QPushButton::clicked, this, [this]() {
        std::wstring u = m_usernameEdit->text().toStdWString();
        std::wstring d = m_domainEdit->text().toStdWString();
        std::wstring p = m_passwordEdit->text().toStdWString();
        if (u.empty() || p.empty()) {
            QMessageBox::warning(this, "Validation", "Username and Password cannot be empty.");
            return;
        }
        KeyStore::Instance().SetWindowsCredential(u, d, p);
        m_passwordEdit->clear();
        RefreshData();
        QMessageBox::information(this, "Security", "Windows credential saved to machine-bound DPAPI vault.");
    });

    m_clearCredBtn = new QPushButton("Clear Credential", this);
    m_clearCredBtn->setStyleSheet("background-color: #1e293b; border-color: #334155; color: #f8fafc; padding: 8px 16px; border-radius: 6px; font-weight: 600; margin-top: 8px;");
    connect(m_clearCredBtn, &QPushButton::clicked, this, [this]() {
        KeyStore::Instance().ClearWindowsCredential();
        m_passwordEdit->clear();
        RefreshData();
        QMessageBox::information(this, "Security", "Windows credentials cleared from DPAPI vault.");
    });

    credBtnLayout->addWidget(m_saveCredBtn);
    credBtnLayout->addWidget(m_clearCredBtn);
    credBtnLayout->addStretch();
    credLayout->addLayout(credBtnLayout);

    mainLayout->addWidget(credFrame);
    mainLayout->addStretch();
}

void SecurityWidget::RefreshData() {
    auto identity = KeyStore::Instance().GetPcIdentity();
    m_fingerprintLabel->setText("Fingerprint: " + QString::fromStdString(identity.fingerprint));

    auto cred = KeyStore::Instance().GetWindowsCredential();
    if (cred.has_value() && !cred->username.empty()) {
        m_usernameEdit->setText(QString::fromStdWString(cred->username));
        m_domainEdit->setText(QString::fromStdWString(cred->domain));
        m_vaultStatusLabel->setText("Vault: Machine-bound AES-256-GCM (DPAPI Protected - Credential Active)");
        m_vaultStatusLabel->setStyleSheet("font-size: 13px; color: #10b981; margin-top: 2px;");
    } else {
        // Auto-populate default machine name & user name if unset
        wchar_t compName[MAX_COMPUTERNAME_LENGTH + 1] = { 0 };
        DWORD compSize = ARRAYSIZE(compName);
        GetComputerNameW(compName, &compSize);

        wchar_t userName[256] = { 0 };
        DWORD userSize = ARRAYSIZE(userName);
        GetUserNameW(userName, &userSize);

        if (m_domainEdit->text().isEmpty()) m_domainEdit->setText(QString::fromWCharArray(compName));
        if (m_usernameEdit->text().isEmpty()) m_usernameEdit->setText(QString::fromWCharArray(userName));

        m_vaultStatusLabel->setText("Vault: Machine-bound AES-256-GCM (DPAPI Protected - Awaiting Credential)");
        m_vaultStatusLabel->setStyleSheet("font-size: 13px; color: #f59e0b; margin-top: 2px;");
    }
}

} // namespace AnshuBio
