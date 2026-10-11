#pragma once

#include <QFrame>
#include <QTreeView>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QStyledItemDelegate>
#include "DropTreeView.h"

namespace QuarkMeta {

class LibraryItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit LibraryItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    void setFilterKeyword(const QString& keyword) { m_filterKeyword = keyword; }
    static QRect getAddButtonRect(const QRect& optionRect);

private:
    QString m_filterKeyword;
};

class LibraryPanel : public QFrame {
    Q_OBJECT

public:
    explicit LibraryPanel(QWidget* parent = nullptr);
    ~LibraryPanel() override = default;

    void loadLibrary();

signals:
    void categoryPathsSelected(const QStringList& paths);
    void requestLocateFile(const QString& path);

private slots:
    void onCategoryClicked(const QModelIndex& index);
    void onCategoryContextMenu(const QPoint& pos);
    void onPathsDroppedToCategory(const QStringList& paths, const QModelIndex& target);

private:
    void initUi();
    void createAndEditCategory(int parentId = 0);
    void filterTreeNodes(const QString& keyword);
    QStandardItem* findItemByNodeId(QStandardItem* parent, int nodeId);

    QVBoxLayout* m_mainLayout = nullptr;
    DropTreeView* m_treeView = nullptr;
    QStandardItemModel* m_model = nullptr;
    LibraryItemDelegate* m_delegate = nullptr;
    QLineEdit* m_filterEdit = nullptr;
    bool m_isLoading = false;
    int m_pendingEditNodeId = 0;
};

} // namespace QuarkMeta
