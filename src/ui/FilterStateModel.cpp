#include "FilterStateModel.h"
#include "Logger.h"

namespace QuarkMeta {

FilterStateModel::FilterStateModel(QObject* parent) : QObject(parent) {}

void FilterStateModel::setState(const FilterState& state) {
    m_state = state;
    Logger::log(QString("[FilterStateModel::setState] Ratings: %1, Colors: %2, Types: %3, Ratio: %4, ThumbPresence: %5, Kw: '%6'")
                .arg(m_state.ratings.size())
                .arg(m_state.colors.join(","))
                .arg(m_state.types.join(","))
                .arg(static_cast<int>(m_state.ratio))
                .arg(static_cast<int>(m_state.thumbnailPresence))
                .arg(m_state.keyword));
    emit stateChanged(m_state);
}

void FilterStateModel::reset(bool force) {
    Logger::log(QString("[FilterStateModel::reset] Resetting state (force: %1)").arg(force));
    m_state = FilterState();
    emit stateChanged(m_state);
}

} // namespace QuarkMeta
