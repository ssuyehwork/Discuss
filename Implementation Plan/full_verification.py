import re

# Read files
h_mm = open('src/meta/MetadataManager.h', 'r', encoding='utf-8').read()
cpp_mm = open('src/meta/MetadataManager.cpp', 'r', encoding='utf-8').read()
h_ceh = open('src/core/CentralEventHub.h', 'r', encoding='utf-8').read()
h_mmc = open('src/meta/MetaMemoryCache.h', 'r', encoding='utf-8').read()
h_stats = open('src/meta/StatisticsService.h', 'r', encoding='utf-8').read()
h_cpe = open('src/util/ColorPaletteEngine.h', 'r', encoding='utf-8').read()

print("CentralEventHub::publishEvent:", "publishEvent" in h_ceh)
print("MetaMemoryCache::put:", "put(" in h_mmc)
print("MetaMemoryCache::getShardIndex:", "getShardIndex" in h_mmc)
print("MetaMemoryCache::shards:", "shards(" in h_mmc)
print("StatisticsService::purgeAsset:", "purgeAsset" in h_stats)
print("ColorPaletteEngine::isGraphicsFile:", "isGraphicsFile" in h_cpe)

