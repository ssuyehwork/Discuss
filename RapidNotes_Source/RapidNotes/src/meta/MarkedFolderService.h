#pragma once
#include <QObject>
#include <QString>
#include <QMenu>
#include <QAction>

namespace QuarkMeta {

class MarkedFolderService : public QObject {
    Q_OBJECT
public:
    static MarkedFolderService& instance() {
        static MarkedFolderService s_instance;
        return s_instance;
    }
};

} // namespace QuarkMeta
