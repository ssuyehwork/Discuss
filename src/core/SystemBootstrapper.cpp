#include "SystemBootstrapper.h"
#include "../meta/MetadataManager.h"
#include <QDir>
#include <QDebug>

namespace QuarkMeta {

SystemBootstrapper& SystemBootstrapper::instance() {
    static SystemBootstrapper inst;
    return inst;
}

SystemBootstrapper::SystemBootstrapper(QObject* parent) : QObject(parent) {}

void SystemBootstrapper::bootstrapMonitors() {
    const auto drives = QDir::drives();
    for (const QFileInfo& d : drives) {
    }
}

} // namespace QuarkMeta
