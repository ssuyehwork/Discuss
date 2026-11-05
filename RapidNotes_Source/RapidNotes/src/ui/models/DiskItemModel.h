#pragma once

#include "ItemModelBase.h"
#include <QCache>
#include <QMap>
#include <QIcon>
#include <QMutex>
#include <memory>
#include <unordered_map>
#include <QSet>
#include <QPointer>
#include <QTimer>
#include "../core/ItemRecord.h"

namespace QuarkMeta {

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

    const std::vector<QuarkMeta::ItemRecord>& allRecords() const override { return m_records; }
    void setRecords(const std::vector<QuarkMeta::ItemRecord>& records, RecordSource source = RecordSource::Disk) override {
        beginResetModel();
        m_records = records;
        endResetModel();
    }
    void appendRecord(const QuarkMeta::ItemRecord& record) override {
        beginInsertRows(QModelIndex(), static_cast<int>(m_records.size()), static_cast<int>(m_records.size()));
        m_records.push_back(record);
        endInsertRows();
    }
    void clear() override {
        beginResetModel();
        m_records.clear();
        endResetModel();
    }
    void updateRecordMetadata(QString path) override {}
    void loadThumbnailsForRows(const QList<int>& rows) override {}
    void migrateCache(const QString& oldPath, const QString& newPath) override {}
    void clearCacheForFolder(const QString& folderPath) override {}
    QStringList applyIncrementalChanges(const std::vector<QuarkMeta::ItemRecord>& records, const QStringList& removedPaths) override {
        return QStringList();
    }

    void incrementGeneration() {}
    void reloadThumbnailForPath(const QString& path) {}

private:
    std::vector<QuarkMeta::ItemRecord> m_records;
};

} // namespace QuarkMeta
