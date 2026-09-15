#ifndef DROPLISTVIEW_H
#define DROPLISTVIEW_H

#include <QListView>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QStringList>
#include <QModelIndex>

namespace QuarkMeta {

class DropListView : public QListView {
    Q_OBJECT
public:
    explicit DropListView(QWidget* parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;

private:
    QModelIndex m_currentHoverDropIdx;
    void clearDropHighlight();

signals:
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex);
    void blankSpaceDoubleClicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    bool m_foldersCollapsed = false;
    QRect m_folderHeaderRect;
    int m_folderCount = 0;
};

} // namespace QuarkMeta

#endif // DROPLISTVIEW_H
