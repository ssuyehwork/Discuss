# Implementation Plan: 库分类新建行内编辑与 F2 重命名闭环实施方案 (Library Category Inline Rename & F2 Implementation Plan)

Base Commit Hash: `9e1a729`
关联模块: `QuarkMeta_Source / LibraryPanel`

---

## 证据清单 (Pre-Draft Physical Evidence)

### 1. 物理读取文件列表
- `QuarkMeta_Source/src/ui/LibraryPanel.h` (已确认读取，含 `LibraryPanel` 类声明及 `createAndEditCategory`)
- `QuarkMeta_Source/src/ui/LibraryPanel.cpp` (已确认读取，含 `initUi`、`onCategoryContextMenu`、`onCategoryClicked`、`eventFilter`、`loadLibrary`、`createAndEditCategory`)
- `QuarkMeta_Source/src/meta/LibraryService.h` (已确认读取，含 `createCategory` 接口声明与 `libraryChanged` 信号声明)
- `QuarkMeta_Source/src/meta/LibraryService.cpp` (已确认读取，含 `createCategory` 调用 `LibraryDao::addCategory` 并发出 `emit libraryChanged()`)
- `QuarkMeta_Source/src/ui/FavoritePanel.cpp` (已确认读取作为对比基准，含 `createAndEditCategory` 及节点行内编辑唤起逻辑)

### 2. 头文件原样签名核验
- `LibraryService::createCategory`
  ```cpp
  int createCategory(const QString& name, int parentId = 0);
  ```
- `LibraryPanel::createAndEditCategory`
  ```cpp
  void createAndEditCategory(int parentId = 0);
  ```
- `QWidget::setFocus`
  ```cpp
  void setFocus(Qt::FocusReason reason = Qt::OtherFocusReason);
  ```
- `QAbstractItemView::edit`
  ```cpp
  void edit(const QModelIndex &index);
  ```
- `QAbstractItemView::setCurrentIndex`
  ```cpp
  void setCurrentIndex(const QModelIndex &index);
  ```

### 3. 既有 API 检索与防另起炉灶日志
- `grep -rn "createCategory" QuarkMeta_Source/src/`
  命中了唯一真理源 `LibraryService::instance().createCategory(defaultName, parentId)`；
- `grep -rn "libraryChanged" QuarkMeta_Source/src/`
  命中了底层核心领域模型变更事件 `LibraryService::instance().libraryChanged`；
- `grep -rn "m_pendingEditNodeId" QuarkMeta_Source/src/ui/LibraryPanel.*`
  确认了面板层用于记录待进入重命名节点 ID 的状态变量 `m_pendingEditNodeId`。

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题现象与根因诊断
1. **右键新建分类无法进入行内编辑**：
   - 之前“+”号按钮能进入行内编辑，但右键菜单触发“新建库分类”却无法进入编辑；
   - 根因：在 `LibraryPanel::createAndEditCategory` 中，调用 `LibraryService::instance().createCategory()` 后，既调用了 `loadLibrary()`，底层服务又发出了 `libraryChanged` 异步通知再次调用 `loadLibrary()`，产生双重刷新竞态。更关键的是，右键菜单是在阻塞式的 `menu.exec(...)` 内触发的。当菜单点击并销毁关闭时，Qt 的事件分发机制会触发窗口焦点切换和 `FocusOut` 事件。如果树视图此时被动丢失焦点或未显式持有焦点，刚唤起的行内重命名编辑控件就会被瞬间提交并关闭（Commit & Close），导致用户完全看不到编辑框。
2. **F2 键在库分类树上无法唤起行内重命名**：
   - 根因：用户通过鼠标点击分类节点时（`onCategoryClicked`），树视图没有显式调用 `m_treeView->setFocus()`。由于主窗口焦点经常处于中心内容面板（`ContentPanel`）或底部搜索过滤框，按下的 `F2` 键被路由到了其他组件；此外，右键菜单弹出重命名项时也未先将键盘焦点赋予树视图；
   - 同时，树视图在 `initUi()` 中配置了 `NoEditTriggers`，必须在事件过滤器拦截 F2 以及在点击/右键时显式保证树视图持有键盘焦点（`setFocus()`）。

### 1.2 重构设计目标
- 消除 `createAndEditCategory` 中冗余的多重 `loadLibrary()` 竞态，统一由 `libraryChanged` 驱动树重载；
- 消除右键菜单销毁时的焦点抢占冲突，在唤起 `m_treeView->edit()` 前显式调用 `m_treeView->setFocus()`；
- 在用户鼠标点击分类节点（`onCategoryClicked`）与右键菜单弹出时，显式为 `m_treeView` 激活焦点，保证 F2 按键能够可靠分发至树视图并进入行内编辑。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `QuarkMeta_Source/src/ui/LibraryPanel.cpp` (修改：重构新建分类后编辑激活逻辑、解除双重 load 竞态、补齐 setFocus 焦点流转)

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `QuarkMeta_Source/src/ui/LibraryPanel.cpp`

在 `onCategoryClicked` 中增加焦点获取，确保鼠标选中分类节点后按下 F2 能够精准路由至树视图：

<<<<<<< SEARCH
void LibraryPanel::onCategoryClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    int nodeId = index.data(Qt::UserRole + 1).toInt();
=======
void LibraryPanel::onCategoryClicked(const QModelIndex& index) {
    if (m_treeView) {
        m_treeView->setFocus();
    }
    if (!index.isValid()) return;
    int nodeId = index.data(Qt::UserRole + 1).toInt();
>>>>>>> REPLACE

在右键菜单弹出前，确保树视图获取键盘焦点，并确保右键“重命名”菜单项触发时显式调用 `setFocus()`：

<<<<<<< SEARCH
    QAction* renameAct = menu.addAction(UiHelper::getIcon("rename-1.svg", QColor("#EEEEEE")), "重命名");
    connect(renameAct, &QAction::triggered, this, [this, index]() {
        if (m_treeView) m_treeView->edit(index);
    });
=======
    QAction* renameAct = menu.addAction(UiHelper::getIcon("rename-1.svg", QColor("#EEEEEE")), "重命名");
    connect(renameAct, &QAction::triggered, this, [this, index]() {
        if (m_treeView) {
            m_treeView->setFocus();
            m_treeView->edit(index);
        }
    });
>>>>>>> REPLACE

在 `loadLibrary` 末尾唤起行内编辑时，确保先获取树视图焦点并安全展开父节点：

<<<<<<< SEARCH
    if (m_pendingEditNodeId > 0 && m_treeView) {
        int targetNodeId = m_pendingEditNodeId;
        QTimer::singleShot(0, this, [this, targetNodeId]() {
            if (m_pendingEditNodeId != targetNodeId) return;
            m_pendingEditNodeId = 0;

            QStandardItem* targetItem = findItemByNodeId(m_model->invisibleRootItem(), targetNodeId);
            if (targetItem && m_treeView) {
                QModelIndex parentIdx = targetItem->parent() ? targetItem->parent()->index() : QModelIndex();
                if (parentIdx.isValid()) {
                    m_treeView->expand(parentIdx);
                }
                m_treeView->setCurrentIndex(targetItem->index());
                m_treeView->edit(targetItem->index());
            }
        });
    }
=======
    if (m_pendingEditNodeId > 0 && m_treeView) {
        int targetNodeId = m_pendingEditNodeId;
        m_pendingEditNodeId = 0;
        QTimer::singleShot(50, this, [this, targetNodeId]() {
            QStandardItem* targetItem = findItemByNodeId(m_model->invisibleRootItem(), targetNodeId);
            if (targetItem && m_treeView) {
                QModelIndex parentIdx = targetItem->parent() ? targetItem->parent()->index() : QModelIndex();
                if (parentIdx.isValid()) {
                    m_treeView->expand(parentIdx);
                }
                m_treeView->setFocus();
                m_treeView->setCurrentIndex(targetItem->index());
                m_treeView->edit(targetItem->index());
            }
        });
    }
>>>>>>> REPLACE

在 `createAndEditCategory` 中，移除冗余的直接 `loadLibrary()`，避免与 `LibraryService::libraryChanged` 产生双重调用竞态：

<<<<<<< SEARCH
    QString defaultName = (parentId == -20) ? "新建智能文件夹" : "新建分类";
    int newId = LibraryService::instance().createCategory(defaultName, parentId);
    if (newId > 0) {
        m_pendingEditNodeId = newId;
        loadLibrary();
    }
=======
    QString defaultName = (parentId == -20) ? "新建智能文件夹" : "新建分类";
    int newId = LibraryService::instance().createCategory(defaultName, parentId);
    if (newId > 0) {
        m_pendingEditNodeId = newId;
    }
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 构建命令
- 项目构建验证：
  由于沙盒无 Qt6 运行环境，执行代码语法及包含关系静态核验。在具备 Qt 编译环境的宿主机器上执行：
  ```bash
  cmake -B build -S .
  cmake --build build --config Release
  ```

### 4.2 运行时交互验收标准
1. **右键空白处/标题行新建分类**：
   - 打开侧边栏“库”选项卡；
   - 在文件夹空白区域或右键菜单中点击“新建库分类”；
   - 验证：分类树立即展开“文件夹”分组，并生成一个新节点，文本框自动高亮并进入行内重命名编辑模式（光标闪烁，可直接输入名称）。
2. **“+”号按钮新建分类**：
   - 点击“文件夹”右侧的“+”号；
   - 验证：同样平滑展开并进入行内重命名模式，行为与右键菜单 100% 对齐。
3. **F2 快捷键重命名**：
   - 鼠标单击某个普通分类节点（如“测试”）；
   - 按下键盘 `F2`；
   - 验证：当前节点立即进入行内重命名模式；
   - 单击系统内置分类（“全部数据”、“未分类”）并按 `F2`，验证：系统保护节点受到保护，不会误触发编辑。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- **SSOT 复用**：复用既有的 `LibraryService::instance().createCategory`，完全不重新写本地数据库插入；
- **事件流复用**：复用 `LibraryService::libraryChanged` 统一驱动 `loadLibrary()`，彻底消除双重加载竞态；
- **编辑机制复用**：复用 Qt 标准的 `QAbstractItemView::edit(index)` 与 `RenameCapableDelegate` 架构，严禁引入自定义弹窗重命名。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一真理源 (SSOT) | 表现/派生形式 | 同步/刷新机制 |
| :--- | :--- | :--- | :--- |
| 分类结构与节点 ID | `global.db` (通过 `LibraryDao` / `LibraryService`) | `QStandardItemModel` 树视图展示 | 通过 `LibraryService::libraryChanged` 信号驱动 `LibraryPanel::loadLibrary()` |
| 待进入重命名节点 ID | `LibraryPanel::m_pendingEditNodeId` | 界面瞬态变量 | 创建成功后设置，树加载完成后激活并清零 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

| 调用接口 / 成员 | 所在头文件 | 访问级别 | 签名一致性核验 |
| :--- | :--- | :--- | :--- |
| `m_treeView->setFocus()` | `<QWidget>` | `public` | 一致 |
| `m_treeView->setCurrentIndex()` | `<QAbstractItemView>` | `public` | 一致 |
| `m_treeView->edit()` | `<QAbstractItemView>` | `public` | 一致 |
| `LibraryService::createCategory` | `LibraryService.h` | `public` | `int createCategory(const QString& name, int parentId = 0)` |
| `LibraryPanel::createAndEditCategory` | `LibraryPanel.h` | `public/slot` | `void createAndEditCategory(int parentId = 0)` |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- `QTimer`：`LibraryPanel.cpp` 既有包含 `#include <QTimer>`；
- `m_treeView`：`LibraryPanel.h` 已包含 `DropTreeView.h` 并完整定义；
- 本方案未新增任何未定义的新类型或移除任何头文件包含。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项 | 结论 | 位置 / 说明 |
| :---: | :--- | :---: | :--- |
| 1 | 每个 SEARCH 块在目标文件中唯一且完全匹配 | 满足 | `LibraryPanel.cpp` 4 个块均唯一匹配 |
| 2 | 未直接修改业务代码（严格物理隔离） | 满足 | 方案独立位于 `Implementation Plan/` 目录 |
| 3 | 不破坏既有对外头文件签名 | 满足 | 未修改任何 `.h` 接口签名 |
| 4 | 没有触及方案范围外的不相关文件 | 满足 | 仅涉及 `LibraryPanel.cpp` |
| 5 | 无平台级抢焦点 Hack（如 Win32 API） | 满足 | 严格使用 Qt 标准 `setFocus()` 与事件循环 |
| 6 | 包含全部 10 大必备章节 | 满足 | 包含 Chapter 1 至 Chapter 10 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 检查项 | 规范要求 | 方案确认 |
| :--- | :--- | :--- |
| **交互设计零篡改** | 保持行内编辑重命名交互，严禁改弹窗 | 保持使用 `view->edit(index)` 行内重命名模式 |
| **样式与数值零篡改** | 严禁改变或微调既有任何颜色 Hex 与尺寸数值 | 不触碰任何 QSS 与绘图尺寸参数 |
| **系统节点保护** | 内置分类（全部数据、未分类）屏蔽重命名 | 保持屏蔽保护逻辑 |
