#pragma once

#include <QProgressBar>
#include <QEvent>
#include <QWidget>
#include "../core/TaskProgressService.h"

namespace QuarkMeta {

class TaskProgressStrip : public QProgressBar {
    Q_OBJECT

public:
    explicit TaskProgressStrip(QWidget* parent = nullptr);
    ~TaskProgressStrip() override = default;

    void setAnchors(QWidget* bodyWrapper, QWidget* statusBar);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void updateGeometryPos();

    QWidget* m_bodyWrapper = nullptr;
    QWidget* m_statusBar = nullptr;
};

} // namespace QuarkMeta
