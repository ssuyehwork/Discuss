# OCR - 屏幕视觉工具集

## 简介
`OCR` 是从 RapidNotes 中完全独立解耦出的屏幕视觉桌面工具集，集成了以下四大核心功能：
1. **屏幕截图 (ScreenshotTool)**：多屏捕获、框选微调、矩形/椭圆/画笔/箭头/文字涂鸦标记、模糊马赛克、撤销重做、保存与复制；
2. **文字识别 (OCRManager / OCRWindow / OCRResultWindow)**：基于 Windows 运行时 (WinRT) OCR 原生内核，支持单图及批量拖入识别、即时复制排版文本；
3. **屏幕拾色器 (ColorPickerWindow)**：高倍放大镜微调、多格式色码 (HEX / RGB / HSL / HSV) 一键复制、历史色板与调色板；
4. **像素标尺 (PixelRulerOverlay)**：横向与纵向屏幕像素精准测量、高 DPI 适配。

## 工程规范
- **代码目录**：`OCR_Source/OCR/`
- **生成输出目录**：`OCR_Build/OCR/`（严格遵循“三权分立”与“T 统一编译路径”闭环）
- **持久化配置**：独立使用 `QSettings("OCR", ...)`，与任何其他应用完全隔离，无任何数据串扰。
