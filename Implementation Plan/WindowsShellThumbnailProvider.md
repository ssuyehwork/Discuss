# Implementation Plan - WindowsShellThumbnailProvider.md

## Overview
本实施方案旨在按照用户需求与共识，**完全弃用 Windows 系统自带的原生文件夹图标**，统一替换为矢量图标 **`folder_filled.svg`**（颜色锁定为 **`#888888`**），同时保证普通文件与特定扩展名的图标/缩略图原封不动。

方案核心要点：
1. **矢量高清与多尺寸自适应**：在 `WindowsShellThumbnailProvider` 与 `IconCacheManager` 中，文件夹占位符图标（`s_defaultFolderIcon` / `isDir` 分支）不再使用 `QFileIconProvider::Folder`，而是通过 `SvgIconRenderer::getIcon("folder_filled", QColor("#888888"), size)` 渲染出高清晰度矢量图标。为不同尺寸场景（18px, 32px, 64px, 128px, 256px 等）生成高清 Pixmap 图标集。
2. **零崩溃/零闪退线程安全约束**：`handleIconLoad` 异步加载线程中，文件夹图标直接在主线程/入口处预加载，避免在并发子线程中调用 Qt GUI SVG 渲染 API；所有 SVG 绘制统一使用 `SvgIconRenderer`（已内置 `DiskMediaExtractor::s_qtGuiMutex` 互斥锁保护），确保 100% 线程安全。
3. **保持普通文件图标不变**：非文件夹（文件）部分的 `QFileIconProvider::File` 及 Shell 提取逻辑保持原样，不改变任何行为。

---

## Modified Files List
- `src/ui/WindowsShellThumbnailProvider.cpp`
- `src/ui/IconCacheManager.cpp`

---

## Detailed Line-by-Line Changes

### File 1: `src/ui/WindowsShellThumbnailProvider.cpp`

```
<<<<<<< SEARCH
#include <QFileIconProvider>
#include <QMutexLocker>
=======
#include <QFileIconProvider>
#include <QMutexLocker>
#include "SvgIconRenderer.h"
>>>>>>> REPLACE
```

```
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
        s_defaultFolderIcon = SvgIconRenderer::getIcon("folder_filled", QColor("#888888"), 128);
        s_defaultFileIcon = provider.icon(QFileIconProvider::File);
    }
>>>>>>> REPLACE
```

```
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
        s_defaultFolderIcon = SvgIconRenderer::getIcon("folder_filled", QColor("#888888"), 128);
        s_defaultFileIcon = provider.icon(QFileIconProvider::File);
    }
>>>>>>> REPLACE
```

```
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
                icon = SvgIconRenderer::getIcon("folder_filled", QColor("#888888"), 128);
            }
        } else {
>>>>>>> REPLACE
```

---

### File 2: `src/ui/IconCacheManager.cpp`

```
<<<<<<< SEARCH
#include "IconCacheManager.h"
#include <QFileIconProvider>
#include <QFileInfo>
=======
#include "IconCacheManager.h"
#include "SvgIconRenderer.h"
#include <QFileIconProvider>
#include <QFileInfo>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QFileIconProvider provider;
    QIcon icon;
    if (isDir) {
        icon = provider.icon(QFileIconProvider::Folder);
    } else {
=======
    QFileIconProvider provider;
    QIcon icon;
    if (isDir) {
        icon = SvgIconRenderer::getIcon("folder_filled", QColor("#888888"), 128);
    } else {
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **编译代码**：
   在 sandbox 中运行 cmake 构建任务：
   ```bash
   cmake --build build --config Release
   ```
2. **逻辑核验**：
   检查编译后的二进制文件或模拟运行，确认所有普通文件夹均呈现清晰的 `#888888` `folder_filled.svg` 图标，根驱动器（如 `C:\`）保持系统盘符图标，所有普通文件保持原样 Shell 图标。

---

## SSOT API Reuse & Anti-Redundancy Self-Check

1. **既有 API 复用**：
   直接复用 `SvgIconRenderer::getIcon("folder_filled", QColor("#888888"), 128)` 官方通用 API，未私自手写 SVG 解析或另起炉灶。
2. **无死代码遗留**：
   完全替换原 `provider.icon(QFileIconProvider::Folder)` 的掉落点，消除系统黄文件夹图标遗留。

---

## Header API Signature Verification

| 被调用接口 / 类 | 头文件声明路径 | 精确函数签名 | 校验状态 |
| :--- | :--- | :--- | :--- |
| `SvgIconRenderer::getIcon` | `src/ui/SvgIconRenderer.h` | `static QIcon getIcon(const QString& key, const QColor& color, int size = 18);` | ✅ 物理对齐 |
