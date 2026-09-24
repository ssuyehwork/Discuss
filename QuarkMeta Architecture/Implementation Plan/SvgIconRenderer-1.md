# Implementation Plan - SvgIconRenderer-1.md

## Overview
本实施方案旨在增强 `SvgIconRenderer::getIcon` 的多分辨率多 Pixmap 预渲染能力，为 `QIcon` 自动装载 1x, 2x, 4x 及 128px, 256px 多阶高清位图帧。确保全应用所有基于 `UiHelper::getIcon` / `SvgIconRenderer::getIcon` 渲染的矢量图标，在 4K/HiDPI 屏幕以及大幅度网格视图拉伸下，均具备 100% 锐利高清晰度，彻底杜绝采样拉伸导致的失真与模糊。

## Modified Files List
- `src/ui/SvgIconRenderer.cpp`

## Detailed Line-by-Line Changes

### File: `src/ui/SvgIconRenderer.cpp`

```
<<<<<<< SEARCH
QIcon SvgIconRenderer::getIcon(const QString& key, const QColor& color, int size) {
    QIcon icon;
    QPixmap pix = getPixmap(key, QSize(size, size), color);
    if (!pix.isNull()) icon.addPixmap(pix);
    return icon;
}
=======
QIcon SvgIconRenderer::getIcon(const QString& key, const QColor& color, int size) {
    QIcon icon;
    const int baseSz = (size > 0) ? size : 18;
    const int sizes[] = { baseSz, baseSz * 2, baseSz * 4, 128, 256 };
    for (int s : sizes) {
        QPixmap pix = getPixmap(key, QSize(s, s), color);
        if (!pix.isNull()) icon.addPixmap(pix);
    }
    return icon;
}
>>>>>>> REPLACE
```

## Build & Verification Steps
```bash
# 1. 配置并编译 CMake 项目
cmake -B build -S .
cmake --build build --config Release

# 2. 验证：
# - 检查各种尺寸下发出的 QIcon 是否包含多阶高清帧；
# - 校验高 DPI 缩放场景下的渲染清晰度。
```

## SSOT API Reuse & Anti-Redundancy Self-Check
- 方案完美增强既有 `SvgIconRenderer::getIcon` 全局入口，无任何冗余代码。

## Header API Signature Verification
| 类名 / 模块名 | 调用的成员/方法 | 物理头文件精确签名 |
| :--- | :--- | :--- |
| `SvgIconRenderer` | `getIcon` | `static QIcon getIcon(const QString& key, const QColor& color, int size = 18)` (`src/ui/SvgIconRenderer.h`) |
| `SvgIconRenderer` | `getPixmap` | `static QPixmap getPixmap(const QString& key, const QSize& size, const QColor& color)` (`src/ui/SvgIconRenderer.h`) |
