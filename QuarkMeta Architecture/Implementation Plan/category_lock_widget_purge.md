# 彻底清除 CategoryLockWidget 残留消灭 3 个 LNK2019 链接错误无脑实施方案 (Purge CategoryLockWidget Implementation Plan)

## 1. Overview（概述与解决的问题）

本实施方案专门解决因旧分类锁控件 `CategoryLockWidget.cpp` 已从 `CMakeLists.txt` 中解绑，但 `ContentPanel` 中依然残留其实例化与信号绑定所引发的 **3 个 LNK2019 无法解析的外部符号**链接错误：
1. **彻底清理 `ContentPanel.h`**：移除 `CategoryLockWidget` 前置声明与 `m_lockWidget` 指针成员变量。
2. **彻底清理 `ContentPanel.cpp`**：擦除 `#include "CategoryLockWidget.h"` 头文件包含，清理 `initUi()` 中的 `new CategoryLockWidget` 实例化、`m_viewStack` 控件挂载与 `connect(m_lockWidget, &CategoryLockWidget::unlocked, ...)` 信号监听，清理 `restoreActiveView()` 中的旧指针隐退调用。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/ContentPanel.h`
2. `src/ui/ContentPanel.cpp`
3. `QuarkMeta Architecture/QuarkMeta-Architecture-Planning.md`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/ContentPanel.h`
从 `ContentPanel.h` 中彻底清理前置声明与 `m_lockWidget` 成员变量。

```
<<<<<<< SEARCH
class CategoryLockWidget;
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    CategoryLockWidget* m_lockWidget = nullptr;
=======
>>>>>>> REPLACE
```

---

### 3.2 `src/ui/ContentPanel.cpp`
从 `ContentPanel.cpp` 中彻底清理头文件包含、`initUi()` 实例化/信号绑定与 `restoreActiveView()` 调用。

```
<<<<<<< SEARCH
#include "CategoryLockDialog.h"
#include "CategoryLockWidget.h"
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_lockWidget = new CategoryLockWidget(this);

    m_viewStack->addWidget(m_gridView);
    m_viewStack->addWidget(m_treeView);
    m_viewStack->addWidget(m_lockWidget);

    m_viewStack->setCurrentWidget(m_gridView);

    connect(m_lockWidget, &CategoryLockWidget::unlocked, this, [this](int id) {
        MainWindow* mw = nullptr;
        QWidget* parentWin = window();
        while (parentWin) {
            if ((mw = qobject_cast<MainWindow*>(parentWin))) break;
            parentWin = parentWin->parentWidget();
        }
        loadCategory(id);
    });
=======
    m_viewStack->addWidget(m_gridView);
    m_viewStack->addWidget(m_treeView);

    m_viewStack->setCurrentWidget(m_gridView);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::restoreActiveView() {
    if (m_lockWidget) {
        m_lockWidget->hide();
    }
    if (m_currentViewMode == ListView) {
        m_viewStack->setCurrentWidget(m_treeView);
    } else {
        m_viewStack->setCurrentWidget(m_gridView);
    }
}
=======
void ContentPanel::restoreActiveView() {
    if (m_currentViewMode == ListView) {
        m_viewStack->setCurrentWidget(m_treeView);
    } else {
        m_viewStack->setCurrentWidget(m_gridView);
    }
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译确认**：
   在命令行运行 CMake 编译，验证上述 3 个 LNK2019 无法解析的外部符号错误彻底消失，工程 **100% 成功链接**：
   ```bash
   cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```
2. **校验 Checkpoint**：
   全局搜索 `CategoryLockWidget` 与 `m_lockWidget`，确认全工程匹配计数彻底归零（0 匹配）。
