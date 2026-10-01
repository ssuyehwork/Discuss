#pragma once

#include <QFrame>
#include <QTreeView>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include "DropTreeView.h"

namespace QuarkMeta {

class LibraryPanel : public QFrame {
    Q_OBJECT

public:
    explicit LibraryPanel(QWidget* parent = nullptr);
    ~LibraryPanel() override = default;

    void loadLibrary();

signals:
    void categoryPathsSelected(const QStringList& paths);

private slots:
    void onCategoryClicked(const QModelIndex& index);
    void onCategoryContextMenu(const QPoint& pos);
    void onPathsDroppedToCategory(const QStringList& paths, const QModelIndex& target);

private:
    void initUi();
    void createAndEditCategory(int parentId = 0);

    QVBoxLayout* m_mainLayout = nullptr;
    DropTreeView* m_treeView = nullptr;
    QStandardItemModel* m_model = nullptr;
    bool m_isLoading = false;
};

} // namespace QuarkMeta
