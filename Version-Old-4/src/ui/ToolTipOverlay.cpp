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

    hide();
}

void ToolTipOverlay::silentWarmup() {
    if (m_warmedUp) return;
    m_warmedUp = true;

    // 1. 强制向操作系统申请底层真正的 Win32 窗口句柄 (HWND)
    (void)this->winId();

    // 2. 绝对不可见双重保护：移至屏幕外万象之外 + 全透明
    setWindowOpacity(0.0);
    move(-9999, -9999);
    resize(40, 24);

    // 3. 预先激活 QTextDocument 字体解析引擎与字形缓存
    m_doc.setHtml(
        "<html><body style='margin:0; padding:0; color:#EEEEEE; font-family:\"Microsoft YaHei\",\"Segoe UI\",sans-serif;'>"
        "<div style='color:#EEEEEE;'>Warmup</div></body></html>"
    );
    m_doc.setTextWidth(-1);
    (void)m_doc.idealWidth();
    (void)m_doc.size();

    // 4. 驱动 DWM 在显存中分配透明混合图层并完成首次 GPU 渲染指令预编译
    show();
    render(this);
    hide();

    // 5. 静默复位，随时就绪
    setWindowOpacity(1.0);
    m_doc.clear();
}

void ToolTipOverlay::hideOverlay() {
    m_showDelayTimer.stop();
    m_hideTimer.stop();
    hide();
}

void ToolTipOverlay::showText(const QPoint& globalPos, const QString& text, int timeout, const QColor& borderColor, bool exactPosition, const QColor& backgroundColor) {
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

    // 若已经处于显示状态，且文本/边框无变化，仅当位置发生显著变化（>5px）时更新，避免无谓的重绘与抖动
    if (isVisible() && m_pendingText == text && m_currentBorderColor == borderColor && !exactPosition) {
        if ((globalPos - m_lastPos).manhattanLength() < 5) {
            return;
        }
        m_lastPos = globalPos;
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

    m_lastPos = globalPos;
    m_pendingPos = globalPos;
    m_pendingText = text;
    m_pendingTimeout = timeout;
    m_pendingBorderColor = borderColor;
    m_pendingBackgroundColor = backgroundColor;
    m_pendingExactPosition = exactPosition;

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
    // ToolTip 圆角锁定为 2px
    p.drawRoundedRect(rectF, 2, 2);
    
    if (!m_text.isEmpty()) {
        p.save();
        p.translate(12, 8); 
        m_doc.drawContents(&p);
        p.restore();
    }
}

} // namespace QuarkMeta