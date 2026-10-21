#pragma once

#include "../core/ItemRecord.h"
#include "SmartFolderDao.h"

namespace QuarkMeta {

class SmartFolderMatcher {
public:
    static bool matches(const ItemRecord& record, const SmartFolderRecord& sfRec);
};

} // namespace QuarkMeta
