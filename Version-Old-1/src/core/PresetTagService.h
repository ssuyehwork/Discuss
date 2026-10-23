#pragma once

#include <QString>
#include <QStringList>
#include <functional>

namespace QuarkMeta {

class PresetTagService {
public:
    static PresetTagService& instance();

    bool saveLibraryPresetTags(int categoryId, const QStringList& newTags, const QStringList& initialTags, std::function<void(int tagCount, int itemCount)> onTraceFinished);

    int applyPresetTagsForDroppedItems(int categoryId, const QStringList& paths);

private:
    PresetTagService() = default;
    ~PresetTagService() = default;
};

} // namespace QuarkMeta
