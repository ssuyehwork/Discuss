#pragma once

#include <QObject>
#include <vector>
#include "../ScanStats.h"
#include "../../core/ItemRecord.h"

namespace QuarkMeta {

class ContentStatsWorker : public QObject {
    Q_OBJECT
public:
    explicit ContentStatsWorker(QObject* parent = nullptr);
    ~ContentStatsWorker() override = default;

    static ScanStats calculateStats(const std::vector<ItemRecord>& records, bool showHidden);
    void processAsync(const std::vector<ItemRecord>& records, bool showHidden);

signals:
    void statsReady(const QuarkMeta::ScanStats& stats);
};

} // namespace QuarkMeta
