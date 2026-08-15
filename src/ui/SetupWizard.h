#pragma once
#include <QDialog>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>

namespace AnshuBio {

class SetupWizard : public QDialog {
    Q_OBJECT
public:
    explicit SetupWizard(QWidget* parent = nullptr);

signals:
    void WizardFinished();

private:
    void SetupUi();
    QWidget* CreateWelcomeStep();
    QWidget* CreateIdentityStep();
    QWidget* CreatePairingStep();
    QWidget* CreateConfirmationStep();
    QWidget* CreateCompleteStep();

    QStackedWidget* m_stackedWidget = nullptr;
    QLineEdit* m_nameEdit = nullptr;
    QLabel* m_codeLabel = nullptr;
    std::string m_sessionId;
};

} // namespace AnshuBio
