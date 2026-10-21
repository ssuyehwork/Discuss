# MultiSelectionBatchRating.md - 多选状态下星级协同标定与模型通道批量更新实施方案

## Chapter 1: Overview
在现有的 `ThumbnailDelegate`（网格/瀑布流视图代理）与 `DiskItemModel` 实现中：
当用户在内容面板中选中多个文件/文件夹时（例如多选了 5 个项目），如果点击其中某个项目的星级控件（如打 2 星）：
1. 现有代码 `ThumbnailDelegate::editorEvent` 中直接调用 `model->setData(index, hitVal, m_ratingRole)`，仅将被点击的单个 `QModelIndex` 传入。
2. `DiskItemModel::setData` 接收到单行 index 后，仅更新该单行记录的 `rating` 并发出单行 `dataChanged`，导致多选状态下的其他高亮选中项未被同步标定。

**改进方案（绝不顾此失彼）**：
1. **多选批量重构**：在 `ThumbnailDelegate::editorEvent` 捕获星级按压事件时，判断该 `index` 是否处于选中状态：
   - 若被点击项目属于选中集且当前选中数量 $> 1$，提取视图中所有被选中项的路径列表（`selectedPaths`），通过统一的 `CoreEngine::handleSetRating` 发送批量指令修改全量被选中的磁盘元数据与 SQLite 数据库。
   - 若被点击项目为单选（或未选中），则保持原有的精准单项 `model->setData` 操作，保证单选性能与逻辑 100% 毫无不受干扰。
2. **视图与模型双向同步**：视图在触发批量修改后，通过遍历所有选中项的 `QModelIndex` 发送范围 `dataChanged`，确保内容面板中所有被选中卡片/行的星级图标实时刷新亮起，同时触发右侧 `MetaPanel` 与 `FilterPanel` 的统计更新。

---

## Chapter 2: Modified Files List
1. `src/ui/ThumbnailDelegate.cpp` (重构 `editorEvent` 星级点击事件处理，引入多选集合识别与批量指令分发)

---

## Chapter 3: Detailed Line-by-Line Changes

### 1. `src/ui/ThumbnailDelegate.cpp`
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
                        // 检查当前视图是否有多选集合
                        const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(option.widget);
                        if (view && view->selectionModel()) {
                            QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
                            // 过滤去重，确保每个选中的 row 只取 column 0
                            QList<int> selectedRows;
                            for (const QModelIndex& selIdx : selectedIndexes) {
                                if (!selectedRows.contains(selIdx.row())) {
                                    selectedRows.append(selIdx.row());
                                }
                            }

                            if (selectedRows.size() > 1 && selectedRows.contains(index.row())) {
                                // 多选模式：收集所有选中项路径批量更新
                                QStringList selectedPaths;
                                for (int r : selectedRows) {
                                    QModelIndex rowIdx = model->index(r, 0);
                                    QString p = rowIdx.data(m_pathRole).toString();
                                    if (!p.isEmpty()) {
                                        selectedPaths.append(p);
                                    }
                                }

                                if (!selectedPaths.isEmpty()) {
                                    AppCommand cmd;
                                    cmd.type = AppCommandType::SetRating;
                                    cmd.targetPaths = selectedPaths;
                                    cmd.params["rating"] = hitVal;
                                    CoreEngine::instance().executeCommand(cmd);

                                    // 批量发送 dataChanged 刷新模型数据与 UI
                                    for (int r : selectedRows) {
                                        QModelIndex targetIdx = model->index(r, 0);
                                        model->setData(targetIdx, hitVal, m_ratingRole);
                                    }
                                    return true;
                                }
                            }
                        }

                        // 单选或回退场景：保持原有单条 setData 修改
                        model->setData(index, hitVal, m_ratingRole);
                        return true; // 已选中且无组合键，修改评分并消费事件
                    }
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps
1. 使用 `cmake --build build` 编译项目。
2. **单选场景验证**：只选中 1 个文件，点击 3 星，验证该文件星级变 3 星，其他文件不受影响。
3. **多选场景验证**：在内容面板中通过 Ctrl/Shift 或框选选中 5 个文件，点击其中任意一个文件的 2 星位置：
   - 验证所有 5 个被选中的图片卡片星级全部同步变亮为 2 星。
   - 验证数据库与 `.QuarkMeta.json` 中 5 个文件元数据均已更新。
4. **属性面板与筛选器同步**：验证右侧 FilterPanel 统计数据即时刷新。

---

## Chapter 5: SSOT API Reuse Self-Check
- 恪守数据单源原则：多选更新复用系统既有的 `CoreEngine::instance().executeCommand(cmd)` 发送 `AppCommandType::SetRating` 聚合命令，完全与 `MetaPanel` 的星级修改命令通道保持一致。

---

## Chapter 6: Header API Signature Verification
- 本修改仅涉及 `ThumbnailDelegate.cpp` 内部逻辑实现重构，无物理头文件 API 增删变动。

---

## Chapter 7: Header Inclusion Chain & Type Completeness Check
- `ThumbnailDelegate.cpp` 包含 `<QAbstractItemView>` 与 `<QItemSelectionModel>` 支持选中模式判断，头文件链完整。

---

## Chapter 8: Compliance Self-Check Table
| 检查项 | 状态 | 备注 |
|---|---|---|
| 1. 不直接编辑源文件 | 合规 | 实施方案保存在隔离目录 |
| 2. 方案采用 Git Merge Diff 格式 | 合规 | 使用 Standard Git Merge Diff |
| 3. 多选状态下同步修改所有项 | 合规 | 提取多选集合发送 SetRating 指令 |
| 4. 顾此失彼自查 | 合规 | 单选场景 100% 保持原分支逻辑 |

---

## Chapter 9: Zero-Value-Alteration Check
- UI 交互数值及星级绘图尺寸零改动。
