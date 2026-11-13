#include "AppLockService.h"
#include <QSettings>

AppLockService& AppLockService::instance() {
    static AppLockService s_instance;
    return s_instance;
}

bool AppLockService::hasPassword() const {
    QSettings settings("RapidNotes", "QuickWindow");
    return !settings.value("appPassword").toString().isEmpty();
}

QString AppLockService::hint() const {
    QSettings settings("RapidNotes", "QuickWindow");
    return settings.value("appPasswordHint", "请输入启动密码").toString();
}

bool AppLockService::verify(const QString& input) const {
    QSettings settings("RapidNotes", "QuickWindow");
    const QString saved = settings.value("appPassword").toString();
    return !saved.isEmpty() && input == saved;
}
