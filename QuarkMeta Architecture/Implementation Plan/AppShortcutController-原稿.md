您指出的这一条**极其重要且切中本质**：

1. **命名与作用域纠偏（拒绝误导性命名）**：
   原类名 `GlobalShortcutController` 是严重的“名不副实”——它让人误以为是调用了操作系统的全局热键钩子（`RegisterHotKey`），而实际上这只是**应用内部（局内）的窗口级快捷键**。
2. **拒绝“手写 `eventFilter` 截获按键”的低级补丁做法**：
   在 `eventFilter` 里用 `if (key == Qt::Key_Z)` 硬截按键是一个巨大的隐患补丁：**当用户在任何输入框（`QLineEdit` / 搜索框 / 标签栏）里打字并按下 `Ctrl+Z`（想撤销打字）时，硬截获会越权拦截并直接触发全局文件撤销！**

---

### 一、 事实与原因分析

1. **手写 `eventFilter` 的事实缺陷**：
   - 依赖底层的 `QKeyEvent` 原始事件拦截；
   - 无法与当前获得焦点的控件（如正在重命名文件、正在搜索输入、正在编辑备注的输入框）协调冲突；
   - 必须在 `MainWindow` 内部重写 `keyPressEvent` 配合转发，破坏了主窗口的纯洁性。
2. **Qt 标准的应用内快捷键架构**：
   Qt 官方标准方案是使用 **`QShortcut`**，并将其作用域显式限定为 **`Qt::WindowShortcut`（仅当前主窗口处于活动状态时生效，绝不影响操作系统其他软件）**。

---

### 二、 正规架构方案：标准应用内快捷键管理器 (`AppShortcutController`)

彻底废除底层的 `eventFilter` 与按键拦截函数，改用标准的 Qt 声明式快捷键体系：

```
                          【标准应用内快捷键体系架构】

             ┌─────────────────────────────────────────────────────────────┐
             │       AppShortcutController (应用内局域快捷键管理器)          │
             │           (所有 QShortcut 严格绑定 Qt::WindowShortcut)       │
             └──────────────────────────────┬──────────────────────────────┘
                                            │
         ┌──────────────────┬───────────────┴──────────────┬──────────────────┐
         │                  │                              │                  │
         ▼ (F5)             ▼ (Ctrl+Z / Ctrl+Shift+Z)      ▼ (Alt+Q)          ▼ (Ctrl+F)
【 NavigationService 】 【 UndoManager 】         【 窗口置顶抽象 】   【 搜索控制器 】
• refresh() 刷新当前目录 • undo() / redo() 撤销重做   • 触发置顶状态翻转   • focusSearch() 聚焦
```

---

### 三、 规范代码实现

#### 1. 规范 `src/ui/AppShortcutController.h`（更名为应用内控制器）

```cpp
#pragma once

#include <QObject>
#include <QPointer>
#include <QWidget>
#include <QShortcut>

namespace QuarkMeta {

class SearchController;

/**
 * @brief 局内（应用内）快捷键控制器
 * 基于 Qt 标准 QShortcut 实现，严格限定为 Qt::WindowShortcut 作用域，绝不侵入操作系统全局
 */
class AppShortcutController : public QObject {
    Q_OBJECT

public:
    explicit AppShortcutController(QWidget* targetWindow,
                                  SearchController* searchController,
                                  QObject* parent = nullptr);
    ~AppShortcutController() override = default;

signals:
    /**
     * @brief Alt+Q 局内快捷键触发置顶状态翻转
     */
    void togglePinRequested();

private:
    void initShortcuts();

    QPointer<QWidget> m_window;
    QPointer<SearchController> m_searchController;
};

} // namespace QuarkMeta
```

---

#### 2. 规范 `src/ui/AppShortcutController.cpp`

```cpp
#include "AppShortcutController.h"
#include "SearchController.h"
#include "../core/NavigationService.h"
#include "../core/UndoManager.h"
#include <QLineEdit>

namespace QuarkMeta {

AppShortcutController::AppShortcutController(QWidget* targetWindow,
                                             SearchController* searchController,
                                             QObject* parent)
    : QObject(parent),
      m_window(targetWindow),
      m_searchController(searchController) {
    initShortcuts();
}

void AppShortcutController::initShortcuts() {
    if (!m_window) return;

    // 1. F5: 局内刷新当前目录
    QShortcut* scRefresh = new QShortcut(QKeySequence(Qt::Key_F5), m_window);
    scRefresh->setContext(Qt::WindowShortcut); // 👈 铁律：严格限制为当前窗口活动时生效
    connect(scRefresh, &QShortcut::activated, this, []() {
        NavigationService::instance().refresh();
    });

    // 2. Ctrl+Z: 局内文件撤销 (Qt 自动处理输入框冲突，输入框获焦时优先处理文字撤销)
    QShortcut* scUndo = new QShortcut(QKeySequence::Undo, m_window);
    scUndo->setContext(Qt::WindowShortcut);
    connect(scUndo, &QShortcut::activated, this, []() {
        UndoManager::instance().undo();
    });

    // 3. Ctrl+Shift+Z / Ctrl+Y: 局内文件重做
    QShortcut* scRedo = new QShortcut(QKeySequence::Redo, m_window);
    scRedo->setContext(Qt::WindowShortcut);
    connect(scRedo, &QShortcut::activated, this, []() {
        UndoManager::instance().redo();
    });

    // 4. Alt+Q: 局内切换窗口置顶
    QShortcut* scPin = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Q), m_window);
    scPin->setContext(Qt::WindowShortcut);
    connect(scPin, &QShortcut::activated, this, &AppShortcutController::togglePinRequested);

    // 5. Ctrl+F: 局内聚焦搜索栏
    QShortcut* scFind = new QShortcut(QKeySequence::Find, m_window);
    scFind->setContext(Qt::WindowShortcut);
    connect(scFind, &QShortcut::activated, this, [this]() {
        if (m_searchController && m_searchController->searchEdit()) {
            m_searchController->searchEdit()->setFocus(Qt::ShortcutFocusReason);
            m_searchController->searchEdit()->selectAll();
        }
    });
}

} // namespace QuarkMeta
```

---

#### 3. 彻底清除 `MainWindow.h` 与 `MainWindow.cpp` 中的按键拦截代码

- `MainWindow` 不再需要安装任何 `eventFilter` 给快捷键；
- 彻底删除 `MainWindow::keyPressEvent` 中关于快捷键的转发代码。

```cpp
// MainWindow.h 中：
// 声明纯净成员：
AppShortcutController* m_shortcutController = nullptr;

// MainWindow.cpp 构造函数中纯净装配：
m_shortcutController = new AppShortcutController(this, m_searchController, this);
connect(m_shortcutController, &AppShortcutController::togglePinRequested, this, [this]() {
    if (m_btnPinTop) {
        m_btnPinTop->setChecked(!m_btnPinTop->isChecked());
    }
});

// 🚨 MainWindow::keyPressEvent 彻底删除 handleKeyPress 判定，回归纯默认基类调用：
void MainWindow::keyPressEvent(QKeyEvent* event) {
    QMainWindow::keyPressEvent(event);
}
```

---

### 架构收益：
1. **完全符合 Qt 官方规范**：基于 `QShortcut(Qt::WindowShortcut)` 构建，**100% 局限于应用内部**，绝不注册操作系统级全局钩子，绝不干扰系统其他软件；
2. **彻底消灭按键冲突**：当用户在输入框打字按 `Ctrl+Z` 时，Qt 会智能优先响应输入框的文字撤销，不会误触全局文件撤销；
3. **彻底消灭 C2248 编译错误与 `eventFilter` 补丁**：代码纯粹声明式绑定，解耦彻底。