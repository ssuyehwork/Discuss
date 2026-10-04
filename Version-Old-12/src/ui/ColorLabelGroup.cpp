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
        int cnt = colorCounts.value(item.hex, colorCounts.value(item.name, 0));
        bool isChecked = (currentState.colors.contains(item.name) || currentState.colors.contains(item.hex));

        if (cnt == 0 && !isChecked) {
            continue;
        }

        QCheckBox* cb = addFilterRow(contentLayout, item.name, cnt, item.color);
        cb->setChecked(isChecked);
        QObject::connect(cb, &QCheckBox::checkStateChanged, contextObj, [filterModel, name = item.name, hex = item.hex](Qt::CheckState state) {
            FilterState st = filterModel->state();
            if (state == Qt::Checked) {
                if (!st.colors.contains(name)) st.colors.append(name);
            } else {
                st.colors.removeAll(name);
                st.colors.removeAll(hex);
            }
            filterModel->setState(st);
        });
    }
}

} // namespace QuarkMeta
