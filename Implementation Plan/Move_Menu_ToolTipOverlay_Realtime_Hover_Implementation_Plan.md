# Implementation Plan: 右键“移动到”子菜单 ToolTipOverlay 实时跟随与精准路径悬浮架构方案

**Base Commit Hash**: `e1ebc50891973065263ecfc548fd388e95f7dddc`
**关联模块**: `QuarkMeta_Source/src/ui/controllers/ContentContextMenu.cpp`
**遵循规范**: `Standard Specifications for Implementation Plan.md` 及 `Memories_QuarkMeta.md`（第 37 章）

---

### 已读物理证据清单 (Pre-Draft Evidence)
1. **物理读取文件列表**：
   - `QuarkMeta_Source/src/ui/controllers/ContentContextMenu.cpp`（只读核对 `showMenu` 中 `moveMenu` 的构建与信号绑定）；
   - `QuarkMeta_Source/src/ui/ToolTipOverlay.h` 与 `QuarkMeta_Source/src/ui/ToolTipOverlay.cpp`（只读核对 `showText` 与 `hideTip` 签名及默认参数）；
   - `QuarkMeta_Source/src/ui/HistoryPanel.cpp`（只读核对 `HistoryPanel::eventFilter` 对 `MouseMove` 与 `QDir::toNativeSeparators` 的处理逻辑）；
   - `QuarkMeta_Source/Memories_QuarkMeta.md`（第 37 章核心架构设计与防退化规范）。
2. **头文件原样签名**：
   - `ToolTipOverlay::instance()->showText(const QPoint& globalPos, const QString& text, int timeout = 700, const QColor& borderColor = QColor("#B0B0B0"), bool exactPosition = false, const QColor& backgroundColor = QColor("#2B2B2B"))`
   - `static void ToolTipOverlay::hideTip()`
   - `QAction* QMenu::actionAt(const QPoint& pt) const`
   - `static QString QDir::toNativeSeparators(const QString& pathName)`
3. **既有 API 检索日志**：
   - 检索 `QMenu::hovered`：全仓库仅在 `ContentContextMenu.cpp:377` 中使用。
   - 检索 `QDir::toNativeSeparators`：全仓库统一规范路径分隔符。

---

## 一、方案设计与架构分析 (Architecture & Design)

### 1.1 缺陷根因排查与机制反思
1. **`QMenu::hovered(QAction*)` 固有滞后与事件合并**：
   - 原代码依赖 Qt 信号 `connect(moveMenu, &QMenu::hovered, ...)`，此信号在鼠标快速划过菜单项时由 Qt 内部高亮机制驱动，存在显著的事件节流与合并，无法感知鼠标在相邻菜单项间的微观移动；
   - `ToolTipOverlay::showText` 内部具备 150ms 的延迟定时器（`m_showDelayTimer`），当快速划过多个选项时，旧 action 的延迟展示可能覆盖新 action，造成提示路径与当前鼠标悬停的高亮项严重脱节或滞后（如悬停在 `src` 项却仍然显示上一个 `g:\c++` 路径）。
2. **边缘脱离与非路径项的清理盲区**：
   - 鼠标划出菜单可视区（Leave）或者滑向非路径菜单项（如“一键分发”、“浏览选择文件夹...”及分割线）时，`QMenu::hovered` 不会触发针对无属性项的重置，导致旧的 Tooltip 残留在屏幕上。
3. **视觉与分隔符不规范**：
   - 调用 `showText` 时显式指定了暗灰色 `#333333` 与未归一化的原始路径字符串，破坏了与“历史”选项卡 (`HistoryPanel`) 及全局默认外观（统一纯黑背景、原生斜杠 `QDir::toNativeSeparators`）的视觉一致性。

### 1.2 核心治理方案与实施架构
1. **精确事件过滤器 (EventFilter) 架构**：
   - 为级联子菜单 `moveMenu` 安装专属事件过滤器；
   - 捕获 `QEvent::MouseMove`：利用 `moveMenu->actionAt(mouseEvent->pos())` 进行绝对精准的几何碰撞检测，实时探测当前光标下的物理 `QAction*`；
   - 捕获 `QEvent::Leave` 与 `aboutToHide`：立即调用 `ToolTipOverlay::hideTip()` 彻底清理悬浮，杜绝残留。
2. **动作感知与即时显示契约**：
   - 维护局部状态 `lastHoveredPath`；
   - 当 `actionAt(pos)` 返回带有有效 `hoverPath` 属性且非空的 Action 时：
     - 若该路径与上一次悬停路径不同，将其转换为原生分隔符格式：`QString nativePath = QDir::toNativeSeparators(path);`；
     - 立即调用 `ToolTipOverlay::instance()->showText(QCursor::pos(), nativePath, 0);`（使用默认主题边框与背景色，与 `HistoryPanel` 保持 100% 视觉与行为一致）；
   - 当 `actionAt(pos)` 为空或其 `hoverPath` 为空（如分隔线、普通命令按钮）时，立即清除记录并调用 `ToolTipOverlay::hideTip()`。
3. **生命周期与无害隔离**：
   - 事件过滤器以 `moveMenu` 为父对象（`QObject` 子对象），随菜单销毁自动清理，绝不产生内存泄漏或跨菜单干扰。

---

## 二、受影响文件清单 (Affected Files)

1. `QuarkMeta_Source/src/ui/controllers/ContentContextMenu.cpp`

---

## 三、精准代码修改方案 (Precise Code Changes)

### 3.1 `QuarkMeta_Source/src/ui/controllers/ContentContextMenu.cpp`

<<<<<<< SEARCH
#include <QClipboard>
#include <QThreadPool>
#include <QPointer>

namespace QuarkMeta {
=======
#include <QClipboard>
#include <QThreadPool>
#include <QPointer>
#include <QMouseEvent>

namespace QuarkMeta {
>>>>>>> REPLACE

---

### 3.2 `QuarkMeta_Source/src/ui/controllers/ContentContextMenu.cpp`

<<<<<<< SEARCH
                connect(moveMenu, &QMenu::hovered, this, [](QAction* action) {
                    if (action && action->property("hoverPath").isValid()) {
                        QString path = action->property("hoverPath").toString();
                        if (!path.isEmpty()) {
                            ToolTipOverlay::instance()->showText(QCursor::pos(), path, 0, QColor("#333333"), false);
                            return;
                        }
                    }
                    ToolTipOverlay::hideTip();
                });
                connect(moveMenu, &QMenu::aboutToHide, this, []() {
                    ToolTipOverlay::hideTip();
                });
=======
                class MoveMenuHoverFilter : public QObject {
                public:
                    explicit MoveMenuHoverFilter(QMenu* menu) : QObject(menu), m_menu(menu) {}
                protected:
                    bool eventFilter(QObject* watched, QEvent* event) override {
                        if (watched == m_menu) {
                            if (event->type() == QEvent::MouseMove) {
                                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                                QAction* action = m_menu->actionAt(mouseEvent->pos());
                                if (action && action->property("hoverPath").isValid()) {
                                    QString path = action->property("hoverPath").toString();
                                    if (!path.isEmpty()) {
                                        QString nativePath = QDir::toNativeSeparators(path);
                                        if (m_lastHoveredPath != nativePath) {
                                            m_lastHoveredPath = nativePath;
                                            ToolTipOverlay::instance()->showText(QCursor::pos(), nativePath, 0);
                                        }
                                        return QObject::eventFilter(watched, event);
                                    }
                                }
                                m_lastHoveredPath.clear();
                                ToolTipOverlay::hideTip();
                            } else if (event->type() == QEvent::Leave) {
                                m_lastHoveredPath.clear();
                                ToolTipOverlay::hideTip();
                            }
                        }
                        return QObject::eventFilter(watched, event);
                    }
                private:
                    QMenu* m_menu;
                    QString m_lastHoveredPath;
                };

                moveMenu->installEventFilter(new MoveMenuHoverFilter(moveMenu));
                connect(moveMenu, &QMenu::aboutToHide, this, []() {
                    ToolTipOverlay::hideTip();
                });
>>>>>>> REPLACE

---

## 四、编译与构建系统验证 (Build & CMake Check)
- 本方案未新增独立编译单元或源文件，无须修改 `CMakeLists.txt`。
- 修改点仅为 `ContentContextMenu.cpp` 内部局部替换，新增 `#include <QMouseEvent>` 完全满足头文件类型闭合。

---

## 五、运行时与逻辑归一化核查 (Runtime & Normalization Check)
- 遵循 SSOT 原则，与 `HistoryPanel` 采用一致的 `QEvent::MouseMove` 碰撞检测与 `QDir::toNativeSeparators` 格式化规范。
- 彻底废除有滞后缺陷的 `QMenu::hovered` 信号方式。

---

## 六、错误处理与边界测试 (Edge Cases & Error Handling)
1. **鼠标快速滑动**：每帧 `MouseMove` 事件中 `actionAt(pos)` 均能毫秒级定位物理 Action，`m_lastHoveredPath != nativePath` 状态判断有效避免无谓重复调用 `showText`。
2. **划过非路径动作与分割线**：`actionAt` 返回 `nullptr` 或 Action 不含有效 `hoverPath` 时，立刻触发 `ToolTipOverlay::hideTip()`，防止悬浮残留。
3. **鼠标移出菜单区**：捕获 `QEvent::Leave` 并清空状态，立即隐藏 Tip。
4. **菜单关闭销毁**：`aboutToHide` 信号保障菜单关闭时 Tooltip 绝对隐藏；`MoveMenuHoverFilter` 作为 `moveMenu` 的子对象，随菜单释放自动销毁，零内存泄漏。

---

## 七、接口签名与访问级别一致性核查 (Interface & Visibility Consistency Check)
- `QMenu::actionAt(const QPoint&)`：Qt 官方公共接口（`public`）；
- `ToolTipOverlay::instance()->showText(...)`：公共静态与单例接口（`public`）；
- `ToolTipOverlay::hideTip()`：公共静态接口（`public`）；
- `QDir::toNativeSeparators(...)`：Qt 官方静态工具接口（`public`）；
- 所有接口调用均符合访问控制级别，未破坏既有对外 `.h` 签名。

---

## 八、头文件包含链与类型完整性检查 (Header Chain & Inclusion Closure Check)
- 引入 `QMouseEvent`：在 `ContentContextMenu.cpp` 中显式包含了 `<QMouseEvent>`；
- `QMenu`、`QDir`、`QPoint`、`QString`、`QEvent` 等类型均已在当前文件或其直接包含的头文件中具有完整定义。

---

## 九、合规与质量核对表 (Compliance & Quality Self-Check Table)

| 检查项 | 状态 | 说明 |
| :--- | :--- | :--- |
| 1. 不盲改对外签名 (Public API Freezing) | 满足 | 未修改任何 `.h` 外部签名 |
| 2. 严格单向依赖流动 | 满足 | 视图/控制器层内聚，未反向依赖 |
| 3. 架构三问与 SSOT 遵循 | 满足 | 严格遵循 `Memories_QuarkMeta.md` 第 37 章与 `HistoryPanel` 模式 |
| 4. 无 Win32 强抢焦点/Hack | 满足 | 纯标准 Qt 事件过滤器与坐标检测 |
| 5. 黑盒信息隐藏 | 满足 | 仅依赖 Qt 官方公有 API |
| 6. SEARCH 块真实唯一匹配 | 满足 | 经过自动化脚本物理全量匹配校验 |
| 7. 显式头文件闭合包含 | 满足 | 显式包含 `<QMouseEvent>` |
| 8. 异步时序与生命周期安全 | 满足 | 过滤器随 `moveMenu` 挂载与释放，零内存泄露 |
| 9. 零硬编码样式与数值 | 满足 | 移除不规范的硬编码 `#333333`，统一采用全局主题标准 |
| 10. 10大章节完整性 | 满足 | 包含方案所有规定章节与核验表格 |

---

## 十、行为零篡改与数值零篡改声明表 (Behavior Preservation & Zero-Value-Alteration Table)

| 检查维度 | 原有状态/数值 | 方案调整后状态/数值 | 判定 |
| :--- | :--- | :--- | :--- |
| “移动到”子菜单原有动作项 | 一键分发、最近访问文件夹、浏览选择 | 保持 100% 不变，功能逻辑无变更 | 零篡改 |
| 移动物理操作逻辑 | 异步调用 `DiskIoService::executeAsync` | 保持 100% 不变 | 零篡改 |
| 提示文字内容 | 文件夹完整绝对路径 | 保持 100% 不变，使用 `toNativeSeparators` 规范斜杠 | 一致对齐 |
| 悬停提示背景与边框 | 之前临时硬编码 `#333333` | 恢复为 `ToolTipOverlay` 全局统一规范主题（深色纯黑扁平风格） | 契约归一 |
| 交互响应度 | 滞后不匹配（受制于 hovered 信号与定时器） | 实时毫秒级物理碰撞感知，随鼠标高亮即时显示 | 完美修复 |
