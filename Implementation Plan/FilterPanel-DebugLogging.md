# Implementation Plan - FilterPanel & FilterProxyModel Complete Debug Logging (FilterPanel-DebugLogging.md)

## 1. Overview
本实施方案为 `FilterPanel`（筛选面板）、`FilterStateModel`（筛选状态模型）及 `FilterProxyModel`（筛选代理模型）补充全流程调试日志（`Logger::log`）。
在勾选/取消勾选任意筛选复选框、输入筛选文本、重置筛选状态、广播 `stateChanged` 以及 `FilterProxyModel::updateFilter` 触发时记录清晰的日志，方便在 `quarkmeta_debug.log` 文件中实时追踪筛选器所有点击事件与过滤评估链路。

---

## 2. Modified Files List
- `src/ui/FilterStateModel.cpp`
- `src/ui/FilterPanel.cpp`
- `src/ui/models/FilterProxyModel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/FilterStateModel.cpp`
```
<<<<<<< SEARCH
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
=======
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
>>>>>>> REPLACE
```

### File 2: `src/ui/models/FilterProxyModel.cpp`
```
<<<<<<< SEARCH
#include "FilterProxyModel.h"
#include "../ContentPanel.h"
#include "../UiHelper.h"
#include <QDateTime>
#include <cmath>

namespace QuarkMeta {

FilterProxyModel::FilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

void FilterProxyModel::updateFilter() {
    beginFilterChange();
    endFilterChange();
}
=======
#include "FilterProxyModel.h"
#include "../ContentPanel.h"
#include "../UiHelper.h"
#include "../Logger.h"
#include <QDateTime>
#include <cmath>

namespace QuarkMeta {

FilterProxyModel::FilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

void FilterProxyModel::updateFilter() {
    Logger::log(QString("[FilterProxyModel::updateFilter] Invalidate filter. ShowFolders: %1, ShowFiles: %2, Ratings: %3, Colors: %4, Types: %5, Kw: '%6'")
                .arg(currentFilter.showFolders).arg(currentFilter.showFiles)
                .arg(currentFilter.ratings.size())
                .arg(currentFilter.colors.join(","))
                .arg(currentFilter.types.join(","))
                .arg(currentFilter.keyword));
    beginFilterChange();
    endFilterChange();
}
>>>>>>> REPLACE
```

### File 3: `src/ui/FilterPanel.cpp`
```
<<<<<<< SEARCH
    connect(m_filterModel, &FilterStateModel::stateChanged, this, [this](const FilterState& st) {
        m_filter = st;
        emit filterChanged(st);
        updateHeaderStatus();
    });
=======
    connect(m_filterModel, &FilterStateModel::stateChanged, this, [this](const FilterState& st) {
        m_filter = st;
        Logger::log(QString("[FilterPanel] Emitting filterChanged signal for updated state."));
        emit filterChanged(st);
        updateHeaderStatus();
    });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void FilterPanel::clearAllFilters(bool force) {
    if (!force && m_isFilterPinned) {
        return;
    }
=======
void FilterPanel::clearAllFilters(bool force) {
    Logger::log(QString("[FilterPanel::clearAllFilters] Force: %1, Pinned: %2").arg(force).arg(m_isFilterPinned));
    if (!force && m_isFilterPinned) {
        return;
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. 编译并启动程序，在侧边栏筛选面板上点击选择/勾选任何复选框（如“已标签”、“无评级”、“红色”、“16:9”等）。
2. 打开项目根目录下的 `quarkmeta_debug.log` 文件，确认每一条点击事件均输出对应 `[FilterStateModel::setState]`、`[FilterPanel]` 及 `[FilterProxyModel::updateFilter]` 日志条目。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 统一使用 `QuarkMeta::Logger::log` 日志接口。

---

## 6. Header API Signature Verification
- `Logger::log(const QString& msg)` 定义于 `src/ui/Logger.h`。

---

## 7. Header Inclusion Chain & Type Completeness Check
- 各涉及 `.cpp` 文件均包含 `"Logger.h"`，编译类型完整。
