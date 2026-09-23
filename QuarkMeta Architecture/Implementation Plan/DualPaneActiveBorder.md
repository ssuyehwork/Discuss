# DualPaneActiveBorder.md

## 1. Overview
本方案针对双窗格（Split Pane）在激活时选用的高亮边框颜色进行微调。原高亮边框选用了 `#ff551c`（鲜红/亮橙色），在暗色主题背景下对比度过高且显得刺眼，破坏了深色 UI 的整体平滑度与沉浸感。
根据共识，现将 `resources/style.qss` 中激活窗格（`activePane="true"`）的边框颜色替换为相近且微亮一档的相近暗灰色 `#555555`，使当前激活窗格的视觉提示既低调清晰，又保持自然沉静。

## 2. Modified Files List
- `resources/style.qss`

## 3. Detailed Line-by-Line Changes

### `resources/style.qss`
```
<<<<<<< SEARCH
#EditorContainer[activePane="true"], ContentPanel[activePane="true"] {
    border: 1px solid #ff551c;
}
=======
#EditorContainer[activePane="true"], ContentPanel[activePane="true"] {
    border: 1px solid #555555;
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. 在实际 Qt 运行环境中启动 QuarkMeta 应用。
2. 开启双窗格分屏模式。
3. 切换鼠标点击左侧/右侧窗格，观察激活窗格外边框：
   - 非激活窗格边框为默认 `#333333`；
   - 激活窗格边框为相近暗灰色 `#555555`，界面视觉风格和谐柔和。

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 方案仅微调 QSS 属性选择器对应的边框颜色，未新增任何 C++ 硬编码样式，符合 `Memories.md` 样式集中管理规范。

## 6. Header API Signature Verification
- 本方案不涉及 C++ 头文件或 API 签名的修改。
