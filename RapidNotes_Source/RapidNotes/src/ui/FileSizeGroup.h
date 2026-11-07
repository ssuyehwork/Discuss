#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include "FilterStateModel.h"

namespace RapidNotes {

class FileSizeGroup {
public:
    static void populate(QWidget* parentWidget,
                         QVBoxLayout* contentLayout,
                         FilterStateModel* filterModel);
};

} // namespace RapidNotes
