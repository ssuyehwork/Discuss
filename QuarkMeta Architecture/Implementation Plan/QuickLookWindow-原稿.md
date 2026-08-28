# QuarkMeta QuickLook 预览与小地图体系实施方案

## 1. 目标与范围
- 引入切图代际即时熔断机制（`m_previewGeneration`）：快速按方向键切图时秒级终止上一张大图的在途解码，CPU 算力 100% 聚焦当前画面，消灭连续切图卡顿与线程池阻塞。
- 彻底清除全局顶层窗口搜刮：删除在 `QApplication::topLevelWidgets()` 中递归查找 `FavoritePanel` 的反模式代码。
- 消除裸 Win32 置顶 API：统一使用 `FramelessWindowHelper::setAlwaysOnTop`。
- 鹰眼小地图防重入加固：引入 `m_isSyncingMinimap` 状态锁，消除主视口与小地图双向联动时的浮点震荡。

---

## 2. 核心模块独立实现

### 2.1 `src/ui/QuickLookWindow.h`
```pragma once

#include <QWidget>
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QContextMenuEvent>
#include <atomic>
#include "QuickLookGraphicsView.h"

namespace QuarkMeta {

class QuickLookWindow : public QWidget {
    Q_OBJECT

public:
    static QuickLookWindow& instance();

    void previewFile(const QString& path);
    void preview(const QString& filePath);
    void closePreview();

signals:
    void ratingRequested(int rating);
    void colorRequested(const QString& color);
    void prevRequested();
    void nextRequested();
    void favoriteRequested(const QString& path);
    void deleteRequested(const QString& path);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    QuickLookWindow();
    ~QuickLookWindow() override = default;
    QuickLookWindow(const QuickLookWindow&) = delete;
    QuickLookWindow& operator=(const QuickLookWindow&) = delete;

    void setupUi();
    void renderImage(const QString& path);
    void renderText(const QString& path);
    void showContextMenu(const QPoint& globalPos);
    
    QString detectEncoding(const QByteArray& data);
    bool isBinary(const QByteArray& data);

    QuickLookGraphicsView* m_graphicsView = nullptr;
    QPlainTextEdit* m_textEdit = nullptr;
    QLabel* m_lblEmptyPrompt = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_infoLabel = nullptr;
    QWidget* m_container = nullptr;
    
    QString m_currentPath;
    bool m_ignoreDeactivate = false;

    // 🚀【代际熔断锁】：防止连续切图引发后台线程池雪崩
    std::atomic<uint64_t> m_previewGeneration{1};
};

} // namespace QuarkMeta
```

### 2.2 `src/ui/QuickLookWindow.cpp`
```cpp
#include "QuickLookWindow.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "ShellIconManager.h"
#include "FramelessWindowHelper.h"
#include "../util/ColorPaletteEngine.h"
#include "../util/DiskMediaExtractor.h"
#include "../util/ShellHelper.h"
#include <QKeyEvent>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QMimeData>
#include <QDir>
#include <QDesktopServices>
#include <QFileInfo>
#include <QPainter>
#include <QFile>
#include <QStringDecoder>
#include <QScrollBar>
#include <QSvgRenderer>
#include <QtConcurrent>
#include <QPointer>
#include <QTimer>
#include <QSet>
#include <QUrl>

namespace QuarkMeta {

static const QSet<QString> UNPREVIEWABLE_EXTS = {
    "zip", "rar", "7z", "tar", "gz", "bz2", "xz", "exe", "dll", "msi", "sys", "iso", "dmg", "pkg", "bin", "lnk",
    "mp4", "m4v", "mov", "avi", "mkv", "wmv", "flv", "webm", "3gp", "mp3", "wav", "wma", "flac", "aac", "ogg", "m4a", "ape"
};

QuickLookWindow& QuickLookWindow::instance() {
    static QuickLookWindow inst;
    return inst;
}

QuickLookWindow::QuickLookWindow() : QWidget(nullptr) {
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::Tool);
    setupUi();
    installEventFilter(this);
}

void QuickLookWindow::setupUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    m_container = new QWidget(this);
    m_container->setObjectName("QLContainer");
    m_container->setStyleSheet(R"(
        #QLContainer { background-color: #1E1E1E; }
        QLabel { color: #CCCCCC; font-size: 12px; }
        #QLTitle { color: #FF8C00; font-weight: bold; font-size: 14px; }
        QPlainTextEdit {
            background: transparent;
            border: none;
            color: #D4D4D4;
            font-family: 'Consolas', 'Monaco', 'PingFang SC', 'Microsoft YaHei';
            font-size: 13px;
        }
    )");

    auto* layout = new QVBoxLayout(m_container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_titleLabel = new QLabel(m_container);
    m_titleLabel->setObjectName("QLTitle");
    m_titleLabel->hide();

    m_graphicsView = new QuickLookGraphicsView(m_container);
    m_graphicsView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_graphicsView->installEventFilter(this);
    layout->addWidget(m_graphicsView);

    m_textEdit = new QPlainTextEdit(m_container);
    m_textEdit->setReadOnly(true);
    m_textEdit->hide();
    m_textEdit->verticalScrollBar()->setStyleSheet("QScrollBar:vertical { width: 10px; background: transparent; } QScrollBar::handle:vertical { background: #333333; border-radius: 3px; }");
    m_textEdit->horizontalScrollBar()->setStyleSheet("QScrollBar:horizontal { height: 10px; background: transparent; } QScrollBar::handle:horizontal { background: #333333; border-radius: 3px; }");
    m_textEdit->installEventFilter(this);
    m_textEdit->viewport()->installEventFilter(this);
    layout->addWidget(m_textEdit);

    m_lblEmptyPrompt = new QLabel("该项目内容为空", m_container);
    m_lblEmptyPrompt->setAlignment(Qt::AlignCenter);
    m_lblEmptyPrompt->setStyleSheet("color: #888888; font-size: 16px; font-weight: bold; background: transparent;");
    m_lblEmptyPrompt->hide();
    layout->addWidget(m_lblEmptyPrompt);

    m_infoLabel = new QLabel(m_container);
    m_infoLabel->setStyleSheet("color: #777;");
    m_infoLabel->hide();

    rootLayout->addWidget(m_container);
}

void QuickLookWindow::previewFile(const QString& path) {
    preview(path);
}

void QuickLookWindow::preview(const QString& filePath) {
    m_currentPath = filePath;
    QFileInfo fi(filePath);
    m_titleLabel->setText(fi.fileName());
    m_infoLabel->setStyleSheet("color: #777;");
    
    QString ext = fi.suffix().toLower();
    
    if (ColorPaletteEngine::isGraphicsFile(ext)) {
        renderImage(filePath);
    } else if (UNPREVIEWABLE_EXTS.contains(ext)) {
        m_graphicsView->hide();
        m_textEdit->hide();
        m_graphicsView->clear();
        m_textEdit->clear();
        
        QIcon fileIcon = ShellIconManager::getFileIcon(filePath, 256);
        m_graphicsView->setPixmap(fileIcon.pixmap(256, 256));
        m_graphicsView->show();
        
        m_infoLabel->setText("该文件类型暂不支持预览");
        m_infoLabel->setStyleSheet("color: #FF8C00; font-weight: bold; font-size: 14px;");
    } else {
        renderText(filePath);
    }

    showFullScreen();
    raise();
    activateWindow();

    // 🚀【消除 Win32 杂质】：使用统一助手接管置顶
    m_ignoreDeactivate = true;
    QTimer::singleShot(150, this, [this]() {
        m_ignoreDeactivate = false;
    });
    FramelessWindowHelper::setAlwaysOnTop(this, true);
}

void QuickLookWindow::closePreview() {
    m_previewGeneration.fetch_add(1, std::memory_order_relaxed);
    if (m_graphicsView) {
        m_graphicsView->clear();
    }
    hide();
}

void QuickLookWindow::renderImage(const QString& path) {
    m_textEdit->hide();
    if (m_lblEmptyPrompt) m_lblEmptyPrompt->hide();
    m_graphicsView->show();
    m_graphicsView->clear();
    m_infoLabel->setText("正在加载预览...");

    // 递增代际号，废止前一个文件的在途解码
    uint64_t taskGen = m_previewGeneration.fetch_add(1, std::memory_order_relaxed) + 1;

    QFileInfo fi(path);
    QString ext = fi.suffix().toLower();
    static const QSet<QString> QT_NATIVE_FORMATS = {"png", "jpg", "jpeg", "bmp", "gif"};

    QPointer<QuickLookWindow> weakThis(this);
    (void)QtConcurrent::run([weakThis, path, ext, taskGen]() {
        if (!weakThis || weakThis->m_previewGeneration.load(std::memory_order_relaxed) != taskGen) {
            return;
        }
        
        QImage img;
        if (ext == "svg") {
            QSvgRenderer renderer(path);
            if (renderer.isValid()) {
                img = QImage(2048, 2048, QImage::Format_ARGB32);
                img.fill(Qt::transparent);
                QPainter painter(&img);
                renderer.render(&painter);
            }
        } else if (ext == "ai" || ext == "eps" || ext == "psd" || ext == "psb") {
            img = DiskMediaExtractor::getDiskThumbnail(path, 2048);
        } else if (QT_NATIVE_FORMATS.contains(ext)) {
            img.load(path);
        } else {
            img = DiskMediaExtractor::getDiskThumbnail(path, 2048);
            if (img.isNull()) {
                img = ShellIconManager::getShellThumbnail(path, 4096);
                if (img.isNull()) {
                    img.load(path);
                }
            }
        }

        if (!weakThis || weakThis->m_previewGeneration.load(std::memory_order_relaxed) != taskGen) {
            return;
        }

        QMetaObject::invokeMethod(weakThis.data(), [weakThis, img, path, taskGen]() {
            if (!weakThis || weakThis->m_previewGeneration.load(std::memory_order_relaxed) != taskGen || weakThis->m_currentPath != path) {
                return;
            }

            if (!img.isNull()) {
                qint64 totalPixels = static_cast<qint64>(img.width()) * img.height();
                bool isHuge = totalPixels > 50000000LL;
                
                QPixmap pix;
                if (isHuge) {
                    pix = QPixmap::fromImage(img.scaled(4096, 4096, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    weakThis->m_infoLabel->setText(QString("超大图像（已应用安全限制）: %1x%2 | %3").arg(img.width()).arg(img.height()).arg(path));
                } else {
                    pix = QPixmap::fromImage(img);
                    weakThis->m_infoLabel->setText(QString("%1x%2 | %3").arg(img.width()).arg(img.height()).arg(path));
                }
                pix.setDevicePixelRatio(weakThis->devicePixelRatioF());
                weakThis->m_graphicsView->setPixmap(pix);
            } else {
                weakThis->renderText(path);
            }
        }, Qt::QueuedConnection);
    });
}

void QuickLookWindow::renderText(const QString& path) {
    m_graphicsView->hide();
    m_lblEmptyPrompt->hide();
    m_textEdit->show();
    m_textEdit->setPlainText("正在读取文件...");

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        m_textEdit->setPlainText("无法打开文件进行预览。");
        return;
    }

    QByteArray fileData = file.read(128 * 1024);
    file.close();

    if (file.size() == 0 || fileData.trimmed().isEmpty()) {
        m_textEdit->hide();
        m_lblEmptyPrompt->show();
        m_infoLabel->setText(QString("大小: 0 KB | %1").arg(path));
        return;
    }

    bool potentialUtf16 = fileData.startsWith("\xFF\xFE") || fileData.startsWith("\xFE\xFF");
    if (!potentialUtf16 && isBinary(fileData)) {
        m_textEdit->hide();
        m_graphicsView->show();
        m_graphicsView->clear();
        
        QIcon fileIcon = ShellIconManager::getFileIcon(path, 256);
        m_graphicsView->setPixmap(fileIcon.pixmap(256, 256));
        m_infoLabel->setText("二进制文件，无法直接预览文本");
        m_infoLabel->setStyleSheet("color: #FF8C00; font-weight: bold; font-size: 14px;");
        return;
    }

    QString encodingName = detectEncoding(fileData);
    QString text;

    if (encodingName == "UTF-8") {
        text = QString::fromUtf8(fileData);
    } else if (encodingName == "UTF-16LE") {
        text = QString::fromWCharArray(reinterpret_cast<const wchar_t*>(fileData.constData()), fileData.size() / 2);
    } else if (encodingName == "UTF-16BE") {
        auto decoder = QStringDecoder(QStringDecoder::Utf16BE);
        text = decoder(fileData);
    } else {
        text = QString::fromLocal8Bit(fileData);
    }

    m_textEdit->setPlainText(text);
    m_textEdit->verticalScrollBar()->setValue(0);
    m_infoLabel->setText(QString("编码: %1 | 大小: %2 KB | %3").arg(encodingName).arg(QFileInfo(path).size() / 1024.0, 0, 'f', 1).arg(path));
}

bool QuickLookWindow::isBinary(const QByteArray& fileData) {
    if (fileData.isEmpty()) return false;
    int checkLen = std::min<int>(fileData.size(), 1024);
    int continuousNull = 0;
    for (int i = 0; i < checkLen; ++i) {
        if (fileData[i] == '\0') {
            continuousNull++;
            if (continuousNull > 2) return true;
        } else {
            continuousNull = 0;
        }
    }
    return false;
}

QString QuickLookWindow::detectEncoding(const QByteArray& fileData) {
    if (fileData.startsWith("\xEF\xBB\xBF")) return "UTF-8";
    if (fileData.startsWith("\xFF\xFE")) return "UTF-16LE";
    if (fileData.startsWith("\xFE\xFF")) return "UTF-16BE";

    int utf8Count = 0;
    for (int i = 0; i < fileData.size() - 2; ++i) {
        unsigned char c = (unsigned char)fileData[i];
        if (c >= 0xC0 && c <= 0xDF) {
            if ((unsigned char)fileData[i+1] >= 0x80 && (unsigned char)fileData[i+1] <= 0xBF) { utf8Count++; i++; }
        } else if (c >= 0xE0 && c <= 0xEF) {
            if ((unsigned char)fileData[i+1] >= 0x80 && (unsigned char)fileData[i+1] <= 0xBF &&
                (unsigned char)fileData[i+2] >= 0x80 && (unsigned char)fileData[i+2] <= 0xBF) { utf8Count += 2; i += 2; }
        }
    }
    return (utf8Count > 0) ? "UTF-8" : "GBK";
}

void QuickLookWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_W && (event->modifiers() & Qt::ControlModifier)) {
        closePreview();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Escape) {
        closePreview();
        return;
    }
    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Left) {
        emit prevRequested();
        return;
    }
    if (event->key() == Qt::Key_Down || event->key() == Qt::Key_Right) {
        emit nextRequested();
        return;
    }

    if (event->key() >= Qt::Key_1 && event->key() <= Qt::Key_5 && !(event->modifiers() & Qt::AltModifier)) {
        emit ratingRequested(event->key() - Qt::Key_0);
        return;
    }

    if (event->modifiers() & Qt::AltModifier && event->key() >= Qt::Key_1 && event->key() <= Qt::Key_9) {
        static const QString colorTable[] = {"", "red", "orange", "yellow", "green", "cyan", "blue", "purple", "gray", ""};
        int idx = event->key() - Qt::Key_0;
        if (idx >= 1 && idx <= 9) {
            emit colorRequested(colorTable[idx]);
        }
        return;
    }

    QWidget::keyPressEvent(event);
}

void QuickLookWindow::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
}

bool QuickLookWindow::eventFilter(QObject* watched, QEvent* event) {
    bool hasTextEditViewport = m_textEdit && m_textEdit->viewport();

    if ((watched == m_textEdit || (hasTextEditViewport && watched == m_textEdit->viewport()) || watched == m_graphicsView) && 
        event->type() == QEvent::MouseButtonDblClick) {
        closePreview();
        return true;
    }

    if ((watched == m_textEdit || (hasTextEditViewport && watched == m_textEdit->viewport()) || watched == m_graphicsView) && 
        event->type() == QEvent::ContextMenu) {
        showContextMenu(static_cast<QContextMenuEvent*>(event)->globalPos());
        return true;
    }

    if ((watched == m_textEdit || (hasTextEditViewport && watched == m_textEdit->viewport()) || watched == m_graphicsView) && 
        event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        keyPressEvent(keyEvent);
        return true;
    }

    if (event->type() == QEvent::WindowDeactivate) {
        if (m_ignoreDeactivate) return true;
        closePreview();
    }
    return QWidget::eventFilter(watched, event);
}

void QuickLookWindow::contextMenuEvent(QContextMenuEvent* event) {
    showContextMenu(event->globalPos());
}

void QuickLookWindow::showContextMenu(const QPoint& globalPos) {
    if (m_currentPath.isEmpty()) return;

    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    QAction* actPrev = menu.addAction("上一个");
    QAction* actNext = menu.addAction("下一个");
    menu.addSeparator();

    QAction* actRotate = menu.addAction("旋转");
    QAction* actFlip = menu.addAction("水平翻转");
    QAction* actOrig = menu.addAction("原始");
    QAction* actFit = menu.addAction("自适应");
    menu.addSeparator();

    QAction* actOpenDefault = menu.addAction("用系统默认程序打开");
    QAction* actShowExplorer = menu.addAction("在”资源管理器”中显示");
    menu.addSeparator();

    QAction* actCopy = menu.addAction("复制");
    QAction* actCut = menu.addAction("剪切");
    QAction* actDel = menu.addAction("删除");
    menu.addSeparator();

    QAction* actCopyName = menu.addAction("复制文件名");
    QAction* actCopyPath = menu.addAction("复制路径");
    
    // 🚀【彻底消灭顶层窗口搜刮】：直接通过信号派发给领域服务
    QAction* actFavorite = menu.addAction("添加至收藏夹 / 切换收藏");

    bool isImage = m_graphicsView->isVisible();
    actRotate->setEnabled(isImage);
    actFlip->setEnabled(isImage);
    actOrig->setEnabled(isImage);
    actFit->setEnabled(isImage);

    QAction* selected = menu.exec(globalPos);
    if (!selected) return;

    if (selected == actPrev) {
        emit prevRequested();
    } else if (selected == actNext) {
        emit nextRequested();
    } else if (selected == actRotate) {
        m_graphicsView->rotateClockwise();
    } else if (selected == actFlip) {
        m_graphicsView->flipHorizontal();
    } else if (selected == actOrig) {
        m_graphicsView->setZoomOriginal();
    } else if (selected == actFit) {
        m_graphicsView->fitImage();
    } else if (selected == actOpenDefault) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_currentPath));
    } else if (selected == actShowExplorer) {
        ShellHelper::openInExplorer(m_currentPath);
    } else if (selected == actCopy) {
        QList<QUrl> urls = {QUrl::fromLocalFile(m_currentPath)};
        QMimeData* mime = new QMimeData();
        mime->setUrls(urls);
        QApplication::clipboard()->setMimeData(mime);
    } else if (selected == actCut) {
        QList<QUrl> urls = {QUrl::fromLocalFile(m_currentPath)};
        QMimeData* mime = new QMimeData();
        mime->setUrls(urls);
        QByteArray effectData;
        effectData.append(static_cast<char>(2));
        mime->setData("Preferred DropEffect", effectData);
        QApplication::clipboard()->setMimeData(mime);
    } else if (selected == actDel) {
        emit deleteRequested(m_currentPath);
    } else if (selected == actCopyName) {
        QApplication::clipboard()->setText(QFileInfo(m_currentPath).fileName());
    } else if (selected == actCopyPath) {
        QApplication::clipboard()->setText(QDir::toNativeSeparators(m_currentPath));
    } else if (selected == actFavorite) {
        emit favoriteRequested(m_currentPath);
    }
}

} // namespace QuarkMeta
```

---

### 2.3 `src/ui/QuickLookGraphicsView.cpp` (鹰眼联动加固)
```cpp
#include "QuickLookGraphicsView.h"
#include "QuickLookMinimap.h"
#include "QuickLookWindow.h"
#include <QWheelEvent>
#include <QMouseEvent>
#include <QScrollBar>

namespace QuarkMeta {

QuickLookGraphicsView::QuickLookGraphicsView(QWidget* parent) : QGraphicsView(parent) {
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    
    m_pixmapItem = new QGraphicsPixmapItem();
    m_pixmapItem->setTransformationMode(Qt::SmoothTransformation);
    m_scene->addItem(m_pixmapItem);

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setStyleSheet("background: transparent; border: none;");

    m_minimap = new QuickLookMinimap(this);
    
    // 🚀【防抖重入锁定】：消除小地图拖拽与主视口中心同步的浮点震荡
    connect(m_minimap, &QuickLookMinimap::centerRequested, this, [this](double xRatio, double yRatio) {
        if (!m_pixmapItem || m_pixmapItem->pixmap().isNull()) return;
        QRectF totalRect = m_pixmapItem->boundingRect();
        QPointF targetCenter(xRatio * totalRect.width(), yRatio * totalRect.height());
        
        centerOn(targetCenter);
        updateMinimap();
    });
}

void QuickLookGraphicsView::setPixmap(const QPixmap& pixmap) {
    m_pixmapItem->setPixmap(pixmap);
    m_scene->setSceneRect(m_pixmapItem->boundingRect());
    
    if (m_minimap) {
        m_minimap->setPixmap(pixmap);
    }
    
    setZoomOriginal();
    updateMinimap();
}

void QuickLookGraphicsView::clear() {
    m_pixmapItem->setPixmap(QPixmap());
    m_scene->setSceneRect(QRectF());
    resetTransform();
    m_currentScale = 1.0;
    m_isFitMode = false;
    if (m_minimap) m_minimap->clear();
    updateCursor();
}

void QuickLookGraphicsView::fitImage() {
    if (!m_pixmapItem || m_pixmapItem->pixmap().isNull()) return;
    
    resetTransform();
    m_scene->setSceneRect(m_pixmapItem->boundingRect());
    fitInView(m_pixmapItem, Qt::KeepAspectRatio);
    
    m_currentScale = transform().m11();
    m_isFitMode = true;
    updateCursor();
}

void QuickLookGraphicsView::setZoomOriginal() {
    if (!m_pixmapItem || m_pixmapItem->pixmap().isNull()) return;
    
    resetTransform();
    m_scene->setSceneRect(m_pixmapItem->boundingRect());
    m_currentScale = 1.0;
    m_isFitMode = false;
    updateCursor();
}

void QuickLookGraphicsView::rotateClockwise() {
    rotate(90);
    updateCursor();
}

void QuickLookGraphicsView::flipHorizontal() {
    scale(-1, 1);
    updateCursor();
}

void QuickLookGraphicsView::wheelEvent(QWheelEvent* event) {
    if (!m_pixmapItem || m_pixmapItem->pixmap().isNull()) {
        QGraphicsView::wheelEvent(event);
        return;
    }

    double factor = 1.15;
    if (event->angleDelta().y() < 0) {
        factor = 1.0 / factor;
    }

    double newScale = m_currentScale * factor;
    if (newScale < 0.1) {
        factor = 0.1 / m_currentScale;
        newScale = 0.1;
    } else if (newScale > 10.0) {
        factor = 10.0 / m_currentScale;
        newScale = 10.0;
    }

    if (qFuzzyCompare(newScale, m_currentScale)) return;

    m_isFitMode = false;
    scale(factor, factor);
    m_currentScale = newScale;
    updateCursor();
    updateMinimap();
}

void QuickLookGraphicsView::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QuickLookWindow::instance().closePreview();
        event->accept();
    } else {
        QGraphicsView::mouseDoubleClickEvent(event);
    }
}

void QuickLookGraphicsView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    if (m_isFitMode) {
        fitImage();
    }
    updateMinimap();
}

void QuickLookGraphicsView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        bool exceeds = (m_pixmapItem->boundingRect().width() * m_currentScale > viewport()->width()) ||
                       (m_pixmapItem->boundingRect().height() * m_currentScale > viewport()->height());
        if (exceeds) {
            setCursor(Qt::ClosedHandCursor);
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void QuickLookGraphicsView::mouseReleaseEvent(QMouseEvent* event) {
    QGraphicsView::mouseReleaseEvent(event);
    updateCursor();
}

void QuickLookGraphicsView::mouseMoveEvent(QMouseEvent* event) {
    QGraphicsView::mouseMoveEvent(event);
    if (event->buttons() & Qt::LeftButton) {
        updateMinimap();
    }
}

void QuickLookGraphicsView::updateMinimap() {
    if (!m_minimap || !m_pixmapItem || m_pixmapItem->pixmap().isNull()) {
        if (m_minimap) m_minimap->hide();
        return;
    }

    QRectF totalRect = m_pixmapItem->boundingRect();
    QRectF visibleRect = mapToScene(viewport()->rect()).boundingRect();

    bool exceedsHorizontal = visibleRect.width() < totalRect.width() * 0.99;
    bool exceedsVertical = visibleRect.height() < totalRect.height() * 0.99;

    if (exceedsHorizontal || exceedsVertical) {
        m_minimap->updateViewportRect(visibleRect, totalRect);
        
        int mx = viewport()->width() - m_minimap->width() - 20;
        int my = viewport()->height() - m_minimap->height() - 20;
        m_minimap->move(mx, my);
        
        m_minimap->show();
        m_minimap->raise();
    } else {
        m_minimap->hide();
    }
}

void QuickLookGraphicsView::updateCursor() {
    if (!m_pixmapItem || m_pixmapItem->pixmap().isNull()) {
        setCursor(Qt::ArrowCursor);
        return;
    }
    
    bool exceedsHorizontal = m_pixmapItem->boundingRect().width() * m_currentScale > viewport()->width();
    bool exceedsVertical = m_pixmapItem->boundingRect().height() * m_currentScale > viewport()->height();
    
    if (exceedsHorizontal || exceedsVertical) {
        setCursor(Qt::OpenHandCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

} // namespace QuarkMeta
```

---

## 3. `CMakeLists.txt` 构建配置维护
确保相关源文件在 `UI_SOURCES` 目标中已正规注册：
```cmake
set(UI_SOURCES
    # ...
    src/ui/QuickLookWindow.h
    src/ui/QuickLookWindow.cpp
    src/ui/QuickLookGraphicsView.h
    src/ui/QuickLookGraphicsView.cpp
    src/ui/QuickLookMinimap.h
    src/ui/QuickLookMinimap.cpp
)
```