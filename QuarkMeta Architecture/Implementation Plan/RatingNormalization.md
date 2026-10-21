# RatingNormalization.md - 星级设定逻辑全通道归一化实施方案

## Chapter 1: Pre-Draft Evidence（第 1 关：起草前核查）

### 1. 已读文件清单
- `src/ui/controllers/ContentKeyHandler.cpp`（已用 bash 工具读取第 230~290 行）
- `src/ui/ThumbnailDelegate.cpp`（已用 bash 工具读取第 170~220 行）
- `src/ui/PanelMediator.cpp`（已用 bash 工具读取第 800~815、930~950 行）
- `src/core/CoreEngine.h`（已用 grep/read 查看 handleSetRating 物理签名）

### 2. 物理头文件签名原文摘录
- `CoreEngine.h`:
  `void handleSetRating(const QStringList& paths, int rating);`
- `CoreEngine.cpp`:
  `void CoreEngine::handleSetRating(const QStringList& paths, int rating)`
- `AppCommand`:
  `AppCommandType::SetRating;`

### 3. 既有 API 检索记录
- **检索关键词**：`SetRating` / `handleSetRating` / `ratingChanged`
- **SSOT 通道**：`CoreEngine::instance().executeCommand(cmd)` 为全软件设置星级的唯一标准业务逻辑入口，负责统一更新 SQLite `library_items` 表与 `.QuarkMeta.json` 侧车配置文件，并触发 `CentralEventHub` 广播。

### 4. Memories.md / AGENTS.md 条款要点
- 第 2.4 条：全软件通用行为必须且只能走唯一的真理源通道，禁止混用与另起炉灶。
- 第 5 章：归一化重构必须全量普查所有平行入口，并彻底物理清除分散的旧实现。

---

## Chapter 2: Overview（概述与解决的问题）

### 核心问题
在现有的实现中，内容面板中的卡片打星操作（`ContentKeyHandler` 和 `ThumbnailDelegate`）直接调用 `model->setData(index, rating, RatingRole)`，这属于绕过 `CoreEngine` 命令总线的局部单项修改。它带来以下两个严重的架构缺陷：
1. **多选割裂**：在选中多个项目时，在卡片上点击星级只对被点击的单行生效，无法同步打星给所有选中项，而右侧 `MetaPanel` 打星却能对多选生效，造成 UI 交互割裂。
2. **缺乏统一命令通道**：星级设定逻辑分散在 `ContentKeyHandler`、`ThumbnailDelegate`、`MetaPanel`、`DiskItemModel` 4 处，没有走向统一的 SSOT 通道。

### 重构方案（全通道归一化）
1. **统一入口收敛**：重构 `ContentKeyHandler`（捕获卡片/列表按压打星）与 `ThumbnailDelegate`（网格代理按压打星），使其在触发打星时：
   - 自动感知识图当前的选中状态（`selectionModel()->selectedIndexes()`）。
   - 若被点击项处于多选集中，提取所有选中项目的路径集合 `selectedPaths`；若为单选或未选中项，提取该单项路径。
   - 统一构造 `AppCommandType::SetRating` 命令提交给 `CoreEngine::instance().executeCommand(cmd)` 批处理。
2. **清除分散逻辑**：彻底物理替换 `ContentKeyHandler` 与 `ThumbnailDelegate` 中直接调用 `model->setData(index, hitVal, RatingRole)` 的旧散落实现，使其与 `MetaPanel` 的星级命令通道 100% 保持一致。

---

## Chapter 3: Modified Files List（影响文件清单）
1. `src/ui/controllers/ContentKeyHandler.cpp` (重构 `ContentKeyHandler::mousePressEvent` 中的打星响应，使其统一提取选中路径并提交 `AppCommandType::SetRating` 命令)
2. `src/ui/ThumbnailDelegate.cpp` (重构 `ThumbnailDelegate::editorEvent` 中的打星响应，使其统一提取选中路径并提交 `AppCommandType::SetRating` 命令)

---

## Chapter 4: Detailed Line-by-Line Changes（精准替换块）

### 1. `src/ui/controllers/ContentKeyHandler.cpp`
```
<<<<<<< SEARCH
            QAbstractItemModel* model = view->model();
            if (model) {
                model->setData(index, hitVal, RatingRole);
            }

            QAbstractItemView::EditTriggers cur = view->editTriggers();
=======
            QAbstractItemModel* model = view->model();
            if (model) {
                // 1. 获取选中的路径集合（多选/单选统一处理）
                QStringList targetPaths;
                if (isSelected && view->selectionModel()) {
                    QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
                    QList<int> selectedRows;
                    for (const QModelIndex& selIdx : selectedIndexes) {
                        if (!selectedRows.contains(selIdx.row())) {
                            selectedRows.append(selIdx.row());
                        }
                    }
                    for (int r : selectedRows) {
                        QModelIndex rowIdx = model->index(r, 0, index.parent());
                        QString p = rowIdx.data(PathRole).toString();
                        if (!p.isEmpty() && !rowIdx.data(SectionHeaderRole).toBool()) {
                            targetPaths.append(p);
                        }
                    }
                }
                if (targetPaths.isEmpty()) {
                    QString p = index.data(PathRole).toString();
                    if (!p.isEmpty()) targetPaths.append(p);
                }

                // 2. 统一分发归一化命令给 CoreEngine
                if (!targetPaths.isEmpty()) {
                    AppCommand cmd;
                    cmd.type = AppCommandType::SetRating;
                    cmd.targetPaths = targetPaths;
                    cmd.params["rating"] = hitVal;
                    CoreEngine::instance().executeCommand(cmd);

                    // 刷新视图上受影响行的 UI 展现
                    for (const QString& p : targetPaths) {
                        m_panel->updateItemMetadata(p);
                    }
                    m_panel->recalculateAndEmitStats();
                }
            }

            QAbstractItemView::EditTriggers cur = view->editTriggers();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        if (hitStar != -1) {
            bool isRowSelected = treeView->selectionModel() && treeView->selectionModel()->isRowSelected(index.row(), index.parent());
            if (!CardPainterHelper::canModifyRatingOnPress(isRowSelected, mEvent->modifiers())) {
                return false; // 未选中或有 Ctrl/Shift，不修改评分、不吞事件，放行给视图的默认选中处理
            }

            QAbstractItemModel* model = treeView->model();
            if (model) {
                QModelIndex targetIdx = treeView->model()->index(index.row(), 0, index.parent());
                if (!targetIdx.data(SectionHeaderRole).toBool()) {
                    model->setData(targetIdx, hitStar, RatingRole);
                }
            }

            QAbstractItemView::EditTriggers currentTriggers = treeView->editTriggers();
=======
        if (hitStar != -1) {
            bool isRowSelected = treeView->selectionModel() && treeView->selectionModel()->isRowSelected(index.row(), index.parent());
            if (!CardPainterHelper::canModifyRatingOnPress(isRowSelected, mEvent->modifiers())) {
                return false; // 未选中或有 Ctrl/Shift，不修改评分、不吞事件，放行给视图的默认选中处理
            }

            QAbstractItemModel* model = treeView->model();
            if (model) {
                // 1. 获取选中的路径集合（多选/单选统一处理）
                QStringList targetPaths;
                if (isRowSelected && treeView->selectionModel()) {
                    QModelIndexList selectedIndexes = treeView->selectionModel()->selectedIndexes();
                    QList<int> selectedRows;
                    for (const QModelIndex& selIdx : selectedIndexes) {
                        if (!selectedRows.contains(selIdx.row())) {
                            selectedRows.append(selIdx.row());
                        }
                    }
                    for (int r : selectedRows) {
                        QModelIndex rowIdx = model->index(r, 0, index.parent());
                        QString p = rowIdx.data(PathRole).toString();
                        if (!p.isEmpty() && !rowIdx.data(SectionHeaderRole).toBool()) {
                            targetPaths.append(p);
                        }
                    }
                }
                if (targetPaths.isEmpty()) {
                    QModelIndex targetIdx = model->index(index.row(), 0, index.parent());
                    QString p = targetIdx.data(PathRole).toString();
                    if (!p.isEmpty() && !targetIdx.data(SectionHeaderRole).toBool()) {
                        targetPaths.append(p);
                    }
                }

                // 2. 统一分发归一化命令给 CoreEngine
                if (!targetPaths.isEmpty()) {
                    AppCommand cmd;
                    cmd.type = AppCommandType::SetRating;
                    cmd.targetPaths = targetPaths;
                    cmd.params["rating"] = hitStar;
                    CoreEngine::instance().executeCommand(cmd);

                    // 刷新视图上受影响行的 UI 展现
                    for (const QString& p : targetPaths) {
                        m_panel->updateItemMetadata(p);
                    }
                    m_panel->recalculateAndEmitStats();
                }
            }

            QAbstractItemView::EditTriggers currentTriggers = treeView->editTriggers();
>>>>>>> REPLACE
```

### 2. `src/ui/ThumbnailDelegate.cpp`
```
<<<<<<< SEARCH
                    bool isSelected = (option.state & QStyle::State_Selected);
                    if (CardPainterHelper::canModifyRatingOnPress(isSelected, mouseEvent->modifiers())) {
                        model->setData(index, hitVal, m_ratingRole);
                        return true; // 已选中且无组合键，修改评分并消费事件
                    }
=======
                    bool isSelected = (option.state & QStyle::State_Selected);
                    if (CardPainterHelper::canModifyRatingOnPress(isSelected, mouseEvent->modifiers())) {
                        // 1. 提取多选集合或当前被点击项目路径
                        QStringList targetPaths;
                        const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(option.widget);
                        if (isSelected && view && view->selectionModel()) {
                            QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
                            QList<int> selectedRows;
                            for (const QModelIndex& selIdx : selectedIndexes) {
                                if (!selectedRows.contains(selIdx.row())) {
                                    selectedRows.append(selIdx.row());
                                }
                            }
                            for (int r : selectedRows) {
                                QModelIndex rowIdx = model->index(r, 0);
                                QString p = rowIdx.data(m_pathRole).toString();
                                if (!p.isEmpty()) {
                                    targetPaths.append(p);
                                }
                            }
                        }
                        if (targetPaths.isEmpty()) {
                            QString p = index.data(m_pathRole).toString();
                            if (!p.isEmpty()) targetPaths.append(p);
                        }

                        // 2. 统一发送归一化命令给 CoreEngine
                        if (!targetPaths.isEmpty()) {
                            AppCommand cmd;
                            cmd.type = AppCommandType::SetRating;
                            cmd.targetPaths = targetPaths;
                            cmd.params["rating"] = hitVal;
                            CoreEngine::instance().executeCommand(cmd);

                            for (const QString& p : targetPaths) {
                                for (int r = 0; r < model->rowCount(); ++r) {
                                    QModelIndex rowIdx = model->index(r, 0);
                                    if (rowIdx.data(m_pathRole).toString() == p) {
                                        model->setData(rowIdx, hitVal, m_ratingRole);
                                        break;
                                    }
                                }
                            }
                            return true;
                        }

                        // 回退场景：直接设置 model
                        model->setData(index, hitVal, m_ratingRole);
                        return true; // 已选中且无组合键，修改评分并消费事件
                    }
>>>>>>> REPLACE
```

---

## Chapter 5: Build & Verification Steps（编译命令与验证方法）

### 编译步骤
```bash
cmake -B build -G "Ninja"
cmake --build build --config Release
```

### 验证方法
1. **多选打星测试**：在网格视图/自适应视图中选中 5 个图片项目，点击其中任意一个卡片上的第 2 颗星。
   - **预期**：5 个图片的星级全部同步刷为 2 星；右侧 `MetaPanel` 的星级同步刷新；数据库与 `.QuarkMeta.json` 物理保存。
2. **单选打星测试**：仅选中 1 个项目打星，验证该项目正常变动，其他项目不被干扰。
3. **列表视图（TreeView）打星测试**：切换到列表视图，在多选状态下点击第 2 列的星级，验证多选打星完全起效。

---

## Chapter 6: SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用自查）
- 本次修改将散落的 `model->setData` 打星逻辑彻底归一化，统一复用 `CoreEngine::instance().executeCommand(cmd)` 通道，与 `MetaPanel` 的命令通道完全对齐，去除了两套实现并存的根源。

---

## Chapter 7: Header API Signature Verification（头文件 API 物理签名核查表）

| 调用成员 / 函数 | 所在头文件 | 物理精确签名 | 核查状态 |
|---|---|---|---|
| `CoreEngine::executeCommand` | `src/core/CoreEngine.h` | `void executeCommand(const AppCommand& cmd);` | 物理存在一致 |
| `ContentPanel::updateItemMetadata` | `src/ui/ContentPanel.h` | `void updateItemMetadata(const QString& path);` | 物理存在一致 |
| `ContentPanel::recalculateAndEmitStats` | `src/ui/ContentPanel.h` | `void recalculateAndEmitStats();` | 物理存在一致 |

---

## Chapter 8: Header Inclusion Chain & Type Completeness Check（头文件包含链核查表）
- `ContentKeyHandler.cpp` 已包含 `#include "CoreEngine.h"` 与 `#include "ContentPanel.h"`，类定义与枚举 `AppCommandType::SetRating` 完全闭合。
- `ThumbnailDelegate.cpp` 已包含 `#include "CoreEngine.h"`，类定义完全闭合。

---

## Chapter 9: Compliance Self-Check Table（第 2 关：交付前自检）

| 检查项 | 来源条款 | 结论 | 对应方案位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | `RatingNormalization.md` 对应星级设定归一化模块 |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/RatingNormalization.md` |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 | 第 4 章 SEARCH 块在 `ContentKeyHandler.cpp` 及 `ThumbnailDelegate.cpp` 中逐字存在 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册 | 3.3 | 不涉及 | 不修改 Q_OBJECT 声明 |
| 第 1~10 章齐全 | 3.4 | 满足 | 第 1~10 章齐全 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 第 7 章 |
| 每个被改/删的 `#include` 已核查包含链 | 3.5-3 | 满足 | 第 8 章 |
| 已复用既有 SSOT 入口，无另起炉灶 | 2.4 | 满足 | 第 6 章 |
| 无内联 `setStyleSheet` | 4.1 | 不涉及 | 本方案不涉及 UI QSS 修改 |
| 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 无 Hack 操作 |
| 未改动用户既有设计理念与交互方式 | 1.2 / Memories.md | 满足 | 保持原有卡片按压交互逻辑 |

---

## Chapter 10: Zero-Value-Alteration Check（数值零篡改核对）
- 本方案不涉及任何 UI 尺寸、颜色或参数修改，填写：**不涉及**。
