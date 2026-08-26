#pragma once
#include "ActionCommand.h"
#include "../meta/MetadataManager.h"
#include "../util/ShellHelper.h"
#include <QString>
#include <QVariant>
#include <QFileInfo>
#include <QDir>
#include <string>
#include <vector>
#include <utility>

#include <QtConcurrent>
#include <QCoreApplication>
#include "../meta/FileOperationHelper.h"
#include "../util/DiskMediaExtractor.h"
#include "../ui/DiskBatchRenameService.h"

namespace QuarkMeta {

/**
 * @brief 重命名指令
 */
class RenameCommand : public ActionCommand {
public:
    RenameCommand(const QString& oldPath, const QString& newPath)
        : m_oldPath(oldPath), m_newPath(newPath) {}

    void execute() override {
        // 第一次 execute 由外部触发物理操作后调用 pushCommand，
        // 或者在 redo 时被调用。
    }

    void undo() override {
        ShellHelper::renameItem(m_newPath, m_oldPath);
    }

    void redo() override {
        ShellHelper::renameItem(m_oldPath, m_newPath);
    }

    QString description() const override { return "重命名"; }

    bool affectsPath(const QString& path) const override {
        return m_oldPath == path || m_newPath == path;
    }

private:
    QString m_oldPath;
    QString m_newPath;
};

/**
 * @brief 移动/剪切指令
 */
class MoveCommand : public ActionCommand {
public:
    MoveCommand(const QStringList& sourcePaths, const QString& oldDir, const QString& newDir)
        : m_oldDir(oldDir), m_newDir(newDir) {
        for (const QString& p : sourcePaths) {
            m_fileNames << QFileInfo(p).fileName();
        }
    }

    void execute() override {}

    void undo() override {
        QStringList currentPaths;
        for (const QString& name : m_fileNames) {
            currentPaths << QDir(m_newDir).filePath(name);
        }
        ShellHelper::copyOrMoveItems(currentPaths, m_oldDir, true);
    }

    void redo() override {
        QStringList currentPaths;
        for (const QString& name : m_fileNames) {
            currentPaths << QDir(m_oldDir).filePath(name);
        }
        ShellHelper::copyOrMoveItems(currentPaths, m_newDir, true);
    }

    QString description() const override { return "移动文件"; }

    bool affectsPath(const QString& path) const override {
        if (path.startsWith(m_oldDir) || path.startsWith(m_newDir)) return true;
        for (const QString& name : m_fileNames) {
            if (QDir(m_oldDir).filePath(name) == path || QDir(m_newDir).filePath(name) == path) return true;
        }
        return false;
    }

private:
    QStringList m_fileNames;
    QString m_oldDir;
    QString m_newDir;
};

/**
 * @brief 元数据变更指令 (星级、颜色)
 */
class MetadataCommand : public ActionCommand {
public:
    enum Type { Rating, Color };
    MetadataCommand(const QString& path, Type type, const QVariant& oldVal, const QVariant& newVal)
        : m_path(path), m_type(type), m_oldVal(oldVal), m_newVal(newVal) {}

    void execute() override {}

    void undo() override {
        applyValue(m_oldVal);
    }

    void redo() override {
        applyValue(m_newVal);
    }

    QString description() const override { return m_type == Rating ? "更改星级" : "更改颜色"; }

    bool affectsPath(const QString& path) const override {
        return m_path == path;
    }

private:
    void applyValue(const QVariant& val) {
        if (m_type == Rating) {
            MetadataManager::instance().setRating(m_path.toStdWString(), val.toInt());
        } else {
            MetadataManager::instance().setColor(m_path.toStdWString(), val.toString().toStdWString());
        }
    }

    QString m_path;
    Type m_type;
    QVariant m_oldVal;
    QVariant m_newVal;
};


/**
 * @brief 安全物理删除指令
 */
class SecureDeleteCommand : public ActionCommand {
public:
    explicit SecureDeleteCommand(const QStringList& paths) : m_targetPaths(paths) {}

    void execute() override {
        for (const auto& path : m_targetPaths) {
            QFileInfo info(path);
            if (info.isDir()) {
                QDir dir(path);
                dir.removeRecursively();
            } else {
                QFile::remove(path);
            }
            MetadataManager::instance().removeMetadataSync(path.toStdWString());
        }
    }

    void undo() override {
        // 物理删除不可撤销，此接口留空
    }

    void redo() override {
        execute();
    }

    QString description() const override { return "安全物理删除"; }

    bool affectsPath(const QString& path) const override {
        for (const auto& p : m_targetPaths) {
            if (p == path) return true;
        }
        return false;
    }

private:
    QStringList m_targetPaths;
};

/**
 * @brief 安全加密指令
 */
class EncryptCommand : public ActionCommand {
public:
    EncryptCommand(const QStringList& paths, const std::string& pwd)
        : m_targetPaths(paths), m_pwd(pwd) {}

    void execute() override {
        // 后台加密由外部 QThreadPool 调用，此处指令记录该状态并支持后续扩展
    }

    void undo() override {
        // 解除加密
    }

    void redo() override {
        execute();
    }

    QString description() const override { return "安全加密保护"; }

    bool affectsPath(const QString& path) const override {
        for (const auto& p : m_targetPaths) {
            if (p == path || (p + ".amenc") == path) return true;
        }
        return false;
    }

private:
    QStringList m_targetPaths;
    std::string m_pwd;
};

/**
 * @brief 2026-06-xx 按照用户期望：增加批量重命名/移动/复制大事务的撤销与重做指令
 */
class BatchRenameCommand : public ActionCommand {
public:
    BatchRenameCommand(DiskOperationMode mode,
                       const std::vector<std::wstring>& oldPaths,
                       const std::vector<std::wstring>& newPaths)
        : m_mode(mode), m_oldPaths(oldPaths), m_newPaths(newPaths) {}

    void execute() override {
        // 第一次 execute 已由 BatchRenameDialog 直接执行，无需重复操作
    }

    void undo() override {
        // 进行安全的局部变量值捕获，彻底避免 Lambda 后台线程运行期间 "this" 被释放析构带来的 Use-After-Free 悬空崩溃隐患
        std::vector<std::wstring> oldPaths = m_oldPaths;
        std::vector<std::wstring> newPaths = m_newPaths;
        DiskOperationMode mode = m_mode;

        (void)QtConcurrent::run([oldPaths, newPaths, mode]() {
            std::vector<std::pair<std::wstring, std::wstring>> rawPairs;
            for (size_t i = 0; i < oldPaths.size(); ++i) {
                QString oldPathStr = QString::fromStdWString(oldPaths[i]);
                QString newPathStr = QString::fromStdWString(newPaths[i]);

                if (mode == DiskOperationMode::Copy) {
                    QFile::remove(newPathStr);
                    QString newThumbHashPath = DiskMediaExtractor::getDiskThumbCachePath(newPathStr);
                    if (QFile::exists(newThumbHashPath)) {
                        QFile::remove(newThumbHashPath);
                    }
                } else {
                    bool ok = false;
                    if (mode == DiskOperationMode::Move) {
                        ok = FileOperationHelper::safeMove(newPathStr, oldPathStr);
                    } else {
                        ok = FileOperationHelper::safeRename(newPathStr, oldPathStr);
                    }

                    if (ok) {
                        QString oldThumbHashPath = DiskMediaExtractor::getDiskThumbCachePath(oldPathStr);
                        QString newThumbHashPath = DiskMediaExtractor::getDiskThumbCachePath(newPathStr);
                        if (QFile::exists(newThumbHashPath)) {
                            if (mode == DiskOperationMode::Move) {
                                FileOperationHelper::safeMove(newThumbHashPath, oldThumbHashPath);
                            } else {
                                FileOperationHelper::safeRename(newThumbHashPath, oldThumbHashPath);
                            }
                        }
                        rawPairs.push_back({newPaths[i], oldPaths[i]});
                    }
                }
            }

            if (!rawPairs.empty()) {
                MetadataManager::instance().renameBatchAsync(rawPairs);
            }
        });
    }

    void redo() override {
        // 进行安全的局部变量值捕获，彻底避免 Lambda 后台线程运行期间 "this" 被释放析构带来的 Use-After-Free 悬空崩溃隐患
        std::vector<std::wstring> oldPaths = m_oldPaths;
        std::vector<std::wstring> newPaths = m_newPaths;
        DiskOperationMode mode = m_mode;

        (void)QtConcurrent::run([oldPaths, newPaths, mode]() {
            std::vector<std::pair<std::wstring, std::wstring>> rawPairs;
            for (size_t i = 0; i < oldPaths.size(); ++i) {
                QString oldPathStr = QString::fromStdWString(oldPaths[i]);
                QString newPathStr = QString::fromStdWString(newPaths[i]);

                if (mode == DiskOperationMode::Copy) {
                    if (QFile::copy(oldPathStr, newPathStr)) {
                        QString oldThumbHashPath = DiskMediaExtractor::getDiskThumbCachePath(oldPathStr);
                        QString newThumbHashPath = DiskMediaExtractor::getDiskThumbCachePath(newPathStr);
                        if (QFile::exists(oldThumbHashPath)) {
                            QFile::copy(oldThumbHashPath, newThumbHashPath);
                        }
                    }
                } else {
                    bool ok = false;
                    if (mode == DiskOperationMode::Move) {
                        ok = FileOperationHelper::safeMove(oldPathStr, newPathStr);
                    } else {
                        ok = FileOperationHelper::safeRename(oldPathStr, newPathStr);
                    }

                    if (ok) {
                        QString oldThumbHashPath = DiskMediaExtractor::getDiskThumbCachePath(oldPathStr);
                        QString newThumbHashPath = DiskMediaExtractor::getDiskThumbCachePath(newPathStr);
                        if (QFile::exists(oldThumbHashPath)) {
                            if (mode == DiskOperationMode::Move) {
                                FileOperationHelper::safeMove(oldThumbHashPath, newThumbHashPath);
                            } else {
                                FileOperationHelper::safeRename(oldThumbHashPath, newThumbHashPath);
                            }
                        }
                        rawPairs.push_back({oldPaths[i], newPaths[i]});
                    }
                }
            }

            if (!rawPairs.empty()) {
                MetadataManager::instance().renameBatchAsync(rawPairs);
            }
        });
    }

    QString description() const override {
        switch (m_mode) {
            case DiskOperationMode::Rename: return "批量重命名 (磁盘)";
            case DiskOperationMode::Move: return "批量移动";
            case DiskOperationMode::Copy: return "批量复制";
        }
        return "批量操作";
    }

    bool affectsPath(const QString& path) const override {
        std::wstring p = QDir::toNativeSeparators(path).toStdWString();
        for (const auto& op : m_oldPaths) {
            if (op == p) return true;
        }
        for (const auto& np : m_newPaths) {
            if (np == p) return true;
        }
        return false;
    }

private:
    DiskOperationMode m_mode;
    std::vector<std::wstring> m_oldPaths;
    std::vector<std::wstring> m_newPaths;
};

} // namespace QuarkMeta
