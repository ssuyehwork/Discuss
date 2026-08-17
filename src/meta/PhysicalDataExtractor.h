#ifndef ARCMETA_PHYSICAL_DATA_EXTRACTOR_H
#define ARCMETA_PHYSICAL_DATA_EXTRACTOR_H

#include <string>
#include <QString>
#include <windows.h>

namespace ArcMeta {

/**
 * @brief 专职负责 Windows 物理磁盘 I/O 元数据与 FRN 指纹直接获取的静态纯函数服务
 */
class PhysicalDataExtractor {
public:
    static std::string generateFallbackFolderId(const std::wstring& vol, const std::wstring& frn);
    static std::string generateDeterministicFolderId(const std::wstring& path);
    static std::wstring generateDeterministicFrn(const std::wstring& path);
    static std::wstring getVolumeSerialNumber(const std::wstring& path);

    /**
     * @brief 通过 WinAPI 获取 File ID 和基础元数据 (从 MetadataManager 物理移入，保持 FRN:VOL:FRN 统一指纹格式)
     */
    static bool fetchWinApiMetadataDirect(
        const std::wstring& path, 
        std::string& outId128, 
        std::wstring* outFrn = nullptr, 
        long long* outSize = nullptr, 
        std::wstring* outType = nullptr, 
        long long* outCtime = nullptr, 
        long long* outMtime = nullptr, 
        long long* outAtime = nullptr
    );
};

} // namespace ArcMeta

#endif // ARCMETA_PHYSICAL_DATA_EXTRACTOR_H
