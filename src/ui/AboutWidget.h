#pragma once
#include <QWidget>

namespace AnshuBio {

class AboutWidget : public QWidget {
    Q_OBJECT
public:
    explicit AboutWidget(QWidget* parent = nullptr);

private:
    void SetupUi();
};

} // namespace AnshuBio
