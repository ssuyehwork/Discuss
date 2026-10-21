#pragma once

#include <QObject>
#include <QList>
#include <QStringList>
#include "../core/FilterState.h"

namespace QuarkMeta {

class FilterStateModel : public QObject {
    Q_OBJECT
public:
    explicit FilterStateModel(QObject* parent = nullptr);
    ~FilterStateModel() override = default;

    const FilterState& state() const { return m_state; }
    void setState(const FilterState& state);
    void reset(bool force = false);

signals:
    void stateChanged(const FilterState& state);

private:
    FilterState m_state;
};

} // namespace QuarkMeta
