#include "ColorLabelGroup.h"
#include "StyleLibrary.h"

namespace QuarkMeta {

void ColorLabelGroup::populate(QWidget* parentWidget,
                               QVBoxLayout* contentLayout,
                               FilterStateModel* filterModel,
                               const QMap<QString, int>& colorCounts,
                               const FilterState& currentState,
                               AddFilterRowFunc addFilterRow) {
    if (!contentLayout || !filterModel || !addFilterRow) return;

    QWidget* contextObj = parentWidget ? parentWidget : contentLayout->parentWidget();
    const auto& colorsList = Style::getColorPalette();

    for (const auto& item : colorsList) {
        QString itemNorm = Style::normalizeManualColor(item.hex.isEmpty() ? item.name : item.hex);
        QString rowKey = QString("color:%1").arg(itemNorm);

        int cnt = 0;
        for (auto it = colorCounts.begin(); it != colorCounts.end(); ++it) {
            if (Style::normalizeManualColor(it.key()) == itemNorm) {
                cnt += it.value();
            }
        }

        bool isChecked = false;
        for (const QString& mc : currentState.manualColors) {
            if (Style::normalizeManualColor(mc) == itemNorm) {
                isChecked = true;
                break;
            }
        }

        if (cnt == 0 && !isChecked) {
            continue;
        }

        QCheckBox* cb = addFilterRow(contentLayout, item.name, cnt, item.color, rowKey);
        cb->setChecked(isChecked);
        QObject::connect(cb, &QCheckBox::checkStateChanged, contextObj, [filterModel, name = item.name](Qt::CheckState state) {
            FilterState st = filterModel->state();
            QString normTarget = Style::normalizeManualColor(name);
            if (state == Qt::Checked) {
                bool found = false;
                for (const QString& mc : st.manualColors) {
                    if (Style::normalizeManualColor(mc) == normTarget) {
                        found = true;
                        break;
                    }
                }
                if (!found) st.manualColors.append(name);
            } else {
                QMutableStringListIterator it(st.manualColors);
                while (it.hasNext()) {
                    if (Style::normalizeManualColor(it.next()) == normTarget) {
                        it.remove();
                    }
                }
            }
            filterModel->setState(st);
        });
    }
}

} // namespace QuarkMeta
