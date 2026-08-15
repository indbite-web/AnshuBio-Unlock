#pragma once
#include <QWidget>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>

namespace AnshuBio {

class SettingsWidget : public QWidget {
    Q_OBJECT
public:
    explicit SettingsWidget(QWidget* parent = nullptr);
    void RefreshData();

private:
    void SetupUi();

    QLineEdit* m_pcDisplayNameEdit = nullptr;
    QCheckBox* m_startWithWindowsCheck = nullptr;
    QCheckBox* m_protectionEnabledCheck = nullptr;
    QCheckBox* m_soundCheck = nullptr;
    QCheckBox* m_vibrationCheck = nullptr;
    QPushButton* m_saveBtn = nullptr;
};

} // namespace AnshuBio
