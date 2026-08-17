#include "PhysicalDataExtractor.h"
#include "MetadataManager.h"
#include <QFileInfo>
#include <QCryptographicHash>
#include <QDir>
#include <QDebug>
#include <cwchar>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace ArcMeta {

std::string PhysicalDataExtractor::generateFallbackFolderId(const std::wstring& vol, const std::wstring& frn) {
    if (vol.empty() || frn.empty()) return "";
    std::string result = "FRN:";
    result.append(QString::fromStdWString(vol).toUpper().toStdString());
    result.append(":");
    result.append(QString::fromStdWString(frn).toUpper().toStdString());
    return result;
}

std::string PhysicalDataExtractor::generateDeterministicFolderId(const std::wstring& path) {
    if (path.empty()) return "";
    std::wstring nPath = MetadataManager::normalizePath(path);
    std::wstring vol = PhysicalDataExtractor::getVolumeSerialNumber(nPath);

    std::wstring seedW(vol);
    seedW.append(L":");
    seedW.append(nPath);

    QByteArray seed = QString::fromStdWString(seedW).toUtf8();
    QByteArray hash = QCryptographicHash::hash(seed, QCryptographicHash::Sha256);

    std::string result = "PATHURL:";
    result.append(hash.left(16).toHex().toUpper().toStdString());
    return result;
}

std::wstring PhysicalDataExtractor::generateDeterministicFrn(const std::wstring& path) {
    if (path.empty()) return L"VIRTUAL_EMPTY";
    QByteArray hash = QCryptographicHash::hash(QString::fromStdWString(path).toUtf8(), QCryptographicHash::Sha256);
    return QString(hash.left(8).toHex().toUpper()).toStdWString();
}

std::wstring PhysicalDataExtractor::getVolumeSerialNumber(const std::wstring& path) {
    if (path.length() < 2 || path[1] != L':') return L"UNKNOWN";
    wchar_t root[4] = { static_cast<wchar_t>(towupper(path[0])), L':', L'\\', L'\0' };
    DWORD serial = 0;
    if (GetVolumeInformationW(root, nullptr, 0, &serial, nullptr, nullptr, nullptr, 0)) {
        wchar_t buf[16];
        swprintf_s(buf, 16, L"%08X", serial);
        return buf;
    }
    return L"UNKNOWN";
}

bool PhysicalDataExtractor::fetchWinApiMetadataDirect(
    const std::wstring& path, 
    std::string& outId128, 
    std::wstring* outFrn, 
    long long* outSize, 
    std::wstring* outType, 
    long long* outCtime, 
    long long* outMtime, 
    long long* outAtime
) {
    HANDLE hFile = CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    std::wstring vol = getVolumeSerialNumber(path);
    if (hFile == INVALID_HANDLE_VALUE) {
        if (outFrn) *outFrn = generateDeterministicFrn(path);
        outId128 = generateDeterministicFolderId(path);
        return false;
    }
    BY_HANDLE_FILE_INFORMATION basicInfo;
    if (GetFileInformationByHandle(hFile, &basicInfo)) {
        wchar_t frnBuf[17];
        unsigned long long fullFrn = (static_cast<unsigned long long>(basicInfo.nFileIndexHigh) << 32) | basicInfo.nFileIndexLow;
        swprintf_s(frnBuf, 17, L"%016llX", fullFrn);
        if (outFrn) *outFrn = frnBuf;
        outId128 = generateFallbackFolderId(vol, frnBuf);
        if (outSize) *outSize = (static_cast<long long>(basicInfo.nFileSizeHigh) << 32) | basicInfo.nFileSizeLow;
        if (outType) *outType = (basicInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? L"folder" : L"file";
        auto toMS = [](const FILETIME& ft) {
            ULARGE_INTEGER ull; ull.LowPart = ft.dwLowDateTime; ull.HighPart = ft.dwHighDateTime;
            return static_cast<long long>((ull.QuadPart - 116444736000000000ULL) / 10000ULL);
        };
        if (outCtime) *outCtime = toMS(basicInfo.ftCreationTime);
        if (outMtime) *outMtime = toMS(basicInfo.ftLastWriteTime);
        if (outAtime) *outAtime = toMS(basicInfo.ftLastAccessTime);
        CloseHandle(hFile);
        return true;
    }
    CloseHandle(hFile);
    return false;
}

} // namespace ArcMeta
