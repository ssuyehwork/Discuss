# 清理 TagManagerDialog 僵尸代码与归一化至 TagManagerView 实施方案 (Purge TagManagerDialog & Unify to TagManagerView Implementation Plan)

## 1. Overview（概述与解决的问题）

本实施方案旨在彻底消灭被废弃的 `TagManagerDialog` 旧弹窗（僵尸代码），并将标签管理功能彻底归一化收拢至主界面内嵌式的 **`TagManagerView`** 矢量大视图：
1. **废弃旧弹窗并消除符号报错**：物理删除 `TagManagerDialog.h` 与 `TagManagerDialog.cpp`，并清理 `CategoryPanel.cpp` 中的 `#include "TagManagerDialog.h"` 头文件引用，彻底杜绝违规调用旧弹窗带来的“无法解析的外部符号”及白底旧弹窗视觉问题。
2. **标签管理按钮归一化至 `TagManagerView`**：在 `MainWindow.cpp` 中将 `m_btnTagManager` 点击槽函数重构绑定为内嵌式 **`m_tagManagerView`** 视图的显示/切换逻辑，实现主功能区无缝平滑切至标签管理矢量视图。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/MainWindow.cpp`
2. `src/ui/CategoryPanel.cpp`
3. `src/ui/TagManagerDialog.h` (物理删除)
4. `src/ui/TagManagerDialog.cpp` (物理删除)
5. `QuarkMeta Architecture/QuarkMeta-Architecture-Planning.md`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/MainWindow.cpp`
将 `m_btnTagManager` 的点击事件从调用已废弃的旧弹窗 `TagManagerDialog::showDialog` 重构为切换展示内嵌式的 `m_tagManagerView` 视图。

```
<<<<<<< SEARCH
    connect(m_btnTagManager, &QPushButton::clicked, this, [this]() {
        TagManagerDialog::showDialog(this, m_currentPath, false);
    });
=======
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
>>>>>>> REPLACE
```

---

### 3.2 `src/ui/CategoryPanel.cpp`
移除废弃旧弹窗 `#include "TagManagerDialog.h"` 头文件引用。

```
<<<<<<< SEARCH
#include "TagManagerDialog.h"
=======
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译确认**：
   在命令行运行 CMake 编译，验证全工程无符号缺失错误：
   ```bash
   cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```
2. **功能验证**：
   - **清除验证**：代码中不再包含 `TagManagerDialog.h`，绝无“无法解析的外部符号”问题。
   - **切换验证**：点击顶部“标签管理”矢量按钮，主视图无缝切换至内嵌式 `TagManagerView` 大视图；再次点击切回 `m_contentPanel` 内容视图。
