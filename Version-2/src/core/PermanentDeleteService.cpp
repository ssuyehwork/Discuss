#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "PermanentDeleteService.h"
#include "../ui/dialogs/FramelessMessageBox.h"
#include "../ui/BatchProgressDialog.h"
#include "../util/SecureFileEraser.h"
#include "../core/UndoManager.h"
#include "../core/CoreEngine.h"
#include "../core/DiskTrashService.h"
#include "../meta/MetadataManager.h"
#include "../meta/TrashRepository.h"
#include <QtConcurrent>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QPointer>
#include <QCoreApplication>

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

    BatchProgressDialog* progress = new BatchProgressDialog("正在执行永久删除（深层抹除）...", parentWidget);
    progress->show();

    QPointer<BatchProgressDialog> weakProgress(progress);
    QPointer<PermanentDeleteService> weakThis(this);

    MetadataManager::instance().beginInternalOperation();

    (void)QtConcurrent::run([paths, isSecureShred, weakProgress, weakThis]() {
        int total = paths.size();
        int count = 0;

        for (const QString& p : paths) {
            std::wstring wp = QDir::toNativeSeparators(p).toStdWString();
            
            // 检查是否为回收站条目
            int trashId = -1;
            QString trashPath;
            bool foundRecord = TrashRepository::instance().getDiskTrashRecordByPath(wp, trashId, trashPath);
            bool isTrashItem = foundRecord || p.contains(".QuarkMeta/disk_trash");

            if (isTrashItem) {
                DiskTrashService::permanentlyDeleteDiskTrash(trashId, p);
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
                    AppCommand cmd;
                    cmd.type = AppCommandType::DeletePermanently;
                    cmd.targetPaths << QString::fromStdWString(wp);
                    CoreEngine::instance().executeCommand(cmd);
                    UndoManager::instance().removeCommandsForPath(p);
                }
            }

            count++;
            if (weakProgress) {
                int percent = (int)((float)count / total * 100);
                QMetaObject::invokeMethod(QCoreApplication::instance(), [weakProgress, percent]() {
                    if (weakProgress) weakProgress->setValue(percent);
                });
            }
        }

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakProgress, weakThis]() {
            if (weakProgress) {
                weakProgress->accept();
                weakProgress->deleteLater();
            }
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

    BatchProgressDialog* progress = new BatchProgressDialog("正在销毁回收站项目...", parentWidget);
    progress->show();

    QPointer<BatchProgressDialog> weakProgress(progress);
    QPointer<PermanentDeleteService> weakThis(this);

    MetadataManager::instance().beginInternalOperation();

    (void)QtConcurrent::run([trashItems, weakProgress, weakThis]() {
        int total = static_cast<int>(trashItems.size());
        int count = 0;

        for (const auto& item : trashItems) {
            DiskTrashService::permanentlyDeleteDiskTrash(item.first, item.second);
            count++;
            if (weakProgress) {
                int percent = (int)((float)count / total * 100);
                QMetaObject::invokeMethod(QCoreApplication::instance(), [weakProgress, percent]() {
                    if (weakProgress) weakProgress->setValue(percent);
                });
            }
        }

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakProgress, weakThis]() {
            if (weakProgress) {
                weakProgress->accept();
                weakProgress->deleteLater();
            }
            MetadataManager::instance().endInternalOperation();
            if (weakThis) {
                emit weakThis->permanentDeleteCompleted();
            }
        });
    });

    return true;
}

} // namespace QuarkMeta
