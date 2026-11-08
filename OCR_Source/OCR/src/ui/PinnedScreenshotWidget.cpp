#include "PinnedScreenshotWidget.h"
#include "IconHelper.h"

#include <QPainter>
#include <QMenu>
#include <QApplication>
#include <QClipboard>
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>
#include <QFileDialog>

PinnedScreenshotWidget::PinnedScreenshotWidget(const QPixmap& pixmap, const QRect& screenRect, QWidget* parent)
    : QWidget(nullptr, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool), m_pixmap(pixmap)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setFixedSize(pixmap.size() / pixmap.devicePixelRatio());
    move(screenRect.topLeft());
}

void PinnedScreenshotWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.drawPixmap(rect(), m_pixmap);
    p.setPen(QPen(QColor(0, 120, 255, 200), 2));
    p.drawRect(rect().adjusted(0, 0, -1, -1));
}

void PinnedScreenshotWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) m_dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
}

void PinnedScreenshotWidget::mouseMoveEvent(QMouseEvent* e) {
    if (e->buttons() & Qt::LeftButton) move(e->globalPosition().toPoint() - m_dragPos);
}

void PinnedScreenshotWidget::mouseDoubleClickEvent(QMouseEvent*) { close(); }

void PinnedScreenshotWidget::contextMenuEvent(QContextMenuEvent* e) {
    QMenu menu(this);
    IconHelper::setupMenu(&menu);
    menu.addAction("复制", [this](){
                QApplication::clipboard()->setPixmap(m_pixmap);
    });
    menu.addAction("保存", [this](){
        QSettings settings("OCR", "Screenshot");
        QString defaultPath = QCoreApplication::applicationDirPath() + "/RPN_screenshot";
        QString saveDir = settings.value("savePath", defaultPath).toString();
        QString defaultFile = QDir(saveDir).filePath(QString("RPN_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")));
        QString f = QFileDialog::getSaveFileName(this, "保存截图", defaultFile, "PNG(*.png)");
        if(!f.isEmpty()) m_pixmap.save(f);
    });
    menu.addSeparator();
    menu.addAction("关闭", this, &QWidget::close);
    menu.exec(e->globalPos());
}
