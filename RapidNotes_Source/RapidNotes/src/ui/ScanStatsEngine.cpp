#include "ScanStatsEngine.h"

namespace RapidNotes {

ScanStatsEngine::ScanStatsEngine(QObject* parent) : QObject(parent) {}

void ScanStatsEngine::updateStats(const RapidNotes::ScanStats& stats) {
    m_stats = stats;
    emit statsUpdated(m_stats);
}

} // namespace RapidNotes
