# Implementation Plan - DiskItemModel-1.md

## Overview
本方案旨在为 `DiskItemModel` 中的文件夹图标增加自绘 SVG 渲染与手动色标着色逻辑，彻底摆脱系统 Shell 图标依赖，提高渲染效率并确保 UI 主题统一。

## Modified Files List
- `QuarkMeta Architecture/Implementation Plan/DiskItemModel-1.md`

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
            return UiHelper::getIcon("folder_filled", folderColor);
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
1. 方案存放在 `QuarkMeta Architecture/Implementation Plan/DiskItemModel-1.md`；
2. 物理核查头文件与 API 准确性。

## SSOT API Reuse & Anti-Redundancy Self-Check
- 方案完全复用 `UiHelper::getIcon` 与 `UiHelper::parseColorName`，无重新造轮子行为。

## Header API Signature Verification
- `UiHelper::getIcon(const QString&, const QColor&, int)`
- `UiHelper::parseColorName(const QString&)`
