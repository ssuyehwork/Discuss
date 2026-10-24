#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "PermanentDeleteService.h"
#include "../ui/dialogs/FramelessMessageBox.h"
#include "TaskProgressService.h"
#include "../ui/ToolTipOverlay.h"
#include "../util/SecureFileEraser.h"
#include "../core/UndoManager.h"
#include "../core/CoreEngine.h"
#include "../core/DiskTrashService.h"
#include "../meta/MetadataManager.h"
#include "../meta/TrashRepository.h"
#include "../meta/QuarkMetaJson.h"
#include "../meta/DiskTrashRepo.h"
#include <QtConcurrent>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QPointer>
#include <QCoreApplication>
#include <QCursor>

namespace QuarkMeta {

PermanentDeleteService& PermanentDeleteService::instance() {
    static PermanentDeleteService s_instance;
    return s_instance;
}

PermanentDeleteService::PermanentDeleteService(QObject* parent) : QObject(parent) {}

bool PermanentDeleteService::execute(const QStringList& paths, QWidget* parentWidget, bool isSecureShred) {
    if (paths.isEmpty()) return false;

    QString msg = "确定要永久删除选中的项目吗？数据将被物理覆写并彻底抹除，此操作不可恢复。";
    if (!FramelessMessageBox::question(parentWidget, "确认删除", msg)) {
        return false;
    }

    TaskJobOptions opts;
    opts.title = "正在执行永久删除（深层抹除）";
    opts.total = paths.size();
    opts.showCount = true;
    opts.showEta = true;
    opts.finishTextTemplate = "永久删除完成  数量：{total}  |  实际耗时：{elapsed}";

    TaskProgressJob job = TaskProgressService::instance().createJobHandle(opts);
    std::shared_ptr<CancellationToken> token = job.cancellationToken();

    QPointer<PermanentDeleteService> weakThis(this);

    MetadataManager::instance().beginInternalOperation();

    (void)QtConcurrent::run([paths, isSecureShred, token, weakThis, job = std::move(job)]() mutable {
        for (const QString& p : paths) {
            if (token->isCanceled()) break;

            std::wstring wp = QDir::toNativeSeparators(p).toStdWString();
            
            int trashId = -1;
            QString trashPath;
            bool foundRecord = TrashRepository::instance().getDiskTrashRecordByPath(wp, trashId, trashPath);
            bool isTrashItem = foundRecord || p.contains(".QuarkMeta/disk_trash");

            if (isTrashItem) {
                if (isSecureShred) {
                    SecureFileEraser::shredFile(p);
                }
                DiskTrashService::permanentlyDeleteDiskTrash(trashId, p);
                UndoManager::instance().removeCommandsForPath(p);
            } else {
                bool physicalOk = false;
                if (isSecureShred) {
                    physicalOk = SecureFileEraser::shredFile(p);
                } else {
                    QFileInfo info(p);
                    if (info.isDir()) {
                        physicalOk = QDir(p).removeRecursively();
                    } else {
                        physicalOk = QFile::remove(p);
                    }
                }

                if (physicalOk) {
                    QuarkMetaJson::removeItemMetadata(p);
                    AppCommand cmd;
                    cmd.type = AppCommandType::DeletePermanently;
                    cmd.targetPaths << QString::fromStdWString(wp);
                    CoreEngine::instance().executeCommand(cmd);
                    UndoManager::instance().removeCommandsForPath(p);
                }
            }

            job.advance(1);
        }

        job.finish();

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis]() {
            MetadataManager::instance().endInternalOperation();
            if (weakThis) {
                emit weakThis->permanentDeleteCompleted();
            }
        });
    });

    return true;
}

bool PermanentDeleteService::executeTrashItems(const std::vector<std::pair<int, QString>>& trashItems, QWidget* parentWidget) {
    if (trashItems.empty()) return false;

    QString msg = "确定要永久删除选中的回收站项目吗？数据将被彻底抹除且不可恢复。";
    if (!FramelessMessageBox::question(parentWidget, "确认删除", msg)) {
        return false;
    }

    TaskJobOptions opts;
    opts.title = "正在销毁回收站项目";
    opts.total = static_cast<int>(trashItems.size());
    opts.showCount = true;
    opts.showEta = true;
    opts.finishTextTemplate = "销毁回收站项目完成  数量：{total}  |  实际耗时：{elapsed}";

    TaskProgressJob job = TaskProgressService::instance().createJobHandle(opts);
    std::shared_ptr<CancellationToken> token = job.cancellationToken();

    QPointer<PermanentDeleteService> weakThis(this);

    MetadataManager::instance().beginInternalOperation();

    (void)QtConcurrent::run([trashItems, token, weakThis, job = std::move(job)]() mutable {
        for (const auto& item : trashItems) {
            if (token->isCanceled()) break;

            SecureFileEraser::shredFile(item.second);
            DiskTrashService::permanentlyDeleteDiskTrash(item.first, item.second);
            UndoManager::instance().removeCommandsForPath(item.second);
            job.advance(1);
        }

        job.finish();

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis]() {
            MetadataManager::instance().endInternalOperation();
            if (weakThis) {
                emit weakThis->permanentDeleteCompleted();
            }
        });
    });

    return true;
}

bool PermanentDeleteService::executeEmptyTrash(QWidget* parentWidget, bool isSecureShred) {
    auto allTrash = DiskTrashRepo::getAllTrashItems();
    if (allTrash.empty()) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "回收站为空，无需清理", 1500);
        return true;
    }

    QString msg = QString("确定要清空回收站吗？共有 %1 个项目将被物理覆写并彻底抹除，此操作不可恢复。").arg(allTrash.size());
    if (!FramelessMessageBox::question(parentWidget, "清空回收站确认", msg)) {
        return false;
    }

    TaskJobOptions opts;
    opts.title = "正在清空回收站（深层物理抹除）";
    opts.total = static_cast<int>(allTrash.size());
    opts.showCount = true;
    opts.showEta = true;
    opts.finishTextTemplate = "清空回收站完成  数量：{total}  |  实际耗时：{elapsed}";

    TaskProgressJob job = TaskProgressService::instance().createJobHandle(opts);
    std::shared_ptr<CancellationToken> token = job.cancellationToken();

    QPointer<PermanentDeleteService> weakThis(this);

    MetadataManager::instance().beginInternalOperation();

    (void)QtConcurrent::run([allTrash, isSecureShred, token, weakThis, job = std::move(job)]() mutable {
        for (const auto& item : allTrash) {
            if (token->isCanceled()) break;

            QString path = QString::fromStdWString(item.trashPath);
            if (isSecureShred) {
                SecureFileEraser::shredFile(path);
            }
            DiskTrashService::permanentlyDeleteDiskTrash(item.id, path);
            UndoManager::instance().removeCommandsForPath(QString::fromStdWString(item.originalPath));

            job.advance(1);
        }

        job.finish();

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis]() {
            MetadataManager::instance().endInternalOperation();
            MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
            if (weakThis) {
                emit weakThis->permanentDeleteCompleted();
            }
        });
    });

    return true;
}

} // namespace QuarkMeta