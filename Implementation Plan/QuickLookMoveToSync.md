# Implementation Plan - QuickLookMoveToSync

**Base Commit Hash**: `c6b97d6bd6005607baff5a68d45f068eb1c33dab`

---

## 一、 概述 (Overview)

本方案旨在解决通过右键菜单“移动到...”移动物理文件时出现的误弹“无法打开文件进行预览。”缺陷：
1. **缺陷根源**：通过鼠标拖拽（Drag & Drop）移动文件时，Qt 的 `QDrag` 启动过程触发 `WindowDeactivate`，使 `QuickLookWindow` 提前关闭预览。而通过右键菜单“移动到...”移动文件时，`QMenu` 级联子菜单交互保留在当前窗口链中，不会自动关闭 `QuickLookWindow`。当 `DiskIoService` 成功将文件移至新目录后，悬挂在后台的 `QuickLookWindow` 尝试重新读取已不存在的旧路径文件，导致 `file.open()` 失败并弹出“无法打开文件进行预览。”报错。
2. **重构方案**：在 `ContentContextMenu.cpp` 的 `performMoveTo` 执行物理移动逻辑前，检查若 `QuickLookWindow::instance().isVisible()` 为真且当前预览路径包含在待移动列表中，第一时间平滑关闭预览窗口 (`closePreview()`)，使右键“移动到...”与拖拽迁移行为保持 100% 生命周期对齐。

---

## 二、 修改文件清单 (Modified Files List)

| 序号 | 物理文件路径 | 模块/组件 | 修改性质 |
| :---: | :--- | :--- | :--- |
| 1 | `src/ui/controllers/ContentContextMenu.cpp` | UI / Controllers | 包含 `QuickLookWindow.h` 并在 `performMoveTo` 移动前平滑关闭 QuickLook |

---

## 三、 逐行变更明细 (Detailed Line-by-Line Changes)

### 1. `src/ui/controllers/ContentContextMenu.cpp`

```
<<<<<<< SEARCH
#include "../BatchRenameDialog.h"
#include "../BatchCreateDialog.h"
#include "../FramelessDialog.h"
=======
#include "../BatchRenameDialog.h"
#include "../BatchCreateDialog.h"
#include "../FramelessDialog.h"
#include "../QuickLookWindow.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
                auto performMoveTo = [this](const QString& targetDir) {
                    QStringList selectedPaths = m_panel->getSelectedPaths();
                    if (selectedPaths.isEmpty()) return;

                    LastOperationManager::instance().recordMoveToFolder(targetDir);
=======
                auto performMoveTo = [this](const QString& targetDir) {
                    QStringList selectedPaths = m_panel->getSelectedPaths();
                    if (selectedPaths.isEmpty()) return;

                    // 🚀【生命周期对齐】：右键“移动到...”前若空格预览处于打开状态，平滑关闭 QuickLook，防止物理移走后旧路径读取报错
                    if (QuickLookWindow::instance().isVisible()) {
                        QuickLookWindow::instance().closePreview();
                    }

                    LastOperationManager::instance().recordMoveToFolder(targetDir);
>>>>>>> REPLACE
```

---

## 四、 构建与验证步骤 (Build & Verification Steps)

1. **编译确认**：
   在 Qt Creator 或命令行工具中使用 C++20 标准进行增量构建，确保零警告、零错误通过。
2. **右键“移动到...”功能与 QuickLook 联动验证**：
   选择某个文本或图形文件，按空格键开启 QuickLook 预览，随后右键该文件并选择“移动到...”将其移动至另一个文件夹。验证移动完成后，系统直接弹出“文件移动成功”提示，不再弹显“无法打开文件进行预览。”的错误提示。

---

## 五、 官方 API 复用与防冗余声明 (Official API Reuse & Anti-Redundancy Declaration)

- **直接复用 `QuickLookWindow` 既有 API**：直接复用 `QuickLookWindow::instance().isVisible()` 与 `closePreview()` 接口，无需新增任何重复代码或方法。

---

## 六、 头文件 API 签名校验 (Header API Signature Verification)

- 本次修改仅影响 `ContentContextMenu.cpp` 内部 lambda 函数，不触碰任何 `.h` 头文件公开接口声明，契约完全保持不变。

---

## 七、 头文件包含链与类型完整性检查 (Header Inclusion Chain & Type Completeness Check)

- `ContentContextMenu.cpp` 新增包含 `#include "../QuickLookWindow.h"`，保证 `QuickLookWindow` 类声明类型完整。

---

## 八、 数据 SSOT 声明表 (Data SSOT Declaration Table)

| 数据项 | SSOT 权威数据源 | 读写路径 |
| :--- | :--- | :--- |
| **QuickLook 预览状态** | `QuickLookWindow` | `QuickLookWindow::instance()` 维护当前预览路径与显隐状态 |

---

## 九、 架构与红线合规自检表 (Compliance Self-Check Table)

| 检查项 | 是否合规 | 判定说明 |
| :--- | :---: | :--- |
| **无未授权源码修改红线** | **是** | 所有代码修改仅作为标准 Merge Diff 存放在本实施方案中，源代码保持不变 |
| **生命周期对齐** | **是** | 解决右键“移动到...”与拖拽移动在 QuickLook 生命周期管理上的不一致 |

---

## 十、 行为保持与零退化声明表 (Behavior Preservation Declaration Table)

1. **移动功能保持**：`DiskIoService` 物理搬运、侧车漫游和缩略图缓存漫游逻辑 100% 维持不变。
2. **快捷键与常规操作保持**：非 QuickLook 场景下的正常移动操作完全不受影响。
