#include "ArchiveTaskHelper.h"
#include "../ContentPanel.h"
#include "../ToolTipOverlay.h"
#include "../../util/ShellHelper.h"

#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTimer>
#include <QPointer>
#include <QCoreApplication>
#include <QStringList>

namespace QuarkMeta {

void ArchiveTaskHelper::extractArchive(ContentPanel* panel, const QString& path) {
    if (!panel) return;

    // 确定压缩包路径：仅处理第一个选中文件
    QStringList selectedPaths = panel->getSelectedPaths();
    QString archivePath = !selectedPaths.isEmpty() ? selectedPaths.first() : path;
    if (archivePath.isEmpty() || !QFileInfo::exists(archivePath)) return;

    QFileInfo archiveInfo(archivePath);
    QString parentDir = archiveInfo.absolutePath();
    QString baseName  = archiveInfo.completeBaseName();

    // 自动生成唯一目标子文件夹名（同名递增编号）
    QString destDir = QDir(parentDir).filePath(baseName);
    if (QDir(destDir).exists()) {
        int counter = 1;
        while (QDir(QDir(parentDir).filePath(
            QString("%1-%2").arg(baseName).arg(counter))).exists()) {
            ++counter;
        }
        destDir = QDir(parentDir).filePath(QString("%1-%2").arg(baseName).arg(counter));
    }
    QDir().mkpath(destDir);

    QString ext = archiveInfo.suffix().toLower();

    if (ext == QStringLiteral("zip")) {
        // .zip：通过 QProcess 追踪进程退出，完成后安全刷新面板
        ToolTipOverlay::instance()->showToast(
            QString("正在解压 %1 ...").arg(archiveInfo.fileName()), 2000, QColor("#3498db")
        );

        QPointer<ContentPanel> weakPanel(panel);
        QString capturedArchive = archivePath;
        QString capturedDest    = destDir;

        QProcess* proc = new QProcess();
        QObject::connect(
            proc,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [weakPanel, proc, capturedArchive, capturedDest](int exitCode, QProcess::ExitStatus) {
                QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, exitCode, capturedArchive, capturedDest, proc]() {
                    if (weakPanel) weakPanel->refreshAll();
                    QFileInfo info(capturedArchive);
                    if (exitCode == 0) {
                        ToolTipOverlay::instance()->showToast(
                            QString("解压完成：%1").arg(info.fileName()), 2000, QColor("#2ecc71")
                        );
                    } else {
                        // 失败时清理可能为空的目标目录
                        QDir failDir(capturedDest);
                        if (failDir.exists() && failDir.isEmpty()) failDir.rmdir(capturedDest);
                        ToolTipOverlay::instance()->showToast(
                            QString("解压失败：%1").arg(info.fileName()), 2000, QColor("#e81123")
                        );
                    }
                    proc->deleteLater();
                });
            }
        );

        QString nativeSrc  = QDir::toNativeSeparators(capturedArchive);
        QString nativeDest = QDir::toNativeSeparators(capturedDest);
        QString script = QString(
            "Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force"
        ).arg(nativeSrc, nativeDest);
        proc->start(QStringLiteral("powershell"),
            QStringList() << QStringLiteral("-NoProfile")
                          << QStringLiteral("-NonInteractive")
                          << QStringLiteral("-WindowStyle") << QStringLiteral("Hidden")
                          << QStringLiteral("-Command") << script);
    } else {
        // 其他格式：委托系统关联程序（7-Zip / WinRAR），无法精确追踪退出
        ShellHelper::extractArchive(archivePath, destDir);
        ToolTipOverlay::instance()->showToast(
            QString("已委托系统程序处理，解压完成后将自动刷新"), 2500, QColor("#3498db")
        );
        QPointer<ContentPanel> weakPanel(panel);
        // 延迟 1.5s 刷新，给系统程序完成初始化的时间
        QTimer::singleShot(1500, QCoreApplication::instance(), [weakPanel]() {
            if (weakPanel) weakPanel->refreshAll();
        });
    }
}

void ArchiveTaskHelper::compressToZip(ContentPanel* panel, const QString& currentPath, const QString& path) {
    if (!panel) return;

    QStringList selectedPaths = panel->getSelectedPaths();
    if (selectedPaths.isEmpty() && !path.isEmpty()) {
        selectedPaths << path;
    }
    if (selectedPaths.isEmpty()) return;

    QString firstItem = selectedPaths.first();
    QFileInfo firstInfo(firstItem);
    QString parentDir = firstInfo.isDir() && selectedPaths.size() == 1
        ? firstInfo.dir().absolutePath()
        : (currentPath.isEmpty() || currentPath.contains("://") ? firstInfo.absolutePath() : currentPath);

    // 基础名称：文件夹取目录名，文件取主文件名（无后缀）
    QString baseName = firstInfo.isDir() ? firstInfo.fileName() : firstInfo.completeBaseName();
    if (baseName.isEmpty()) baseName = QStringLiteral("Archive");

    // 自动计算递增编号目标 zip 路径，防冲洗覆盖
    QString destZip = QDir(parentDir).filePath(baseName + QStringLiteral(".zip"));
    if (QFile::exists(destZip)) {
        int counter = 1;
        while (QFile::exists(QDir(parentDir).filePath(QString("%1-%2.zip").arg(baseName).arg(counter)))) {
            ++counter;
        }
        destZip = QDir(parentDir).filePath(QString("%1-%2.zip").arg(baseName).arg(counter));
    }

    QFileInfo destZipInfo(destZip);
    ToolTipOverlay::instance()->showToast(
        QString("正在将 %1 个项目压缩为 %2 ...").arg(selectedPaths.size()).arg(destZipInfo.fileName()),
        2000,
        QColor("#3498db")
    );

    // 构造 PowerShell 脚本：使用单引号与转义处理路径
    QStringList nativePaths;
    for (const QString& p : selectedPaths) {
        QString nPath = QDir::toNativeSeparators(p);
        nPath.replace("'", "''"); // PowerShell 单引号转义
        nativePaths << QString("'%1'").arg(nPath);
    }
    QString nativeDest = QDir::toNativeSeparators(destZip);
    nativeDest.replace("'", "''");

    QString script = QString(
        "$files = @(%1); Compress-Archive -LiteralPath $files -DestinationPath '%2' -Force"
    ).arg(nativePaths.join(", "), nativeDest);

    QPointer<ContentPanel> weakPanel(panel);
    QString capturedDestZip = destZip;

    QProcess* proc = new QProcess();
    QObject::connect(
        proc,
        QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [weakPanel, proc, capturedDestZip](int exitCode, QProcess::ExitStatus) {
            QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, exitCode, capturedDestZip, proc]() {
                if (weakPanel) weakPanel->refreshAll();
                QFileInfo info(capturedDestZip);
                if (exitCode == 0) {
                    ToolTipOverlay::instance()->showToast(
                        QString("压缩完成：%1").arg(info.fileName()), 2000, QColor("#2ecc71")
                    );
                } else {
                    // 若压缩失败且产生了残缺文件，则进行清理
                    if (QFile::exists(capturedDestZip)) {
                        QFile::remove(capturedDestZip);
                    }
                    ToolTipOverlay::instance()->showToast(
                        QString("压缩失败：%1").arg(info.fileName()), 2000, QColor("#e81123")
                    );
                }
                proc->deleteLater();
            });
        }
    );

    proc->start(
        QStringLiteral("powershell"),
        QStringList() << QStringLiteral("-NoProfile")
                      << QStringLiteral("-NonInteractive")
                      << QStringLiteral("-WindowStyle") << QStringLiteral("Hidden")
                      << QStringLiteral("-Command") << script
    );
}

} // namespace QuarkMeta
