#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>

namespace QuarkMeta {

class MarkedPanel : public QWidget {
    Q_OBJECT

public:
    explicit MarkedPanel(QWidget* parent = nullptr);
    ~MarkedPanel() override = default;

    void loadMarkedFolders();
    void filterTreeNodes(const QString& keyword);

signals:
    void requestLocateFolder(const QString& path);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);
    void onContextMenuRequested(const QPoint& pos);
    void onMarkedFoldersChanged();

private:
    void initUi();
    QTreeWidgetItem* createMarkedItem(const QString& path, bool active);
    void handleDropPaths(const QMimeData* mimeData, const QPoint& pos, bool isViewportPos);

    QVBoxLayout* m_mainLayout = nullptr;
    QTreeWidget* m_treeWidget = nullptr;
};

} // namespace QuarkMeta
