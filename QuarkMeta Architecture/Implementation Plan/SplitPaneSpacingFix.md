# SplitPaneSpacingFix Implementation Plan

## 1. Overview
推导并解决创建多窗格（双分栏）后，内容面板与地址栏之间的物理间距由 5 像素变成 6 像素（偏移了 1 像素）的数学与 CSS 盒模型本质原因：
- **单窗格模式**：`ContentPanel`（`QFrame`）带有 `border: 1px solid #333333`，其 `frameWidth()` 为 1px，内容顶部偏移为 **1px**。
- **多窗格模式**：`m_panel` 触发 `isHostPanel="true"` 使得其 `border: none`（偏移 0px）；而内部嵌套的 `m_primaryPaneContainer`（也是 `QFrame`）本身带有 `border: 1px solid #333333`（顶边框 1px + `frameWidth()` 1px），导致内部内容顶部偏移累加为了 **2px**（0px + 1px + 1px = 2px）。
- **1px 偏差修解**：在 `resources/style.qss` 中，针对 `isHostPanel="true"` 内部嵌套的 `#EditorContainer` 消除重复的 `border-top`（设为 `border-top: none;`），使多窗格模式下内容顶部的几何偏移严格等于 **1px**（与单窗格完全一致），从而保证内容面板与地址栏之间的视效间距严格统一保持在 **5 像素**！

---

## 2. Modified Files List
1. `resources/style.qss`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Changes to `resources/style.qss`

<<<<<<< SEARCH
#EditorContainer[isHostPanel="true"] {
    border: none;
}
=======
#EditorContainer[isHostPanel="true"] {
    border: none;
}

#EditorContainer[isHostPanel="true"] #EditorContainer {
    border-top: none;
}
>>>>>>> REPLACE

---

## 4. Build & Verification Steps

### 4.1 编译指令
```cmd
cmake --build build --config Release
```

### 4.2 功能验证步骤
1. 启动应用，测算单窗格下地址栏与内容面板标题栏的物理间距（5px）。
2. 点击分栏按钮或拖拽标签开分栏，测算分栏后主窗格及副窗格与地址栏的物理间距，确认其与单窗格完全一致，绝对无 1px 下移，保持统一 5px 间距。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 纯 CSS 盒模型顶边框归一化，无 C++ 逻辑改动。

---

## 6. Header API Signature Verification
- 未修改任何头文件 API 签名。

---

## 7. Header Inclusion Chain & Type Completeness Check
- 未修改头文件包含链。
