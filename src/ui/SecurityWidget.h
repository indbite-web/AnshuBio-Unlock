#pragma once
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace AnshuBio {

class SecurityWidget : public QWidget {
    Q_OBJECT
public:
    explicit SecurityWidget(QWidget* parent = nullptr);
    void RefreshData();

private:
    void SetupUi();

    QLabel* m_fingerprintLabel = nullptr;
    QLabel* m_vaultStatusLabel = nullptr;
    QLineEdit* m_usernameEdit = nullptr;
    QLineEdit* m_domainEdit = nullptr;
    QLineEdit* m_passwordEdit = nullptr;
    QPushButton* m_saveCredBtn = nullptr;
    QPushButton* m_clearCredBtn = nullptr;
};

} // namespace AnshuBio
