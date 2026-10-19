#include "FilterStateModel.h"
#include "AppConfig.h"

namespace QuarkMeta {

FilterStateModel::FilterStateModel(QObject* parent) : QObject(parent) {
    m_state.colorTolerance = AppConfig::instance().getValue("Filter/ColorSimilarity", 33).toInt();
    m_state.minColorArea = AppConfig::instance().getValue("Filter/MinColorArea", 0).toInt();
}

void FilterStateModel::setState(const FilterState& state) {
    m_state = state;
    emit stateChanged(m_state);
}

void FilterStateModel::reset(bool force) {
    Q_UNUSED(force);
    FilterState defaultState;
    defaultState.colorTolerance = m_state.colorTolerance;
    defaultState.minColorArea = m_state.minColorArea;
    m_state = defaultState;
    emit stateChanged(m_state);
}

} // namespace QuarkMeta
