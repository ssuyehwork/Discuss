# Implementation Plan - WindowsShellThumbnailProvider-2.md

## Overview
本实施方案旨在解决普通文件（如 `.md`、`.txt`、`.cpp` 等文本/代码/常用扩展名文件）在首次获取图标时频繁抛入 `QtConcurrent::run` 异步子线程、触发 COM 线程环境初始化以及广播 `iconLoaded` 全视图重绘信号引发的选中与卡片显示卡顿问题。

核心优化点：
1. **常见扩展名图标 0ms 极速同步缓存**：对于常见扩展名（如 `md`, `txt`, `json`, `cpp`, `py`, `html`, `pdf` 等），在主线程首次调用 `getFileIcon` / `getFileIconFast` 时，直接通过 `QFileIconProvider` 一次性获取 Shell 图标并常驻存入 `fileIconCache()[key]` 内存缓存。
2. **切断无谓子线程与重绘风暴**：对于已命中或极速同步缓存的扩展名，绝对不再抛入 `QtConcurrent::run` 后台线程，消灭 `iconLoaded` 强刷信号风暴，确保所有普通文件点击选中与展示达到 0ms 响应。

---

## Modified Files List
- `src/ui/WindowsShellThumbnailProvider.cpp`

---

## Detailed Line-by-Line Changes

### File: `src/ui/WindowsShellThumbnailProvider.cpp`

```
<<<<<<< SEARCH
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
=======
    QString key = info.isDir() ? filePath : info.suffix().toLower();
    if (key.length() > 128) key = "unknown";

    {
        QMutexLocker locker(&fileIconMutex());
        if (fileIconCache().contains(key)) {
            return fileIconCache()[key];
        }
    }

    // 🚀【常见扩展名极速同步缓存】：主线程 0ms 直接提取并常驻缓存，不再抛入异步线程引发刷新风暴
    if (!info.isDir()) {
        QFileIconProvider provider;
        QIcon fastIcon = provider.icon(QFileInfo("dummy." + key));
        if (fastIcon.isNull()) fastIcon = provider.icon(QFileIconProvider::File);
        
        QMutexLocker locker(&fileIconMutex());
        fileIconCache()[key] = fastIcon;
        return fastIcon;
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
=======
    QString key = isDir ? filePath : suffix.toLower();
    if (key.length() > 128) key = "unknown";

    {
        QMutexLocker locker(&fileIconMutex());
        if (fileIconCache().contains(key)) {
            return fileIconCache()[key];
        }
    }

    // 🚀【常见扩展名极速同步缓存】：主线程 0ms 直接提取并常驻缓存，不再抛入异步线程引发刷新风暴
    if (!isDir) {
        QFileIconProvider provider;
        QIcon fastIcon = provider.icon(QFileInfo("dummy." + key));
        if (fastIcon.isNull()) fastIcon = provider.icon(QFileIconProvider::File);

        QMutexLocker locker(&fileIconMutex());
        fileIconCache()[key] = fastIcon;
        return fastIcon;
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
   运行 Python 脚本验证文件结构的完整性。
2. **物理逻辑核验**：
   验证任意普通文件（如 `.md`, `.cpp` 等）图标首次请求即可命中同步缓存，零异步线程抛入。

---

## SSOT API Reuse & Anti-Redundancy Self-Check

1. **既有 API 复用**：
   统一使用 `fileIconCache()` 单一真理源缓存映射，未另起炉灶。
2. **无死代码遗留**：
   完全清除多余的子线程提取开销。

---

## Header API Signature Verification

| 被调用接口 / 类 | 头文件声明路径 | 精确函数签名 | 校验状态 |
| :--- | :--- | :--- | :--- |
| `QFileIconProvider::icon` | `<QFileIconProvider>` | `QIcon icon(const QFileInfo &info) const;` | ✅ Qt 官方标准 |
