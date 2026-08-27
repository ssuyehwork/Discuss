# QuarkMeta 批量重命名服务归一化实施方案 (BatchRenameService)

## 1. 目标与范围
- 新建 `BatchRenameService`（位于 `src/core/`）：归一化整合规则解析计算、同名冲突前置校验、Windows 两阶段 UUID 安全重命名、缩略图与 JSON 自动漫游、以及单一撤销快照闭环。
- 彻底物理删除分层错位且重复的冗余文件：`src/ui/DiskBatchRenameService.h` 与 `src/ui/DiskBatchRenameService.cpp`。
- 净化 `BatchRenameEngine.cpp`：废除在主线程裸调 `std::filesystem::rename` 的劣质执行代码。
- 净化 `BatchRenameDialog.cpp`：彻底消灭双重撤销（`UndoManager` 与 `nullptr` 快照打架）的 Bug，将 Toast 提示时长规范化为 7 秒（7000ms）。

---

## 2. 归一化核心模块实现

### 2.1 `src/core/BatchRenameService.h`
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

    /**
     * @brief 纯内存毫秒级预览计算
     */
    std::vector<std::wstring> computePreview(const std::vector<std::wstring>& originalPaths, 
                                            const std::vector<RenameRule>& rules);

    /**
     * @brief 异步物理磁盘重命名/移动/复制执行流水线 (含两阶段 UUID 安全中转与元数据漫游)
     */
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

### 2.2 `src/core/BatchRenameService.cpp`
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

    // 1. 构建物理路径映射快照 (用于撤销管道)
    std::vector<std::wstring> oldPathsSnap = originalPaths;
    std::vector<std::wstring> newPathsSnap;
    newPathsSnap.reserve(originalPaths.size());

    QStringList targetPathsList;
    for (size_t i = 0; i < originalPaths.size(); ++i) {
        QString oldPath = QString::fromStdWString(originalPaths[i]);
        targetPathsList << oldPath;

        QFileInfo oldInfo(oldPath);
        QString destDir = (mode == DiskOperationMode::Rename) ? oldInfo.absolutePath() : targetDir;
        QString newPathStr = QDir(destDir).filePath(QString::fromStdWString(newNames[i]));
        newPathsSnap.push_back(QDir::toNativeSeparators(newPathStr).toStdWString());
    }

    // 2. 统一交由后台线程池执行物理写盘与两阶段 UUID 安全重命名
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
            } else { // Rename
                // 强制调用两阶段 UUID 中转重命名，解决 Windows NTFS 大小写不敏感缺陷
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

        // 3. 回到主线程执行数据库异步更新、撤销命令推入与 Toast 提示
        auto onFinishedInMain = [oldPathsSnap, newPathsSnap, mode, parentWidget, callback, successCount]() {
            if (successCount > 0) {
                // 向 UndoManager 注入单次原子的 BatchRenameCommand
                UndoManager::instance().pushCommand(
                    std::make_unique<BatchRenameCommand>(mode, oldPathsSnap, newPathsSnap)
                );

                // 弹出统一规范的 7 秒 (7000ms) 撤销 Toast
                UndoToastOverlay::instance()->showToast(
                    parentWidget,
                    QString("成功处理 %1 个项目").arg(successCount),
                    [successCount]() {
                        if (successCount > 0) {
                            UndoManager::instance().undo();
                        }
                    },
                    7000 // 👈 统一 7 秒规范
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

---

## 3. `BatchRenameEngine.h` 瘦身

废除 `BatchRenameEngine::execute` 冗余代码，使其成为轻量级计算代理：

```cpp
#pragma once

#include "BatchRenameService.h"

namespace QuarkMeta {

class BatchRenameEngine {
public:
    static BatchRenameEngine& instance() {
        static BatchRenameEngine inst;
        return inst;
    }

    std::vector<std::wstring> preview(const std::vector<std::wstring>& originalPaths, 
                                     const std::vector<RenameRule>& rules) {
        return BatchRenameService::instance().computePreview(originalPaths, rules);
    }

private:
    BatchRenameEngine() = default;
    ~BatchRenameEngine() = default;
};

} // namespace QuarkMeta
```

---

## 4. `BatchRenameDialog.cpp` 净化改造

消灭对话框内部手写的路径遍历、模式分流与双重撤销冲突：

```cpp
#include "BatchRenameDialog.h"
#include "RuleRow.h"
#include "UiHelper.h"
#include "PresetManager.h"
#include "ShellIconManager.h"
#include "../util/DiskMediaExtractor.h"
#include "../core/BatchRenameService.h"
#include "../ui/dialogs/FramelessMessageBox.h"
#include <QFileInfo>
#include <QDir>
#include <QPointer>

namespace QuarkMeta {

// ... [构造函数、initContent、applyTheme、onAddRow 保持不变] ...

void BatchRenameDialog::updatePreview() {
    if (m_isInitializing) return;

    std::vector<RenameRule> rules;
    for (auto* row : m_ruleRows) {
        if (row) rules.push_back(row->getRule());
    }

    // 统一调用 BatchRenameService 毫秒级预览
    auto newNames = BatchRenameService::instance().computePreview(m_originalPaths, rules);
    int total = static_cast<int>(newNames.size());

    for (int i = 0; i < total; ++i) {
        QTableWidgetItem* itemNew = m_table->item(i, 1);
        if (itemNew) {
            itemNew->setText(QString::fromStdWString(newNames[static_cast<size_t>(i)]));
        }
    }
}

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

    // 🚀【单一行标准调度】：交由 BatchRenameService 统筹执行、安全中转与单向 7 秒快照
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

} // namespace QuarkMeta
```

---

## 5. 物理清理与构建配置更新

### 5.1 物理删除废弃文件
- 删除 `src/ui/DiskBatchRenameService.h`
- 删除 `src/ui/DiskBatchRenameService.cpp`

### 5.2 `CMakeLists.txt` 构建配置注册
```cmake
set(CORE_SOURCES
    # ... 现有 core 源文件 ...
    src/core/BatchRenameService.h
    src/core/BatchRenameService.cpp
)

set(UI_SOURCES
    # ... 现有 UI 源文件 ...
    # 🚨 移除 DiskBatchRenameService.h 与 DiskBatchRenameService.cpp
)
```