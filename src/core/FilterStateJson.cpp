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
