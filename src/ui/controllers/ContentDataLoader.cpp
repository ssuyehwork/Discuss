#include "ContentDataLoader.h"
#include "../ContentPanel.h"
#include "../DiskScanService.h"
#include "../../meta/DiskTrashRepo.h"
#include "../../meta/MetaCacheDecorator.h"
#include "../../meta/MediaExtractorPipeline.h"
#include "../../util/ThumbnailPipelineService.h"

#include <QDir>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>
#include <QCoreApplication>
#include <QPointer>

namespace QuarkMeta {

ContentDataLoader::ContentDataLoader(ContentPanel* panel)
    : QObject(panel), m_panel(panel) {}

static std::vector<ItemRecord> loadTrashItemsDirect() {
    std::vector<ItemRecord> records;
    auto rawDiskTrash = DiskTrashRepo::getAllTrashItems();
    for (const auto& raw : rawDiskTrash) {
        ItemRecord rec;
        rec.isDiskTrash = true;
        rec.diskTrashId = raw.id;
        rec.fileId = QString::fromStdWString(raw.fileId);
        rec.path = QString::fromStdWString(raw.trashPath);
        rec.originalPath = QString::fromStdWString(raw.originalPath);
        rec.filename = QString::fromStdWString(raw.fileName);
        rec.suffix = QFileInfo(rec.filename).suffix();
        rec.isDir = raw.isFolder;
        rec.size = raw.fileSize;
        rec.ctime = raw.createdAt > 0 ? raw.createdAt : raw.deletedAt;
        rec.mtime = raw.deletedAt;
        records.push_back(rec);
    }
    MetaCacheDecorator::decorate(records);
    return records;
}

void ContentDataLoader::loadDirectory(const QString& path, bool recursive) {
    if (!m_panel) return;
    m_panel->restoreActiveView();
    MediaExtractorPipeline::instance().cancelAll();
    if (m_panel->diskModel()) {
        m_panel->diskModel()->incrementGeneration();
    }
    m_panel->ensureSourceModelIsDiskModel();
    ThumbnailPipelineService::instance().cancelAll();

    m_panel->setLoading(true);
    int reqId = ++m_loadRequestId;
    m_panel->setCurrentCategoryType("");
    emit m_panel->dataSourceChanged("nav");

    m_panel->setIsRecursive(recursive);

    if (path.isEmpty() || path == "computer://") {
        m_panel->setCurrentPath("computer://");
        m_panel->updateLayersButtonState();
        std::vector<ItemRecord> driveRecords;
        for (const QFileInfo& drive : QDir::drives()) {
            driveRecords.push_back(ItemRecord::create(drive.absolutePath()));
        }
        MetaCacheDecorator::decorate(driveRecords);
        if (m_panel->model()) {
            m_panel->model()->setRecords(driveRecords);
        }
        m_panel->applySort();
        m_panel->setLoading(false);
        m_panel->recalculateAndEmitStats();
        return;
    }

    m_panel->setCurrentPath(path);
    m_panel->updateLayersButtonState();

    QPointer<ContentPanel> panelPtr(m_panel);
    (void)QtConcurrent::run([panelPtr, path, recursive, reqId]() {
        if (!panelPtr) return;
        std::vector<ItemRecord> allItems = DiskScanService::scanDirectory(path, recursive, [panelPtr]() { return static_cast<bool>(panelPtr); });
        if (!panelPtr) return;

        MetaCacheDecorator::decorate(allItems);

        QMetaObject::invokeMethod(QCoreApplication::instance(), [panelPtr, allItems, reqId]() {
            if (panelPtr && panelPtr->loadRequestId() == reqId) {
                if (panelPtr->model()) {
                    panelPtr->model()->setRecords(allItems);
                }
                panelPtr->applySort();
                panelPtr->setLoading(false);
                panelPtr->recalculateAndEmitStats();
                panelPtr->applyFilters();
                panelPtr->restoreSelections();
                panelPtr->startVisibleTimer();
            }
        }, Qt::QueuedConnection);
    });
}

void ContentDataLoader::loadCategory(const QString& categoryType) {
    if (!m_panel) return;
    m_panel->setCurrentCategoryType(categoryType);
    if (categoryType == "trash") {
        m_panel->setCurrentPath("trash://");
        loadPaths({});
    }
}

void ContentDataLoader::loadPaths(const QStringList& paths, int reqId) {
    if (!m_panel) return;
    m_panel->restoreActiveView();
    m_panel->ensureSourceModelIsDiskModel();

    if (paths.isEmpty() && m_panel->getCurrentCategoryType() != "trash") {
        if (m_panel->model()) {
            m_panel->model()->clear();
        }
        m_panel->setLoading(false);
        m_panel->recalculateAndEmitStats();
        return;
    }

    m_panel->setLoading(true);
    if (reqId == 0) {
        reqId = ++m_loadRequestId;
    }
    if (m_panel->getCurrentCategoryType().isEmpty()) {
        m_panel->setCurrentCategoryType("path_list");
    }
    m_panel->updateLayersButtonState();

    QPointer<ContentPanel> weakPanel(m_panel);
    (void)QtConcurrent::run([weakPanel, paths, reqId]() {
        if (!weakPanel) return;
        std::vector<ItemRecord> records;

        if (weakPanel->getCurrentCategoryType() == "trash") {
            records = loadTrashItemsDirect();
        } else {
            for (const QString& p : paths) {
                records.push_back(ItemRecord::create(p));
            }
            MetaCacheDecorator::decorate(records);
        }
        if (!weakPanel) return;

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, records, reqId]() {
            if (weakPanel && weakPanel->loadRequestId() == reqId) {
                if (weakPanel->model()) {
                    weakPanel->model()->setRecords(records);
                }
                weakPanel->applySort();
                weakPanel->setLoading(false);
                weakPanel->recalculateAndEmitStats();
                weakPanel->applyFilters();
                weakPanel->restoreSelections();
            }
        });
    });
}

void ContentDataLoader::appendPaths(const QStringList& paths, int reqId) {
    if (!m_panel || paths.isEmpty() || (reqId != 0 && m_loadRequestId != reqId)) return;
    QPointer<ContentPanel> weakPanel(m_panel);
    (void)QtConcurrent::run([weakPanel, paths, reqId]() {
        if (!weakPanel) return;
        std::vector<ItemRecord> newRecs;
        for (const QString& p : paths) {
            newRecs.push_back(ItemRecord::create(p));
        }
        MetaCacheDecorator::decorate(newRecs);
        if (!weakPanel) return;

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, newRecs, reqId]() {
            if (weakPanel && (reqId == 0 || weakPanel->loadRequestId() == reqId)) {
                if (weakPanel->model()) {
                    std::vector<ItemRecord> all = weakPanel->model()->allRecords();
                    all.insert(all.end(), newRecs.begin(), newRecs.end());
                    weakPanel->model()->setRecords(all);
                }
                weakPanel->recalculateAndEmitStats();
                weakPanel->applyFilters();
            }
        });
    });
}

} // namespace QuarkMeta
