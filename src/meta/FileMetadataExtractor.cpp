#include "FileMetadataExtractor.h"
#include "UiHelper.h"
#include <windows.h>
#include <QImageReader>
#include <QFileInfo>
#include <QDebug>

namespace ArcMeta {

// 从原 MetadataManager::fetchWinApiMetadataDirect 移植物理元数据提取
bool FileMetadataExtractor::fetchWinApiMetadataDirect(
    const std::wstring& path, 
    std::string& outId128, 
    std::wstring* outFrn, 
    long long* outSize, 
    std::wstring* outType, 
    long long* outCtime, 
    long long* outMtime, 
    long long* outAtime
) {
    HANDLE hFile = CreateFileW(
        path.c_str(),
        FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    BY_HANDLE_FILE_INFORMATION fileInfo;
    if (GetFileInformationByHandle(hFile, &fileInfo)) {
        if (outSize) {
            *outSize = (static_cast<long long>(fileInfo.nFileSizeHigh) << 32) | fileInfo.nFileSizeLow;
        }
        if (outCtime) {
            ULARGE_INTEGER uli;
            uli.LowPart = fileInfo.ftCreationTime.dwLowDateTime;
            uli.HighPart = fileInfo.ftCreationTime.dwHighDateTime;
            *outCtime = uli.QuadPart;
        }
        if (outMtime) {
            ULARGE_INTEGER uli;
            uli.LowPart = fileInfo.ftLastWriteTime.dwLowDateTime;
            uli.HighPart = fileInfo.ftLastWriteTime.dwHighDateTime;
            *outMtime = uli.QuadPart;
        }
        if (outAtime) {
            ULARGE_INTEGER uli;
            uli.LowPart = fileInfo.ftLastAccessTime.dwLowDateTime;
            uli.HighPart = fileInfo.ftLastAccessTime.dwHighDateTime;
            *outAtime = uli.QuadPart;
        }

        // 生成 FIDs 指纹 (32 字符的 128 位十六进制)
        char fidBuf[33];
        sprintf_s(fidBuf, "%08X%08X%08X%08X",
                  fileInfo.dwVolumeSerialNumber,
                  fileInfo.nFileIndexHigh,
                  fileInfo.nFileIndexLow,
                  0); // 预留
        outId128 = std::string(fidBuf);

        if (outFrn) {
            wchar_t frnBuf[33];
            swprintf_s(frnBuf, L"%08X%08X", fileInfo.nFileIndexHigh, fileInfo.nFileIndexLow);
            *outFrn = frnBuf;
        }

        if (outType) {
            if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                *outType = L"DIR";
            } else {
                QFileInfo fi(QString::fromStdWString(path));
                *outType = fi.suffix().toLower().toStdWString();
            }
        }

        CloseHandle(hFile);
        return true;
    }

    CloseHandle(hFile);
    return false;
}

bool FileMetadataExtractor::extractDimensions(const std::wstring& path, int& width, int& height) {
    QFileInfo fi(QString::fromStdWString(path));
    QString ext = fi.suffix().toLower();
    if (ext == "svg" || ext == "pdf" || ext == "ai") {
        width = 0; height = 0;
        return false;
    }
    
    QImageReader reader(QString::fromStdWString(path));
    if (reader.canRead()) {
        QSize sz = reader.size();
        if (sz.isValid()) {
            width = sz.width();
            height = sz.height();
            return true;
        }
    }
    width = 0; height = 0;
    return false;
}

QColor FileMetadataExtractor::analyzeDominantColor(const std::wstring& path) {
    QString qPath = QString::fromStdWString(path);
    auto palette = UiHelper::extractPalette(qPath);
    if (!palette.isEmpty()) {
        return palette.first().first;
    }
    return QColor();
}

} // namespace ArcMeta
