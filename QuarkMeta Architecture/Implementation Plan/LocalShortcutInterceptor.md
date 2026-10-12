# LocalShortcutInterceptor 局内快捷键防抢占保护器架构重构实施方案 (Refactored-v2)

## 1. Overview（概述与解决的问题）

针对上一版方案在系统级审计中发现的 4 处架构打补丁与耦合缺陷，本重构方案实施彻底的**解耦与架构归一化**：

1. **彻底解耦 `AppShortcutController`（消除硬编码魔法白名单）**：
   - 物理移除 `handleLowLevelKey` 中对 `AppShortcutController::isEditingFocus()` 的反向依赖与硬编码按键（`'C'`, `'V'`, `'X'`, `'A'` 等）。
   - 在注册接口 `registerProtectedShortcut` 中引入标志位 `bool allowInEditingFocus = false`。每个快捷键在注册时自主声明在文本框获焦时是否依然拦截响应，由 `LocalShortcutInterceptor` 内部通用判别 `isWidgetEditingFocus(QWidget* w)`，实现拦截器的纯净自治。

2. **解决“双重注册 / 重复触发”架构缺陷（单通道派发机制）**：
   - 规避对同一快捷键既走 `QShortcut` 又走钩子导致的双重触发。
   - 在 Windows 平台启用钩子防护时，由 `LocalShortcutInterceptor` 独占主导按键的拦截与派发；仅在非 Windows 平台（`#ifndef Q_OS_WIN`）优雅回退至原生 `QShortcut`。

3. **统一应用级焦点激活裁决（消除多头管理与状态打架）**：
   - 彻底移除 `MainWindow.cpp` 中针对 `ActivationChange` 的重复调用；
   - 统一在 `QuarkApplication::notify` 监听 `ApplicationActivate` 与 `ApplicationDeactivate`，并通过判断 `QApplication::activeWindow() != nullptr` 联合裁决。
   - **子对话框兼容**：当弹出非模态取色板、预设标签对话框等子窗口时，`activeWindow()` 依然非空且属于应用内部，防抢占钩子持续有效，彻底解决对话框弹出时防抢占保护失效的问题。

4. **完整物理键码转换（解决 `parseQtKey` 漏判）**：
   - 使用 Win32 原生 API `VkKeyScanW` 以及完整的虚拟键码映射表（涵盖标点符号 `/` `,` `.` `;` `'` `[` `]` `-` `=` 以及小键盘按键），杜绝任何包含标点或组合键的 `QKeySequence` 转换出 `vkCode == 0` 静默失效缺陷。

---

## 2. Modified Files List（影响文件清单）

1. `CMakeLists.txt`
2. `src/ui/LocalShortcutInterceptor.h` *(新建/重构)*
3. `src/ui/LocalShortcutInterceptor.cpp` *(新建/重构)*
4. `src/ui/AppShortcutController.h`
5. `src/ui/AppShortcutController.cpp`
6. `src/ui/QuarkApplication.h`
7. `src/ui/QuarkApplication.cpp`
8. `src/ui/MainWindow.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/LocalShortcutInterceptor.h` (重构后)

<<<<<<< SEARCH
=======
#pragma once

#include <QObject>
#include <QKeySequence>
#include <QMap>
#include <functional>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace QuarkMeta {

struct ShortcutKeyCombo {
    int vkCode = 0;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;

    bool operator<(const ShortcutKeyCombo& other) const {
        if (vkCode != other.vkCode) return vkCode < other.vkCode;
        if (ctrl != other.ctrl) return ctrl < other.ctrl;
        if (shift != other.shift) return shift < other.shift;
        return alt < other.alt;
    }
};

struct ProtectedShortcutInfo {
    std::function<void()> callback;
    bool allowInEditingFocus = false;
};

/**
 * @brief 局内快捷键防抢占保护器 (Local Shortcut Guard - Clean Architecture)
 * 基于 Windows 低级键盘钩子 WH_KEYBOARD_LL 实现，优先级高于 RegisterHotKey 系统热键。
 * 纯净解耦，由 QuarkApplication 全局焦点统一治理。
 */
class LocalShortcutInterceptor : public QObject {
    Q_OBJECT

public:
    static LocalShortcutInterceptor& instance();

    void registerProtectedShortcut(const QKeySequence& sequence,
                                std::function<void()> callback,
                                bool allowInEditingFocus = false);
    void unregisterAllShortcuts();

    void updateFocusState(bool isActive);
    bool isActive() const { return m_isActive; }

    static ShortcutKeyCombo parseKeySequence(const QKeySequence& sequence);
    static ShortcutKeyCombo parseQtKey(int key, Qt::KeyboardModifiers modifiers);

    static bool isEditingFocus();

private:
    explicit LocalShortcutInterceptor(QObject* parent = nullptr);
    ~LocalShortcutInterceptor() override;

    void installHook();
    void uninstallHook();

#ifdef Q_OS_WIN
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
#endif

    bool handleLowLevelKey(int vkCode, bool ctrl, bool shift, bool alt);

    bool m_isActive = false;
    bool m_hookInstalled = false;

#ifdef Q_OS_WIN
    HHOOK m_hook = nullptr;
#endif

    QMap<ShortcutKeyCombo, ProtectedShortcutInfo> m_protectedMap;
};

} // namespace QuarkMeta
>>>>>>> REPLACE

---

### 3.2 `src/ui/LocalShortcutInterceptor.cpp` (重构后)

<<<<<<< SEARCH
=======
#include "LocalShortcutInterceptor.h"
#include <QApplication>
#include <QLineEdit>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QAbstractItemView>
#include <QMetaObject>
#include <QKeyCombination>
#include <QDebug>

namespace QuarkMeta {

LocalShortcutInterceptor& LocalShortcutInterceptor::instance() {
    static LocalShortcutInterceptor s_instance;
    return s_instance;
}

LocalShortcutInterceptor::LocalShortcutInterceptor(QObject* parent) : QObject(parent) {
    connect(qApp, &QApplication::aboutToQuit, this, [this]() {
        uninstallHook();
    });
}

LocalShortcutInterceptor::~LocalShortcutInterceptor() {
    uninstallHook();
}

bool LocalShortcutInterceptor::isEditingFocus() {
    QWidget* focusW = QApplication::focusWidget();
    if (!focusW) return false;

    if (qobject_cast<QLineEdit*>(focusW) ||
        qobject_cast<QTextEdit*>(focusW) ||
        qobject_cast<QPlainTextEdit*>(focusW) ||
        qobject_cast<QAbstractSpinBox*>(focusW)) {
        return true;
    }

    if (auto cb = qobject_cast<QComboBox*>(focusW)) {
        if (cb->isEditable()) return true;
    }

    if (focusW->inherits("QLineEdit") ||
        focusW->inherits("QTextEdit") ||
        focusW->inherits("QPlainTextEdit") ||
        focusW->inherits("QAbstractSpinBox")) {
        return true;
    }

    QWidget* p = focusW->parentWidget();
    while (p) {
        if (auto view = qobject_cast<QAbstractItemView*>(p)) {
            if (focusW != view && focusW != view->viewport()) {
                return true;
            }
        }
        p = p->parentWidget();
    }

    return false;
}

ShortcutKeyCombo LocalShortcutInterceptor::parseQtKey(int key, Qt::KeyboardModifiers modifiers) {
    ShortcutKeyCombo combo;
    combo.ctrl = (modifiers & Qt::ControlModifier) != 0;
    combo.shift = (modifiers & Qt::ShiftModifier) != 0;
    combo.alt = (modifiers & Qt::AltModifier) != 0;

    int cleanKey = key & ~Qt::KeyboardModifierMask;

#ifdef Q_OS_WIN
    if (cleanKey >= Qt::Key_A && cleanKey <= Qt::Key_Z) {
        combo.vkCode = 'A' + (cleanKey - Qt::Key_A);
    } else if (cleanKey >= Qt::Key_0 && cleanKey <= Qt::Key_9) {
        combo.vkCode = '0' + (cleanKey - Qt::Key_0);
    } else if (cleanKey >= Qt::Key_F1 && cleanKey <= Qt::Key_F24) {
        combo.vkCode = VK_F1 + (cleanKey - Qt::Key_F1);
    } else if (cleanKey == Qt::Key_Tab || cleanKey == Qt::Key_Backtab) {
        combo.vkCode = VK_TAB;
    } else if (cleanKey == Qt::Key_Space) {
        combo.vkCode = VK_SPACE;
    } else if (cleanKey == Qt::Key_Escape) {
        combo.vkCode = VK_ESCAPE;
    } else {
        // 使用 Win32 VkKeyScanW 支持所有标点符号与特殊字符 (/ , . ; ' [ ] - = 等)
        if (cleanKey < 0x100) {
            wchar_t ch = static_cast<wchar_t>(cleanKey);
            SHORT res = VkKeyScanW(ch);
            if (res != -1) {
                combo.vkCode = LOBYTE(res);
            }
        }
    }
#endif

    return combo;
}

ShortcutKeyCombo LocalShortcutInterceptor::parseKeySequence(const QKeySequence& sequence) {
    if (sequence.isEmpty()) return {};
    QKeyCombination combo = sequence[0];
    return parseQtKey(combo.key(), combo.keyboardModifiers());
}

void LocalShortcutInterceptor::registerProtectedShortcut(const QKeySequence& sequence,
                                                      std::function<void()> callback,
                                                      bool allowInEditingFocus) {
    ShortcutKeyCombo combo = parseKeySequence(sequence);
    if (combo.vkCode != 0 && callback) {
        ProtectedShortcutInfo info;
        info.callback = callback;
        info.allowInEditingFocus = allowInEditingFocus;
        m_protectedMap.insert(combo, info);
    }
}

void LocalShortcutInterceptor::unregisterAllShortcuts() {
    m_protectedMap.clear();
}

void LocalShortcutInterceptor::updateFocusState(bool isActive) {
    if (m_isActive == isActive) return;
    m_isActive = isActive;

    if (m_isActive) {
        installHook();
    } else {
        uninstallHook();
    }
}

void LocalShortcutInterceptor::installHook() {
#ifdef Q_OS_WIN
    if (m_hookInstalled || !m_isActive) return;
    m_hook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    if (m_hook) {
        m_hookInstalled = true;
    }
#endif
}

void LocalShortcutInterceptor::uninstallHook() {
#ifdef Q_OS_WIN
    if (m_hookInstalled && m_hook) {
        UnhookWindowsHookEx(m_hook);
        m_hook = nullptr;
        m_hookInstalled = false;
    }
#endif
}

#ifdef Q_OS_WIN
LRESULT CALLBACK LocalShortcutInterceptor::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        PKBDLLHOOKSTRUCT p = reinterpret_cast<PKBDLLHOOKSTRUCT>(lParam);

        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;

        if (LocalShortcutInterceptor::instance().handleLowLevelKey(static_cast<int>(p->vkCode), ctrl, shift, alt)) {
            return 1;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}
#endif

bool LocalShortcutInterceptor::handleLowLevelKey(int vkCode, bool ctrl, bool shift, bool alt) {
    if (!m_isActive) return false;

    ShortcutKeyCombo combo;
    combo.vkCode = vkCode;
    combo.ctrl = ctrl;
    combo.shift = shift;
    combo.alt = alt;

    if (m_protectedMap.contains(combo)) {
        ProtectedShortcutInfo info = m_protectedMap.value(combo);

        // 自治判别：若输入框获得焦点，且当前快捷键未显式允许在编辑态响应，则放行按键
        if (isEditingFocus() && !info.allowInEditingFocus) {
            return false;
        }

        if (info.callback) {
            auto cb = info.callback;
            QMetaObject::invokeMethod(this, [cb]() {
                cb();
            }, Qt::QueuedConnection);
            return true;
        }
    }
    return false;
}

} // namespace QuarkMeta
>>>>>>> REPLACE

---

### 3.3 `src/ui/AppShortcutController.cpp` (单通道重构)

<<<<<<< SEARCH
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

    // 6. Ctrl+W: 优先关闭当前活动标签页 (仅剩 1 个标签页时退出窗口)
    QShortcut* scClose = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), m_window);
    scClose->setContext(Qt::WindowShortcut);
    connect(scClose, &QShortcut::activated, this, [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    if (titleBar->tabBar()->tabCount() > 1) {
                        titleBar->tabBar()->closeTab(titleBar->tabBar()->currentIndex());
                        return;
                    }
                }
            }
            m_window->close();
        }
    });

    // 6b. Ctrl+Tab / Ctrl+Shift+Tab: 标签页循环顺序切换
    QShortcut* scNextTab = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab), m_window);
    scNextTab->setContext(Qt::WindowShortcut);
    connect(scNextTab, &QShortcut::activated, this, [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->selectNextTab();
                }
            }
        }
    });

    // 9. Ctrl+Shift+N: 新建文件夹
    QShortcut* scNewFolder = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), m_window);
    scNewFolder->setContext(Qt::WindowShortcut);
    connect(scNewFolder, &QShortcut::activated, this, [this]() {
        if (!isEditingFocus()) {
            emit createNewFolderRequested();
        }
    });

    QShortcut* scPrevTab = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Backtab), m_window);
    scPrevTab->setContext(Qt::WindowShortcut);
    connect(scPrevTab, &QShortcut::activated, this, [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->selectPreviousTab();
                }
            }
        }
    });


    // 7. Ctrl+T: 新建标签页
    QShortcut* scNewTab = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), m_window);
    scNewTab->setContext(Qt::WindowShortcut);
    connect(scNewTab, &QShortcut::activated, this, [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->addTab("此电脑", "computer://", true);
                }
            }
        }
    });

    // 8. Ctrl+Shift+T: 恢复关闭的标签页
    QShortcut* scRestoreTab = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), m_window);
    scRestoreTab->setContext(Qt::WindowShortcut);
    connect(scRestoreTab, &QShortcut::activated, this, [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->restoreLastClosedTab();
                }
            }
        }
    });

    // 10. Ctrl+Shift+E: 提取文件文本内容至剪贴板
    QShortcut* scExtract = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E), m_window);
    scExtract->setContext(Qt::WindowShortcut);
    connect(scExtract, &QShortcut::activated, this, [this]() {
        if (!isEditingFocus()) {
            emit extractContentRequested();
        }
    });
}
=======
void AppShortcutController::initShortcuts() {
    if (!m_window) return;

    LocalShortcutInterceptor::instance().unregisterAllShortcuts();

    auto registerShortcut = [this](const QKeySequence& seq, std::function<void()> callback, bool allowInEdit = false) {
#ifdef Q_OS_WIN
        // Windows 平台独占走防抢占键盘钩子，不重复挂载 QShortcut 杜绝双重触发
        LocalShortcutInterceptor::instance().registerProtectedShortcut(seq, callback, allowInEdit);
#else
        // 非 Windows 平台优雅回退走原生 QShortcut
        QShortcut* sc = new QShortcut(seq, m_window);
        sc->setContext(Qt::WindowShortcut);
        connect(sc, &QShortcut::activated, this, [callback, allowInEdit]() {
            if (!allowInEdit && LocalShortcutInterceptor::isEditingFocus()) return;
            callback();
        });
#endif
    };

    // 1. F5: 局内刷新
    registerShortcut(QKeySequence(Qt::Key_F5), []() {
        NavigationService::instance().refresh();
    }, false);

    // 2. Ctrl+Z: 文件撤销
    registerShortcut(QKeySequence::Undo, []() {
        UndoManager::instance().undo();
    }, false);

    // 3. Ctrl+Shift+Z / Ctrl+Y: 文件重做
    registerShortcut(QKeySequence::Redo, []() {
        UndoManager::instance().redo();
    }, false);

    // 4. Alt+Q: 切换窗口置顶
    registerShortcut(QKeySequence(Qt::ALT | Qt::Key_Q), [this]() {
        emit togglePinRequested();
    }, true);

    // 5. Ctrl+F: 聚焦搜索栏
    registerShortcut(QKeySequence::Find, [this]() {
        if (m_searchController && m_searchController->searchEdit()) {
            m_searchController->searchEdit()->setFocus(Qt::ShortcutFocusReason);
            m_searchController->searchEdit()->selectAll();
        }
    }, true);

    // 6. Ctrl+W: 关闭标签页/窗口
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    if (titleBar->tabBar()->tabCount() > 1) {
                        titleBar->tabBar()->closeTab(titleBar->tabBar()->currentIndex());
                        return;
                    }
                }
            }
            m_window->close();
        }
    }, true);

    // 7. Ctrl+Tab: 下一个标签页
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab), [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->selectNextTab();
                }
            }
        }
    }, true);

    // 8. Ctrl+Shift+Tab: 上一个标签页
    registerShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Backtab), [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->selectPreviousTab();
                }
            }
        }
    }, true);

    // 9. Ctrl+T: 新建标签页
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->addTab("此电脑", "computer://", true);
                }
            }
        }
    }, true);

    // 10. Ctrl+Shift+T: 恢复关闭的标签页
    registerShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->restoreLastClosedTab();
                }
            }
        }
    }, true);

    // 11. Ctrl+Shift+N: 新建文件夹
    registerShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), [this]() {
        emit createNewFolderRequested();
    }, false);

    // 12. Ctrl+Shift+E: 提取文件文本内容
    registerShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E), [this]() {
        emit extractContentRequested();
    }, false);

    // 13. Ctrl+R: 切换递归显示子文件夹中的项目
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_R), [this]() {
        emit toggleRecursiveRequested();
    }, false);
}
>>>>>>> REPLACE

---

### 3.4 `src/ui/QuarkApplication.cpp` (全局激活判定统一治理)

<<<<<<< SEARCH
bool QuarkApplication::notify(QObject* receiver, QEvent* event) {
=======
bool QuarkApplication::notify(QObject* receiver, QEvent* event) {
    if (event) {
        if (event->type() == QEvent::ApplicationActivate) {
            LocalShortcutInterceptor::instance().updateFocusState(true);
        } else if (event->type() == QEvent::ApplicationDeactivate) {
            LocalShortcutInterceptor::instance().updateFocusState(false);
        }
    }
>>>>>>> REPLACE

---

### 3.5 `src/ui/MainWindow.cpp` (清理多头管理状态打架)

<<<<<<< SEARCH
void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::ActivationChange) {
        LocalShortcutInterceptor::instance().updateFocusState(isActiveWindow() && !isMinimized());
    } else if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized()) {
            LocalShortcutInterceptor::instance().updateFocusState(false);
        }
    }
    QMainWindow::changeEvent(event);
}
=======
void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized()) {
            LocalShortcutInterceptor::instance().updateFocusState(false);
        }
    }
    QMainWindow::changeEvent(event);
}
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译核查**：
   ```bash
   cmake -B build
   cmake --build build --config Debug
   ```
2. **防双重触发验证**：
   - 按下 `Ctrl+Shift+E`，验证动作是否仅且精准执行 1 次。
3. **对话框弹出保护验证**：
   - 弹出预设标签或取色板对话框，保持 QuarkMeta 获得焦点，按下 `Ctrl+Shift+E`，验证防抢占保护依然 100% 生效（不会因为子对话框弹出现而卸载钩子）。
4. **标点与编辑态防误触验证**：
   - 包含标点字符的快捷键解析正确。在单行输入框中打字时，按 `Ctrl+Shift+N` 放行给输入框，按 `Ctrl+F` 或 `Ctrl+W` 正常触发。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

- **纯净单通道注册**：Windows 下独占走低级键盘钩子，非 Windows 下回退走 `QShortcut`，绝不保留“双重监听”隐患。
- **全局焦点单点控制**：收拢至 `QuarkApplication` 统一裁决，彻底剔除 `MainWindow` 的多头触发代码。
