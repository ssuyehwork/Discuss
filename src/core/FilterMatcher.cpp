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
