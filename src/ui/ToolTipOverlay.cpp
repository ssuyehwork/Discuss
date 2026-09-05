#include "ToolTipOverlay.h"

#include <QTimer>

namespace QuarkMeta {

ToolTipOverlay::ToolTipOverlay() : QWidget(nullptr) {
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | 
                  Qt::WindowTransparentForInput | Qt::NoDropShadowWindowHint | 
                  Qt::WindowDoesNotAcceptFocus | Qt::WindowStaysOnTopHint);
    setObjectName("ToolTipOverlay");

    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    
    m_doc.setUndoRedoEnabled(false);
    // [ULTIMATE FIX] 强制锁定调色板颜色
    QPalette pal = palette();
    pal.setColor(QPalette::WindowText, QColor("#EEEEEE"));
    pal.setColor(QPalette::Text, QColor("#EEEEEE"));
    pal.setColor(QPalette::ButtonText, QColor("#EEEEEE"));
    setPalette(pal);

    m_doc.setDefaultStyleSheet("body, div, p, span, b, i { color: #EEEEEE !important; font-family: 'Microsoft YaHei', 'Segoe UI'; }"); 

    QFont f = font();
    f.setPointSize(9);
    m_doc.setDefaultFont(f);

    m_hideTimer.setSingleShot(true);
    connect(&m_hideTimer, &QTimer::timeout, this, &ToolTipOverlay::hideOverlay);

    m_showDelayTimer.setSingleShot(true);
    connect(&m_showDelayTimer, &QTimer::timeout, this, &ToolTipOverlay::triggerPendingShow);

    // 初始静默隐藏，等待 MainWindow 的 showEvent 触发真正有效的 GPU 预热
    hide();
}

void ToolTipOverlay::hideOverlay() {
    m_showDelayTimer.stop();
    m_hideTimer.stop();
    hide();
}

void ToolTipOverlay::showText(const QPoint& globalPos, const QString& text, int timeout, const QColor& borderColor, bool exactPosition, const QColor& backgroundColor) {
    // [THREAD SAFE] 强制确保在主线程执行
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, [this, globalPos, text, timeout, borderColor, exactPosition, backgroundColor]() { 
            showText(globalPos, text, timeout, borderColor, exactPosition, backgroundColor); 
        });
        return;
    }

    if (text.isEmpty() && !exactPosition) {
        hideOverlay();
        return;
    }

    // 若已经处于显示状态，且文本/边框无变化，仅更新位置，不需要重新延迟防抖
    if (isVisible() && m_pendingText == text && m_currentBorderColor == borderColor && !exactPosition) {
        m_showDelayTimer.stop();
        QPoint pos = globalPos + QPoint(15, 15);
        QScreen* screen = QGuiApplication::screenAt(globalPos);
        if (!screen) screen = QGuiApplication::primaryScreen();
        if (screen) {
            QRect screenGeom = screen->geometry();
            if (pos.x() + width() > screenGeom.right()) {
                pos.setX(globalPos.x() - width() - 15);
            }
            if (pos.y() + height() > screenGeom.bottom()) {
                pos.setY(globalPos.y() - height() - 15);
            }
        }
        move(pos);
        return;
    }

    // 保存等待显示的参数
    m_pendingPos = globalPos;
    m_pendingText = text;
    m_pendingTimeout = timeout;
    m_pendingBorderColor = borderColor;
    m_pendingBackgroundColor = backgroundColor;
    m_pendingExactPosition = exactPosition;

    // 启动 150ms 延迟防抖定时器，频繁划过时只响应最后停顿的控件
    m_showDelayTimer.start(150);
}

void ToolTipOverlay::triggerPendingShow() {
    m_currentBorderColor = m_pendingBorderColor;
    m_currentBackgroundColor = m_pendingBackgroundColor;

    int timeout = m_pendingTimeout;
    if (timeout > 0) {
        timeout = qBound(500, timeout, 60000); 
    }

    int w = 40;
    int h = 24;

    if (m_pendingText.isEmpty()) {
        m_text = "";
        m_doc.clear();
        w = 60;
        h = 24;
    } else {
        QString htmlBody;
        if (m_pendingText.contains("<") && m_pendingText.contains(">")) {
            htmlBody = m_pendingText;
        } else {
            htmlBody = m_pendingText.toHtmlEscaped().replace("\n", "<br>");
        }

        m_text = QString(
            "<html><head><style>div, p, span, body { color: #EEEEEE !important; }</style></head>"
            "<body style='margin:0; padding:0; color:#EEEEEE; font-family:\"Microsoft YaHei\",\"Segoe UI\",sans-serif;'>"
            "<div style='color:#EEEEEE !important;'>%1</div>"
            "</body></html>"
        ).arg(htmlBody);
        
        m_doc.setHtml(m_text);
        m_doc.setDocumentMargin(0); 
        
        m_doc.setTextWidth(-1); 
        qreal idealW = m_doc.idealWidth();
        
        if (idealW > 450) {
            m_doc.setTextWidth(450); 
        } else {
            m_doc.setTextWidth(idealW); 
        }
        
        QSize textSize = m_doc.size().toSize();
        
        int padX = 12; 
        int padY = 8;
        
        w = textSize.width() + padX * 2;
        h = textSize.height() + padY * 2;
        
        w = qMax(w, 40);
        h = qMax(h, 24);
    }
    
    resize(w, h);
    
    QPoint pos;
    if (m_pendingExactPosition) {
        pos = m_pendingPos;
    } else {
        pos = m_pendingPos + QPoint(15, 15);
        QScreen* screen = QGuiApplication::screenAt(m_pendingPos);
        if (!screen) screen = QGuiApplication::primaryScreen();
        if (screen) {
            QRect screenGeom = screen->geometry();
            if (pos.x() + width() > screenGeom.right()) {
                pos.setX(m_pendingPos.x() - width() - 15);
            }
            if (pos.y() + height() > screenGeom.bottom()) {
                pos.setY(m_pendingPos.y() - height() - 15);
            }
        }
    }
    
    move(pos);
    setWindowOpacity(1.0);
    show();
    update();
    raise();

    if (timeout > 0) {
        m_hideTimer.start(timeout);
    } else {
        m_hideTimer.stop();
    }
}

void ToolTipOverlay::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    QRectF rectF(0.5, 0.5, width() - 1, height() - 1);
    
    p.setPen(QPen(m_currentBorderColor, 1));
    p.setBrush(m_currentBackgroundColor);
    // 2026-03-xx 按照用户硬性要求：ToolTip 圆角必须锁定为 2px
    p.drawRoundedRect(rectF, 2, 2);
    
    if (!m_text.isEmpty()) {
        p.save();
        p.translate(12, 8); 
        m_doc.drawContents(&p);
        p.restore();
    }
}

} // namespace QuarkMeta
