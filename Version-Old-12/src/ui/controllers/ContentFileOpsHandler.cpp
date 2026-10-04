#include "ContentFileOpsHandler.h"
#include "../ContentPanel.h"
#include "../../core/FileCreationService.h"
#include "../ColumnViewWidget.h"
#include "../ToolTipOverlay.h"
#include "../BatchRenameDialog.h"
#include "../FileCollisionDialog.h"
#include "../models/DiskItemModel.h"
#include "../../core/AppConfig.h"
#include "../../core/ClipboardService.h"
#include "../../core/NavigationHistoryService.h"
#include "../../core/LastOperationManager.h"
#include "../../util/DiskIoService.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QApplication>
#include <QPointer>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>
#include <QCoreApplication>
#include <QMetaObject>

namespace QuarkMeta {

ContentFileOpsHandler::ContentFileOpsHandler(ContentPanel* panel)
    : QObject(panel), m_panel(panel) {}

void ContentFileOpsHandler::createNewItem(const QString& type) {
    FileCreationService::instance().createNewItem(m_panel, type);
}

void ContentFileOpsHandler::performBatchRename() {
    if (!m_panel) return;
    std::vector<std::wstring> originalPaths;
    for (const auto& idx : m_panel->getSelectedIndexes()) {
        if (idx.column() == 0) {
            QString p = idx.data(PathRole).toString();
            if (!p.isEmpty()) {
                originalPaths.push_back(QDir::toNativeSeparators(p).toStdWString());
            }
        }
    }
    if (originalPaths.empty()) return;

    BatchRenameDialog dlg(originalPaths, m_panel);
    if (dlg.exec() == QDialog::Accepted) {
        m_panel->refreshAll();
    }
}

bool ContentFileOpsHandler::resolvePasteDestination() {
    if (!m_panel) return false;
    if (m_panel->getCurrentCategoryType() == "trash") {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "当前视图为回收站，不支持粘贴或拖拽导入新项目", 2000, QColor("#e81123"));
        return false;
    }
    QString currentPath = m_panel->activePath();
    if (currentPath.isEmpty() || currentPath == "computer://") {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "粘贴失败：当前未处于任何有效目录中", 2000, QColor("#e81123"));
        return false;
    }
    return true;
}

void ContentFileOpsHandler::onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride, QAbstractItemModel* sourceModelOverride) {
    if (!m_panel || paths.isEmpty()) return;
    
    QString baseDir = !targetDirOverride.isEmpty() ? targetDirOverride : m_panel->currentPath();
    if (baseDir.isEmpty() || baseDir == "computer://") return;

    QString destDir = baseDir;
    QAbstractItemModel* proxyModel = sourceModelOverride ? sourceModelOverride : m_panel->getProxyModel();

    if (targetIndex.isValid() && proxyModel) {
        QModelIndex srcIdx = targetIndex;
        if (auto* filterProxy = qobject_cast<QSortFilterProxyModel*>(proxyModel)) {
            srcIdx = filterProxy->mapToSource(targetIndex);
        }
        if (srcIdx.isValid() && QFileInfo(srcIdx.data(PathRole).toString()).isDir()) {
            destDir = srcIdx.data(PathRole).toString();
        }
    }

    qDebug() << "[ColumnView DragDrop Debug] Sources:" << paths 
             << "| TargetDirOverride:" << targetDirOverride 
             << "| Final DestDir:" << destDir;

    bool isMove = !(QApplication::keyboardModifiers() & Qt::ControlModifier);

    if (!destDir.isEmpty() && destDir != "computer://") {
        NavigationHistoryService::recordRecentVisitedFolder(QDir::toNativeSeparators(destDir).toStdWString());
        AppConfig::instance().setValue("RecentVisited/LastDragDropDestination", destDir);
        AppConfig::instance().sync();
        if (isMove) {
            LastOperationManager::instance().recordMoveToFolder(destDir);
        }
    }

    // 0. 原地/同目录拖放保护：剔除源目录与目标目录一模一样的项目
    QStringList externalPaths;
    for (const QString& src : paths) {
        QFileInfo srcInfo(src);
        if (QDir::cleanPath(srcInfo.absolutePath()) != QDir::cleanPath(QDir(destDir).absolutePath())) {
            externalPaths.append(src);
        }
    }

    if (externalPaths.isEmpty()) {
        // 全为同目录内自拖放，直接静默恢复/忽略，绝不误触同名冲突弹窗
        return;
    }

    // 1. 仅对来自外部目录的项目检测目标文件夹中的同名冲突文件
    QStringList conflictingSources;
    for (const QString& src : externalPaths) {
        QString fileName = QFileInfo(src).fileName();
        QString destPath = QDir(destDir).filePath(fileName);
        if (QFile::exists(destPath)) {
            conflictingSources.append(src);
        }
    }

    DiskIoContext ioCtx;
    ioCtx.sources = externalPaths;
    ioCtx.destination = destDir;
    ioCtx.isMove = isMove;

    if (!conflictingSources.isEmpty()) {
        QStringList activeSources = externalPaths;
        int remainingConflicts = conflictingSources.size();

        for (int i = 0; i < conflictingSources.size(); ++i) {
            const QString& srcFile = conflictingSources.at(i);
            FileCollisionDialog dialog(srcFile, destDir, remainingConflicts--, m_panel);

            if (dialog.exec() != QDialog::Accepted) {
                return; // 用户取消
            }

            CollisionResolveAction action = dialog.selectedAction();
            bool applyToAll = dialog.applyToAll();

            if (action == CollisionResolveAction::Cancel) {
                return;
            }

            if (applyToAll) {
                if (action == CollisionResolveAction::AutoResolve) {
                    ioCtx.autoRenameAll = true;
                } else if (action == CollisionResolveAction::Replace) {
                    ioCtx.overwriteAll = true;
                } else if (action == CollisionResolveAction::Skip) {
                    for (int j = i; j < conflictingSources.size(); ++j) {
                        activeSources.removeOne(conflictingSources.at(j));
                    }
                }
                break;
            } else {
                if (action == CollisionResolveAction::AutoResolve) {
                    ioCtx.autoRenameFiles.insert(srcFile);
                } else if (action == CollisionResolveAction::Replace) {
                    ioCtx.overwriteFiles.insert(srcFile);
                } else if (action == CollisionResolveAction::Skip) {
                    activeSources.removeOne(srcFile);
                }
            }
        }

        if (activeSources.isEmpty()) {
            ToolTipOverlay::instance()->showText(QCursor::pos(), "已跳过所有同名文件", 1500, QColor("#378ADD"));
            return;
        }
        ioCtx.sources = activeSources;
    }

    QPointer<ContentPanel> weakPanel(m_panel);
    DiskIoService::instance().executeAsync(ioCtx, [weakPanel](bool success) {
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, success]() {
            if (weakPanel && success) {
                weakPanel->refreshAll();
            }
        });
    });
}

} // namespace QuarkMeta
