# Implementation Plan - WindowsShellThumbnailProvider-1.md

## Overview
本实施方案旨在解决上一次修改中由于为普通文件夹频繁触发 `QtConcurrent::run` 后台线程、锁竞争以及 `iconLoaded` 信号广播导致的点击选中卡顿性能回归。

核心优化点：
1. **0ms 秒级内存响应**：普通文件夹（`isDir && !isRoot`）图标统一由 `SvgIconRenderer::getIcon("folder_filled", QColor("#888888"), 128)` 静态渲染，并在首次调用时直接写入 `fileIconCache()["folder"]` 缓存。
2. **彻底断开异步线程与强刷信号**：普通文件夹不再发射 `requestIconLoad` 信号，也不进入 `QtConcurrent::run` 异步线程，彻底消灭全图卡片刷新风暴与锁竞争，实现 0ms 同步秒级响应。

---

## Modified Files List
- `src/ui/WindowsShellThumbnailProvider.cpp`

---

## Detailed Line-by-Line Changes

### File: `src/ui/WindowsShellThumbnailProvider.cpp`

```
<<<<<<< SEARCH
QIcon WindowsShellThumbnailProvider::getFileIcon(const QString& filePath, int size) {
    Q_UNUSED(size);
    QFileInfo info(filePath);

    QString key = info.isDir() ? (info.isRoot() ? filePath : "folder") : info.suffix().toLower();
    if (key.length() > 128) key = "unknown";

    {
        QMutexLocker locker(&fileIconMutex());
        if (fileIconCache().contains(key)) {
            return fileIconCache()[key];
        }
    }

    static QIcon s_defaultFileIcon;
    static QIcon s_defaultFolderIcon;
    if (s_defaultFileIcon.isNull() || s_defaultFolderIcon.isNull()) {
        QFileIconProvider provider;
        s_defaultFolderIcon = SvgIconRenderer::getIcon("folder_filled", QColor("#888888"), 128);
        s_defaultFileIcon = provider.icon(QFileIconProvider::File);
    }
    QIcon placeholderIcon = info.isDir() ? s_defaultFolderIcon : s_defaultFileIcon;
=======
QIcon WindowsShellThumbnailProvider::getFileIcon(const QString& filePath, int size) {
    Q_UNUSED(size);
    QFileInfo info(filePath);

    if (info.isDir() && !info.isRoot()) {
        static QIcon s_folderIcon;
        if (s_folderIcon.isNull()) {
            s_folderIcon = SvgIconRenderer::getIcon("folder_filled", QColor("#888888"), 128);
        }
        {
            QMutexLocker locker(&fileIconMutex());
            fileIconCache()["folder"] = s_folderIcon;
        }
        return s_folderIcon;
    }

    QString key = info.isDir() ? filePath : info.suffix().toLower();
    if (key.length() > 128) key = "unknown";

    {
        QMutexLocker locker(&fileIconMutex());
        if (fileIconCache().contains(key)) {
            return fileIconCache()[key];
        }
    }

    static QIcon s_defaultFileIcon;
    static QIcon s_defaultFolderIcon;
    if (s_defaultFileIcon.isNull() || s_defaultFolderIcon.isNull()) {
        QFileIconProvider provider;
        s_defaultFolderIcon = SvgIconRenderer::getIcon("folder_filled", QColor("#888888"), 128);
        s_defaultFileIcon = provider.icon(QFileIconProvider::File);
    }
    QIcon placeholderIcon = info.isDir() ? s_defaultFolderIcon : s_defaultFileIcon;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
QIcon WindowsShellThumbnailProvider::getFileIconFast(const QString& filePath, bool isDir, const QString& suffix) {
    bool isRoot = isDir && (filePath.endsWith(":\\") || filePath.endsWith(":/") || filePath.length() <= 3);
    QString key = isDir ? (isRoot ? filePath : "folder") : suffix.toLower();
    if (key.length() > 128) key = "unknown";

    {
        QMutexLocker locker(&fileIconMutex());
        if (fileIconCache().contains(key)) {
            return fileIconCache()[key];
        }
    }

    static QIcon s_defaultFileIcon;
    static QIcon s_defaultFolderIcon;
    if (s_defaultFileIcon.isNull() || s_defaultFolderIcon.isNull()) {
        QFileIconProvider provider;
        s_defaultFolderIcon = SvgIconRenderer::getIcon("folder_filled", QColor("#888888"), 128);
        s_defaultFileIcon = provider.icon(QFileIconProvider::File);
    }
    QIcon placeholderIcon = isDir ? s_defaultFolderIcon : s_defaultFileIcon;
=======
QIcon WindowsShellThumbnailProvider::getFileIconFast(const QString& filePath, bool isDir, const QString& suffix) {
    bool isRoot = isDir && (filePath.endsWith(":\\") || filePath.endsWith(":/") || filePath.length() <= 3);

    if (isDir && !isRoot) {
        static QIcon s_folderIcon;
        if (s_folderIcon.isNull()) {
            s_folderIcon = SvgIconRenderer::getIcon("folder_filled", QColor("#888888"), 128);
        }
        {
            QMutexLocker locker(&fileIconMutex());
            fileIconCache()["folder"] = s_folderIcon;
        }
        return s_folderIcon;
    }

    QString key = isDir ? filePath : suffix.toLower();
    if (key.length() > 128) key = "unknown";

    {
        QMutexLocker locker(&fileIconMutex());
        if (fileIconCache().contains(key)) {
            return fileIconCache()[key];
        }
    }

    static QIcon s_defaultFileIcon;
    static QIcon s_defaultFolderIcon;
    if (s_defaultFileIcon.isNull() || s_defaultFolderIcon.isNull()) {
        QFileIconProvider provider;
        s_defaultFolderIcon = SvgIconRenderer::getIcon("folder_filled", QColor("#888888"), 128);
        s_defaultFileIcon = provider.icon(QFileIconProvider::File);
    }
    QIcon placeholderIcon = isDir ? s_defaultFolderIcon : s_defaultFileIcon;
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **结构规范核验**：
   运行 Python 工具进行结构完整性核验。
2. **逻辑物理核验**：
   确认普通文件夹图标直接命中 `s_folderIcon` 内存缓存，不再发射 `requestIconLoad`，消除异步线程与强刷信号。

---

## SSOT API Reuse & Anti-Redundancy Self-Check

1. **既有 API 复用**：
   复用 `SvgIconRenderer::getIcon("folder_filled", QColor("#888888"), 128)` 官方 API，零冗余。
2. **无死代码遗留**：
   彻底闭环普通文件夹图标在主线程的 0ms 同步获取，不遗留冗余子线程调用。

---

## Header API Signature Verification

| 被调用接口 / 类 | 头文件声明路径 | 精确函数签名 | 校验状态 |
| :--- | :--- | :--- | :--- |
| `SvgIconRenderer::getIcon` | `src/ui/SvgIconRenderer.h` | `static QIcon getIcon(const QString& key, const QColor& color, int size = 18);` | ✅ 物理对齐 |
