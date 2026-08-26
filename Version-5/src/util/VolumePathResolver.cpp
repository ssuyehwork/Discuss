#include "VolumePathResolver.h"
#include <cwctype>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace QuarkMeta {

std::wstring VolumePathResolver::getVolumeSerialNumber(const std::wstring& path) {
    if (path.length() < 2 || path[1] != L':') return L"UNKNOWN";
#ifdef Q_OS_WIN
    wchar_t root[4] = { static_cast<wchar_t>(towupper(path[0])), L':', L'\\', L'\0' };
    wchar_t volumeName[MAX_PATH + 1] = { 0 };
    DWORD serialNumber = 0;
    if (GetVolumeInformationW(root, volumeName, MAX_PATH, &serialNumber, nullptr, nullptr, nullptr, 0)) {
        wchar_t buf[64];
        swprintf_s(buf, 64, L"%08X", serialNumber);
        return std::wstring(buf);
    }
#endif
    return L"UNKNOWN";
}

} // namespace QuarkMeta
