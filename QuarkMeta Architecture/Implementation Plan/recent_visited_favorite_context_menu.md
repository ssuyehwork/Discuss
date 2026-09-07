# “最近访问”文件夹右键菜单支持“添加至收藏夹”重构实施方案

## 一、 需求背景与功能定义

在目录导航面板（`NavPanel`）中，用户可以通过展开“最近访问”节点快速点击历史访问过的文件夹。
为了提升操作效率，用户期望**在“最近访问”列表项（以及导航树其他有效文件夹项）上右键点击时，能弹出右键上下文菜单，支持“添加至收藏夹 / 从收藏夹移除”以及“复制完整路径”**功能。

---

## 二、 现状缺陷与根因分析

1. **`NavPanel` 开启了右键策略但未连接信号**：
   - 构造函数中虽然设置了 `setContextMenuPolicy(Qt::CustomContextMenu);`，但未连接 `m_treeView` 的 `customContextMenuRequested(QPoint)` 信号到任何处理槽函数。
2. **缺乏右键菜单弹出逻辑**：
   - 当用户在“最近访问”的子项上击右键时，系统无任何响应，无法唤起包含“添加至收藏夹”的 `QMenu` 菜单。
3. **`NavPanel` 与 `FavoritePanel` 缺乏 Mediator 路由绑定**：
   - `PanelMediator.cpp` 中仅绑定了 `NavPanel::directorySelected` 和 `requestOpenTrash`，尚未建立 `NavPanel::requestAddFavorite` 与 `favoritePanel` 的信号响应回路。

---

## 三、 架构设计原则（Architecture Alignment）

1. **统一的 Mediator 解耦**：
   - `NavPanel` 自身不直接依赖 `FavoriteDao` 或 `FavoritePanel`，仅对外发射 `requestAddFavorite(QString)` 信号。
   - 所有收藏夹操作与 Tip 气泡弹框统一由 `PanelMediator.cpp` 管理调度，确保与 `AddressBar` 和 `QuickLookWindow` 的收藏体验完全一致。
2. **状态感知菜单（State-Aware Context Menu）**：
   - 在弹窗右键菜单前，实时校验目标路径是否已存在于收藏夹中：
     - 若**已在收藏夹**中：显示“从收藏夹移除”图标与文本（红色关闭图标）。
     - 若**不在收藏夹**中：显示“添加至收藏夹”图标与文本（金色星星图标）。

---

## 四、 具体重构实施步骤蓝图（Blueprint）

### 1. `NavPanel` 增加信号与右键槽函数 (`src/ui/NavPanel.h`)
```cpp
signals:
    void requestAddFavorite(const QString& path);

private slots:
    void onCustomContextMenuRequested(const QPoint& pos);
```

### 2. `NavPanel` 实现右键菜单构建 (`src/ui/NavPanel.cpp`)
在 `initUi()` 中添加信号连接：
```cpp
m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
connect(m_treeView, &QTreeView::customContextMenuRequested, this, &NavPanel::onCustomContextMenuRequested);
```

在 `onCustomContextMenuRequested(const QPoint& pos)` 中实现具体的菜单弹出：
```cpp
void NavPanel::onCustomContextMenuRequested(const QPoint& pos) {
    QModelIndex idx = m_treeView->indexAt(pos);
    if (!idx.isValid()) return;

    QString path = idx.data(Qt::UserRole + 1).toString();
    if (path.isEmpty() || path == "computer://" || path == "recent_root" || path == "trash_root") {
        return;
    }

    QString nativePath = QDir::toNativeSeparators(path);
    if (!QFileInfo::exists(nativePath)) return;

    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    QAction* actFavToggle = menu.addAction(UiHelper::getIcon("star_filled", QColor("#FDB70A")), "添加至收藏夹");
    QAction* actCopyPath = menu.addAction(UiHelper::getIcon("copy", QColor("#EEEEEE")), "复制完整路径");

    QAction* selected = menu.exec(m_treeView->viewport()->mapToGlobal(pos));
    if (selected == actFavToggle) {
        emit requestAddFavorite(nativePath);
    } else if (selected == actCopyPath) {
        QApplication::clipboard()->setText(nativePath);
        ToolTipOverlay::instance()->showText(QCursor::pos(), "已复制路径至剪贴板", 1500, Style::SuccessGreen);
    }
}
```

### 3. `PanelMediator` 建立收藏绑定回路 (`src/ui/PanelMediator.cpp`)
在 `PanelMediator::setupConnections()` 中添加：
```cpp
if (navPanel && favoritePanel) {
    connect(navPanel, &NavPanel::requestAddFavorite, favoritePanel, [favoritePanel](const QString& path) {
        if (favoritePanel->containsPath(path)) {
            favoritePanel->removeFavoriteItem(path);
            favoritePanel->saveFavorites();
            ToolTipOverlay::instance()->showText(QCursor::pos(), "已从收藏夹移除", 1500, QColor("#e74c3c"));
        } else {
            favoritePanel->addFavoriteItem(path);
            favoritePanel->saveFavorites();
            ToolTipOverlay::instance()->showText(QCursor::pos(), "已成功添加至收藏夹", 1500, QColor("#2ecc71"));
        }
    });
}
```

---

## 五、 验证与测试方案

1. **右键响应测试**：
   - 展开左侧导航栏的“最近访问”节点。
   - 对任意历史文件夹右键，确认弹出美化后的右键菜单。
2. **添加/移除收藏夹功能测试**：
   - 点击“添加至收藏夹”，确认左侧上方“收藏夹”面板立即出现该文件夹，同时屏幕上弹出绿色 Tip。
   - 再次右键该文件夹，确认可顺利移除或重新添加。
3. **复制路径测试**：
   - 点击“复制完整路径”，确认剪贴板中已存入规范化的原生绝对路径（如 `H:\测试\测试-3`）。
