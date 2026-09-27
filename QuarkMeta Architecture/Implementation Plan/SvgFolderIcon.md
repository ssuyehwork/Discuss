# SvgFolderIcon Implementation Plan (文件夹矢量图标统一重构方案)

## Overview
本实施方案旨在将当前主干代码中**所有调用 Windows 系统原生文件夹图标（`QFileIconProvider::Folder`）的逻辑全面弃用**，统一改为使用内置的**矢量 SVG 实心文件夹图标 (`folder_filled`)**（复用 `UiHelper::getIcon("folder_filled", QColor("#888888"), 128)`）。

修改后：
1. `IconCacheManager.cpp` 中当 `isDir == true` 时，直接返回 `UiHelper::getIcon("folder_filled", QColor("#888888"), 128)`，不再使用 `provider.icon(QFileIconProvider::Folder)`。
2. `WindowsShellThumbnailProvider.cpp` 中默认文件夹占位图标 `s_defaultFolderIcon` 及后台加载 `handleIconLoad` 中文件夹图标，统一使用 `UiHelper::getIcon("folder_filled", QColor("#888888"), 128)`。
3. 彻底避免因 Windows 默认黄色文件夹图标渲染带来的视觉不一致与锯齿问题。

---

## Modified Files List
1. `src/ui/IconCacheManager.cpp`
2. `src/ui/WindowsShellThumbnailProvider.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/IconCacheManager.cpp`

<<<<<<< SEARCH
#include "IconCacheManager.h"
#include <QFileIconProvider>
#include <QFileInfo>

namespace QuarkMeta {

IconCacheManager& IconCacheManager::instance() {
    static IconCacheManager inst;
    return inst;
}

IconCacheManager::IconCacheManager(QObject* parent)
    : QObject(parent) {
}

QIcon IconCacheManager::getCachedIcon(const QString& ext, bool isDir) {
    QString key = isDir ? "folder" : ext.toLower();
    {
        QReadLocker lock(&m_cacheLock);
        auto it = m_iconCache.find(key);
        if (it != m_iconCache.end()) return *it;
    }

    QFileIconProvider provider;
    QIcon icon;
    if (isDir) {
        icon = provider.icon(QFileIconProvider::Folder);
    } else {
        if (key.length() > 12) key = "unknown";
        icon = provider.icon(QFileInfo("dummy." + key));
        if (icon.isNull()) icon = provider.icon(QFileIconProvider::File);
    }

    {
        QWriteLocker lock(&m_cacheLock);
        m_iconCache[key] = icon;
    }
    return icon;
}

} // namespace QuarkMeta
=======
#include "IconCacheManager.h"
#include "UiHelper.h"
#include <QFileIconProvider>
#include <QFileInfo>

namespace QuarkMeta {

IconCacheManager& IconCacheManager::instance() {
    static IconCacheManager inst;
    return inst;
}

IconCacheManager::IconCacheManager(QObject* parent)
    : QObject(parent) {
}

QIcon IconCacheManager::getCachedIcon(const QString& ext, bool isDir) {
    QString key = isDir ? "folder" : ext.toLower();
    {
        QReadLocker lock(&m_cacheLock);
        auto it = m_iconCache.find(key);
        if (it != m_iconCache.end()) return *it;
    }

    QFileIconProvider provider;
    QIcon icon;
    if (isDir) {
        icon = UiHelper::getIcon("folder_filled", QColor("#888888"), 128);
    } else {
        if (key.length() > 12) key = "unknown";
        icon = provider.icon(QFileInfo("dummy." + key));
        if (icon.isNull()) icon = provider.icon(QFileIconProvider::File);
    }

    {
        QWriteLocker lock(&m_cacheLock);
        m_iconCache[key] = icon;
    }
    return icon;
}

} // namespace QuarkMeta
>>>>>>> REPLACE

---

### 2. `src/ui/WindowsShellThumbnailProvider.cpp`

<<<<<<< SEARCH
    static QIcon s_defaultFileIcon;
    static QIcon s_defaultFolderIcon;
    if (s_defaultFileIcon.isNull() || s_defaultFolderIcon.isNull()) {
        QFileIconProvider provider;
        s_defaultFolderIcon = provider.icon(QFileIconProvider::Folder);
        s_defaultFileIcon = provider.icon(QFileIconProvider::File);
    }
=======
    static QIcon s_defaultFileIcon;
    static QIcon s_defaultFolderIcon;
    if (s_defaultFileIcon.isNull() || s_defaultFolderIcon.isNull()) {
        QFileIconProvider provider;
        s_defaultFolderIcon = UiHelper::getIcon("folder_filled", QColor("#888888"), 128);
        s_defaultFileIcon = provider.icon(QFileIconProvider::File);
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
    static QIcon s_defaultFileIcon;
    static QIcon s_defaultFolderIcon;
    if (s_defaultFileIcon.isNull() || s_defaultFolderIcon.isNull()) {
        QFileIconProvider provider;
        s_defaultFolderIcon = provider.icon(QFileIconProvider::Folder);
        s_defaultFileIcon = provider.icon(QFileIconProvider::File);
    }
=======
    static QIcon s_defaultFileIcon;
    static QIcon s_defaultFolderIcon;
    if (s_defaultFileIcon.isNull() || s_defaultFolderIcon.isNull()) {
        QFileIconProvider provider;
        s_defaultFolderIcon = UiHelper::getIcon("folder_filled", QColor("#888888"), 128);
        s_defaultFileIcon = provider.icon(QFileIconProvider::File);
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
        if (isDir) {
            if (isRoot) {
                icon = provider.icon(info);
            } else {
                icon = provider.icon(QFileIconProvider::Folder);
            }
        } else {
=======
        if (isDir) {
            if (isRoot) {
                icon = provider.icon(info);
            } else {
                icon = UiHelper::getIcon("folder_filled", QColor("#888888"), 128);
            }
        } else {
>>>>>>> REPLACE

---

## Build & Verification Steps
1. 检查修改文件无语法错误。
2. 运行应用，确认普通文件夹均渲染为精致统一的矢量实心 SVG 图标，极佳地提升了界面美观度与矢量跨平台高分屏适配。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- 复用全局唯一 SVG 渲染逻辑 `UiHelper::getIcon("folder_filled", ...)`，全面替代系统自带文件夹图标。

---

## Header API Signature Verification
- `UiHelper::getIcon(const QString&, const QColor&, int)`: 存在且与头文件 100% 一致。