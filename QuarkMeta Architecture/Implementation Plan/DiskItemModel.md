# 第 1 关：起草前核查（Pre-Draft Evidence）

1. **已读文件清单**：
   - `src/ui/models/DiskItemModel.h`：已读取物理头文件，核实类声明与成员变量 `m_iconCache`、`m_aspectRatios`、`m_requestedPaths` 定义；
   - `src/ui/models/DiskItemModel.cpp`：已读取物理源文件，核实 `loadBatchAsync` 异步加载回调实现与数据竞态发生的具体位置（第 640 ~ 675 行）。

2. **签名原文摘录**：
   - `void DiskItemModel::loadThumbnailsForRows(const QList<int>& rows);` (声明于 `DiskItemModel.h`)
   - `void ThumbnailPipelineService::loadBatchAsync(const QStringList& paths, int targetSize, std::function<void(const QString&, const QPixmap&)> callback);` (声明于 `ThumbnailPipelineService.h`)

3. **既有 API 检索记录**：
   - 检索关键词：`loadBatchAsync` / `m_aspectRatios` / `m_iconCache`
   - 命中结果：确认异步缩略图加载管线统一通过 `ThumbnailPipelineService::instance().loadBatchAsync` 处理，但 `DiskItemModel.cpp` 在接收到子线程解码完成回调后，在回调最顶层（子线程环境）直接操作了主线程模型成员 `m_iconCache.insert`、`m_aspectRatios[...]` 和 `m_requestedPaths.remove`，引发致命跨线程数据竞态。

4. **Memories.md 相关条款**：
   - 章节 3.1 & 4.2：线程安全边界与 UI 响应防抖规范，后台提取管道在涉及 UI 主线程模型数据/控件结构更新时，必须通过 `QMetaObject::invokeMethod` 归一化派发至主线程执行，严禁跨线程并发读写 Qt 容器（如 `QMap` / `QCache`）。

---

# Implementation Plan - DiskItemModel.md

## 1. Overview（概述与解决的问题）

### 问题背景
在筛选器（FilterPanel）中勾选或取消勾选筛选条目（如评级星级、文件类型、颜色标记等）时，应用程序频繁触发 UI 假死，Windows 弹窗警告 **“应用程序没有响应 (Application Not Responding)”**。

### 根因分析
通过物理追踪代码与多线程数据流定位，发现假死的根本原因在于 `DiskItemModel::loadBatchAsync` 回调函数中的**严重跨线程数据竞态 (Cross-Thread Data Race)**：
1. 当用户切换筛选条件时，视图更新显隐卡片并触发 `refreshVisibleThumbnails()`，进而调用 `DiskItemModel::loadBatchAsync` 向 `ThumbnailPipelineService` 提交批量异步缩略图加载请求。
2. 后台工作线程（Worker Thread）解码完成后，执行传入的回调 Lambda。
3. 在原实现中（`DiskItemModel.cpp` 第 643 ~ 651 行），**子线程在调用 `QMetaObject::invokeMethod` 跨线程投递之前，直接在子线程上下文中执行了**：
   ```cpp
   weakThis->m_requestedPaths.remove(path); // 修改 QSet
   weakThis->m_iconCache.insert(path, new QIcon(icon)); // 修改 QCache
   weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = ar; // 修改 QMap
   ```
4. `m_iconCache`（`QCache`）、`m_aspectRatios`（`QMap`）与 `m_requestedPaths`（`QSet`）均为 `DiskItemModel` 在 UI 主线程上持有的成员变量。当 UI 主线程正在执行 `JustifiedView::doLayout()` 或 `paintEvent()` 绘制遍历 `m_aspectRatios` / `m_iconCache` 时，后台子线程同时对 `QMap` 进行插入/删除节点操作。
5. Qt 的 `QMap` 非线程安全。并发读写直接损坏 `QMapNode` 的左右子树指针链表，产生指针环路。
6. 主线程在随后的 `m_aspectRatios.value(...)` 节点查找中陷入**死循环（Infinite Loop）**，导致主 UI 线程 CPU 100% 占用并彻底假死，触发 Windows 系统“应用程序没有响应”强关弹窗。

### 解决策略
将 `m_requestedPaths.remove`、`m_iconCache.insert` 和 `m_aspectRatios[...]` 的赋值逻辑完整包裹并移入 `QMetaObject::invokeMethod(weakThis.data(), ..., Qt::QueuedConnection)` 的主线程闭包函数内部。确保 `DiskItemModel` 所有容器变量的读写彻底 100% 隔离并限制在 UI 主线程执行，消除跨线程数据竞态与 `QMap` 指针损坏。

---

## 2. Modified Files List（影响文件清单）

- `src/ui/models/DiskItemModel.cpp`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 修改 `src/ui/models/DiskItemModel.cpp`

```
<<<<<<< SEARCH
    QPointer<DiskItemModel> weakThis(this);
    ThumbnailPipelineService::instance().loadBatchAsync(pathsToLoad, DiskMediaExtractor::kThumbSize, [weakThis, thisGen](const QString& path, const QPixmap& pixmap) {
        if (!weakThis) return;

        weakThis->m_requestedPaths.remove(path);
        if (weakThis->currentGeneration() != thisGen) return;

        if (!pixmap.isNull()) {
            QIcon icon(pixmap);
            weakThis->m_iconCache.insert(path, new QIcon(icon));
            double ar = (double)pixmap.width() / pixmap.height();
            weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = ar;
        }

        // 注意：下面的状态更新必须在 isNull 判断之外，空图（提取失败）才能走到 Failed 分支
        QMetaObject::invokeMethod(weakThis, [weakThis, path, pixmap]() {
            if (!weakThis) return;
            weakThis->m_requestedPaths.remove(path);
            auto it = weakThis->m_pathToIndex.find(path);
            if (it != weakThis->m_pathToIndex.end()) {
                int currentIdx = it->second;
                if (currentIdx >= 0 && currentIdx < static_cast<int>(weakThis->m_allRecords.size())) {
                    if (weakThis->m_allRecords[currentIdx].path == path) {
                        if (!pixmap.isNull()) {
                            weakThis->m_allRecords[currentIdx].thumbnailState = ItemRecord::ThumbnailState::Ready;
                            weakThis->m_pendingThumbRows.insert(currentIdx);
                            if (weakThis->m_thumbBatchTimer && !weakThis->m_thumbBatchTimer->isActive()) {
                                weakThis->m_thumbBatchTimer->start();
                            }
                            emit weakThis->thumbnailLoaded(currentIdx);
                        } else {
                            weakThis->m_allRecords[currentIdx].thumbnailState = ItemRecord::ThumbnailState::Failed;
                            emit weakThis->dataChanged(
                                weakThis->index(currentIdx, 0),
                                weakThis->index(currentIdx, weakThis->columnCount() - 1),
                                {HasThumbnailRole}
                            );
                        }
                    }
                }
            }
        }, Qt::QueuedConnection);
    });
=======
    QPointer<DiskItemModel> weakThis(this);
    ThumbnailPipelineService::instance().loadBatchAsync(pathsToLoad, DiskMediaExtractor::kThumbSize, [weakThis, thisGen](const QString& path, const QPixmap& pixmap) {
        if (!weakThis) return;

        // 🚀【主线程归一化/跨线程锁隔离】：所有的 m_requestedPaths/m_iconCache/m_aspectRatios 修改必须统一置于主线程回调内，
        // 彻底杜绝后台工作线程并发写入 QMap/QCache 导致主线程遍历 QMapNode 时死循环引发的 UI 假死与“应用程序没有响应”！
        QMetaObject::invokeMethod(weakThis.data(), [weakThis, path, pixmap, thisGen]() {
            if (!weakThis || weakThis->currentGeneration() != thisGen) return;

            weakThis->m_requestedPaths.remove(path);

            if (!pixmap.isNull()) {
                QIcon icon(pixmap);
                weakThis->m_iconCache.insert(path, new QIcon(icon));
                double ar = (double)pixmap.width() / pixmap.height();
                weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = ar;
            }

            auto it = weakThis->m_pathToIndex.find(path);
            if (it != weakThis->m_pathToIndex.end()) {
                int currentIdx = it->second;
                if (currentIdx >= 0 && currentIdx < static_cast<int>(weakThis->m_allRecords.size())) {
                    if (weakThis->m_allRecords[currentIdx].path == path) {
                        if (!pixmap.isNull()) {
                            weakThis->m_allRecords[currentIdx].thumbnailState = ItemRecord::ThumbnailState::Ready;
                            weakThis->m_pendingThumbRows.insert(currentIdx);
                            if (weakThis->m_thumbBatchTimer && !weakThis->m_thumbBatchTimer->isActive()) {
                                weakThis->m_thumbBatchTimer->start();
                            }
                            emit weakThis->thumbnailLoaded(currentIdx);
                        } else {
                            weakThis->m_allRecords[currentIdx].thumbnailState = ItemRecord::ThumbnailState::Failed;
                            emit weakThis->dataChanged(
                                weakThis->index(currentIdx, 0),
                                weakThis->index(currentIdx, weakThis->columnCount() - 1),
                                {HasThumbnailRole}
                            );
                        }
                    }
                }
            }
        }, Qt::QueuedConnection);
    });
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译测试**：
   在 sandbox 或环境终端中执行 CMake 构建命令：
   ```bash
   cmake --build build --config Debug
   ```
2. **功能验证**：
   - 启动 QuarkMeta 应用程序，进入包含较多文件/图片的目录（如 `src`）；
   - 在右侧“筛选”面板中反复勾选与取消勾选各个筛选条件（评级星级、文件类型、颜色标记等）；
   - 验证缩略图平滑加载，主 UI 界面保持流畅响应，不再出现无响应窗口与卡死现象。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（SSOT 入口复用与防另起炉灶自查）

- 本次修改严格复用了既有的 `ThumbnailPipelineService::instance().loadBatchAsync` 管线与 `QMetaObject::invokeMethod` 机制，没有新增任何冗余或分散的局部多线程管道。
- 物理消除了在子线程直接篡改主线程状态的 Hack 代码，收敛至标准 Qt 主线程事件队列派发模型。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 外部/内部函数调用 | `.h` 文件中物理精确签名 | 校验状态 |
|---|---|---|
| `ThumbnailPipelineService::loadBatchAsync` | `void loadBatchAsync(const QStringList& paths, int targetSize, std::function<void(const QString&, const QPixmap&)> callback);` | 100% 一致 |
| `DiskItemModel::currentGeneration` | `uint64_t currentGeneration() const;` | 100% 一致 |
| `QMetaObject::invokeMethod` | `static bool invokeMethod(QObject *obj, Functor functor, Qt::ConnectionType type = Qt::AutoConnection);` | 100% 一致 |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

- `DiskItemModel.cpp` 中已包含：
  - `#include "DiskItemModel.h"`
  - `#include "../util/ThumbnailPipelineService.h"`
  - `#include <QMetaObject>`
- 本次修改未添加或删除任何 `#include` 行，无头文件包含断裂风险。

---

## 8. Compliance Self-Check Table（合规核对表）

| 检查项 | 来源条款 | 状态/位置 |
|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 (`DiskItemModel.md`) |
| 方案位于物理隔离目录 | 3.1-1 | 满足 (`QuarkMeta Architecture/Implementation Plan/`) |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 (对应 `DiskItemModel.cpp` 640-675 行) |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册 | 3.3 | 不涉及 (仅修改已有 .cpp 内部逻辑) |
| 第 1~9 章齐全 | 3.4 | 满足 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 (见第 6 章) |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 不涉及 (无 include 变动) |
| 已复用既有 SSOT 入口，无另起炉灶 | 2.4 | 满足 (见第 5 章) |
| 无内联 setStyleSheet，样式走 style.qss | 4.1 | 不涉及 |
| 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 |
| 批量操作为一次聚合广播 | 4.3 | 满足 |
| 迁移的所有数值原样照抄 | 4.4 | 不涉及 |
| 未改动用户既有设计理念与交互方式 | 1.2 / Memories.md | 满足 |
| 未触及方案范围外的文件 | 5.3 | 满足 (仅触及 `DiskItemModel.cpp`) |

---

## 9. Zero-Value-Alteration Check（数值零篡改核对）

- 本方案不涉及任何 UI 样式、颜色、尺寸或交互参数迁移，写“不涉及”。
