#pragma once

#include <QFrame>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include "DropTreeView.h"

namespace QuarkMeta {

class FavoriteItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit FavoriteItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

/**
 * @brief 独立收藏夹面板（主界面第二栏）
 * 垂直贯通独占，专门展示常用快捷文件/文件夹
 */
class FavoritePanel : public QFrame {
    Q_OBJECT

public:
    explicit FavoritePanel(QWidget* parent = nullptr);
    ~FavoritePanel() override = default;

    void setFocusHighlight(bool visible);
    bool containsPath(const QString& path) const;
    void removeFavoriteItem(const QString& path);
    void addFavoriteItem(const QString& path);
    void loadFavorites();
    void saveFavorites();

signals:
    void directorySelected(const QString& path);
    void requestLocateFile(const QString& path);

private slots:
    void onFavoriteClicked(const QModelIndex& index);
    void onFavoriteContextMenu(const QPoint& pos);
    void onPathsDroppedToFavorite(const QStringList& paths, const QModelIndex& target);

private:
    void initUi();
    void updateItemThumbnail(const QString& path, const QPixmap& pix);

    QVBoxLayout* m_mainLayout = nullptr;
    DropTreeView* m_favoriteView = nullptr;
    QStandardItemModel* m_favoriteModel = nullptr;
    bool m_isLoading = false;
};

} // namespace QuarkMeta