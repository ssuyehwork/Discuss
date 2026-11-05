#pragma once

#include "ItemModelBase.h"
#include <QCache>
#include <QMap>
#include <QIcon>
#include <QMutex>
#include <memory>
#include <vector>
#include "../../core/ItemRecord.h"

namespace RapidNotes {

class DiskItemModel : public ItemModelBase {
    Q_OBJECT
public:
    explicit DiskItemModel(QObject* parent = nullptr) : ItemModelBase(parent) {}
    ~DiskItemModel() override = default;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        if (parent.isValid()) return 0;
        return static_cast<int>(m_records.size());
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        return 7;
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_records.size())) {
            return QVariant();
        }
        const auto& rec = m_records[index.row()];
        if (role == Qt::DisplayRole) {
            return rec.filename.isEmpty() ? rec.path : rec.filename;
        }
        return QVariant();
    }

    const std::vector<RapidNotes::ItemRecord>& allRecords() const override { return m_records; }
    void setRecords(const std::vector<RapidNotes::ItemRecord>& records, RecordSource source = RecordSource::Disk) override {
        beginResetModel();
        m_records = records;
        endResetModel();
    }
    void appendRecord(const RapidNotes::ItemRecord& record) override {
        beginInsertRows(QModelIndex(), static_cast<int>(m_records.size()), static_cast<int>(m_records.size()));
        m_records.push_back(record);
        endInsertRows();
    }
    void clear() override {
        beginResetModel();
        m_records.clear();
        endResetModel();
    }
    void updateRecordMetadata(QString) override {}
    void loadThumbnailsForRows(const QList<int>&) override {}
    void migrateCache(const QString&, const QString&) override {}
    void clearCacheForFolder(const QString&) override {}
    QStringList applyIncrementalChanges(const std::vector<RapidNotes::ItemRecord>&, const QStringList&) override {
        return QStringList();
    }

    void incrementGeneration() {}
    void reloadThumbnailForPath(const QString&) {}

private:
    std::vector<RapidNotes::ItemRecord> m_records;
};

} // namespace RapidNotes
