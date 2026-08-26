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

    /**
     * @brief 物理还原：设置 1px 高亮线的显隐状态
     */
    void setFocusHighlight(bool visible);

    /**
     * @brief 向收藏夹追加项目并防重
     */
    void addFavoriteItem(const QString& path);

    /**
     * @brief 持久化保存收藏夹到 AppConfig
     */
    void saveFavorites();

    /**
     * @brief 从 AppConfig 加载收藏夹
     */
    void loadFavorites();

signals:
    /**
     * @brief 当点击收藏的文件夹时发出，通知主窗口跳转
     */
    void directorySelected(const QString& path);

    /**
     * @brief 当点击收藏的文件时发出，通知主窗口跳转到父目录并高亮文件
     */
    void requestLocateFile(const QString& path);

private slots:
    void onFavoriteClicked(const QModelIndex& index);
    void onFavoriteContextMenu(const QPoint& pos);
    void onPathsDroppedToFavorite(const QStringList& paths, const QModelIndex& target);

private:
    void initUi();

    QVBoxLayout* m_mainLayout = nullptr;
    QWidget* m_focusLine = nullptr;

    DropTreeView* m_favoriteView = nullptr;
    QStandardItemModel* m_favoriteModel = nullptr;
};

} // namespace QuarkMeta
