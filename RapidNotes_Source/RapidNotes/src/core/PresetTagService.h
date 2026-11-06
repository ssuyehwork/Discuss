#pragma once

#include <QString>
#include <QStringList>
#include <functional>

namespace RapidNotes {

class PresetTagService {
public:
    static PresetTagService& instance() {
        static PresetTagService s_instance;
        return s_instance;
    }

    QStringList getPresetTagsWithAncestors(int categoryId) {
        Q_UNUSED(categoryId);
        return {};
    }

    bool updateCategoryPresetTags(int categoryId, const QStringList& tags) {
        Q_UNUSED(categoryId);
        Q_UNUSED(tags);
        return true;
    }

    int applyPresetTagsToCategory(int categoryId, const QStringList& oldTags, const QStringList& newTags) {
        Q_UNUSED(categoryId);
        Q_UNUSED(oldTags);
        Q_UNUSED(newTags);
        return 0;
    }

private:
    PresetTagService() = default;
    ~PresetTagService() = default;
};

} // namespace RapidNotes
