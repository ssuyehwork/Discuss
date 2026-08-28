# ShellHelper 系统服务与 Shell 工具纯化实施方案

## 1. Overview（概述与解决的问题）
本实施方案旨在彻底纯化 `ShellHelper` 工具类：
1. **剥离越权代码**：物理移除 `moveToTrash` 与 `copyOrMoveItems` 历史越权逻辑，将文件删除与传输职责 100% 归还归口领域服务。
2. **两阶段安全重命名**：`ShellHelper::renameItem` 内部接入 `FileOperationHelper::safeRename`，以临时 UUID 做中转，根治 Windows NTFS 文件系统大小写不敏感重命名缺陷，并同步触发 `.QuarkMeta.json` 元数据、磁盘 Hash 缩略图与内存/SQLite 索引的全量漫游。
3. **保留纯正外壳能力**：专注提供 `openInExplorer`（Explorer 定位高亮）、`showProperties`（呼出属性框）、`ensureHidden`（赋予隐藏属性）与 `formatSize`（字节格式化）。

---

## 2. Modified Files List（影响文件清单）
- `src/util/ShellHelper.h`
- `src/util/ShellHelper.cpp`
- `CMakeLists.txt`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/util/ShellHelper.h`
<<<<<<< SEARCH
class ShellHelper {
public:
    /**
     * @brief 移入回收站
     */
    static bool moveToTrash(const QStringList& paths);

    /**
     * @brief 执行复制或移动
     */
    static bool copyOrMoveItems(const QStringList& sourcePaths, const QString& destDir, bool isMove);

    /**
     * @brief 显示文件属性对话框
     */
    static void showProperties(const QString& path);

    /**
     * @brief 在资源管理器中定位
     */
    static void openInExplorer(const QString& path);

    /**
     * @brief 重命名条目
     */
    static bool renameItem(const QString& oldPath, const QString& newPath);

    /**
     * @brief 格式化字节大小
     */
    static QString formatSize(qint64 bytes);

    /**
     * @brief 物理设置文件/文件夹隐藏属性 (解耦自 DatabaseManager)
     */
    static void ensureHidden(const std::wstring& path);

};
=======
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
>>>>>>> REPLACE

### 3.2 `src/util/ShellHelper.cpp`
<<<<<<< SEARCH
bool ShellHelper::moveToTrash(const QStringList& paths) {
    return DiskTrashService::moveToDiskTrash(paths);
}

bool ShellHelper::copyOrMoveItems(const QStringList& sourcePaths, const QString& destDir, bool isMove) {
#ifdef Q_OS_WIN
    if (sourcePaths.isEmpty() || destDir.isEmpty()) return false;
    
    std::wstring from;
    for (const QString& p : sourcePaths) {
        from += QDir::toNativeSeparators(p).toStdWString() + L'\0';
    }
    from += L'\0';

    std::wstring to = QDir::toNativeSeparators(destDir).toStdWString() + L'\0' + L'\0';

    SHFILEOPSTRUCTW fileOp = { 0 };
    fileOp.wFunc = isMove ? FO_MOVE : FO_COPY;
    fileOp.pFrom = from.c_str();
    fileOp.pTo = to.c_str();
    // 🚨 核心改动：移除 FOF_NOCONFIRMATION，遇到同名冲突由系统弹出确认或允许用户选择保留两者，绝不静默覆写！
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR;
    bool ok = (SHFileOperationW(&fileOp) == 0 && !fileOp.fAnyOperationsAborted);

    if (ok) {
        for (const QString& p : sourcePaths) {
            QFileInfo info(p);
            QString newPath = QDir(destDir).filePath(info.fileName());

            // 🚨 无论 Copy 还是 Move，自动触发整包元数据与缩略图原子漫游！
            QuarkMetaJson::roamItemMetadata(p, newPath, isMove);
            DiskMediaExtractor::roamThumbnailCache(p, newPath, isMove);
        }
    }
    return ok;
#else
    Q_UNUSED(sourcePaths);
    Q_UNUSED(destDir);
    Q_UNUSED(isMove);
    return false;
#endif
}

void ShellHelper::showProperties(const QString& path) {
=======
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
>>>>>>> REPLACE

<<<<<<< SEARCH
bool ShellHelper::renameItem(const QString& oldPath, const QString& newPath) {
    if (QFile::rename(oldPath, newPath)) {
        // 1. 物理漫游迁移 .QuarkMeta.json 元数据 
        QuarkMetaJson::migrateItemMetadata(oldPath, newPath);
        // 同步数据库
        MetadataManager::instance().renameItem(oldPath.toStdWString(), newPath.toStdWString());
        return true;
    }
    return false;
}
=======
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
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）
1. 检查 CMake 配置：确保 `CMakeLists.txt` 中已正规注册 `src/util/ShellHelper.h` 与 `src/util/ShellHelper.cpp`。
2. 静态依赖校验：确认 `moveToTrash` 与 `copyOrMoveItems` 已经被干净清除，`ShellHelper` 零冗余传输写盘逻辑。
3. 动态验证：测试将 `test.jpg` 改名为 `Test.jpg`，验证通过两阶段 UUID safeRename 成功完成改名并完整同步元数据和缩略图。
