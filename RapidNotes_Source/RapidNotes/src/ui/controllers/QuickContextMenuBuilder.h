#ifndef QUICKCONTEXTMENUBUILDER_H
#define QUICKCONTEXTMENUBUILDER_H

#include <QMenu>
#include <QModelIndexList>
#include <QModelIndex>
#include <QWidget>

class QuickWindow;
namespace RapidNotes { class DropTreeView; }

namespace RapidNotes {

class QuickContextMenuBuilder {
public:
    // 构建笔记列表右键菜单 (单选、多选、合并、复制提取、回收站模式与排序)
    static QMenu* buildListContextMenu(QuickWindow* window, const QModelIndexList& selected, QWidget* parent);

    // 构建侧边栏树形节点右键菜单 (分类管理、递归导出、导入、密码保护、锁定与排序)
    static QMenu* buildSidebarContextMenu(QuickWindow* window, DropTreeView* tree, const QModelIndex& index, const QModelIndexList& selected, QWidget* parent);
};

} // namespace RapidNotes

#endif // QUICKCONTEXTMENUBUILDER_H