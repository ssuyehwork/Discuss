import os, sys, re

cpp_path = 'src/meta/MetadataManager.cpp'
h_path = 'src/meta/MetadataManager.h'

cpp_content = open(cpp_path, 'r', encoding='utf-8').read()
h_content = open(h_path, 'r', encoding='utf-8').read()

# 2a. Search block checks
search_blocks = [
    ("3.1 SEARCH 1 (MetadataManager.h)", """    QTimer* m_uiSignalTimer = nullptr;
    std::unordered_set<QString> m_pendingUiPaths;

    void persistBatchAsync(const std::vector<std::wstring>& paths);
};""", h_content),
    ("3.1 SEARCH 2 (MetadataManager.h)", """signals:
    void pendingSyncChanged(bool hasPending);

private slots:
    void triggerUiSignalTimer() {
        if (m_uiSignalTimer && !m_uiSignalTimer->isActive()) {
            m_uiSignalTimer->start();
        }
    }

private:""", h_content),
    ("3.2(1) SEARCH 3 (MetadataManager.cpp)", """MetadataManager::MetadataManager(QObject* parent) : QObject(parent) {
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

    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, [this]() {""", cpp_content),
    ("3.2(2) SEARCH 4 (MetadataManager.cpp)", """void MetadataManager::notifyUI(RefreshLevel level, const QString& path) {
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
}""", cpp_content),
    ("3.2(3) SEARCH 5 (MetadataManager.cpp)", """void MetadataManager::seedMemoryCacheBatch(const std::vector<ItemRecord>& records) {
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
}""", cpp_content),
    ("3.2(4) SEARCH 6 (MetadataManager.cpp)", """    for (const auto& item : items) {
        std::wstring nPath = normalizePath(item.path);

        QFileInfo info(QString::fromStdWString(nPath));
        if (!info.isFile() || !ColorPaletteEngine::isGraphicsFile(info.suffix().toLower())) continue;
        if (item.width <= 0 || item.height <= 0 || item.autoColor.empty() || item.palettes.empty()) continue;""", cpp_content),
    ("3.2(5) SEARCH 7 (MetadataManager.cpp)", """void MetadataManager::removeMetadataSync(const std::wstring& path) {
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
}""", cpp_content)
]

print("=== 检查 2.a: SEARCH 块真值与唯一性 ===")
for title, search_str, source in search_blocks:
    cnt = source.count(search_str)
    print(f"[{title}]: 匹配次数 = {cnt}")
