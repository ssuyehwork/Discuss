# QuarkMeta 底层系统服务与 Shell 工具纯化实施方案 (ShellHelper)

## 1. 目标与范围
- 彻底纯化 `ShellHelper`：移除 `moveToTrash` 与 `copyOrMoveItems` 历史越权代码，将删除与文件移动 100% 归还给 `TrashService` 与 `DiskIoService`。
- 升级两阶段安全重命名：`ShellHelper::renameItem` 内部接入 `FileOperationHelper::safeRename`，通过两阶段 UUID 中转彻底根治 Windows NTFS 文件系统大小写不敏感导致的重命名失败缺陷。
- 保留纯正操作系统外壳关联能力：资源管理器定位选中（`openInExplorer`）、属性对话框呼出（`showProperties`）、隐藏属性赋予（`ensureHidden`）及字节格式化（`formatSize`）。

---

## 2. 核心模块独立实现

### 2.1 `src/util/ShellHelper.h`
```cpp
#pragma once

#include <QString>
#include <string>

namespace QuarkMeta {

/**
 * @brief 系统服务层工具类 (ShellHelper)
 * 纯粹负责 Windows 原生 Shell 调用与系统级关联操作，0 冗余 I/O 业务。
 */
class ShellHelper {
public:
    /**
     * @brief 在 Windows 文件资源管理器中高亮定位指定物理路径
     */
    static void openInExplorer(const QString& path);

    /**
     * @brief 呼出 Windows 原生文件属性对话框
     */
    static void showProperties(const QString& path);

    /**
     * @brief 两阶段 UUID 安全重命名 (解决 NTFS 大小写不敏感缺陷并自动漫游元数据)
     */
    static bool renameItem(const QString& oldPath, const QString& newPath);

    /**
     * @brief 格式化字节大小为易读文本 (B / KB / MB / GB)
     */
    static QString formatSize(qint64 bytes);

    /**
     * @brief 物理赋予文件/文件夹 Windows 隐藏属性
     */
    static void ensureHidden(const std::wstring& path);
};

} // namespace QuarkMeta
```

### 2.2 `src/util/ShellHelper.cpp`
```cpp
#include "ShellHelper.h"
#include "DiskMediaExtractor.h"
#include "../meta/MetadataManager.h"
#include "../meta/QuarkMetaJson.h"
#include "../meta/FileOperationHelper.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QProcess>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

namespace QuarkMeta {

void ShellHelper::openInExplorer(const QString& path) {
    if (path.isEmpty() || path == "computer://" || path.contains("://")) return;

#ifdef Q_OS_WIN
    QStringList args;
    args << "/select," << QDir::toNativeSeparators(path);
    QProcess::startDetached("explorer", args);
#else
    Q_UNUSED(path);
#endif
}

void ShellHelper::showProperties(const QString& path) {
    if (path.isEmpty() || path.contains("://")) return;

#ifdef Q_OS_WIN
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_INVOKEIDLIST;
    sei.lpVerb = L"properties";
    std::wstring wpath = QDir::toNativeSeparators(path).toStdWString();
    sei.lpFile = wpath.c_str();
    sei.nShow = SW_SHOW;
    ShellExecuteExW(&sei);
#else
    Q_UNUSED(path);
#endif
}

bool ShellHelper::renameItem(const QString& oldPath, const QString& newPath) {
    if (oldPath.isEmpty() || newPath.isEmpty() || oldPath == newPath) return true;

    // 🚀【核心升级】：使用两阶段 UUID safeRename，彻底解决 Windows 大小写重命名失败
    if (FileOperationHelper::safeRename(oldPath, newPath)) {
        // 1. 物理漫游迁移本地 .QuarkMeta.json 元数据
        QuarkMetaJson::migrateItemMetadata(oldPath, newPath);

        // 2. 物理漫游磁盘 Hash 缩略图缓存
        QString oldThumbHashPath = DiskMediaExtractor::getDiskThumbCachePath(oldPath);
        QString newThumbHashPath = DiskMediaExtractor::getDiskThumbCachePath(newPath);
        if (QFile::exists(oldThumbHashPath)) {
            FileOperationHelper::safeRename(oldThumbHashPath, newThumbHashPath);
        }

        // 3. 同步更新 MetadataManager 内存缓存与 SQLite 索引
        MetadataManager::instance().renameItem(oldPath.toStdWString(), newPath.toStdWString());
        return true;
    }
    return false;
}

QString ShellHelper::formatSize(qint64 bytes) {
    if (bytes < 1024) return QString("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 2);
    if (bytes < 1024LL * 1024 * 1024) return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 2);
    return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
}

void ShellHelper::ensureHidden(const std::wstring& path) {
#ifdef Q_OS_WIN
    SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_HIDDEN);
#else
    Q_UNUSED(path);
#endif
}

} // namespace QuarkMeta
```

---

## 3. 构建配置注册
确保相关源文件在构建系统中已正规注册：
```cmake
set(UTIL_SOURCES
    # ...
    src/util/ShellHelper.h
    src/util/ShellHelper.cpp
    src/util/ImageDecoderFacade.h
    src/util/ImageDecoderFacade.cpp
    src/util/FormatDecoders.h
    src/util/FormatDecoders.cpp
    src/ui/ShellIconManager.h
)
```