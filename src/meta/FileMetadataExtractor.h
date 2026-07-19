#ifndef ARCMETA_FILE_METADATA_EXTRACTOR_H
#define ARCMETA_FILE_METADATA_EXTRACTOR_H

#include <string>
#include <QColor>

namespace ArcMeta {

class FileMetadataExtractor {
public:
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

    static bool extractDimensions(const std::wstring& path, int& width, int& height);
    static QColor analyzeDominantColor(const std::wstring& path);
};

} // namespace ArcMeta

#endif // ARCMETA_FILE_METADATA_EXTRACTOR_H
