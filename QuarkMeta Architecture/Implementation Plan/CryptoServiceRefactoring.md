# Implementation Plan: CryptoServiceRefactoring.md

## 1. Overview
本方案旨在彻底解决【外壳保护】（加密、解密、修改密码）在 `ContentContextMenu.cpp` 中硬编码内联 150+ 行异步线程及弹窗交互的架构违规问题，并修复加解密完成后调用 `loadDirectory` 破坏视角/状态的 SSOT 违规：
1. **解耦与下沉**：在 `EncryptionManager` 中增加异步批量处理接口 `encryptBatchAsync`、`decryptBatchAsync`、`changePasswordBatchAsync`，将后台线程池管理与元数据更新从菜单控制器下沉到 `EncryptionManager`；
2. **SSOT 归一化**：加解密完成后，统一通过回调或主线程槽函数调用 `m_panel->refreshAll()` 进行原位刷新，彻底消除 `loadDirectory` 另起炉灶问题；
3. **菜单瘦身**：`ContentContextMenu.cpp` 中的 `ActionEncrypt`, `ActionDecrypt`, `ActionChangePwd` 分支精简为仅负责搜集用户输入与调用 `EncryptionManager` 对应的服务接口。

## 2. Modified Files List
- `src/crypto/EncryptionManager.h`
- `src/crypto/EncryptionManager.cpp`
- `src/ui/controllers/ContentContextMenu.cpp`

## 3. Detailed Line-by-Line Changes

### `src/crypto/EncryptionManager.h`

```
<<<<<<< SEARCH
    /**
     * @brief 解密文件并保存至指定物理路径
     */
    bool decryptFile(const std::wstring& amencPath, const std::wstring& destPath, const std::string& password);
=======
    /**
     * @brief 解密文件并保存至指定物理路径
     */
    bool decryptFile(const std::wstring& amencPath, const std::wstring& destPath, const std::string& password);

    /**
     * @brief 异步批量加密文件
     */
    void encryptBatchAsync(const QStringList& targets, const std::string& password, std::function<void(bool success)> onFinished);

    /**
     * @brief 异步批量解密文件
     */
    void decryptBatchAsync(const QStringList& targets, const std::string& password, std::function<void(bool anySuccess)> onFinished);

    /**
     * @brief 异步批量修改密码
     */
    void changePasswordBatchAsync(const QStringList& targets, const std::string& oldPassword, const std::string& newPassword, std::function<void(bool anySuccess)> onFinished);
>>>>>>> REPLACE
```

### `src/crypto/EncryptionManager.cpp`

```
<<<<<<< SEARCH
#include "EncryptionManager.h"
#include <fstream>
#include <algorithm>
#include <random>

namespace QuarkMeta {
=======
#include "EncryptionManager.h"
#include "../meta/MetadataManager.h"
#include <fstream>
#include <algorithm>
#include <random>
#include <QThreadPool>
#include <QCoreApplication>
#include <QMetaObject>
#include <QDir>
#include <QFile>

namespace QuarkMeta {

void EncryptionManager::encryptBatchAsync(const QStringList& targets, const std::string& password, std::function<void(bool success)> onFinished) {
    (void)QThreadPool::globalInstance()->start([targets, password, onFinished]() {
        bool allOk = true;
        for (const QString& src : targets) {
            QString dest = src + ".amenc";
            if (EncryptionManager::instance().encryptFile(src.toStdWString(), dest.toStdWString(), password)) {
                QFile::remove(src);
                MetadataManager::instance().setEncrypted(dest.toStdWString(), true);
            } else {
                allOk = false;
            }
        }
        QMetaObject::invokeMethod(QCoreApplication::instance(), [onFinished, allOk]() {
            if (onFinished) onFinished(allOk);
        });
    });
}

void EncryptionManager::decryptBatchAsync(const QStringList& targets, const std::string& password, std::function<void(bool anySuccess)> onFinished) {
    (void)QThreadPool::globalInstance()->start([targets, password, onFinished]() {
        bool anySuccess = false;
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

            if (EncryptionManager::instance().decryptFile(src.toStdWString(), dest.toStdWString(), password)) {
                QFile::remove(src);
                MetadataManager::instance().setEncrypted(dest.toStdWString(), false);
                anySuccess = true;
            }
        }
        QMetaObject::invokeMethod(QCoreApplication::instance(), [onFinished, anySuccess]() {
            if (onFinished) onFinished(anySuccess);
        });
    });
}

void EncryptionManager::changePasswordBatchAsync(const QStringList& targets, const std::string& oldPassword, const std::string& newPassword, std::function<void(bool anySuccess)> onFinished) {
    (void)QThreadPool::globalInstance()->start([targets, oldPassword, newPassword, onFinished]() {
        bool anySuccess = false;
        for (const QString& src : targets) {
            QString tempPlain = src + ".tmp_dec";
            if (EncryptionManager::instance().decryptFile(src.toStdWString(), tempPlain.toStdWString(), oldPassword)) {
                if (EncryptionManager::instance().encryptFile(tempPlain.toStdWString(), src.toStdWString(), newPassword)) {
                    QFile::remove(tempPlain);
                    anySuccess = true;
                } else {
                    QFile::remove(tempPlain);
                }
            }
        }
        QMetaObject::invokeMethod(QCoreApplication::instance(), [onFinished, anySuccess]() {
            if (onFinished) onFinished(anySuccess);
        });
    });
}
>>>>>>> REPLACE
```

### `src/ui/controllers/ContentContextMenu.cpp`

```
<<<<<<< SEARCH
        case ContentPanel::ActionEncrypt: {
            FramelessInputDialog dlg("加密保护", "设置加密密码:", "", m_panel);
            dlg.setEchoMode(QLineEdit::Password);
            if (dlg.exec() == QDialog::Accepted) {
                QString pwd = dlg.text();
                if (pwd.isEmpty()) break;
                auto indexes = view->selectionModel()->selectedIndexes();
                QStringList targets;
                for (const auto& idx : indexes) if (idx.column() == 0) targets << idx.data(PathRole).toString();

                ToolTipOverlay::instance()->showText(QCursor::pos(), "加密任务已在后台启动...", 2000);

                std::string stdPwd = pwd.toStdString();
                QPointer<ContentPanel> self(m_panel);
                QString curDir = currentPath;

                (void)QThreadPool::globalInstance()->start([self, targets, stdPwd, curDir]() {
                    for (const QString& src : targets) {
                        QString dest = src + ".amenc";
                        if (EncryptionManager::instance().encryptFile(src.toStdWString(), dest.toStdWString(), stdPwd)) {
                            QFile::remove(src);
                            MetadataManager::instance().setEncrypted(dest.toStdWString(), true);
                        }
                    }
                    QMetaObject::invokeMethod(QCoreApplication::instance(), [self, curDir]() {
                        if (self && self->currentPath() == curDir) self->loadDirectory(curDir, self->isRecursive());
                        ToolTipOverlay::instance()->showText(QCursor::pos(), "加密任务处理完成", 1500, QColor("#2ecc71"));
                    });
                });
            }
            break;
        }
        case ContentPanel::ActionDecrypt: {
            FramelessInputDialog dlg("解除外壳保护", "输入解密密码:", "", m_panel);
            dlg.setEchoMode(QLineEdit::Password);
            if (dlg.exec() == QDialog::Accepted) {
                QString pwd = dlg.text();
                if (pwd.isEmpty()) break;
                auto indexes = view->selectionModel()->selectedIndexes();
                QStringList targets;
                for (const auto& idx : indexes) if (idx.column() == 0) targets << idx.data(PathRole).toString();

                ToolTipOverlay::instance()->showText(QCursor::pos(), "解密还原任务已在后台启动...", 2000);

                std::string stdPwd = pwd.toStdString();
                QPointer<ContentPanel> self(m_panel);
                QString curDir = currentPath;

                (void)QThreadPool::globalInstance()->start([self, targets, stdPwd, curDir]() {
                    bool anySuccess = false;
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

                    QMetaObject::invokeMethod(QCoreApplication::instance(), [self, curDir, anySuccess]() {
                        if (self && self->currentPath() == curDir) self->loadDirectory(curDir, self->isRecursive());
                        if (anySuccess) {
                            ToolTipOverlay::instance()->showText(QCursor::pos(), "解除保护成功，文件已还原", 1500, QColor("#2ecc71"));
                        } else {
                            ToolTipOverlay::instance()->showText(QCursor::pos(), "解密失败，请检查密码是否正确", 2000, QColor("#e74c3c"));
                        }
                    });
                });
            }
            break;
        }
        case ContentPanel::ActionChangePwd: {
            FramelessInputDialog dlgOld("修改保护密码", "输入原密码:", "", m_panel);
            dlgOld.setEchoMode(QLineEdit::Password);
            if (dlgOld.exec() != QDialog::Accepted || dlgOld.text().isEmpty()) break;
            QString oldPwd = dlgOld.text();

            FramelessInputDialog dlgNew("修改保护密码", "输入新密码:", "", m_panel);
            dlgNew.setEchoMode(QLineEdit::Password);
            if (dlgNew.exec() != QDialog::Accepted || dlgNew.text().isEmpty()) break;
            QString newPwd = dlgNew.text();

            auto indexes = view->selectionModel()->selectedIndexes();
            QStringList targets;
            for (const auto& idx : indexes) if (idx.column() == 0) targets << idx.data(PathRole).toString();

            ToolTipOverlay::instance()->showText(QCursor::pos(), "密码修改中...", 2000);

            std::string stdOldPwd = oldPwd.toStdString();
            std::string stdNewPwd = newPwd.toStdString();
            QPointer<ContentPanel> self(m_panel);
            QString curDir = currentPath;

            (void)QThreadPool::globalInstance()->start([self, targets, stdOldPwd, stdNewPwd, curDir]() {
                bool anySuccess = false;
                for (const QString& src : targets) {
                    QString tempPlain = src + ".tmp_dec";
                    if (EncryptionManager::instance().decryptFile(src.toStdWString(), tempPlain.toStdWString(), stdOldPwd)) {
                        if (EncryptionManager::instance().encryptFile(tempPlain.toStdWString(), src.toStdWString(), stdNewPwd)) {
                            QFile::remove(tempPlain);
                            anySuccess = true;
                        } else {
                            QFile::remove(tempPlain);
                        }
                    }
                }

                QMetaObject::invokeMethod(QCoreApplication::instance(), [self, curDir, anySuccess]() {
                    if (self && self->currentPath() == curDir) self->loadDirectory(curDir, self->isRecursive());
                    if (anySuccess) {
                        ToolTipOverlay::instance()->showText(QCursor::pos(), "保护密码修改成功", 1500, QColor("#2ecc71"));
                    } else {
                        ToolTipOverlay::instance()->showText(QCursor::pos(), "原密码错误，修改失败", 2000, QColor("#e74c3c"));
                    }
                });
            });
            break;
        }
=======
        case ContentPanel::ActionEncrypt: {
            FramelessInputDialog dlg("加密保护", "设置加密密码:", "", m_panel);
            dlg.setEchoMode(QLineEdit::Password);
            if (dlg.exec() == QDialog::Accepted) {
                QString pwd = dlg.text();
                if (pwd.isEmpty()) break;
                auto indexes = view->selectionModel()->selectedIndexes();
                QStringList targets;
                for (const auto& idx : indexes) if (idx.column() == 0) targets << idx.data(PathRole).toString();

                ToolTipOverlay::instance()->showText(QCursor::pos(), "加密任务已在后台启动...", 2000);

                QPointer<ContentPanel> self(m_panel);
                EncryptionManager::instance().encryptBatchAsync(targets, pwd.toStdString(), [self](bool success) {
                    if (self) self->refreshAll();
                    ToolTipOverlay::instance()->showText(QCursor::pos(), success ? "加密任务处理完成" : "部分项目加密失败", 1500, success ? QColor("#2ecc71") : QColor("#e81123"));
                });
            }
            break;
        }
        case ContentPanel::ActionDecrypt: {
            FramelessInputDialog dlg("解除外壳保护", "输入解密密码:", "", m_panel);
            dlg.setEchoMode(QLineEdit::Password);
            if (dlg.exec() == QDialog::Accepted) {
                QString pwd = dlg.text();
                if (pwd.isEmpty()) break;
                auto indexes = view->selectionModel()->selectedIndexes();
                QStringList targets;
                for (const auto& idx : indexes) if (idx.column() == 0) targets << idx.data(PathRole).toString();

                ToolTipOverlay::instance()->showText(QCursor::pos(), "解密还原任务已在后台启动...", 2000);

                QPointer<ContentPanel> self(m_panel);
                EncryptionManager::instance().decryptBatchAsync(targets, pwd.toStdString(), [self](bool anySuccess) {
                    if (self) self->refreshAll();
                    if (anySuccess) {
                        ToolTipOverlay::instance()->showText(QCursor::pos(), "解除保护成功，文件已还原", 1500, QColor("#2ecc71"));
                    } else {
                        ToolTipOverlay::instance()->showText(QCursor::pos(), "解密失败，请检查密码是否正确", 2000, QColor("#e74c3c"));
                    }
                });
            }
            break;
        }
        case ContentPanel::ActionChangePwd: {
            FramelessInputDialog dlgOld("修改保护密码", "输入原密码:", "", m_panel);
            dlgOld.setEchoMode(QLineEdit::Password);
            if (dlgOld.exec() != QDialog::Accepted || dlgOld.text().isEmpty()) break;
            QString oldPwd = dlgOld.text();

            FramelessInputDialog dlgNew("修改保护密码", "输入新密码:", "", m_panel);
            dlgNew.setEchoMode(QLineEdit::Password);
            if (dlgNew.exec() != QDialog::Accepted || dlgNew.text().isEmpty()) break;
            QString newPwd = dlgNew.text();

            auto indexes = view->selectionModel()->selectedIndexes();
            QStringList targets;
            for (const auto& idx : indexes) if (idx.column() == 0) targets << idx.data(PathRole).toString();

            ToolTipOverlay::instance()->showText(QCursor::pos(), "密码修改中...", 2000);

            QPointer<ContentPanel> self(m_panel);
            EncryptionManager::instance().changePasswordBatchAsync(targets, oldPwd.toStdString(), newPwd.toStdString(), [self](bool anySuccess) {
                if (self) self->refreshAll();
                if (anySuccess) {
                    ToolTipOverlay::instance()->showText(QCursor::pos(), "保护密码修改成功", 1500, QColor("#2ecc71"));
                } else {
                    ToolTipOverlay::instance()->showText(QCursor::pos(), "原密码错误，修改失败", 2000, QColor("#e74c3c"));
                }
            });
            break;
        }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. 编译验证：`cmake --build build`
2. 运行应用，选中单个或多个文件选择【外壳保护】->【执行外壳保护】，输入密码；
3. 校验完成后，界面通过 `refreshAll()` 刷出新 `.amenc` 文件，当前展开列/视角保持不动（原位刷新成功，不归零）；
4. 测试解密与修改密码功能，校验线程安全与提示正常。

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **是否复用既有 SSOT 接口**：是。统一复用了原位刷新唯一入口 `ContentPanel::refreshAll()`，彻底消除了加解密完成后调用 `loadDirectory` 另起炉灶的违规点；
- **是否消除逻辑冗余**：是。将 150+ 行硬编码异步逻辑完全从 UI 菜单类抽取下沉至 `EncryptionManager` 基础设施服务中。

## 6. Header API Signature Verification
- `EncryptionManager::encryptBatchAsync(const QStringList& targets, const std::string& password, std::function<void(bool success)> onFinished)`
- `EncryptionManager::decryptBatchAsync(const QStringList& targets, const std::string& password, std::function<void(bool anySuccess)> onFinished)`
- `EncryptionManager::changePasswordBatchAsync(const QStringList& targets, const std::string& oldPassword, const std::string& newPassword, std::function<void(bool anySuccess)> onFinished)`
  - 物理源头：`src/crypto/EncryptionManager.h`

## 7. Header Inclusion Chain & Type Completeness Check
- `ContentContextMenu.cpp` 已包含 `#include "../../crypto/EncryptionManager.h"`；
- `EncryptionManager.cpp` 补齐 `#include "../meta/MetadataManager.h"`、`#include <QThreadPool>`、`#include <QMetaObject>`、`#include <QCoreApplication>`，头文件闭合完整。
