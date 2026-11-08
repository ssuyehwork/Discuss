#ifndef SCREENSHOTTOOL_H
#define SCREENSHOTTOOL_H

#include <QWidget>
#include <QMouseEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QPainter>
#include <QLineEdit>
#include <QList>
#include <QAbstractItemView>

#include "ScreenshotShapes.h"
#include "ScreenshotToolbar.h"
#include "PinnedScreenshotWidget.h"

class ScreenshotTool : public QWidget {
    Q_OBJECT
    friend class ScreenshotToolbar;
public:
    explicit ScreenshotTool(QWidget* parent = nullptr);
    ~ScreenshotTool() override;
    
    void setDrawColor(const QColor& color);
    void setDrawWidth(int width);
    void setArrowStyle(ArrowStyle style);
    void setFillEnabled(bool enabled);
    void setBold(bool bold);
    void setItalic(bool italic);
    void setFontFamily(const QString& family);
    void setFontSize(int size);
    
    void updateToolbarPosition();
    void undo();
    void redo();
    void copyToClipboard();
    void save();
    void confirm();
    void pin();
    void cancel(); 
    void executeOCR();

    void setImmediateOCRMode(bool enabled) { m_isImmediateOCR = enabled; }

signals:
    void screenshotCaptured(const QImage& image, bool isOcr = false);
    void screenshotCanceled();

protected:
    void paintEvent(QPaintEvent*) override;
    void wheelEvent(QWheelEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

public slots:
    void setTool(ScreenshotToolType type);

private:
    QRect selectionRect() const;
    QList<QRect> getHandleRects() const;
    int getHandleAt(const QPoint& pos) const;
    void updateCursor(const QPoint& pos);
    void showTextInput(const QPoint& pos);
    void commitTextInput();
    QImage generateFinalImage();
    void autoSaveImage(const QImage& img);
    void detectWindows();
    void collectQtWidgets(QWidget* parent);
    void detectItemViewRects(QAbstractItemView* view);
#ifdef Q_OS_WIN
    void collectUIAElements(HWND hwnd);
#endif
    void drawMagnifier(QPainter& p, const QPoint& pos);

    QPixmap m_screenPixmap;
    QImage m_screenImage;
    QPixmap m_mosaicPixmap;
    
    ScreenshotState m_state = ScreenshotState::Selecting;
    ScreenshotToolType m_currentTool = ScreenshotToolType::None;
    
    QList<QRect> m_detectedRects;
    QRect m_highlightedRect;

    QPoint m_startPoint, m_endPoint;
    QPoint m_dragOrigin;
    bool m_isDragging = false;
    int m_dragHandle = -1; 
    bool m_isConfirmed = false;

    QList<BaseShape*> m_annotations;
    QList<BaseShape*> m_redoStack;
    BaseShape* m_activeShape = nullptr;
    BaseShape* m_hoveredShape = nullptr;
    int m_editHandle = -1;
    DrawingAnnotation m_currentAnnotation;
    bool m_isDrawing = false;

    ScreenshotToolbar* m_toolbar = nullptr;
    SelectionInfoBar* m_infoBar = nullptr;
    QLineEdit* m_textInput = nullptr;
    QPoint m_lastMouseMovePos;
    int m_colorFormatIndex = 0; // 0: Hex, 1: RGB, 2: HSL
    bool m_isImmediateOCR = false;

    QColor m_currentColor = QColor(255, 50, 50); 
    int m_currentStrokeWidth = 3; 
    ArrowStyle m_currentArrowStyle = ArrowStyle::SolidSingle;
    bool m_fillEnabled = false;
    LineStyle m_currentLineStyle = LineStyle::Solid;

    QString m_currentFontFamily = "Microsoft YaHei";
    int m_currentFontSize = 14;
    bool m_currentBold = true;
    bool m_currentItalic = false;
};

#endif // SCREENSHOTTOOL_H
