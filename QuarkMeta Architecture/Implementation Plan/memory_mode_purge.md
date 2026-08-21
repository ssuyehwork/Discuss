# 纯磁盘目录模式·托管库与内存模式遗留代码物理彻底根除实施方案 (Memory Mode & Managed Library Legacy Purge Implementation Plan)

## 1. Overview (概述与解决的问题)

QuarkMeta 现已全面升级为**纯磁盘目录直连模式独立应用**，彻底摒弃了原有的“内存托管库/镜像数据库（Memory Mode）”及分类索引体系。
但在从双模式版本拆分剥离的过程中，代码库中依然残存了部分已废弃模块的“幽灵引用”（如 `#include` 包含、废弃成员变量、遗留槽函数关联）以及衍生的僵尸源码文件。

本实施方案旨在全面、干净地彻底根除项目中的所有内存模式与托管库遗留，包括：
1. **清理用户已删 18 个文件在源码中的残留头文件包含与逻辑调用**（涉及 `AssetImporter`、`ImportHelper`、`CategoryLockDialog`、`CategoryLockWidget` 等）。
2. **清理衍生的孤立僵尸源码文件**（物理删除磁盘上残存的 `CategoryPanel.h/cpp`、`CategoryLockDialog.h/cpp`、`CategoryLockWidget.h/cpp`、`AssetImporter.h/cpp`、`ImportHelper.h/cpp`、`AmMetaJson.h/cpp`、`CategoryModel.h/cpp`、`CategoryFilterProxyModel.h`、`CategoryDelegate.h/cpp`、`CategoryBindingManager.h/cpp`、`SyncStatusService.h/cpp`）。
3. **清理 `BatchCreateDialog` 中的内存模式分支逻辑**（移除 `isMemoryMode` 构造参数、`m_isMemoryMode` 成员及 `m_libraryCombo` 托管库选择下拉框相关死代码）。
4. **清理 `MetadataManager.cpp` 和 `MediaExtractorPipeline.cpp` 中的遗留同步/分类刷新信号**。
5. **同步更新 `CMakeLists.txt`**（彻底剔除 `SyncStatusService.cpp/.h` 的编译注册）。

---

## 2. Modified Files List (影响文件清单)

### 2.1 修改的现有源文件与构建项 (Modified Files)
1. `CMakeLists.txt`
2. `src/ui/MainWindow.cpp`
3. `src/ui/ContentPanel.h`
4. `src/ui/ContentPanel.cpp`
5. `src/core/CoreController.cpp`
6. `src/meta/MediaExtractorPipeline.cpp`
7. `src/meta/MetadataManager.cpp`
8. `src/ui/BatchCreateDialog.h`
9. `src/ui/BatchCreateDialog.cpp`

### 2.2 物理删除的僵尸文件 (Deleted Files)
1. `src/util/AssetImporter.h` / `src/util/AssetImporter.cpp`
2. `src/util/ImportHelper.h` / `src/util/ImportHelper.cpp`
3. `src/ui/CategoryLockDialog.h` / `src/ui/CategoryLockDialog.cpp`
4. `src/ui/CategoryLockWidget.h` / `src/ui/CategoryLockWidget.cpp`
5. `src/ui/CategoryPanel.h` / `src/ui/CategoryPanel.cpp`
6. `src/meta/AmMetaJson.h` / `src/meta/AmMetaJson.cpp`
7. `src/ui/CategoryModel.h` / `src/ui/CategoryModel.cpp`
8. `src/ui/CategoryFilterProxyModel.h`
9. `src/ui/CategoryDelegate.h` / `src/ui/CategoryDelegate.cpp`
10. `src/meta/CategoryBindingManager.h` / `src/meta/CategoryBindingManager.cpp`
11. `src/core/SyncStatusService.h` / `src/core/SyncStatusService.cpp`

---

## 3. Detailed Line-by-Line Changes (精准替换块)

### 3.1 `CMakeLists.txt`
```
<<<<<<< SEARCH
    src/core/SearchHistoryService.cpp
    src/core/SearchHistoryService.h
    src/core/SyncStatusService.cpp
    src/core/SyncStatusService.h
    src/crypto/EncryptionManager.cpp
=======
    src/core/SearchHistoryService.cpp
    src/core/SearchHistoryService.h
    src/crypto/EncryptionManager.cpp
>>>>>>> REPLACE
```

### 3.2 `src/ui/MainWindow.cpp`
```
<<<<<<< SEARCH
#include "../core/SearchHistoryService.h"
#include "../core/SyncStatusService.h"
#include "DriveButton.h"
#include "TagManagerDialog.h"
#include "../util/ShellHelper.h"
#include "../util/ImportHelper.h"
#include "../util/AssetImporter.h"
=======
#include "../core/SearchHistoryService.h"
#include "DriveButton.h"
#include "TagManagerDialog.h"
#include "../util/ShellHelper.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 2. 监听后台扫描状态变动
    connect(&SyncStatusService::instance(), &SyncStatusService::statusUpdated,
            this, [this, formatTime](bool syncing, int pendingCount) {
        if (syncing && pendingCount > 0) {
            if (m_syncStartTime == 0) {
                m_syncStartTime = QDateTime::currentMSecsSinceEpoch();
                m_totalBatchCount = pendingCount;
                m_elapsedTimer->start();
                updateProgressBarGeometry();

                m_topProgressBar->setValue(1);
                m_topProgressBar->show();
            }

            if (pendingCount > m_totalBatchCount) {
                m_totalBatchCount = pendingCount;
            }

            int completedCount = m_totalBatchCount - pendingCount;
            int pct = qBound(1, (int)((double)completedCount / m_totalBatchCount * 100), 99);
            m_topProgressBar->setValue(pct);
        } else {
            if (m_syncStartTime > 0) {
                m_topProgressBar->setValue(100);
                m_elapsedTimer->stop();

                qint64 totalSec = (QDateTime::currentMSecsSinceEpoch() - m_syncStartTime) / 1000;

                // 完成时展示标准格式
                m_statusLeft->setText(QString("数据扫描完成  数量：%1  |  实际耗时: %2")
                                      .arg(m_totalBatchCount)
                                      .arg(formatTime(totalSec)));

                // 400ms 后隐藏顶层进度条，3 秒后恢复常态项目计数
                QTimer::singleShot(400, this, [this]() {
                    m_topProgressBar->hide();
                    m_syncStartTime = 0;
                    m_totalBatchCount = 0;
                });
            }
        }
    });
=======
>>>>>>> REPLACE
```

### 3.3 `src/ui/ContentPanel.h`
```
<<<<<<< SEARCH
namespace QuarkMeta {

class CategoryLockWidget;

struct RuntimeMeta;
=======
namespace QuarkMeta {

struct RuntimeMeta;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QVBoxLayout* m_mainLayout = nullptr;
    QStackedWidget* m_viewStack = nullptr;
    CategoryLockWidget* m_lockWidget = nullptr;
    QPushButton* m_btnLayers = nullptr;
=======
    QVBoxLayout* m_mainLayout = nullptr;
    QStackedWidget* m_viewStack = nullptr;
    QPushButton* m_btnLayers = nullptr;
>>>>>>> REPLACE
```

### 3.4 `src/ui/ContentPanel.cpp`
```
<<<<<<< SEARCH
#include "../crypto/EncryptionManager.h"
#include "CategoryLockDialog.h"
#include "CategoryLockWidget.h"
#include "BatchRenameDialog.h"
=======
#include "../crypto/EncryptionManager.h"
#include "BatchRenameDialog.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    initGridView();
    initListView();

    m_lockWidget = new CategoryLockWidget(this);

    m_viewStack->addWidget(m_gridView);
    m_viewStack->addWidget(m_treeView);
    m_viewStack->addWidget(m_lockWidget);

    m_viewStack->setCurrentWidget(m_gridView);

    connect(m_lockWidget, &CategoryLockWidget::unlocked, this, [this](int id) {
        MainWindow* mw = nullptr;
        QWidget* parentWin = window();
        while (parentWin) {
            if ((mw = qobject_cast<MainWindow*>(parentWin))) break;
            parentWin = parentWin->parentWidget();
        }
        loadCategory(id);
    });
=======
    initGridView();
    initListView();

    m_viewStack->addWidget(m_gridView);
    m_viewStack->addWidget(m_treeView);

    m_viewStack->setCurrentWidget(m_gridView);
>>>>>>> REPLACE
```

### 3.5 `src/core/CoreController.cpp`
```
<<<<<<< SEARCH
#include "PhysicalDiskSearchExtractor.h"
#include "../util/AssetImporter.h"

namespace QuarkMeta {
=======
#include "PhysicalDiskSearchExtractor.h"

namespace QuarkMeta {
>>>>>>> REPLACE
```

### 3.6 `src/meta/MediaExtractorPipeline.cpp`
```
<<<<<<< SEARCH
#include "../ui/ImageDecoderFacade.h"
#include "../ui/ColorAlgorithmEngine.h"
#include "../core/SyncStatusService.h"
#include "DatabaseManager.h"
=======
#include "../ui/ImageDecoderFacade.h"
#include "../ui/ColorAlgorithmEngine.h"
#include "DatabaseManager.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        SyncStatusService::instance().updateMediaPending(static_cast<int>(m_queue.size()) + active);
=======
>>>>>>> REPLACE
```

### 3.7 `src/meta/MetadataManager.cpp`
```
<<<<<<< SEARCH
void MetadataManager::notifyUI(RefreshLevel level, const QString& path) {
    switch (level) {
        case RefreshLevel::CountsOnly:
            notifyCategoryCountChanged();
            break;
        case RefreshLevel::PathUpdate:
            if (!path.isEmpty()) {
                {
                    std::unique_lock<std::shared_mutex> lock(m_mutex);
                    m_pendingUiPaths.insert(path);
                }
                QMetaObject::invokeMethod(this, "triggerUiSignalTimer", Qt::QueuedConnection);
            }
            break;
        case RefreshLevel::FullRebuild:
            notifyFullUIRebuild();
            break;
        case RefreshLevel::CategoryOnly:
            if (m_isInternalOperating) return;
            {
                std::unique_lock<std::shared_mutex> lock(m_mutex);
                m_pendingUiPaths.insert("__RELOAD_CATEGORY_ONLY__");
            }
            QMetaObject::invokeMethod(this, "triggerUiSignalTimer", Qt::QueuedConnection);
            break;
    }
}
=======
void MetadataManager::notifyUI(RefreshLevel level, const QString& path) {
    switch (level) {
        case RefreshLevel::CountsOnly:
            break;
        case RefreshLevel::PathUpdate:
            if (!path.isEmpty()) {
                {
                    std::unique_lock<std::shared_mutex> lock(m_mutex);
                    m_pendingUiPaths.insert(path);
                }
                QMetaObject::invokeMethod(this, "triggerUiSignalTimer", Qt::QueuedConnection);
            }
            break;
        case RefreshLevel::FullRebuild:
            notifyFullUIRebuild();
            break;
        case RefreshLevel::CategoryOnly:
            break;
    }
}
>>>>>>> REPLACE
```

### 3.8 `src/ui/BatchCreateDialog.h`
```
<<<<<<< SEARCH
class BatchCreateDialog : public FramelessDialog {
    Q_OBJECT
public:
    explicit BatchCreateDialog(const QString& currentDirectory, bool isMemoryMode = false, QWidget* parent = nullptr);
    ~BatchCreateDialog() override = default;

    bool isFile() const;
    QString fileSuffix() const;
    QStringList renderAllNames() const;
    QString selectedLibraryPath() const;

private slots:
    void scheduleAutoSave();
    void doAutoSave();

private:
    void initContent();
    void onExecute();
    void onInsertRowAfter(CreateRuleRow* targetRow = nullptr);
    void applyTheme();
    void updateLibraryControlState();
    QString renderOne(int index, const std::vector<RenameRule>& rules) const;

    QString m_currentDir;
    bool m_isMemoryMode = false;
=======
class BatchCreateDialog : public FramelessDialog {
    Q_OBJECT
public:
    explicit BatchCreateDialog(const QString& currentDirectory, QWidget* parent = nullptr);
    ~BatchCreateDialog() override = default;

    bool isFile() const;
    QString fileSuffix() const;
    QStringList renderAllNames() const;

private slots:
    void scheduleAutoSave();
    void doAutoSave();

private:
    void initContent();
    void onExecute();
    void onInsertRowAfter(CreateRuleRow* targetRow = nullptr);
    void applyTheme();
    QString renderOne(int index, const std::vector<RenameRule>& rules) const;

    QString m_currentDir;
>>>>>>> REPLACE
```

### 3.9 `src/ui/BatchCreateDialog.cpp`
```
<<<<<<< SEARCH
BatchCreateDialog::BatchCreateDialog(const QString& currentDirectory, bool isMemoryMode, QWidget* parent)
    : FramelessDialog("批量创建 - QuarkMeta", parent), m_currentDir(currentDirectory), m_isMemoryMode(isMemoryMode) {
    resize(550, 420);
    initContent();
    applyTheme();

    // 扫描并填充 m_libraryCombo 数据
    if (m_libraryCombo) {
        for (const QFileInfo& drive : QDir::drives()) {
            QString letter = drive.absolutePath().left(1).toUpper();
            QString drivePath = QDir::toNativeSeparators(QString("%1:\\").arg(letter));
            if (QDir(drivePath).exists()) {
                m_libraryCombo->addItem(QString("磁盘 (%1:)").arg(letter), drivePath);
            }
        }
        QString lastLibPath = AppConfig::instance().getValue("BatchCreate/LastLibraryPath").toString();
        if (!lastLibPath.isEmpty()) {
            int idx = m_libraryCombo->findData(lastLibPath);
            if (idx != -1) m_libraryCombo->setCurrentIndex(idx);
        }
    }

    // 1. 初始化自动保存防抖定时器
    m_autoSaveTimer = new QTimer(this);

}
=======
BatchCreateDialog::BatchCreateDialog(const QString& currentDirectory, QWidget* parent)
    : FramelessDialog("批量创建 - QuarkMeta", parent), m_currentDir(currentDirectory) {
    resize(550, 420);
    initContent();
    applyTheme();

    // 1. 初始化自动保存防抖定时器
    m_autoSaveTimer = new QTimer(this);

}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps (编译命令与验证方法)

### 4.1 物理删除残存僵尸源文件命令
在终端执行以下命令彻底清除残存在磁盘上的历史僵尸文件：
```bash
rm -f src/util/AssetImporter.h src/util/AssetImporter.cpp
rm -f src/util/ImportHelper.h src/util/ImportHelper.cpp
rm -f src/ui/CategoryLockDialog.h src/ui/CategoryLockDialog.cpp
rm -f src/ui/CategoryLockWidget.h src/ui/CategoryLockWidget.cpp
rm -f src/ui/CategoryPanel.h src/ui/CategoryPanel.cpp
rm -f src/meta/AmMetaJson.h src/meta/AmMetaJson.cpp
rm -f src/ui/CategoryModel.h src/ui/CategoryModel.cpp
rm -f src/ui/CategoryFilterProxyModel.h
rm -f src/ui/CategoryDelegate.h src/ui/CategoryDelegate.cpp
rm -f src/meta/CategoryBindingManager.h src/meta/CategoryBindingManager.cpp
rm -f src/core/SyncStatusService.h src/core/SyncStatusService.cpp
```

### 4.2 构建与编译验证
在项目根目录下，使用 CMake 重新配置与构建：
```bash
cmake -B build -S .
cmake --build build --config Release
```

验证结果：
1. 项目可顺利生成 `QuarkMeta.exe` 目标文件，无任何符号丢失、缺少头文件或 `qt_metacall`/`metaObject` 链接错误。
2. 彻底消除了内存模式与旧分类系统的残存分支代码。
