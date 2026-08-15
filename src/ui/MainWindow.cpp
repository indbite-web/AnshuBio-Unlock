#include "MainWindow.h"
#include "Theme.h"
#include "DashboardWidget.h"
#include "TrustedPhonesWidget.h"
#include "SecurityWidget.h"
#include "SettingsWidget.h"
#include "LogsWidget.h"
#include "AboutWidget.h"
#include "SetupWizard.h"
#include "../session/SessionMonitor.h"
#include "../storage/KeyStore.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QCloseEvent>
#include <QApplication>

namespace AnshuBio {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("AnshuBio Unlock");
    resize(980, 680);
    setMinimumSize(850, 580);
    setStyleSheet(Theme::GetMainStyleSheet());

    SetupUi();
    SetupTray();

    // Check if first launch wizard needed
    auto settings = KeyStore::Instance().GetSettings();
    if (!settings.firstLaunchComplete && KeyStore::Instance().GetTrustedPhones().empty()) {
        auto* wizard = new SetupWizard(this);
        connect(wizard, &SetupWizard::WizardFinished, this, [this]() {
            m_dashboardWidget->RefreshData();
            m_trustedPhonesWidget->RefreshData();
        });
        wizard->show();
    }
}

MainWindow::~MainWindow() {}

void MainWindow::SetupUi() {
    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // 1. Sidebar
    auto* sidebar = new QFrame(this);
    sidebar->setFixedWidth(230);
    sidebar->setStyleSheet("background-color: #080a10; border-right: 1px solid #1e293b;");
    auto* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(16, 24, 16, 24);
    sidebarLayout->setSpacing(8);

    // App Branding in sidebar
    auto* brandLayout = new QHBoxLayout();
    auto* brandTitle = new QLabel("AnshuBio", sidebar);
    brandTitle->setStyleSheet("font-size: 18px; font-weight: 800; color: #3b82f6; letter-spacing: -0.5px;");
    auto* brandSub = new QLabel("Unlock", sidebar);
    brandSub->setStyleSheet("font-size: 18px; font-weight: 800; color: #f8fafc; letter-spacing: -0.5px;");
    brandLayout->addWidget(brandTitle);
    brandLayout->addWidget(brandSub);
    brandLayout->addStretch();
    sidebarLayout->addLayout(brandLayout);

    auto* versionLabel = new QLabel("v1.0.0 (Native)", sidebar);
    versionLabel->setStyleSheet("font-size: 11px; color: #64748b; margin-bottom: 20px;");
    sidebarLayout->addWidget(versionLabel);

    // Nav Buttons
    const std::vector<std::string> navItems = {
        "Dashboard",
        "Trusted Phones",
        "Security & Vault",
        "Settings",
        "Audit Logs",
        "About"
    };

    for (size_t i = 0; i < navItems.size(); ++i) {
        auto* btn = new QPushButton(QString::fromStdString(navItems[i]), sidebar);
        btn->setCheckable(true);
        btn->setAutoExclusive(true);
        btn->setStyleSheet(R"(
            QPushButton {
                background-color: transparent;
                border: 1px solid transparent;
                border-radius: 6px;
                padding: 10px 14px;
                text-align: left;
                font-weight: 600;
                color: #94a3b8;
            }
            QPushButton:hover {
                background-color: #151926;
                color: #f8fafc;
            }
            QPushButton:checked {
                background-color: #1e293b;
                color: #3b82f6;
                border-left: 3px solid #3b82f6;
            }
        )");

        connect(btn, &QPushButton::clicked, this, [this, i]() {
            SwitchTab(static_cast<int>(i));
        });

        m_navButtons.push_back(btn);
        sidebarLayout->addWidget(btn);
    }

    sidebarLayout->addStretch();
    rootLayout->addWidget(sidebar);

    // 2. Main Content Area (Stacked Widgets)
    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->setStyleSheet("background-color: #0c0f17;");

    m_dashboardWidget = new DashboardWidget(this);
    m_trustedPhonesWidget = new TrustedPhonesWidget(this);
    m_securityWidget = new SecurityWidget(this);
    m_settingsWidget = new SettingsWidget(this);
    m_logsWidget = new LogsWidget(this);
    m_aboutWidget = new AboutWidget(this);

    connect(m_trustedPhonesWidget, &TrustedPhonesWidget::RequestPairingWizard, this, [this]() {
        auto* wizard = new SetupWizard(this);
        connect(wizard, &SetupWizard::WizardFinished, this, [this]() {
            m_dashboardWidget->RefreshData();
            m_trustedPhonesWidget->RefreshData();
        });
        wizard->exec();
    });

    m_stackedWidget->addWidget(m_dashboardWidget);
    m_stackedWidget->addWidget(m_trustedPhonesWidget);
    m_stackedWidget->addWidget(m_securityWidget);
    m_stackedWidget->addWidget(m_settingsWidget);
    m_stackedWidget->addWidget(m_logsWidget);
    m_stackedWidget->addWidget(m_aboutWidget);

    rootLayout->addWidget(m_stackedWidget);

    // Select first tab
    if (!m_navButtons.empty()) {
        m_navButtons[0]->setChecked(true);
        SwitchTab(0);
    }
}

void MainWindow::SwitchTab(int index) {
    m_stackedWidget->setCurrentIndex(index);
    for (size_t i = 0; i < m_navButtons.size(); ++i) {
        m_navButtons[i]->setChecked(static_cast<int>(i) == index);
    }

    if (index == 0 && m_dashboardWidget) m_dashboardWidget->RefreshData();
    if (index == 1 && m_trustedPhonesWidget) m_trustedPhonesWidget->RefreshData();
    if (index == 2 && m_securityWidget) m_securityWidget->RefreshData();
    if (index == 3 && m_settingsWidget) m_settingsWidget->RefreshData();
    if (index == 4 && m_logsWidget) m_logsWidget->RefreshData();
}

void MainWindow::SetupTray() {
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setToolTip("AnshuBio Unlock");

    m_trayMenu = new QMenu(this);
    auto* showAction = m_trayMenu->addAction("Open Dashboard");
    connect(showAction, &QAction::triggered, this, [this]() {
        showNormal();
        activateWindow();
    });

    auto* lockAction = m_trayMenu->addAction("Lock PC Now");
    connect(lockAction, &QAction::triggered, this, []() {
        SessionMonitor::Instance().LockWorkStation();
    });

    m_trayMenu->addSeparator();

    auto* quitAction = m_trayMenu->addAction("Quit");
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            showNormal();
            activateWindow();
        }
    });
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}

} // namespace AnshuBio
