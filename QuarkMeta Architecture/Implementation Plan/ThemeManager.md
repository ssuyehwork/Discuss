# ThemeManager 全局统一主题与样式中枢实施方案 (ThemeManager.md)

## Overview
本实施方案旨在彻底解决全系统 UI 样式碎片化散落（`StyleLibrary.h`、`SvgIconRenderer.cpp`、`MainWindow.cpp`）与托盘/右键菜单样式缺失问题，通过建立全局唯一的 **`ThemeManager`** 主题中枢（Single Source of Styling Truth），一键统领 `qApp` 全局 QSS 与菜单透光等组件级特定样式。

---

## Modified Files List
1. `src/ui/ThemeManager.h` (新建)
2. `src/ui/ThemeManager.cpp` (新建)
3. `CMakeLists.txt` (注册新建源码)
4. `src/main.cpp` (全局 1 行注入 initialize)
5. `src/ui/UiHelper.h` (转发平滑平移至 ThemeManager)
6. `src/ui/SvgIconRenderer.h/cpp` (彻底清理解耦 applyMenuStyle)
7. `src/ui/MainWindow.cpp` (清空手写 QSS 拼接代码)

---

## Detailed Line-by-Line Changes

### 1. `src/ui/ThemeManager.h` (新建)
```cpp
#pragma once

#include <QObject>
#include <QApplication>
#include <QWidget>
#include <QColor>
#include <QString>

namespace QuarkMeta {

/**
 * @brief 全局唯一主题与样式管理器 (Single Source of Styling Truth)
 */
class ThemeManager : public QObject {
    Q_OBJECT

public:
    static ThemeManager& instance();

    /**
     * @brief 应用程序启动时一键注入全局暗黑主题
     */
    void initialize(QApplication* app);

    /**
     * @brief 为任意弹出菜单 (QMenu) 一键赋予标准的精致暗黑样式 (包括托盘菜单)
     */
    void applyMenuStyle(QWidget* menu) const;

    /**
     * @brief 获取权威的全局 QSS 样式表字符串
     */
    QString getGlobalStyleSheet() const;

private:
    explicit ThemeManager(QObject* parent = nullptr);
    ~ThemeManager() override = default;
};

} // namespace QuarkMeta
```

### 2. `src/ui/ThemeManager.cpp` (新建)
```cpp
#include "ThemeManager.h"
#include <QFile>

namespace QuarkMeta {

ThemeManager& ThemeManager::instance() {
    static ThemeManager inst;
    return inst;
}

ThemeManager::ThemeManager(QObject* parent) : QObject(parent) {}

void ThemeManager::initialize(QApplication* app) {
    if (!app) return;
    app->setStyleSheet(getGlobalStyleSheet());
}

QString ThemeManager::getGlobalStyleSheet() const {
    QFile file(":/style.qss");
    if (file.open(QFile::ReadOnly)) {
        return QLatin1String(file.readAll());
    }

    return QString(R"(
        /* 1. 顶层窗口与基础容器 */
        QMainWindow, QDialog { background-color: #1E1E1E; color: #EEEEEE; }
        QWidget#CentralWidget { background-color: #1E1E1E; }
        #SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
            background-color: #1E1E1E; border: none; border-radius: 0px;
        }
        #ContainerHeader { background-color: #252526; border-bottom: 1px solid #333333; }

        /* 2. 统一全软件 QMenu 菜单 (包括托盘菜单与右键菜单) */
        QMenu {
            background-color: #252526;
            color: #EEEEEE;
            border: 1px solid #333333;
            border-radius: 6px;
            padding: 4px;
        }
        QMenu::item {
            background-color: transparent;
            color: #EEEEEE;
            padding: 6px 24px 6px 12px;
            border-radius: 4px;
            font-size: 12px;
        }
        QMenu::item:selected {
            background-color: #378ADD;
            color: #FFFFFF;
        }
        QMenu::separator {
            height: 1px;
            background-color: #333333;
            margin: 4px 6px;
        }

        /* 3. 统一全局滚动条 */
        QScrollBar:vertical { border: none; background: transparent; width: 10px; }
        QScrollBar::handle:vertical { background: #333333; min-height: 20px; border-radius: 3px; }
        QScrollBar::handle:vertical:hover { background: #4E4E52; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { width: 0px; height: 0px; }
        QScrollBar:horizontal { border: none; background: transparent; height: 10px; }
        QScrollBar::handle:horizontal { background: #333333; min-width: 20px; border-radius: 3px; }
        QScrollBar::handle:horizontal:hover { background: #4E4E52; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; height: 0px; }

        /* 4. 统一全局输入框 */
        QLineEdit, QTextEdit, QPlainTextEdit {
            background: #252526; border: 1px solid #333333; border-radius: 4px; color: #EEEEEE; padding-left: 6px;
        }
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus { border: 1px solid #378ADD; }
    )");
}

void ThemeManager::applyMenuStyle(QWidget* menu) const {
    if (!menu) return;
    menu->setAttribute(Qt::WA_TranslucentBackground, true);
    menu->setWindowFlags(menu->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
}

} // namespace QuarkMeta
```

### 3. `CMakeLists.txt` 注册
在 `SOURCES` 和 `HEADERS` 列表中加入 `src/ui/ThemeManager.cpp` 与 `src/ui/ThemeManager.h`。

### 4. `src/ui/UiHelper.h` 转发调整
```cpp
<<<<<<< SEARCH
    static inline void applyMenuStyle(QWidget* menu) {
        SvgIconRenderer::applyMenuStyle(menu);
    }
=======
    static inline void applyMenuStyle(QWidget* menu) {
        ThemeManager::instance().applyMenuStyle(menu);
    }
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. 运行 CMake 构建工程，确认 `ThemeManager.h` / `ThemeManager.cpp` 正确参与 MOC 与二进制编译。
2. 检查全局各个右键上下文菜单、托盘菜单、输入框及主窗口底色，确认全系统 QSS 统一样式无死角生效。
