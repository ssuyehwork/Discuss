# QuarkMeta 应用局域快捷键控制器实施方案 (AppShortcutController.md)

## 1. Overview（概述与解决的问题）

### 1.1 解决的问题
1. **修正误导性命名**：原 `GlobalShortcutController` 名称误导开发者以为调用了操作系统全局快捷键钩子（`RegisterHotKey`），而实际仅为窗口局域控制。
2. **彻底解决输入框按键冲突**：原方案依赖手写 `eventFilter` 硬截获 `QKeyEvent`。当用户在输入框（如搜索栏/文件重命名框）打字并按 `Ctrl+Z` 撤销打字时，硬截获会越权拦截并误触发全局文件系统撤销，造成数据风险。
3. **消除私有成员依赖与 `friend class` 侵入**：废除访问 `MainWindow` 私有成员的做法，采用标准的声明式 `QShortcut` 绑定与信号通知。

### 1.2 重构目标
1. 废除 `GlobalShortcutController.h/cpp`，新建标准的 `AppShortcutController.h/cpp`。
2. 全线采用 Qt 官方声明式 `QShortcut`，作用域显式指定为 `Qt::WindowShortcut`。Qt 会智能协调输入框焦点，输入框获焦时自动优先响应文字撤销。
3. 彻底删除 `MainWindow` 内部的快捷键转发算式与事件过滤器。

---

## 2. Modified Files List（影响文件清单）

1. `CMakeLists.txt`（将 `GlobalShortcutController` 替换注册为 `AppShortcutController`）
2. `src/ui/GlobalShortcutController.h`（彻底删除）
3. `src/ui/GlobalShortcutController.cpp`（彻底删除）
4. `src/ui/AppShortcutController.h`（全新创建，定义应用局域快捷键控制器）
5. `src/ui/AppShortcutController.cpp`（全新创建，基于 QShortcut 实现声明式绑定）
6. `src/ui/MainWindow.h`（清理旧成员声明，更换为 AppShortcutController*）
7. `src/ui/MainWindow.cpp`（清理 keyPressEvent 转发，装配 AppShortcutController 并绑定置顶信号）

---

## 3. Detailed Line-by-Line Changes（精准代码替换块）

### 3.1 `CMakeLists.txt` 替换注册

<<<<<<< SEARCH
    src/ui/GlobalShortcutController.cpp
    src/ui/GlobalShortcutController.h
=======
    src/ui/AppShortcutController.cpp
    src/ui/AppShortcutController.h
>>>>>>> REPLACE

---

### 3.2 新建 `src/ui/AppShortcutController.h`

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

### 3.3 新建 `src/ui/AppShortcutController.cpp`

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
    scRefresh->setContext(Qt::WindowShortcut);
    connect(scRefresh, &QShortcut::activated, this, []() {
        NavigationService::instance().refresh();
    });

    // 2. Ctrl+Z: 局内文件撤销 (输入框获焦时 Qt 底层优先响应文字撤销)
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

### 3.4 `src/ui/MainWindow.h` 替换类成员

<<<<<<< SEARCH
class GlobalShortcutController;
=======
class AppShortcutController;
>>>>>>> REPLACE

<<<<<<< SEARCH
    GlobalShortcutController* m_shortcutController = nullptr;
=======
    AppShortcutController* m_shortcutController = nullptr;
>>>>>>> REPLACE

---

### 3.5 `src/ui/MainWindow.cpp` 组装与 KeyPress 清理

<<<<<<< SEARCH
#include "GlobalShortcutController.h"
=======
#include "AppShortcutController.h"
>>>>>>> REPLACE

<<<<<<< SEARCH
    m_shortcutController = new GlobalShortcutController(this, this);
=======
    m_shortcutController = new AppShortcutController(this, m_searchController, this);
    connect(m_shortcutController, &AppShortcutController::togglePinRequested, this, [this]() {
        if (m_btnPinTop) {
            m_btnPinTop->setChecked(!m_btnPinTop->isChecked());
        }
    });
>>>>>>> REPLACE

<<<<<<< SEARCH
void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (m_shortcutController && m_shortcutController->handleKeyPress(event)) {
        return;
    }

    setAttribute(Qt::WA_Hover);
    QMainWindow::keyPressEvent(event);
}
=======
void MainWindow::keyPressEvent(QKeyEvent* event) {
    setAttribute(Qt::WA_Hover);
    QMainWindow::keyPressEvent(event);
}
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 4.1 构建步骤
```bash
cmake -B build
cmake --build build --config Release
```

### 4.2 验证用例
1. **输入框撤销冲突校验**：双击重命名某文件或在搜索输入框中打字（如输入 `"abc"`），按下 `Ctrl+Z`，验证是否仅撤销文字输入而绝对不触发底层文件操作撤销。
2. **快捷键生效校验**：按 `F5` 校验当前目录是否正常刷新；按 `Ctrl+F` 校验搜索框是否获焦并全选；按 `Alt+Q` 校验窗口置顶状态是否正常翻转。
3. **后台焦点隔离校验**：将应用最小化或切换至其他软件（如 Notepad），按 `Ctrl+Z` 或 `F5`，校验快捷键是否不会对外部系统软件造成任何误触侵入。
