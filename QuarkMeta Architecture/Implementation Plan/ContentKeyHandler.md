# QuarkMeta Architecture/Implementation Plan/ContentKeyHandler.md

## 1. Overview（概述与解决的问题）

### 1.1 核心问题定位
1. **`Ctrl + V` 与右键菜单粘贴通道分裂（违背 SSOT 契约）**：
   - 右键菜单点击“粘贴”时，调用的是 `ContentPanel::performPaste()`，已准确走 `activePath()`（第二列 `G:\T 图片`），因此**右键菜单粘贴完全正确**；
   - 键盘按下 `Ctrl + V` 时，`ContentKeyHandler.cpp` 违背了《AGENTS.md》第 2.4 条【核心通用行为 SSOT 入口字典】，没有复用 `performPaste()`，而是另起炉灶手写了一套粘贴触发逻辑，在底层依然传导了老旧的 `m_currentPath`（第三列 `J 截图`），导致键盘粘贴被错误地灌回第三列，引发同名文件冲突弹窗；
2. **定焦父列时深层子列残留虚焦高亮**：
   在第二列定焦时，第三列先前被复制的文件（`PixPin_...png`）依然挂着蓝色选中底色，造成视觉焦点与操作目标产生严重混淆。

### 1.2 解决方案
1. **彻底物理收敛至 `performPaste()`**：
   将 `ContentKeyHandler.cpp` 中 `Ctrl + C`、`Ctrl + X`、`Ctrl + V` 的局部手写逻辑彻底物理删除，100% 收敛至调用已验证完全正确的官方 SSOT 入口：`m_panel->performCopy(...)` 和 `m_panel->performPaste()`；
2. **激活列时物理清除右侧深层列的旧选区**：
   在 `ColumnViewWidget.cpp` 中，无论是点击条目（`folderClicked` / `fileClicked`）还是点击空白处（`activatePaneFromBlankClick`），在确立第 $k$ 列为活跃列的同时，调用已有的 `clearOtherSelections(k)` 清空第 $k+1 \dots N$ 列的遗留选区。

---

## 2. Modified Files List（影响文件清单）
1. `src/ui/controllers/ContentKeyHandler.cpp`（收敛 `Ctrl+C/X/V` 快捷键至 `ContentPanel` 官方入口）
2. `src/ui/ColumnViewWidget.cpp`（激活父列时清空右侧子列旧选区，消除视觉双高亮）

---

## 3. Detailed Line-by-Line Changes（原理说明）

### 3.1 `src/ui/controllers/ContentKeyHandler.cpp`
收敛 `Qt::Key_C`、`Qt::Key_X`、`Qt::Key_V` 的按键拦截分支，统一委托至 `m_panel->performCopy()` 和 `m_panel->performPaste()`。

### 3.2 `src/ui/ColumnViewWidget.cpp`
在 `activatePaneFromBlankClick` 以及 `folderClicked` / `fileClicked` 信号回调中增加 `clearOtherSelections(paneIdx)` 显式清空深层子列遗留选区。
