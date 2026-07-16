# “添加至收藏夹”右键菜单功能设计 —— Analysis_Modification_Plan-161.md

## 1. 任务背景
在当前版本中，“目录导航”下方（对应用户原话：““目录导航”下方有一个“收藏夹”区”）虽然提供了高频路径的“收藏夹”展示功能，但用户只能通过拖拽方式向其中加入路径。在日常文件管理流程中，用户期望能够通过右键操作更方便地将内容容器中选中的文件或目录添加至收藏夹。
为此，我们需要在内容容器（ContentPanel）的右键菜单中（对应用户原话：“在右键菜单新增一个选项”）新增一个名为“添加至收藏夹”的选项（对应用户原话：“新增一个选项为“添加至收藏夹””），当用户选中某个项目单击右键（对应用户原话：“在内容容器里选中某个项目单击右键选择该选项“添加至收藏夹”时”）并选择此项时，自动将选中的项目收藏到左侧收藏区（对应用户原话：“则把选中的项目收藏到收藏区里”），实现流畅、开箱即用的右键收藏交互。

## 2. 问题定位
- **菜单定义与枚举缺失**：现有的右键菜单动作枚举 `ContextAction`（定义于 `src/ui/ContentPanel.h`）中，尚不包含用于“添加至收藏夹”的枚举标记。
- **右键菜单构建逻辑**：构建内容面板右键菜单的函数 `ContentPanel::onCustomContextMenuRequested`（位于 `src/ui/ContentPanel.cpp`）中，需要增加添加该动作的菜单项入口。
- **收藏接口非公开**：导航面板 `NavPanel` 拥有管理收藏项的成员函数 `addFavoriteItem` 和 `saveFavorites`，但当前这些函数在 `src/ui/NavPanel.h` 中属于 `private` 作用域，外部的 `MainWindow` 无法直接调用它们。
- **中转通信链路缺失**：当在 `ContentPanel` 的右键菜单触发收藏动作时，需要通过 Qt 的信号槽（Signal/Slot）机制通知 `MainWindow`，再由 `MainWindow` 获取并调用 `m_navPanel`（即 `NavPanel`）完成收藏添加与保存。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | “目录导航”下方有一个“收藏夹”区 | 方案中明确对齐该收藏夹视图，利用 `NavPanel` 底部管理收藏夹。 | ✅ |
| 2    | 我期望在右键菜单新增一个选项为“添加至收藏夹” | 在右键菜单构建中加入“添加至收藏夹”选项，绑定 ActionAddToFavorites 枚举动作。 | ✅ |
| 3    | 当在内容容器里选中某个项目单击右键选择该选项“添加至收藏夹”时 | `ContentPanel` 捕获右键选中动作，对选中的项目（PathRole 物理路径）触发并发出添加收藏信号。 | ✅ |
| 4    | 则把选中的项目收藏到收藏区里 | `MainWindow` 连接信号并调用 `NavPanel` 现有的 `addFavoriteItem` 和 `saveFavorites` 将选中的项目添加并持久化。 | ✅ |

## 4. 详细解决方案

### 4.1 方案架构与时序图
```
[ContentPanel] (选中项目右键)
      │
      ├─► 弹出右键菜单 ──► 用户点击“添加至收藏夹” (对应用户原话：“单击右键选择该选项“添加至收藏夹””)
      │
      ├─► 收集所有选中的 PathRole 路径
      │
      ├─► emit requestAddFavorite(selectedPaths)
      ▼
[MainWindow] (主中枢槽函数接收)
      │
      ├─► 遍历路径调用 m_navPanel->addFavoriteItem(path)
      │
      ├─► 调用 m_navPanel->saveFavorites()
      ▼
[NavPanel] (添加收藏并保存)
      │
      ├─► 排重并利用 QStandardItem 增加对应行 ──► 自动更新 UI 视图
      │
      └─► 持久化写入 AppConfig 配置文件
```

### 4.2 接口声明与实现说明

#### 第一部分：修改 `src/ui/ContentPanel.h`
1. 在 `ContextAction` 右键菜单动作枚举中，在最后一个元素前追加 `ActionAddToFavorites`。
2. 在 `signals:` 信号声明区域追加一个信号用于通知外部：
   ```cpp
   /**
    * @brief 请求将指定路径添加至收藏夹的信号 (对应用户原话：“把选中的项目收藏到收藏区里”)
    * @param paths 选中的项目绝对物理路径列表 (对应用户原话：“选中某个项目”)
    */
   void requestAddFavorite(const QStringList& paths);
   ```

#### 第二部分：修改 `src/ui/ContentPanel.cpp`
1. 在 `ContentPanel::onCustomContextMenuRequested` 构建上下文菜单时：
   在 `onItem` 为 true 的核心操作区中，为多选项目或者非空项目增加“添加至收藏夹”动作。我们将其优雅地安插在“属性”之上，以保持视觉对齐：
   ```cpp
   // 菜单项构造位置 (对应用户原话：“在右键菜单新增一个选项为“添加至收藏夹””)
   menu.addAction("添加至收藏夹")->setData(ActionAddToFavorites);
   menu.addAction("属性")->setData(ActionProperties);
   ```
2. 在该方法的动作分发 `switch (action)` block 中，增加对 `ActionAddToFavorites` 的处理：
   ```cpp
   case ActionAddToFavorites: {
       QStringList selectedPaths;
       auto indexes = view->selectionModel()->selectedIndexes();
       for (const auto& idx : indexes) {
           if (idx.column() == 0) {
               QString p = idx.data(PathRole).toString();
               if (!p.isEmpty()) {
                   selectedPaths << p;
               }
           }
       }
       if (!selectedPaths.isEmpty()) {
           emit requestAddFavorite(selectedPaths);
           // 气泡提示
           ToolTipOverlay::instance()->showText(QCursor::pos(), "已成功添加至收藏夹", 1500, QColor("#2ecc71"));
       }
       break;
   }
   ```

#### 第三部分：修改 `src/ui/NavPanel.h`
为了让外部 `MainWindow` 能够直接驱动收藏流程，需要把 `addFavoriteItem` 和 `saveFavorites` 从 `private:` 移动到 `public:` 访问控制区域：
```cpp
public:
    explicit NavPanel(QWidget* parent = nullptr);
    ~NavPanel() override = default;

    // 新增：向外暴露的收藏夹追加与持久化保存接口
    void addFavoriteItem(const QString& path);
    void saveFavorites();
```

#### 第四部分：修改 `src/ui/MainWindow.cpp`
在构造函数中，组件间信号绑定链条（靠近 `m_contentPanel` 其它信号连接的区域，如行号 250 至 360 附近）连接此新增信号：
```cpp
    // 监听内容容器的右键添加至收藏夹信号 (对应用户原话：“选中某个项目单击右键选择该选项“添加至收藏夹”时，则把选中的项目收藏到收藏区里”)
    connect(m_contentPanel, &ContentPanel::requestAddFavorite, this, [this](const QStringList& paths) {
        for (const QString& p : paths) {
            m_navPanel->addFavoriteItem(p);
        }
        m_navPanel->saveFavorites();
    });
```

---

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [ ] `src/ui/ContentPanel.h`：增加 `ActionAddToFavorites` 枚举、信号 `requestAddFavorite` 声明。
- [ ] `src/ui/ContentPanel.cpp`：添加“添加至收藏夹”菜单项，编写 `ActionAddToFavorites` 触发逻辑、触发提示。
- [ ] `src/ui/NavPanel.h`：将 `addFavoriteItem` 和 `saveFavorites` 提升为 `public` 访问属性。
- [ ] `src/ui/MainWindow.cpp`：建立 `m_contentPanel` 与 `m_navPanel` 之间的通信信号连接。

**明确禁止越界修改的范围：**
- [ ] 严禁修改任何底层物理 USN 模型、SQLite 缓存结构。
- [ ] 严禁修改左侧分类树（`CategoryPanel`）及属性编辑区（`MetaPanel`）。
- [ ] 严禁在方案之外定义自定义的、不属于 `QStandardItemModel` 的独立收藏夹结构。

---

## 6. 实现准则与预警【核心】
1. **依赖头文件引入与预警**：
   - 菜单操作和提示需要调用系统悬浮泡。确保 `src/ui/ContentPanel.cpp` 已引入 `#include "ToolTipOverlay.h"`。
   - 属性提取需要获取 `PathRole`。
2. **指针安全与生命周期检测**：
   - 在 `MainWindow` 的 Lambda 回调中，`m_navPanel` 指针生命周期受主窗口管理，处于强绑定状态。在回调前，建议添加指针非空断言（`if (m_navPanel)`），杜绝析构过程中或未初始化完成时的无效触发引发的崩溃。
3. **去重与排重提醒**：
   - `NavPanel::addFavoriteItem` 内部已经具备了一套针对已有项目的去重循环机制，并在不存在该文件时自动忽略。因此不需要在 `ContentPanel` 侧或 `MainWindow` 侧增加复杂的去重逻辑，保持职责归一化。
4. **Qt 信号与槽连接规范**：
   - `connect` 应使用标准新版指针写法，以实现编译期类型安全，避免宏写法在重命名时产生运行时报错。

---

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| **视觉反馈气泡** | 气泡提示需要使用系统 `ToolTipOverlay::instance()->showText(...)`，配合绿色颜色（如 `#2ecc71`）反馈。 | ✅ 符合（详见方案 4.2 第二部分设计） |
| **清除与编辑框** | e.g. setClearButtonEnabled 只用于输入框，禁止自行用 Action 模拟清除功能 | ✅ 符合（不涉及文本清除） |
| **快速预览 (QuickLook)** | 快速预览拦截与平滑加载规范 | ✅ 符合（本方案为右键纯逻辑触发，完全不干涉 Preview 功能） |

---

## 8. 待确认事项（可选）
- **关于本方案的方向词与数量词说明**：
  - 本方案中的“右键菜单”（对应用户原话：“在右键菜单新增一个选项”）与“下方收藏夹区”（对应用户原话：““目录导航”下方有一个“收藏夹”区”）已完全通过锚定用户原话进行了对应，没有进行任何无端假定。当前不存在任何需要用户待确认的技术悬置点，方案完全满足开箱即用的质量标准。
