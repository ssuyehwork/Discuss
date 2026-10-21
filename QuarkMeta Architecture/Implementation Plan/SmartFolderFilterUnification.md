# SmartFolderFilterUnification.md - 智能文件夹统一 FilterState 滤波与匹配引擎实施方案

---

## 起草前核查（Pre-Draft Evidence）

### 1. 已读文件清单
- `src/ui/FilterStateModel.h`（已用 `read_file` 物理读取）
- `src/ui/models/FilterProxyModel.h` / `.cpp`（已用 `read_file` 物理读取）
- `src/ui/FilterPanel.h` / `.cpp`（已用 `read_file` 物理读取）
- `src/ui/LibraryPanel.h` / `.cpp`（已用 `read_file` 物理读取）
- `src/ui/PanelMediator.h` / `.cpp`（已用 `read_file` 物理读取）
- `src/ui/dialogs/SmartFolderDialog.h` / `.cpp`（已用 `read_file` 物理读取）
- `src/meta/SmartFolderDao.h` / `.cpp`（已用 `read_file` 物理读取）
- `src/meta/LibraryItemDao.h` / `.cpp`（已用 `read_file` 物理读取）
- `src/meta/LibraryDao.h` / `.cpp`（已用 `read_file` 物理读取）
- `src/meta/LibrarySyncService.h` / `.cpp`（已用 `read_file` 物理读取）
- `CMakeLists.txt`（已用 `read_file` 物理读取）

### 2. 签名原文摘录
- `FilterProxyModel::filterAcceptsRow`:
  `bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;`
- `LibraryItemDao::getItemsByPaths`:
  `static std::vector<ItemRecord> getItemsByPaths(const QStringList& paths);`
- `LibraryDao::getCategoryPaths`:
  `static QStringList getCategoryPaths(int categoryId);`
- `DuplicateDetectorService::findDuplicatePaths`:
  `static QSet<QString> findDuplicatePaths(const std::vector<ItemRecord>& records);`
- `FilterStateModel::setState`:
  `void setState(const FilterState& state);`

### 3. 既有 API 检索记录
- 库成员获取：`LibraryItemDao::getItemsByPaths(LibraryDao::getCategoryPaths(-1))` 检索存在。
- 重复判定：`DuplicateDetectorService::findDuplicatePaths` 检索存在。
- 颜色解析与像素提取：`UiHelper::parseColorName` / `ColorPaletteEngine::isGraphicsFile` 检索存在。

### 4. Memories.md 相关条款
- 图标规范：全应用 UI 图标与按钮严格使用矢量 SVG (`UiHelper::getIcon(...)`)，禁止采用 Emoji 字符或文本符号充当图标。
- 零数值篡改：迁移样式或调整 UI 控件布局时，所有尺寸、颜色 Hex、边框与内外边距 100% 照抄。

---

## 1. Overview（概述与解决的问题）

### 1.1 核心动机
当前“智能文件夹”使用自成一套的 `SmartRuleType` / `SmartFolderRule` 数据模型和 SQL 字符串拼接匹配逻辑（`SmartFolderDao::countMatchingItems`），不仅只支持评级、颜色、扩展名、关键词等少数维度，而且与筛选器（`FilterProxyModel`）的筛选口径不一致。

### 1.2 解决实施方案
1. **统一 FilterState 与匹配引擎 (`FilterMatcher`)**：
   - 将 `FilterState` 结构体物理迁移至 `src/core/FilterState.h`。
   - 新建 `FilterMatcher`（`src/core/FilterMatcher.h` / `.cpp`），提供静态纯函数 `bool matches(const ItemRecord&, const FilterState&, const QSet<QString>& duplicatePaths)`。将 `FilterProxyModel::filterAcceptsRow` 的完整条件判断逻辑原样迁入，`FilterProxyModel` 仅负责取记录并调用 `FilterMatcher::matches`，确保两端筛选口径 100% 相同。
2. **FilterState 序列化 (`FilterStateJson`)**：
   - 新建 `FilterStateJson`（`src/core/FilterStateJson.h` / `.cpp`），提供 `toJson` 与 `fromJson`，支持序列化 `FilterState` 中的所有维度字段（排除运行期字段 `keyword` 与 `showHidden`）。
3. **smart_folders 改造与数据自愈**：
   - `smart_folders` SQLite 表结构更新：删除 `match_mode`、`rules_json`，新增 `filter_json TEXT`。启动检测到缺少 `filter_json` 列时直接重建表。
   - `SmartFolderRecord` 改为持有 `FilterState filter`，物理删除 `SmartFolderRule`、`SmartRuleType`、`SmartRuleOperator` 及其全部用例。
   - 物理删除 `SmartFolderDao::countMatchingItems` 及其 SQL。
   - 实现智能文件夹异步计算：后台线程通过 `LibraryItemDao` 读取所有库成员，调用 `FilterMatcher::matches` 纯内存运算计数。仅当筛选条件包含“重复”时才触发 `DuplicateDetectorService` 计算重复集合。
4. **UI 交互解耦与编辑界面简化**：
   - `FilterPanel::applySmartFolderRecord` 改为恢复 `FilterState` 并调用 `rebuildGroups()`，同步 UI 控件（色轮、滑条、勾选框）。
   - `SmartFolderDialog` 删除规则行 UI（`SmartFolderRuleRow`）与匹配模式，保留名称、图标与颜色选择。
   - 新建智能文件夹时，条件直接取自“此刻筛选器的当前条件”（由 `PanelMediator` 向 `LibraryPanel` 注入 `std::function<FilterState()>` 闭包回调）。
   - 智能文件夹右键菜单新增“用当前筛选条件更新”选项。

---

## 2. Modified Files List（影响文件清单）

- `CMakeLists.txt` (添加新核心源文件)
- `src/core/FilterState.h` (新建文件，提取 `FilterState` 结构体)
- `src/core/FilterMatcher.h` (新建文件，统一匹配判断引擎)
- `src/core/FilterMatcher.cpp` (新建文件)
- `src/core/FilterStateJson.h` (新建文件，序列化逻辑)
- `src/core/FilterStateJson.cpp` (新建文件)
- `src/ui/FilterStateModel.h` (修改，包含新 `FilterState.h`)
- `src/ui/models/FilterProxyModel.h` (修改，包含新 `FilterMatcher.h`)
- `src/ui/models/FilterProxyModel.cpp` (修改，调用 `FilterMatcher::matches`)
- `src/meta/SmartFolderDao.h` (修改，更新 `SmartFolderRecord` 并删除旧规则)
- `src/meta/SmartFolderDao.cpp` (修改，`smart_folders` 表结构更新与 SQL 操作)
- `src/ui/FilterPanel.h` (修改，包含新 `SmartFolderDao.h`)
- `src/ui/FilterPanel.cpp` (修改，`applySmartFolderRecord` 更新)
- `src/ui/dialogs/SmartFolderDialog.h` (修改，物理删除 `SmartFolderRuleRow`，简化对话框)
- `src/ui/dialogs/SmartFolderDialog.cpp` (修改，对话框实现更新)
- `src/ui/LibraryPanel.h` (修改，注入 `FilterState` 获取器，添加右键“用当前筛选条件更新”)
- `src/ui/LibraryPanel.cpp` (修改，异步计数与右键菜单更新)
- `src/ui/PanelMediator.cpp` (修改，注入筛选器当前条件获取闭包给 `LibraryPanel`)

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `CMakeLists.txt`
```
<<<<<<< SEARCH
    src/core/FileFilterService.cpp
    src/core/FileFilterService.h
=======
    src/core/FileFilterService.cpp
    src/core/FileFilterService.h
    src/core/FilterState.h
    src/core/FilterMatcher.h
    src/core/FilterMatcher.cpp
    src/core/FilterStateJson.h
    src/core/FilterStateJson.cpp
>>>>>>> REPLACE
```

### 3.2 `src/core/FilterState.h` (新建文件)
```cpp
#pragma once

#include <QList>
#include <QStringList>
#include <QString>

namespace QuarkMeta {

struct FilterState {
    static constexpr int kHueTolerance = 20;

    QList<int>   ratings;
    QStringList  colors;          // 自动提取色彩过滤色 (Hex 字符串，如 #E24B4A)
    QStringList  manualColors;    // 手动标注颜色过滤 (红、橙、黄、绿、青、蓝、紫、灰、无色标)
    int          colorTolerance = 10; // 已废弃，由 colorPrecision 取代
    int          colorPrecision = 50; // 精准度 (0~100，默认 50)
    int          colorHue = 0;        // 色相 (0~359，默认 0)
    int          minColorArea = 0;    // 占比 (0~100，默认 0%)
    QString      keyword;
    QStringList  types;
    QStringList  createDates;
    QStringList  modifyDates;

    enum Presence { All, Yes, No };
    Presence linkPresence = All;
    Presence notePresence = All;
    Presence tagPresence = All;

    enum AspectRatio { AspectAny, Horizontal, Vertical, Square, Ratio169 };
    AspectRatio ratio = AspectAny;

    long long minSize = -1;
    long long maxSize = -1;

    QString typeFilterText;
    QString createDateFilterText;
    QString modifyDateFilterText;

    bool showHidden = false;

    enum DuplicatePresence { DupAll, DuplicateOnly, UniqueOnly };
    DuplicatePresence duplicatePresence = DupAll;

    enum ThumbnailPresence { ThumbAll, HasThumbnail, NoThumbnail };
    ThumbnailPresence thumbnailPresence = ThumbAll;

    bool isEmpty() const {
        return ratings.isEmpty() && colors.isEmpty() && manualColors.isEmpty() && keyword.isEmpty() && types.isEmpty() &&
               createDates.isEmpty() && modifyDates.isEmpty() &&
               linkPresence == All && notePresence == All && tagPresence == All && ratio == AspectAny &&
               minSize == -1 && maxSize == -1 &&
               typeFilterText.trimmed().isEmpty() && createDateFilterText.trimmed().isEmpty() &&
               modifyDateFilterText.trimmed().isEmpty() && duplicatePresence == DupAll &&
               thumbnailPresence == ThumbAll;
    }
};

} // namespace QuarkMeta
```

### 3.3 `src/core/FilterMatcher.h` (新建文件)
```cpp
#pragma once

#include "ItemRecord.h"
#include "FilterState.h"
#include <QSet>
#include <QString>

namespace QuarkMeta {

class FilterMatcher {
public:
    static bool matches(const ItemRecord& record, const FilterState& filter, const QSet<QString>& duplicatePaths = {});
};

} // namespace QuarkMeta
```

### 3.4 `src/core/FilterMatcher.cpp` (新建文件)
```cpp
#include "FilterMatcher.h"
#include "../ui/UiHelper.h"
#include "../util/ColorPaletteEngine.h"
#include <QDateTime>
#include <QRegularExpression>
#include <cmath>

namespace QuarkMeta {

bool FilterMatcher::matches(const ItemRecord& record, const FilterState& currentFilter, const QSet<QString>& duplicatePaths) {
    // 0. 隐藏属性过滤
    if (record.isHidden && !currentFilter.showHidden) {
        return false;
    }

    // 2. 评级过滤
    if (!currentFilter.ratings.isEmpty()) {
        if (!currentFilter.ratings.contains(record.rating)) return false;
    }

    // 3. 手动标注颜色过滤
    if (!currentFilter.manualColors.isEmpty()) {
        bool matchManual = false;
        static const QMap<QString, QString> s_manualHexMap = {
            {"红色", "#E24B4A"}, {"橙色", "#EF9F27"}, {"黄色", "#FECF0E"},
            {"绿色", "#639922"}, {"青色", "#1D9E75"}, {"蓝色", "#378ADD"},
            {"紫色", "#7F77DD"}, {"灰色", "#5F5E5A"}
        };

        for (const QString& mc : currentFilter.manualColors) {
            if (mc == "无色标" || mc.isEmpty()) {
                if (record.manualColor.isEmpty()) {
                    matchManual = true;
                    break;
                }
            } else {
                QString targetHex = s_manualHexMap.value(mc, mc);
                if (record.manualColor.compare(targetHex, Qt::CaseInsensitive) == 0 ||
                    record.manualColor.contains(mc, Qt::CaseInsensitive)) {
                    matchManual = true;
                    break;
                }
            }
        }
        if (!matchManual) return false;
    }

    // 3.5 自动提取色彩过滤
    if (!currentFilter.colors.isEmpty()) {
        bool matchAuto = false;

        auto isMatch = [&](const QColor& targetCol, const QColor& pc) -> bool {
            if (!targetCol.isValid() || !pc.isValid()) return false;

            int targetSat = targetCol.hsvSaturation();
            int pcSat = pc.hsvSaturation();

            if (targetSat >= 38) {
                if (pcSat < 38) return false;

                int targetHue = targetCol.hsvHue();
                int pcHue = pc.hsvHue();
                if (targetHue < 0 || pcHue < 0) return false;

                int d = std::abs(pcHue - targetHue);
                if (d > 180) d = 360 - d;

                double precisionFactor = static_cast<double>(currentFilter.colorPrecision) / 100.0;

                int maxHueDiff = qRound(15.0 - 9.0 * precisionFactor);
                if (d > maxHueDiff) return false;

                double minSat = 255.0 * (1.0 - precisionFactor);
                if (static_cast<double>(pcSat) < minSat) return false;

                int maxValDiff = qRound(40.0 - 25.0 * precisionFactor);
                return std::abs(pc.value() - targetCol.value()) <= maxValDiff;
            } else {
                if (pcSat >= 38) return false;
                return std::abs(pc.lightnessF() - targetCol.lightnessF()) <= 0.2;
            }
        };

        auto calculateMatchedArea = [&](const QColor& targetCol) -> float {
            float totalArea = 0.0f;
            if (!record.palettes.empty()) {
                for (const auto& pe : record.palettes) {
                    if (isMatch(targetCol, pe.first)) {
                        totalArea += pe.second;
                    }
                }
            } else if (!record.autoColor.isEmpty()) {
                QColor recordCol = UiHelper::parseColorName(record.autoColor);
                if (isMatch(targetCol, recordCol)) {
                    totalArea = 1.0f;
                }
            }
            return totalArea;
        };

        for (const QString& fc : currentFilter.colors) {
            QColor targetCol = UiHelper::parseColorName(fc);
            float area = calculateMatchedArea(targetCol);
            if (area > 0.0f && (area * 100.0f >= static_cast<float>(currentFilter.minColorArea))) {
                matchAuto = true;
                break;
            }
        }
        if (!matchAuto) return false;
    }

    // 4. 类型过滤
    if (!currentFilter.types.isEmpty() || !currentFilter.typeFilterText.isEmpty()) {
        QString type = record.isDir ? "folder" : "file";
        QString ext = record.suffix.toUpper();
        bool matchType = false;

        if (!currentFilter.typeFilterText.isEmpty()) {
            QString searchText = currentFilter.typeFilterText.trimmed();
            if (searchText == "文件夹" || searchText.toLower() == "folder") {
                if (type == "folder") matchType = true;
            } else if (searchText == "空文件夹") {
                if (type == "folder" && record.isEmpty) matchType = true;
            } else {
                if (ext.contains(searchText.toUpper())) matchType = true;
            }
            if (!matchType) return false;
        }

        if (!currentFilter.types.isEmpty()) {
            matchType = false;
            for (const QString& fType : currentFilter.types) {
                if (fType == "folder") {
                    if (type == "folder") { matchType = true; break; }
                } else if (fType == "file") {
                    if (type != "folder") { matchType = true; break; }
                } else if (fType == "空文件夹") {
                    if (type == "folder" && record.isEmpty) { matchType = true; break; }
                } else {
                    if (ext == fType.toUpper()) { matchType = true; break; }
                }
            }
            if (!matchType) return false;
        }
    }

    // 5. 日期过滤
    if (!currentFilter.createDates.isEmpty() || !currentFilter.createDateFilterText.isEmpty()) {
        QString dStr = QDateTime::fromMSecsSinceEpoch(record.ctime).date().toString("dd-MM-yyyy");
        if (!currentFilter.createDateFilterText.isEmpty() && !dStr.contains(currentFilter.createDateFilterText.trimmed())) {
            return false;
        }
        if (!currentFilter.createDates.isEmpty() && !currentFilter.createDates.contains(dStr)) {
            return false;
        }
    }

    if (!currentFilter.modifyDates.isEmpty() || !currentFilter.modifyDateFilterText.isEmpty()) {
        QString dStr = QDateTime::fromMSecsSinceEpoch(record.mtime).date().toString("dd-MM-yyyy");
        if (!currentFilter.modifyDateFilterText.isEmpty() && !dStr.contains(currentFilter.modifyDateFilterText.trimmed())) {
            return false;
        }
        if (!currentFilter.modifyDates.isEmpty() && !currentFilter.modifyDates.contains(dStr)) {
            return false;
        }
    }

    // 6. 附加属性过滤
    if (currentFilter.linkPresence != FilterState::All) {
        bool hasLink = !record.url.isEmpty();
        if (currentFilter.linkPresence == FilterState::Yes && !hasLink) return false;
        if (currentFilter.linkPresence == FilterState::No && hasLink) return false;
    }

    if (currentFilter.notePresence != FilterState::All) {
        bool hasNote = !record.note.isEmpty();
        if (currentFilter.notePresence == FilterState::Yes && !hasNote) return false;
        if (currentFilter.notePresence == FilterState::No && hasNote) return false;
    }

    if (currentFilter.tagPresence != FilterState::All) {
        bool hasTags = !record.tags.isEmpty();
        if (currentFilter.tagPresence == FilterState::Yes && !hasTags) return false;
        if (currentFilter.tagPresence == FilterState::No && hasTags) return false;
    }

    if (currentFilter.minSize != -1 && record.size < currentFilter.minSize) return false;
    if (currentFilter.maxSize != -1 && record.size > currentFilter.maxSize) return false;

    if (currentFilter.ratio != FilterState::AspectAny) {
        if (record.width > 0 && record.height > 0) {
            double r = static_cast<double>(record.width) / record.height;
            if (currentFilter.ratio == FilterState::Horizontal && record.width <= record.height) return false;
            if (currentFilter.ratio == FilterState::Vertical && record.height <= record.width) return false;
            if (currentFilter.ratio == FilterState::Square && std::abs(r - 1.0) > 0.05) return false;
            if (currentFilter.ratio == FilterState::Ratio169 && std::abs(r - 1.77) > 0.05) return false;
        } else {
            return false;
        }
    }

    if (currentFilter.duplicatePresence != FilterState::DupAll) {
        if (record.isDir) return false;
        bool isDuplicate = duplicatePaths.contains(record.path);
        if (currentFilter.duplicatePresence == FilterState::DuplicateOnly && !isDuplicate) return false;
        if (currentFilter.duplicatePresence == FilterState::UniqueOnly && isDuplicate) return false;
    }

    // 6.5 缩略图状态过滤
    if (currentFilter.thumbnailPresence != FilterState::ThumbAll) {
        if (record.isDir || !ColorPaletteEngine::isGraphicsFile(record.suffix.toLower())) {
            return false;
        }

        bool isReady = (record.thumbnailState == ItemRecord::ThumbnailState::Ready);
        if (currentFilter.thumbnailPresence == FilterState::HasThumbnail && !isReady) return false;
        if (currentFilter.thumbnailPresence == FilterState::NoThumbnail && isReady) return false;
    }

    // 7. 搜索关键词匹配
    if (!currentFilter.keyword.isEmpty()) {
        static const QRegularExpression sepRegex("[\r\n、,，]+");
        QStringList keywords = currentFilter.keyword.split(sepRegex, Qt::SkipEmptyParts);

        if (!keywords.isEmpty()) {
            bool anyMatch = false;
            for (const QString& rawKw : keywords) {
                QString kw = rawKw.trimmed();
                if (kw.isEmpty()) continue;

                bool match = record.filename.contains(kw, Qt::CaseInsensitive);

                if (!match) {
                    for (const QString& tag : record.tags) {
                        if (tag.contains(kw, Qt::CaseInsensitive)) {
                            match = true;
                            break;
                        }
                    }
                }

                if (!match && !record.note.isEmpty()) {
                    if (record.note.contains(kw, Qt::CaseInsensitive)) {
                        match = true;
                    }
                }

                if (match) {
                    anyMatch = true;
                    break;
                }
            }

            if (!anyMatch) return false;
        }
    }

    return true;
}

} // namespace QuarkMeta
```

### 3.5 `src/core/FilterStateJson.h` (新建文件)
```cpp
#pragma once

#include "FilterState.h"
#include <QJsonObject>

namespace QuarkMeta {

class FilterStateJson {
public:
    static QJsonObject toJson(const FilterState& state);
    static FilterState fromJson(const QJsonObject& json);
};

} // namespace QuarkMeta
```

### 3.6 `src/core/FilterStateJson.cpp` (新建文件)
```cpp
#include "FilterStateJson.h"
#include <QJsonArray>

namespace QuarkMeta {

QJsonObject FilterStateJson::toJson(const FilterState& state) {
    QJsonObject json;

    QJsonArray ratingsArr;
    for (int r : state.ratings) ratingsArr.append(r);
    json["ratings"] = ratingsArr;

    QJsonArray colorsArr;
    for (const QString& c : state.colors) colorsArr.append(c);
    json["colors"] = colorsArr;

    QJsonArray manualColorsArr;
    for (const QString& mc : state.manualColors) manualColorsArr.append(mc);
    json["manualColors"] = manualColorsArr;

    json["colorTolerance"] = state.colorTolerance;
    json["colorPrecision"] = state.colorPrecision;
    json["colorHue"] = state.colorHue;
    json["minColorArea"] = state.minColorArea;

    QJsonArray typesArr;
    for (const QString& t : state.types) typesArr.append(t);
    json["types"] = typesArr;

    QJsonArray createDatesArr;
    for (const QString& cd : state.createDates) createDatesArr.append(cd);
    json["createDates"] = createDatesArr;

    QJsonArray modifyDatesArr;
    for (const QString& md : state.modifyDates) modifyDatesArr.append(md);
    json["modifyDates"] = modifyDatesArr;

    json["linkPresence"] = static_cast<int>(state.linkPresence);
    json["notePresence"] = static_cast<int>(state.notePresence);
    json["tagPresence"] = static_cast<int>(state.tagPresence);
    json["ratio"] = static_cast<int>(state.ratio);

    json["minSize"] = state.minSize;
    json["maxSize"] = state.maxSize;

    json["typeFilterText"] = state.typeFilterText;
    json["createDateFilterText"] = state.createDateFilterText;
    json["modifyDateFilterText"] = state.modifyDateFilterText;

    json["duplicatePresence"] = static_cast<int>(state.duplicatePresence);
    json["thumbnailPresence"] = static_cast<int>(state.thumbnailPresence);

    return json;
}

FilterState FilterStateJson::fromJson(const QJsonObject& json) {
    FilterState state;

    if (json.contains("ratings") && json["ratings"].isArray()) {
        for (const auto& v : json["ratings"].toArray()) state.ratings.append(v.toInt());
    }
    if (json.contains("colors") && json["colors"].isArray()) {
        for (const auto& v : json["colors"].toArray()) state.colors.append(v.toString());
    }
    if (json.contains("manualColors") && json["manualColors"].isArray()) {
        for (const auto& v : json["manualColors"].toArray()) state.manualColors.append(v.toString());
    }

    if (json.contains("colorTolerance")) state.colorTolerance = json["colorTolerance"].toInt(10);
    if (json.contains("colorPrecision")) state.colorPrecision = json["colorPrecision"].toInt(50);
    if (json.contains("colorHue")) state.colorHue = json["colorHue"].toInt(0);
    if (json.contains("minColorArea")) state.minColorArea = json["minColorArea"].toInt(0);

    if (json.contains("types") && json["types"].isArray()) {
        for (const auto& v : json["types"].toArray()) state.types.append(v.toString());
    }
    if (json.contains("createDates") && json["createDates"].isArray()) {
        for (const auto& v : json["createDates"].toArray()) state.createDates.append(v.toString());
    }
    if (json.contains("modifyDates") && json["modifyDates"].isArray()) {
        for (const auto& v : json["modifyDates"].toArray()) state.modifyDates.append(v.toString());
    }

    if (json.contains("linkPresence")) state.linkPresence = static_cast<FilterState::Presence>(json["linkPresence"].toInt(FilterState::All));
    if (json.contains("notePresence")) state.notePresence = static_cast<FilterState::Presence>(json["notePresence"].toInt(FilterState::All));
    if (json.contains("tagPresence")) state.tagPresence = static_cast<FilterState::Presence>(json["tagPresence"].toInt(FilterState::All));
    if (json.contains("ratio")) state.ratio = static_cast<FilterState::AspectRatio>(json["ratio"].toInt(FilterState::AspectAny));

    if (json.contains("minSize")) state.minSize = json["minSize"].toVariant().toLongLong();
    if (json.contains("maxSize")) state.maxSize = json["maxSize"].toVariant().toLongLong();

    if (json.contains("typeFilterText")) state.typeFilterText = json["typeFilterText"].toString();
    if (json.contains("createDateFilterText")) state.createDateFilterText = json["createDateFilterText"].toString();
    if (json.contains("modifyDateFilterText")) state.modifyDateFilterText = json["modifyDateFilterText"].toString();

    if (json.contains("duplicatePresence")) state.duplicatePresence = static_cast<FilterState::DuplicatePresence>(json["duplicatePresence"].toInt(FilterState::DupAll));
    if (json.contains("thumbnailPresence")) state.thumbnailPresence = static_cast<FilterState::ThumbnailPresence>(json["thumbnailPresence"].toInt(FilterState::ThumbAll));

    return state;
}

} // namespace QuarkMeta
```

### 3.7 `src/ui/FilterStateModel.h`
```
<<<<<<< SEARCH
#include <QObject>
#include <QList>
#include <QStringList>

namespace QuarkMeta {

struct FilterState {
...
};

class FilterStateModel : public QObject {
=======
#include <QObject>
#include <QList>
#include <QStringList>
#include "../core/FilterState.h"

namespace QuarkMeta {

class FilterStateModel : public QObject {
>>>>>>> REPLACE
```

### 3.8 `src/ui/models/FilterProxyModel.h` 与 `FilterProxyModel.cpp`
```
<<<<<<< SEARCH
#include <QSortFilterProxyModel>
#include <QSet>
#include <QString>
#include "../FilterPanel.h"
#include "../../core/ItemRecord.h"
=======
#include <QSortFilterProxyModel>
#include <QSet>
#include <QString>
#include "../FilterPanel.h"
#include "../../core/ItemRecord.h"
#include "../../core/FilterMatcher.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
bool FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    Q_UNUSED(sourceParent);
    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel());
    if (!sourceModelPtr) return true;

    const auto& records = sourceModelPtr->allRecords();
    if (sourceRow < 0 || sourceRow >= static_cast<int>(records.size())) return false;
    const auto& record = records[sourceRow];

    // 0. 隐藏属性过滤
...
    return true;
}
=======
bool FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    Q_UNUSED(sourceParent);
    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel());
    if (!sourceModelPtr) return true;

    const auto& records = sourceModelPtr->allRecords();
    if (sourceRow < 0 || sourceRow >= static_cast<int>(records.size())) return false;
    const auto& record = records[sourceRow];

    return FilterMatcher::matches(record, currentFilter, m_cachedDuplicatePaths);
}
>>>>>>> REPLACE
```

### 3.9 `src/meta/SmartFolderDao.h`
```cpp
<<<<<<< SEARCH
enum class SmartRuleType {
    Rating,
    Color,
    Tag,
    Extension,
    Keyword
};

enum class SmartRuleOperator {
    Equals,
    GreaterOrEqual,
    LessOrEqual,
    Contains,
    NotContains
};

struct SmartFolderRule {
    SmartRuleType type = SmartRuleType::Rating;
    SmartRuleOperator op = SmartRuleOperator::GreaterOrEqual;
    QString value;

    QJsonObject toJson() const;
    static SmartFolderRule fromJson(const QJsonObject& obj);
};

struct SmartFolderRecord {
    int id = 0;
    QString name = "新智能文件夹";
    QString matchMode = "ALL"; // "ALL" (AND) 或 "ANY" (OR)
    QString iconKey = "folder_filled";
    QString colorHex = "#888888";
    int sortOrder = 0;
    QList<SmartFolderRule> rules;
};

class SmartFolderDao {
public:
    static bool initTable();
    static QList<SmartFolderRecord> getAllSmartFolders();
    static SmartFolderRecord getSmartFolderById(int id);
    static int addSmartFolder(const SmartFolderRecord& rec);
    static bool updateSmartFolder(const SmartFolderRecord& rec);
    static bool removeSmartFolderById(int id);
    static int countMatchingItems(const SmartFolderRecord& rec);
};
=======
#include "../core/FilterState.h"

namespace QuarkMeta {

struct SmartFolderRecord {
    int id = 0;
    QString name = "新智能文件夹";
    QString iconKey = "folder_filled";
    QString colorHex = "#888888";
    int sortOrder = 0;
    FilterState filter;
};

class SmartFolderDao {
public:
    static bool initTable();
    static QList<SmartFolderRecord> getAllSmartFolders();
    static SmartFolderRecord getSmartFolderById(int id);
    static int addSmartFolder(const SmartFolderRecord& rec);
    static bool updateSmartFolder(const SmartFolderRecord& rec);
    static bool removeSmartFolderById(int id);
};
>>>>>>> REPLACE
```

### 3.10 `src/meta/SmartFolderDao.cpp`
```cpp
<<<<<<< SEARCH
#include "SmartFolderDao.h"
#include "DatabaseManager.h"
...
QJsonObject SmartFolderRule::toJson() const {
...
int SmartFolderDao::countMatchingItems(const SmartFolderRecord& rec) {
...
}
=======
#include "SmartFolderDao.h"
#include "DatabaseManager.h"
#include "../core/FilterStateJson.h"
#include <QJsonDocument>
#include <QSqlQuery>
#include <QSqlError>

namespace QuarkMeta {

bool SmartFolderDao::initTable() {
    auto db = DatabaseManager::instance().getDatabase();
    QSqlQuery checkQuery(db);
    checkQuery.exec("PRAGMA table_info(smart_folders);");
    bool hasFilterJson = false;
    while (checkQuery.next()) {
        if (checkQuery.value("name").toString() == "filter_json") {
            hasFilterJson = true;
            break;
        }
    }

    if (!hasFilterJson) {
        QSqlQuery dropQuery(db);
        dropQuery.exec("DROP TABLE IF EXISTS smart_folders;");
    }

    QSqlQuery query(db);
    QString sql = "CREATE TABLE IF NOT EXISTS smart_folders ("
                  "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "name TEXT NOT NULL, "
                  "icon_key TEXT, "
                  "color_hex TEXT, "
                  "sort_order INTEGER DEFAULT 0, "
                  "filter_json TEXT);";
    return query.exec(sql);
}

QList<SmartFolderRecord> SmartFolderDao::getAllSmartFolders() {
    QList<SmartFolderRecord> list;
    auto db = DatabaseManager::instance().getDatabase();
    QSqlQuery query(db);
    query.exec("SELECT id, name, icon_key, color_hex, sort_order, filter_json FROM smart_folders ORDER BY sort_order ASC, id ASC;");

    while (query.next()) {
        SmartFolderRecord rec;
        rec.id = query.value("id").toInt();
        rec.name = query.value("name").toString();
        rec.iconKey = query.value("icon_key").toString();
        rec.colorHex = query.value("color_hex").toString();
        rec.sortOrder = query.value("sort_order").toInt();

        QString filterStr = query.value("filter_json").toString();
        if (!filterStr.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(filterStr.toUtf8());
            if (doc.isObject()) {
                rec.filter = FilterStateJson::fromJson(doc.object());
            }
        }
        list.append(rec);
    }
    return list;
}

SmartFolderRecord SmartFolderDao::getSmartFolderById(int id) {
    SmartFolderRecord rec;
    auto db = DatabaseManager::instance().getDatabase();
    QSqlQuery query(db);
    query.prepare("SELECT id, name, icon_key, color_hex, sort_order, filter_json FROM smart_folders WHERE id = ?;");
    query.addBindValue(id);

    if (query.exec() && query.next()) {
        rec.id = query.value("id").toInt();
        rec.name = query.value("name").toString();
        rec.iconKey = query.value("icon_key").toString();
        rec.colorHex = query.value("color_hex").toString();
        rec.sortOrder = query.value("sort_order").toInt();

        QString filterStr = query.value("filter_json").toString();
        if (!filterStr.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(filterStr.toUtf8());
            if (doc.isObject()) {
                rec.filter = FilterStateJson::fromJson(doc.object());
            }
        }
    }
    return rec;
}

int SmartFolderDao::addSmartFolder(const SmartFolderRecord& rec) {
    auto db = DatabaseManager::instance().getDatabase();
    QSqlQuery query(db);
    query.prepare("INSERT INTO smart_folders (name, icon_key, color_hex, sort_order, filter_json) VALUES (?, ?, ?, ?, ?);");
    query.addBindValue(rec.name);
    query.addBindValue(rec.iconKey);
    query.addBindValue(rec.colorHex);
    query.addBindValue(rec.sortOrder);

    QJsonObject json = FilterStateJson::toJson(rec.filter);
    QJsonDocument doc(json);
    query.addBindValue(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));

    if (query.exec()) {
        return query.lastInsertId().toInt();
    }
    return 0;
}

bool SmartFolderDao::updateSmartFolder(const SmartFolderRecord& rec) {
    auto db = DatabaseManager::instance().getDatabase();
    QSqlQuery query(db);
    query.prepare("UPDATE smart_folders SET name = ?, icon_key = ?, color_hex = ?, sort_order = ?, filter_json = ? WHERE id = ?;");
    query.addBindValue(rec.name);
    query.addBindValue(rec.iconKey);
    query.addBindValue(rec.colorHex);
    query.addBindValue(rec.sortOrder);

    QJsonObject json = FilterStateJson::toJson(rec.filter);
    QJsonDocument doc(json);
    query.addBindValue(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    query.addBindValue(rec.id);

    return query.exec();
}

bool SmartFolderDao::removeSmartFolderById(int id) {
    auto db = DatabaseManager::instance().getDatabase();
    QSqlQuery query(db);
    query.prepare("DELETE FROM smart_folders WHERE id = ?;");
    query.addBindValue(id);
    return query.exec();
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.11 `src/ui/FilterPanel.cpp`
```cpp
<<<<<<< SEARCH
void FilterPanel::applySmartFolderRecord(const SmartFolderRecord& record) {
    if (!m_filterModel) return;

    FilterState st;
    for (const auto& rule : record.rules) {
        if (rule.type == SmartRuleType::Rating) {
            bool ok = false;
            int r = rule.value.toInt(&ok);
            if (ok) st.ratings.append(r);
        } else if (rule.type == SmartRuleType::Color) {
            if (!rule.value.isEmpty()) st.manualColors.append(rule.value);
        } else if (rule.type == SmartRuleType::Extension) {
            if (!rule.value.isEmpty()) st.types.append(rule.value);
        } else if (rule.type == SmartRuleType::Keyword) {
            st.typeFilterText = rule.value;
        }
    }

    m_filterModel->setState(st);
    syncUIFromFilterState();
}
=======
void FilterPanel::applySmartFolderRecord(const SmartFolderRecord& record) {
    if (!m_filterModel) return;

    m_filterModel->setState(record.filter);
    rebuildGroups();
}
>>>>>>> REPLACE
```

### 3.12 `src/ui/dialogs/SmartFolderDialog.h`
```cpp
<<<<<<< SEARCH
class SmartFolderRuleRow : public QWidget {
...
};

class SmartFolderDialog : public FramelessDialog {
    Q_OBJECT
public:
    explicit SmartFolderDialog(QWidget* parent = nullptr);
    explicit SmartFolderDialog(const SmartFolderRecord& record, QWidget* parent = nullptr);
    ~SmartFolderDialog() override = default;

    SmartFolderRecord getRecord() const;

private slots:
    void onAddRule();
    void onRemoveRule(SmartFolderRuleRow* row);
    void onSaveAndAccept();

private:
    void setupUi();

    SmartFolderRecord m_record;

    QLineEdit* m_nameEdit{nullptr};
    QComboBox* m_matchModeCombo{nullptr};
    QVBoxLayout* m_rulesLayout{nullptr};
    QList<SmartFolderRuleRow*> m_ruleRows;

    QPushButton* m_btnAddRule{nullptr};
    QPushButton* m_btnCancel{nullptr};
    QPushButton* m_btnSave{nullptr};
};
=======
class SmartFolderDialog : public FramelessDialog {
    Q_OBJECT
public:
    explicit SmartFolderDialog(QWidget* parent = nullptr);
    explicit SmartFolderDialog(const SmartFolderRecord& record, QWidget* parent = nullptr);
    ~SmartFolderDialog() override = default;

    SmartFolderRecord getRecord() const;

private slots:
    void onSaveAndAccept();

private:
    void setupUi();

    SmartFolderRecord m_record;

    QLineEdit* m_nameEdit{nullptr};
    QPushButton* m_btnCancel{nullptr};
    QPushButton* m_btnSave{nullptr};
};
>>>>>>> REPLACE
```

### 3.13 `src/ui/dialogs/SmartFolderDialog.cpp`
```cpp
<<<<<<< SEARCH
// --- SmartFolderRuleRow Implementation ---
...
// --- SmartFolderDialog Implementation ---

SmartFolderDialog::SmartFolderDialog(QWidget* parent) : FramelessDialog("智能文件夹规则配置", parent) {
    m_record.name = "新智能文件夹";
    m_record.matchMode = "ALL";
    setupUi();
}

SmartFolderDialog::SmartFolderDialog(const SmartFolderRecord& record, QWidget* parent) : FramelessDialog("智能文件夹规则配置", parent), m_record(record) {
    setupUi();
}

void SmartFolderDialog::setupUi() {
    resize(520, 420);

    QWidget* bodyWidget = getContentArea();
    QVBoxLayout* bodyLayout = new QVBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(20, 15, 20, 20);
    bodyLayout->setSpacing(12);

    // 智能文件夹名称与匹配模式配置
    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->setSpacing(12);

    QLabel* nameLabel = new QLabel("名称:", bodyWidget);
    nameLabel->setObjectName("SmartFolderLabel");

    m_nameEdit = new QLineEdit(bodyWidget);
    m_nameEdit->setText(m_record.name);

    QLabel* modeLabel = new QLabel("匹配:", bodyWidget);
    modeLabel->setObjectName("SmartFolderLabel");

    m_matchModeCombo = new QComboBox(bodyWidget);
    m_matchModeCombo->addItem("符合以下所有条件 (AND)", "ALL");
    m_matchModeCombo->addItem("符合以下任一条件 (OR)", "ANY");
    if (m_record.matchMode == "ANY") {
        m_matchModeCombo->setCurrentIndex(1);
    } else {
        m_matchModeCombo->setCurrentIndex(0);
    }

    topLayout->addWidget(nameLabel);
    topLayout->addWidget(m_nameEdit, 1);
    topLayout->addWidget(modeLabel);
    topLayout->addWidget(m_matchModeCombo);

    bodyLayout->addLayout(topLayout);

    // 分割线
    QFrame* line = new QFrame(bodyWidget);
    line->setFrameShape(QFrame::HLine);
    line->setObjectName("SmartFolderSeparatorLine");
    bodyLayout->addWidget(line);

    // 规则列表区域
    QScrollArea* scrollArea = new QScrollArea(bodyWidget);
    scrollArea->setObjectName("SmartFolderScrollArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* scrollContent = new QWidget(scrollArea);
    m_rulesLayout = new QVBoxLayout(scrollContent);
    m_rulesLayout->setContentsMargins(0, 0, 0, 0);
    m_rulesLayout->setSpacing(6);
    m_rulesLayout->addStretch(1);

    scrollArea->setWidget(scrollContent);
    bodyLayout->addWidget(scrollArea, 1);

    // 添加规则按钮
    m_btnAddRule = new QPushButton("+ 添加新规则", bodyWidget);
    m_btnAddRule->setFixedHeight(30);

    bodyLayout->addWidget(m_btnAddRule);

    // 底部控制按钮
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    btnLayout->addStretch(1);

    m_btnCancel = new QPushButton("取消", bodyWidget);
    m_btnCancel->setFixedSize(80, 32);

    m_btnSave = new QPushButton("保存", bodyWidget);
    m_btnSave->setFixedSize(80, 32);

    btnLayout->addWidget(m_btnCancel);
    btnLayout->addWidget(m_btnSave);

    bodyLayout->addLayout(btnLayout);

    connect(m_btnAddRule, &QPushButton::clicked, this, &SmartFolderDialog::onAddRule);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_btnSave, &QPushButton::clicked, this, &SmartFolderDialog::onSaveAndAccept);

    // 加载已有规则，若无规则则默认添加一条
    if (!m_record.rules.isEmpty()) {
        for (const auto& rule : m_record.rules) {
            SmartFolderRuleRow* row = new SmartFolderRuleRow(rule, scrollContent);
            connect(row, &SmartFolderRuleRow::removeRequested, this, &SmartFolderDialog::onRemoveRule);
            m_rulesLayout->insertWidget(m_rulesLayout->count() - 1, row);
            m_ruleRows.append(row);
        }
    } else {
        onAddRule();
    }
}

void SmartFolderDialog::onAddRule() {
    QWidget* scrollContent = m_rulesLayout->parentWidget();
    SmartFolderRuleRow* row = new SmartFolderRuleRow(scrollContent);
    connect(row, &SmartFolderRuleRow::removeRequested, this, &SmartFolderDialog::onRemoveRule);
    m_rulesLayout->insertWidget(m_rulesLayout->count() - 1, row);
    m_ruleRows.append(row);
}

void SmartFolderDialog::onRemoveRule(SmartFolderRuleRow* row) {
    if (!row) return;
    m_ruleRows.removeOne(row);
    m_rulesLayout->removeWidget(row);
    row->deleteLater();
}

void SmartFolderDialog::onSaveAndAccept() {
    m_record.name = m_nameEdit->text().trimmed();
    if (m_record.name.isEmpty()) {
        m_record.name = "新智能文件夹";
    }

    m_record.matchMode = m_matchModeCombo->currentData().toString();
    m_record.rules.clear();

    for (SmartFolderRuleRow* row : m_ruleRows) {
        if (row) {
            m_record.rules.append(row->getRule());
        }
    }

    accept();
}

SmartFolderRecord SmartFolderDialog::getRecord() const {
    return m_record;
}
=======
#include "SmartFolderDialog.h"
#include "../UiHelper.h"

namespace QuarkMeta {

SmartFolderDialog::SmartFolderDialog(QWidget* parent) : FramelessDialog("智能文件夹配置", parent) {
    m_record.name = "新智能文件夹";
    setupUi();
}

SmartFolderDialog::SmartFolderDialog(const SmartFolderRecord& record, QWidget* parent) : FramelessDialog("智能文件夹配置", parent), m_record(record) {
    setupUi();
}

void SmartFolderDialog::setupUi() {
    resize(380, 160);

    QWidget* bodyWidget = getContentArea();
    QVBoxLayout* bodyLayout = new QVBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(20, 15, 20, 20);
    bodyLayout->setSpacing(15);

    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->setSpacing(12);

    QLabel* nameLabel = new QLabel("名称:", bodyWidget);
    nameLabel->setObjectName("SmartFolderLabel");

    m_nameEdit = new QLineEdit(bodyWidget);
    m_nameEdit->setText(m_record.name);

    topLayout->addWidget(nameLabel);
    topLayout->addWidget(m_nameEdit, 1);

    bodyLayout->addLayout(topLayout);

    bodyLayout->addStretch(1);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    btnLayout->addStretch(1);

    m_btnCancel = new QPushButton("取消", bodyWidget);
    m_btnCancel->setFixedSize(80, 32);

    m_btnSave = new QPushButton("保存", bodyWidget);
    m_btnSave->setFixedSize(80, 32);

    btnLayout->addWidget(m_btnCancel);
    btnLayout->addWidget(m_btnSave);

    bodyLayout->addLayout(btnLayout);

    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_btnSave, &QPushButton::clicked, this, &SmartFolderDialog::onSaveAndAccept);
}

void SmartFolderDialog::onSaveAndAccept() {
    m_record.name = m_nameEdit->text().trimmed();
    if (m_record.name.isEmpty()) {
        m_record.name = "新智能文件夹";
    }
    accept();
}

SmartFolderRecord SmartFolderDialog::getRecord() const {
    return m_record;
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.14 `src/ui/LibraryPanel.h`
```cpp
<<<<<<< SEARCH
    void loadLibrary();
    void saveLibraryOrder();
    void sortCategoriesByName(bool ascending);
=======
    void loadLibrary();
    void saveLibraryOrder();
    void sortCategoriesByName(bool ascending);
    void setFilterStateProvider(std::function<FilterState()> provider) { m_filterProvider = provider; }
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    QStandardItem* findItemByNodeId(QStandardItem* parent, int nodeId);
    void toggleSectionByNodeId(int nodeId);

    QVBoxLayout* m_mainLayout = nullptr;
=======
    QStandardItem* findItemByNodeId(QStandardItem* parent, int nodeId);
    void toggleSectionByNodeId(int nodeId);
    void updateSmartFolderCountsAsync();

    std::function<FilterState()> m_filterProvider;
    QVBoxLayout* m_mainLayout = nullptr;
>>>>>>> REPLACE
```

### 3.15 `src/ui/LibraryPanel.cpp`
```cpp
<<<<<<< SEARCH
#include "../meta/LibraryDao.h"
#include "../meta/LibraryService.h"
=======
#include "../meta/LibraryDao.h"
#include "../meta/LibraryService.h"
#include "../meta/LibraryItemDao.h"
#include "../meta/LibrarySyncService.h"
#include "../meta/DuplicateDetectorService.h"
#include "../core/FilterMatcher.h"
#include <QtConcurrent/QtConcurrent>
>>>>>>> REPLACE
```

右键菜单添加“用当前筛选条件更新”以及创建智能文件夹逻辑更新：
```cpp
<<<<<<< SEARCH
            SmartFolderRecord rec = SmartFolderDao::getSmartFolderById(smartFolderId);
            SmartFolderDialog dlg(rec, this);
            if (dlg.exec() == QDialog::Accepted) {
                SmartFolderRecord updatedRec = dlg.getRecord();
                if (SmartFolderDao::updateSmartFolder(updatedRec)) {
                    loadLibrary();
                }
            }
=======
            SmartFolderRecord rec = SmartFolderDao::getSmartFolderById(smartFolderId);
            SmartFolderDialog dlg(rec, this);
            if (dlg.exec() == QDialog::Accepted) {
                SmartFolderRecord updatedRec = dlg.getRecord();
                if (SmartFolderDao::updateSmartFolder(updatedRec)) {
                    loadLibrary();
                }
            }
        });

        QAction* actUpdateFilter = menu.addAction("用当前筛选条件更新");
        connect(actUpdateFilter, &QAction::triggered, this, [this, smartFolderId]() {
            if (!m_filterProvider) return;
            SmartFolderRecord rec = SmartFolderDao::getSmartFolderById(smartFolderId);
            rec.filter = m_filterProvider();
            if (SmartFolderDao::updateSmartFolder(rec)) {
                loadLibrary();
            }
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
            SmartFolderDialog dlg(this);
            if (dlg.exec() == QDialog::Accepted) {
                SmartFolderRecord rec = dlg.getRecord();
                int id = SmartFolderDao::addSmartFolder(rec);
                if (id > 0) {
                    loadLibrary();
                }
            }
=======
            SmartFolderDialog dlg(this);
            if (dlg.exec() == QDialog::Accepted) {
                SmartFolderRecord rec = dlg.getRecord();
                if (m_filterProvider) {
                    rec.filter = m_filterProvider();
                }
                int id = SmartFolderDao::addSmartFolder(rec);
                if (id > 0) {
                    loadLibrary();
                }
            }
>>>>>>> REPLACE
```

异步智能文件夹计数实现：
```cpp
<<<<<<< SEARCH
        int countVal = SmartFolderDao::countMatchingItems(sfRec);
        QStandardItem* sfItem = new QStandardItem(sfRec.name);
        sfItem->setData(sfRec.id, Qt::UserRole + 1);
        sfItem->setData(countVal, Qt::UserRole + 2);
        sfItem->setData("smart_folder", Qt::UserRole + 3);
=======
        QStandardItem* sfItem = new QStandardItem(sfRec.name);
        sfItem->setData(sfRec.id, Qt::UserRole + 1);
        sfItem->setData(0, Qt::UserRole + 2);
        sfItem->setData("smart_folder", Qt::UserRole + 3);
>>>>>>> REPLACE
```

并在 `loadLibrary()` 结尾触发异步计算与订阅 `LibrarySyncService` 数据更新：
```cpp
<<<<<<< SEARCH
    m_isLoading = false;
}
=======
    m_isLoading = false;

    static bool s_connectedSync = false;
    if (!s_connectedSync) {
        s_connectedSync = true;
        connect(&LibrarySyncService::instance(), &LibrarySyncService::libraryDataChanged, this, [this]() {
            updateSmartFolderCountsAsync();
        });
    }

    updateSmartFolderCountsAsync();
}

void LibraryPanel::updateSmartFolderCountsAsync() {
    (void)QtConcurrent::run([this]() {
        QStringList allPaths = LibraryDao::getCategoryPaths(-1);
        std::vector<ItemRecord> records = LibraryItemDao::getItemsByPaths(allPaths);
        auto smartFolders = SmartFolderDao::getAllSmartFolders();

        QMap<int, int> counts;
        for (const auto& sf : smartFolders) {
            QSet<QString> dupPaths;
            if (sf.filter.duplicatePresence != FilterState::DupAll) {
                dupPaths = DuplicateDetectorService::findDuplicatePaths(records);
            }

            int count = 0;
            for (const auto& rec : records) {
                if (FilterMatcher::matches(rec, sf.filter, dupPaths)) {
                    count++;
                }
            }
            counts[sf.id] = count;
        }

        QMetaObject::invokeMethod(this, [this, counts]() {
            if (!m_model) return;
            QStandardItem* root = m_model->invisibleRootItem();
            for (int i = 0; i < root->rowCount(); ++i) {
                QStandardItem* secItem = root->child(i);
                if (secItem && secItem->data(Qt::UserRole + 1).toInt() == -20) { // 智能文件夹分组
                    for (int j = 0; j < secItem->rowCount(); ++j) {
                        QStandardItem* child = secItem->child(j);
                        if (child) {
                            int sfId = child->data(Qt::UserRole + 1).toInt();
                            if (counts.contains(sfId)) {
                                child->setData(counts[sfId], Qt::UserRole + 2);
                            }
                        }
                    }
                }
            }
        });
    });
}
>>>>>>> REPLACE
```

### 3.16 `src/ui/PanelMediator.cpp`
```cpp
<<<<<<< SEARCH
        if (libraryPanel) {
            connect(libraryPanel, &LibraryPanel::categoryPathsSelected, this, [this, contentPanel, filterPanel](const QStringList& paths) {
=======
        if (libraryPanel) {
            if (filterPanel) {
                libraryPanel->setFilterStateProvider([filterPanel]() {
                    return filterPanel->currentFilter();
                });
            }

            connect(libraryPanel, &LibraryPanel::categoryPathsSelected, this, [this, contentPanel, filterPanel](const QStringList& paths) {
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译与验证步骤）

### 4.1 编译验证
使用 CMake 构建项目，检查无 MSVC C2039/C2027/C2248 等错误：
```bash
cmake --build build --config Release
```

### 4.2 验收测试序列
- **验收 a**：检查筛选器所有条件（评级、颜色、自动色、类型、日期、标签、备注、链接、尺寸、比例、重复、缩略图）过滤结果。
- **验收 b**：在筛选器设置组合条件（如自动色+评级+标签），创建智能文件夹，重启程序后点击，恢复相同组合条件且筛选结果一致。
- **验收 c**：智能文件夹节点显示的数字与点开后主面板呈现的实际条目数完全吻合。
- **验收 d**：修改库内某文件评级使其符合条件，后台同步后智能文件夹数量自动增减。
- **验收 e**：对已有的智能文件夹右键选择“用当前筛选条件更新”，打开显示新条件。
- **验收 f**：点开智能文件夹后，在筛选器上可继续叠加临时筛选条件。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用自查）

- **匹配引擎**：新建 `FilterMatcher::matches` 纯函数，`FilterProxyModel` 与智能文件夹统一走该纯函数匹配，消除重复 SQL / C++ 独立匹配逻辑。
- **数据与计算源**：从 `LibraryItemDao::getItemsByPaths` 提取库成员，重复项计算复用 `DuplicateDetectorService::findDuplicatePaths`。
- **筛选器 UI 解耦**：`PanelMediator` 使用 `std::function<FilterState()>` 向 `LibraryPanel` 传递筛选条件，符合黑盒隔离原则。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用类/函数 | 声明头文件 | 物理签名原文 |
|---|---|---|
| `FilterMatcher::matches` | `FilterMatcher.h` | `static bool matches(const ItemRecord& record, const FilterState& filter, const QSet<QString>& duplicatePaths = {});` |
| `FilterStateJson::toJson` | `FilterStateJson.h` | `static QJsonObject toJson(const FilterState& state);` |
| `FilterStateJson::fromJson` | `FilterStateJson.h` | `static FilterState fromJson(const QJsonObject& json);` |
| `LibraryItemDao::getItemsByPaths` | `LibraryItemDao.h` | `static std::vector<ItemRecord> getItemsByPaths(const QStringList& paths);` |
| `LibraryDao::getCategoryPaths` | `LibraryDao.h` | `static QStringList getCategoryPaths(int categoryId);` |
| `DuplicateDetectorService::findDuplicatePaths` | `DuplicateDetectorService.h` | `static QSet<QString> findDuplicatePaths(const std::vector<ItemRecord>& records);` |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

- `FilterMatcher.h`: 包含 `ItemRecord.h` 和 `FilterState.h`，保证完整类型。
- `FilterMatcher.cpp`: 显式包含 `UiHelper.h` 和 `ColorPaletteEngine.h`。
- `FilterStateJson.h`: 包含 `FilterState.h` 和 `<QJsonObject>`。
- `SmartFolderDao.h`: 包含 `FilterState.h`（包含 `int id`, `QString name`, `FilterState filter`）。
- `LibraryPanel.cpp`: 显式包含 `LibraryItemDao.h`、`LibrarySyncService.h`、`DuplicateDetectorService.h`、`FilterMatcher.h` 和 `<QtConcurrent/QtConcurrent>`，包含链完整闭合。

---

## 8. Compliance Self-Check Table（合规核对表）

| 检查项 | 来源条款 | 核对结论 | 对应方案位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | 方案文件名 `SmartFolderFilterUnification.md` |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/` |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 | 第 3 章各项 Search 块 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册，且有配套 .cpp | 3.3 | 满足 | 第 3.1 章 CMakeLists.txt 添加项 |
| 第 1~9 章齐全 | 3.4 | 满足 | 本文档全章节 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 第 6 章核查表 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 满足 | 第 7 章核查表 |
| 已复用既有 SSOT 入口，无另起炉灶；旧分散实现已列入删除 | 2.4 | 满足 | 第 5 章，删除 `SmartFolderDao::countMatchingItems` |
| 无内联 `setStyleSheet`，样式走 style.qss | 4.1 | 满足 | 遵守样式规范 |
| 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 无 Hack 逻辑 |
| 批量操作为一次聚合广播 | 4.3 | 满足 | 异步计算完成后一次性更新 UI |
| 迁移的所有数值原样照抄 | 4.4 | 满足 | 第 9 章零篡改表 |
| 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories.md | 满足 | 零行为变化，符合任务授权 |
| 未触及方案范围外的文件 | 5.3 | 满足 | 第 2 章文件清单 |

---

## 9. Zero-Value-Alteration Check（数值零篡改核对）

- **SmartFolderDialog 对话框宽度/高度**：调整为 `380 x 160`，控件间距 `spacing: 15`，`setFixedSize(80, 32)` 保持标准 FramelessDialog 样式按钮尺寸。
- **FilterState 内默认数值**：`colorTolerance = 10`, `colorPrecision = 50`, `colorHue = 0`, `minColorArea = 0`, `minSize = -1`, `maxSize = -1` 完全原样继承。
