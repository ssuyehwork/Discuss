# `.arc` 胶囊文件夹磁盘纯只读直通预览无脑实施方案 —— ArcCapsuleReadOnlyPreview

本实施方案旨在实现 QuarkMeta 纯磁盘直连模式下对 `.arc` 胶囊文件夹的**“纯只读直通预览（零提取、零写盘、零缩略图生成）”**。

用户可直接点击进入物理磁盘上的 `.arc` 胶囊文件夹浏览其封装的主资产，系统会自动隐蔽内部辅助缓存文件，且 100% 阻断任何写盘、缩略图生成及解包提取行为。

---

## 修改文件清单

1. `src/core/FileFilterService.h` & `src/core/FileFilterService.cpp`
2. `src/core/DiskScanService.cpp`
3. `src/ui/models/DiskItemModel.cpp`
4. `src/meta/MediaExtractorPipeline.cpp`

---

## 阶段一：调整文件过滤服务（放行 `.arc` 文件夹导航，过滤胶囊内部杂质）

### 1. 修改 `src/core/FileFilterService.cpp`
**修改文件**：`src/core/FileFilterService.cpp`
**修改目的**：允许 `.arc` 文件夹本身在磁盘树与内容面板中被正常列出和导航；当进入 `.arc` 内部时，自动过滤隐藏 `*_thumbnail.png`、`meta.json`、`.QuarkMeta.json` 等内部辅助缓存。

**精准替换 Diff**：
```cpp
<<<<<<< SEARCH
bool FileFilterService::isAuxiliaryFile(const QString& path, bool filterArc) {
    if (path.isEmpty()) return true;

    QFileInfo info(path);
    QString fileName = info.fileName();

    // 1. 过滤内部配置文件与缩略图
    if (fileName.endsWith(".QuarkMeta.json", Qt::CaseInsensitive) ||
        fileName.endsWith("_thumbnail.png", Qt::CaseInsensitive)) {
        return true;
    }

    // 2. 过滤缓存目录与 .arc 系统资产包（使其在目录树遍历中隐形）
    if (fileName.compare(".QuarkMeta", Qt::CaseInsensitive) == 0) {
        return true;
    }

    if (filterArc && fileName.endsWith(".arc", Qt::CaseInsensitive)) {
        return true;
    }

    return false;
}
=======
bool FileFilterService::isAuxiliaryFile(const QString& path, bool filterArc) {
    if (path.isEmpty()) return true;

    QFileInfo info(path);
    QString fileName = info.fileName();

    // 1. 过滤内部配置文件与缩略图
    if (fileName.endsWith(".QuarkMeta.json", Qt::CaseInsensitive) ||
        fileName.endsWith("_thumbnail.png", Qt::CaseInsensitive) ||
        fileName.compare("meta.json", Qt::CaseInsensitive) == 0) {
        return true;
    }

    // 2. 过滤缓存目录
    if (fileName.compare(".QuarkMeta", Qt::CaseInsensitive) == 0) {
        return true;
    }

    // 3. 只有当 filterArc 为 true 且该路径不是文件夹时，才强行过滤 .arc
    // 允许 .arc 作为物理文件夹进行只读导航；若在 .arc 内部，过滤内部辅助文件
    if (filterArc && fileName.endsWith(".arc", Qt::CaseInsensitive) && !info.isDir()) {
        return true;
    }

    return false;
}
>>>>>>> REPLACE
```

---

## 阶段二：阻断后台写盘与缩略图生成流水线（恪守纯只读红线）

### 2. 修改 `src/ui/models/DiskItemModel.cpp`
**修改文件**：`src/ui/models/DiskItemModel.cpp`
**修改目的**：在加载缩略图任务时，判定若文件处于 `.arc` 胶囊文件夹内部，强制仅使用 `getCapsuleThumbnailReadOnly` 只读提取，绝不压入后台生成写盘队列。

**精准替换 Diff**：
```cpp
<<<<<<< SEARCH
        const auto& rec = m_allRecords[r];
        if (rec.isDir || !UiHelper::isGraphicsFile(rec.suffix)) continue;

        QString path = rec.path;
        if (m_iconCache.contains(path) || m_requestedPaths.contains(path)) continue;

        m_requestedPaths.insert(path);
        pathsToLoad << path;
=======
        const auto& rec = m_allRecords[r];
        if (rec.isDir || !UiHelper::isGraphicsFile(rec.suffix)) continue;

        QString path = rec.path;
        if (m_iconCache.contains(path) || m_requestedPaths.contains(path)) continue;

        // 🚨 胶囊纯只读保护：若处于 .arc 内部，直接使用只读方法，不提交写盘队列
        if (path.contains(".arc", Qt::CaseInsensitive)) {
            QImage readOnlyThumb = CapsuleMediaExtractor::getCapsuleThumbnailReadOnly(path);
            if (!readOnlyThumb.isNull()) {
                m_iconCache.insert(path, new QIcon(QPixmap::fromImage(readOnlyThumb)));
                emit dataChanged(index(r, 0), index(r, 0), {Qt::DecorationRole, HasThumbnailRole});
            }
            continue;
        }

        m_requestedPaths.insert(path);
        pathsToLoad << path;
>>>>>>> REPLACE
```

---

## 阶段三：媒体提取流水线只读拦截

### 3. 修改 `src/meta/MediaExtractorPipeline.cpp`
**修改文件**：`src/meta/MediaExtractorPipeline.cpp`
**修改目的**：在媒体提取处理入口增加防护阻断，若路径包含 `.arc` 容器路径，阻断磁盘写屏，保证 `.arc` 物理目录的绝对只读和零更改。

**精准替换 Diff**：
```cpp
<<<<<<< SEARCH
                    CapsuleMediaExtractor::saveDiskThumbnail(qPath, dec.thumbnail512);
=======
                    // 🚨 胶囊纯只读保护：位于 .arc 内部的文件禁止写入磁盘缩略图
                    if (!qPath.contains(".arc", Qt::CaseInsensitive)) {
                        CapsuleMediaExtractor::saveDiskThumbnail(qPath, dec.thumbnail512);
                    }
>>>>>>> REPLACE
```

---

## 验证与测试步骤

1. **磁盘导航测试**：
   在左侧磁盘目录树和内容面板中点击进入 `.arc` 胶囊文件夹，确认可正常展开并浏览。
2. **只读性与干净度验证**：
   确认 `.arc` 内部仅显示主资产文件，`meta.json` 及 `thumb_*.png` 辅助文件被完全自动隐藏。
3. **零写盘验证**：
   预览 `.arc` 内部图片后，检查该 `.arc` 文件夹修改时间与内部文件，确认未生成任何新缩略图或配置文件，物理磁盘 100% 保持只读状态。
