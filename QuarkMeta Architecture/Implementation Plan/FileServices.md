# QuarkMeta 核心文件操作与生命周期服务模块化实施方案 (file-service.md)

## 1. Overview
本方案旨在解决 `ContentPanel.cpp` 等视图层组件中混杂底层磁盘 I/O、MIME 解析、回收站数据库存取及多线程硬删逻辑的问题。通过解耦并抽离出 3 个核心服务类（`TrashService`、`PermanentDeleteService`、`ClipboardService`），彻底规范 Clean Architecture 架构分层，实现视图层与物理传输、撤销快照及擦除逻辑的彻底分离。

---

## 2. Modified Files List
1. **新建** `src/core/TrashService.h`
2. **新建** `src/core/TrashService.cpp`
3. **新建** `src/core/PermanentDeleteService.h`
4. **新建** `src/core/PermanentDeleteService.cpp`
5. **新建** `src/core/ClipboardService.h`
6. **新建** `src/core/ClipboardService.cpp`
7. **修改** `src/ui/ContentPanel.cpp`
8. **修改** `CMakeLists.txt`

---

## 3. Detailed Line-by-Line Changes

### 3.1 新增服务头文件与实现文件

#### `src/core/TrashService.h`
```cpp
#pragma once

#include <QObject>
#include <QStringList>
#include <QList>
#include <QWidget>

namespace QuarkMeta {

class TrashService : public QObject {
    Q_OBJECT

public:
    static TrashService& instance();

    bool moveToTrash(const QStringList& paths, QWidget* parentWidget = nullptr);
    bool restoreItems(const QList<int>& trashIds, QWidget* parentWidget = nullptr);
    bool restoreAll(QWidget* parentWidget = nullptr);
    bool restoreToDirectory(const QStringList& trashPaths, const QString& targetDir, QWidget* parentWidget = nullptr);
    bool emptyTrash(QWidget* parentWidget = nullptr);

signals:
    void trashItemCountChanged(int totalCount);
    void trashOperationCompleted();

private:
    explicit TrashService(QObject* parent = nullptr);
    ~TrashService() override = default;
    TrashService(const TrashService&) = delete;
    TrashService& operator=(const TrashService&) = delete;
};

} // namespace QuarkMeta
```

#### `src/core/TrashService.cpp`
```cpp
#include "TrashService.h"
#include "../core/DiskTrashService.h"
#include "../core/OperationSnapshotEngine.h"
#include "../meta/TrashRepository.h"
#include "../meta/MetadataManager.h"
#include "../ui/ToolTipOverlay.h"
#include <QCursor>

namespace QuarkMeta {

TrashService& TrashService::instance() {
    static TrashService s_instance;
    return s_instance;
}

TrashService::TrashService(QObject* parent) : QObject(parent) {}

bool TrashService::moveToTrash(const QStringList& paths, QWidget* parentWidget) {
    if (paths.isEmpty()) return false;

    QStringList validPaths;
    for (const QString& p : paths) {
        if (!p.isEmpty() && p != "computer://" && p != "trash://" && !p.contains(".QuarkMeta/disk_trash")) {
            validPaths << p;
        }
    }
    if (validPaths.isEmpty()) return false;

    return OperationSnapshotEngine::instance().executeWithSnapshot(
        parentWidget,
        SnapshotOperationType::DeleteToTrash,
        validPaths,
        QString("已将 %1 个项目移入回收站").arg(validPaths.size()),
        [validPaths]() {
            MetadataManager::instance().beginInternalOperation();
            bool ok = DiskTrashService::moveToDiskTrash(validPaths);
            MetadataManager::instance().endInternalOperation();
            return ok;
        },
        [](const QVector<AssetItemSnapshot>& beforeState) {
            for (const auto& snap : beforeState) {
                int trashId = -1;
                QString trashPath;
                if (TrashRepository::instance().getDiskTrashRecordByPath(snap.path.toStdWString(), trashId, trashPath)) {
                    DiskTrashService::restoreFromDiskTrash(trashId, trashPath);
                }
            }
            MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
            return true;
        }
    );
}

bool TrashService::restoreItems(const QList<int>& trashIds, QWidget* parentWidget) {
    Q_UNUSED(parentWidget);
    if (trashIds.isEmpty()) return false;

    for (int id : trashIds) {
        DiskTrashService::restoreFromDiskTrash(id, "");
    }
    MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
    emit trashOperationCompleted();
    return true;
}

bool TrashService::restoreAll(QWidget* parentWidget) {
    Q_UNUSED(parentWidget);
    bool ok = DiskTrashService::restoreAllDiskTrash();
    if (ok) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "所有回收站项目已成功还原", 1500, QColor("#2ecc71"));
        MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
        emit trashOperationCompleted();
    }
    return ok;
}

bool TrashService::restoreToDirectory(const QStringList& trashPaths, const QString& targetDir, QWidget* parentWidget) {
    Q_UNUSED(parentWidget);
    if (trashPaths.isEmpty() || targetDir.isEmpty()) return false;

    int successCount = 0;
    for (const QString& p : trashPaths) {
        if (DiskTrashService::restoreToDirectory(p, targetDir)) {
            successCount++;
        }
    }
    if (successCount > 0) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), QString("成功恢复 %1 个项目").arg(successCount), 1500, QColor("#2ecc71"));
        emit trashOperationCompleted();
    }
    return successCount > 0;
}

bool TrashService::emptyTrash(QWidget* parentWidget) {
    Q_UNUSED(parentWidget);
    bool ok = DiskTrashService::emptyDiskTrash();
    if (ok) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "回收站已清空", 1500, QColor("#2ecc71"));
        MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
        emit trashOperationCompleted();
    }
    return ok;
}

} // namespace QuarkMeta
```

#### `src/core/PermanentDeleteService.h`
```cpp
#pragma once

#include <QObject>
#include <QStringList>
#include <QWidget>
#include <vector>
#include <utility>

namespace QuarkMeta {

class PermanentDeleteService : public QObject {
    Q_OBJECT

public:
    static PermanentDeleteService& instance();

    bool execute(const QStringList& paths, QWidget* parentWidget = nullptr, bool isSecureShred = true);
    bool executeTrashItems(const std::vector<std::pair<int, QString>>& trashItems, QWidget* parentWidget = nullptr);

signals:
    void permanentDeleteCompleted();

private:
    explicit PermanentDeleteService(QObject* parent = nullptr);
    ~PermanentDeleteService() override = default;
    PermanentDeleteService(const PermanentDeleteService&) = delete;
    PermanentDeleteService& operator=(const PermanentDeleteService&) = delete;
};

} // namespace QuarkMeta
```

#### `src/core/PermanentDeleteService.cpp`
```cpp
#include "PermanentDeleteService.h"
#include "../ui/dialogs/FramelessMessageBox.h"
#include "../ui/BatchProgressDialog.h"
#include "../util/SecureFileEraser.h"
#include "../core/UndoManager.h"
#include "../core/CoreEngine.h"
#include "../core/DiskTrashService.h"
#include "../meta/MetadataManager.h"
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
```

#### `src/core/ClipboardService.h`
```cpp
#pragma once

#include <QObject>
#include <QStringList>
#include <QWidget>

namespace QuarkMeta {

class ClipboardService : public QObject {
    Q_OBJECT

public:
    static ClipboardService& instance();

    void copyItems(const QStringList& paths);
    void cutItems(const QStringList& paths);
    bool canPaste(const QString& targetDir) const;
    void executePaste(const QString& targetDir, QWidget* parentWidget = nullptr);

signals:
    void pasteCompleted(const QString& targetDir);

private:
    explicit ClipboardService(QObject* parent = nullptr);
    ~ClipboardService() override = default;
    ClipboardService(const ClipboardService&) = delete;
    ClipboardService& operator=(const ClipboardService&) = delete;
};

} // namespace QuarkMeta
```

#### `src/core/ClipboardService.cpp`
```cpp
#include "ClipboardService.h"
#include "TrashService.h"
#include "../util/DiskIoService.h"
#include "../ui/ToolTipOverlay.h"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QImage>
#include <QCursor>
#include <QCoreApplication>
#include <QPointer>

namespace QuarkMeta {

ClipboardService& ClipboardService::instance() {
    static ClipboardService s_instance;
    return s_instance;
}

ClipboardService::ClipboardService(QObject* parent) : QObject(parent) {}

void ClipboardService::copyItems(const QStringList& paths) {
    if (paths.isEmpty()) return;

    QMimeData* mime = new QMimeData();
    QList<QUrl> urls;
    for (const QString& p : paths) {
        if (!p.isEmpty()) urls << QUrl::fromLocalFile(p);
    }
    mime->setUrls(urls);
    QApplication::clipboard()->setMimeData(mime);
}

void ClipboardService::cutItems(const QStringList& paths) {
    if (paths.isEmpty()) return;

    QMimeData* mime = new QMimeData();
    QList<QUrl> urls;
    for (const QString& p : paths) {
        if (!p.isEmpty()) urls << QUrl::fromLocalFile(p);
    }
    mime->setUrls(urls);

    QByteArray effectData;
    effectData.append(static_cast<char>(2)); // DROPEFFECT_MOVE
    mime->setData("Preferred DropEffect", effectData);

    QApplication::clipboard()->setMimeData(mime);
}

bool ClipboardService::canPaste(const QString& targetDir) const {
    if (targetDir.isEmpty() || targetDir == "computer://" || targetDir == "trash://" || targetDir.contains("://")) {
        return false;
    }

    QFileInfo destInfo(targetDir);
    if (!destInfo.exists() || !destInfo.isDir() || !destInfo.isWritable()) {
        return false;
    }

    const QMimeData* mime = QApplication::clipboard()->mimeData();
    if (!mime) return false;

    if (mime->hasImage()) {
        QImage img = qvariant_cast<QImage>(mime->imageData());
        if (!img.isNull() && img.width() > 0 && img.height() > 0) return true;
    }

    if (!mime->hasUrls() || mime->urls().isEmpty()) {
        return false;
    }

    bool isCut = false;
    if (mime->hasFormat("Preferred DropEffect")) {
        QByteArray effect = mime->data("Preferred DropEffect");
        if (!effect.isEmpty() && (effect.at(0) & 0x02)) isCut = true;
    }

    QString cleanDest = QDir::toNativeSeparators(QDir::cleanPath(targetDir)).toLower();
    bool hasValidPhysicalSource = false;
    bool isSameDirForCut = true;

    for (const QUrl& url : mime->urls()) {
        if (!url.isLocalFile()) continue;
        QString localPath = url.toLocalFile();
        if (localPath.isEmpty()) continue;

        QFileInfo srcInfo(localPath);
        if (!srcInfo.exists()) continue;

        hasValidPhysicalSource = true;
        QString cleanSrc = QDir::toNativeSeparators(QDir::cleanPath(localPath)).toLower();

        if (srcInfo.isDir()) {
            if (cleanDest == cleanSrc || cleanDest.startsWith(cleanSrc + "\\") || cleanDest.startsWith(cleanSrc + "/")) {
                return false;
            }
        }

        QString srcParent = QDir::toNativeSeparators(QDir::cleanPath(srcInfo.absolutePath())).toLower();
        if (srcParent != cleanDest) {
            isSameDirForCut = false;
        }
    }

    if (!hasValidPhysicalSource) return false;
    if (isCut && isSameDirForCut) return false;

    return true;
}

void ClipboardService::executePaste(const QString& targetDir, QWidget* parentWidget) {
    Q_UNUSED(parentWidget);
    const QMimeData* mime = QApplication::clipboard()->mimeData();
    if (!mime) return;

    if (mime->hasImage() && (!mime->hasUrls() || mime->urls().isEmpty())) {
        QImage img = qvariant_cast<QImage>(mime->imageData());
        if (!img.isNull() && !targetDir.isEmpty() && QDir(targetDir).exists()) {
            QString timeStr = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
            QString baseName = QString("贴图_%1").arg(timeStr);
            QString fileName = baseName + ".png";
            QString fullPath = QDir(targetDir).filePath(fileName);

            int counter = 1;
            while (QFile::exists(fullPath)) {
                fileName = QString("%1_(%2).png").arg(baseName).arg(counter++);
                fullPath = QDir(targetDir).filePath(fileName);
            }

            if (img.save(fullPath, "PNG")) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "已将剪贴板图片粘贴为新文件", 1500, QColor("#2ecc71"));
                emit pasteCompleted(targetDir);
            }
            return;
        }
    }

    QStringList fromPaths;
    for (const QUrl& url : mime->urls()) {
        if (url.isLocalFile()) fromPaths << url.toLocalFile();
    }
    if (fromPaths.isEmpty()) return;

    bool hasTrashSource = false;
    for (const QString& p : fromPaths) {
        if (p.contains(".QuarkMeta/disk_trash", Qt::CaseInsensitive)) {
            hasTrashSource = true;
            break;
        }
    }

    if (hasTrashSource) {
        TrashService::instance().restoreToDirectory(fromPaths, targetDir);
        emit pasteCompleted(targetDir);
        return;
    }

    bool isMove = false;
    if (mime->hasFormat("Preferred DropEffect")) {
        QByteArray effect = mime->data("Preferred DropEffect");
        if (!effect.isEmpty() && (effect.at(0) & 0x02)) isMove = true;
    }

    DiskIoContext ioCtx;
    ioCtx.sources = fromPaths;
    ioCtx.destination = targetDir;
    ioCtx.isMove = isMove;

    QPointer<ClipboardService> weakThis(this);
    DiskIoService::instance().executeAsync(ioCtx, [weakThis, targetDir](bool success) {
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, targetDir, success]() {
            if (weakThis) {
                if (success) {
                    emit weakThis->pasteCompleted(targetDir);
                } else {
                    ToolTipOverlay::instance()->showText(QCursor::pos(), "粘贴失败：物理写入未能完成", 2000, QColor("#e81123"));
                }
            }
        });
    });
}

} // namespace QuarkMeta
```

---

### 3.2 `CMakeLists.txt` 构建依赖注册

```cmake
<<<<<<< SEARCH
    src/core/OperationSnapshotEngine.h
    src/core/OperationSnapshotEngine.cpp
=======
    src/core/OperationSnapshotEngine.h
    src/core/OperationSnapshotEngine.cpp
    src/core/TrashService.h
    src/core/TrashService.cpp
    src/core/PermanentDeleteService.h
    src/core/PermanentDeleteService.cpp
    src/core/ClipboardService.h
    src/core/ClipboardService.cpp
>>>>>>> REPLACE
```

---

### 3.3 `src/ui/ContentPanel.cpp` 引入服务并替换响应逻辑

#### A. 引入服务头文件
```cpp
<<<<<<< SEARCH
#include "../core/DiskTrashService.h"
#include "../util/DiskIoService.h"
=======
#include "../core/TrashService.h"
#include "../core/PermanentDeleteService.h"
#include "../core/ClipboardService.h"
>>>>>>> REPLACE
```

#### B. 构造函数中连接服务完成信号
```cpp
<<<<<<< SEARCH
    connect(m_proxyModel, &FilterProxyModel::modelReset, this, [this]() {
        recalculateAndEmitStats();
    });
=======
    connect(m_proxyModel, &FilterProxyModel::modelReset, this, [this]() {
        recalculateAndEmitStats();
    });

    connect(&QuarkMeta::TrashService::instance(), &QuarkMeta::TrashService::trashOperationCompleted, this, &ContentPanel::refreshAll);
    connect(&QuarkMeta::PermanentDeleteService::instance(), &QuarkMeta::PermanentDeleteService::permanentDeleteCompleted, this, &ContentPanel::refreshAll);
    connect(&QuarkMeta::ClipboardService::instance(), &QuarkMeta::ClipboardService::pasteCompleted, this, [this](const QString& dir) {
        if (m_currentPath == dir) refreshAll();
    });
>>>>>>> REPLACE
```

#### C. 键盘按键逻辑（Delete / Ctrl+C / Ctrl+X / Ctrl+V）抽离
```cpp
<<<<<<< SEARCH
            if (keyEvent->key() == Qt::Key_Delete) { 
                if (keyEvent->modifiers() & Qt::ShiftModifier) {
                    QList<QModelIndex> selectedIndexes = view->selectionModel()->selectedIndexes();
                    if (!selectedIndexes.isEmpty()) {
                        QStringList targetPaths;
                        for (const auto& idx : selectedIndexes) {
                            if (idx.column() == 0) targetPaths << idx.data(PathRole).toString();
                        }
                        if (!targetPaths.isEmpty()) {
                            if (FramelessMessageBox::question(this, "确认删除", "确定要永久删除选中的项目吗？数据将被物理覆写并彻底抹除，此操作不可恢复。")) {
                                for (const QString& p : targetPaths) {
                                    QFileInfo info(p);
                                    if (info.isDir()) {
                                        QDir(p).removeRecursively();
                                    } else {
                                        QFile::remove(p);
                                    }
                                    MetadataManager::instance().removeMetadataSync(p.toStdWString());
                                }
                                refreshAll();
                            }
                        }
                    }
                } else {
                    // Del 键：移入回收站
                    QList<QModelIndex> selectedIndexes = view->selectionModel()->selectedIndexes();
                    if (!selectedIndexes.isEmpty()) {
                        QStringList targetPaths;
                        for (const auto& idx : selectedIndexes) {
                            if (idx.column() == 0) targetPaths << idx.data(PathRole).toString();
                        }
                        if (!targetPaths.isEmpty()) {
                            DiskTrashService::moveToDiskTrash(targetPaths);
                            refreshAll();
                        }
                    }
                }
                return true; 
            } 
=======
            if (keyEvent->key() == Qt::Key_Delete) { 
                if (keyEvent->modifiers() & Qt::ShiftModifier) {
                    QuarkMeta::PermanentDeleteService::instance().execute(getSelectedPaths(), this);
                } else {
                    QuarkMeta::TrashService::instance().moveToTrash(getSelectedPaths(), this);
                }
                return true; 
            } 
>>>>>>> REPLACE
```

#### D. 右键上下文菜单动作逻辑抽离
```cpp
<<<<<<< SEARCH
        case ActionDelete: 
        case ActionSecureDelete: {
            auto indexes = view->selectionModel()->selectedIndexes();
            QStringList targetPaths;
            std::vector<std::pair<int, QString>> diskTrashItems; // id, trashPath

            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    if (idx.data(IsDiskTrashRole).toBool()) {
                        diskTrashItems.push_back({idx.data(DiskTrashIdRole).toInt(), idx.data(PathRole).toString()});
                    } else {
                        targetPaths << idx.data(PathRole).toString();
                    }
                }
            }

            if (action == ActionDelete) {
                OperationSnapshotEngine::instance().executeWithSnapshot(
                    this,
                    SnapshotOperationType::DeleteToTrash,
                    targetPaths,
                    QString("已将 %1 个项目移入回收站").arg(targetPaths.size()),
                    [this, targetPaths]() {
                        MetadataManager::instance().beginInternalOperation();
                        bool ok = false;
                        ok = DiskTrashService::moveToDiskTrash(targetPaths);
                        if (ok) {
                            refreshAll();
                        }
                        MetadataManager::instance().endInternalOperation();
                        return ok;
                    },
                    [this](const QVector<AssetItemSnapshot>& beforeState) {
                        for (const auto& snap : beforeState) {
                            int trashId = -1;
                            QString trashPath;
                            if (QuarkMeta::TrashRepository::instance().getDiskTrashRecordByPath(snap.path.toStdWString(), trashId, trashPath)) {
                                DiskTrashService::restoreFromDiskTrash(trashId, trashPath);
                            }
                        }
                        refreshAll();
                        MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
                        return true;
                    }
                );
            } else {
                // 永久删除分支
                ...
            }
            break;
        }
=======
        case ActionDelete: {
            QuarkMeta::TrashService::instance().moveToTrash(getSelectedPaths(), this);
            break;
        }
        case ActionSecureDelete: {
            QuarkMeta::PermanentDeleteService::instance().execute(getSelectedPaths(), this);
            break;
        }
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
        case ActionRestore: {
            auto indexes = view->selectionModel()->selectedIndexes();
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    if (idx.data(IsDiskTrashRole).toBool()) {
                        int id = idx.data(DiskTrashIdRole).toInt();
                        QString trashPath = idx.data(PathRole).toString();
                        DiskTrashService::restoreFromDiskTrash(id, trashPath);
                    }
                }
            }
            refreshAll();
            MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
            break;
        }
=======
        case ActionRestore: {
            QuarkMeta::TrashService::instance().restoreItems(getSelectedTrashIds(), this);
            break;
        }
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
        case ActionRestoreAll: {
            if (DiskTrashService::restoreAllDiskTrash()) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "所有回收站项目已成功还原", 1500, QColor("#2ecc71"));
            }
            refreshAll();
            MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
            break;
        }
        case ActionEmptyTrash: {
            if (FramelessMessageBox::question(this, "清空回收站", "确定要清空回收站吗？回收站内的所有文件将被彻底删除，此操作不可撤销。")) {
                if (DiskTrashService::emptyDiskTrash()) {
                    ToolTipOverlay::instance()->showText(QCursor::pos(), "回收站已清空", 1500, QColor("#2ecc71"));
                }
                refreshAll();
                MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
            }
            break;
        }
=======
        case ActionRestoreAll: {
            QuarkMeta::TrashService::instance().restoreAll(this);
            break;
        }
        case ActionEmptyTrash: {
            QuarkMeta::TrashService::instance().emptyTrash(this);
            break;
        }
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
        case ActionCopy: {
            performCopy(false);
            break;
        }
        case ActionCut: {
            performCopy(true);
            break;
        }
        case ActionPaste: {
            performPaste(isFolder ? path : m_currentPath);
            break;
        }
=======
        case ActionCopy: {
            QuarkMeta::ClipboardService::instance().copyItems(getSelectedPaths());
            break;
        }
        case ActionCut: {
            QuarkMeta::ClipboardService::instance().cutItems(getSelectedPaths());
            break;
        }
        case ActionPaste: {
            QuarkMeta::ClipboardService::instance().executePaste(isFolder ? path : m_currentPath, this);
            break;
        }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **构建集成验证**：
   - 检查新增的三个服务类是否在 `CMakeLists.txt` 中被 MOC 正确拾取和编译。
   - 确认无未解析的外部符号（如 `trashOperationCompleted` 或 `permanentDeleteCompleted` 信号槽链接错误）。

2. **核心逻辑与防护验证**：
   - **移入回收站/还原测试**：验证执行删除后弹出撤销通知，且还原能精确按原物理路径恢复。
   - **深层物理粉碎测试**：验证执行永久删除时在后台线程中异步擦除，且进度条能够平滑显示。
   - **剪贴板防护测试**：验证截图直接粘贴功能、复制/剪切操作，以及防止将父目录复制/剪切放入其子目录的非法操作拦截。
