#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QPushButton>

namespace AnshuBio {

class LogsWidget : public QWidget {
    Q_OBJECT
public:
    explicit LogsWidget(QWidget* parent = nullptr);
    void RefreshData();

private:
    void SetupUi();

    QTableWidget* m_table = nullptr;
    QPushButton* m_refreshBtn = nullptr;
};

} // namespace AnshuBio
