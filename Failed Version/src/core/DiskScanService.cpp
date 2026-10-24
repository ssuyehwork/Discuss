#include "DiskScanService.h" 
#include "FileFilterService.h" 
#include "../ui/Logger.h"
#include <QDir> 
#include <QFileInfo> 
#include <QSet>
#include <QElapsedTimer>
 
namespace QuarkMeta { 
 
std::vector<ItemRecord> DiskScanService::scanDirectory(const QString& path, 
                                                        bool recursive, 
                                                        const std::function<bool()>& shouldContinue,
                                                        int maxDepth) { 
    QElapsedTimer scanTimer;
    scanTimer.start();
    std::vector<ItemRecord> allItems; 
    QSet<QString> visitedDirs;

    std::function<void(const QString&, bool, int)> scanDir; 
    scanDir = [&](const QString& p, bool rec, int currentDepth) { 
        if (currentDepth > maxDepth) return;

        QDir dir(p); 
        if (!dir.exists()) return; 

        QString canonicalDir = dir.canonicalPath();
        if (!canonicalDir.isEmpty()) {
            if (visitedDirs.contains(canonicalDir)) return;
            visitedDirs.insert(canonicalDir);
        }
 
        QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden, QDir::DirsFirst | QDir::Name); 
        for (const QFileInfo& info : entries) { 
            if (shouldContinue && !shouldContinue()) return; 
 
            QString absPath = info.absoluteFilePath(); 
             
            // 🚨 统一调用文件过滤服务（归一化处理所有辅助文件、.arc、.QuarkMeta） 
            if (FileFilterService::isAuxiliaryFile(absPath)) continue; 
 
            ItemRecord itemRec = ItemRecord::create(absPath, nullptr); 
            allItems.push_back(itemRec); 
 
            if (rec && info.isDir() && !info.isSymLink()) { 
                scanDir(absPath, true, currentDepth + 1); 
            } 
        } 
    }; 
 
    scanDir(path, recursive, 0); 
    QuarkMeta::Logger::log(QString("[PerfTrace] DiskScanService::scanDirectory(%1) scanned %2 items in %3ms").arg(path).arg(allItems.size()).arg(scanTimer.elapsed()));
    return allItems; 
}

std::vector<ItemRecord> DiskScanService::scanDirectory(const QString& path, 
                                                        bool recursive, 
                                                        std::shared_ptr<CancellationToken> token) { 
    return scanDirectory(path, recursive, [token]() {
        return token ? !token->isCanceled() : true;
    });
}

} // namespace QuarkMeta
