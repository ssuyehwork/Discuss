#include "MetaCacheDecorator.h" 
#include "DriveMetaDao.h"
#include "MetadataManager.h"
#include "ItemMetaMapper.h"
#include <QFileInfo> 
#include <QDir>
#include <unordered_map> 
#include <memory> 
 
namespace RapidNotes { 
void MetaCacheDecorator::decorate(std::vector<ItemRecord>& records) { 
    if (records.empty()) return; 

    // 预先批量拉取全局盘符元数据
    auto driveMetas = DriveMetaDao::getAllDriveMeta();
 
    for (auto& itemRec : records) { 
        // 【盘符特殊处理】：如果是驱动器根目录（如 C:\、D:\）
        std::wstring normWPath = MetadataManager::normalizePath(itemRec.path.toStdWString());
        QFileInfo info(itemRec.path);
        if (info.isRoot() || itemRec.path.endsWith(":\\") || itemRec.path.endsWith(":/")) {
            auto driveIt = driveMetas.find(normWPath);
            if (driveIt != driveMetas.end()) {
                itemRec.rating = driveIt->second.rating;
                itemRec.manualColor = QString::fromStdWString(driveIt->second.color);
                itemRec.pinned = driveIt->second.pinned;
                itemRec.note = QString::fromStdWString(driveIt->second.note);
                itemRec.url = QString::fromStdWString(driveIt->second.url);
            }
            continue;
        }
    } 
} 
}
