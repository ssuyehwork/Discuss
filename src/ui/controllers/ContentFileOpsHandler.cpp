#include "ContentFileOpsHandler.h"
#include "../ContentPanel.h"
#include "../ToolTipOverlay.h"
#include "../BatchRenameDialog.h"
#include "../../core/AppConfig.h"
#include "../../core/ClipboardService.h"
#include "../../core/NavigationHistoryService.h"
#include "../../util/DiskIoService.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QApplication>
#include <QPointer>

namespace QuarkMeta {

ContentFileOpsHandler::ContentFileOpsHandler(ContentPanel* panel)
    : QObject(panel), m_panel(panel) {}

void ContentFileOpsHandler::createNewItem(const QString& type) {
    if (!m_panel) return;
    QString currentPath = m_panel->currentPath();
    if (currentPath.isEmpty() || currentPath == "computer://") return;

    QString baseName = (type == "folder") ? "新建文件夹" : "未命名";
    QString ext = (type == "md") ? ".md" : ((type == "txt") ? ".txt" : "");
    QString finalName = baseName + ext;
    QString fullPath = currentPath + "/" + finalName;
    int counter = 1;

    while (QFileInfo::exists(fullPath)) {
        finalName = baseName + QString(" (%1)").arg(counter++) + ext;
        fullPath = currentPath + "/" + finalName;
    }

    if (type == "folder") {
        QDir(currentPath).mkdir(finalName);
    } else {
        QFile f(fullPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.close();
        }
    }

    m_panel->setPendingSelectName(finalName, true);
    m_panel->loadDirectory(currentPath, m_panel->isRecursive());
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
    QString currentPath = m_panel->currentPath();
    if (currentPath.isEmpty() || currentPath == "computer://") {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "粘贴失败：当前未处于任何有效目录中", 2000, QColor("#e81123"));
        return false;
    }
    return true;
}

void ContentFileOpsHandler::onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex) {
    if (!m_panel || paths.isEmpty()) return;
    QString currentPath = m_panel->currentPath();
    if (currentPath.isEmpty() || currentPath == "computer://") return;

    QString destDir = currentPath;
    if (targetIndex.isValid() && m_panel->getProxyModel()) {
        QModelIndex srcIdx = m_panel->getProxyModel()->mapToSource(targetIndex);
        if (srcIdx.isValid() && QFileInfo(srcIdx.data(PathRole).toString()).isDir()) {
            destDir = srcIdx.data(PathRole).toString();
        }
    }

    if (!destDir.isEmpty() && destDir != "computer://") {
        NavigationHistoryService::recordRecentVisitedFolder(QDir::toNativeSeparators(destDir).toStdWString());
        AppConfig::instance().setValue("RecentVisited/LastDragDropDestination", destDir);
        AppConfig::instance().sync();
    }

    DiskIoContext ioCtx;
    ioCtx.sources = paths;
    ioCtx.destination = destDir;
    ioCtx.isMove = !(QApplication::keyboardModifiers() & Qt::ControlModifier);

    QPointer<ContentPanel> weakPanel(m_panel);
    DiskIoService::instance().executeAsync(ioCtx, [weakPanel](bool success) {
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, success]() {
            if (weakPanel && success) {
                weakPanel->loadDirectory(weakPanel->currentPath(), weakPanel->isRecursive());
            }
        });
    });
}

} // namespace QuarkMeta
