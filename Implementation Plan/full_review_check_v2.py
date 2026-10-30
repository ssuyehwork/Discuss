import os, sys, re

cpp_path = 'src/meta/MetadataManager.cpp'
h_path = 'src/meta/MetadataManager.h'

cpp_content = open(cpp_path, 'r', encoding='utf-8').read()
h_content = open(h_path, 'r', encoding='utf-8').read()

search5 = """void MetadataManager::seedMemoryCacheBatch(const std::vector<ItemRecord>& records) {
    for (const auto& rec : records) {
        std::wstring nPath = normalizePath(rec.path.toStdWString());
        QFileInfo info(rec.path);
        if (info.isRoot() || rec.path.endsWith(":\\\\") || rec.path.endsWith(":/")) {
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

search7 = """void MetadataManager::removeMetadataSync(const std::wstring& path) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        for (size_t i = 0; i < MetaMemoryCache::NUM_SHARDS; ++i) {
            std::unique_lock<std::shared_mutex> shardLock(MetaMemoryCache::instance().shards()[i].mutex);
            for (auto it = MetaMemoryCache::instance().shards()[i].items.begin(); it != MetaMemoryCache::instance().shards()[i].items.end(); ) {
                if (it->first == nPath || it->first.find(nPath + L"\\\\") == 0 || it->first.find(nPath + L"/") == 0) {
                    StatisticsService::instance().purgeAsset(!it->second.tags.isEmpty(), false); 
                    it = MetaMemoryCache::instance().shards()[i].items.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
}"""

print("search5 match:", cpp_content.count(search5))
print("search7 match:", cpp_content.count(search7))

