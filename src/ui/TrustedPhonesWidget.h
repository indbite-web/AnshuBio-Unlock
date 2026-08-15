#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QPushButton>

namespace AnshuBio {

class TrustedPhonesWidget : public QWidget {
    Q_OBJECT
public:
    explicit TrustedPhonesWidget(QWidget* parent = nullptr);
    void RefreshData();

signals:
    void RequestPairingWizard();

private:
    void SetupUi();

    QTableWidget* m_table = nullptr;
    QPushButton* m_addPhoneBtn = nullptr;
    QPushButton* m_removePhoneBtn = nullptr;
    QPushButton* m_revokePhoneBtn = nullptr;
};

} // namespace AnshuBio
