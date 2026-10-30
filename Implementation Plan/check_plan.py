import sys

h_content = open('src/meta/MetadataManager.h', 'r', encoding='utf-8').read()
cpp_content = open('src/meta/MetadataManager.cpp', 'r', encoding='utf-8').read()

search1 = """    QTimer* m_uiSignalTimer = nullptr;
    std::unordered_set<QString> m_pendingUiPaths;

    void persistBatchAsync(const std::vector<std::wstring>& paths);
};"""

search2 = """signals:
    void pendingSyncChanged(bool hasPending);

private slots:
    void triggerUiSignalTimer() {
        if (m_uiSignalTimer && !m_uiSignalTimer->isActive()) {
            m_uiSignalTimer->start();
        }
    }

private:"""

search3 = """MetadataManager::MetadataManager(QObject* parent) : QObject(parent) {
    m_uiSignalTimer = new QTimer(this);
    m_uiSignalTimer->setInterval(200);
    m_uiSignalTimer->setSingleShot(true);

    connect(m_uiSignalTimer, &QTimer::timeout, [this]() {
        std::vector<QString> paths;
        bool hasReloadAll = false;
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            for (const auto& p : m_pendingUiPaths) {
                if (p == "__RELOAD_ALL__") {
                    hasReloadAll = true;
                }
                paths.push_back(p);
            }
            m_pendingUiPaths.clear();
        }

        if (hasReloadAll || paths.size() > 50) {
            Q_UNUSED(hasReloadAll);
        } else {
            for (const auto& p : paths) {
                Q_UNUSED(p);
            }
        }
    });

    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, [this]() {"""

search4 = """void MetadataManager::notifyUI(RefreshLevel level, const QString& path) {
    switch (level) {
        case RefreshLevel::CountsOnly:
            if (m_isInternalOperating) return;
            {
                std::unique_lock<std::shared_mutex> lock(m_mutex);
                m_pendingUiPaths.insert("__RELOAD_COUNT__");
            }
            QMetaObject::invokeMethod(this, "triggerUiSignalTimer", Qt::QueuedConnection);
            break;
        case RefreshLevel::PathUpdate:
            if (!path.isEmpty()) {
                {
                    std::unique_lock<std::shared_mutex> lock(m_mutex);
                    m_pendingUiPaths.insert(path);
                }
                QMetaObject::invokeMethod(this, "triggerUiSignalTimer", Qt::QueuedConnection);
            }
            break;
        case RefreshLevel::FullRebuild:
            notifyFullUIRebuild();
            break;
    }
}

void MetadataManager::notifyFullUIRebuild() {
    if (m_isInternalOperating) return;
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_pendingUiPaths.insert("__RELOAD_ALL__");
    }
    QMetaObject::invokeMethod(this, "triggerUiSignalTimer", Qt::QueuedConnection);
}"""

search5 = """void MetadataManager::seedMemoryCacheBatch(const std::vector<ItemRecord>& records) {
    for (const auto& rec : records) {
        std::wstring nPath = normalizePath(rec.path.toStdWString());
        QFileInfo info(rec.path);
        if (info.isRoot() || rec.path.endsWith(":\\") || rec.path.endsWith(":/")) {
            continue; // 盘符根目录不参与
        }

        ensureActivated(nPath);

        MetaMemoryCache::instance().update(nPath, [&rec](RuntimeMeta& m) {
            m.rating = rec.rating;
            m.manualColor = rec.manualColor.toStdWString();
            m.pinned = rec.pinned;
            m.tags = rec.tags;
            m.note = rec.note.toStdWString();
            m.url = rec.url.toStdWString();
            m.width = rec.width;
            m.height = rec.height;
            m.autoColor = rec.autoColor.toStdWString();
            m.thumbStatus = (rec.thumbnailState == ItemRecord::ThumbnailState::Failed) ? 1 : 0;
            m.added_at = rec.added_at;
            m.palettes.clear();
            for (const auto& pe : rec.palettes) {
                m.palettes.push_back(PaletteEntry(pe.first, pe.second));
            }
        });
    }
}"""

search6 = """    for (const auto& item : items) {
        std::wstring nPath = normalizePath(item.path);
        
        QFileInfo info(QString::fromStdWString(nPath));
        if (!info.isFile() || !ColorPaletteEngine::isGraphicsFile(info.suffix().toLower())) continue;
        if (item.width <= 0 || item.height <= 0 || item.autoColor.empty() || item.palettes.empty()) continue;"""

search7 = """void MetadataManager::removeMetadataSync(const std::wstring& path) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        for (size_t i = 0; i < MetaMemoryCache::NUM_SHARDS; ++i) {
            std::unique_lock<std::shared_mutex> shardLock(MetaMemoryCache::instance().shards()[i].mutex);
            for (auto it = MetaMemoryCache::instance().shards()[i].items.begin(); it != MetaMemoryCache::instance().shards()[i].items.end(); ) {
                if (it->first == nPath || it->first.find(nPath + L"\\") == 0 || it->first.find(nPath + L"/") == 0) {
                    StatisticsService::instance().purgeAsset(!it->second.tags.isEmpty(), false); 
                    it = MetaMemoryCache::instance().shards()[i].items.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
}"""

blocks = [
    ("SEARCH 1 (h)", search1, h_content),
    ("SEARCH 2 (h)", search2, h_content),
    ("SEARCH 3 (cpp)", search3, cpp_content),
    ("SEARCH 4 (cpp)", search4, cpp_content),
    ("SEARCH 5 (cpp)", search5, cpp_content),
    ("SEARCH 6 (cpp)", search6, cpp_content),
    ("SEARCH 7 (cpp)", search7, cpp_content)
]

for name, s, src in blocks:
    count = src.count(s)
    print(f"{name}: count = {count}")
    if count != 1:
        print("MATCH FAILED!")

