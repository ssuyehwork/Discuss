#pragma once

#include <QObject>
#include <QProgressBar>
#include <QTimer>
#include <QLabel>
#include <QWidget>
#include <QDateTime>

namespace QuarkMeta {

class TaskProgressController : public QObject {
    Q_OBJECT
public:
    explicit TaskProgressController(QWidget* parentWidget, QWidget* anchorWidget, QLabel* statusLabel, QObject* parent = nullptr);
    ~TaskProgressController() override = default;

    void start(int totalCount);
    void updateProgress(int pct);
    void finish();

private slots:
    void onTick();

private:
    void formatTime(qint64 totalSeconds, QString& out) const;

    QProgressBar* m_topProgressBar = nullptr;
    QTimer* m_elapsedTimer = nullptr;
    qint64 m_syncStartTime = 0;
    int m_totalBatchCount = 0;
    QLabel* m_statusLabel = nullptr;
};

} // namespace QuarkMeta
