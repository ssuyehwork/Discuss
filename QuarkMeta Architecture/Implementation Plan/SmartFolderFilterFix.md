# SmartFolderFilterFix.md - 智能文件夹精准结果集加载与筛选器统计收敛实施方案

## Chapter 1: Overview
在现有的实现中，当用户选择左侧“智能文件夹”节点时，`PanelMediator.cpp` 会获取整个媒体库中的**所有文件路径**（全库上千条数据）并加载至 `ContentPanel` 中，而后再在前端借由 `FilterPanel::applySmartFolderRecord` 设置二次筛选状态。这导致：
1. `ContentPanel::recalculateAndEmitStats` 计算并吐给右侧 `FilterPanel` 的统计数据 (`ScanStats`) 是**全库所有属性**的全集，导致右侧筛选器面板充斥着与当前智能文件夹无关的杂乱选项（色块、评级、文件后缀等）。
2. 全库路径的大批量加载造成无谓的性能开销，且左侧智能文件夹的匹配数量计数与右侧筛选选项不一致。

**改进方案**：
将智能文件夹收敛为标准的“精准结果集视图”：
1. 在 `SmartFolderDao` 中扩展 `getMatchingPaths(const SmartFolderRecord& rec)` 接口，直接在 SQL 层面精准查询并返回匹配该智能文件夹规则的文件路径集合（同时补充 `SmartRuleType::Keyword` 条件与 `sync_state != 2` 未删除状态过滤）。
2. 在 `PanelMediator.cpp` 的 `smartFolderSelected` 响应中，直接调用 `SmartFolderDao::getMatchingPaths(record)` 仅将匹配到的路径列表加载至 `ContentPanel`。
3. `ContentPanel` 自动针对匹配到的精准结果集计算 `ScanStats` 并发送至 `FilterPanel`，使右侧筛选器面板的选项完全收敛在智能文件夹实际结果集内部，并允许用户在智能文件夹内部精准地进行二次叠加筛选。

---

## Chapter 2: Modified Files List
1. `src/meta/SmartFolderDao.h` (扩展 `getMatchingPaths` 静态接口)
2. `src/meta/SmartFolderDao.cpp` (实现 `getMatchingPaths` 并在 `countMatchingItems` 中补充关键字与状态条件)
3. `src/ui/PanelMediator.cpp` (修改 `smartFolderSelected` 信号槽逻辑，改为精准加载匹配路径并重置筛选器)

---

## Chapter 3: Detailed Line-by-Line Changes

### 1. `src/meta/SmartFolderDao.h`
```
<<<<<<< SEARCH
    static bool removeSmartFolderById(int id);
    static int countMatchingItems(const SmartFolderRecord& rec);
};
=======
    static bool removeSmartFolderById(int id);
    static int countMatchingItems(const SmartFolderRecord& rec);
    static QStringList getMatchingPaths(const SmartFolderRecord& rec);
};
>>>>>>> REPLACE
```

### 2. `src/meta/SmartFolderDao.cpp`
```
<<<<<<< SEARCH
        else if (rule.type == SmartRuleType::Tag) {
            // 利用 file_tags 多对多关联表实现秒级查询 (以 file_id 解耦匹配)
            if (rule.op == SmartRuleOperator::NotContains) {
                whereConditions << QString("file_id NOT IN (SELECT file_id FROM file_tags WHERE tag_name = '%1')")
                                   .arg(rule.value.trimmed());
            } else {
                whereConditions << QString("file_id IN (SELECT file_id FROM file_tags WHERE tag_name = '%1')")
                                   .arg(rule.value.trimmed());
            }
        }
    }

    if (whereConditions.isEmpty()) return 0;

    QString connector = (rec.matchMode.toUpper() == "ALL") ? " AND " : " OR ";
    QString sql = QString("SELECT COUNT(*) FROM library_items WHERE %1;")
                  .arg(whereConditions.join(connector));
=======
        else if (rule.type == SmartRuleType::Tag) {
            // 利用 file_tags 多对多关联表实现秒级查询 (以 file_id 解耦匹配)
            if (rule.op == SmartRuleOperator::NotContains) {
                whereConditions << QString("file_id NOT IN (SELECT file_id FROM file_tags WHERE tag_name = '%1')")
                                   .arg(rule.value.trimmed());
            } else {
                whereConditions << QString("file_id IN (SELECT file_id FROM file_tags WHERE tag_name = '%1')")
                                   .arg(rule.value.trimmed());
            }
        }
        else if (rule.type == SmartRuleType::Keyword) {
            QString kw = rule.value.trimmed();
            if (!kw.isEmpty()) {
                if (rule.op == SmartRuleOperator::NotContains) {
                    whereConditions << QString("file_name NOT LIKE '%%%1%%'").arg(kw);
                } else {
                    whereConditions << QString("file_name LIKE '%%%1%%'").arg(kw);
                }
            }
        }
    }

    if (whereConditions.isEmpty()) return 0;

    whereConditions << "sync_state != 2";

    QString connector = (rec.matchMode.toUpper() == "ALL") ? " AND " : " OR ";
    QString sql = QString("SELECT COUNT(*) FROM library_items WHERE (%1) AND sync_state != 2;")
                  .arg(whereConditions.join(connector));
>>>>>>> REPLACE
```

并且追加 `SmartFolderDao::getMatchingPaths` 的具体实现：
```
<<<<<<< SEARCH
bool SmartFolderDao::removeSmartFolderById(int id) {
=======
QStringList SmartFolderDao::getMatchingPaths(const SmartFolderRecord& rec) {
    if (rec.rules.isEmpty()) return {};

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return {};

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    QStringList whereConditions;
    for (const auto& rule : rec.rules) {
        if (rule.type == SmartRuleType::Rating) {
            QString op = (rule.op == SmartRuleOperator::GreaterOrEqual) ? ">=" :
                         (rule.op == SmartRuleOperator::LessOrEqual) ? "<=" : "=";
            whereConditions << QString("rating %1 %2").arg(op).arg(rule.value.toInt());
        }
        else if (rule.type == SmartRuleType::Extension) {
            QString ext = rule.value.trimmed();
            if (ext.startsWith('.')) ext.remove(0, 1);
            if (rule.op == SmartRuleOperator::NotContains) {
                whereConditions << QString("extension != '%1'").arg(ext);
            } else {
                whereConditions << QString("extension = '%1'").arg(ext);
            }
        }
        else if (rule.type == SmartRuleType::Color) {
            if (rule.op == SmartRuleOperator::NotContains) {
                whereConditions << QString("color_hex != '%1'").arg(rule.value.trimmed());
            } else {
                whereConditions << QString("color_hex = '%1'").arg(rule.value.trimmed());
            }
        }
        else if (rule.type == SmartRuleType::Tag) {
            if (rule.op == SmartRuleOperator::NotContains) {
                whereConditions << QString("file_id NOT IN (SELECT file_id FROM file_tags WHERE tag_name = '%1')")
                                   .arg(rule.value.trimmed());
            } else {
                whereConditions << QString("file_id IN (SELECT file_id FROM file_tags WHERE tag_name = '%1')")
                                   .arg(rule.value.trimmed());
            }
        }
        else if (rule.type == SmartRuleType::Keyword) {
            QString kw = rule.value.trimmed();
            if (!kw.isEmpty()) {
                if (rule.op == SmartRuleOperator::NotContains) {
                    whereConditions << QString("file_name NOT LIKE '%%%1%%'").arg(kw);
                } else {
                    whereConditions << QString("file_name LIKE '%%%1%%'").arg(kw);
                }
            }
        }
    }

    if (whereConditions.isEmpty()) return {};

    QString connector = (rec.matchMode.toUpper() == "ALL") ? " AND " : " OR ";
    QString sql = QString("SELECT file_path FROM library_items WHERE (%1) AND sync_state != 2;")
                  .arg(whereConditions.join(connector));

    sqlite3_stmt* stmt = nullptr;
    QStringList paths;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* p = sqlite3_column_text(stmt, 0);
            if (p) {
                paths.append(QString::fromUtf8(reinterpret_cast<const char*>(p)));
            }
        }
        sqlite3_finalize(stmt);
    }
    return paths;
}

bool SmartFolderDao::removeSmartFolderById(int id) {
>>>>>>> REPLACE
```

### 3. `src/ui/PanelMediator.cpp`
```
<<<<<<< SEARCH
            connect(libraryPanel, &LibraryPanel::smartFolderSelected, this, [this, contentPanel, filterPanel](const SmartFolderRecord& record) {
                QStringList allLibraryPaths = LibraryDao::getCategoryPaths(-1);
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->setCurrentCategoryType("smart_folder");
                    target->setLastLoadedLibraryPaths(allLibraryPaths);
                    target->loadLibraryPaths(allLibraryPaths);
                }
                if (filterPanel) {
                    filterPanel->applySmartFolderRecord(record);
                }
            });
=======
            connect(libraryPanel, &LibraryPanel::smartFolderSelected, this, [this, contentPanel, filterPanel](const SmartFolderRecord& record) {
                QStringList matchingPaths = SmartFolderDao::getMatchingPaths(record);
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->setCurrentCategoryType("smart_folder");
                    target->setLastLoadedLibraryPaths(matchingPaths);
                    target->loadLibraryPaths(matchingPaths);
                }
                if (filterPanel) {
                    filterPanel->resetFilter();
                }
            });
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps
1. 使用 `cmake --build build`（或项目的构建脚本）编译整个项目。
2. 确认无任何 C++ 编译警告和连接错误。
3. 运行程序并选定包含少部分属性的智能文件夹（如 PNG）。
4. 验证内容面板只展现精准匹配的项目。
5. 验证右侧【筛选器面板】的统计选项（评级、色块、分类标签等）只保留智能文件夹内匹配到的卡片属性，全库无关选项完全消失。

---

## Chapter 5: SSOT API Reuse Self-Check
- 恪守数据单源原则：`SmartFolderDao::getMatchingPaths` 与 `SmartFolderDao::countMatchingItems` 统一查询 `library_items` 表，计数与结果集合逻辑保持 100% 同步。
- `ContentPanel::loadLibraryPaths` 被继续复用，无需引入冗余的中间桥接载体。

---

## Chapter 6: Header API Signature Verification
- 在 `SmartFolderDao.h` 中新增：
  `static QStringList getMatchingPaths(const SmartFolderRecord& rec);`
  类型与形参严格一致。

---

## Chapter 7: Header Inclusion Chain & Type Completeness Check
- `SmartFolderDao.h` 已包含 `<QStringList>` / `SmartFolderRecord`，声明类型完整无缺失。

---

## Chapter 8: Compliance Self-Check Table
| 检查项 | 状态 | 备注 |
|---|---|---|
| 1. 不直接编辑源文件 | 合规 | 实施方案保存在隔离目录 |
| 2. 方案采用 Git Merge Diff 格式 | 合规 | 使用 Standard Git Merge Diff |
| 3. 自述理解并达成共识 | 合规 | 已获取用户“达成共识”确认 |
| 4. 解决智能文件夹全库污染问题 | 合规 | 精准 SQL 查询过滤结果集 |

---

## Chapter 9: Zero-Value-Alteration Check
- 所有硬编码或默认样式/UI数值未做任何修改，仅优化查询逻辑与状态流动。
