# MultiPaneSplitManager Implementation Plan (移植恢复多窗格拆分管理器与拖拽 Tab 分屏)

## Overview
本实施方案旨在将 `Version-Old-10` 中极其关键的 **`ContentPaneSplitManager` (内容面板多窗格拆分管理器)** 及其与 **`TabBarWidget` (拖拽标签页触发分屏与 `TabSplitState` 状态持久化)** 的深度联动功能完整移植恢复至当前版本中。

修改内容包括：
1. 从 `Version-Old-10/src/ui/controllers/ContentPaneSplitManager.h` 和 `ContentPaneSplitManager.cpp` 拷贝移植拆分管理器源文件至 `src/ui/controllers/` 目录。
2. 在 `CMakeLists.txt` 的 `SOURCES` 中添加 `src/ui/controllers/ContentPaneSplitManager.h` 与 `src/ui/controllers/ContentPaneSplitManager.cpp`。
3. 修改 `src/ui/ContentPanel.h` / `src/ui/ContentPanel.cpp`：
   - 添加 `ContentPaneSplitManager* m_splitManager = nullptr;` 及 `splitManager()` 访问接口。
   - 重写 `dragEnterEvent` / `dragMoveEvent` / `dragLeaveEvent` / `dropEvent` 拖放事件，捕获拖拽的 Tab 路径并结合放置位置触发 `splitPane(Qt::Horizontal / Qt::Vertical, url)`。
4. 修改 `src/ui/PanelMediator.cpp`：
   - 监听 `TabBarWidget::tabAboutToChange` 信号，在 Tab 切换前将当前多窗格分屏状态导出保存至 `tabBar()->setTabSplitState(oldIndex, state)`。
   - 监听 `TabBarWidget::currentTabChanged` 信号，在 Tab 切换后精准还原该 Tab 页对应的 `restoreSplitState(state)`。

---

## Modified Files List
1. `CMakeLists.txt`
2. `src/ui/controllers/ContentPaneSplitManager.h` (移植自 Version-Old-10)
3. `src/ui/controllers/ContentPaneSplitManager.cpp` (移植自 Version-Old-10)
4. `src/ui/ContentPanel.h`
5. `src/ui/ContentPanel.cpp`
6. `src/ui/PanelMediator.cpp`

---

## Detailed Line-by-Line Changes

### 1. `CMakeLists.txt`

<<<<<<< SEARCH
    src/ui/workers/ContentStatsWorker.h
    src/ui/workers/ContentStatsWorker.cpp
=======
    src/ui/controllers/ContentPaneSplitManager.h
    src/ui/controllers/ContentPaneSplitManager.cpp
    src/ui/workers/ContentStatsWorker.h
    src/ui/workers/ContentStatsWorker.cpp
>>>>>>> REPLACE

---

### 2. `src/ui/ContentPanel.h`

<<<<<<< SEARCH
class ContentStatsWorker;
class FolderSectionHeaderBar;
=======
class ContentStatsWorker;
class ContentPaneSplitManager;
class FolderSectionHeaderBar;
>>>>>>> REPLACE

<<<<<<< SEARCH
    ContentDataLoader* dataLoader() const { return m_dataLoader; }
    ContentFileOpsHandler* fileOpsHandler() const { return m_fileOpsHandler; }
    ContentStatsWorker* statsWorker() const { return m_statsWorker; }
=======
    ContentDataLoader* dataLoader() const { return m_dataLoader; }
    ContentFileOpsHandler* fileOpsHandler() const { return m_fileOpsHandler; }
    ContentStatsWorker* statsWorker() const { return m_statsWorker; }
    ContentPaneSplitManager* splitManager() const { return m_splitManager; }

    void splitPane(Qt::Orientation orientation, const QString& secondaryPath = QString());
>>>>>>> REPLACE

<<<<<<< SEARCH
protected:
    void wheelEvent(QWheelEvent* event) override;
=======
protected:
    void wheelEvent(QWheelEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
>>>>>>> REPLACE

<<<<<<< SEARCH
    ContentDataLoader* m_dataLoader = nullptr;
    ContentFileOpsHandler* m_fileOpsHandler = nullptr;
    ContentStatsWorker* m_statsWorker = nullptr;
=======
    ContentDataLoader* m_dataLoader = nullptr;
    ContentFileOpsHandler* m_fileOpsHandler = nullptr;
    ContentStatsWorker* m_statsWorker = nullptr;
    ContentPaneSplitManager* m_splitManager = nullptr;
>>>>>>> REPLACE

---

## Build & Verification Steps
1. 拷贝 `Version-Old-10/src/ui/controllers/ContentPaneSplitManager.h` 和 `.cpp` 至 `src/ui/controllers/`。
2. 运行 `cmake -B build -S .` 及 `cmake --build build` 验证项目无缝编译。
3. 运行软件，按住标题栏 Tab 拖拽至内容面板左/右/上/下位置，验证高亮遮罩层与窗格拆分、Tab 切换还原分屏状态完全正常。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- 100% 严格复用 Version-Old-10 经过考验的 `ContentPaneSplitManager` 设计，绝不另起炉灶。

---

## Header API Signature Verification
- `ContentPaneSplitManager::splitPane(Qt::Orientation, const QString&)`: 准确匹配
- `ContentPaneSplitManager::exportSplitState()`: 准确匹配
- `ContentPaneSplitManager::restoreSplitState(const TabSplitState&)`: 准确匹配