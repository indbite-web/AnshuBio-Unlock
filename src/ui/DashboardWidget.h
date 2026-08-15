#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace AnshuBio {

class DashboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit DashboardWidget(QWidget* parent = nullptr);
    void RefreshData();

signals:
    void NavigateToTab(int tabIndex);

private:
    void SetupUi();
    QWidget* CreateCard(const QString& title, const QString& value, const QString& status, const QString& accentColor);

    QLabel* m_pcNameLabel = nullptr;
    QLabel* m_protectionBadge = nullptr;
    QLabel* m_osStateLabel = nullptr;
    QLabel* m_wifiStatusLabel = nullptr;
    QLabel* m_btStatusLabel = nullptr;
    QLabel* m_phonesCountLabel = nullptr;
    QLabel* m_powerStatusLabel = nullptr;

    QPushButton* m_manualLockBtn = nullptr;
    QPushButton* m_toggleProtectionBtn = nullptr;
};

} // namespace AnshuBio
