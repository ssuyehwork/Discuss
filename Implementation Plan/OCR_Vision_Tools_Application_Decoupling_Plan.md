# Implementation Plan: 独立应用“OCR”屏幕视觉工具集解耦构建方案

本方案严格遵循系统规范、三权分立与统一编译路径准则，将原视觉工具集完整解耦并成立独立桌面应用。

---

## 一、 概述与解耦设计
1. **应用名称与工程定位**：
   - 应用名称：`OCR`
   - 源码路径：`Source_Code/OCR_Source/OCR`
   - 构建输出：`OCR_Build/OCR/OCR.exe`
2. **纳入的功能模块**：
   - 屏幕截图：`ScreenshotTool.cpp / .h`
   - 文字识别：`OCRManager.cpp / .h`、`OCRWindow.cpp / .h`、`OCRResultWindow.cpp / .h`
   - 屏幕拾色器：`ColorPickerWindow.cpp / .h`、`FlowLayout.cpp / .h`
   - 像素标尺：`PixelRulerOverlay.cpp / .h`
   - 基础支持组件：`FramelessDialog`、`ToolTipOverlay`、`IconHelper.h`、`StringUtils.h`、`SvgIcons.h`、`ClipboardMonitor`
3. **数据独立与持久化隔离**：
   - 全面改用 `QSettings("OCR", ...)`，与 RapidNotes 及其他应用 100% 物理隔离，绝不产生数据串扰；
   - 独立 Logo、独立 `.rc` 与独立 `.manifest`。
4. **克制原则**：
   - 仅做代码搬迁与解耦，原汁原味保留既有交互与逻辑实现。

---

## 二、 影响与新建文件清单
- `OCR_Source/OCR/CMakeLists.txt`
- `OCR_Source/OCR/README.md`
- `OCR_Source/OCR/src/main.cpp`
- `OCR_Source/OCR/src/core/OCRManager.h / .cpp`
- `OCR_Source/OCR/src/core/ClipboardMonitor.h / .cpp`
- `OCR_Source/OCR/src/core/DatabaseManager.h` (轻量化隔离适配)
- `OCR_Source/OCR/src/ui/ScreenshotTool.h / .cpp`
- `OCR_Source/OCR/src/ui/OCRWindow.h / .cpp`
- `OCR_Source/OCR/src/ui/OCRResultWindow.h / .cpp`
- `OCR_Source/OCR/src/ui/PixelRulerOverlay.h / .cpp`
- `OCR_Source/OCR/src/ui/ColorPickerWindow.h / .cpp`
- `OCR_Source/OCR/src/ui/FlowLayout.h / .cpp`
- `OCR_Source/OCR/src/ui/FramelessDialog.h / .cpp / Base.h`
- `OCR_Source/OCR/src/ui/ToolTipOverlay.h / .cpp`
- `OCR_Source/OCR/src/ui/IconHelper.h`
- `OCR_Source/OCR/src/ui/StringUtils.h`
- `OCR_Source/OCR/src/ui/SvgIcons.h`
- `OCR_Source/OCR/resources/resources.qrc`
- `OCR_Source/OCR/resources/app_icon.rc`
- `OCR_Source/OCR/resources/app.manifest`
- `OCR_Source/OCR/resources/icons/app_logo.svg`
- `.gitignore` (同步加入 `!OCR_Source/` 允许上传规则)

---

## 三、 验证与合规
- 独立 CMakeLists.txt 包含全部源文件与头文件；
- 独立 QSettings 彻底隔离；
- 无侵入修改原 RapidNotes 代码；
- 完全满足各项工程要求。
