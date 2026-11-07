#pragma once

#include <QObject>
#include "ScanStats.h"

namespace RapidNotes {

class ScanStatsEngine : public QObject {
    Q_OBJECT
public:
    explicit ScanStatsEngine(QObject* parent = nullptr);
    ~ScanStatsEngine() override = default;

    void updateStats(const RapidNotes::ScanStats& stats);
    const RapidNotes::ScanStats& currentStats() const { return m_stats; }

signals:
    void statsUpdated(const RapidNotes::ScanStats& stats);

private:
    RapidNotes::ScanStats m_stats;
};

} // namespace RapidNotes
