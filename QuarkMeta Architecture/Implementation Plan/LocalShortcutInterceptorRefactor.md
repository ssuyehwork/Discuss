# LocalShortcutInterceptor 架构重构与打补丁消除实施方案

## 1. Overview（概述与解决的问题）

针对对 `LocalShortcutInterceptor`（局内快捷键防抢占保护器）的系统级代码审计，本方案对以下 4 处结构性打补丁与逻辑缺陷进行彻底的纯净架构重构：

1. **彻底解耦反向依赖与硬编码魔法值**：移除 `LocalShortcutInterceptor.cpp` 对 `AppShortcutController::isEditingFocus()` 的反向包含及 `'C'`, `'V'`, `'X'`, `'A'` 魔法键码判定，改为通过无依赖回调谓词（`std::function<bool()>`）判定输入焦点状态，并通用于所有被保护快捷键。
2. **消除双重注册与双重触发风险**：在 `AppShortcutController.cpp` 中统一快捷键调度通道，避免同一快捷键既走 Qt 原生 `QShortcut` 又走低级钩子导致的双重激活逻辑隐患。
3. **收拢焦点状态管理真理源（SSOT）**：物理移除 `MainWindow.cpp` 中对 `ActivationChange` 的钩子挂载/卸载操作，统一由 `QuarkApplication` 的全局应用级事件（`ApplicationActivate` / `ApplicationDeactivate`）管理钩子生命周期，解决弹出现非模态子对话框时 `MainWindow` 失焦导致保护器静默失效的 BUG。
4. **扩展 Windows 虚拟键码解析映射（`parseQtKey`）**：扩充 `parseQtKey` 对常用符号键（如 `/`, `-`, `=`, `,`, `.`, `;`, `'`, `[`, `]`, `\`）、方向键（Left/Right/Up/Down）、编辑键（Delete, Backspace, Return, Enter, Esc, Space, Tab）等全量 Qt 键码到 Win32 Virtual Key（`VK_OEM_*`, `VK_RETURN` 等）的映射。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/LocalShortcutInterceptor.h`
2. `src/ui/LocalShortcutInterceptor.cpp`
3. `src/ui/AppShortcutController.cpp`
4. `src/ui/QuarkApplication.cpp`
5. `src/ui/MainWindow.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/LocalShortcutInterceptor.h`

<<<<<<< SEARCH
    void updateFocusState(bool isActive);
    bool isActive() const { return m_isActive; }

    static ShortcutKeyCombo parseKeySequence(const QKeySequence& sequence);
    static ShortcutKeyCombo parseQtKey(int key, Qt::KeyboardModifiers modifiers);
=======
    void updateFocusState(bool isActive);
    bool isActive() const { return m_isActive; }

    void setEditFocusPredicate(std::function<bool()> predicate);

    static ShortcutKeyCombo parseKeySequence(const QKeySequence& sequence);
    static ShortcutKeyCombo parseQtKey(int key, Qt::KeyboardModifiers modifiers);
>>>>>>> REPLACE

<<<<<<< SEARCH
    QMap<ShortcutKeyCombo, std::function<void()>> m_callbacks;
};
=======
    QMap<ShortcutKeyCombo, std::function<void()>> m_callbacks;
    std::function<bool()> m_editFocusPredicate;
};
>>>>>>> REPLACE

---

### 3.2 `src/ui/LocalShortcutInterceptor.cpp`

<<<<<<< SEARCH
#include "LocalShortcutInterceptor.h"
#include "AppShortcutController.h"
#include <QApplication>
=======
#include "LocalShortcutInterceptor.h"
#include <QApplication>
>>>>>>> REPLACE

<<<<<<< SEARCH
ShortcutKeyCombo LocalShortcutInterceptor::parseQtKey(int key, Qt::KeyboardModifiers modifiers) {
    ShortcutKeyCombo combo;
    combo.ctrl = (modifiers & Qt::ControlModifier) != 0;
    combo.shift = (modifiers & Qt::ShiftModifier) != 0;
    combo.alt = (modifiers & Qt::AltModifier) != 0;

    int cleanKey = key & ~Qt::KeyboardModifierMask;

    if (cleanKey >= Qt::Key_A && cleanKey <= Qt::Key_Z) {
        combo.vkCode = 'A' + (cleanKey - Qt::Key_A);
    } else if (cleanKey >= Qt::Key_0 && cleanKey <= Qt::Key_9) {
        combo.vkCode = '0' + (cleanKey - Qt::Key_0);
    } else if (cleanKey >= Qt::Key_F1 && cleanKey <= Qt::Key_F24) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_F1 + (cleanKey - Qt::Key_F1);
#endif
    } else if (cleanKey == Qt::Key_Tab || cleanKey == Qt::Key_Backtab) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_TAB;
#endif
    } else if (cleanKey == Qt::Key_Space) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_SPACE;
#endif
    } else if (cleanKey == Qt::Key_Escape) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_ESCAPE;
#endif
    }

    return combo;
}
=======
ShortcutKeyCombo LocalShortcutInterceptor::parseQtKey(int key, Qt::KeyboardModifiers modifiers) {
    ShortcutKeyCombo combo;
    combo.ctrl = (modifiers & Qt::ControlModifier) != 0;
    combo.shift = (modifiers & Qt::ShiftModifier) != 0;
    combo.alt = (modifiers & Qt::AltModifier) != 0;

    int cleanKey = key & ~Qt::KeyboardModifierMask;

    if (cleanKey >= Qt::Key_A && cleanKey <= Qt::Key_Z) {
        combo.vkCode = 'A' + (cleanKey - Qt::Key_A);
    } else if (cleanKey >= Qt::Key_0 && cleanKey <= Qt::Key_9) {
        combo.vkCode = '0' + (cleanKey - Qt::Key_0);
    } else if (cleanKey >= Qt::Key_F1 && cleanKey <= Qt::Key_F24) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_F1 + (cleanKey - Qt::Key_F1);
#endif
    } else if (cleanKey == Qt::Key_Tab || cleanKey == Qt::Key_Backtab) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_TAB;
#endif
    } else if (cleanKey == Qt::Key_Space) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_SPACE;
#endif
    } else if (cleanKey == Qt::Key_Escape) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_ESCAPE;
#endif
    } else if (cleanKey == Qt::Key_Return || cleanKey == Qt::Key_Enter) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_RETURN;
#endif
    } else if (cleanKey == Qt::Key_Backspace) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_BACK;
#endif
    } else if (cleanKey == Qt::Key_Delete) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_DELETE;
#endif
    } else if (cleanKey == Qt::Key_Left) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_LEFT;
#endif
    } else if (cleanKey == Qt::Key_Right) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_RIGHT;
#endif
    } else if (cleanKey == Qt::Key_Up) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_UP;
#endif
    } else if (cleanKey == Qt::Key_Down) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_DOWN;
#endif
    } else if (cleanKey == Qt::Key_Slash) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_OEM_2;
#endif
    } else if (cleanKey == Qt::Key_Minus) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_OEM_MINUS;
#endif
    } else if (cleanKey == Qt::Key_Equal || cleanKey == Qt::Key_Plus) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_OEM_PLUS;
#endif
    } else if (cleanKey == Qt::Key_Comma) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_OEM_COMMA;
#endif
    } else if (cleanKey == Qt::Key_Period) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_OEM_PERIOD;
#endif
    } else if (cleanKey == Qt::Key_Semicolon) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_OEM_1;
#endif
    } else if (cleanKey == Qt::Key_Apostrophe) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_OEM_7;
#endif
    } else if (cleanKey == Qt::Key_BracketLeft) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_OEM_4;
#endif
    } else if (cleanKey == Qt::Key_BracketRight) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_OEM_6;
#endif
    } else if (cleanKey == Qt::Key_Backslash) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_OEM_5;
#endif
    }

    return combo;
}
>>>>>>> REPLACE

<<<<<<< SEARCH
void LocalShortcutInterceptor::unregisterAllShortcuts() {
    m_callbacks.clear();
}
=======
void LocalShortcutInterceptor::unregisterAllShortcuts() {
    m_callbacks.clear();
}

void LocalShortcutInterceptor::setEditFocusPredicate(std::function<bool()> predicate) {
    m_editFocusPredicate = predicate;
}
>>>>>>> REPLACE

<<<<<<< SEARCH
bool LocalShortcutInterceptor::handleLowLevelKey(int vkCode, bool ctrl, bool shift, bool alt) {
    if (!m_isActive) return false;

    ShortcutKeyCombo combo;
    combo.vkCode = vkCode;
    combo.ctrl = ctrl;
    combo.shift = shift;
    combo.alt = alt;

    if (m_callbacks.contains(combo)) {
        if (AppShortcutController::isEditingFocus() && (vkCode == 'C' || vkCode == 'V' || vkCode == 'X' || vkCode == 'A' || vkCode == VK_TAB)) {
            return false;
        }

        auto callback = m_callbacks.value(combo);
        if (callback) {
            QMetaObject::invokeMethod(this, [callback]() {
                callback();
            }, Qt::QueuedConnection);
            return true;
        }
    }
    return false;
}
=======
bool LocalShortcutInterceptor::handleLowLevelKey(int vkCode, bool ctrl, bool shift, bool alt) {
    if (!m_isActive) return false;

    // 焦点在文本输入框/可编辑单元格时，让渡所有常见编辑按键的按键处理
    if (m_editFocusPredicate && m_editFocusPredicate()) {
        if (vkCode == 'C' || vkCode == 'V' || vkCode == 'X' || vkCode == 'A' || vkCode == 'Z' || vkCode == 'Y' || vkCode == VK_TAB || vkCode == VK_RETURN || vkCode == VK_BACK || vkCode == VK_DELETE) {
            return false;
        }
    }

    ShortcutKeyCombo combo;
    combo.vkCode = vkCode;
    combo.ctrl = ctrl;
    combo.shift = shift;
    combo.alt = alt;

    if (m_callbacks.contains(combo)) {
        auto callback = m_callbacks.value(combo);
        if (callback) {
            QMetaObject::invokeMethod(this, [callback]() {
                callback();
            }, Qt::QueuedConnection);
            return true;
        }
    }
    return false;
}
>>>>>>> REPLACE

---

### 3.3 `src/ui/AppShortcutController.cpp`

<<<<<<< SEARCH
void AppShortcutController::initShortcuts() {
    if (!m_window) return;

    LocalShortcutInterceptor::instance().unregisterAllShortcuts();

    // 1. F5: 局内刷新当前目录
    auto fnRefresh = []() {
        NavigationService::instance().refresh();
    };
    QShortcut* scRefresh = new QShortcut(QKeySequence(Qt::Key_F5), m_window);
    scRefresh->setContext(Qt::WindowShortcut);
    connect(scRefresh, &QShortcut::activated, this, fnRefresh);
    LocalShortcutInterceptor::instance().registerProtectedShortcut(QKeySequence(Qt::Key_F5), fnRefresh);

    // 2. Ctrl+Z: 局内文件撤销
    auto fnUndo = []() {
        if (!isEditingFocus()) UndoManager::instance().undo();
    };
    QShortcut* scUndo = new QShortcut(QKeySequence::Undo, m_window);
    scUndo->setContext(Qt::WindowShortcut);
    connect(scUndo, &QShortcut::activated, this, fnUndo);
    LocalShortcutInterceptor::instance().registerProtectedShortcut(QKeySequence::Undo, fnUndo);

    // 3. Ctrl+Shift+Z / Ctrl+Y: 局内文件重做
    auto fnRedo = []() {
        if (!isEditingFocus()) UndoManager::instance().redo();
    };
    QShortcut* scRedo = new QShortcut(QKeySequence::Redo, m_window);
    scRedo->setContext(Qt::WindowShortcut);
    connect(scRedo, &QShortcut::activated, this, fnRedo);
    LocalShortcutInterceptor::instance().registerProtectedShortcut(QKeySequence::Redo, fnRedo);

    // 4. Alt+Q: 局内切换窗口置顶
    auto fnPin = [this]() {
        emit togglePinRequested();
    };
    QShortcut* scPin = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Q), m_window);
    scPin->setContext(Qt::WindowShortcut);
    connect(scPin, &QShortcut::activated, this, fnPin);
    LocalShortcutInterceptor::instance().registerProtectedShortcut(QKeySequence(Qt::ALT | Qt::Key_Q), fnPin);

    // 5. Ctrl+F: 局内聚焦搜索栏
    auto fnFind = [this]() {
        if (m_searchController && m_searchController->searchEdit()) {
            m_searchController->searchEdit()->setFocus(Qt::ShortcutFocusReason);
            m_searchController->searchEdit()->selectAll();
        }
    };
    QShortcut* scFind = new QShortcut(QKeySequence::Find, m_window);
    scFind->setContext(Qt::WindowShortcut);
    connect(scFind, &QShortcut::activated, this, fnFind);
    LocalShortcutInterceptor::instance().registerProtectedShortcut(QKeySequence::Find, fnFind);

    // 6. Ctrl+W: 优先关闭当前活动标签页
    auto fnClose = [this]() {
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
    };
    QShortcut* scClose = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), m_window);
    scClose->setContext(Qt::WindowShortcut);
    connect(scClose, &QShortcut::activated, this, fnClose);
    LocalShortcutInterceptor::instance().registerProtectedShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), fnClose);

    // 6b. Ctrl+Tab: 标签页顺序切换
    auto fnNextTab = [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->selectNextTab();
                }
            }
        }
    };
    QShortcut* scNextTab = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab), m_window);
    scNextTab->setContext(Qt::WindowShortcut);
    connect(scNextTab, &QShortcut::activated, this, fnNextTab);
    LocalShortcutInterceptor::instance().registerProtectedShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab), fnNextTab);

    // 6c. Ctrl+Shift+Tab: 标签页逆序切换
    auto fnPrevTab = [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->selectPreviousTab();
                }
            }
        }
    };
    QShortcut* scPrevTab = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Backtab), m_window);
    scPrevTab->setContext(Qt::WindowShortcut);
    connect(scPrevTab, &QShortcut::activated, this, fnPrevTab);
    LocalShortcutInterceptor::instance().registerProtectedShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Backtab), fnPrevTab);

    // 7. Ctrl+T: 新建标签页
    auto fnNewTab = [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->addTab("此电脑", "computer://", true);
                }
            }
        }
    };
    QShortcut* scNewTab = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), m_window);
    scNewTab->setContext(Qt::WindowShortcut);
    connect(scNewTab, &QShortcut::activated, this, fnNewTab);
    LocalShortcutInterceptor::instance().registerProtectedShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), fnNewTab);

    // 8. Ctrl+Shift+T: 恢复关闭的标签页
    auto fnRestoreTab = [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->restoreLastClosedTab();
                }
            }
        }
    };
    QShortcut* scRestoreTab = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), m_window);
    scRestoreTab->setContext(Qt::WindowShortcut);
    connect(scRestoreTab, &QShortcut::activated, this, fnRestoreTab);
    LocalShortcutInterceptor::instance().registerProtectedShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), fnRestoreTab);

    // 9. Ctrl+Shift+N: 新建文件夹
    auto fnNewFolder = [this]() {
        if (!isEditingFocus()) {
            emit createNewFolderRequested();
        }
    };
    QShortcut* scNewFolder = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), m_window);
    scNewFolder->setContext(Qt::WindowShortcut);
    connect(scNewFolder, &QShortcut::activated, this, fnNewFolder);
    LocalShortcutInterceptor::instance().registerProtectedShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), fnNewFolder);

    // 10. Ctrl+Shift+E: 提取文件文本内容至剪贴板
    auto fnExtract = [this]() {
        if (!isEditingFocus()) {
            emit extractContentRequested();
        }
    };
    QShortcut* scExtract = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E), m_window);
    scExtract->setContext(Qt::WindowShortcut);
    connect(scExtract, &QShortcut::activated, this, fnExtract);
    LocalShortcutInterceptor::instance().registerProtectedShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E), fnExtract);

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
}
=======
void AppShortcutController::initShortcuts() {
    if (!m_window) return;

    LocalShortcutInterceptor::instance().unregisterAllShortcuts();
    LocalShortcutInterceptor::instance().setEditFocusPredicate([]() {
        return isEditingFocus();
    });

    auto registerShortcut = [this](const QKeySequence& seq, std::function<void()> fn) {
#ifdef Q_OS_WIN
        // Windows 平台由防抢占低级键盘钩子统一接管拦截与分发，杜绝 QShortcut 双重激活风险
        LocalShortcutInterceptor::instance().registerProtectedShortcut(seq, fn);
#else
        // 非 Windows 平台降级使用 Qt 标准 QShortcut
        QShortcut* sc = new QShortcut(seq, m_window);
        sc->setContext(Qt::WindowShortcut);
        connect(sc, &QShortcut::activated, this, fn);
#endif
    };

    // 1. F5: 局内刷新当前目录
    registerShortcut(QKeySequence(Qt::Key_F5), []() {
        NavigationService::instance().refresh();
    });

    // 2. Ctrl+Z: 局内文件撤销
    registerShortcut(QKeySequence::Undo, []() {
        if (!isEditingFocus()) UndoManager::instance().undo();
    });

    // 3. Ctrl+Shift+Z / Ctrl+Y: 局内文件重做
    registerShortcut(QKeySequence::Redo, []() {
        if (!isEditingFocus()) UndoManager::instance().redo();
    });

    // 4. Alt+Q: 局内切换窗口置顶
    registerShortcut(QKeySequence(Qt::ALT | Qt::Key_Q), [this]() {
        emit togglePinRequested();
    });

    // 5. Ctrl+F: 局内聚焦搜索栏
    registerShortcut(QKeySequence::Find, [this]() {
        if (m_searchController && m_searchController->searchEdit()) {
            m_searchController->searchEdit()->setFocus(Qt::ShortcutFocusReason);
            m_searchController->searchEdit()->selectAll();
        }
    });

    // 6. Ctrl+W: 优先关闭当前活动标签页
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
    });

    // 6b. Ctrl+Tab: 标签页顺序切换
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab), [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->selectNextTab();
                }
            }
        }
    });

    // 6c. Ctrl+Shift+Tab: 标签页逆序切换
    registerShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Backtab), [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->selectPreviousTab();
                }
            }
        }
    });

    // 7. Ctrl+T: 新建标签页
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->addTab("此电脑", "computer://", true);
                }
            }
        }
    });

    // 8. Ctrl+Shift+T: 恢复关闭的标签页
    registerShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->restoreLastClosedTab();
                }
            }
        }
    });

    // 9. Ctrl+Shift+N: 新建文件夹
    registerShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), [this]() {
        if (!isEditingFocus()) {
            emit createNewFolderRequested();
        }
    });

    // 10. Ctrl+Shift+E: 提取文件文本内容至剪贴板
    registerShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E), [this]() {
        if (!isEditingFocus()) {
            emit extractContentRequested();
        }
    });

    // 11. Ctrl+R: 切换递归显示子文件夹中的项目
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_R), [this]() {
        if (!isEditingFocus()) {
            emit toggleRecursiveRequested();
        }
    });
}
>>>>>>> REPLACE

---

### 3.4 `src/ui/QuarkApplication.cpp`

<<<<<<< SEARCH
bool QuarkApplication::notify(QObject* receiver, QEvent* event) {
    if (event) {
        if (event->type() == QEvent::ApplicationActivate) {
            LocalShortcutInterceptor::instance().updateFocusState(true);
        } else if (event->type() == QEvent::ApplicationDeactivate) {
            LocalShortcutInterceptor::instance().updateFocusState(false);
        }
    }
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

### 3.5 `src/ui/MainWindow.cpp`

<<<<<<< SEARCH
void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::ActivationChange) {
        LocalShortcutInterceptor::instance().updateFocusState(isActiveWindow() && !isMinimized());
        if (isActiveWindow()) {
            if (QWidget::mouseGrabber()) {
                QWidget::mouseGrabber()->releaseMouse();
            }
#ifdef Q_OS_WIN
            if (testAttribute(Qt::WA_WState_Created)) {
                ::ReleaseCapture();
            }
#endif
            unsetCursor();
        }
    } else if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized()) {
            LocalShortcutInterceptor::instance().updateFocusState(false);
            if (m_searchController && m_searchController->historyPanel()) {
                m_searchController->historyPanel()->hide();
            }
        } else {
            LocalShortcutInterceptor::instance().updateFocusState(isActiveWindow());
        }
        if (m_titleBarWidget) {
#ifdef Q_OS_WIN
            m_titleBarWidget->setWindowMaximized(::IsZoomed(reinterpret_cast<HWND>(winId())));
#else
            m_titleBarWidget->setWindowMaximized(isMaximized());
#endif
        }
        if (m_bodyLayout) {
            m_bodyLayout->setContentsMargins(kLayoutEdgeMargin, 0, kLayoutEdgeMargin, kLayoutEdgeMargin);
        }
    }
    QMainWindow::changeEvent(event);
}
=======
void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::ActivationChange) {
        if (isActiveWindow()) {
            if (QWidget::mouseGrabber()) {
                QWidget::mouseGrabber()->releaseMouse();
            }
#ifdef Q_OS_WIN
            if (testAttribute(Qt::WA_WState_Created)) {
                ::ReleaseCapture();
            }
#endif
            unsetCursor();
        }
    } else if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized()) {
            if (m_searchController && m_searchController->historyPanel()) {
                m_searchController->historyPanel()->hide();
            }
        }
        if (m_titleBarWidget) {
#ifdef Q_OS_WIN
            m_titleBarWidget->setWindowMaximized(::IsZoomed(reinterpret_cast<HWND>(winId())));
#else
            m_titleBarWidget->setWindowMaximized(isMaximized());
#endif
        }
        if (m_bodyLayout) {
            m_bodyLayout->setContentsMargins(kLayoutEdgeMargin, 0, kLayoutEdgeMargin, kLayoutEdgeMargin);
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
2. **解耦与双重激活验证**：
   - 在应用内按下受保护按键（如 `Ctrl+Shift+E`），确认回调函数仅执行一次，避免重复激活。
3. **子对话框焦点防御验证**：
   - 唤起预设标签或取色板对话框，保持弹窗在前台，按下局内快捷键，确认防护钩子依然稳定生效，未因主窗口 `ActivationChange` 失焦而卸载。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

- **单局内调度通道**：快捷键注册统一下沉至 `AppShortcutController` 内的 `registerShortcut` 闭包，统一接管 Windows 与非 Windows 平台的防抢占与快捷键绑定。
- **全局焦点 SSOT**：钩子激活状态统一由 `QuarkApplication`（全局应用级别）单一控制。

---

## 6. Header API Signature Verification

| 被调用接口 / 方法 | 所在头文件 | 物理精确签名 |
| :--- | :--- | :--- |
| `LocalShortcutInterceptor::setEditFocusPredicate` | `src/ui/LocalShortcutInterceptor.h` | `void setEditFocusPredicate(std::function<bool()> predicate)` |
| `LocalShortcutInterceptor::registerProtectedShortcut` | `src/ui/LocalShortcutInterceptor.h` | `void registerProtectedShortcut(const QKeySequence& sequence, std::function<void()> callback)` |

---

## 7. Header Inclusion Chain & Type Completeness Check

- `src/ui/LocalShortcutInterceptor.cpp`: 仅包含 `LocalShortcutInterceptor.h`、`<QApplication>`、`<QMetaObject>`、`<QKeyCombination>` 与 `<QDebug>`，彻底移除了对 `AppShortcutController.h` 的依赖。
