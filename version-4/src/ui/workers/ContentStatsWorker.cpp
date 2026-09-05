#include "ContentStatsWorker.h"
#include "../UiHelper.h"
#include "../../meta/DuplicateDetectorService.h"

#include <QDateTime>
#include <QtConcurrent/QtConcurrent>
#include <QCoreApplication>
#include <QPointer>
#include <cmath>

namespace QuarkMeta {

ContentStatsWorker::ContentStatsWorker(QObject* parent) : QObject(parent) {}

ScanStats ContentStatsWorker::calculateStats(const std::vector<ItemRecord>& records, bool showHidden) {
    ScanStats stats;
    if (records.empty()) return stats;

    stats.duplicatePaths = DuplicateDetectorService::findDuplicatePaths(records);
    stats.duplicateCount = static_cast<int>(stats.duplicatePaths.size());

    for (const auto& record : records) {
        if (record.isHidden && !showHidden) continue;

        stats.ratingCounts[record.rating]++;
        stats.colorCounts[UiHelper::normalizeColorHex(record.manualColor)]++;

        if (record.ctime > 0) {
            QString cDateStr = QDateTime::fromMSecsSinceEpoch(record.ctime).date().toString("dd-MM-yyyy");
            if (!cDateStr.isEmpty()) stats.createDateCounts[cDateStr]++;
        }
        if (record.mtime > 0) {
            QString mDateStr = QDateTime::fromMSecsSinceEpoch(record.mtime).date().toString("dd-MM-yyyy");
            if (!mDateStr.isEmpty()) stats.modifyDateCounts[mDateStr]++;
        }

        if (record.isDir) {
            stats.typeCounts["folder"]++;
            if (record.isEmpty) stats.emptyFolderCount++;
        } else {
            stats.typeCounts["file"]++;
            stats.typeCounts[record.suffix.toUpper()]++;
            if (!record.url.isEmpty()) stats.hasLinkCount++; else stats.noLinkCount++;
            if (!record.note.isEmpty()) stats.hasNoteCount++; else stats.noNoteCount++;
            if (!record.tags.isEmpty()) stats.hasTagCount++; else stats.noTagCount++;

            if (record.width > 0 && record.height > 0) {
                double r = static_cast<double>(record.width) / record.height;
                if (record.width > record.height) stats.ratioHorizontalCount++;
                if (record.height > record.width) stats.ratioVerticalCount++;
                if (std::abs(r - 1.0) <= 0.05) stats.ratioSquareCount++;
                if (std::abs(r - 1.77) <= 0.05) stats.ratio169Count++;
            }

            static const QStringList iconOnlyExts = {"cur", "ico", "ani"};
            QString ext = record.suffix.toLower();
            if (record.thumbStatus == 1) {
                stats.noThumbnailCount++;
            } else if (UiHelper::isGraphicsFile(ext) || (record.width > 0 && record.height > 0)) {
                if (!iconOnlyExts.contains(ext)) {
                    stats.hasThumbnailCount++;
                }
            }
        }
    }

    return stats;
}

void ContentStatsWorker::processAsync(const std::vector<ItemRecord>& records, bool showHidden) {
    if (records.empty()) {
        emit statsReady(ScanStats());
        return;
    }

    QPointer<ContentStatsWorker> weakThis(this);
    (void)QtConcurrent::run([weakThis, records, showHidden]() {
        ScanStats stats = ContentStatsWorker::calculateStats(records, showHidden);
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, stats]() {
            if (weakThis) {
                emit weakThis->statsReady(stats);
            }
        });
    });
}

} // namespace QuarkMeta
