# 回收站按键统一执行清空回收站实施方案 (RecycleBin All Delete Keys Empty Trash Plan)

**方案版本**：v1.0  
**日期**：2026-11-07  
**需求描述**：进入回收站之后，无论是按下 `Ctrl+Space`、`Del` 还是 `Shift+Del`，执行的均是“清空回收站”逻辑接口（`PermanentDeleteService::instance().executeEmptyTrash(...)`）。

---

## 已读物理证据清单（第 1 关：Gate 1）

| 文件 | 确认已读 | 关键物理签名 |
|---|---|---|
| `QuarkMeta_Source/src/ui/PanelMediator.cpp` | ✅ | `connect(shortcutController, &AppShortcutController::moveToTrashRequested, ...)`（第 281-295 行） |
| `QuarkMeta_Source/src/ui/controllers/ContentKeyHandler.cpp` | ✅ | `keyEvent->key() == Qt::Key_Delete` 与 `(keyEvent->modifiers() & Qt::ControlModifier) && keyEvent->key() == Qt::Key_Space` 处理逻辑（第 612-639 行） |
| `QuarkMeta_Source/src/core/PermanentDeleteService.h` | ✅ | `bool executeEmptyTrash(QWidget* parentWidget = nullptr, bool isSecureShred = true);`（第 33 行） |

**防重复造轮子与根因检索记录**：
- 在回收站视图下，用户明确要求将 `Ctrl+Space`、`Del` 以及 `Shift+Del` 全部收敛路由至统一的“清空回收站”逻辑接口；
- `PermanentDeleteService::instance().executeEmptyTrash(...)` 为官方“清空回收站”SSOT 入口。

---

## Chapter 1: Overview（概述与根因分析）

### 设计目标
当且仅当当前处于回收站视图下（`isTrashView` 为 true），无论用户按下 `Ctrl+Space`、`Del` 还是 `Shift+Del`，统一调用 `PermanentDeleteService::instance().executeEmptyTrash(...)`。在非回收站的普通文件夹下，严格保持既有行为（`Del` 与 `Ctrl+Space` 移入回收站，`Shift+Del` 永久删除）。

---

## Chapter 2: Modified Files List（影响文件清单）

| 操作类型 | 文件相对路径 |
|---|---|
| **修改** | `QuarkMeta_Source/src/ui/controllers/ContentKeyHandler.cpp` |
| **修改** | `QuarkMeta_Source/Memories_QuarkMeta.md` |
| 不涉及 | `CMakeLists.txt`（无文件增删） |

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `QuarkMeta_Source/src/ui/controllers/ContentKeyHandler.cpp` — 回收站下 Delete 与 Shift+Del 统一路由至清空回收站

```
<<<<<<< SEARCH
    if ((keyEvent->modifiers() & Qt::ControlModifier) && keyEvent->key() == Qt::Key_Space) {
        bool isTrashView = (m_panel->getCurrentCategoryType() == "trash" || m_panel->currentPath() == "trash://");
        if (isTrashView) {
            PermanentDeleteService::instance().executeEmptyTrash(m_panel);
        } else {
            TrashService::instance().moveToTrash(m_panel->getSelectedPaths(), m_panel);
        }
        return true;
    }
    if (keyEvent->key() == Qt::Key_Delete) {
        bool isTrashView = (m_panel->getCurrentCategoryType() == "trash" || m_panel->currentPath() == "trash://");
        if (isTrashView) {
            QStringList selectedPaths = m_panel->getSelectedPaths();
            if (!selectedPaths.isEmpty()) {
                PermanentDeleteService::instance().execute(selectedPaths, m_panel);
            } else {
                ToolTipOverlay::instance()->showToast("请先选择要永久删除的项目", 2000, QColor("#e81123"), m_panel);
            }
        } else {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                PermanentDeleteService::instance().execute(m_panel->getSelectedPaths(), m_panel);
            } else {
                TrashService::instance().moveToTrash(m_panel->getSelectedPaths(), m_panel);
            }
        }
        return true;
    }
=======
    if (((keyEvent->modifiers() & Qt::ControlModifier) && keyEvent->key() == Qt::Key_Space) || keyEvent->key() == Qt::Key_Delete) {
        bool isTrashView = (m_panel->getCurrentCategoryType() == "trash" || m_panel->currentPath() == "trash://");
        if (isTrashView) {
            PermanentDeleteService::instance().executeEmptyTrash(m_panel);
        } else {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                PermanentDeleteService::instance().execute(m_panel->getSelectedPaths(), m_panel);
            } else {
                TrashService::instance().moveToTrash(m_panel->getSelectedPaths(), m_panel);
            }
        }
        return true;
    }
>>>>>>> REPLACE
```

### 3.2 `QuarkMeta_Source/Memories_QuarkMeta.md` — 更新回收站快捷键设计理念

```
<<<<<<< SEARCH
2. **快捷键触发规则与智能路由**：
   - 触发键位集合：`Del`、`Shift+Del`、`Ctrl+Space`；
   - **分支 A（有选中项）**：当用户选中了 1 个或多个回收站项目时，按下任一键位统一路由至**“永久删除选中项”**（`PermanentDeleteService::instance().execute(m_panel->getSelectedPaths(), m_panel)`）；
   - **分支 B（无选中项）**：当用户未选择任何项目或聚焦在空白处时，按下任一键位统一路由至**“清空回收站”**（`PermanentDeleteService::instance().executeEmptyTrash(m_panel)`）；
=======
2. **快捷键触发规则与智能路由**：
   - 触发键位集合：`Del`、`Shift+Del`、`Ctrl+Space`；
   - **统一收敛**：在回收站视图下，无论是否选中项目，按下 `Del`、`Shift+Del` 或 `Ctrl+Space` 中的任一键位，统一路由执行**“清空回收站”**逻辑接口（`PermanentDeleteService::instance().executeEmptyTrash(m_panel)`）；
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 编译验证步骤
```bash
# 因沙盒环境中不包含 Qt6 库，跳过实际编译步骤
```

### 运行时验收标准

| 场景 | 操作步骤 | 预期结果 |
|---|---|---|
| 回收站内按 Del | 进入回收站，无论有无选中项目，按 Del 键 | 弹出“确定要清空回收站吗？...”对话框，调用 `executeEmptyTrash` |
| 回收站内按 Shift+Del | 进入回收站，无论有无选中项目，按 Shift+Del | 弹出“确定要清空回收站吗？...”对话框，调用 `executeEmptyTrash` |
| 回收站内按 Ctrl+Space | 进入回收站，无论有无选中项目，按 Ctrl+Space | 弹出“确定要清空回收站吗？...”对话框，调用 `executeEmptyTrash` |
| 普通目录下按 Del / Ctrl+Space | 普通文件夹内选中文件，按 Del 或 Ctrl+Space | 移入回收站（保持既有行为） |
| 普通目录下按 Shift+Del | 普通文件夹内选中文件，按 Shift+Del | 永久删除选中项目（保持既有行为） |

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

| 功能 | 复用的官方 SSOT 入口 | 位置 |
|---|---|---|
| 清空回收站 | `PermanentDeleteService::instance().executeEmptyTrash` | `PermanentDeleteService.h` |

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | SSOT 来源 | 派生/消费点 | 同步方向 |
|---|---|---|---|
| 回收站视图状态 | `m_panel->getCurrentCategoryType()` / `m_panel->currentPath()` | `ContentKeyHandler` | 只读分支判定 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

| 调用 | 物理签名位置 | 访问级别 |
|---|---|---|
| `PermanentDeleteService::executeEmptyTrash(QWidget*, bool)` | `PermanentDeleteService.h`，第 33 行 | ✅ public |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

| 类型/API | `#include` 状态 |
|---|---|
| `PermanentDeleteService` | `ContentKeyHandler.cpp` 第 11 行已显式包含 ✅ |

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 编号 | 检查项 | 状态 | 位置/说明 |
|---|---|---|---|
| 1 | 方案已通过需求确认 | ✅ 满足 | 用户明确要求：进入回收站后按下 Ctrl+Space 或 Del 或 Shift+Del 执行“清空回收站” |
| 2 | 物理读取所有涉及文件（Gate 1） | ✅ 满足 | 见已读物理证据清单 |
| 3 | SEARCH 块原文真实存在且唯一匹配 | ✅ 满足 | 逐行核对真实存在且唯一匹配 |
| 4 | 新增接口声明、定义、调用点签名一致 | ✅ 满足 | 严格调用已有 public 接口 |
| 5 | 被修改的 `#include` 间接包含链闭合 | ✅ 满足 | 无需修改头文件包含 |
| 6 | 新建/删除文件已在 CMakeLists.txt 同步 | ✅ 不涉及 | 无文件增删 |
| 7 | 跨类调用访问级别均为 public | ✅ 满足 | 均为 public |
| 8 | 方案未触及范围外文件 | ✅ 满足 | 仅修改指定键盘分发与核心记忆 |
| 9 | 未采用笨拙浪费计算资源的手法 | ✅ 满足 | 纯键位聚合分流 |
| 10 | 代码中无打补丁行为 | ✅ 满足 | 统一收敛按键出口 |
| 11 | 遵循 Memories.md 规范 | ✅ 满足 | 同步更新记忆文档 |
| 12 | ToolTipOverlay 不追随鼠标 | ✅ 不涉及 | 未调用 ToolTipOverlay |
| 13 | 100% 语义化矢量图标 | ✅ 不涉及 | 无图标变动 |
| 14 | 异步回调按值捕获 | ✅ 不涉及 | 无新增异步 Lambda |
| 15 | 无 Lambda 循环引用 | ✅ 不涉及 | 无 Lambda |
| 16 | 后台线程 UI 操作回归主线程 | ✅ 满足 | UI 线程直接唤起清空弹窗 |
| 17 | Qt 信号槽使用 C++11 函数指针语法 | ✅ 不涉及 | 无信号槽变更 |
| 18 | 对外 `.h` 签名冻结 | ✅ 满足 | 零修改 `.h`，100% 冻结 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 行为零篡改声明

| 既有行为 | 本方案影响 | 结论 |
|---|---|---|
| 普通非回收站目录下的 Del、Shift+Del、Ctrl+Space | `isTrashView` 为 false 时完全直通原有删除与移入回收站逻辑 | ✅ 100% 原样保留 |
