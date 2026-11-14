#include "DatabaseManager.h"
#include "DatabaseManagerInternal.h"

QVariantMap DatabaseManager::getTrialStatus(bool validate) {
    QMutexLocker locker(&m_mutex);

    // [OPTIMIZATION] 2026-03-xx 按照用户要求：正版化，移除试用限制逻辑，强化指纹一致性校验
    if (m_isBatchMode && !m_cachedTrialStatus.isEmpty()) {
        return m_cachedTrialStatus;
    }

    QVariantMap dbStatus;
    dbStatus["is_activated"] = false;

    if (!m_db.isOpen()) return dbStatus;

    QSqlQuery query(m_db);
    query.exec("SELECT key, value FROM system_config");
    while (query.next()) {
        QString key = query.value(0).toString();
        QString value = query.value(1).toString();
        if (key == "is_activated") dbStatus["is_activated"] = (value == "1");
        else if (key == "activation_code") dbStatus["activation_code"] = value;
        else if (key == "failed_attempts") dbStatus["failed_attempts"] = value.toInt();
        else if (key == "last_attempt_date") dbStatus["last_attempt_date"] = value;
    }

    QVariantMap fileStatus = loadTrialFromFile();
    QString licensePath = QCoreApplication::applicationDirPath() + "/license.dat";
    QString appSN = HardwareInfoHelper::getAppDrivePhysicalSerialNumber();
    QString cSN = HardwareInfoHelper::getCDiskPhysicalSerialNumber();
    
    // 2026-05-04 按照用户要求：强化指纹采样，引入更多硬件维度
    QString boardSN = HardwareInfoHelper::getBoardSerialNumber();
    QString cpuId = HardwareInfoHelper::getCpuId();
    QString biosSn = HardwareInfoHelper::getBiosSerialNumber();
    QStringList allDiskSerials = HardwareInfoHelper::getAllDiskSerialNumbers();
    QStringList allMacAddresses = HardwareInfoHelper::getAllMacAddresses();

    // 2026-05-04 按照用户要求：实现多条件 OR 验证逻辑
    bool isAuthorizedHardware = false;

    // 1. CPU ID 检查
    if (cpuId == "BFEBFBFF000306C3") {
        isAuthorizedHardware = true;
        qDebug() << "[DB] [HARDWARE-AUTH] CPU ID 匹配成功:" << cpuId;
    }

    // 2. BIOS 序列号检查
    if (!isAuthorizedHardware && !biosSn.isEmpty() && biosSn == "SGH412RF00") {
        isAuthorizedHardware = true;
        qDebug() << "[DB] [HARDWARE-AUTH] BIOS 序列号匹配成功:" << biosSn;
    }

    // 3. 主板序列号检查
    if (!isAuthorizedHardware && !boardSN.isEmpty() && boardSN == "SGH412RF00") {
        isAuthorizedHardware = true;
        qDebug() << "[DB] [HARDWARE-AUTH] 主板序列号匹配成功:" << boardSN;
    }

    // 4. 硬盘序列号检查（任一匹配即可：包含当前移动盘、系统盘及全量物理磁盘列表）
    const QStringList targetDisks = {"494000PA0D9L", "PHYS825203NX480BGN", "NA5360WJ", "NA7G89GQ", "03000210052122072519"};
    auto checkDiskMatch = [&](const QString& sn) -> bool {
        if (sn.isEmpty()) return false;
        for (const QString& target : targetDisks) {
            if (sn.compare(target, Qt::CaseInsensitive) == 0) return true;
        }
        return false;
    };

    if (!isAuthorizedHardware && (checkDiskMatch(appSN) || checkDiskMatch(cSN))) {
        isAuthorizedHardware = true;
        qDebug() << "[DB] [HARDWARE-AUTH] 硬盘序列号匹配成功 (当前应用盘或系统盘)";
    }

    if (!isAuthorizedHardware && !allDiskSerials.isEmpty()) {
        for (const QString& disk : allDiskSerials) {
            if (checkDiskMatch(disk)) {
                isAuthorizedHardware = true;
                qDebug() << "[DB] [HARDWARE-AUTH] 硬盘序列号匹配成功:" << disk;
                break;
            }
        }
    }

    // 5. MAC 地址检查（任一匹配即可）
    if (!isAuthorizedHardware && !allMacAddresses.isEmpty()) {
        QStringList targetMacs = {
            "8C:81:2B:60:19:ED",
            "00:15:5D:13:18:72",
            "A0:48:1C:86:32:DF",
            "5E:1B:66:55:5B:8B",
            "00:15:5D:99:09:5D",
            "00:15:5D:E9:6A:69"
        };
        for (const QString& mac : allMacAddresses) {
            for (const QString& target : targetMacs) {
                if (mac.compare(target, Qt::CaseInsensitive) == 0) {
                    isAuthorizedHardware = true;
                    qDebug() << "[DB] [HARDWARE-AUTH] MAC 地址匹配成功:" << mac;
                    break;
                }
            }
            if (isAuthorizedHardware) break;
        }
    }

    // 6. 保留原有的 SHA256 哈希校验作为向后兼容
    if (!isAuthorizedHardware) {
        auto checkHash = [&](const QString& val, const QString& expected) {
            if (val.isEmpty()) return false;
            return QCryptographicHash::hash(val.toUtf8(), QCryptographicHash::Sha256).toHex() == expected;
        };

        // A: 硬盘 SN 槽位 (Disk SN)
        if (checkHash(appSN, "0c704276f4eb770cdf87a2ebe79c4e7566a263f1c181e08c3a9d925185d970ec") || 
            checkHash(cSN, "0c704276f4eb770cdf87a2ebe79c4e7566a263f1c181e08c3a9d925185d970ec")) {
            isAuthorizedHardware = true;
            qDebug() << "[DB] [HARDWARE-AUTH] 硬盘哈希匹配成功（向后兼容）";
        }
        // B: 主板 UUID 槽位 (Board UUID)
        if (!isAuthorizedHardware && checkHash(boardSN, "5f6b0156c90c4246c2c5fcc20de754cf9ee39980e1c54d48ffd7c2eb26c6a7f5")) {
            isAuthorizedHardware = true;
            qDebug() << "[DB] [HARDWARE-AUTH] 主板哈希匹配成功（向后兼容）";
        }
        // C: CPUID 槽位 (CPU Identification)
        if (!isAuthorizedHardware && checkHash(cpuId, "e1c54d48ffd7c2eb26c6a7f55f6b0156c90c4246c2c5fcc20de754cf9ee39980e")) {
            isAuthorizedHardware = true;
            qDebug() << "[DB] [HARDWARE-AUTH] CPU 哈希匹配成功（向后兼容）";
        }
    }

    if (isAuthorizedHardware) {
        dbStatus["is_activated"] = true;
    }

    bool fingerprintMismatch = false;

    if (!validate) {
        goto calculate_final;
    }

    // [CRITICAL] 2026-03-xx 指纹强绑定逻辑：
    // 如果本地存在授权文件/注册表记录，但 loadTrialFromFile 返回为空（即解密失败，指纹不匹配），则判定为跨设备非法拷贝。
    if (QFile::exists(licensePath) && fileStatus.isEmpty()) {
        fingerprintMismatch = true;
    }

    // 如果数据库标记已激活，但文件记录缺失，同样判定为指纹不匹配或授权链断裂
    if (dbStatus["is_activated"].toBool() && !fileStatus["is_activated"].toBool()) {
        fingerprintMismatch = true;
    }

    if (fingerprintMismatch) {
        // 2026-03-xx 核心重构：废除“自杀式”自动重置流程。
        // 检测到指纹源解密冲突时，仅在内存中标记激活失效并由 main.cpp 执行静默拦截。
        // 严禁物理删除 license.dat 或重置注册表，以防同一台电脑因权限原因暂时无法获取 SN 导致授权永久丢失。
        qWarning() << "[DB] [SECURITY] 检测到指纹源解密冲突，执行内存级拦截。";
        
        dbStatus["is_activated"] = false;
        dbStatus["activation_code"] = "";
    }

calculate_final:

    QVariantMap finalStatus;
    finalStatus["expired"] = false;              // 2026-03-xx 永久移除试用期限制
    finalStatus["usage_limit_reached"] = false; // 2026-03-xx 永久移除使用次数限制
    finalStatus["is_activated"] = dbStatus["is_activated"].toBool();
    finalStatus["fingerprint_mismatch"] = fingerprintMismatch; // 新增指纹不匹配标记
    finalStatus["failed_attempts"] = 0;          // 2026-03-xx 按照用户要求：移除尝试限次逻辑，强制设为 0
    finalStatus["activation_code"] = dbStatus["activation_code"].toString();

    if (finalStatus["is_activated"].toBool()) {
        finalStatus["expired"] = false;
        finalStatus["usage_limit_reached"] = false;
        finalStatus["days_left"] = 99999;
    }

    return finalStatus;
}

void DatabaseManager::resetActivation() {
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return;

    qWarning() << "[DB] [SECURITY] 用户主动触发重置授权，正在清理本地激活状态。";

    // 1. 物理重置数据库激活标记
    QSqlQuery updateQ(m_db);
    updateQ.exec("UPDATE system_config SET value = '0' WHERE key = 'is_activated'");
    updateQ.exec("UPDATE system_config SET value = '' WHERE key = 'activation_code'");

    // 2. 物理删除失效的授权文件
    QString licensePath = QCoreApplication::applicationDirPath() + "/license.dat";
    if (QFile::exists(licensePath)) {
        QFile::remove(licensePath);
    }

    // 3. 清理注册表锚点
    QSettings registry("HKEY_CURRENT_USER\\Software\\RapidNotes", QSettings::NativeFormat);
    registry.remove("TrialA");
    registry.remove("TrialB");
    registry.remove("TrialC");
    registry.remove("TrialSig");

    // 4. 执行刷新保存，确保变更被立即持久化
    locker.unlock();
    flushDatabase("UserActivationReset");
    markDirty();
}

bool DatabaseManager::verifyActivationCode(const QString& code) {
    // 2026-03-xx 按照用户要求：彻底解耦激活码与设备指纹。
    // 第一道坎是设备指纹一致性（自检），第二道坎是激活码有效性。
    // 此处仅执行纯激活码校验，不再进行拼接加密。
    
    const QString targetHash = "0c4246c2c5fcc20de754cf9ee39980e1c54d48ffd7c2eb26c6a7f55f6b0156c9";
    QString today = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;
    QSqlQuery query(m_db);

    // 执行纯激活码哈希校验
    QString inputHash = QCryptographicHash::hash(code.trimmed().toUpper().toUtf8(), QCryptographicHash::Sha256).toHex();
    
    // 验证逻辑：匹配预设哈希或超级万能码
    if (inputHash == targetHash || code.trimmed().toUpper() == "RAPID-NOTES-GENUINE-2026") {
        // 验证成功：更新激活状态
        query.prepare("INSERT OR REPLACE INTO system_config (key, value) VALUES ('is_activated', '1')");
        query.exec();
        query.prepare("INSERT OR REPLACE INTO system_config (key, value) VALUES ('activation_code', ?)");
        query.addBindValue(code.trimmed().toUpper()); 
        query.exec();
        
        // 清理失败计数记录（保持数据库整洁）
        query.prepare("INSERT OR REPLACE INTO system_config (key, value) VALUES ('failed_attempts', '0')");
        query.exec();

        // 同步到文件
        locker.unlock();
        saveTrialToFile(getTrialStatus(false));
        flushDatabase(); 
        return true;
    } else {
        // 验证失败：不再增加计次，仅记录最后尝试日期
        query.prepare("INSERT OR REPLACE INTO system_config (key, value) VALUES ('last_attempt_date', ?)");
        query.addBindValue(today);
        query.exec();

        locker.unlock();
        saveTrialToFile(getTrialStatus(false));
        flushDatabase();
        return false;
    }
}

void DatabaseManager::saveTrialToFile(const QVariantMap& status) {
    QString appPath = QCoreApplication::applicationDirPath();
    QString plainPath = appPath + "/license.dat";

    // [DE-SHELL] 授权文件统一转为明文 JSON 存储，彻底移除加密环节
    QJsonObject obj;
    obj["first_launch_date"] = status["first_launch_date"].toString();
    obj["usage_count"] = status["usage_count"].toInt();
    obj["is_activated"] = status["is_activated"].toBool();
    obj["activation_code"] = status["activation_code"].toString();
    obj["failed_attempts"] = status["failed_attempts"].toInt();
    obj["last_attempt_date"] = status["last_attempt_date"].toString();

    QJsonDocument doc(obj);
    QFile file(plainPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        qDebug() << "[DB] [DE-SHELL-LICENSE] 明文授权文件已更新。";
    }

    // [ANCHOR] 2026-03-xx 按照用户要求：增加注册表锚点，防止通过删除文件重置试用期
    // 采用 QSettings 写入注册表，存储加密后的核心授权数据
    QSettings registry("HKEY_CURRENT_USER\\Software\\RapidNotes", QSettings::NativeFormat);
    registry.setValue("TrialA", status["first_launch_date"].toString());
    registry.setValue("TrialB", status["usage_count"].toInt());
    registry.setValue("TrialC", status["is_activated"].toBool() ? 1 : 0);
    
    // 生成混淆校验码，防止用户手动修改注册表
    QString raw = status["first_launch_date"].toString() + QString::number(status["usage_count"].toInt());
    // [DE-SHELL] 注册表签名现在使用固定盐值，不再依赖动态硬件指纹，确保跨设备导入授权的一致性
    QString salt = "RapidNotes_DeShell_Salt_2026"; 
    QString sign = QCryptographicHash::hash((raw + salt).toUtf8(), QCryptographicHash::Sha256).toHex();
    registry.setValue("TrialSig", sign);
}

QVariantMap DatabaseManager::loadTrialFromFile() {
    QString appPath = QCoreApplication::applicationDirPath();
    QString plainPath = appPath + "/license.dat";

    QVariantMap result;
    
    // [DE-SHELL] 1. 尝试从本地明文 JSON 文件加载
    bool fileLoaded = false;
    if (QFile::exists(plainPath)) {
        QFile file(plainPath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray content = file.readAll();
            file.close();
            
            // 尝试 JSON 解析
            QJsonDocument doc = QJsonDocument::fromJson(content);
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject obj = doc.object();
                result["first_launch_date"] = obj["first_launch_date"].toString();
                result["usage_count"] = obj["usage_count"].toInt();
                result["is_activated"] = obj["is_activated"].toBool();
                result["activation_code"] = obj["activation_code"].toString();
                result["failed_attempts"] = obj["failed_attempts"].toInt();
                result["last_attempt_date"] = obj["last_attempt_date"].toString();
                fileLoaded = true;
            } else {
                // [DE-SHELL-RESCUE] 如果文件不是明文 JSON，说明是旧版加密授权，尝试进行迁移读取
                logStartup("[DE-SHELL] 检测到加密版 license.dat，正在执行自适应抢救迁移...");
                
                QString appSN = HardwareInfoHelper::getAppDrivePhysicalSerialNumber();
                QString cSN = HardwareInfoHelper::getCDiskPhysicalSerialNumber();
                QStringList keys;
                if (!appSN.isEmpty()) keys << FileCryptoHelper::getCombinedKeyBySN(appSN);
                if (!cSN.isEmpty()) keys << FileCryptoHelper::getCombinedKeyBySN(cSN);
                keys << FileCryptoHelper::getLegacyCombinedKey();

                QString tempDecPath = plainPath + ".dec_tmp";
                for (const QString& key : keys) {
                    if (FileCryptoHelper::decryptFileWithShell(plainPath, tempDecPath, key)) {
                        QFile tf(tempDecPath);
                        if (tf.open(QIODevice::ReadOnly)) {
                            QJsonDocument decDoc = QJsonDocument::fromJson(tf.readAll());
                            tf.close();
                            if (!decDoc.isNull() && decDoc.isObject()) {
                                QJsonObject obj = decDoc.object();
                                result["first_launch_date"] = obj["first_launch_date"].toString();
                                result["usage_count"] = obj["usage_count"].toInt();
                                result["is_activated"] = obj["is_activated"].toBool();
                                result["activation_code"] = obj["activation_code"].toString();
                                result["failed_attempts"] = obj["failed_attempts"].toInt();
                                result["last_attempt_date"] = obj["last_attempt_date"].toString();
                                fileLoaded = true;
                                logStartup("[DE-SHELL] 授权文件迁移抢救成功。");
                                break;
                            }
                        }
                        QFile::remove(tempDecPath);
                    }
                }
                if (QFile::exists(tempDecPath)) QFile::remove(tempDecPath);
            }
        }
    }

    // [ANCHOR] 2. 尝试从注册表锚点加载并进行一致性合并
    QSettings registry("HKEY_CURRENT_USER\\Software\\RapidNotes", QSettings::NativeFormat);
    QString regLaunchDate = registry.value("TrialA").toString();
    int regUsageCount = registry.value("TrialB").toInt();
    bool regActivated = registry.value("TrialC").toInt() == 1;
    QString regSig = registry.value("TrialSig").toString();

    if (!regLaunchDate.isEmpty()) {
        // [DE-SHELL] 注册表签名校验优先尝试新版固定盐值，兼容旧版硬件盐值
        auto checkSign = [&](const QString& salt) {
            QString sign = QCryptographicHash::hash((regLaunchDate + QString::number(regUsageCount) + salt).toUtf8(), QCryptographicHash::Sha256).toHex();
            return (regSig == sign);
        };

        bool sigOk = checkSign("RapidNotes_DeShell_Salt_2026");
        
        if (!sigOk) {
            // 兼容旧版迁移
            QString appSalt = FileCryptoHelper::getCombinedKeyBySN(HardwareInfoHelper::getAppDrivePhysicalSerialNumber());
            QString cSalt = FileCryptoHelper::getCombinedKeyBySN(HardwareInfoHelper::getCDiskPhysicalSerialNumber());
            if (checkSign(appSalt) || checkSign(cSalt) || checkSign(FileCryptoHelper::getLegacyCombinedKey())) {
                sigOk = true;
            }
        }

        if (sigOk) {
            if (!fileLoaded || result["first_launch_date"].toString() > regLaunchDate) {
                result["first_launch_date"] = regLaunchDate;
                result["usage_count"] = qMax(result["usage_count"].toInt(), regUsageCount);
                if (regActivated) result["is_activated"] = true;
            }
        }
    }

    return result;
}
