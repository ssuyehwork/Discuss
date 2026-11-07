#pragma once
#include <string>

namespace RapidNotes {

class VolumePathResolver {
public:
    static std::wstring getVolumeSerialNumber(const std::wstring& path);
};

} // namespace RapidNotes
