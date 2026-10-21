#pragma once

#include "FilterState.h"
#include <QJsonObject>

namespace QuarkMeta {

class FilterStateJson {
public:
    static QJsonObject toJson(const FilterState& state);
    static FilterState fromJson(const QJsonObject& json);
};

} // namespace QuarkMeta
