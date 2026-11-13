#ifndef APPLOCKSERVICE_H
#define APPLOCKSERVICE_H

#include <QString>

/**
 * 应用锁服务：应用启动密码的唯一读取与校验入口。
 * 职责仅限：是否设置了密码 / 密码提示 / 校验密码。
 * 不持有界面，不保存锁定状态（锁定状态属于 QuickWindow）。
 */
class AppLockService {
public:
    static AppLockService& instance();

    bool hasPassword() const;
    QString hint() const;
    bool verify(const QString& input) const;

private:
    AppLockService() = default;
};

#endif // APPLOCKSERVICE_H
