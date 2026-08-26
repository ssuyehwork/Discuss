# 全代码库物理四层解耦与职责单一重构实施方案 (Full Codebase Four-Layer Decoupling & SRP Implementation Plan)

## Overview（概述与解决的问题）
本实施方案旨在全面清理代码库中所有违背单一职责原则（SRP）、跨层越权调用的历史负债：
1. **清理 UI 跨层越权**：将 `ContentPanel.cpp` 中的 `SecureFileEraser`（物理粉碎）、`EncryptionManager`（AES 加解密）与 `QtConcurrent` 线程池调度解耦剥离；将 `MetaPanel.cpp` 中的磁盘 `.QuarkMeta.json` 物理文件读写剥离；将 `MainWindow.cpp` 中的 Win32 消息拦截与多面板数据散装拼装解耦。
2. **清理过载头文件**：将 `BasicCommands.h`（混杂 6 个独立 Command）、`DatabaseMigrator.h`（混杂 SQLite 建表与卷标解析）、`FramelessDialog.h`（混杂 5 个独立对话框）等过载头文件按“一类一文件”或“主题独立组件”彻底物理拆分。
3. **建立四层架构标准**：建立 UI 视图层、Controller 调度层、Service 业务服务层与 DAO 持久层的标准流转通道，为未来无限扩展新功能打下坚实的物理基础。

---

## Modified Files List（影响文件清单）

### 1. 拆分新建头文件与源文件 (New Sub-Components)
- `src/core/commands/RenameCommand.h`
- `src/core/commands/MoveCommand.h`
- `src/core/commands/MetadataCommand.h`
- `src/core/commands/SecureDeleteCommand.h`
- `src/core/commands/EncryptCommand.h`
- `src/core/commands/BatchRenameCommand.h`
- `src/util/VolumePathResolver.h`
- `src/ui/dialogs/FramelessInputDialog.h`
- `src/ui/dialogs/FramelessColorPicker.h`
- `src/ui/dialogs/FramelessConfirmDialog.h`
- `src/ui/dialogs/FramelessMessageBox.h`

### 2. 重构与解耦原文件 (Modified Existing Files)
- `CMakeLists.txt`
- `QuarkMeta Architecture/QuarkMeta-Architecture-Planning.md`
- `src/core/BasicCommands.h`
- `src/meta/DatabaseMigrator.h`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/MetaPanel.h`
- `src/ui/MetaPanel.cpp`
- `src/ui/MainWindow.h`
- `src/ui/MainWindow.cpp`
- `src/ui/FramelessDialog.h`

---

## Detailed Line-by-Line Changes（精准替换块与结构解耦）

### 1. CMakeLists.txt 新增文件注册 (SOURCES / HEADERS)
```
<<<<<<< SEARCH
    src/core/BasicCommands.h
    src/meta/DatabaseMigrator.h
=======
    src/core/BasicCommands.h
    src/core/commands/RenameCommand.h
    src/core/commands/MoveCommand.h
    src/core/commands/MetadataCommand.h
    src/core/commands/SecureDeleteCommand.h
    src/core/commands/EncryptCommand.h
    src/core/commands/BatchRenameCommand.h
    src/util/VolumePathResolver.h
    src/meta/DatabaseMigrator.h
>>>>>>> REPLACE
```

### 2. `src/meta/DatabaseMigrator.h` 解耦 `VolumePathResolver`
```
<<<<<<< SEARCH
class VolumePathResolver {
public:
    static std::wstring getVolumeSerialNumber(const std::wstring& path) {
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
};
=======
// VolumePathResolver 已物理解耦提取至 src/util/VolumePathResolver.h
>>>>>>> REPLACE
```

### 3. `src/ui/ContentPanel.cpp` 解耦跨层物理抹除与加密
```
<<<<<<< SEARCH
#include "../util/SecureFileEraser.h"
#include "../crypto/EncryptionManager.h"
#include <QtConcurrent>
=======
// 彻底解耦越权头文件，物理抹除与加解密统一改由 CoreEngine::submitCommand 转发处理
>>>>>>> REPLACE
```

### 4. `src/ui/MetaPanel.cpp` 物理解耦磁盘 JSON 越权读写
```
<<<<<<< SEARCH
// 内部直接调用 QuarkMetaJson::loadMetadataFromFile
=======
// 解耦越权文件 I/O，改为统一通过 MetadataManager::instance().getMetaAsync 异步获取
>>>>>>> REPLACE
```

---

## Build & Verification Steps（编译命令与验证方法）

1. **编译验证**：
   在根目录下运行 CMake 构建命令，确保 MOC 编译与新文件注册无任何缺失：
   ```bash
   cmake -B build
   cmake --build build --config Release
   ```

2. **架构规范验证**：
   - 检查 `ContentPanel.cpp` 与 `MetaPanel.cpp` 中不再包含 `SecureFileEraser.h` / `EncryptionManager.h` 等底层跨层头文件。
   - 检查 `BasicCommands.h` 与 `DatabaseMigrator.h` 均完成单一职责拆分，各无关联子类已物理隔离至独立 `.h` 头文件。
