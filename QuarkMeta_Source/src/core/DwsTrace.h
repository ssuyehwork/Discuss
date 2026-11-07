#pragma once

#include <QThread>
#include <QString>
#include <QCoreApplication>
#include "../ui/Logger.h"

// 目录监听追踪统一日志宏
#define QUARKMETA_DWS_TRACE_ENABLED 1

#if QUARKMETA_DWS_TRACE_ENABLED
#define QUARKMETA_DWS_TRACE(className, funcName, message) \
    do { \
        quintptr _dws_tid = reinterpret_cast<quintptr>(QThread::currentThreadId()); \
        QString _dws_msg = QString("[DWS-TRACE][tid=%1] %2::%3: %4") \
                            .arg(_dws_tid) \
                            .arg(className) \
                            .arg(funcName) \
                            .arg(message); \
        QuarkMeta::Logger::log(_dws_msg); \
    } while (0)
#else
#define QUARKMETA_DWS_TRACE(className, funcName, message) do {} while (0)
#endif
