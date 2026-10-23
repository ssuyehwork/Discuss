# Ctrl+R 快捷键切换“显示子文件夹中的项目”实施方案

## 1. Overview（概述与解决的问题）

为 `QuarkMeta` 新增 **`Ctrl + R` 局内快捷键**，使用户不仅可以通过点击 `ContentHeaderWidget` 顶栏上的图标按钮（`m_btnLayers`），还可以随时通过按下 `Ctrl + R` 组合键直接翻转当前激活内容面板（`m_activeContentPanel`）的递归加载状态，调用唯一的 SSOT 视角加载通道 `ContentPanel::loadDirectory(m_currentPath, !m_isRecursive)` 原位刷出子文件夹项目。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/AppShortcutController.h`
2. `src/ui/AppShortcutController.cpp`
3. `src/ui/PanelMediator.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/AppShortcutController.h`

<<<<<<< SEARCH
    /**
     * @brief Ctrl+Shift+E 触发提取文本内容至剪贴板
     */
    void extractContentRequested();
=======
    /**
     * @brief Ctrl+Shift+E 触发提取文本内容至剪贴板
     */
    void extractContentRequested();

    /**
     * @brief Ctrl+R 触发切换递归显示子文件夹中的项目
     */
    void toggleRecursiveRequested();
>>>>>>> REPLACE

---

### 3.2 `src/ui/AppShortcutController.cpp`

<<<<<<< SEARCH
    // 10. Ctrl+Shift+E: 提取文件文本内容至剪贴板
    QShortcut* scExtract = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E), m_window);
    scExtract->setContext(Qt::WindowShortcut);
    connect(scExtract, &QShortcut::activated, this, [this]() {
        if (!isEditingFocus()) {
            emit extractContentRequested();
        }
    });
=======
    // 10. Ctrl+Shift+E: 提取文件文本内容至剪贴板
    QShortcut* scExtract = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E), m_window);
    scExtract->setContext(Qt::WindowShortcut);
    connect(scExtract, &QShortcut::activated, this, [this]() {
        if (!isEditingFocus()) {
            emit extractContentRequested();
        }
    });

    // 11. Ctrl+R: 切换递归显示子文件夹中的项目
    auto fnRecursive = [this]() {
        if (!isEditingFocus()) {
            emit toggleRecursiveRequested();
        }
    };
    QShortcut* scRecursive = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_R), m_window);
    scRecursive->setContext(Qt::WindowShortcut);
    connect(scRecursive, &QShortcut::activated, this, fnRecursive);
    LocalShortcutInterceptor::instance().registerProtectedShortcut(QKeySequence(Qt::CTRL | Qt::Key_R), fnRecursive);
>>>>>>> REPLACE

---

### 3.3 `src/ui/PanelMediator.cpp`

<<<<<<< SEARCH
        connect(shortcutController, &AppShortcutController::extractContentRequested, this, [this, contentPanel]() {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                QStringList selectedPaths = target->getSelectedPaths();
                if (!selectedPaths.isEmpty()) {
                    ContextMenuFactory::extractContentToClipboard(selectedPaths.first());
                }
            }
        });
=======
        connect(shortcutController, &AppShortcutController::extractContentRequested, this, [this, contentPanel]() {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                QStringList selectedPaths = target->getSelectedPaths();
                if (!selectedPaths.isEmpty()) {
                    ContextMenuFactory::extractContentToClipboard(selectedPaths.first());
                }
            }
        });

        connect(shortcutController, &AppShortcutController::toggleRecursiveRequested, this, [this, contentPanel]() {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target && !target->currentPath().isEmpty() && target->currentPath() != "computer://") {
                bool nextState = !target->isRecursive();
                target->setIsRecursive(nextState);
                target->loadDirectory(target->currentPath(), nextState);
            }
        });
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **执行编译验证**：
   ```bash
   cmake -B build
   cmake --build build --config Debug
   ```
2. **运行界面验证**：
   - 打开某个物理文件夹路径（如 `D:/Photos`）。
   - 按下 `Ctrl + R` 快捷键，检查顶栏“显示子文件夹中的项目”按钮图标是否高亮切换，且列表是否成功刷出深层子文件夹中的所有项目。
   - 再次按下 `Ctrl + R`，验证递归显示关闭，回归单层级目录列表。
   - 焦点移入输入框（如搜索框、名称修改框）时按下 `Ctrl + R`，验证触发输入的正常字符 `r`，快捷键自动避让。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

- **SSOT 复用**：切换递归动作严格调用既有入口 `target->setIsRecursive(nextState)` 与 `target->loadDirectory(target->currentPath(), nextState)`，绝对禁止私自复制扫描代码或另起炉灶。
- **活动面板动态路由**：在中介者 `PanelMediator` 中绑定 `m_activeContentPanel`，确保在多窗格分屏模式下 `Ctrl + R` 始终精准作用于当前聚焦激活的窗格。

---

## 6. Header API Signature Verification

| 被调用接口 / 方法 | 所在头文件 | 物理精确签名 |
| :--- | :--- | :--- |
| `ContentPanel::isRecursive` | `src/ui/ContentPanel.h` | `bool isRecursive() const` |
| `ContentPanel::setIsRecursive` | `src/ui/ContentPanel.h` | `void setIsRecursive(bool recursive)` |
| `ContentPanel::loadDirectory` | `src/ui/ContentPanel.h` | `void loadDirectory(const QString& path, bool recursive = false)` |

---

## 7. Header Inclusion Chain & Type Completeness Check

- `src/ui/AppShortcutController.h`: 显式新增 `void toggleRecursiveRequested()` 信号；
- `src/ui/PanelMediator.cpp`: 已包含 `#include "ContentPanel.h"` 与 `#include "AppShortcutController.h"`，受影响指针调用具备完整的物理头文件定义。
