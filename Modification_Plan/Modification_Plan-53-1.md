# 托管库级联扫描多维特征提取闭环重构方案 —— Modification_Plan-53.md

> 状态：待批准执行（尚未获得用户“批准执行”指令）

## 1. 任务背景
在将含有大量图形文件的文件夹拷贝入 `ArcMeta.Library_[盘符]` 托管库执行同步时：
1. **并发写入被独占**：因 Windows 异步复制存在时间差，文件监控被触发时，部分图片正被独占写入，后台抽取直接返回失败；
2. **状态强行标记 1 堵死自愈**：流水线在未校验提取结果的情况下，粗暴强行将 `ingestionStatus` 写入为已完成（`1`），导致下次对账时该文件被指纹准入拦截，永远缺失高级属性；
3. **缓存纠偏缺陷导致缩略图瞬间消失**：前台刷新清空了内存中的 `m_aspectRatios` 缓存，在绘制时强行拉起二次同步抽取，如果再次遇阻则空结果直接在 `m_iconCache` 中覆盖掉已绘制好的优质缓存，产生“闪现后闪退消失”的严重视觉抖动缺陷。

为彻底根除历史 AI 的“碎片化临时拼凑打补丁”和“职责过载”通病，本方案提供**百分之百精确、不留任何脑补和发散余地**的代码修改实施方案。

## 2. 问题定位与修改边界
- 元数据对账过滤：`src/meta/MetadataManager.cpp`
- 媒体抽取流水线：`src/meta/MediaExtractorPipeline.cpp`
- 缩略图异步加载回调：`src/ui/ContentPanel.cpp`

---

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 含有图形图像文件夹添加到托管库后未提取出颜色尺寸 | 优化快速准入对账，只有当高级元数据确实提取成功时才允许不入队列，否则自动补算。 (对应用户原话：“只提取了基本元数据？而没有提取颜色尺寸等逻辑？”) | ✅ |
| 2    | 媒体提取在并发被锁占时丢失数据 | 提取失败时拒绝置 1，维持 0 状态并自动移送重试任务，实现提取流百分百闭环。 (对应用户原话：“只提取了基本元数据？而没有提取颜色尺寸等逻辑？”) | ✅ |
| 3    | 缩略图“显示出来之后秒消失” | 修正回调，提取失败时绝不覆写已成功建立的内存 QIcon 缓存，无损退避。 (对应用户原话：“只提取了基本元数据？而没有提取颜色尺寸等逻辑？”) | ✅ |

---

## 4. 详细解决方案与 Git Merge Diffs

### 4.1 引入真正的两阶段注册自愈机制 (`src/meta/MetadataManager.cpp`)
重构逻辑：在 `registerItem` 的指纹比对层中，只有当该项目的状态已经是已完成（`ingestionStatus == 1`），且该项目的高级元数据（宽、高、主导色）在数据库或缓存中确实**已经存有合法值（`width > 0 && height > 0` 且 `color` 非空）**时，才允许返回跳过。否则，即使大小和修改时间没有变化，也将降级退化到待解析状态并塞入抽取队列，实现完美自愈。

```
<<<<<<< SEARCH
    // [Plan-131 方案 C] 物理指纹准入机制
    std::string pFid;
    long long pSize = 0, pMtime = 0;
    if (fetchWinApiMetadataDirect(nPath, pFid, nullptr, &pSize, nullptr, nullptr, &pMtime, nullptr)) {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        auto it = m_cache.find(nPath);
        if (it != m_cache.end()) {
            if (it->second.ingestionStatus == 1 && it->second.fileSize == pSize && it->second.mtime == pMtime) {
                return; // 指纹一致且已完成解析，跳过后续所有流程
            }
        }
    }
=======
    // [Plan-131 方案 C + Plan-53 降级自愈安全防护] 物理指纹与高级特征双重准入机制
    std::string pFid;
    long long pSize = 0, pMtime = 0;
    if (fetchWinApiMetadataDirect(nPath, pFid, nullptr, &pSize, nullptr, nullptr, &pMtime, nullptr)) {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        auto it = m_cache.find(nPath);
        if (it != m_cache.end()) {
            // 只有当文件指纹一致、曾经被置为1，且色彩和尺寸物理属性都确切存在、非残缺时，才允许返回跳过！
            // 这杜绝了历史解析失败时留下空元数据、又因状态为 1 无法再次扫描提取的致命 Bug
            bool metadataValid = true;
            QFileInfo info(QString::fromStdWString(nPath));
            if (info.isFile() && MediaColorExtractor::isGraphicsFile(info.suffix().toLower())) {
                if (it->second.width <= 0 || it->second.height <= 0 || it->second.color.empty()) {
                    metadataValid = false;
                }
            }
            if (it->second.ingestionStatus == 1 && it->second.fileSize == pSize && it->second.mtime == pMtime && metadataValid) {
                return; // 物理指纹及高级多媒体特征完备且未发生改变，安全返回
            }
        }
    }
>>>>>>> REPLACE
```

### 4.2 强化媒体提取流水线的状态安全机制 (`src/meta/MediaExtractorPipeline.cpp`)
重构逻辑：在 `processItemDirect` 提取结束后，必须判断高级属性是否至少提取成功其中一项。如果因 Windows 拷贝锁占等竞态造成色彩和尺寸全部返回失败（`w <= 0 && h <= 0 && !success`），说明本次物理读取流损坏。此时**严禁**将 IngestionStatus 更新为 `1`，而是保持其状态为 `0` 不变，仅将其异步放入重试队列，等待重试自愈成功后方能置为已完成，实现状态流闭环。

```
<<<<<<< SEARCH
void MediaExtractorPipeline::processItemDirect(const std::wstring& path) {
    int w = 0, h = 0;
    extractDimensions(path, w, h);
    if (w > 0 && h > 0) {
        MetadataManager::instance().setItemDimensions(path, w, h);
    }

    std::wstring colorStr;
    QVector<QPair<QColor, float>> palette;
    bool success = extractColor(path, colorStr, palette);
    if (success) {
        MetadataManager::instance().setItemVisualMetadata(path, colorStr, palette, false);
    }

    MetadataManager::instance().updateIngestionStatus(path, 1);
    MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::PathUpdate, QString::fromStdWString(path));

    if (!success) {
        QFileInfo info(QString::fromStdWString(path));
        if (info.isDir() || MediaColorExtractor::isGraphicsFile(info.suffix().toLower())) {
            std::lock_guard<std::mutex> lock(m_retryMutex);
            if (std::find(m_visualRetryQueue.begin(), m_visualRetryQueue.end(), path) == m_visualRetryQueue.end()) {
                m_visualRetryQueue.push_back(path);
                QMetaObject::invokeMethod(m_retryTimer, "start", Qt::QueuedConnection);
            }
        }
    }
}
=======
void MediaExtractorPipeline::processItemDirect(const std::wstring& path) {
    int w = 0, h = 0;
    extractDimensions(path, w, h);
    if (w > 0 && h > 0) {
        MetadataManager::instance().setItemDimensions(path, w, h);
    }

    std::wstring colorStr;
    QVector<QPair<QColor, float>> palette;
    bool success = extractColor(path, colorStr, palette);
    if (success) {
        MetadataManager::instance().setItemVisualMetadata(path, colorStr, palette, false);
    }

    // [Plan-53 状态安全拦截] 校验是否真正完成了至少一项多媒体特征提取。
    // 如果由于文件拷贝中、独占等竞态，导致 w, h 均无效且色彩也提取失败，则保持其 IngestionStatus = 0（待命自愈状态）！
    // 只有当提取成功，或者该文件类型确定非媒体图像时，才允许将其置为 1（已完成）状态
    QFileInfo info(QString::fromStdWString(path));
    bool isGraphics = MediaColorExtractor::isGraphicsFile(info.suffix().toLower());
    bool isExtractedOk = (w > 0 && h > 0) || success;

    if (isExtractedOk || (!isGraphics && !info.isDir())) {
        MetadataManager::instance().updateIngestionStatus(path, 1);
    } else {
        qDebug() << "[Pipeline] [Plan-53] 多媒体文件读取遇阻，保留待处理状态(0)，送入重试链 ->" << QString::fromStdWString(path);
    }

    MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::PathUpdate, QString::fromStdWString(path));

    if (!success) {
        if (info.isDir() || isGraphics) {
            std::lock_guard<std::mutex> lock(m_retryMutex);
            if (std::find(m_visualRetryQueue.begin(), m_visualRetryQueue.end(), path) == m_visualRetryQueue.end()) {
                m_visualRetryQueue.push_back(path);
                QMetaObject::invokeMethod(m_retryTimer, "start", Qt::QueuedConnection);
            }
        }
    }
}
>>>>>>> REPLACE
```

### 4.3 在列表异步重绘中引入缓存无损退避机制 (`src/ui/ContentPanel.cpp`)
重构逻辑：在重新加载缩略图的回调逻辑中，如果由于独占、重绘冲突而未能重新加载成功（`img.isNull()` 且 `hasThumb` 为假，或 `img.isNull()` 且系统无法获得哪怕是系统大图标，或者只是临时的加载受阻），如果内存缓存中已经存在了一套有效的 icon 缓存项，**绝对禁止**使用空白或普通文件图标对其进行覆盖。这确保了在缩放、重置、刷新重合产生竞态时，界面的高清晰缓存永不覆灭消失。

```
<<<<<<< SEARCH
                    if (weakThis) {
                        QMetaObject::invokeMethod(const_cast<FerrexVirtualDbModel*>(weakThis.data()), [weakThis, path, cacheKey, img, ar, hasThumb]() {
                            if (!weakThis) return;
                            auto* mutableThis = const_cast<FerrexVirtualDbModel*>(weakThis.data());

                            QIcon icon;
                            if (!img.isNull()) {
                                icon = QIcon(QPixmap::fromImage(img));
                            } else {
                                icon = UiHelper::getFileIcon(path, 128);
                            }

                            mutableThis->m_iconCache.insert(cacheKey, new QIcon(icon));
                            if (hasThumb) mutableThis->m_aspectRatios[QDir::toNativeSeparators(path)] = ar;

                            for (int i = 0; i < mutableThis->m_displayCount; ++i) {
=======
                    if (weakThis) {
                        QMetaObject::invokeMethod(const_cast<FerrexVirtualDbModel*>(weakThis.data()), [weakThis, path, cacheKey, img, ar, hasThumb]() {
                            if (!weakThis) return;
                            auto* mutableThis = const_cast<FerrexVirtualDbModel*>(weakThis.data());

                            // [Plan-53 内存缓存无损退避机制]
                            // 在刷新或重置导致二次强行提取时，如果由于物理拷贝尚未完成或图片暂时遇阻，
                            // img 返回空图，若此时缓存 m_iconCache 中已经存在了我们之前成功绘制出来的缩略图，
                            // 我们必须无损退退避，绝对禁止用空图或低质默认文件图标将优质的内存 QIcon 缓存覆灭覆盖！
                            if (img.isNull()) {
                                if (mutableThis->m_iconCache.contains(cacheKey)) {
                                    // 缓存已有优质图像，无损保留
                                    return;
                                }
                            }

                            QIcon icon;
                            if (!img.isNull()) {
                                icon = QIcon(QPixmap::fromImage(img));
                            } else {
                                icon = UiHelper::getFileIcon(path, 128);
                            }

                            mutableThis->m_iconCache.insert(cacheKey, new QIcon(icon));
                            if (hasThumb) mutableThis->m_aspectRatios[QDir::toNativeSeparators(path)] = ar;

                            for (int i = 0; i < mutableThis->m_displayCount; ++i) {
>>>>>>> REPLACE
```

---

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [x] 模块/文件：`src/meta/MetadataManager.cpp`（重构物理及高级属性双重准入，残缺项强制退化补救）
- [x] 模块/文件：`src/meta/MediaExtractorPipeline.cpp`（重构状态合拢判定，确保失败文件永远在 0 状态进行异步重试，闭环保障）
- [x] 模块/文件：`src/ui/ContentPanel.cpp`（重构异步加载回调，引入缩略图无损退避逻辑，捍卫内存优质缓存）

**明确禁止越界修改的范围：**
- [ ] 除上述三处以外的其他底层核心类、MFT 磁盘扫描组件和文件系统监控逻辑 —— 不修改

---

## 6. 实现准则与预警【核心】
1. **防止递归死循环**：在进行残缺属性降级补算时，需确保指纹确切变化或提取完成前不会造成无限重复排队。
2. **防性能阻塞**：批量重试必须利用非主线程池（`QtConcurrent::run`）排队，严禁出现前台同步等待。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 缩略图平滑加载规范 | 后台提取期间返回空图标，由 Delegate 绘制灰色圆角矩形 `#3A3A3A` 占位，防止二段式闪烁。本方案重在内存缓存防洗劫，完全兼容。 | ✅ 符合 |

## 8. 待确认事项（可选）
*无。*
