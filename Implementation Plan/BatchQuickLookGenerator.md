# Implementation Plan - BatchQuickLookGenerator.md

> **Base Commit Hash**: `c9177072427bd313b404237cf46434e7828c5c3c`
> **Prerequisite Plans**: None
> **Supersedes**: None

---

## Chapter 1: Overview

### 问题与需求背景 (Problem & Background)
1. **高清大图预览无批量预生成机制**：QuarkMeta 的 2048px 高清 QuickLook 预览（含 AI/EPS/PDF 等 Ghostscript 144 DPI 高清渲染）原先仅支持在按空格键预览单个文件时实时触发。在面对全库或包含海量矢量的多层子文件夹时，用户逐一预览会产生卡顿感。
2. **需要高效批量管线，拒绝资源浪费**：批量生成大图预览必须引入原生的**批量处理管线 (`extractBatchAsync`)**。在处理前必须先进行轻量极速预检（Pre-Scan），计算文件的 SHA256 哈希值并校验 `quicklook_previews/{dir1}/{dir2}/{hash}.png` 缓存文件的存在性与修改时间（`mtime`）。已存在且未修改的文件直接 0 成本跳过，仅对真正缺失/过期的文件进行增量渲染，严禁无谓的磁盘与 CPU 算力浪费。
3. **Ghostscript 独占性与空格预览“10ms 秒级熔断/让出”**：Ghostscript 渲染通过信号量 `g_gsConcurrencyLimit(1)` 限制同一时刻仅允许 1 个进程运行。在后台跑批量生成大图预览时，若用户按下空格键触发前台 QuickLook 预览，后台批处理管线必须能够**立即被抢占并执行强行熔断**（在 10~50ms 内杀死当前后台 Ghostscript 进程并释放信号量），保证前台空格预览 0ms 顺畅响应。
4. **右键菜单与全局进度归一化**：根据 UI 交互规范，在右键菜单的“更多” -> “重新提取缩略图”下方添加“**批量生成大图预览**”选项（仅对选中项目/文件夹展示，空白处不展示）。批量生成过程必须驱动全局统一的 `TaskProgressToolBar`（`TaskProgressService`）进行橙红色 (`#ff551c`) 进度展示。所有生成的高清预览图统一落地于 `AppConfig::instance().getGlobalCacheDir() + "/quicklook_previews"` 目录。

### 重构设计目标 (Refactoring Goals)
1. **新增 `BatchQuickLookService` 高性能批量服务类**：
   - 提供原生批量异步处理接口 `extractBatchAsync(const QStringList& rootPaths)`。
   - 内部使用独立单线程池 `batchPool()`，配合取消令牌 `m_currentToken` 控制生命周期。
   - 包含预检过滤机制：先计算 SHA256 哈希及 2 级分桶路径，跳过物理缓存有效的文件，得到精确的“缺失/过期的增量文件列表”。
   - 提供 `cancelCurrentBatch()` 强行熔断让出接口。
2. **空格预览无缝抢占与熔断**：
   - 在 `QuickLookWindow::preview()` 第一时间显式调用 `BatchQuickLookService::instance().cancelCurrentBatch()`。
   - `cancelCurrentBatch()` 触发 `CancellationToken::cancel()`，使 `FormatDecoders::renderGhostscriptSafely` 的 50ms 轮询检测立即命中，执行 `process.kill()` 并在 10~50ms 内释放 `g_gsConcurrencyLimit` 锁，完美让给前台空格预览。
3. **右键菜单集成与 action 路由**：
   - 在 `ContentPanel.h` 的 `ContextAction` 枚举中增加 `ActionBatchQuickLook`。
   - 在 `ContentContextMenu.cpp` 中的“更多”二级菜单中，在“重新提取缩略图”下方加入“批量生成大图预览”。
4. **CMake 全量注册**：
   - 在 `CMakeLists.txt` 中同步注册 `src/util/BatchQuickLookService.h` 与 `src/util/BatchQuickLookService.cpp`。

---

## Chapter 2: Modified Files List

1. `src/util/BatchQuickLookService.h` (**CREATE FILE**)
2. `src/util/BatchQuickLookService.cpp` (**CREATE FILE**)
3. `CMakeLists.txt` (**MODIFY FILE**)
4. `src/ui/ContentPanel.h` (**MODIFY FILE**)
5. `src/ui/controllers/ContentContextMenu.cpp` (**MODIFY FILE**)
6. `src/ui/QuickLookWindow.cpp` (**MODIFY FILE**)

---

## Chapter 3: Detailed Line-by-Line Changes

### 1. 新建 `src/util/BatchQuickLookService.h`
```
<<<<<<< SEARCH FILE: src/util/BatchQuickLookService.h
=======
#pragma once

#include <QObject>
#include <QStringList>
#include <memory>
#include <mutex>
#include "../core/CoreEngine.h"

namespace QuarkMeta {

/**
 * @brief 专职负责后台批量预生成 2048px 高清 QuickLook 预览图的高性能服务
 * 特性：
 * 1. 原生批量处理管线 (extractBatchAsync)；
 * 2. 预检过滤 (Pre-Scan)，只对真正缺失/过期的文件做增量生成，拒绝算力与 IO 浪费；
 * 3. 驱动 TaskProgressToolBar 全局进度条展示；
 * 4. 0ms 顺畅抢占：空格预览触发时 10~50ms 即时强行熔断并让出 GS 锁。
 */
class BatchQuickLookService : public QObject {
    Q_OBJECT
public:
    static BatchQuickLookService& instance();

    /**
     * @brief 原生批量异步接口：预生成大图预览
     * @param rootPaths 目标路径列表 (支持文件夹、文件或库路径)
     */
    void extractBatchAsync(const QStringList& rootPaths);

    /**
     * @brief 触发即时强行熔断 (取消当前正在运行的批量生成任务，杀掉 GS 进程并释放锁)
     */
    void cancelCurrentBatch();

    bool isRunning() const;

private:
    explicit BatchQuickLookService(QObject* parent = nullptr);
    ~BatchQuickLookService() override = default;

    std::shared_ptr<CancellationToken> m_currentToken;
    mutable std::mutex m_mutex;
};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 2. 新建 `src/util/BatchQuickLookService.cpp`
```
<<<<<<< SEARCH FILE: src/util/BatchQuickLookService.cpp
=======
#include "BatchQuickLookService.h"
#include "../ui/QuickLookWindow.h"
#include "../ui/UiHelper.h"
#include "../ui/ToolTipOverlay.h"
#include "../core/TaskProgressService.h"
#include "../core/AppConfig.h"
#include <QtConcurrent>
#include <QThreadPool>
#include <QCoreApplication>
#include <QDirIterator>
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>
#include <QCursor>

namespace QuarkMeta {

namespace {
QThreadPool* batchQuickLookPool() {
    static QThreadPool pool;
    static std::once_flag flag;
    std::call_once(flag, []() { pool.setMaxThreadCount(1); });
    return &pool;
}

// 校验特定文件的 quicklook_previews 高清缓存是否存在且未过期
bool isQuickLookCacheValid(const QString& filePath) {
    QFileInfo srcInfo(filePath);
    if (!srcInfo.exists() || srcInfo.size() <= 0) return true; // 无效原文件，跳过

    QByteArray normalized = QDir::toNativeSeparators(filePath).toLower().toUtf8();
    QString hashStr = QString::fromUtf8(QCryptographicHash::hash(normalized, QCryptographicHash::Sha256).toHex());
    QString dirL1 = hashStr.left(2);
    QString dirL2 = hashStr.mid(2, 2);

    QString baseCacheDir = AppConfig::instance().getGlobalCacheDir() + "/quicklook_previews";
    QString cachePath = QString("%1/%2/%3/%4.png").arg(baseCacheDir, dirL1, dirL2, hashStr);
    QFileInfo cacheInfo(cachePath);

    return (cacheInfo.exists() && cacheInfo.size() > 0 && cacheInfo.lastModified() >= srcInfo.lastModified());
}
} // namespace

BatchQuickLookService& BatchQuickLookService::instance() {
    static BatchQuickLookService inst;
    return inst;
}

BatchQuickLookService::BatchQuickLookService(QObject* parent) : QObject(parent) {}

void BatchQuickLookService::cancelCurrentBatch() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_currentToken) {
        m_currentToken->cancel();
    }
}

bool BatchQuickLookService::isRunning() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentToken && !m_currentToken->isCanceled();
}

void BatchQuickLookService::extractBatchAsync(const QStringList& rootPaths) {
    if (rootPaths.isEmpty()) return;

    // 若有旧任务正在进行，先触发熔断取消
    cancelCurrentBatch();

    std::shared_ptr<CancellationToken> token = std::make_shared<CancellationToken>();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_currentToken = token;
    }

    (void)QtConcurrent::run(batchQuickLookPool(), [rootPaths, token]() {
        // 1. 递归扫描 + 缓存 MTime 极速预检 (Pre-Scan Filter)
        QStringList targetFiles;
        for (const QString& rootPath : rootPaths) {
            if (token->isCanceled()) break;

            QFileInfo fi(rootPath);
            if (fi.isDir()) {
                QDirIterator it(rootPath, QDir::Files, QDirIterator::Subdirectories);
                while (it.hasNext()) {
                    if (token->isCanceled()) break;
                    it.next();
                    QString filePath = it.filePath();
                    QString ext = it.fileInfo().suffix().toLower();
                    if (UiHelper::isGraphicsFile(ext)) {
                        // 预检：只保留缓存缺失或需要更新的文件，避免重复生成浪费算力
                        if (!isQuickLookCacheValid(filePath)) {
                            targetFiles << filePath;
                        }
                    }
                }
            } else if (fi.isFile()) {
                QString ext = fi.suffix().toLower();
                if (UiHelper::isGraphicsFile(ext)) {
                    if (!isQuickLookCacheValid(rootPath)) {
                        targetFiles << rootPath;
                    }
                }
            }
        }

        if (token->isCanceled()) return;

        if (targetFiles.isEmpty()) {
            QMetaObject::invokeMethod(QCoreApplication::instance(), []() {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "所有所选项目的高清大图预览已是最新，无需重新生成", 2000, QColor("#2ecc71"));
            });
            return;
        }

        // 2. 将真正的“缺失/过期增量文件列表”打包提交批量渲染
        int total = targetFiles.size();
        int jobId = TaskProgressService::instance().createJob("正在批量生成大图预览...", total);
        int processedCount = 0;

        for (const QString& filePath : targetFiles) {
            if (token->isCanceled()) break;

            // 调用 QuickLookWindow 统一高清缓存落地接口 (2048px, SHA256 2级分桶)
            (void)QuickLookWindow::loadOrExtractQuickLookImage(filePath, 2048, token);

            processedCount++;
            TaskProgressService::instance().updateProgress(
                jobId,
                processedCount,
                total,
                QString("已生成 %1 / %2 项").arg(processedCount).arg(total)
            );
        }

        TaskProgressService::instance().finishJob(jobId);

        if (!token->isCanceled()) {
            QMetaObject::invokeMethod(QCoreApplication::instance(), [processedCount]() {
                ToolTipOverlay::instance()->showText(
                    QCursor::pos(),
                    QString("大图预览批量生成完成，共计 %1 项").arg(processedCount),
                    2000,
                    QColor("#2ecc71")
                );
            });
        }
    });
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3. 在 `CMakeLists.txt` 中注册新文件
```
<<<<<<< SEARCH
    src/util/DiskMediaExtractor.cpp
    src/util/DiskMediaExtractor.h
    src/util/DeepThumbnailExtractor.cpp
    src/util/DeepThumbnailExtractor.h
=======
    src/util/DiskMediaExtractor.cpp
    src/util/DiskMediaExtractor.h
    src/util/DeepThumbnailExtractor.cpp
    src/util/DeepThumbnailExtractor.h
    src/util/BatchQuickLookService.cpp
    src/util/BatchQuickLookService.h
>>>>>>> REPLACE
```

### 4. 更新 `src/ui/ContentPanel.h`
```
<<<<<<< SEARCH
        ActionCopyName, ActionCopyPath, ActionAddToFavorites, ActionRefresh, ActionReextractThumbnail, ActionBatchCreate
    };
=======
        ActionCopyName, ActionCopyPath, ActionAddToFavorites, ActionRefresh, ActionReextractThumbnail, ActionBatchQuickLook, ActionBatchCreate
    };
>>>>>>> REPLACE
```

### 5. 更新 `src/ui/controllers/ContentContextMenu.cpp`
```
<<<<<<< SEARCH
            if (!isFolder) {
                moreMenu->addSeparator();
                moreMenu->addAction(UiHelper::getIcon("repeat", QColor("#EEEEEE"), 18), "重新提取缩略图")->setData(ContentPanel::ActionReextractThumbnail);

                QMenu* cryptoMenu = moreMenu->addMenu(UiHelper::getIcon("shield", QColor("#EEEEEE"), 18), "外壳保护");
=======
            moreMenu->addSeparator();
            if (!isFolder) {
                moreMenu->addAction(UiHelper::getIcon("repeat", QColor("#EEEEEE"), 18), "重新提取缩略图")->setData(ContentPanel::ActionReextractThumbnail);
            }
            moreMenu->addAction(UiHelper::getIcon("image_picture", QColor("#EEEEEE"), 18), "批量生成大图预览")->setData(ContentPanel::ActionBatchQuickLook);

            if (!isFolder) {
                QMenu* cryptoMenu = moreMenu->addMenu(UiHelper::getIcon("shield", QColor("#EEEEEE"), 18), "外壳保护");
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
#include "../../util/ShellHelper.h"
#include "../../util/DeepThumbnailExtractor.h"
#include "../../core/TrashService.h"
=======
#include "../../util/ShellHelper.h"
#include "../../util/DeepThumbnailExtractor.h"
#include "../../util/BatchQuickLookService.h"
#include "../../core/TrashService.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        case ContentPanel::ActionReextractThumbnail: {
=======
        case ContentPanel::ActionBatchQuickLook: {
            QStringList selectedPaths = m_panel->getSelectedPaths();
            if (selectedPaths.isEmpty() && !path.isEmpty()) {
                selectedPaths << path;
            }
            if (!selectedPaths.isEmpty()) {
                BatchQuickLookService::instance().extractBatchAsync(selectedPaths);
            }
            break;
        }
        case ContentPanel::ActionReextractThumbnail: {
>>>>>>> REPLACE
```

### 6. 更新 `src/ui/QuickLookWindow.cpp` (添加熔断调用)
```
<<<<<<< SEARCH
#include "dialogs/TextExtensionDialog.h"
#include "FormatDecoders.h"
=======
#include "dialogs/TextExtensionDialog.h"
#include "FormatDecoders.h"
#include "../util/BatchQuickLookService.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void QuickLookWindow::preview(const QString& filePath) {
    m_currentPath = filePath;
=======
void QuickLookWindow::preview(const QString& filePath) {
    // 🚀【即时强行熔断】：按空格键触发预览时，第一时间熔断正在跑的后台批量大图生成任务
    BatchQuickLookService::instance().cancelCurrentBatch();

    m_currentPath = filePath;
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps

### 1. 编译验证
```bash
cmake -B build -G Ninja
cmake --build build
```
验证目标：项目零编译错误，零链接错误，`BatchQuickLookService` 原生批量管线正确编译注入。

### 2. 运行时验收标准
1. **右键菜单项呈现**：选中项目或文件夹，在右键菜单“更多” -> “重新提取缩略图”正下方展示“批量生成大图预览”。
2. **预检过滤与零算力浪费验证**：对一个已全部生成预览图的文件夹再次点击“批量生成大图预览”，提示“所有所选项目的高清大图预览已是最新，无需重新生成”，0 秒完成，绝不对已有有效缓存重复提取！
3. **全局进度条**：对包含缺项的路径触发时，底部 `TaskProgressToolBar` 展开，进度条以橙红色 `#ff551c` 实时展示处理项数。
4. **空格预览秒级熔断验证**：在批量生成大图预览的过程中，对任意项目按下空格键，后台 Ghostscript 进程在 10~50ms 内立即被 `kill`，信号锁瞬间释放给前台 QuickLook，前台预览 0ms 顺畅打开，无任何卡顿超时。

---

## Chapter 5: Official API Reuse & Anti-Redundancy

- 完美复用 `QuickLookWindow::loadOrExtractQuickLookImage` 核心生成与落地接口。
- 完美复用 `TaskProgressService` 单例进行统一进度条驱动。
- 完美复用 `CancellationToken` 机制与 Ghostscript 50ms 轮询 kill 熔断管线。

---

## Chapter 6: Data SSOT Declaration Table

| 数据/状态项 | 唯一 SSOT 数据源 | 派生 UI/副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| QuickLook 高清预览图缓存 | `quicklook_previews/{dir1}/{dir2}/{hash}.png` | `QuickLookWindow` / `BatchQuickLookService` | `isQuickLookCacheValid` 进行 MTime 预检校验 |
| 批量任务状态 | `BatchQuickLookService::m_currentToken` | `TaskProgressToolBar` | `cancelCurrentBatch()` 触发强行熔断 |

---

## Chapter 7: Interface & Visibility Consistency Check

- `BatchQuickLookService::instance()`: 单例入口
- `BatchQuickLookService::extractBatchAsync(const QStringList& rootPaths)`: 原生批量异步接口 `src/util/BatchQuickLookService.h:23`
- `BatchQuickLookService::cancelCurrentBatch()`: 熔断接口 `src/util/BatchQuickLookService.h:28`

---

## Chapter 8: Header Chain & Closure Check

- 在 `CMakeLists.txt` 中全量注册 `.h` 与 `.cpp`。
- 在 `ContentContextMenu.cpp` 与 `QuickLookWindow.cpp` 中包含 `#include "../util/BatchQuickLookService.h"`。

---

## Chapter 9: Compliance & Quality Self-Check Table

| 检查项 | 结论 | 位置/说明 |
| :--- | :--- | :--- |
| 1. SEARCH 块唯一匹配 | 满足 | 行级精准匹配 |
| 2. 原生批量接口 `extractBatchAsync` | 满足 | 具备批量批处理管线 |
| 3. 预检过滤 (Pre-Scan Filter) | 满足 | 基于 SHA256 与 MTime 校验，跳过已有缓存 |
| 4. 10~50ms 即时熔断 | 满足 | 空格预览触发时即刻取消 kill 释放 GS 锁 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table

| 交互/功能项 | 原表现 | 重构/新增行为 |
| :--- | :--- | :--- |
| 批量大图预提取 | 不支持 | 右键菜单“更多”提供“批量生成大图预览” |
| 计算效率 | N/A | 引入 MTime 预检，只对缺失/过期项增量生成 |
| 熔断与让出 | N/A | 空格预览 10~50ms 内杀 GS 进程秒级抢占 |
