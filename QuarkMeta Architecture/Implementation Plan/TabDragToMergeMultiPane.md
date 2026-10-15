# Implementation Plan - Merge Folder into Multi-Pane Split on Tab Button Drag Drop

## 1. Overview
This implementation plan implements intelligent drag-and-drop merging of folders into multi-pane splits when dropped onto tab buttons:
1. When a folder is dropped over a **blank area** of the title/tab bar (not over any tab button), it creates or focuses a separate new tab (maintains current behavior).
2. When a folder is dropped directly **onto a specific tab button** (`TabItemButton`), instead of opening a separate tab, it merges the dragged folder into that target tab as a **multi-pane split (`splitPane(Qt::Horizontal, path)`)**.
3. Updates `TabBarWidget::dropEvent` to perform hit-testing against `m_tabWidgets` geometries, automatically switching to the target tab if necessary and invoking multi-pane split.

---

## 2. Modified Files List
- `src/ui/TabBarWidget.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/TabBarWidget.cpp`
Update `TabBarWidget::dropEvent` to distinguish dropping onto a tab button (merging into a multi-pane split) vs. dropping onto blank space (opening as a new tab):

```cpp
<<<<<<< SEARCH
        } else if (event->mimeData()->hasUrls()) {
            for (const QUrl& url : event->mimeData()->urls()) {
                QString path = url.toLocalFile();
                if (!path.isEmpty() && QFileInfo(path).isDir()) {
                    openOrFocusTab(path);
                    event->acceptProposedAction();
                    return;
                }
            }
        }
=======
        } else if (event->mimeData()->hasUrls()) {
            for (const QUrl& url : event->mimeData()->urls()) {
                QString path = url.toLocalFile();
                if (!path.isEmpty() && QFileInfo(path).isDir()) {
                    QPoint dropPos = event->position().toPoint();

                    // 1. 命中检测：判断拖拽落点是否落在某一个具体的标签页按钮 (TabItemButton) 上
                    int hitTabIdx = -1;
                    for (int i = 0; i < m_tabWidgets.size(); ++i) {
                        if (m_tabWidgets[i] && m_tabWidgets[i]->geometry().contains(dropPos)) {
                            hitTabIdx = i;
                            break;
                        }
                    }

                    if (hitTabIdx != -1) {
                        // 场景 2：落点在特定标签页上方 ➔ 合并到该标签页创建多窗格 (Split Pane)
                        if (hitTabIdx != m_currentIndex) {
                            setCurrentIndex(hitTabIdx, true);
                        }
                        emit splitRequested(hitTabIdx);
                        // 通过 splitRequested 或直接通知 ContentPanel 拆分当前窗格
                        emit currentTabChanged(m_currentIndex, path);
                        event->acceptProposedAction();
                        return;
                    } else {
                        // 场景 1：落点在空白处（非标签页上方） ➔ 保持现状创建/聚焦独立标签页
                        openOrFocusTab(path);
                        event->acceptProposedAction();
                        return;
                    }
                }
            }
        }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Recompile QuarkMeta target using CMake:
   `cmake --build build`
2. Launch QuarkMeta with one or more open tabs.
3. Drag a folder from the content view or sidebar:
   - **Test A (Blank Area Drop)**: Drop the folder onto the blank area to the right of the tab bar (not over any tab button). Verify that a separate new tab is created and focused (Scenario 1).
   - **Test B (Tab Button Drop)**: Drag a folder and drop it directly over an existing tab button. Verify that the target tab splits horizontally into a multi-pane layout (`TabSplitState.isSplit = true`), displaying the existing folder and new folder side-by-side (Scenario 2).

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Multi-Pane SSOT**: Uses existing `splitRequested` signal and `ContentPaneSplitManager::splitPane` multi-pane pipeline.
- **Tab Management SSOT**: Reuses `m_tabWidgets[i]->geometry()` hit-testing without creating duplicate geometry calculation logic.

---

## 6. Header API Signature Verification
| Class / Function Signature | Declared Header File | Signature Match Verification |
| :--- | :--- | :--- |
| `void openOrFocusTab(const QString& rawPath)` | `src/ui/TabBarWidget.h` | 100% Exact Match |
| `void setCurrentIndex(int index, bool forceNotify = false)` | `src/ui/TabBarWidget.h` | 100% Exact Match |

---

## 7. Header Inclusion Chain & Type Completeness Check
| Inclusion / Type Name | File | Purpose & Completeness Check |
| :--- | :--- | :--- |
| `#include <QDropEvent>` | `src/ui/TabBarWidget.cpp` | Provides `QDropEvent` position and MimeData methods. |
| `#include <QFileInfo>` | `src/ui/TabBarWidget.cpp` | Provides `QFileInfo::isDir()` check. |
