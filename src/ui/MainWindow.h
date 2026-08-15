#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QSystemTrayIcon>
#include <QMenu>
#include <vector>

namespace AnshuBio {

class DashboardWidget;
class TrustedPhonesWidget;
class SecurityWidget;
class SettingsWidget;
class LogsWidget;
class AboutWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void SetupUi();
    void SetupSidebar();
    void SetupTray();
    void SwitchTab(int index);

    QStackedWidget* m_stackedWidget = nullptr;
    std::vector<QPushButton*> m_navButtons;

    DashboardWidget* m_dashboardWidget = nullptr;
    TrustedPhonesWidget* m_trustedPhonesWidget = nullptr;
    SecurityWidget* m_securityWidget = nullptr;
    SettingsWidget* m_settingsWidget = nullptr;
    LogsWidget* m_logsWidget = nullptr;
    AboutWidget* m_aboutWidget = nullptr;

    QSystemTrayIcon* m_trayIcon = nullptr;
    QMenu* m_trayMenu = nullptr;
};

} // namespace AnshuBio
