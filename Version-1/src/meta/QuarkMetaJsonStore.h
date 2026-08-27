#pragma once

#include <string>
#include <QStringList>
#include "QuarkMetaJson.h"

namespace QuarkMeta {

class QuarkMetaJsonStore {
public:
    static QuarkMetaJsonStore& instance();

    void updateItemMeta(const std::wstring& path, std::function<void(ItemMeta&)> modifier);
    bool renameItem(const QString& folderPath, const QString& oldName, const QString& newName);
    bool migrateFolderCache(const QString& oldFolderPath, const QString& newFolderPath);

private:
    QuarkMetaJsonStore() = default;
    ~QuarkMetaJsonStore() = default;
};

} // namespace QuarkMeta
