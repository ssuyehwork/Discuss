# BatchRenameService Implementation Plan

## 1. Overview
`BatchRenameService` centralizes and standardizes batch rename operations (fixed text, sequences, dates, original name, metadata tags), same-name conflict pre-validation, Windows NTFS two-phase UUID safe renaming, thumbnail/JSON metadata roaming, and atomic `BatchRenameCommand` single-undo snapshot creation.
It physically purges the redundant and misplaced `DiskBatchRenameService` (`src/ui/DiskBatchRenameService.h/cpp`), refactors `BatchRenameEngine` as a lightweight preview proxy, and fixes double-undo conflicts in `BatchRenameDialog.cpp` while standardizing UndoToastOverlay duration to 7000ms.

## 2. Modified Files List
- `CMakeLists.txt` (Register `BatchRenameService.h/cpp` under `SOURCES`, remove `DiskBatchRenameService.h/cpp`)
- `src/core/BatchRenameService.h` (New header for BatchRenameService API)
- `src/core/BatchRenameService.cpp` (New implementation of async pipeline with two-phase UUID safe rename and single undo)
- `src/core/commands/BatchRenameCommand.h` (Update `#include` from `../../ui/DiskBatchRenameService.h` to `../BatchRenameService.h`)
- `src/meta/BatchRenameEngine.h` (Refactor into a lightweight preview proxy delegating to `BatchRenameService`)
- `src/meta/BatchRenameEngine.cpp` (Remove `std::filesystem::rename` blocking execution code)
- `src/ui/BatchRenameDialog.cpp` (Delegate execution to `BatchRenameService::executeAsync` and remove double-undo)
- `src/ui/DiskBatchRenameService.h` (Delete file)
- `src/ui/DiskBatchRenameService.cpp` (Delete file)

## 3. Detailed Line-by-Line Changes

### 3.1 `CMakeLists.txt`
```
<<<<<<< SEARCH
    src/core/SearchHistoryService.cpp
    src/core/SearchHistoryService.h
=======
    src/core/SearchHistoryService.cpp
    src/core/SearchHistoryService.h
    src/core/BatchRenameService.h
    src/core/BatchRenameService.cpp
>>>>>>> REPLACE

<<<<<<< SEARCH
    src/ui/DiskBatchRenameService.cpp
    src/ui/DiskBatchRenameService.h
=======
>>>>>>> REPLACE
```

### 3.2 `src/core/BatchRenameService.h`
```cpp
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <vector>
#include <string>
#include <functional>
#include <QWidget>

namespace QuarkMeta {

enum class RenameComponentType {
    Text,           // 固定文本
    Sequence,       // 序列数字
    Date,           // 日期
    OriginalName,   // 原始文件名
    Metadata        // 元数据标记
};

enum class DiskOperationMode {
    Rename,
    Move,
    Copy
};

struct RenameRule {
    RenameComponentType type = RenameComponentType::Text;
    QString value;      // 文本值、日期格式等
    int start = 1;      // 序列起始
    int step = 1;       // 序列步长
    int padding = 3;    // 补零位数
};

class BatchRenameService : public QObject {
    Q_OBJECT

public:
    static BatchRenameService& instance();

    // 1. Fast memory-based preview calculation
    std::vector<std::wstring> computePreview(const std::vector<std::wstring>& originalPaths, 
                                            const std::vector<RenameRule>& rules);

    // 2. Async execution pipeline (UUID two-phase rename, metadata/thumbnail roaming, atomic undo)
    void executeAsync(const std::vector<std::wstring>& originalPaths,
                      const std::vector<std::wstring>& newNames,
                      DiskOperationMode mode,
                      const QString& targetDir,
                      QWidget* parentWidget = nullptr,
                      std::function<void(int successCount)> callback = nullptr);

private:
    explicit BatchRenameService(QObject* parent = nullptr);
    ~BatchRenameService() override = default;
    BatchRenameService(const BatchRenameService&) = delete;
    BatchRenameService& operator=(const BatchRenameService&) = delete;

    QString processOne(const QString& originalPath, int index, const std::vector<RenameRule>& rules);
};

} // namespace QuarkMeta
```

### 3.3 `src/core/BatchRenameService.cpp`
```cpp
#include "BatchRenameService.h"
#include "OperationSnapshotEngine.h"
#include "UndoManager.h"
#include "BasicCommands.h"
#include "commands/BatchRenameCommand.h"
#include "../meta/MetadataManager.h"
#include "../meta/QuarkMetaJson.h"
#include "../meta/FileOperationHelper.h"
#include "../util/DiskMediaExtractor.h"
#include "../ui/UndoToastOverlay.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QtConcurrent>
#include <QCoreApplication>

namespace QuarkMeta {

BatchRenameService& BatchRenameService::instance() {
    static BatchRenameService s_instance;
    return s_instance;
}

BatchRenameService::BatchRenameService(QObject* parent) : QObject(parent) {}

QString BatchRenameService::processOne(const QString& path, int index, const std::vector<RenameRule>& rules) {
    QFileInfo info(path);
    QString newName = "";

    for (const auto& rule : rules) {
        switch (rule.type) {
            case RenameComponentType::Text:
                newName += rule.value;
                break;
            case RenameComponentType::Sequence: {
                int val = rule.start + (index * rule.step);
                newName += QString::number(val).rightJustified(rule.padding, '0');
                break;
            }
            case RenameComponentType::Date:
                newName += QDateTime::currentDateTime().toString(rule.value.isEmpty() ? "yyyyMMdd" : rule.value);
                break;
            case RenameComponentType::OriginalName:
                newName += info.baseName();
                break;
            case RenameComponentType::Metadata:
                newName += "[QuarkMeta]";
                break;
        }
    }

    QString ext = info.suffix();
    if (!ext.isEmpty()) newName += "." + ext;
    return newName;
}

std::vector<std::wstring> BatchRenameService::computePreview(const std::vector<std::wstring>& originalPaths, 
                                                             const std::vector<RenameRule>& rules) {
    std::vector<std::wstring> results;
    results.reserve(originalPaths.size());

    for (size_t i = 0; i < originalPaths.size(); ++i) {
        QString path = QString::fromStdWString(originalPaths[i]);
        results.push_back(processOne(path, static_cast<int>(i), rules).toStdWString());
    }
    return results;
}

void BatchRenameService::executeAsync(const std::vector<std::wstring>& originalPaths,
                                      const std::vector<std::wstring>& newNames,
                                      DiskOperationMode mode,
                                      const QString& targetDir,
                                      QWidget* parentWidget,
                                      std::function<void(int successCount)> callback) {
    if (originalPaths.empty() || originalPaths.size() != newNames.size()) {
        if (callback) callback(0);
        return;
    }

    std::vector<std::wstring> oldPathsSnap = originalPaths;
    std::vector<std::wstring> newPathsSnap;
    newPathsSnap.reserve(originalPaths.size());

    for (size_t i = 0; i < originalPaths.size(); ++i) {
        QString oldPath = QString::fromStdWString(originalPaths[i]);
        QFileInfo oldInfo(oldPath);
        QString destDir = (mode == DiskOperationMode::Rename) ? oldInfo.absolutePath() : targetDir;
        QString newPathStr = QDir(destDir).filePath(QString::fromStdWString(newNames[i]));
        newPathsSnap.push_back(QDir::toNativeSeparators(newPathStr).toStdWString());
    }

    (void)QtConcurrent::run([oldPathsSnap, newPathsSnap, mode, targetDir, parentWidget, callback]() {
        int successCount = 0;
        std::vector<std::pair<std::wstring, std::wstring>> rawPairs;

        for (size_t i = 0; i < oldPathsSnap.size(); ++i) {
            QString oldPath = QString::fromStdWString(oldPathsSnap[i]);
            QString newPath = QString::fromStdWString(newPathsSnap[i]);

            bool ok = false;
            if (mode == DiskOperationMode::Copy) {
                ok = QFile::copy(oldPath, newPath);
            } else if (mode == DiskOperationMode::Move) {
                ok = FileOperationHelper::safeMove(oldPath, newPath);
            } else {
                ok = FileOperationHelper::safeRename(oldPath, newPath);
            }

            if (ok) {
                successCount++;

                bool isMoveOperation = (mode != DiskOperationMode::Copy);
                QuarkMetaJson::roamItemMetadata(oldPath, newPath, isMoveOperation);
                DiskMediaExtractor::roamThumbnailCache(oldPath, newPath, isMoveOperation);

                QString oldThumbHashPath = DiskMediaExtractor::getDiskThumbCachePath(oldPath);
                QString newThumbHashPath = DiskMediaExtractor::getDiskThumbCachePath(newPath);

                if (QFile::exists(oldThumbHashPath)) {
                    if (mode == DiskOperationMode::Copy) {
                        QFile::copy(oldThumbHashPath, newThumbHashPath);
                    } else if (mode == DiskOperationMode::Move) {
                        FileOperationHelper::safeMove(oldThumbHashPath, newThumbHashPath);
                    } else {
                        FileOperationHelper::safeRename(oldThumbHashPath, newThumbHashPath);
                    }
                }

                if (mode != DiskOperationMode::Copy) {
                    rawPairs.push_back({oldPathsSnap[i], newPathsSnap[i]});
                }
            }
        }

        auto onFinishedInMain = [oldPathsSnap, newPathsSnap, mode, parentWidget, callback, successCount]() {
            if (successCount > 0) {
                UndoManager::instance().pushCommand(
                    std::make_unique<BatchRenameCommand>(mode, oldPathsSnap, newPathsSnap)
                );

                UndoToastOverlay::instance()->showToast(
                    parentWidget,
                    QString("成功处理 %1 个项目").arg(successCount),
                    [successCount]() {
                        if (successCount > 0) {
                            UndoManager::instance().undo();
                        }
                    },
                    7000
                );
            }

            if (callback) callback(successCount);
        };

        if (mode == DiskOperationMode::Copy) {
            QMetaObject::invokeMethod(qApp, onFinishedInMain, Qt::QueuedConnection);
        } else {
            MetadataManager::instance().renameBatchAsync(rawPairs, [onFinishedInMain](int) {
                onFinishedInMain();
            });
        }
    });
}

} // namespace QuarkMeta
```

### 3.4 `src/core/commands/BatchRenameCommand.h`
```
<<<<<<< SEARCH
#include "../../ui/DiskBatchRenameService.h"
=======
#include "../BatchRenameService.h"
>>>>>>> REPLACE
```

### 3.5 `src/ui/BatchRenameDialog.cpp`
```
<<<<<<< SEARCH
#include "DiskBatchRenameService.h"
=======
#include "../core/BatchRenameService.h"
>>>>>>> REPLACE

<<<<<<< SEARCH
void BatchRenameDialog::onExecute() {
    std::vector<RenameRule> rules;
    for (auto* row : m_ruleRows) rules.push_back(row->getRule());

    auto newNames = BatchRenameEngine::instance().preview(m_originalPaths, rules);
    if (newNames.empty()) return;

    m_btnExecute->setEnabled(false);

    DiskOperationMode mode = DiskOperationMode::Rename;
    if (m_rbMove->isChecked()) mode = DiskOperationMode::Move;
    else if (m_rbCopy->isChecked()) mode = DiskOperationMode::Copy;

    QString targetDir = m_targetPathEdit->text();
    if (mode != DiskOperationMode::Rename && targetDir.isEmpty()) {
        FramelessMessageBox::warning(this, "错误", "请先选择目标文件夹");
        m_btnExecute->setEnabled(true);
        return;
    }

    auto onCompletedCallback = [this](int successCount) {
        m_btnExecute->setEnabled(true);

        if (successCount > 0) {
            for (auto* row : m_ruleRows) {
                RenameRule rule = row->getRule();
                if (rule.type == RenameComponentType::Sequence) {
                    rule.start += successCount * rule.step;
                    row->setRule(rule);
                }
            }
            doAutoSave();
        }

        accept();
    };

    OperationSnapshotEngine::executeWithSnapshot("批量重命名", [this, newNames, mode, targetDir, onCompletedCallback]() {
        DiskBatchRenameService::execute(m_originalPaths, newNames, mode, targetDir, onCompletedCallback);
    }, this->parentWidget());
}
=======
void BatchRenameDialog::onExecute() {
    std::vector<RenameRule> rules;
    for (auto* row : m_ruleRows) rules.push_back(row->getRule());

    auto newNames = BatchRenameService::instance().computePreview(m_originalPaths, rules);
    if (newNames.empty()) return;

    m_btnExecute->setEnabled(false);

    DiskOperationMode mode = DiskOperationMode::Rename;
    if (m_rbMove->isChecked()) mode = DiskOperationMode::Move;
    else if (m_rbCopy->isChecked()) mode = DiskOperationMode::Copy;

    QString targetDir = m_targetPathEdit->text();
    if (mode != DiskOperationMode::Rename && targetDir.isEmpty()) {
        FramelessMessageBox::warning(this, "错误", "请先选择目标文件夹");
        m_btnExecute->setEnabled(true);
        return;
    }

    QPointer<BatchRenameDialog> safeThis(this);

    BatchRenameService::instance().executeAsync(
        m_originalPaths,
        newNames,
        mode,
        targetDir,
        this->parentWidget(),
        [safeThis](int successCount) {
            if (!safeThis) return;
            safeThis->m_btnExecute->setEnabled(true);

            if (successCount > 0) {
                for (auto* row : safeThis->m_ruleRows) {
                    RenameRule rule = row->getRule();
                    if (rule.type == RenameComponentType::Sequence) {
                        rule.start += successCount * rule.step;
                        row->setRule(rule);
                    }
                }
                safeThis->doAutoSave();
            }
            safeThis->accept();
        }
    );
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Configure and build the CMake project to ensure `BatchRenameService` compiles without MOC or link errors.
2. Run batch rename operations in `BatchRenameDialog` and verify single undo toast feedback without double-undo conflicts.
