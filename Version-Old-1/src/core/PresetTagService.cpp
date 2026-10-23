#include "PresetTagService.h"
#include "../meta/LibraryDao.h"
#include "CoreEngine.h"
#include <QSet>

namespace QuarkMeta {

PresetTagService& PresetTagService::instance() {
    static PresetTagService s_instance;
    return s_instance;
}

bool PresetTagService::saveLibraryPresetTags(int categoryId, const QStringList& newTags, const QStringList& initialTags, std::function<void(int tagCount, int itemCount)> onTraceFinished) {
    // 1. 计算相对 initialTags 新增的标签 (忽略大小写比较)
    QSet<QString> initialSetLower;
    for (const QString& tag : initialTags) {
        initialSetLower.insert(tag.toLower());
    }

    QStringList newlyAddedTags;
    for (const QString& tag : newTags) {
        if (!initialSetLower.contains(tag.toLower())) {
            newlyAddedTags.append(tag);
        }
    }

    // 2. 同步调用 LibraryDao::updatePresetTags 写规则
    if (!LibraryDao::updatePresetTags(categoryId, newTags)) {
        return false;
    }

    // 3. 新增标签非空时追溯应用到分类和子分类项目
    if (!newlyAddedTags.isEmpty()) {
        QStringList paths = LibraryDao::getCategoryPaths(categoryId, false);
        paths.removeDuplicates();
        if (!paths.isEmpty()) {
            int newlyAddedCount = newlyAddedTags.size();
            CoreEngine::instance().addTagsToPathsAsync(paths, newlyAddedTags, [onTraceFinished, newlyAddedCount](int pathCount) {
                if (onTraceFinished) {
                    onTraceFinished(newlyAddedCount, pathCount);
                }
            });
        }
    }

    return true;
}

int PresetTagService::applyPresetTagsForDroppedItems(int categoryId, const QStringList& paths) {
    if (paths.isEmpty()) {
        return 0;
    }

    QStringList tags = LibraryDao::getPresetTagsWithAncestors(categoryId);
    if (tags.isEmpty()) {
        return 0;
    }

    CoreEngine::instance().addTagsToPathsAsync(paths, tags);
    return tags.size();
}

} // namespace QuarkMeta
