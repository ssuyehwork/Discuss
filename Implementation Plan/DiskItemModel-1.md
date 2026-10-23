# Implementation Plan - DiskItemModel-1.md

## Overview
本实施方案旨在将 `DiskItemModel` 中的文件夹图标替换为应用内置的实心 SVG 图标（`folder_filled`），并根据手动设置的色标进行即时着色（无色标时回退至默认灰色 `#888888`）。普通文件图标渲染逻辑保持 100% 现状不变。同时引入 128px 高清尺寸生成，确保 HiDPI/4K 屏幕及各视图缩放模式下视觉边缘绝对锐利清晰、无任何毛刺与模糊。

## Modified Files List
- `src/ui/models/DiskItemModel.cpp`

## Detailed Line-by-Line Changes

### File: `src/ui/models/DiskItemModel.cpp`

```
<<<<<<< SEARCH
    } else if (role == Qt::DecorationRole && index.column() == 0) {
        QString cacheKey = path;
        QIcon* cached = m_iconCache.object(cacheKey);
        if (cached) return *cached;

        QString ext = record.suffix.toLower();
        bool isGraphic = UiHelper::isGraphicsFile(ext);
        
        if (isGraphic) return QIcon();
        QIcon icon = ShellIconManager::getFileIconFast(path, record.isDir, ext);
        if (ShellIconManager::isIconCached(path, record.isDir, ext)) {
            m_iconCache.insert(cacheKey, new QIcon(icon));
        }
        return icon;
    }
=======
    } else if (role == Qt::DecorationRole && index.column() == 0) {
        if (record.isDir) {
            QColor folderColor("#888888");
            if (!record.manualColor.isEmpty()) {
                QColor parsed = UiHelper::parseColorName(record.manualColor);
                if (parsed.isValid()) folderColor = parsed;
            }
            return UiHelper::getIcon("folder_filled", folderColor, 128);
        }

        QString cacheKey = path;
        QIcon* cached = m_iconCache.object(cacheKey);
        if (cached) return *cached;

        QString ext = record.suffix.toLower();
        bool isGraphic = UiHelper::isGraphicsFile(ext);
        
        if (isGraphic) return QIcon();
        QIcon icon = ShellIconManager::getFileIconFast(path, record.isDir, ext);
        if (ShellIconManager::isIconCached(path, record.isDir, ext)) {
            m_iconCache.insert(cacheKey, new QIcon(icon));
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

# 2. 交互功能与高清度验证：
# - 启动应用进入任意包含文件夹的视图（网格、列表、分栏、树状）；
# - 校验所有文件夹是否统一展现为精美实心 SVG 图标；
# - 为文件夹设置红、黄、蓝等各种色标，校验图标颜色是否秒级精准响应着色；
# - 清除色标后，校验图标是否平滑恢复为中性灰色 (#888888)；
# - 放大网格卡片或在 HiDPI 高分屏下查看，校验图标边缘是否绝对锐利清晰、无模糊；
# - 校验普通文件图标是否保持 100% 既有逻辑不受任何影响。
```

## SSOT API Reuse & Anti-Redundancy Self-Check
- **复用既有 SSOT 通道**：严格复用全软件统一矢量渲染入口 `UiHelper::getIcon` 与色标解析工具 `UiHelper::parseColorName`，禁止另起炉灶手写 SVG 渲染或颜色转换代码。
- **无重复缓存**：文件夹图标直接利用 `SvgIconRenderer` 线程安全的全局 `QPixmap` 缓存，不在 `m_iconCache` 中冗余存放，彻底避免两套缓存混用。

## Header API Signature Verification
| 类名 / 模块名 | 调用的成员/方法 | 物理头文件精确签名 |
| :--- | :--- | :--- |
| `UiHelper` | `parseColorName` | `static inline QColor parseColorName(const QString& colorName)` (`src/ui/UiHelper.h`) |
| `UiHelper` | `getIcon` | `static inline QIcon getIcon(const QString& key, const QColor& color, int size = 18)` (`src/ui/UiHelper.h`) |
| `SvgIconRenderer` | `getIcon` | `static QIcon getIcon(const QString& key, const QColor& color, int size = 18)` (`src/ui/SvgIconRenderer.h`) |
