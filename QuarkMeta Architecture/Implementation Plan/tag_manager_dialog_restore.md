# 恢复 TagManagerDialog 对话框并弃用 TagManagerView 实施方案 (Restore TagManagerDialog & Deprecate TagManagerView Implementation Plan)

## 1. Overview（概述与解决的问题）

本实施方案旨在按照最新架构指令，重新激活并恢复 **`TagManagerDialog`** 高级无边框对话框作为全局标签管理的主要入口，并弃用内嵌式的 `TagManagerView` 视图：
1. **重新启用并注册 `TagManagerDialog`**：确保 `src/ui/TagManagerDialog.h` 与 `src/ui/TagManagerDialog.cpp` 完好保留，并明确注册在 `CMakeLists.txt` 中参与编译。
2. **“标签管理”按钮（`m_btnTagManager`）重新绑定弹窗**：在 `MainWindow.cpp` 中，将顶部“标签管理”矢量按钮的点击事件重新绑定调用 `TagManagerDialog::showDialog(this, m_currentPath, false)`，一键弹出标签管理界面。
3. **清理 `TagManagerView` 内嵌视图绑定**：从 `MainWindow` 的 Splitter 布局中清理 `m_tagManagerView`，保持主功能区纯净流畅。

---

## 2. Modified Files List（影响文件清单）

1. `CMakeLists.txt`
2. `src/ui/MainWindow.h`
3. `src/ui/MainWindow.cpp`
4. `QuarkMeta Architecture/QuarkMeta-Architecture-Planning.md`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `CMakeLists.txt`
在 `CMakeLists.txt` 中明确保留并注册 `TagManagerDialog.h` 和 `TagManagerDialog.cpp` 编译源码目标。

```
<<<<<<< SEARCH
    src/ui/BatchCreateDialog.h
=======
    src/ui/BatchCreateDialog.h
    src/ui/TagManagerDialog.cpp
    src/ui/TagManagerDialog.h
>>>>>>> REPLACE
```

---

### 3.2 `src/ui/MainWindow.cpp`
在 `MainWindow.cpp` 中引用 `#include "TagManagerDialog.h"`，并将 `m_btnTagManager` 按钮的点击事件绑定为直接调用 `TagManagerDialog::showDialog` 弹出对话框。

```
<<<<<<< SEARCH
#include "DriveButton.h"
=======
#include "DriveButton.h"
#include "TagManagerDialog.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 🚨 归一化架构：彻底放弃 TagManagerDialog 旧弹窗，统一切换至内嵌式 TagManagerView 矢量视图
    connect(m_btnTagManager, &QPushButton::clicked, this, [this]() {
        if (m_tagManagerView) {
            m_isTagManagerMode = !m_isTagManagerMode;
            m_tagManagerView->setVisible(m_isTagManagerMode);
            m_contentPanel->setVisible(!m_isTagManagerMode);
            if (m_isTagManagerMode) {
                m_tagManagerView->refresh();
            }
        }
    });
=======
    // 🚨 最新架构指令：点击“标签管理”按钮，弹出 TagManagerDialog 高级无边框对话框
    connect(m_btnTagManager, &QPushButton::clicked, this, [this]() {
        TagManagerDialog::showDialog(this, m_currentPath, false);
    });
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译确认**：
   在命令行运行 CMake 编译，验证 `TagManagerDialog` 编译注册正常，全工程无符号缺失报错：
   ```bash
   cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```
2. **功能验证**：
   - **点击触发验证**：启动程序后点击顶部“标签管理”矢量按钮，确认立刻弹出来高级无边框对话框 `TagManagerDialog`。
   - **对话框交互验证**：在弹出的 `TagManagerDialog` 对话框中进行标签管理与搜索，功能完备且展现正常。
