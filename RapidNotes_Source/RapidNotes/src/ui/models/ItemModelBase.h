#ifndef ITEMMODELBASE_H
#define ITEMMODELBASE_H

#include <QAbstractTableModel>
#include <QHash>
#include <QSet>
#include <QTimer>
#include <algorithm>
#include <vector>
#include "../../core/ItemRecord.h"

namespace QuarkMeta {
    enum class RecordSource {
        Disk,
        Prepared
    };

    class ItemModelBase : public QAbstractTableModel {
        Q_OBJECT
    public:
        explicit ItemModelBase(QObject* parent = nullptr) : QAbstractTableModel(parent) {}
        virtual ~ItemModelBase() override = default;

        virtual QString currentPath() const { return m_currentPath; }
        virtual void setCurrentPath(const QString& path) { m_currentPath = path; }

        virtual const std::vector<QuarkMeta::ItemRecord>& allRecords() const = 0;
        virtual void setRecords(const std::vector<QuarkMeta::ItemRecord>& records, RecordSource source = RecordSource::Disk) = 0;
        virtual void appendRecord(const QuarkMeta::ItemRecord& record) = 0;
        virtual void clear() = 0;
        virtual void updateRecordMetadata(QString path) = 0;
        virtual void updateRecordMetadataBatch(const QStringList& paths) {
            for (const QString& p : paths) updateRecordMetadata(p);
        }
        virtual void loadThumbnailsForRows(const QList<int>& rows) = 0;
        virtual void migrateCache(const QString& oldPath, const QString& newPath) = 0;
        virtual void clearCacheForFolder(const QString& folderPath) = 0;
        virtual QStringList applyIncrementalChanges(const std::vector<QuarkMeta::ItemRecord>& records, const QStringList& removedPaths) = 0;

    protected:
        QString m_currentPath;
    };
}

using ItemModelBase = QuarkMeta::ItemModelBase;

#endif // ITEMMODELBASE_H
