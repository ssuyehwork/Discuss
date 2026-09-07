#include "FilterStateModel.h"

namespace QuarkMeta {

FilterStateModel::FilterStateModel(QObject* parent) : QObject(parent) {}

void FilterStateModel::setState(const FilterState& state) {
    m_state = state;
    emit stateChanged(m_state);
}

void FilterStateModel::reset(bool force) {
    Q_UNUSED(force);
    m_state = FilterState();
    emit stateChanged(m_state);
}

} // namespace QuarkMeta
