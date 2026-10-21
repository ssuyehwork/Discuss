#pragma once

#include "ItemRecord.h"
#include "FilterState.h"
#include <QSet>
#include <QString>

namespace QuarkMeta {

class FilterMatcher {
public:
    static bool matches(const ItemRecord& record, const FilterState& filter, const QSet<QString>& duplicatePaths = {});
};

} // namespace QuarkMeta
