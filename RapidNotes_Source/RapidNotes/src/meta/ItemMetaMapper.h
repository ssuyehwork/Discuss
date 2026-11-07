#pragma once

#include "../core/ItemRecord.h"
#include "RapidNotesJson.h"

namespace RapidNotes {

/**
 * @brief 集中管理 ItemMeta 至 ItemRecord 的 SSOT 映射转换类
 */
class ItemMetaMapper {
public:
    static void apply(const ItemMeta& meta, ItemRecord& record);
};

} // namespace RapidNotes
