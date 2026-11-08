#ifndef SCREENSHOTTOOLBAR_H
#define SCREENSHOTTOOLBAR_H

#include <QWidget>
#include <QPushButton>
#include <QBoxLayout>
#include <QMap>
#include <QButtonGroup>
#include <QFrame>
#include <QColor>
#include <functional>
#include "ScreenshotShapes.h"

class ScreenshotTool;

class IconFactory {
public:
    static QIcon createArrowStyleIcon(ArrowStyle style);
};

class SelectionInfoBar : public QWidget {
    Q_OBJECT
    friend class ScreenshotToolbar;
public:
    explicit SelectionInfoBar(QWidget* parent = nullptr);
    void updateInfo(const QRect& rect);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QString m_text;
};

class ScreenshotToolbar : public QWidget {
    Q_OBJECT
public:
    explicit ScreenshotToolbar(ScreenshotTool* tool);
    void addToolButton(QBoxLayout* layout, ScreenshotToolType type, const QString& iconType, const QString& tip);
    void addActionButton(QBoxLayout* layout, const QString& iconName, const QString& tip, std::function<void()> func);
    void selectTool(ScreenshotToolType type);
    void updateArrowButtonIcon(ArrowStyle style);
    void addRecentColor(const QColor& c, bool save = true);
    void syncColorSelection(const QColor& color);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void createOptionWidget();
    void showArrowMenu();

public:
    ScreenshotTool* m_tool;
    QMap<ScreenshotToolType, QPushButton*> m_buttons;
    QWidget* m_optionWidget = nullptr;
    QPushButton* m_arrowStyleBtn = nullptr;
    QPushButton* m_outlineBtn = nullptr;
    QPushButton* m_solidBtn = nullptr;
    QPushButton* m_wheelBtn = nullptr;
    QBoxLayout* m_recentLayout = nullptr;
    QWidget* m_textOptionWidget = nullptr;
    QWidget* m_sizeOptionWidget = nullptr;
    QFrame* m_textDivider = nullptr;
    QFrame* m_horizontalDivider = nullptr;
    QPushButton* m_boldBtn = nullptr;
    QPushButton* m_italicBtn = nullptr;
    QPushButton* m_removeColorBtn = nullptr;

    bool m_isDragging = false;
    QPoint m_dragPosition;

    QButtonGroup* m_colorGroup = nullptr;
    QButtonGroup* m_sizeGroup = nullptr;
    QPushButton* m_paletteBtn = nullptr;
};

#endif // SCREENSHOTTOOLBAR_H
