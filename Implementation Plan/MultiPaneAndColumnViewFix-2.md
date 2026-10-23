# MultiPaneAndColumnViewFix-2 Implementation Plan

## 1. Overview
对比“Version-Old-10”历史版本与当前版本的“双窗格分栏视图”逻辑，发现 Version-Old-10 具备更健全、更智能的切分与关闭控制：
1. **副窗格点击分栏按钮时关闭自身**：若用户在副窗格（`isSecondaryPane() == true`）上点击顶部的“双窗格分栏视图”按钮，能够触发 `emit closePaneRequested()` 快速关闭当前副窗格。
2. **智能恢复历史路径**：在分栏打开时，Version-Old-10 不会盲目复制当前路径，而是调用 `NavigationHistoryService::instance().getHistory()` 智能提取用户上一次访问过的不同历史路径 `lastPath` 作为副窗格的初始路径，大幅提升双窗格对照整理文件体验。
3. **补齐头文件包含**：在 `ContentPanel.cpp` 中引入 `#include "../core/NavigationHistoryService.h"` 闭环类型定义。

---

## 2. Modified Files List
1. `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Changes to `src/ui/ContentPanel.cpp`

<<<<<<< SEARCH
#include "../util/ThumbnailPipelineService.h"
#include "../core/NavigationService.h"
=======
#include "../util/ThumbnailPipelineService.h"
#include "../core/NavigationService.h"
#include "../core/NavigationHistoryService.h"
>>>>>>> REPLACE

<<<<<<< SEARCH
    connect(m_headerWidget, &ContentHeaderWidget::splitViewRequested, this, [this]() {
        if (isSplitMode()) {
            closeSecondaryPane();
        } else {
            splitPane(Qt::Horizontal);
        }
    });
=======
    connect(m_headerWidget, &ContentHeaderWidget::splitViewRequested, this, [this]() {
        if (isSecondaryPane()) {
            emit closePaneRequested();
            return;
        }
        if (isSplitMode()) {
            closeSecondaryPane();
        } else {
            QStringList history = NavigationHistoryService::instance().getHistory();
            QString lastPath;
            for (const QString& hPath : history) {
                if (!hPath.isEmpty() && QDir::cleanPath(hPath) != QDir::cleanPath(m_currentPath)) {
                    lastPath = hPath;
                    break;
                }
            }
            if (lastPath.isEmpty()) {
                lastPath = m_currentPath;
            }
            splitPane(Qt::Horizontal, lastPath);
        }
    });
>>>>>>> REPLACE

---

## 4. Build & Verification Steps

### 4.1 编译指令
```cmd
cmake --build build --config Release
```

### 4.2 功能验证步骤
1. 点击主窗格“双窗格分栏视图”按钮，确认副窗格自动打开并优先展示导航历史中的上一个文件夹。
2. 点击副窗格“双窗格分栏视图”按钮，确认副窗格平滑关闭自身。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 复用 `NavigationHistoryService::instance().getHistory()`，对齐 Version-Old-10 的历史行为。

---

## 6. Header API Signature Verification
- `NavigationHistoryService::instance().getHistory()`：`QStringList getHistory() const`

---

## 7. Header Inclusion Chain & Type Completeness Check
- 显式引入 `#include "../core/NavigationHistoryService.h"`，闭环头文件依赖链。
