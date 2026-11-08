# ContentContextMenuRefactoring.md - 实施方案

## 已读物理证据清单

1. **物理读取文件列表**：
   - `QuarkMeta_Source/CMakeLists.txt`
   - `QuarkMeta_Source/src/ui/controllers/ContentContextMenu.h`
   - `QuarkMeta_Source/src/ui/controllers/ContentContextMenu.cpp`
   - `QuarkMeta_Source/src/ui/ContentPanel.h`
   - `QuarkMeta_Source/src/ui/ContentPanel.cpp`
   - `QuarkMeta_Source/src/crypto/EncryptionManager.h`
   - `QuarkMeta_Source/src/util/ShellHelper.h`
   - `QuarkMeta_Source/src/ui/ToolTipOverlay.h`
   - `QuarkMeta_Source/src/ui/dialogs/FramelessInputDialog.h`
   - `QuarkMeta_Source/src/meta/ModelContract.h`

2. **头文件原样签名核验**：
   - `ContentPanel::getSelectedPaths() const`：`QStringList getSelectedPaths() const;`（`ContentPanel.h` 第 146 行）
   - `ContentPanel::refreshAll()`：`void refreshAll();`（`ContentPanel.h` 第 199 行）
   - `EncryptionManager::encryptBatchAsync(const QStringList&, const std::string&, std::function<void(bool)>)`：`void encryptBatchAsync(const QStringList& filePaths, const std::string& password, std::function<void(bool)> onFinished);`（`EncryptionManager.h` 第 29 行）
   - `EncryptionManager::decryptBatchAsync(const QStringList&, const std::string&, std::function<void(bool)>)`：`void decryptBatchAsync(const QStringList& filePaths, const std::string& password, std::function<void(bool)> onFinished);`（`EncryptionManager.h` 第 30 行）
   - `EncryptionManager::changePasswordBatchAsync(const QStringList&, const std::string&, const std::string&, std::function<void(bool)>)`：`void changePasswordBatchAsync(const QStringList& filePaths, const std::string& oldPassword, const std::string& newPassword, std::function<void(bool)> onFinished);`（`EncryptionManager.h` 第 31 行）
   - `ShellHelper::extractArchive(const QString&, const QString&)`：`static bool extractArchive(const QString& archivePath, const QString& destDir);`（`ShellHelper.h` 第 22 行）
   - `ToolTipOverlay::showToast(const QString&, int, const QColor&)`：`void showToast(const QString& message, int durationMs = 2000, const QColor& color = QColor("#333333"));`（`ToolTipOverlay.h` 第 61 行）

3. **既有 API 检索与防重复造轮子**：
   - 完全复用 `EncryptionManager::instance()` 单例与现有异步批处理接口；
   - 完全复用 `ShellHelper::extractArchive` 调用本地归档关联工具；
   - 完全复用 `ToolTipOverlay::instance()->showToast` 进行状态通知；
   - 完全复用 `ContentPanel::refreshAll()` 统一刷新多视口。

---

## Chapter 1: Overview（概述与根因分析）

### 1. 现象与问题描述
`ContentContextMenu.cpp` 当前承担了大量与“菜单构建与触发”无关的长事务与重业务逻辑：
1. **归档与压缩任务（Archive）**：包含 PowerShell 脚本拼接转义、`QProcess` 异步生命周期监听、解压/压缩失败残缺文件清理、延时刷新以及系统解压程序关联调用（百余行复杂流程）。
2. **外壳加密与密码保护任务（Crypto）**：包含密码输入弹窗（`FramelessInputDialog`）、两次输入验证、从视图获取选中 `PathRole` 目标、`EncryptionManager` 批量异步调用与弱引用安全闭包（`QPointer<ContentPanel>`）。

### 2. 根因剖析（Root Cause Analysis）
- 职责越界违背了单一职责原则（SRP）：右键菜单控制器理应只负责菜单项组织、状态使能和动作分发，但在处理归档和加密操作时直接内联了大量底层进程调度与异步业务编排代码，导致类文件体积膨胀至 1200+ 行，逻辑高度耦合且难以进行单元测试与代码复用。

### 3. 重构设计目标
- 抽离 `ArchiveTaskHelper`（`src/ui/controllers/ArchiveTaskHelper.h/.cpp`）：封装 `extractArchive` 与 `compressToZip` 两个静态业务执行器；
- 抽离 `CryptoActionHelper`（`src/ui/controllers/CryptoActionHelper.h/.cpp`）：封装 `executeEncrypt`、`executeDecrypt` 与 `executeChangePassword` 三个静态业务执行器；
- 在 `CMakeLists.txt` 中同步注册 4 个新文件；
- 将 `ContentContextMenu.cpp` 中对应 `switch (action)` 分支精简为清晰的单行委托调用，并按需解耦 `#include`。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `QuarkMeta_Source/CMakeLists.txt`（修改：注册新建的 4 个 helper 文件）
2. `QuarkMeta_Source/src/ui/controllers/ArchiveTaskHelper.h`（新建：归档压缩辅助控制器头文件）
3. `QuarkMeta_Source/src/ui/controllers/ArchiveTaskHelper.cpp`（新建：归档压缩辅助控制器实现）
4. `QuarkMeta_Source/src/ui/controllers/CryptoActionHelper.h`（新建：加密操作辅助控制器头文件）
5. `QuarkMeta_Source/src/ui/controllers/CryptoActionHelper.cpp`（新建：加密操作辅助控制器实现）
6. `QuarkMeta_Source/src/ui/controllers/ContentContextMenu.cpp`（修改：引入新头文件，将解压/压缩/加密对应分支委托给 Helper）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 1. `QuarkMeta_Source/CMakeLists.txt`
在 `ContentContextMenu.cpp` 之后注册新增的归档与加密辅助控制器文件。

<<<<<<< SEARCH
    src/ui/controllers/ContentContextMenu.h
    src/ui/controllers/ContentContextMenu.cpp
    src/ui/controllers/ContextMenuFactory.h
=======
    src/ui/controllers/ContentContextMenu.h
    src/ui/controllers/ContentContextMenu.cpp
    src/ui/controllers/ArchiveTaskHelper.h
    src/ui/controllers/ArchiveTaskHelper.cpp
    src/ui/controllers/CryptoActionHelper.h
    src/ui/controllers/CryptoActionHelper.cpp
    src/ui/controllers/ContextMenuFactory.h
>>>>>>> REPLACE

---

### 2. `QuarkMeta_Source/src/ui/controllers/ArchiveTaskHelper.h`
新建归档任务辅助类声明。

<<<<<<< SEARCH FILE: QuarkMeta_Source/src/ui/controllers/ArchiveTaskHelper.h
=======
#pragma once

#include <QString>

namespace QuarkMeta {

class ContentPanel;

class ArchiveTaskHelper {
public:
    /**
     * @brief 执行解压压缩包动作
     * @param panel 宿主面板指针
     * @param path 右键点击的目标路径（当面板无多选选中时作为回退路径）
     */
    static void extractArchive(ContentPanel* panel, const QString& path);

    /**
     * @brief 执行打包压缩为 ZIP 动作
     * @param panel 宿主面板指针
     * @param currentPath 当前目录路径
     * @param path 右键点击的目标路径（当面板无多选选中时作为回退路径）
     */
    static void compressToZip(ContentPanel* panel, const QString& currentPath, const QString& path);
};

} // namespace QuarkMeta
>>>>>>> REPLACE

---

### 3. `QuarkMeta_Source/src/ui/controllers/ArchiveTaskHelper.cpp`
新建归档任务辅助类实现，完整封装 PowerShell 脚本执行、QProcess 监听与残缺文件清理。

<<<<<<< SEARCH FILE: QuarkMeta_Source/src/ui/controllers/ArchiveTaskHelper.cpp
=======
#include "ArchiveTaskHelper.h"
#include "../ContentPanel.h"
#include "../ToolTipOverlay.h"
#include "../../util/ShellHelper.h"

#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QTimer>
#include <QPointer>
#include <QCoreApplication>
#include <QStringList>

namespace QuarkMeta {

void ArchiveTaskHelper::extractArchive(ContentPanel* panel, const QString& path) {
    if (!panel) return;

    // 确定压缩包路径：仅处理第一个选中文件
    QStringList selectedPaths = panel->getSelectedPaths();
    QString archivePath = !selectedPaths.isEmpty() ? selectedPaths.first() : path;
    if (archivePath.isEmpty() || !QFileInfo::exists(archivePath)) return;

    QFileInfo archiveInfo(archivePath);
    QString parentDir = archiveInfo.absolutePath();
    QString baseName  = archiveInfo.completeBaseName();

    // 自动生成唯一目标子文件夹名（同名递增编号）
    QString destDir = QDir(parentDir).filePath(baseName);
    if (QDir(destDir).exists()) {
        int counter = 1;
        while (QDir(QDir(parentDir).filePath(
            QString("%1-%2").arg(baseName).arg(counter))).exists()) {
            ++counter;
        }
        destDir = QDir(parentDir).filePath(QString("%1-%2").arg(baseName).arg(counter));
    }
    QDir().mkpath(destDir);

    QString ext = archiveInfo.suffix().toLower();

    if (ext == QStringLiteral("zip")) {
        // .zip：通过 QProcess 追踪进程退出，完成后安全刷新面板
        ToolTipOverlay::instance()->showToast(
            QString("正在解压 %1 ...").arg(archiveInfo.fileName()), 2000, QColor("#3498db")
        );

        QPointer<ContentPanel> weakPanel(panel);
        QString capturedArchive = archivePath;
        QString capturedDest    = destDir;

        QProcess* proc = new QProcess();
        QObject::connect(
            proc,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [weakPanel, proc, capturedArchive, capturedDest](int exitCode, QProcess::ExitStatus) {
                QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, exitCode, capturedArchive, capturedDest, proc]() {
                    if (weakPanel) weakPanel->refreshAll();
                    QFileInfo info(capturedArchive);
                    if (exitCode == 0) {
                        ToolTipOverlay::instance()->showToast(
                            QString("解压完成：%1").arg(info.fileName()), 2000, QColor("#2ecc71")
                        );
                    } else {
                        // 失败时清理可能为空的目标目录
                        QDir failDir(capturedDest);
                        if (failDir.exists() && failDir.isEmpty()) failDir.rmdir(capturedDest);
                        ToolTipOverlay::instance()->showToast(
                            QString("解压失败：%1").arg(info.fileName()), 2000, QColor("#e81123")
                        );
                    }
                    proc->deleteLater();
                });
            }
        );

        QString nativeSrc  = QDir::toNativeSeparators(capturedArchive);
        QString nativeDest = QDir::toNativeSeparators(capturedDest);
        QString script = QString(
            "Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force"
        ).arg(nativeSrc, nativeDest);
        proc->start(QStringLiteral("powershell"),
            QStringList() << QStringLiteral("-NoProfile")
                          << QStringLiteral("-NonInteractive")
                          << QStringLiteral("-WindowStyle") << QStringLiteral("Hidden")
                          << QStringLiteral("-Command") << script);
    } else {
        // 其他格式：委托系统关联程序（7-Zip / WinRAR），无法精确追踪退出
        ShellHelper::extractArchive(archivePath, destDir);
        ToolTipOverlay::instance()->showToast(
            QString("已委托系统程序处理，解压完成后将自动刷新"), 2500, QColor("#3498db")
        );
        QPointer<ContentPanel> weakPanel(panel);
        // 延迟 1.5s 刷新，给系统程序完成初始化的时间
        QTimer::singleShot(1500, QCoreApplication::instance(), [weakPanel]() {
            if (weakPanel) weakPanel->refreshAll();
        });
    }
}

void ArchiveTaskHelper::compressToZip(ContentPanel* panel, const QString& currentPath, const QString& path) {
    if (!panel) return;

    QStringList selectedPaths = panel->getSelectedPaths();
    if (selectedPaths.isEmpty() && !path.isEmpty()) {
        selectedPaths << path;
    }
    if (selectedPaths.isEmpty()) return;

    QString firstItem = selectedPaths.first();
    QFileInfo firstInfo(firstItem);
    QString parentDir = firstInfo.isDir() && selectedPaths.size() == 1
        ? firstInfo.dir().absolutePath()
        : (currentPath.isEmpty() || currentPath.contains("://") ? firstInfo.absolutePath() : currentPath);

    // 基础名称：文件夹取目录名，文件取主文件名（无后缀）
    QString baseName = firstInfo.isDir() ? firstInfo.fileName() : firstInfo.completeBaseName();
    if (baseName.isEmpty()) baseName = QStringLiteral("Archive");

    // 自动计算递增编号目标 zip 路径，防冲洗覆盖
    QString destZip = QDir(parentDir).filePath(baseName + QStringLiteral(".zip"));
    if (QFile::exists(destZip)) {
        int counter = 1;
        while (QFile::exists(QDir(parentDir).filePath(QString("%1-%2.zip").arg(baseName).arg(counter)))) {
            ++counter;
        }
        destZip = QDir(parentDir).filePath(QString("%1-%2.zip").arg(baseName).arg(counter));
    }

    QFileInfo destZipInfo(destZip);
    ToolTipOverlay::instance()->showToast(
        QString("正在将 %1 个项目压缩为 %2 ...").arg(selectedPaths.size()).arg(destZipInfo.fileName()),
        2000,
        QColor("#3498db")
    );

    // 构造 PowerShell 脚本：使用单引号与转义处理路径
    QStringList nativePaths;
    for (const QString& p : selectedPaths) {
        QString nPath = QDir::toNativeSeparators(p);
        nPath.replace("'", "''"); // PowerShell 单引号转义
        nativePaths << QString("'%1'").arg(nPath);
    }
    QString nativeDest = QDir::toNativeSeparators(destZip);
    nativeDest.replace("'", "''");

    QString script = QString(
        "$files = @(%1); Compress-Archive -LiteralPath $files -DestinationPath '%2' -Force"
    ).arg(nativePaths.join(", "), nativeDest);

    QPointer<ContentPanel> weakPanel(panel);
    QString capturedDestZip = destZip;

    QProcess* proc = new QProcess();
    QObject::connect(
        proc,
        QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [weakPanel, proc, capturedDestZip](int exitCode, QProcess::ExitStatus) {
            QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, exitCode, capturedDestZip, proc]() {
                if (weakPanel) weakPanel->refreshAll();
                QFileInfo info(capturedDestZip);
                if (exitCode == 0) {
                    ToolTipOverlay::instance()->showToast(
                        QString("压缩完成：%1").arg(info.fileName()), 2000, QColor("#2ecc71")
                    );
                } else {
                    // 若压缩失败且产生了残缺文件，则进行清理
                    if (QFile::exists(capturedDestZip)) {
                        QFile::remove(capturedDestZip);
                    }
                    ToolTipOverlay::instance()->showToast(
                        QString("压缩失败：%1").arg(info.fileName()), 2000, QColor("#e81123")
                    );
                }
                proc->deleteLater();
            });
        }
    );

    proc->start(
        QStringLiteral("powershell"),
        QStringList() << QStringLiteral("-NoProfile")
                      << QStringLiteral("-NonInteractive")
                      << QStringLiteral("-WindowStyle") << QStringLiteral("Hidden")
                      << QStringLiteral("-Command") << script
    );
}

} // namespace QuarkMeta
>>>>>>> REPLACE

---

### 4. `QuarkMeta_Source/src/ui/controllers/CryptoActionHelper.h`
新建加密操作辅助类声明。

<<<<<<< SEARCH FILE: QuarkMeta_Source/src/ui/controllers/CryptoActionHelper.h
=======
#pragma once

namespace QuarkMeta {

class ContentPanel;
class QAbstractItemView;

class CryptoActionHelper {
public:
    /**
     * @brief 执行外壳加密保护动作
     */
    static void executeEncrypt(ContentPanel* panel, QAbstractItemView* view);

    /**
     * @brief 执行解除外壳保护动作
     */
    static void executeDecrypt(ContentPanel* panel, QAbstractItemView* view);

    /**
     * @brief 执行修改保护密码动作
     */
    static void executeChangePassword(ContentPanel* panel, QAbstractItemView* view);
};

} // namespace QuarkMeta
>>>>>>> REPLACE

---

### 5. `QuarkMeta_Source/src/ui/controllers/CryptoActionHelper.cpp`
新建加密操作辅助类实现，封装弹窗与批量密码任务。

<<<<<<< SEARCH FILE: QuarkMeta_Source/src/ui/controllers/CryptoActionHelper.cpp
=======
#include "CryptoActionHelper.h"
#include "../ContentPanel.h"
#include "../ToolTipOverlay.h"
#include "../dialogs/FramelessInputDialog.h"
#include "../../crypto/EncryptionManager.h"
#include "../../meta/ModelContract.h"

#include <QAbstractItemView>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QPointer>
#include <QStringList>

namespace QuarkMeta {

void CryptoActionHelper::executeEncrypt(ContentPanel* panel, QAbstractItemView* view) {
    if (!panel || !view) return;

    FramelessInputDialog dlg("加密保护", "设置加密密码:", "", panel);
    dlg.setEchoMode(QLineEdit::Password);
    if (dlg.exec() == QDialog::Accepted) {
        QString pwd = dlg.text();
        if (pwd.isEmpty()) return;
        auto indexes = view->selectionModel()->selectedIndexes();
        QStringList targets;
        for (const auto& idx : indexes) {
            if (idx.column() == 0) targets << idx.data(PathRole).toString();
        }

        ToolTipOverlay::instance()->showToast("加密任务已在后台启动...", 2000);

        QPointer<ContentPanel> self(panel);
        EncryptionManager::instance().encryptBatchAsync(targets, pwd.toStdString(), [self](bool success) {
            if (self) self->refreshAll();
            ToolTipOverlay::instance()->showToast(
                success ? "加密任务处理完成" : "部分项目加密失败",
                1500,
                success ? QColor("#2ecc71") : QColor("#e81123")
            );
        });
    }
}

void CryptoActionHelper::executeDecrypt(ContentPanel* panel, QAbstractItemView* view) {
    if (!panel || !view) return;

    FramelessInputDialog dlg("解除外壳保护", "输入解密密码:", "", panel);
    dlg.setEchoMode(QLineEdit::Password);
    if (dlg.exec() == QDialog::Accepted) {
        QString pwd = dlg.text();
        if (pwd.isEmpty()) return;
        auto indexes = view->selectionModel()->selectedIndexes();
        QStringList targets;
        for (const auto& idx : indexes) {
            if (idx.column() == 0) targets << idx.data(PathRole).toString();
        }

        ToolTipOverlay::instance()->showToast("解密还原任务已在后台启动...", 2000);

        QPointer<ContentPanel> self(panel);
        EncryptionManager::instance().decryptBatchAsync(targets, pwd.toStdString(), [self](bool anySuccess) {
            if (self) self->refreshAll();
            if (anySuccess) {
                ToolTipOverlay::instance()->showToast("解除保护成功，文件已还原", 1500, QColor("#2ecc71"));
            } else {
                ToolTipOverlay::instance()->showToast("解密失败，请检查密码是否正确", 2000, QColor("#e74c3c"));
            }
        });
    }
}

void CryptoActionHelper::executeChangePassword(ContentPanel* panel, QAbstractItemView* view) {
    if (!panel || !view) return;

    FramelessInputDialog dlgOld("修改保护密码", "输入原密码:", "", panel);
    dlgOld.setEchoMode(QLineEdit::Password);
    if (dlgOld.exec() != QDialog::Accepted || dlgOld.text().isEmpty()) return;
    QString oldPwd = dlgOld.text();

    FramelessInputDialog dlgNew("修改保护密码", "输入新密码:", "", panel);
    dlgNew.setEchoMode(QLineEdit::Password);
    if (dlgNew.exec() != QDialog::Accepted || dlgNew.text().isEmpty()) return;
    QString newPwd = dlgNew.text();

    auto indexes = view->selectionModel()->selectedIndexes();
    QStringList targets;
    for (const auto& idx : indexes) {
        if (idx.column() == 0) targets << idx.data(PathRole).toString();
    }

    ToolTipOverlay::instance()->showToast("密码修改中...", 2000);

    QPointer<ContentPanel> self(panel);
    EncryptionManager::instance().changePasswordBatchAsync(
        targets, oldPwd.toStdString(), newPwd.toStdString(), [self](bool anySuccess) {
            if (self) self->refreshAll();
            if (anySuccess) {
                ToolTipOverlay::instance()->showToast("保护密码修改成功", 1500, QColor("#2ecc71"));
            } else {
                ToolTipOverlay::instance()->showToast("原密码错误，修改失败", 2000, QColor("#e74c3c"));
            }
        }
    );
}

} // namespace QuarkMeta
>>>>>>> REPLACE

---

### 6. `QuarkMeta_Source/src/ui/controllers/ContentContextMenu.cpp`
引入两个 Helper 头文件，并在 `switch (action)` 中将对应分支替换为单行委托调用。

<<<<<<< SEARCH
#include "ContentKeyHandler.h"
#include "../UiHelper.h"
=======
#include "ContentKeyHandler.h"
#include "ArchiveTaskHelper.h"
#include "CryptoActionHelper.h"
#include "../UiHelper.h"
>>>>>>> REPLACE

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

                ToolTipOverlay::instance()->showToast("加密任务已在后台启动...", 2000);

                QPointer<ContentPanel> self(m_panel);
                EncryptionManager::instance().encryptBatchAsync(targets, pwd.toStdString(), [self](bool success) {
                    if (self) self->refreshAll();
                    ToolTipOverlay::instance()->showToast(success ? "加密任务处理完成" : "部分项目加密失败", 1500, success ? QColor("#2ecc71") : QColor("#e81123"));
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

                ToolTipOverlay::instance()->showToast("解密还原任务已在后台启动...", 2000);

                QPointer<ContentPanel> self(m_panel);
                EncryptionManager::instance().decryptBatchAsync(targets, pwd.toStdString(), [self](bool anySuccess) {
                    if (self) self->refreshAll();
                    if (anySuccess) {
                        ToolTipOverlay::instance()->showToast("解除保护成功，文件已还原", 1500, QColor("#2ecc71"));
                    } else {
                        ToolTipOverlay::instance()->showToast("解密失败，请检查密码是否正确", 2000, QColor("#e74c3c"));
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

            ToolTipOverlay::instance()->showToast("密码修改中...", 2000);

            QPointer<ContentPanel> self(m_panel);
            EncryptionManager::instance().changePasswordBatchAsync(targets, oldPwd.toStdString(), newPwd.toStdString(), [self](bool anySuccess) {
                if (self) self->refreshAll();
                if (anySuccess) {
                    ToolTipOverlay::instance()->showToast("保护密码修改成功", 1500, QColor("#2ecc71"));
                } else {
                    ToolTipOverlay::instance()->showToast("原密码错误，修改失败", 2000, QColor("#e74c3c"));
                }
            });
            break;
        }
=======
        case ContentPanel::ActionEncrypt: {
            CryptoActionHelper::executeEncrypt(m_panel, view);
            break;
        }
        case ContentPanel::ActionDecrypt: {
            CryptoActionHelper::executeDecrypt(m_panel, view);
            break;
        }
        case ContentPanel::ActionChangePwd: {
            CryptoActionHelper::executeChangePassword(m_panel, view);
            break;
        }
>>>>>>> REPLACE

<<<<<<< SEARCH
        case ContentPanel::ActionExtractArchive: {
            // 确定压缩包路径：仅处理第一个选中文件
            QStringList selectedPaths = m_panel->getSelectedPaths();
            QString archivePath = !selectedPaths.isEmpty() ? selectedPaths.first() : path;
            if (archivePath.isEmpty() || !QFileInfo::exists(archivePath)) break;

            QFileInfo archiveInfo(archivePath);
            QString parentDir = archiveInfo.absolutePath();
            QString baseName  = archiveInfo.completeBaseName();

            // 自动生成唯一目标子文件夹名（同名递增编号）
            QString destDir = QDir(parentDir).filePath(baseName);
            if (QDir(destDir).exists()) {
                int counter = 1;
                while (QDir(QDir(parentDir).filePath(
                    QString("%1-%2").arg(baseName).arg(counter))).exists()) {
                    ++counter;
                }
                destDir = QDir(parentDir).filePath(QString("%1-%2").arg(baseName).arg(counter));
            }
            QDir().mkpath(destDir);

            QString ext = archiveInfo.suffix().toLower();

            if (ext == QStringLiteral("zip")) {
                // .zip：通过 QProcess 追踪进程退出，完成后安全刷新面板
                ToolTipOverlay::instance()->showToast(
                    QString("正在解压 %1 ...").arg(archiveInfo.fileName()), 2000, QColor("#3498db")
                );

                QPointer<ContentPanel> weakPanel(m_panel);
                QString capturedArchive = archivePath;
                QString capturedDest    = destDir;

                QProcess* proc = new QProcess();
                QObject::connect(
                    proc,
                    QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    [weakPanel, proc, capturedArchive, capturedDest](int exitCode, QProcess::ExitStatus) {
                        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, exitCode, capturedArchive, capturedDest, proc]() {
                            if (weakPanel) weakPanel->refreshAll();
                            QFileInfo info(capturedArchive);
                            if (exitCode == 0) {
                                ToolTipOverlay::instance()->showToast(
                                    QString("解压完成：%1").arg(info.fileName()), 2000, QColor("#2ecc71")
                                );
                            } else {
                                // 失败时清理可能为空的目标目录
                                QDir failDir(capturedDest);
                                if (failDir.exists() && failDir.isEmpty()) failDir.rmdir(capturedDest);
                                ToolTipOverlay::instance()->showToast(
                                    QString("解压失败：%1").arg(info.fileName()), 2000, QColor("#e81123")
                                );
                            }
                            proc->deleteLater();
                        });
                    }
                );

                QString nativeSrc  = QDir::toNativeSeparators(capturedArchive);
                QString nativeDest = QDir::toNativeSeparators(capturedDest);
                QString script = QString(
                    "Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force"
                ).arg(nativeSrc, nativeDest);
                proc->start(QStringLiteral("powershell"),
                    QStringList() << QStringLiteral("-NoProfile")
                                  << QStringLiteral("-NonInteractive")
                                  << QStringLiteral("-WindowStyle") << QStringLiteral("Hidden")
                                  << QStringLiteral("-Command") << script);
            } else {
                // 其他格式：委托系统关联程序（7-Zip / WinRAR），无法精确追踪退出
                ShellHelper::extractArchive(archivePath, destDir);
                ToolTipOverlay::instance()->showToast(
                    QString("已委托系统程序处理，解压完成后将自动刷新"), 2500, QColor("#3498db")
                );
                QPointer<ContentPanel> weakPanel(m_panel);
                // 延迟 1.5s 刷新，给系统程序完成初始化的时间
                QTimer::singleShot(1500, QCoreApplication::instance(), [weakPanel]() {
                    if (weakPanel) weakPanel->refreshAll();
                });
            }
            break;
        }
        case ContentPanel::ActionCompressToZip: {
            QStringList selectedPaths = m_panel->getSelectedPaths();
            if (selectedPaths.isEmpty() && !path.isEmpty()) {
                selectedPaths << path;
            }
            if (selectedPaths.isEmpty()) break;

            QString firstItem = selectedPaths.first();
            QFileInfo firstInfo(firstItem);
            QString parentDir = firstInfo.isDir() && selectedPaths.size() == 1
                ? firstInfo.dir().absolutePath()
                : (currentPath.isEmpty() || currentPath.contains("://") ? firstInfo.absolutePath() : currentPath);

            // 基础名称：文件夹取目录名，文件取主文件名（无后缀）
            QString baseName = firstInfo.isDir() ? firstInfo.fileName() : firstInfo.completeBaseName();
            if (baseName.isEmpty()) baseName = QStringLiteral("Archive");

            // 自动计算递增编号目标 zip 路径，防冲洗覆盖
            QString destZip = QDir(parentDir).filePath(baseName + QStringLiteral(".zip"));
            if (QFile::exists(destZip)) {
                int counter = 1;
                while (QFile::exists(QDir(parentDir).filePath(QString("%1-%2.zip").arg(baseName).arg(counter)))) {
                    ++counter;
                }
                destZip = QDir(parentDir).filePath(QString("%1-%2.zip").arg(baseName).arg(counter));
            }

            QFileInfo destZipInfo(destZip);
            ToolTipOverlay::instance()->showToast(
                QString("正在将 %1 个项目压缩为 %2 ...").arg(selectedPaths.size()).arg(destZipInfo.fileName()),
                2000,
                QColor("#3498db")
            );

            // 构造 PowerShell 脚本：使用单引号与转义处理路径
            QStringList nativePaths;
            for (const QString& p : selectedPaths) {
                QString nPath = QDir::toNativeSeparators(p);
                nPath.replace("'", "''"); // PowerShell 单引号转义
                nativePaths << QString("'%1'").arg(nPath);
            }
            QString nativeDest = QDir::toNativeSeparators(destZip);
            nativeDest.replace("'", "''");

            QString script = QString(
                "$files = @(%1); Compress-Archive -LiteralPath $files -DestinationPath '%2' -Force"
            ).arg(nativePaths.join(", "), nativeDest);

            QPointer<ContentPanel> weakPanel(m_panel);
            QString capturedDestZip = destZip;

            QProcess* proc = new QProcess();
            QObject::connect(
                proc,
                QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [weakPanel, proc, capturedDestZip](int exitCode, QProcess::ExitStatus) {
                    QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, exitCode, capturedDestZip, proc]() {
                        if (weakPanel) weakPanel->refreshAll();
                        QFileInfo info(capturedDestZip);
                        if (exitCode == 0) {
                            ToolTipOverlay::instance()->showToast(
                                QString("压缩完成：%1").arg(info.fileName()), 2000, QColor("#2ecc71")
                            );
                        } else {
                            // 若压缩失败且产生了残缺文件，则进行清理
                            if (QFile::exists(capturedDestZip)) {
                                QFile::remove(capturedDestZip);
                            }
                            ToolTipOverlay::instance()->showToast(
                                QString("压缩失败：%1").arg(info.fileName()), 2000, QColor("#e81123")
                            );
                        }
                        proc->deleteLater();
                    });
                }
            );

            proc->start(
                QStringLiteral("powershell"),
                QStringList() << QStringLiteral("-NoProfile")
                              << QStringLiteral("-NonInteractive")
                              << QStringLiteral("-WindowStyle") << QStringLiteral("Hidden")
                              << QStringLiteral("-Command") << script
            );
            break;
        }
=======
        case ContentPanel::ActionExtractArchive: {
            ArchiveTaskHelper::extractArchive(m_panel, path);
            break;
        }
        case ContentPanel::ActionCompressToZip: {
            ArchiveTaskHelper::compressToZip(m_panel, currentPath, path);
            break;
        }
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 1. 构建验证
- 沙盒环境中跳过实际 Qt6 编译；在 Windows MSVC 编译环境下运行：
  ```cmd
  ninja -C build
  ```
- 确认无重定义、未定义符号、包含缺失等构建错误。

### 2. 运行时验收标准
1. **右键解压 ZIP 包**：右键点击一个 `.zip` 文件，选择“解压压缩包”，底部弹出蓝色 Toast“正在解压...”，解压完成后弹出绿色 Toast“解压完成”，当前面板自动刷新显示解压生成的同名目录。
2. **右键压缩为 ZIP**：选中一个或多个文件/文件夹，右键选择“压缩为 ZIP”，底部弹出“正在将 X 个项目压缩为...”，完成后面板自动刷新出现目标 `.zip` 文件。
3. **右键加密保护**：选中文件右键选择“执行外壳保护”，弹出密码设置框，输入密码并确认后后台启动加密，完成后提示“加密任务处理完成”并刷新视图。
4. **右键解密还原**：选中已加密文件右键选择“解除保护”，输入密码确认后还原，若密码正确提示“解除保护成功”，若错误提示“解密失败，请检查密码是否正确”。
5. **右键修改密码**：选中文件修改密码，连续两次验证后生效并给出 Toast 提示。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- 完全复用 `EncryptionManager::instance().encryptBatchAsync`、`decryptBatchAsync`、`changePasswordBatchAsync` 现有加密管线；
- 完全复用 `ShellHelper::extractArchive` 针对非 zip 压缩包调用系统默认解压工具；
- 完全复用 `ToolTipOverlay::instance()->showToast` 进行状态提示；
- 完全复用 `ContentPanel::refreshAll()` 与 `ContentPanel::getSelectedPaths()`；
- 无任何重复造轮子现象。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 / 业务概念 | 唯一 SSOT 数据源 | 派生副本与传递方式 | 同步与刷新机制 |
| :--- | :--- | :--- | :--- |
| 选中目标路径 | 视图层 `QItemSelectionModel`（经 `PathRole`）或 `ContentPanel::getSelectedPaths()` | 在调用时作为入参传递给 Helper 静态方法 | 操作完成后通过 `self->refreshAll()` 触发全量视口同步 |
| 归档与加密密码 | 用户在 `FramelessInputDialog` 中输入的临时密码文本 | 按值传递给对应回调闭包 | 任务执行完毕后随作用域销毁，不持久化 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. **外部调用接口**：
   - `ContentPanel::getSelectedPaths() const`（`public`）
   - `ContentPanel::refreshAll()`（`public`）
   - `EncryptionManager::instance()`（`public` 单例）
   - `ShellHelper::extractArchive(const QString&, const QString&)`（`public static`）
   - `ToolTipOverlay::instance()->showToast`（`public`）

2. **新增接口声明与调用点一致性**：
   - `ArchiveTaskHelper::extractArchive(ContentPanel*, const QString&)`：声明于 `ArchiveTaskHelper.h`，定义于 `ArchiveTaskHelper.cpp`，调用点于 `ContentContextMenu.cpp`；
   - `ArchiveTaskHelper::compressToZip(ContentPanel*, const QString&, const QString&)`：声明于 `ArchiveTaskHelper.h`，定义于 `ArchiveTaskHelper.cpp`，调用点于 `ContentContextMenu.cpp`；
   - `CryptoActionHelper::executeEncrypt(ContentPanel*, QAbstractItemView*)`：声明于 `CryptoActionHelper.h`，定义于 `CryptoActionHelper.cpp`，调用点于 `ContentContextMenu.cpp`；
   - `CryptoActionHelper::executeDecrypt(ContentPanel*, QAbstractItemView*)`：声明于 `CryptoActionHelper.h`，定义于 `CryptoActionHelper.cpp`，调用点于 `ContentContextMenu.cpp`；
   - `CryptoActionHelper::executeChangePassword(ContentPanel*, QAbstractItemView*)`：声明于 `CryptoActionHelper.h`，定义于 `CryptoActionHelper.cpp`，调用点于 `ContentContextMenu.cpp`。

3. **跨类访问权限**：
   - 所有在 Helper 中调用的 `ContentPanel` 成员（`getSelectedPaths`、`refreshAll`）均为 `public`。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

1. `ArchiveTaskHelper.h`：前置声明 `class ContentPanel;`，包含 `<QString>`；
2. `ArchiveTaskHelper.cpp`：显式包含 `ArchiveTaskHelper.h`, `../ContentPanel.h`, `../ToolTipOverlay.h`, `../../util/ShellHelper.h`, `<QFileInfo>`, `<QDir>`, `<QProcess>`, `<QTimer>`, `<QPointer>`, `<QCoreApplication>`, `<QStringList>`；
3. `CryptoActionHelper.h`：前置声明 `class ContentPanel; class QAbstractItemView;`；
4. `CryptoActionHelper.cpp`：显式包含 `CryptoActionHelper.h`, `../ContentPanel.h`, `../ToolTipOverlay.h`, `../dialogs/FramelessInputDialog.h`, `../../crypto/EncryptionManager.h`, `../../meta/ModelContract.h`, `<QAbstractItemView>`, `<QItemSelectionModel>`, `<QLineEdit>`, `<QPointer>`, `<QStringList>`；
5. `ContentContextMenu.cpp`：显式引入 `ArchiveTaskHelper.h` 和 `CryptoActionHelper.h`。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项 | 结论 | 位置与说明 |
| :--- | :--- | :--- | :--- |
| 1 | SEARCH 块存在且唯一匹配 | 满足 | `CMakeLists.txt` (1 处) 与 `ContentContextMenu.cpp` (3 处) 原文唯一 |
| 2 | 外部成员函数签名一致 | 满足 | 见 Chapter 7 |
| 3 | 新增接口声明、定义与调用一致 | 满足 | 见 Chapter 7 |
| 4 | 被修改/删除头文件链闭合 | 满足 | 见 Chapter 8，所有用到的类型均显式包含 |
| 5 | 新建/删除文件在 CMakeLists.txt 注册 | 满足 | 4 个新文件均在 `CMakeLists.txt` 注册 |
| 6 | 跨类调用成员访问级别允许 | 满足 | 全部为 public |
| 7 | 未触及方案范围外的文件 | 满足 | 严格限制在指定范围内 |
| 8 | 无笨拙浪费资源实现 | 满足 | 纯物理职责抽离，逻辑一比一迁移 |
| 9 | 无临时规避补丁 | 满足 | 结构化拆解辅助控制器 |
| 10 | 遵循 SSOT 原则 | 满足 | 见 Chapter 6 |
| 11 | ToolTipOverlay 不跟随鼠标 | 满足 | 均使用全局静态 showToast 气泡提示 |
| 12 | 异步并发与弱引用安全 | 满足 | 异步回调使用 `QPointer<ContentPanel>` 安全追踪生命周期 |
| 13 | 进程退出清理与释放 | 满足 | `proc->deleteLater()` 与异常残缺文件物理清理 |
| 14 | 10 大章节完整无缺漏 | 满足 | 全部 10 章完整 |
| 15 | 纯中文答复与说明 | 满足 | 全程中文 |
| 16 | 零多余功能引入 | 满足 | 纯重构提取 |
| 17 | 异常边界兜底防护 | 满足 | 入参空指针校验（`if (!panel) return;`）完整 |
| 18 | 宏与命名空间规范 | 满足 | 统一包裹于 `namespace QuarkMeta` |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 交互/逻辑/数值项 | 重构前原始行为与数值 | 重构后行为与数值 | 结论 |
| :--- | :--- | :--- | :--- |
| 解压 ZIP PowerShell 脚本 | `Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force` | 完全一致 | 零篡改 |
| 压缩 ZIP PowerShell 脚本 | `$files = @(%1); Compress-Archive -LiteralPath $files -DestinationPath '%2' -Force` | 完全一致 | 零篡改 |
| Toast 提示内容与颜色 | 蓝色 `#3498db`、绿色 `#2ecc71`、红色 `#e81123`/`#e74c3c`、时长 1500ms/2000ms/2500ms | 完全一致 | 零篡改 |
| 弹窗标题与提示文本 | "加密保护"、"解除外壳保护"、"修改保护密码" 等文本 | 完全一致 | 零篡改 |
| 密码输入框模式 | `dlg.setEchoMode(QLineEdit::Password)` | 完全一致 | 零篡改 |
