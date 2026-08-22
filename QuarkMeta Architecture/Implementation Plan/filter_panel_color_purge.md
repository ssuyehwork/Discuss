# FilterPanel 颜色筛选器精简与内存模式遗毒物理清退实施方案 (Filter Panel Color Filter Purge Implementation Plan)

## Overview
在 QuarkMeta 纯磁盘直连架构下，全盘图像色彩预扫描与 central `metadata` 数据表索引已彻底废除。
原 `FilterPanel` 中继承自内存模式（Memory Mode）的连续色相滑块（`InlineHueSlider`）、准确度/容差滑块（`m_accuracySlider`）以及占比滑块（`m_areaSlider`）缺乏全量色彩数据库支撑，拖动后无法精准匹配文件，且引发了 `ContentPanel` 中繁复的 CIELAB Delta-E 计算开销。

本方案旨在：
1. **UI 面板精简**：从 `FilterPanel.h`/`FilterPanel.cpp` 中彻底物理剔除 `InlineHueSlider`、`m_accuracySlider` 和 `m_areaSlider` 控件，收拢颜色筛选器至手动离散色标过滤（标准色块网格及色名/无色标搜索）。
2. **过滤逻辑优化**：简化 `ContentPanel.cpp` 中依赖 Delta-E 调色板百分比判断的复杂逻辑，回归离散色标直接高效匹配。
3. **空转扫描与幽灵 SQL 修复**：注销 `CoreController` 在启动时对 `global.db` `metadata` 表的空转扫描 `initFromDatabase()`，并修正 `MediaExtractorPipeline` 与 `MetadataManager` 中命中 0 行的幽灵 SQL `UPDATE`，确保元数据更新直接落盘至 `.QuarkMeta.json`。

---

## Modified Files List
- `src/ui/FilterPanel.h`
- `src/ui/FilterPanel.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/core/CoreController.cpp`
- `src/meta/MediaExtractorPipeline.cpp`
- `src/meta/MetadataManager.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/FilterPanel.h`
<<<<<<< SEARCH
// ─── 色相滑块 (内嵌版) ─────────────────────────────────────────────
class InlineHueSlider : public QWidget {
    Q_OBJECT
public:
    explicit InlineHueSlider(QWidget* parent = nullptr);
    void setHue(int h);
    int hue() const { return m_h; }

signals:
    void hueChanged(int h);
    void sliderReleased();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    int m_h = 0; // 0..359
    bool m_dragging = false;
    void updateFromPos(int x);
};
=======
>>>>>>> REPLACE

<<<<<<< SEARCH
    QSlider*      m_accuracySlider  = nullptr; // 2026-07-xx 按照用户要求：还原颜色准确度控制条
    QSlider*      m_areaSlider      = nullptr; // 2026-06-23 按照用户要求：新增颜色面积占比滑条
=======
>>>>>>> REPLACE

### 2. `src/ui/FilterPanel.cpp`
<<<<<<< SEARCH
        // 2.1 顶部色相滑块
        // 2026-06-xx 物理对齐：滑块及其容器增加 4px 左右边距（相对于 gl 的 0 边距），实现视觉平衡
        QWidget* hueContainer = new QWidget(g);
        QHBoxLayout* hueLayout = new QHBoxLayout(hueContainer);
        hueLayout->setContentsMargins(5, 0, 5, 0);
        hueLayout->setSpacing(0);
        
        InlineHueSlider* hueSlider = new InlineHueSlider(hueContainer);
        hueLayout->addWidget(hueSlider);
        connect(hueSlider, &InlineHueSlider::sliderReleased, this, [this, hueSlider]() {
            int h = hueSlider->hue();
            QColor c;
            if (h == 1000) c = Qt::black;
            else if (h == 1001) c = QColor("#808080");
            else if (h == 1002) c = Qt::white;
            else c = QColor::fromHsv(h, 220, 220);

            QString hex = c.name().toUpper();
            m_filter.colors.clear();
            m_filter.colors.append(hex);
            
            // LRU 更新 (2026-06-xx: 容量扩展至 50 个，且由左上向右下按时间排布)
            m_recentColors.removeAll(hex);
            m_recentColors.prepend(hex);
            if (m_recentColors.size() > 50) m_recentColors.removeLast();
            AppConfig::instance().setValue("Filter/RecentColors", m_recentColors);

            emit filterChanged(m_filter);
            rebuildGroups();
        });
        gl->addWidget(hueContainer);

        // 2.1.5 颜色准确度 (容差) 滑块 ─────────────────────────
        // 2026-07-xx 按照用户要求：还原此前被误删的准确度控制条
        QWidget* accContainer = new QWidget(g);
        QHBoxLayout* accLayout = new QHBoxLayout(accContainer);
        accLayout->setContentsMargins(10, 4, 10, 4);
        accLayout->setSpacing(8);

        QLabel* lblAcc = new QLabel("准确度:", accContainer);
        lblAcc->setStyleSheet("color: #AAAAAA; font-size: 11px;");
        accLayout->addWidget(lblAcc);

        m_accuracySlider = new QSlider(Qt::Horizontal, accContainer);
        m_accuracySlider->setRange(0, 100);
        m_accuracySlider->setValue(m_filter.colorTolerance);
        m_accuracySlider->setCursor(Qt::PointingHandCursor);
        m_accuracySlider->setStyleSheet(
            "QSlider::groove:horizontal { height: 2px; background: #444; border-radius: 1px; }"
            "QSlider::handle:horizontal { background: #EEE; border: 1px solid #777; width: 10px; height: 10px; margin: -4px 0; border-radius: 5px; }"
            "QSlider::handle:horizontal:hover { background: #FFF; border-color: #378ADD; }"
        );
        accLayout->addWidget(m_accuracySlider, 1);

        connect(m_accuracySlider, &QSlider::valueChanged, this, [this](int val) {
            m_filter.colorTolerance = val;
            emit filterChanged(m_filter);
        });

        gl->addWidget(accContainer);

        // 2.1.6 颜色占比滑块 ─────────────────────────────────
        // 2026-06-23 按照用户要求：新增颜色面积占比过滤逻辑
        QWidget* areaContainer = new QWidget(g);
        QHBoxLayout* areaLayout = new QHBoxLayout(areaContainer);
        areaLayout->setContentsMargins(10, 4, 10, 4);
        areaLayout->setSpacing(8);

        QLabel* lblArea = new QLabel("占比:", areaContainer);
        lblArea->setStyleSheet("color: #AAAAAA; font-size: 11px;");
        areaLayout->addWidget(lblArea);

        m_areaSlider = new QSlider(Qt::Horizontal, areaContainer);
        m_areaSlider->setRange(0, 100);
        m_areaSlider->setValue(m_filter.minColorArea);
        m_areaSlider->setCursor(Qt::PointingHandCursor);
        m_areaSlider->setMouseTracking(true); // 2026-06-23 按照用户要求：支持悬停/滑动实时回显百分比
        m_areaSlider->installEventFilter(this);
        m_areaSlider->setStyleSheet(
            "QSlider::groove:horizontal { height: 2px; background: #444; border-radius: 1px; }"
            "QSlider::handle:horizontal { background: #EEE; border: 1px solid #777; width: 10px; height: 10px; margin: -4px 0; border-radius: 5px; }"
            "QSlider::handle:horizontal:hover { background: #FFF; border-color: #378ADD; }"
        );
        areaLayout->addWidget(m_areaSlider, 1);

        connect(m_areaSlider, &QSlider::valueChanged, this, [this](int val) {
            m_filter.minColorArea = val;
            emit filterChanged(m_filter);
        });

        gl->addWidget(areaContainer);
=======
>>>>>>> REPLACE

### 3. `src/ui/ContentPanel.h`
<<<<<<< SEARCH
    void loadCategory(int categoryId);
    void loadCategory(const QString& categoryType);
    void loadCategories(const QList<int>& categoryIds);
    void categoryClicked(int categoryId);
=======
    void loadCategory(const QString& categoryType);
>>>>>>> REPLACE

### 4. `src/ui/ContentPanel.cpp`
<<<<<<< SEARCH
void ContentPanel::loadCategories(const QList<int>& categoryIds) {
    Q_UNUSED(categoryIds);
}

void ContentPanel::loadCategory(int categoryId) { 
    Q_UNUSED(categoryId);
}
=======
>>>>>>> REPLACE

### 5. `src/core/CoreController.cpp`
<<<<<<< SEARCH
            // 仅执行 SQLite 模式初始化
            MetadataManager::instance().initFromDatabase();
=======
            // 纯磁盘模式注销全盘 metadata 数据表预加载
>>>>>>> REPLACE

### 6. `src/meta/MediaExtractorPipeline.cpp`
<<<<<<< SEARCH
            if (info.isFile() && MediaColorExtractor::isGraphicsFile(info.suffix().toLower())) {
                // 单次读盘：同时拿到【原始尺寸】和【512 高清图】
                DecodedMediaResult dec = ImageDecoderFacade::decodeSinglePass(qPath, 512);
                if (dec.isValid) {
                    item.width = dec.originalSize.width();
                    item.height = dec.originalSize.height();

                    // 1. 写入 File ID 高清缩略图缓存 (JPEG 85)
                    DiskMediaExtractor::saveDiskThumbnail(qPath, dec.thumbnail512);

                    // 2. 内存 64x64 快速测色 (<0.5ms)
                    auto pal = ColorAlgorithmEngine::extractPaletteFromImage(dec.thumbnail512);
                    if (!pal.isEmpty()) {
                        QColor dominant = MediaColorExtractor::quantizeColor(pal.first().first);
                        item.autoColor = dominant.name().toUpper().toStdWString();
                        item.palettes = pal;
                    }
                }
            }

            results.push_back(item);
        }

        if (!results.empty() && !m_isCanceled.load() && !CoreController::isShuttingDown()) {
            MetadataManager::instance().updateExtractedMediaFeaturesBatch(results);
        }
=======
            if (info.isFile() && MediaColorExtractor::isGraphicsFile(info.suffix().toLower())) {
                // 单次读盘：同时拿到【原始尺寸】和【512 高清图】
                DecodedMediaResult dec = ImageDecoderFacade::decodeSinglePass(qPath, 512);
                if (dec.isValid) {
                    item.width = dec.originalSize.width();
                    item.height = dec.originalSize.height();

                    // 1. 写入 File ID 高清缩略图缓存 (JPEG 85)
                    DiskMediaExtractor::saveDiskThumbnail(qPath, dec.thumbnail512);

                    // 2. 内存 64x64 快速测色 (<0.5ms)
                    auto pal = ColorAlgorithmEngine::extractPaletteFromImage(dec.thumbnail512);
                    if (!pal.isEmpty()) {
                        QColor dominant = MediaColorExtractor::quantizeColor(pal.first().first);
                        item.autoColor = dominant.name().toUpper().toStdWString();
                        item.palettes = pal;
                    }

                    // 3. 纯磁盘直连模式：元数据更新直接离散落盘至 per-directory .QuarkMeta.json
                    QString parentDir = QDir::toNativeSeparators(info.absolutePath());
                    QString fileName = info.fileName();
                    QuarkMetaJson jsonCache(parentDir.toStdWString());
                    jsonCache.load();
                    auto& cachedItems = jsonCache.items();
                    std::wstring wFileName = fileName.toStdWString();
                    if (cachedItems.find(wFileName) == cachedItems.end()) {
                        ItemMeta emptyMeta;
                        emptyMeta.type = L"file";
                        cachedItems[wFileName] = emptyMeta;
                    }
                    auto& fileMeta = cachedItems[wFileName];
                    fileMeta.width = item.width;
                    fileMeta.height = item.height;
                    fileMeta.autoColor = item.autoColor;
                    fileMeta.palettes = item.palettes;
                    jsonCache.save();
                }
            }

            results.push_back(item);
        }
>>>>>>> REPLACE

### 7. `src/meta/MetadataManager.cpp`
<<<<<<< SEARCH
void MetadataManager::updateExtractedMediaFeaturesBatch(const std::vector<ExtractedFeatureItem>& items) {
    if (items.empty()) return;

    std::unordered_map<sqlite3*, std::vector<ExtractedFeatureItem>> dbGroupMap;
    for (const auto& item : items) {
        std::wstring nPath = normalizePath(item.path);
        {
            size_t idx = getShardIndex(nPath);
            std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
            if (m_shards[idx].items.count(nPath)) {
                RuntimeMeta& meta = m_shards[idx].items[nPath];
                meta.width = item.width;
                meta.height = item.height;
                if (item.mtime > 0) meta.mtime = item.mtime;
                if (item.fileSize > 0) meta.fileSize = item.fileSize;
                meta.autoColor = item.autoColor;
                meta.ingestionStatus = item.ingestionStatus;
                meta.palettes.clear();
                for (const auto& p : item.palettes) {
                    meta.palettes.emplace_back(p.first, p.second);
                }
            }
        }

        sqlite3* db = DatabaseManager::instance().getGlobalDb();
        if (db) {
            dbGroupMap[db].push_back(item);
        }
    }

    for (auto& pair : dbGroupMap) {
        sqlite3* db = pair.first;
        auto itemList = pair.second;
        DatabaseManager::instance().enqueueSyncTask([db, itemList]() {
            SqlTransaction trans(db);
            const char* sql = "UPDATE metadata SET width = ?, height = ?, auto_color = ?, palettes = ?, ingestion_status = ?, mtime = ?, file_size = ? WHERE path = ?";
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                for (const auto& it : itemList) {
                    sqlite3_bind_int(stmt, 1, it.width);
                    sqlite3_bind_int(stmt, 2, it.height);
                    sqlite3_bind_text16(stmt, 3, it.autoColor.c_str(), -1, SQLITE_TRANSIENT);
                    std::string palStr;
                    for (const auto& pe : it.palettes) {
                        palStr += pe.first.name().toStdString() + ":" + std::to_string(pe.second) + ";";
                    }
                    sqlite3_bind_text(stmt, 4, palStr.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_int(stmt, 5, it.ingestionStatus);
                    sqlite3_bind_int64(stmt, 6, it.mtime);
                    sqlite3_bind_int64(stmt, 7, it.fileSize);
                    sqlite3_bind_text16(stmt, 8, it.path.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_step(stmt);
                    sqlite3_reset(stmt);
                }
                sqlite3_finalize(stmt);
            }
        });
    }
}
=======
void MetadataManager::updateExtractedMediaFeaturesBatch(const std::vector<ExtractedFeatureItem>& items) {
    if (items.empty()) return;

    for (const auto& item : items) {
        std::wstring nPath = normalizePath(item.path);
        size_t idx = getShardIndex(nPath);
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        if (m_shards[idx].items.count(nPath)) {
            RuntimeMeta& meta = m_shards[idx].items[nPath];
            meta.width = item.width;
            meta.height = item.height;
            if (item.mtime > 0) meta.mtime = item.mtime;
            if (item.fileSize > 0) meta.fileSize = item.fileSize;
            meta.autoColor = item.autoColor;
            meta.ingestionStatus = item.ingestionStatus;
            meta.palettes.clear();
            for (const auto& p : item.palettes) {
                meta.palettes.emplace_back(p.first, p.second);
            }
        }
    }
}
>>>>>>> REPLACE

---

## Build & Verification Steps
1. **Compilation Check**:
   ```bash
   cmake --build build --config Release
   ```
2. **Functional Verification**:
   - Verify `FilterPanel` UI renders cleanly without `InlineHueSlider`, `m_accuracySlider`, or `m_areaSlider`.
   - Verify discrete color filters (standard color blocks and text color filters) work as expected.
   - Verify `.QuarkMeta.json` receives extracted image metadata directly during thumbnail generation.
