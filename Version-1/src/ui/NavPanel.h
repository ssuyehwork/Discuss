#pragma once

#include <QWidget>
#include <QTreeView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QVBoxLayout>
#include <QDir>

namespace QuarkMeta {

/**
 * @brief 导航面板（面板一）
 * 使用 QTreeView + QFileSystemModel 实现文件夹树导航
 */
class NavPanel : public QFrame {
    Q_OBJECT

public:
    explicit NavPanel(QWidget* parent = nullptr);
    ~NavPanel() override = default;

    // 2026-04-12 关键修复：延迟初始化数据模型
    void deferredInit();

    /**
     * @brief 物理还原：设置 1px 翠绿高亮线的显隐状态
     */
    void setFocusHighlight(bool visible);

    /**
     * @brief 设置并跳转到指定目录
     * @param path 完整路径
     */
    void setRootPath(const QString& path);

    /**
     * @brief 在树中选中指定路径对应的项
     */
    void selectPath(const QString& path);

signals:
    /**
     * @brief 当用户点击目录时发出信号
     * @param path 目标目录完整路径
     */
    void directorySelected(const QString& path);

    /**
     * @brief 请求在内容面板中定位并选中某个文件
     * @param path 文件完整路径
     */
    void requestLocateFile(const QString& path);

    /**
     * @brief 请求在内容面板中打开回收站
     */
    void requestOpenTrash();

private slots:
    void onItemExpanded(const QModelIndex& index);
    void onTreeClicked(const QModelIndex& index);
    void updateTreeHeight();

private:
    void initUi();
    void fetchChildDirs(QStandardItem* parent);

    // 上方：磁盘树
    QTreeView* m_treeView = nullptr;
    QStandardItemModel* m_model = nullptr;

    QVBoxLayout* m_mainLayout = nullptr;
    QWidget* m_focusLine = nullptr;
};

} // namespace QuarkMeta
