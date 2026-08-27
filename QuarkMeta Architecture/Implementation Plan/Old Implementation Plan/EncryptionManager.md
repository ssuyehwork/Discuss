# QuarkMeta 外壳保护（加解密与安全生命周期）实施方案

## 1. 目标与范围
- 重构 `.amenc` 文件结构：引入 `Magic ("QUARKENC") + 32B PwdVerifier`，支持 1 毫秒级快速密码校验。
- 升级 `EncryptionManager`：补齐 `encryptFile`（物理加密）、`decryptFile`（物理原路还原）、`decryptToTemp`（RAII 临时预览句柄）、`verifyPassword`（快速验密）及 `isEncryptedFile`（特征检测）。
- 新建 `ProtectionService` 领域服务：统一收敛“执行保护”、“解除保护”、“修改密码”与“加密文件安全预览”交互与调度。
- 净化 `ModelContract.h`：彻底删除歧义的 `IsLockedRole`，严格物理隔离 `PinnedRole` 与 `EncryptedRole`。
- 改造 UI 展现与交互：消除 `ContentPanel` 中的裸多线程与占位空分支，支持双击/空格无缝密码校验预览。

---

## 2. 核心服务与模块实现

### 2.1 `src/crypto/EncryptionManager.h`
```cpp
#ifndef NOMINMAX
#define NOMINMAX
#endif
#pragma once

#include <windows.h>
#include <bcrypt.h>
#include <string>
#include <vector>
#include <memory>

namespace QuarkMeta {

class DecryptedFileHandle {
public:
    DecryptedFileHandle(HANDLE hFile, const std::wstring& path) 
        : m_hFile(hFile), m_path(path) {}
    ~DecryptedFileHandle() {
        if (m_hFile != INVALID_HANDLE_VALUE) CloseHandle(m_hFile);
    }
    std::wstring path() const { return m_path; }
    bool isValid() const { return m_hFile != INVALID_HANDLE_VALUE; }

private:
    HANDLE m_hFile;
    std::wstring m_path;
};

class EncryptionManager {
public:
    static EncryptionManager& instance();

    bool isEncryptedFile(const std::wstring& filePath);
    bool verifyPassword(const std::wstring& amencPath, const std::string& password);

    bool encryptFile(const std::wstring& srcPath, const std::wstring& destPath, const std::string& password);
    bool decryptFile(const std::wstring& amencPath, const std::wstring& destPath, const std::string& password);
    std::shared_ptr<DecryptedFileHandle> decryptToTemp(const std::wstring& amencPath, const std::string& password);

private:
    EncryptionManager();
    ~EncryptionManager();
    EncryptionManager(const EncryptionManager&) = delete;
    EncryptionManager& operator=(const EncryptionManager&) = delete;

    bool deriveKeys(const std::string& password, const std::vector<BYTE>& salt, 
                    std::vector<BYTE>& encKey, std::vector<BYTE>& checkHash);
    std::vector<BYTE> generateRandom(size_t size);

    BCRYPT_ALG_HANDLE m_aesAlg = NULL;
    static constexpr char kMagic[8] = {'Q', 'U', 'A', 'R', 'K', 'E', 'N', 'C'};
};

} // namespace QuarkMeta
```

### 2.2 `src/crypto/EncryptionManager.cpp`
```cpp
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "EncryptionManager.h"
#include <fstream>
#include <filesystem>
#include <QString>
#include <cstring>

#pragma comment(lib, "bcrypt.lib")

namespace QuarkMeta {

EncryptionManager& EncryptionManager::instance() {
    static EncryptionManager inst;
    return inst;
}

EncryptionManager::EncryptionManager() {
    BCryptOpenAlgorithmProvider(&m_aesAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
    BCryptSetProperty(m_aesAlg, BCRYPT_CHAINING_MODE, (PBYTE)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
}

EncryptionManager::~EncryptionManager() {
    if (m_aesAlg) BCryptCloseAlgorithmProvider(m_aesAlg, 0);
}

std::vector<BYTE> EncryptionManager::generateRandom(size_t size) {
    std::vector<BYTE> buffer(size);
    BCryptGenRandom(NULL, buffer.data(), static_cast<ULONG>(size), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    return buffer;
}

bool EncryptionManager::deriveKeys(const std::string& password, const std::vector<BYTE>& salt, 
                                   std::vector<BYTE>& encKey, std::vector<BYTE>& checkHash) {
    BCRYPT_ALG_HANDLE hPbkdf2 = NULL;
    if (BCryptOpenAlgorithmProvider(&hPbkdf2, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG) != 0) {
        return false;
    }

    std::vector<BYTE> derived(64);
    NTSTATUS status = BCryptDeriveKeyPBKDF2(hPbkdf2, (PUCHAR)password.c_str(), static_cast<ULONG>(password.length()),
                                           (PUCHAR)salt.data(), static_cast<ULONG>(salt.size()), 10000, 
                                           derived.data(), static_cast<ULONG>(derived.size()), 0);
    BCryptCloseAlgorithmProvider(hPbkdf2, 0);

    if (status != 0) return false;

    encKey.assign(derived.begin(), derived.begin() + 32);
    checkHash.assign(derived.begin() + 32, derived.end());
    return true;
}

bool EncryptionManager::isEncryptedFile(const std::wstring& filePath) {
    std::ifstream is(QString::fromStdWString(filePath).toStdString(), std::ios::binary);
    if (!is) return false;

    char magic[8];
    is.read(magic, 8);
    if (is.gcount() < 8) return false;

    return std::memcmp(magic, kMagic, 8) == 0;
}

bool EncryptionManager::verifyPassword(const std::wstring& amencPath, const std::string& password) {
    std::ifstream is(QString::fromStdWString(amencPath).toStdString(), std::ios::binary);
    if (!is) return false;

    char magic[8];
    std::vector<BYTE> salt(16);
    std::vector<BYTE> iv(16);
    std::vector<BYTE> storedCheckHash(32);

    is.read(magic, 8);
    if (std::memcmp(magic, kMagic, 8) != 0) return false;

    is.read((char*)salt.data(), 16);
    is.read((char*)iv.data(), 16);
    is.read((char*)storedCheckHash.data(), 32);

    if (is.gcount() < 32) return false;

    std::vector<BYTE> encKey;
    std::vector<BYTE> checkHash;
    if (!deriveKeys(password, salt, encKey, checkHash)) return false;

    return std::memcmp(storedCheckHash.data(), checkHash.data(), 32) == 0;
}

bool EncryptionManager::encryptFile(const std::wstring& srcPath, const std::wstring& destPath, const std::string& password) {
    std::ifstream is(QString::fromStdWString(srcPath).toStdString(), std::ios::binary);
    if (!is) return false;
    std::ofstream os(QString::fromStdWString(destPath).toStdString(), std::ios::binary);
    if (!os) return false;

    std::vector<BYTE> salt = generateRandom(16);
    std::vector<BYTE> iv = generateRandom(16);
    std::vector<BYTE> encKey;
    std::vector<BYTE> checkHash;

    if (!deriveKeys(password, salt, encKey, checkHash)) return false;

    os.write(kMagic, 8);
    os.write((char*)salt.data(), salt.size());
    os.write((char*)iv.data(), iv.size());
    os.write((char*)checkHash.data(), checkHash.size());

    BCRYPT_KEY_HANDLE hKey = NULL;
    BCryptGenerateSymmetricKey(m_aesAlg, &hKey, NULL, 0, encKey.data(), static_cast<ULONG>(encKey.size()), 0);

    const size_t CHUNK_SIZE = 64 * 1024;
    std::vector<BYTE> buffer(CHUNK_SIZE);
    std::vector<BYTE> cipherBuffer(CHUNK_SIZE + 16);

    while (is.read((char*)buffer.data(), CHUNK_SIZE) || is.gcount() > 0) {
        DWORD readBytes = static_cast<DWORD>(is.gcount());
        DWORD cipherLen = 0;
        bool isLast = is.eof();
        
        BCryptEncrypt(hKey, buffer.data(), readBytes, NULL, iv.data(), static_cast<ULONG>(iv.size()), 
                      cipherBuffer.data(), static_cast<ULONG>(cipherBuffer.size()), &cipherLen, 
                      isLast ? BCRYPT_BLOCK_PADDING : 0);
        
        os.write((char*)cipherBuffer.data(), cipherLen);
    }

    BCryptDestroyKey(hKey);
    is.close();
    os.close();
    return true;
}

bool EncryptionManager::decryptFile(const std::wstring& amencPath, const std::wstring& destPath, const std::string& password) {
    if (!verifyPassword(amencPath, password)) return false;

    std::ifstream is(QString::fromStdWString(amencPath).toStdString(), std::ios::binary);
    if (!is) return false;
    std::ofstream os(QString::fromStdWString(destPath).toStdString(), std::ios::binary);
    if (!os) return false;

    char magic[8];
    std::vector<BYTE> salt(16);
    std::vector<BYTE> iv(16);
    std::vector<BYTE> storedCheckHash(32);

    is.read(magic, 8);
    is.read((char*)salt.data(), 16);
    is.read((char*)iv.data(), 16);
    is.read((char*)storedCheckHash.data(), 32);

    std::vector<BYTE> encKey;
    std::vector<BYTE> checkHash;
    if (!deriveKeys(password, salt, encKey, checkHash)) return false;

    BCRYPT_KEY_HANDLE hKey = NULL;
    BCryptGenerateSymmetricKey(m_aesAlg, &hKey, NULL, 0, encKey.data(), static_cast<ULONG>(encKey.size()), 0);

    const size_t CHUNK_SIZE = 64 * 1024;
    std::vector<BYTE> buffer(CHUNK_SIZE + 16);
    std::vector<BYTE> plainBuffer(CHUNK_SIZE + 16);

    while (is.read((char*)buffer.data(), CHUNK_SIZE + 16) || is.gcount() > 0) {
        DWORD readBytes = static_cast<DWORD>(is.gcount());
        DWORD plainLen = 0;
        bool isLast = is.eof();

        NTSTATUS status = BCryptDecrypt(hKey, buffer.data(), readBytes, NULL, iv.data(), static_cast<ULONG>(iv.size()),
                                        plainBuffer.data(), static_cast<ULONG>(plainBuffer.size()), &plainLen,
                                        isLast ? BCRYPT_BLOCK_PADDING : 0);
        if (status != 0) {
            BCryptDestroyKey(hKey);
            is.close();
            os.close();
            return false;
        }

        os.write((char*)plainBuffer.data(), plainLen);
    }

    BCryptDestroyKey(hKey);
    is.close();
    os.close();
    return true;
}

std::shared_ptr<DecryptedFileHandle> EncryptionManager::decryptToTemp(const std::wstring& amencPath, const std::string& password) {
    if (!verifyPassword(amencPath, password)) return nullptr;

    std::ifstream is(QString::fromStdWString(amencPath).toStdString(), std::ios::binary);
    if (!is) return nullptr;

    char magic[8];
    std::vector<BYTE> salt(16);
    std::vector<BYTE> iv(16);
    std::vector<BYTE> storedCheckHash(32);

    is.read(magic, 8);
    is.read((char*)salt.data(), 16);
    is.read((char*)iv.data(), 16);
    is.read((char*)storedCheckHash.data(), 32);

    std::vector<BYTE> encKey;
    std::vector<BYTE> checkHash;
    if (!deriveKeys(password, salt, encKey, checkHash)) return nullptr;

    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    std::wstring amTempDir = std::wstring(tempPath) + L"amtemp\\";
    CreateDirectoryW(amTempDir.c_str(), NULL);

    std::wstring outPath = amTempDir + std::filesystem::path(amencPath).stem().wstring();
    HANDLE hFile = CreateFileW(outPath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    
    if (hFile == INVALID_HANDLE_VALUE) return nullptr;

    BCRYPT_KEY_HANDLE hKey = NULL;
    BCryptGenerateSymmetricKey(m_aesAlg, &hKey, NULL, 0, encKey.data(), static_cast<ULONG>(encKey.size()), 0);

    const size_t CHUNK_SIZE = 64 * 1024;
    std::vector<BYTE> buffer(CHUNK_SIZE + 16);
    std::vector<BYTE> plainBuffer(CHUNK_SIZE + 16);

    while (is.read((char*)buffer.data(), CHUNK_SIZE + 16) || is.gcount() > 0) {
        DWORD readBytes = static_cast<DWORD>(is.gcount());
        DWORD plainLen = 0;
        bool isLast = is.eof();

        NTSTATUS status = BCryptDecrypt(hKey, buffer.data(), readBytes, NULL, iv.data(), static_cast<ULONG>(iv.size()),
                                        plainBuffer.data(), static_cast<ULONG>(plainBuffer.size()), &plainLen,
                                        isLast ? BCRYPT_BLOCK_PADDING : 0);
        if (status != 0) {
            BCryptDestroyKey(hKey);
            CloseHandle(hFile);
            return nullptr;
        }

        DWORD written = 0;
        WriteFile(hFile, plainBuffer.data(), plainLen, &written, NULL);
    }

    BCryptDestroyKey(hKey);
    is.close();
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    return std::make_shared<DecryptedFileHandle>(hFile, outPath);
}

} // namespace QuarkMeta
```

---

### 2.3 `src/core/ProtectionService.h`
```cpp
#pragma once

#include <QObject>
#include <QStringList>
#include <QWidget>
#include <memory>
#include "../crypto/EncryptionManager.h"

namespace QuarkMeta {

class ProtectionService : public QObject {
    Q_OBJECT

public:
    static ProtectionService& instance();

    bool protectFiles(const QStringList& paths, QWidget* parentWidget = nullptr);
    bool unprotectFiles(const QStringList& paths, QWidget* parentWidget = nullptr);
    bool changePassword(const QStringList& paths, QWidget* parentWidget = nullptr);

    std::shared_ptr<DecryptedFileHandle> previewProtectedFile(const QString& amencPath, QWidget* parentWidget = nullptr);

signals:
    void protectionOperationCompleted();

private:
    explicit ProtectionService(QObject* parent = nullptr);
    ~ProtectionService() override = default;
    ProtectionService(const ProtectionService&) = delete;
    ProtectionService& operator=(const ProtectionService&) = delete;
};

} // namespace QuarkMeta
```

### 2.4 `src/core/ProtectionService.cpp`
```cpp
#include "ProtectionService.h"
#include "../ui/dialogs/FramelessInputDialog.h"
#include "../ui/ToolTipOverlay.h"
#include "../meta/MetadataManager.h"
#include <QtConcurrent>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QPointer>
#include <QCoreApplication>
#include <QLineEdit>

namespace QuarkMeta {

ProtectionService& ProtectionService::instance() {
    static ProtectionService s_instance;
    return s_instance;
}

ProtectionService::ProtectionService(QObject* parent) : QObject(parent) {}

bool ProtectionService::protectFiles(const QStringList& paths, QWidget* parentWidget) {
    if (paths.isEmpty()) return false;

    FramelessInputDialog dlg("执行外壳保护", "设置保护密码:", "", parentWidget);
    dlg.setEchoMode(QLineEdit::Password);
    if (dlg.exec() != QDialog::Accepted) return false;

    QString pwd = dlg.text();
    if (pwd.isEmpty()) return false;

    ToolTipOverlay::instance()->showText(QCursor::pos(), "加密保护任务已在后台启动...", 1500, QColor("#3498db"));

    std::string stdPwd = pwd.toStdString();
    QPointer<ProtectionService> weakThis(this);

    MetadataManager::instance().beginInternalOperation();

    (void)QtConcurrent::run([paths, stdPwd, weakThis]() {
        int successCount = 0;
        for (const QString& src : paths) {
            if (src.endsWith(".amenc", Qt::CaseInsensitive)) continue;

            QString dest = src + ".amenc";
            std::wstring wSrc = QDir::toNativeSeparators(src).toStdWString();
            std::wstring wDest = QDir::toNativeSeparators(dest).toStdWString();

            if (EncryptionManager::instance().encryptFile(wSrc, wDest, stdPwd)) {
                QFile::remove(src);
                MetadataManager::instance().setEncrypted(wDest, true);
                successCount++;
            }
        }

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, successCount]() {
            MetadataManager::instance().endInternalOperation();
            if (weakThis) {
                emit weakThis->protectionOperationCompleted();
                ToolTipOverlay::instance()->showText(
                    QCursor::pos(), 
                    QString("成功保护 %1 个项目").arg(successCount), 
                    1500, 
                    QColor("#2ecc71")
                );
            }
        });
    });

    return true;
}

bool ProtectionService::unprotectFiles(const QStringList& paths, QWidget* parentWidget) {
    if (paths.isEmpty()) return false;

    FramelessInputDialog dlg("解除外壳保护", "输入保护密码:", "", parentWidget);
    dlg.setEchoMode(QLineEdit::Password);
    if (dlg.exec() != QDialog::Accepted) return false;

    QString pwd = dlg.text();
    if (pwd.isEmpty()) return false;

    std::string stdPwd = pwd.toStdString();

    for (const QString& amencPath : paths) {
        if (!amencPath.endsWith(".amenc", Qt::CaseInsensitive)) continue;
        if (!EncryptionManager::instance().verifyPassword(amencPath.toStdWString(), stdPwd)) {
            ToolTipOverlay::instance()->showText(QCursor::pos(), "密码错误，解除保护失败", 2000, QColor("#e81123"));
            return false;
        }
    }

    ToolTipOverlay::instance()->showText(QCursor::pos(), "正在执行解密还原...", 1500, QColor("#3498db"));
    QPointer<ProtectionService> weakThis(this);

    MetadataManager::instance().beginInternalOperation();

    (void)QtConcurrent::run([paths, stdPwd, weakThis]() {
        int successCount = 0;
        for (const QString& amencPath : paths) {
            if (!amencPath.endsWith(".amenc", Qt::CaseInsensitive)) continue;

            QString dest = amencPath.left(amencPath.length() - 6); // 移除 .amenc
            std::wstring wAmenc = QDir::toNativeSeparators(amencPath).toStdWString();
            std::wstring wDest = QDir::toNativeSeparators(dest).toStdWString();

            if (EncryptionManager::instance().decryptFile(wAmenc, wDest, stdPwd)) {
                QFile::remove(amencPath);
                MetadataManager::instance().setEncrypted(wDest, false);
                successCount++;
            }
        }

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, successCount]() {
            MetadataManager::instance().endInternalOperation();
            if (weakThis) {
                emit weakThis->protectionOperationCompleted();
                ToolTipOverlay::instance()->showText(
                    QCursor::pos(), 
                    QString("成功解除 %1 个项目的保护").arg(successCount), 
                    1500, 
                    QColor("#2ecc71")
                );
            }
        });
    });

    return true;
}

bool ProtectionService::changePassword(const QStringList& paths, QWidget* parentWidget) {
    if (paths.isEmpty()) return false;

    FramelessInputDialog oldDlg("修改保护密码", "输入当前旧密码:", "", parentWidget);
    oldDlg.setEchoMode(QLineEdit::Password);
    if (oldDlg.exec() != QDialog::Accepted) return false;

    QString oldPwd = oldDlg.text();
    if (oldPwd.isEmpty()) return false;

    std::string stdOldPwd = oldPwd.toStdString();
    for (const QString& p : paths) {
        if (p.endsWith(".amenc", Qt::CaseInsensitive)) {
            if (!EncryptionManager::instance().verifyPassword(p.toStdWString(), stdOldPwd)) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "旧密码校验失败", 2000, QColor("#e81123"));
                return false;
            }
        }
    }

    FramelessInputDialog newDlg("修改保护密码", "输入新保护密码:", "", parentWidget);
    newDlg.setEchoMode(QLineEdit::Password);
    if (newDlg.exec() != QDialog::Accepted) return false;

    QString newPwd = newDlg.text();
    if (newPwd.isEmpty()) return false;

    std::string stdNewPwd = newPwd.toStdString();
    QPointer<ProtectionService> weakThis(this);

    MetadataManager::instance().beginInternalOperation();

    (void)QtConcurrent::run([paths, stdOldPwd, stdNewPwd, weakThis]() {
        int successCount = 0;
        for (const QString& amencPath : paths) {
            if (!amencPath.endsWith(".amenc", Qt::CaseInsensitive)) continue;

            QString tempPlain = amencPath + ".tmp_plain";
            std::wstring wAmenc = QDir::toNativeSeparators(amencPath).toStdWString();
            std::wstring wPlain = QDir::toNativeSeparators(tempPlain).toStdWString();

            if (EncryptionManager::instance().decryptFile(wAmenc, wPlain, stdOldPwd)) {
                QFile::remove(amencPath);
                if (EncryptionManager::instance().encryptFile(wPlain, wAmenc, stdNewPwd)) {
                    QFile::remove(tempPlain);
                    successCount++;
                }
            }
        }

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, successCount]() {
            MetadataManager::instance().endInternalOperation();
            if (weakThis) {
                emit weakThis->protectionOperationCompleted();
                ToolTipOverlay::instance()->showText(
                    QCursor::pos(), 
                    QString("成功修改 %1 个项目的密码").arg(successCount), 
                    1500, 
                    QColor("#2ecc71")
                );
            }
        });
    });

    return true;
}

std::shared_ptr<DecryptedFileHandle> ProtectionService::previewProtectedFile(const QString& amencPath, QWidget* parentWidget) {
    if (!amencPath.endsWith(".amenc", Qt::CaseInsensitive)) return nullptr;

    FramelessInputDialog dlg("访问受保护项目", "请输入保护密码以查看:", "", parentWidget);
    dlg.setEchoMode(QLineEdit::Password);
    if (dlg.exec() != QDialog::Accepted) return nullptr;

    QString pwd = dlg.text();
    if (pwd.isEmpty()) return nullptr;

    auto handle = EncryptionManager::instance().decryptToTemp(amencPath.toStdWString(), pwd.toStdString());
    if (!handle || !handle->isValid()) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "密码错误，无法打开受保护项目", 2000, QColor("#e81123"));
        return nullptr;
    }

    return handle;
}

} // namespace QuarkMeta
```

---

### 2.5 `src/core/ModelContract.h` 契约净化
```cpp
#pragma once
#include <Qt>

namespace QuarkMeta {

enum CommonRole {
    TypeRole            = Qt::UserRole + 0,
    IdRole              = Qt::UserRole + 1,
    NameRole            = Qt::UserRole + 2,
    PathRole            = Qt::UserRole + 3,
    ColorRole           = Qt::UserRole + 4,
    RatingRole          = Qt::UserRole + 5,
    TagsRole            = Qt::UserRole + 6,
    
    // 状态角色：彻底剔除 IsLockedRole，纯粹分工
    PinnedRole          = Qt::UserRole + 101, // 仅代表置顶状态
    EncryptedRole       = Qt::UserRole + 103, // 仅代表外壳保护状态
    EncryptHintRole     = Qt::UserRole + 104,
    IsEmptyRole         = Qt::UserRole + 106,
    
    AspectRatioRole     = Qt::UserRole + 201,
    HasThumbnailRole    = Qt::UserRole + 202,
    PalettesRole        = Qt::UserRole + 203,
    CountRole           = Qt::UserRole + 204,

    IsDiskTrashRole     = Qt::UserRole + 208,
    DiskTrashIdRole     = Qt::UserRole + 209
};

} // namespace QuarkMeta
```

---

## 3. UI 交互与调用改造

### 3.1 `ContentPanel.cpp` 构造绑定
```cpp
connect(&ProtectionService::instance(), &ProtectionService::protectionOperationCompleted, this, &ContentPanel::refreshAll);
```

### 3.2 `ContentPanel.cpp` 右键菜单与动作执行收缩
```cpp
// 置顶动作纯净绑定 PinnedRole
case ActionPin:
case ActionUnpin: {
    auto indexes = view->selectionModel()->selectedIndexes();
    bool pin = (action == ActionPin);
    for (const QModelIndex& idx : indexes) {
        if (idx.column() == 0) {
            m_proxyModel->setData(idx, pin, PinnedRole);
        }
    }
    m_proxyModel->invalidate();
    m_proxyModel->sort(0, m_proxyModel->sortOrder());
    break;
}

// 外壳保护三大动作纯净收敛
case ActionEncrypt: {
    ProtectionService::instance().protectFiles(getSelectedPaths(), this);
    break;
}
case ActionDecrypt: {
    ProtectionService::instance().unprotectFiles(getSelectedPaths(), this);
    break;
}
case ActionChangePwd: {
    ProtectionService::instance().changePassword(getSelectedPaths(), this);
    break;
}
```

### 3.3 `ContentPanel.cpp` 双击与空格键拦截预览
```cpp
// onDoubleClicked 与 Key_Space 中拦截 .amenc 文件：
if (path.endsWith(".amenc", Qt::CaseInsensitive)) {
    auto tempHandle = ProtectionService::instance().previewProtectedFile(path, this);
    if (tempHandle && tempHandle->isValid()) {
        emit requestQuickLook(QString::fromStdWString(tempHandle->path()));
    }
    return;
}
```

---

## 4. `CMakeLists.txt` 构建配置注册
```cmake
set(CORE_SOURCES
    # ... 现有源文件 ...
    src/core/ProtectionService.h
    src/core/ProtectionService.cpp
    src/crypto/EncryptionManager.h
    src/crypto/EncryptionManager.cpp
)
```