#include "TaskProgressController.h" 
#include "UiHelper.h" 
#include "StyleLibrary.h"
#include <QVBoxLayout> 
#include <algorithm> 

using namespace QuarkMeta::Style;
 
namespace QuarkMeta { 
 
TaskProgressController::TaskProgressController(QWidget* parentWidget, QWidget* anchorWidget, QLabel* statusLabel, QObject* parent) 
    : QObject(parent), m_statusLabel(statusLabel) { 
    Q_UNUSED(anchorWidget); 
 
    if (parentWidget) { 
        m_topProgressBar = new QProgressBar(parentWidget); 
        m_topProgressBar->setFixedHeight(5); 
        m_topProgressBar->setTextVisible(false); 
        m_topProgressBar->setRange(0, 100); 
        m_topProgressBar->setInvertedAppearance(false); 
        m_topProgressBar->setStyleSheet(QString( 
            "QProgressBar { background: transparent; border: none; max-height: 5px; }" 
            "QProgressBar::chunk { background-color: %1; border-radius: 1px; }" 
        ).arg(qssColor(PrimaryBlue))); 
        m_topProgressBar->hide(); 
 
        if (QVBoxLayout* bodyLayout = qobject_cast<QVBoxLayout*>(parentWidget->layout())) { 
            bodyLayout->insertWidget(0, m_topProgressBar); 
        } 
    } 
 
    m_elapsedTimer = new QTimer(this); 
    m_elapsedTimer->setInterval(100); 
    connect(m_elapsedTimer, &QTimer::timeout, this, &TaskProgressController::onTick); 
} 
 
void TaskProgressController::start(int totalCount) { 
    m_syncStartTime = QDateTime::currentMSecsSinceEpoch(); 
    m_totalBatchCount = totalCount; 
    if (m_topProgressBar) { 
        m_topProgressBar->setValue(0); 
        m_topProgressBar->show(); 
    } 
    if (m_elapsedTimer) { 
        m_elapsedTimer->start(); 
    } 
} 
 
void TaskProgressController::updateProgress(int pct) { 
    if (m_topProgressBar) { 
        m_topProgressBar->setValue(pct); 
    } 
} 
 
void TaskProgressController::finish() { 
    if (m_elapsedTimer) { 
        m_elapsedTimer->stop(); 
    } 
    if (m_topProgressBar) { 
        m_topProgressBar->hide(); 
    } 
    m_syncStartTime = 0; 
    m_totalBatchCount = 0; 
} 
 
void TaskProgressController::formatTime(qint64 totalSeconds, QString& out) const { 
    if (totalSeconds < 0) totalSeconds = 0; 
    qint64 hours = totalSeconds / 3600; 
    qint64 mins = (totalSeconds % 3600) / 60; 
    qint64 secs = totalSeconds % 60; 
    if (hours > 0) { 
        out = QString("%1:%2:%3") 
            .arg(hours, 2, 10, QChar('0')) 
            .arg(mins, 2, 10, QChar('0')) 
            .arg(secs, 2, 10, QChar('0')); 
    } else { 
        out = QString("%1:%2") 
            .arg(mins, 2, 10, QChar('0')) 
            .arg(secs, 2, 10, QChar('0')); 
    } 
} 
 
void TaskProgressController::onTick() { 
    if (!m_statusLabel || !m_topProgressBar) return; 
 
    if (m_syncStartTime > 0 && m_totalBatchCount > 0) { 
        double elapsedSec = (QDateTime::currentMSecsSinceEpoch() - m_syncStartTime) / 1000.0; 
        int currentPct = m_topProgressBar->value(); 
         
        int completedCount = std::clamp((int)((double)currentPct / 100.0 * m_totalBatchCount), 0, m_totalBatchCount); 
 
        QString countdownStr = "00:00"; 
        QString totalEstStr = "00:00"; 
 
        if (currentPct >= 5) { 
            qint64 remainingSec = static_cast<qint64>(elapsedSec * (100.0 - currentPct) / (double)currentPct); 
            qint64 totalEstSec = static_cast<qint64>(elapsedSec) + remainingSec; 
            formatTime(remainingSec, countdownStr); 
            formatTime(totalEstSec, totalEstStr); 
        } 
 
        m_statusLabel->setText(QString("扫描数据中... %1%  数量：%2/%3  |  倒计时分 %4 / 预计时分: %5") 
                              .arg(currentPct) 
                              .arg(completedCount) 
                              .arg(m_totalBatchCount) 
                              .arg(countdownStr) 
                              .arg(totalEstStr)); 
    } 
} 
 
} // namespace QuarkMeta 
