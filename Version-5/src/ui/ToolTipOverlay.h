#ifndef TOOLTIPOVERLAY_H
#define TOOLTIPOVERLAY_H

#include <QWidget>
#include <QPainter>
#include <QElapsedTimer>
#include <QDebug>
#include <QScreen>
#include <QGuiApplication>
#include <QApplication>
#include <QTimer>
#include <QThread>
#include <QFontMetrics>
#include <QTextDocument>
#include <QPointer>
#include <QPainterPath>
#include <QColor>
#include <QPen>
#include <QBrush>
#include <QRectF>
#include <QPropertyAnimation>

namespace QuarkMeta {

/**
 * @brief ToolTipOverlay: 全局统一的自定义 Tooltip
 * [CRITICAL] 本项目严禁使用任何形式的“Windows 系统默认 Tip 样式”！
 * [RULE] 1. 杜绝原生内容带来的系统阴影和不透明度。
 * [RULE] 2. 所有的 ToolTip 逻辑必须通过此 ToolTipOverlay 渲染。
 * [RULE] 3. 此组件必须保持扁平化 (Flat)，严禁添加任何阴影特效。
 */
class ToolTipOverlay : public QWidget {
    Q_OBJECT
public:
    static ToolTipOverlay* instance() {
        static QPointer<ToolTipOverlay> inst;
        if (!inst) {
            inst = new ToolTipOverlay();
        }
        return inst;
    }

    /**
     * @brief 显示提示文字
     */
    void showText(const QPoint& globalPos, const QString& text, int timeout = 700, const QColor& borderColor = QColor("#B0B0B0"), bool exactPosition = false, const QColor& backgroundColor = QColor("#2B2B2B"));

    // 兼容旧接口
    void showTip(const QString& text, const QPoint& pos, int timeout = 700) {
        showText(pos, text, timeout);
    }

    static void hideTip() {
        if (instance()) instance()->hideOverlay();
    }

    void hideOverlay();

    /**
     * @brief 真·静默预热接口：提前向系统申请 HWND 并激活 DWM 显存分层与排版引擎，杜绝首次悬停顿挫
     */
    void silentWarmup();

protected:
    explicit ToolTipOverlay();
    void paintEvent(QPaintEvent* event) override;

private slots:
    void triggerPendingShow();

private:
    QString m_text;
    QTextDocument m_doc;
    QTimer m_hideTimer;
    QTimer m_showDelayTimer;

    QPoint m_pendingPos;
    QString m_pendingText;
    int m_pendingTimeout = 700;
    QColor m_pendingBorderColor;
    QColor m_pendingBackgroundColor;
    bool m_pendingExactPosition = false;

    QColor m_currentBorderColor = QColor("#B0B0B0");
    QColor m_currentBackgroundColor = QColor("#2B2B2B");
    QPoint m_lastPos;

    bool m_warmedUp = false; // 防重入预热标记
};

} // namespace QuarkMeta

#endif // TOOLTIPOVERLAY_H