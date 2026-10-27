#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

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

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);
    void onContextMenuRequested(const QPoint& pos);
    void onMarkedFoldersChanged();

private:
    void initUi();
    QTreeWidgetItem* createMarkedItem(const QString& path);

    QVBoxLayout* m_mainLayout = nullptr;
    QTreeWidget* m_treeWidget = nullptr;
};

} // namespace QuarkMeta
