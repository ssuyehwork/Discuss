#include "DiskNavigatorService.h"

namespace RapidNotes {

DiskNavigatorService& DiskNavigatorService::instance() {
    static DiskNavigatorService inst;
    return inst;
}

std::unordered_map<std::wstring, ItemMeta> DiskNavigatorService::loadDirectoryItems(const std::wstring& /*folderPath*/) {
    return {};
}

bool DiskNavigatorService::getItemMeta(const std::wstring& /*filePath*/, ItemMeta& /*outMeta*/) {
    return false;
}

void DiskNavigatorService::saveItemMeta(const std::wstring& /*filePath*/, std::function<void(ItemMeta&)> /*updater*/) {
}

void DiskNavigatorService::handleDiskRename(const std::wstring& /*oldPath*/, const std::wstring& /*newPath*/, bool /*isDir*/) {
}

} // namespace RapidNotes
