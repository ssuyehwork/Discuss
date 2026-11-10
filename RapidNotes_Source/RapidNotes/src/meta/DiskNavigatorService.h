#pragma once 
#include <string> 
#include <unordered_map> 
#include <functional> 
#include "MetadataDefs.h" 

namespace RapidNotes { 
class DiskNavigatorService { 
public: 
    static DiskNavigatorService& instance(); 

    // 1. 读取指定物理目录下的所有项目的离散元数据 (侧车已废除，返回空映射)
    std::unordered_map<std::wstring, ItemMeta> loadDirectoryItems(const std::wstring& folderPath); 

    // 2. 获取单个磁盘文件的 ItemMeta 
    bool getItemMeta(const std::wstring& filePath, ItemMeta& outMeta); 

    // 3. 修改并原子落盘单个项目的元数据 (侧车已废除，空实现)
    void saveItemMeta(const std::wstring& filePath, std::function<void(ItemMeta&)> updater); 

    // 4. 本地文件/文件夹重命名时同步更新 (侧车已废除，空实现)
    void handleDiskRename(const std::wstring& oldPath, const std::wstring& newPath, bool isDir); 

private: 
    DiskNavigatorService() = default; 
    ~DiskNavigatorService() = default; 
}; 
}
