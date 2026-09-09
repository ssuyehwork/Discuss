# EncryptionManager Implementation Plan - Shell Protection Decryption and Filename Restoration

This implementation plan details the implementation of full AES-256 decryption file restoration (`decryptFile`) and accurate extension/filename recovery in `EncryptionManager` and `ContentContextMenu`.

---

## 1. Overview & Problem Statement
* **Issue**: When decrypting protected files (`.amenc`), previously some files were named with a `.decrypted` extension suffix (e.g., `DZKJ 电子科技_56635.eps.decrypted`) rather than accurately restoring the exact original filename (e.g., `DZKJ 电子科技_56635.eps`).
* **Solution**: 
  1. Refine the filename chop algorithm in `ContentContextMenu.cpp` (`ActionDecrypt` handler): if source ends with `.amenc`, chop 6 characters; if source ends with `.decrypted`, chop 10 characters, restoring clean original filenames.
  2. Maintain `EncryptionManager::decryptFile` for chunked AES-256 BCrypt decryption and padding verification.

---

## 2. Modified Files List
- `src/crypto/EncryptionManager.h`
- `src/crypto/EncryptionManager.cpp`
- `src/ui/controllers/ContentContextMenu.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/controllers/ContentContextMenu.cpp`

```git
<<<<<<< SEARCH
                    for (const QString& src : targets) {
                        QString dest = src;
                        if (dest.endsWith(".amenc", Qt::CaseInsensitive)) {
                            dest.chop(6);
                        } else {
                            dest += ".decrypted";
                        }

                        if (EncryptionManager::instance().decryptFile(src.toStdWString(), dest.toStdWString(), stdPwd)) {
                            QFile::remove(src);
                            MetadataManager::instance().setEncrypted(dest.toStdWString(), false);
                            anySuccess = true;
                        }
                    }
=======
                    for (const QString& src : targets) {
                        QString dest = src;
                        if (dest.endsWith(".amenc", Qt::CaseInsensitive)) {
                            dest.chop(6);
                        } else if (dest.endsWith(".decrypted", Qt::CaseInsensitive)) {
                            dest.chop(10);
                        }

                        if (dest == src) {
                            dest += ".dec";
                        }

                        if (EncryptionManager::instance().decryptFile(src.toStdWString(), dest.toStdWString(), stdPwd)) {
                            QFile::remove(src);
                            MetadataManager::instance().setEncrypted(dest.toStdWString(), false);
                            anySuccess = true;
                        }
                    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Verify `ContentContextMenu.cpp` chops `.amenc` (6 chars) or `.decrypted` (10 chars), restoring `DZKJ 电子科技_56635.eps` cleanly upon decryption.
2. Confirm `EncryptionManager.md` is present in `QuarkMeta Architecture/Implementation Plan/`.
