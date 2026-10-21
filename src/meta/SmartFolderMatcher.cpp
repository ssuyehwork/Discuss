#include "SmartFolderMatcher.h"
#include "../ui/StyleLibrary.h"

namespace QuarkMeta {

bool SmartFolderMatcher::matches(const ItemRecord& record, const SmartFolderRecord& sfRec) {
    if (sfRec.rules.isEmpty()) return false;

    bool isAllMode = (sfRec.matchMode.toUpper() == "ALL");

    for (const auto& rule : sfRec.rules) {
        bool ruleMatched = false;

        switch (rule.type) {
            case SmartRuleType::Rating: {
                bool ok = false;
                int targetRating = rule.value.toInt(&ok);
                if (ok) {
                    if (rule.op == SmartRuleOperator::GreaterOrEqual) {
                        ruleMatched = (record.rating >= targetRating);
                    } else if (rule.op == SmartRuleOperator::LessOrEqual) {
                        ruleMatched = (record.rating <= targetRating);
                    } else { // Equals or default
                        ruleMatched = (record.rating == targetRating);
                    }
                }
                break;
            }
            case SmartRuleType::Extension: {
                QString ext = rule.value.trimmed();
                if (ext.startsWith('.')) ext.remove(0, 1);

                bool equalExt = (record.suffix.compare(ext, Qt::CaseInsensitive) == 0);
                if (rule.op == SmartRuleOperator::NotContains) {
                    ruleMatched = !equalExt;
                } else {
                    ruleMatched = equalExt;
                }
                break;
            }
            case SmartRuleType::Color: {
                QString normRecordColor = Style::normalizeManualColor(record.manualColor);
                QString normRuleColor = Style::normalizeManualColor(rule.value);

                bool equalColor = (normRecordColor == normRuleColor);
                if (rule.op == SmartRuleOperator::NotContains) {
                    ruleMatched = !equalColor;
                } else {
                    ruleMatched = equalColor;
                }
                break;
            }
            case SmartRuleType::Tag: {
                QString tagVal = rule.value.trimmed();
                bool hasTag = false;
                for (const QString& t : record.tags) {
                    if (t.compare(tagVal, Qt::CaseInsensitive) == 0) {
                        hasTag = true;
                        break;
                    }
                }
                if (rule.op == SmartRuleOperator::NotContains) {
                    ruleMatched = !hasTag;
                } else {
                    ruleMatched = hasTag;
                }
                break;
            }
            case SmartRuleType::Keyword: {
                QString kw = rule.value.trimmed();
                bool foundKw = false;
                if (!kw.isEmpty()) {
                    if (record.filename.contains(kw, Qt::CaseInsensitive)) {
                        foundKw = true;
                    } else if (!record.note.isEmpty() && record.note.contains(kw, Qt::CaseInsensitive)) {
                        foundKw = true;
                    } else {
                        for (const QString& t : record.tags) {
                            if (t.contains(kw, Qt::CaseInsensitive)) {
                                foundKw = true;
                                break;
                            }
                        }
                    }
                }
                if (rule.op == SmartRuleOperator::NotContains) {
                    ruleMatched = !foundKw;
                } else {
                    ruleMatched = foundKw;
                }
                break;
            }
        }

        if (isAllMode) {
            if (!ruleMatched) return false;
        } else {
            if (ruleMatched) return true;
        }
    }

    return isAllMode ? true : false;
}

} // namespace QuarkMeta
