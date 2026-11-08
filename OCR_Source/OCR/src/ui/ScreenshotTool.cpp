#include "ScreenshotTool.h"
#include "ToolTipOverlay.h"
#include "StringUtils.h"
#include "IconHelper.h"
#include "core/ClipboardMonitor.h"

#include <QApplication>
#include <QScreen>
#include <QWheelEvent>
#include <QFileDialog>
#include <QClipboard>
#include <QSettings>
#include <QAbstractItemView>
#include <QTreeView>
#include <QListView>
#include <QTableView>
#include <QDir>
#include <QDateTime>
#include <QCoreApplication>
#include <cmath>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#include <tchar.h>
#include <uiautomation.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "ole32.lib")

QRect getActualWindowRect(HWND hwnd) {
    RECT rect;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect)))) {
        return QRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
    }
    GetWindowRect(hwnd, &rect);
    return QRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
}
#endif

ScreenshotTool::~ScreenshotTool() {
    if (m_toolbar) m_toolbar->deleteLater();
    qDeleteAll(m_annotations);
    qDeleteAll(m_redoStack);
    if (m_activeShape) {
        delete m_activeShape;
        m_activeShape = nullptr;
    }
}

ScreenshotTool::ScreenshotTool(QWidget* parent) 
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint) 
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_QuitOnClose, false);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowState(Qt::WindowFullScreen);
    setMouseTracking(true);
    m_screenPixmap = QGuiApplication::primaryScreen()->grabWindow(0);
    m_screenImage = m_screenPixmap.toImage();
    // [OPTIMIZATION] 移除初始化时的全屏马赛克生成，改为“按需延迟生成”以节省显存 (约节省 130MB+ / 4K屏)
    m_mosaicPixmap = QPixmap();

    QSettings settings("OCR", "Screenshot");
    m_currentColor = settings.value("color", QColor(255, 50, 50)).value<QColor>();
    m_currentStrokeWidth = settings.value("strokeWidth", 3).toInt();
    m_currentArrowStyle = static_cast<ArrowStyle>(settings.value("arrowStyle", 0).toInt());
    m_currentTool = static_cast<ScreenshotToolType>(settings.value("tool", 0).toInt());
    m_currentFontFamily = settings.value("fontFamily", "Microsoft YaHei").toString();
    m_currentFontSize = settings.value("fontSize", 14).toInt();
    m_currentBold = settings.value("bold", true).toBool();
    m_currentItalic = settings.value("italic", false).toBool();
    m_isConfirmed = false;

    m_toolbar = new ScreenshotToolbar(this); m_toolbar->hide();
    m_infoBar = new SelectionInfoBar(this);
    m_lastMouseMovePos = mapFromGlobal(QCursor::pos());
    m_textInput = new QLineEdit(this); m_textInput->hide(); m_textInput->setFrame(false);
    m_textInput->installEventFilter(this);
    connect(m_textInput, &QLineEdit::editingFinished, this, &ScreenshotTool::commitTextInput);
}

void ScreenshotTool::showEvent(QShowEvent* event) { QWidget::showEvent(event); detectWindows(); }
void ScreenshotTool::cancel() { 
    emit screenshotCanceled(); 
    if (m_toolbar) m_toolbar->close(); 
    // [OPTIMIZATION] 退出时显式释放巨大全屏资源，防止 deleteLater 延迟析构导致的内存堆叠
    m_screenPixmap = QPixmap();
    m_screenImage = QImage();
    m_mosaicPixmap = QPixmap();
    close(); 
}


void ScreenshotTool::mousePressEvent(QMouseEvent* e) {
    m_lastMouseMovePos = e->pos();
    setFocus();
    if(m_textInput->isVisible() && !m_textInput->geometry().contains(e->pos())) commitTextInput();
    
    if (m_currentTool == ScreenshotToolType::Picker && e->button() == Qt::LeftButton) {
        QColor c = m_screenImage.pixelColor(e->pos());
        setDrawColor(c);
        // 取色后切回上一个工具，如果上一个是 None 则保持 None
        setTool(ScreenshotToolType::None);
        m_toolbar->selectTool(ScreenshotToolType::None);
        update();
        return;
    }
    if(e->button() == Qt::RightButton) {
        // [用户修改要求] 拦截右键按下，统一在 Release 中处理取消逻辑，防止事件穿透到第三方应用触发菜单
        e->accept();
        return;
    }
    if(e->button() != Qt::LeftButton) return;
    m_dragHandle = -1; m_editHandle = -1;

    // 强化：在按下鼠标时重新进行一次命中测试，确保 hoveredShape 准确
    if (m_state == ScreenshotState::Editing && !m_isDragging && !m_isDrawing) {
        m_hoveredShape = nullptr;
        for (int i = m_annotations.size() - 1; i >= 0; --i) {
            if (m_annotations[i]->hitTest(e->pos()) || m_annotations[i]->getHandleAt(e->pos()) != -1) {
                m_hoveredShape = m_annotations[i];
                break;
            }
        }
    }

    if (m_state == ScreenshotState::Selecting) {
        m_startPoint = e->pos(); m_endPoint = m_startPoint; m_isDragging = true; m_toolbar->hide(); m_infoBar->hide();
    } else {
        // 优先处理已有标注的句柄
        if (m_hoveredShape) {
            int handle = m_hoveredShape->getHandleAt(e->pos());
            // [CRITICAL] 使用 m_dragOrigin 而不是 m_startPoint，防止修改选区大小 / Use m_dragOrigin to prevent selection resize
            if (handle != -1) { m_editHandle = handle; m_dragOrigin = e->pos(); m_isDragging = true; update(); return; }
            if (m_hoveredShape->hitTest(e->pos())) { m_editHandle = 100; m_dragOrigin = e->pos(); m_isDragging = true; update(); return; }
        }

        int handle = getHandleAt(e->pos());
        if (selectionRect().contains(e->pos()) && m_currentTool != ScreenshotToolType::None && handle == -1) {
            if (m_currentTool == ScreenshotToolType::Text) { showTextInput(e->pos()); return; }
            m_isDrawing = true; m_currentAnnotation = {m_currentTool, {e->pos()}, m_currentColor, "", m_currentStrokeWidth, LineStyle::Solid, m_currentArrowStyle, m_fillEnabled, 
                                                       m_currentFontFamily, m_currentFontSize, m_currentBold, m_currentItalic};
            if(m_currentTool == ScreenshotToolType::Marker) {
                int c = 1; for(auto* a : std::as_const(m_annotations)) if(a->data.type == ScreenshotToolType::Marker) c++;
                m_currentAnnotation.text = QString::number(c);
            }
            if (m_activeShape) {
                delete m_activeShape;
                m_activeShape = nullptr;
            }
            m_activeShape = createShape(m_currentAnnotation);
        } else if (handle != -1) {
            m_dragHandle = handle; m_isDragging = true;
        } else if (selectionRect().contains(e->pos())) {
            // [CRITICAL] 移动选区时使用 m_dragOrigin / Use m_dragOrigin when moving selection
            m_dragHandle = 8; m_dragOrigin = e->pos(); m_isDragging = true;
        }
    }
    update();
}

void ScreenshotTool::mouseMoveEvent(QMouseEvent* e) {
    m_lastMouseMovePos = e->pos();
    if (m_state == ScreenshotState::Selecting && !m_isDragging) {
        QRect smallest; long long minArea = -1;
        QPoint p = e->pos();
        
        // 2026-03-xx 改进算法：在包含鼠标点的矩形中，寻找面积最小的那个。
        // 同时引入“宽高比”保护，防止选中极其细长的线条控件。
        for (const QRect& r : std::as_const(m_detectedRects)) {
            if (r.contains(p)) {
                // 过滤掉面积过小的杂质
                if (r.width() < 10 || r.height() < 10) continue;
                
                long long area = (long long)r.width() * r.height();
                if (minArea == -1 || area < minArea) {
                    minArea = area;
                    smallest = r;
                }
            }
        }
        
        // 增加逻辑：如果找到了最小矩形，但其面积占据了全屏的 95% 以上，通常意味着选中的是桌面背景，
        // 此时我们尝试寻找更具体的子项，或者保持原样。
        
        if (m_highlightedRect != smallest) {
            m_highlightedRect = smallest;
            update();
        }
    } else {
        m_highlightedRect = QRect();
    }

    if (!m_isDragging && !m_isDrawing && m_state == ScreenshotState::Editing) {
        BaseShape* prevHover = m_hoveredShape; m_hoveredShape = nullptr;
        for (int i = m_annotations.size() - 1; i >= 0; --i) {
            if (m_annotations[i]->hitTest(e->pos()) || m_annotations[i]->getHandleAt(e->pos()) != -1) {
                m_hoveredShape = m_annotations[i]; break;
            }
        }
        if (prevHover != m_hoveredShape) update();
    }

    if (m_isDragging) {
        QPoint p = e->pos();
        if (m_editHandle != -1 && m_hoveredShape) {
            if (m_editHandle == 100) { m_hoveredShape->moveBy(p - m_dragOrigin); m_dragOrigin = p; }
            else { m_hoveredShape->updatePoint(m_editHandle, p); }
            update(); return;
        }
        if (m_currentTool == ScreenshotToolType::Eraser) {
            bool changed = false;
            for (int i = m_annotations.size() - 1; i >= 0; --i) {
                if (m_annotations[i]->hitTest(p)) { m_redoStack.append(m_annotations.takeAt(i)); changed = true; if(m_hoveredShape == m_redoStack.last()) m_hoveredShape = nullptr; }
            }
            if (changed) update(); return;
        }
        if (m_state == ScreenshotState::Selecting) {
            m_endPoint = e->pos();
        } else if (m_dragHandle == 8) {
            QPoint delta = e->pos() - m_dragOrigin;
            m_startPoint += delta; m_endPoint += delta;
            m_dragOrigin = e->pos();
        } else if (m_dragHandle != -1) {
            if(m_dragHandle==0) m_startPoint = p; else if(m_dragHandle==1) m_startPoint.setY(p.y()); else if(m_dragHandle==2) { m_startPoint.setY(p.y()); m_endPoint.setX(p.x()); }
            else if(m_dragHandle==3) m_endPoint.setX(p.x()); else if(m_dragHandle==4) m_endPoint = p; else if(m_dragHandle==5) m_endPoint.setY(p.y());
            else if(m_dragHandle==6) { m_endPoint.setY(p.y()); m_startPoint.setX(p.x()); } else if(m_dragHandle==7) m_startPoint.setX(p.x());
        }
        if (!selectionRect().isEmpty()) {
            if (!m_isImmediateOCR) {
                m_infoBar->updateInfo(selectionRect()); m_infoBar->show(); m_infoBar->move(selectionRect().left(), selectionRect().top() - 35); m_infoBar->raise();
            }
            updateToolbarPosition();
        }
    } else if (m_isDrawing && m_activeShape) {
        updateToolbarPosition();
        if (m_currentTool == ScreenshotToolType::Arrow || m_currentTool == ScreenshotToolType::Line || m_currentTool == ScreenshotToolType::Rect || m_currentTool == ScreenshotToolType::Ellipse || m_currentTool == ScreenshotToolType::MosaicRect) {
            if (m_activeShape->data.points.size() > 1) m_activeShape->data.points[1] = e->pos(); else m_activeShape->data.points.append(e->pos());
        } else m_activeShape->data.points.append(e->pos());
    } else updateCursor(e->pos());
    update();
}

void ScreenshotTool::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::RightButton) {
        // [用户修改要求] 右键单击触发放弃任务逻辑，且必须在 Release 时处理以完全拦截点击流，防止穿透
        if (m_isDrawing) { 
            m_isDrawing = false; 
            if (m_activeShape) {
                delete m_activeShape; 
                m_activeShape = nullptr; 
            }
            update(); 
        } else if (m_currentTool != ScreenshotToolType::None) { 
            m_currentTool = ScreenshotToolType::None; 
            m_toolbar->selectTool(ScreenshotToolType::None); 
            update(); 
        } else {
            cancel(); 
        }
        e->accept();
        return;
    }

    if (m_isDrawing) {
        m_isDrawing = false;
        if (m_activeShape) {
            m_annotations.append(m_activeShape);
            m_activeShape = nullptr;
            qDeleteAll(m_redoStack);
            m_redoStack.clear();
        }
    }
    else if (m_isDragging) {
        m_isDragging = false;
        m_editHandle = -1;
        if (m_state == ScreenshotState::Selecting) {
            if ((e->pos() - m_startPoint).manhattanLength() < 5) {
                if (!m_highlightedRect.isEmpty()) { m_startPoint = m_highlightedRect.topLeft(); m_endPoint = m_highlightedRect.bottomRight(); }
            }
            if (selectionRect().width() > 2 && selectionRect().height() > 2) {
                m_state = ScreenshotState::Editing;
                m_highlightedRect = QRect();
                m_detectedRects.clear();
            }
        }
    }
    if (m_state == ScreenshotState::Editing) {
        if (m_isImmediateOCR) {
            confirm();
            return;
        }
        updateToolbarPosition(); m_toolbar->show(); m_infoBar->updateInfo(selectionRect());
        m_infoBar->show(); m_infoBar->move(selectionRect().left(), selectionRect().top() - 35);
    }
    update();
}

void ScreenshotTool::contextMenuEvent(QContextMenuEvent* event) {
    // [用户修改要求] 彻底拦截上下文菜单事件，确保在任何情况下都不会弹出系统或第三方菜单
    event->accept();
}

void ScreenshotTool::updateToolbarPosition() {
    QRect r = selectionRect(); if(r.isEmpty()) return; m_toolbar->adjustSize();
    // 补偿 6px 的外边距，并将视觉位置贴近选区边缘（保留 4px 视觉间距）
    int x = r.right() - m_toolbar->width() + 6;
    int y = r.bottom() - 4; 
    
    if (x < -6) x = -6; 
    if (y + m_toolbar->height() - 6 > height()) y = r.top() - m_toolbar->height() + 4;
    
    m_toolbar->move(x, y); 
    if (!m_isImmediateOCR) {
        if (m_state == ScreenshotState::Editing && !m_toolbar->isVisible()) m_toolbar->show();
        if (m_toolbar->isVisible()) m_toolbar->raise();
    }
}

void ScreenshotTool::wheelEvent(QWheelEvent* event) {
    int delta = event->angleDelta().y();
    if (m_hoveredShape) {
        if (m_hoveredShape->data.type == ScreenshotToolType::Text) {
            int newSize = m_hoveredShape->data.fontSize + (delta > 0 ? 2 : -2);
            m_hoveredShape->data.fontSize = std::max(8, std::min(100, newSize));
            ToolTipOverlay::instance()->showText(QCursor::pos(), QString("文字大小: %1 pt").arg(m_hoveredShape->data.fontSize), 500);
        } else {
            int newWidth = m_hoveredShape->data.strokeWidth + (delta > 0 ? 1 : -1);
            m_hoveredShape->data.strokeWidth = std::max(1, std::min(50, newWidth));
            ToolTipOverlay::instance()->showText(QCursor::pos(), QString("线条粗细: %1 px").arg(m_hoveredShape->data.strokeWidth), 500);
        }
        update();
    } else {
        // 如果没有悬浮标注，修改全局设置并实时持久化
        if (m_currentTool == ScreenshotToolType::Text) {
            m_currentFontSize += (delta > 0 ? 2 : -2);
            m_currentFontSize = std::max(8, std::min(100, m_currentFontSize));
            QSettings("OCR", "Screenshot").setValue("fontSize", m_currentFontSize);
            ToolTipOverlay::instance()->showText(QCursor::pos(), QString("当前字号: %1 pt").arg(m_currentFontSize), 500);
        } else {
            m_currentStrokeWidth += (delta > 0 ? 1 : -1);
            m_currentStrokeWidth = std::max(1, std::min(50, m_currentStrokeWidth));
            QSettings("OCR", "Screenshot").setValue("strokeWidth", m_currentStrokeWidth);
            ToolTipOverlay::instance()->showText(QCursor::pos(), QString("当前粗细: %1 px").arg(m_currentStrokeWidth), 500);
        }
        update();
    }
    event->accept();
}

void ScreenshotTool::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    p.drawPixmap(0,0,m_screenPixmap);
    QRect r = selectionRect(); QPainterPath path; path.addRect(rect());
    if(r.isValid()) path.addRect(r); p.fillPath(path, QColor(0,0,0,120));

    if (m_state == ScreenshotState::Selecting && !m_isDragging && !m_highlightedRect.isEmpty()) {
        p.setPen(QPen(QColor(0, 120, 255, 200), 2)); p.setBrush(QColor(0, 120, 255, 30)); p.drawRect(m_highlightedRect);
    }
    if(r.isValid()) {
        p.setPen(QPen(QColor(0, 120, 255), 2)); p.drawRect(r);
        auto h = getHandleRects(); p.setBrush(Qt::white); for(auto& hr : h) p.drawEllipse(hr);
        
        // [STABILITY] 绘制阶段判空保护：确保不会在极其罕见的异步 delete 时刻访问非法地址
        p.save(); // [CRITICAL] 保存状态以防 clip 影响后续绘制
        p.setClipRect(r);
        for(auto* a : std::as_const(m_annotations)) {
            a->draw(p, m_mosaicPixmap);
            if (a == m_hoveredShape) {
                p.save();
                p.setRenderHint(QPainter::Antialiasing);
                p.setPen(QPen(Qt::cyan, 1.2, Qt::DashLine));
                p.setBrush(Qt::NoBrush);
                
                // 针对不同形状绘制更有意义的连接虚线
                if (a->data.type == ScreenshotToolType::Line || a->data.type == ScreenshotToolType::Arrow) {
                    if (a->data.points.size() >= 2) p.drawLine(a->data.points[0], a->data.points[1]);
                } else if (a->data.type == ScreenshotToolType::Rect || a->data.type == ScreenshotToolType::Ellipse || a->data.type == ScreenshotToolType::MosaicRect) {
                    if (a->data.points.size() >= 2) p.drawRect(QRectF(a->data.points[0], a->data.points[1]).normalized());
                } else if (a->data.type == ScreenshotToolType::Text) {
                    // 文字工具显示边界框
                    QFont font(a->data.fontFamily, a->data.fontSize);
                    font.setBold(a->data.isBold); font.setItalic(a->data.isItalic);
                    QRect r = QFontMetrics(font).boundingRect(a->data.text);
                    r.moveTo(a->data.points[0].toPoint().x(), a->data.points[0].toPoint().y() - r.height());
                    p.drawRect(r.adjusted(-4, -2, 4, 2));
                }

                // 绘制编辑句柄
                auto handles = a->getHandles();
                for(const auto& hh : std::as_const(handles)) {
                    p.setPen(QPen(Qt::white, 1));
                    p.setBrush(QColor(0, 120, 255));
                    p.drawRect(hh);
                }
                p.restore();
            }
        }
        if(m_isDrawing && m_activeShape) {
            m_activeShape->draw(p, m_mosaicPixmap);
        }
        p.restore(); // [CRITICAL] 恢复状态，解除选区 clip
    }

    // [CRITICAL] 放大镜绘制必须在选区 clip 之外，以防被裁剪
    if (m_state == ScreenshotState::Selecting || m_isDragging || m_isDrawing || m_currentTool == ScreenshotToolType::Picker) {
        drawMagnifier(p, m_lastMouseMovePos);
    }
}

void ScreenshotTool::setTool(ScreenshotToolType t) { 
    if(m_textInput->isVisible()) commitTextInput(); 
    
    // [OPTIMIZATION] 延迟初始化马赛克底图
    if ((t == ScreenshotToolType::Mosaic || t == ScreenshotToolType::MosaicRect) && m_mosaicPixmap.isNull()) {
        // [FIX] 2026-03-xx 按照用户要求：重命名 small 为 scaledImg 以规避 MSVC/Windows.h 冲突 (#define small char)
        QImage scaledImg = m_screenImage.scaled(m_screenImage.width()/40, m_screenImage.height()/40, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        m_mosaicPixmap = QPixmap::fromImage(scaledImg.scaled(m_screenImage.size(), Qt::IgnoreAspectRatio, Qt::FastTransformation));
    }

    m_currentTool = t; 
    QSettings("OCR", "Screenshot").setValue("tool", static_cast<int>(t)); 
}
void ScreenshotTool::setDrawColor(const QColor& c) { 
    m_currentColor = c; 
    QSettings("OCR", "Screenshot").setValue("color", c); 
    if (m_toolbar) m_toolbar->addRecentColor(c);
}
void ScreenshotTool::setDrawWidth(int w) { m_currentStrokeWidth = w; QSettings("OCR", "Screenshot").setValue("strokeWidth", w); }
void ScreenshotTool::setArrowStyle(ArrowStyle s) { m_currentArrowStyle = s; QSettings("OCR", "Screenshot").setValue("arrowStyle", static_cast<int>(s)); }
void ScreenshotTool::setFillEnabled(bool enabled) { m_fillEnabled = enabled; }
void ScreenshotTool::setBold(bool bold) { m_currentBold = bold; QSettings("OCR", "Screenshot").setValue("bold", bold); }
void ScreenshotTool::setItalic(bool italic) { m_currentItalic = italic; QSettings("OCR", "Screenshot").setValue("italic", italic); }
void ScreenshotTool::setFontFamily(const QString& family) { m_currentFontFamily = family; QSettings("OCR", "Screenshot").setValue("fontFamily", family); }
void ScreenshotTool::setFontSize(int size) { m_currentFontSize = size; QSettings("OCR", "Screenshot").setValue("fontSize", size); }

void ScreenshotTool::undo() { if(!m_annotations.isEmpty()) { m_redoStack.append(m_annotations.takeLast()); update(); } }
void ScreenshotTool::redo() { if(!m_redoStack.isEmpty()) { m_annotations.append(m_redoStack.takeLast()); update(); } }
void ScreenshotTool::copyToClipboard() { 
    QImage img = generateFinalImage();
    emit screenshotCaptured(img, false);
    autoSaveImage(img);
    // [OPTIMIZATION] 强制释放截图缓冲区
    m_screenPixmap = QPixmap();
    m_screenImage = QImage();
    m_mosaicPixmap = QPixmap();
    cancel(); 
}
void ScreenshotTool::save() { 
        QImage img = generateFinalImage();
    emit screenshotCaptured(img, false);
    QSettings settings("OCR", "Screenshot");
    QString defaultPath = QCoreApplication::applicationDirPath() + "/RPN_screenshot";
    QString saveDir = settings.value("savePath", defaultPath).toString();
    QString defaultFile = QDir(saveDir).filePath(QString("RPN_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")));
    QString f = QFileDialog::getSaveFileName(this, "保存截图", defaultFile, "PNG(*.png)"); 
    if(!f.isEmpty()) img.save(f);  
    autoSaveImage(img);
    cancel(); 
}
void ScreenshotTool::confirm() { 
    if (m_isConfirmed) return;
    m_isConfirmed = true;
    QImage img = generateFinalImage();
    emit screenshotCaptured(img, m_isImmediateOCR); 
    autoSaveImage(img);
    // [OPTIMIZATION] 确认后立即销毁内存中驻留的全屏大对象
    m_screenPixmap = QPixmap();
    m_screenImage = QImage();
    m_mosaicPixmap = QPixmap();
    cancel(); 
}
void ScreenshotTool::pin() { QImage img = generateFinalImage(); if (img.isNull()) return; auto* widget = new PinnedScreenshotWidget(QPixmap::fromImage(img), selectionRect()); widget->show(); cancel(); }

QRect ScreenshotTool::selectionRect() const { return QRect(m_startPoint, m_endPoint).normalized(); }
QList<QRect> ScreenshotTool::getHandleRects() const {
    QRect r = selectionRect(); QList<QRect> l; if(r.isEmpty()) return l; int s = 10;
    l << QRect(r.left()-s/2, r.top()-s/2, s, s) << QRect(r.center().x()-s/2, r.top()-s/2, s, s)
      << QRect(r.right()-s/2, r.top()-s/2, s, s) << QRect(r.right()-s/2, r.center().y()-s/2, s, s)
      << QRect(r.right()-s/2, r.bottom()-s/2, s, s) << QRect(r.center().x()-s/2, r.bottom()-s/2, s, s)
      << QRect(r.left()-s/2, r.bottom()-s/2, s, s) << QRect(r.left()-s/2, r.center().y()-s/2, s, s);
    return l;
}
int ScreenshotTool::getHandleAt(const QPoint& p) const { auto l = getHandleRects(); for(int i=0; i<l.size(); ++i) if(l[i].contains(p)) return i; return -1; }
void ScreenshotTool::updateCursor(const QPoint& p) {
    if (m_state == ScreenshotState::Editing) {
        int handle = getHandleAt(p); if (handle != -1) {
            switch (handle) { case 0: case 4: setCursor(Qt::SizeFDiagCursor); break; case 1: case 5: setCursor(Qt::SizeVerCursor); break; case 2: case 6: setCursor(Qt::SizeBDiagCursor); break; case 3: case 7: setCursor(Qt::SizeHorCursor); break; }
            return;
        }
        if (selectionRect().contains(p)) { if (m_currentTool != ScreenshotToolType::None) setCursor(Qt::CrossCursor); else setCursor(Qt::SizeAllCursor); return; }
    }
    setCursor(Qt::ArrowCursor);
}
void ScreenshotTool::showTextInput(const QPoint& p) { 
    m_textInput->move(p); 
    m_textInput->resize(200, 30); 
    QFont font(m_currentFontFamily, m_currentFontSize);
    font.setBold(m_currentBold);
    font.setItalic(m_currentItalic);
    m_textInput->setFont(font);
    m_textInput->setStyleSheet(QString("color: %1; background: transparent;").arg(m_currentColor.name()));
    m_textInput->show(); 
    m_textInput->setFocus(); 
}
void ScreenshotTool::commitTextInput() {
    if(m_textInput->text().isEmpty()) { m_textInput->hide(); return; }
    DrawingAnnotation ann = {ScreenshotToolType::Text, {m_textInput->pos()}, m_currentColor, m_textInput->text(), m_currentStrokeWidth, LineStyle::Solid, ArrowStyle::SolidSingle, false,
                             m_currentFontFamily, m_currentFontSize, m_currentBold, m_currentItalic};
    BaseShape* shape = createShape(ann);
    if (shape) {
        m_annotations.append(shape);
        qDeleteAll(m_redoStack);
        m_redoStack.clear();
    }
    m_textInput->hide(); m_textInput->clear(); update();
}

#ifdef Q_OS_WIN
BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam) {
    QList<QRect>* rects = reinterpret_cast<QList<QRect>*>(lParam);
    // 子窗口使用 GetWindowRect 即可，DWM 扩展边框通常只针对顶层窗口
    if (IsWindowVisible(hwnd)) {
        RECT rect;
        if (GetWindowRect(hwnd, &rect)) {
            QRect qr(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
            if (qr.width() > 5 && qr.height() > 5) rects->append(qr);
        }
    }
    return TRUE;
}
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    QList<QRect>* rects = reinterpret_cast<QList<QRect>*>(lParam);
    if (IsWindowVisible(hwnd) && !IsIconic(hwnd)) {
        TCHAR className[256]; GetClassName(hwnd, className, 256);
        if (_tcscmp(className, _T("Qt662QWindowIcon")) == 0) return TRUE;
        int cloaked = 0; DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
        if (cloaked) return TRUE;
        QRect qr = getActualWindowRect(hwnd); if (qr.width() > 10 && qr.height() > 10) { rects->append(qr); EnumChildWindows(hwnd, EnumChildProc, lParam); }
    }
    return TRUE;
}
#endif
void ScreenshotTool::detectWindows() { 
    if (m_state != ScreenshotState::Selecting) return;
    m_detectedRects.clear();
#ifdef Q_OS_WIN
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&m_detectedRects));
#endif

    // 关键优化：遍历本应用内所有活跃的 Qt 部件，实现对搜索框、分类栏等细微控件的精确侦测
    for (QWidget* top : QApplication::topLevelWidgets()) {
        if (top->isVisible() && !top->isMinimized() && top != this && top->windowOpacity() > 0) {
            collectQtWidgets(top);
            
            // 2026-03-xx 引入 UI Automation 探测外部应用精细 UI 元素
#ifdef Q_OS_WIN
            collectUIAElements((HWND)top->winId());
#endif
        }
    }

    // 针对鼠标当前指向的外部窗口进行深度的 UIA 探测
#ifdef Q_OS_WIN
    POINT pt;
    if (GetCursorPos(&pt)) {
        HWND targetHwnd = WindowFromPoint(pt); 
        if (targetHwnd) {
            // 向上寻找顶层窗口
            HWND rootHwnd = GetAncestor(targetHwnd, GA_ROOT);
            if (rootHwnd) collectUIAElements(rootHwnd);
        }
    }
#endif

    // 统一坐标系转换
    QPoint offset = mapToGlobal(QPoint(0,0)); 
    for(QRect& r : m_detectedRects) r.translate(-offset);
    
    // 去重并过滤极小区域
    QList<QRect> filtered;
    for (const QRect& r : std::as_const(m_detectedRects)) {
        if (r.width() > 5 && r.height() > 5 && !filtered.contains(r)) {
            filtered.append(r);
        }
    }
    m_detectedRects = filtered;
}

void ScreenshotTool::drawMagnifier(QPainter& p, const QPoint& pos) {
    if (pos.x() < 0 || pos.y() < 0 || pos.x() >= m_screenImage.width() || pos.y() >= m_screenImage.height()) return;

    p.save();
    const int zoom = 12;
    const int cols = 21;
    const int rows = 11;
    const int magWidth = cols * zoom;
    const int magHeight = rows * zoom;
    const int infoHeight = 130;
    
    int margin = 30;
    QRect sel = selectionRect();
    QRect magRect(pos.x() + margin, pos.y() + margin, magWidth, magHeight);

    // [CRITICAL] 放大镜绘制逻辑：当鼠标在选区内时，尝试在四个方向寻找空间放置放大镜，避免遮挡。
    if (m_state == ScreenshotState::Editing && sel.contains(pos)) {
        if (sel.right() + margin + magWidth < width()) magRect.moveTo(sel.right() + margin, pos.y() - magHeight / 2);
        else if (sel.left() - margin - magWidth > 0) magRect.moveTo(sel.left() - margin - magWidth, pos.y() - magHeight / 2);
        else if (sel.top() - margin - magHeight - infoHeight > 0) magRect.moveTo(pos.x() - magWidth / 2, sel.top() - margin - magHeight - infoHeight);
        else if (sel.bottom() + margin < height()) magRect.moveTo(pos.x() - magWidth / 2, sel.bottom() + margin);
    }

    // 边界安全检查，确保放大镜和信息面板不超出屏幕
    if (magRect.right() > width()) magRect.moveRight(width() - 5);
    if (magRect.left() < 0) magRect.moveLeft(5);
    if (magRect.bottom() + infoHeight > height()) magRect.moveBottom(height() - infoHeight - 5);
    if (magRect.top() < 0) magRect.moveTop(5);
    
    p.setRenderHint(QPainter::Antialiasing, false);
    
    // 1. 绘制放大区域背景及边框
    p.setPen(QPen(Qt::white, 1));
    p.setBrush(Qt::black);
    p.drawRect(magRect.adjusted(-1, -1, 1, 1));
    
    // 这里的源矩形取自原始屏幕像素
    QRect srcRect(pos.x() - cols/2, pos.y() - rows/2, cols, rows);
    p.drawPixmap(magRect, m_screenPixmap, srcRect);
    
    // 2. 绘制像素网格
    p.setPen(QPen(QColor(255, 255, 255, 50), 1));
    for (int i = 0; i <= magWidth; i += zoom) p.drawLine(magRect.left() + i, magRect.top(), magRect.left() + i, magRect.bottom());
    for (int i = 0; i <= magHeight; i += zoom) p.drawLine(magRect.left(), magRect.top() + i, magRect.right(), magRect.top() + i);
    
    // 3. 绘制中心像素高亮框
    p.setPen(QPen(Qt::white, 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(magRect.left() + (cols/2)*zoom, magRect.top() + (rows/2)*zoom, zoom, zoom);
    
    // 4. 绘制下方黑色信息面板
    QRect infoRect(magRect.left(), magRect.bottom() + 1, magWidth, infoHeight);
    p.fillRect(infoRect, Qt::black);
    
    QColor color = m_screenImage.pixelColor(pos);
    QString colorStr;
    if (m_colorFormatIndex == 0) colorStr = color.name(QColor::HexRgb).toUpper();
    else if (m_colorFormatIndex == 1) colorStr = QString("%1, %2, %3").arg(color.red()).arg(color.green()).arg(color.blue());
    else {
        // HSL 格式处理
        int h = color.hslHue() < 0 ? 0 : color.hslHue();
        int s = int(color.hslSaturationF() * 100);
        int l = int(color.lightnessF() * 100);
        colorStr = QString("HSL(%1, %2, %3)").arg(h).arg(s).arg(l);
    }

    p.setPen(Qt::white);
    p.setRenderHint(QPainter::TextAntialiasing);
    QFont font("Microsoft YaHei", 10);
    p.setFont(font);

    // 第一行：坐标 (x , y)
    QString coordStr = QString("(%1 , %2)").arg(pos.x()).arg(pos.y());
    p.drawText(infoRect.adjusted(0, 10, 0, 0), Qt::AlignHCenter | Qt::AlignTop, coordStr);

    // 第二行：颜色预览方块 + 色值文本
    int textWidth = p.fontMetrics().horizontalAdvance(colorStr);
    int totalContentWidth = 16 + 8 + textWidth; // 方块(16) + 间距(8) + 文本
    int startX = infoRect.left() + (magWidth - totalContentWidth) / 2;
    int line2Y = infoRect.top() + 38;
    
    p.setBrush(color);
    p.setPen(QPen(Qt::white, 1));
    p.drawRect(startX, line2Y, 16, 16);
    
    p.setPen(Qt::white);
    p.drawText(startX + 24, line2Y + 13, colorStr);

    // 第三、四、五行：功能提示
    font.setPointSize(9);
    p.setFont(font);
    p.drawText(infoRect.adjusted(0, 65, 0, 0), Qt::AlignHCenter | Qt::AlignTop, "Shift:  切换颜色格式");
    p.drawText(infoRect.adjusted(0, 86, 0, 0), Qt::AlignHCenter | Qt::AlignTop, "C:  复制色值");
    p.drawText(infoRect.adjusted(0, 107, 0, 0), Qt::AlignHCenter | Qt::AlignTop, "M:  复制坐标");

    p.restore();
}

void ScreenshotTool::collectQtWidgets(QWidget* parent) {
    if (!parent || !parent->isVisible()) return;

    // 记录当前部件的全局几何信息
    QRect r = parent->rect();
    QPoint globalPos = parent->mapToGlobal(QPoint(0,0));
    r.moveTo(globalPos);
    m_detectedRects.append(r);

    // [CRITICAL] 特殊处理：针对 QAbstractItemView (TreeView/ListView) 扫描内部 Item 矩形
    // 这解决了用户提到的“侧边栏分类检测不到”的问题
    if (auto* view = qobject_cast<QAbstractItemView*>(parent)) {
        detectItemViewRects(view);
    }

    // 递归处理子部件
    const QObjectList& children = parent->children();
    for (QObject* childObj : children) {
        QWidget* child = qobject_cast<QWidget*>(childObj);
        if (child && child->isVisible() && !child->rect().isEmpty()) {
            collectQtWidgets(child);
        }
    }
}

void ScreenshotTool::detectItemViewRects(QAbstractItemView* view) {
    if (!view || !view->model()) return;

    // 仅遍历可见区域内的索引，保证性能
    QRect viewportRect = view->viewport()->rect();
    QModelIndex topIndex = view->indexAt(viewportRect.topLeft());
    if (!topIndex.isValid()) return;

    // 简单高效的遍历：从顶层可见项向下扫描
    // 我们限制扫描数量以防万一模型巨大导致卡顿
    int count = 0;
    QModelIndex idx = topIndex;
    
    auto* treeView = qobject_cast<QTreeView*>(view);
    auto* listView = qobject_cast<QListView*>(view);

    while (idx.isValid() && count < 200) {
        QRect r = view->visualRect(idx);
        if (!r.isEmpty()) {
            QPoint globalPos = view->viewport()->mapToGlobal(r.topLeft());
            r.moveTo(globalPos);
            m_detectedRects.append(r);
        }
        
        // 尝试寻找下一个可见索引
        QModelIndex nextIdx;
        if (treeView) {
            nextIdx = treeView->indexBelow(idx);
        } else {
            // 对于 ListView 或 TableView，移动到下一行
            nextIdx = view->model()->index(idx.row() + 1, idx.column(), idx.parent());
        }
        
        idx = nextIdx;
        count++;
        
        // 如果超出了可视区域，可以停止 (可选优化)
        if (!r.isEmpty() && r.top() > viewportRect.bottom()) break;
    }
}

#ifdef Q_OS_WIN
void ScreenshotTool::collectUIAElements(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return;

    // [STABILITY] COM 初始化保护：确保 UI Automation API 能在任何线程环境下正常工作
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    // 2026-03-xx 按照用户要求：使用 Windows UI Automation API 探测外部应用精细 UI 元素
    IUIAutomation* pAutomation = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER, IID_IUIAutomation, reinterpret_cast<void**>(&pAutomation));
    if (FAILED(hr) || !pAutomation) return;

    IUIAutomationElement* pRoot = nullptr;
    hr = pAutomation->ElementFromHandle(hwnd, &pRoot);
    if (SUCCEEDED(hr) && pRoot) {
        IUIAutomationCondition* pCondition = nullptr;
        pAutomation->CreateTrueCondition(&pCondition); // 采集所有元素
        
        IUIAutomationElementArray* pFound = nullptr;
        // 限制探测深度和数量以保持性能。TreeScope_Descendants 会遍历整个子树
        hr = pRoot->FindAll(TreeScope_Children, pCondition, &pFound); 
        
        if (SUCCEEDED(hr) && pFound) {
            int length = 0;
            pFound->get_Length(&length);
            for (int i = 0; i < length && i < 100; ++i) {
                IUIAutomationElement* pElement = nullptr;
                pFound->GetElement(i, &pElement);
                if (pElement) {
                    RECT rect;
                    if (SUCCEEDED(pElement->get_CurrentBoundingRectangle(&rect))) {
                        QRect qr(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
                        if (qr.width() > 10 && qr.height() > 10) m_detectedRects.append(qr);
                    }
                    pElement->Release();
                }
            }
            pFound->Release();
        }
        if (pCondition) pCondition->Release();
        pRoot->Release();
    }
    pAutomation->Release();
    
    // 对应释放 COM 环境
    CoUninitialize();
}
#endif
void ScreenshotTool::executeOCR() {
    QImage img = generateFinalImage();
    emit screenshotCaptured(img, true);
    cancel();
}

QImage ScreenshotTool::generateFinalImage() {
    QRect r = selectionRect(); if(r.isEmpty()) return QImage();
    QPixmap p = m_screenPixmap.copy(r); QPainter painter(&p); painter.translate(-r.topLeft());
    for(auto* a : std::as_const(m_annotations)) a->draw(painter, m_mosaicPixmap);
    return p.toImage();
}

void ScreenshotTool::autoSaveImage(const QImage& img) {
    if (img.isNull()) return;
    
    QSettings settings("OCR", "Screenshot");
    QString defaultPath = QCoreApplication::applicationDirPath() + "/RPN_screenshot";
    QString savePath = settings.value("savePath", defaultPath).toString();
    
    QDir dir(savePath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QString fileName = QString("RPN_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    QString fullPath = dir.absoluteFilePath(fileName);
    
    if (img.save(fullPath)) {
        // 使用非阻塞彩色反馈告知用户已自动保存 (2026-03-xx 统一修改为 700ms)
        ToolTipOverlay::instance()->showText(QCursor::pos(), QString("[OK] 已自动保存至:\n%1").arg(fullPath), 700);
    } else {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "[ERR] 自动保存失败，请检查路径权限", 700);
    }
}
void ScreenshotTool::keyPressEvent(QKeyEvent* e) { 
    if(e->key() == Qt::Key_Escape) {
        // [MODIFIED] 截图工具是瞬时工具，如果正在编辑文字则退出文字，否则关闭截图
        if (m_textInput->isVisible()) {
            commitTextInput();
            e->accept();
            return;
        }
        cancel();
        return;
    }
    else if(e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter || e->key() == Qt::Key_Space) { if(m_state == ScreenshotState::Editing) confirm(); }
    else if (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_Z) undo();
    else if (e->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) && e->key() == Qt::Key_Z) redo();
    else if (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_O) executeOCR();
    else if (e->key() == Qt::Key_F) pin();
    else if (e->key() == Qt::Key_C) { 
        // 如果放大镜可见，C 键用于复制当前像素色值
        if (m_state == ScreenshotState::Selecting || m_isDragging || m_isDrawing || m_currentTool == ScreenshotToolType::Picker) {
            QColor color = m_screenImage.pixelColor(m_lastMouseMovePos);
            QString colorStr;
            if (m_colorFormatIndex == 0) colorStr = color.name(QColor::HexRgb).toUpper();
            else if (m_colorFormatIndex == 1) colorStr = QString("%1, %2, %3").arg(color.red()).arg(color.green()).arg(color.blue());
            else colorStr = QString("HSL(%1, %2, %3)").arg(color.hslHue() < 0 ? 0 : color.hslHue()).arg(int(color.hslSaturationF()*100)).arg(int(color.lightnessF()*100));
            
            ClipboardMonitor::instance().forceNext();
            QApplication::clipboard()->setText(colorStr);
            // 2026-03-13 按照用户要求：提示时长缩短为 700ms
            ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已复制色值: %1").arg(colorStr), 700);
        } else {
            m_toolbar->selectTool(ScreenshotToolType::Picker); 
        }
    }
    else if (e->key() == Qt::Key_Shift) {
        m_colorFormatIndex = (m_colorFormatIndex + 1) % 3;
        update();
    }
    else if (e->key() == Qt::Key_M) {
        QString coordStr = QString("%1, %2").arg(m_lastMouseMovePos.x()).arg(m_lastMouseMovePos.y());
        ClipboardMonitor::instance().forceNext();
        QApplication::clipboard()->setText(coordStr);
        // 2026-03-13 按照用户要求：提示时长缩短为 700ms
        ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已复制坐标: %1").arg(coordStr), 700);
    }
}
void ScreenshotTool::mouseDoubleClickEvent(QMouseEvent* e) { if(selectionRect().contains(e->pos())) confirm(); }

bool ScreenshotTool::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_textInput && event->type() == QEvent::KeyPress) {
        QKeyEvent* ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Escape) {
            // 文字输入时 Esc 仅退出输入模式，不关闭截图工具。事件在此处被消费，不再传递。
            commitTextInput();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}