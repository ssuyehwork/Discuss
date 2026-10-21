# SmartFolderFilterFix-1.md - 智能文件夹精准结果集加载与筛选器统计收敛实施方案（修正版）

## Chapter 1: Overview
针对 Code Review 中指出的 `SmartFolderFilterFix.md` 逻辑缺陷进行修复：
1. **修复 OR 模式下 `countMatchingItems` 逻辑失效问题**：之前将 `"sync_state != 2"` 误追加至 `whereConditions` 列表参与 `join(" OR ")` 拼接，导致表达式在 OR 模式下评估为 `(rule1 OR rule2 OR sync_state != 2) AND sync_state != 2`，全库所有未删除文件均被错误计入。修正方案严格保持 `whereConditions` 仅包含规则条件，生成的 SQL 统一为 `WHERE (%1) AND sync_state != 2`。
2. **安全转义 SQL 字符串**：对规则中的 `rule.value` 文本中的单引号进行安全转义（`.replace("'", "''")`），防止带单引号的文件名或标签引发 SQL 语法异常。
3. **精准结果集收敛**：`PanelMediator.cpp` 在 `smartFolderSelected` 触发时，直接调用 `SmartFolderDao::getMatchingPaths(record)` 仅将精准匹配的路径列表加载至 `ContentPanel`，使右侧 `FilterPanel` 的选项与统计数据完全收敛在智能文件夹实际匹配的结果集内部。

---

## Chapter 2: Modified Files List
1. `src/meta/SmartFolderDao.h` (扩展 `getMatchingPaths` 静态接口)
2. `src/meta/SmartFolderDao.cpp` (实现 `getMatchingPaths` 并修复 `countMatchingItems` 中的 OR 拼接逻辑与 SQL 单引号转义)
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
        else if (rule.type == SmartRuleType::Extension) {
            QString ext = rule.value.trimmed().replace("'", "''");
            if (ext.startsWith('.')) ext.remove(0, 1);
            if (rule.op == SmartRuleOperator::NotContains) {
                whereConditions << QString("extension != '%1'").arg(ext);
            } else {
                whereConditions << QString("extension = '%1'").arg(ext);
            }
        }
        else if (rule.type == SmartRuleType::Color) {
            QString val = rule.value.trimmed().replace("'", "''");
            if (rule.op == SmartRuleOperator::NotContains) {
                whereConditions << QString("color_hex != '%1'").arg(val);
            } else {
                whereConditions << QString("color_hex = '%1'").arg(val);
            }
        }
        else if (rule.type == SmartRuleType::Tag) {
            QString tagVal = rule.value.trimmed().replace("'", "''");
            if (rule.op == SmartRuleOperator::NotContains) {
                whereConditions << QString("file_id NOT IN (SELECT file_id FROM file_tags WHERE tag_name = '%1')")
                                   .arg(tagVal);
            } else {
                whereConditions << QString("file_id IN (SELECT file_id FROM file_tags WHERE tag_name = '%1')")
                                   .arg(tagVal);
            }
        }
        else if (rule.type == SmartRuleType::Keyword) {
            QString kw = rule.value.trimmed().replace("'", "''");
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

    QString connector = (rec.matchMode.toUpper() == "ALL") ? " AND " : " OR ";
    QString sql = QString("SELECT COUNT(*) FROM library_items WHERE (%1) AND sync_state != 2;")
                  .arg(whereConditions.join(connector));
>>>>>>> REPLACE
```

追加 `SmartFolderDao::getMatchingPaths` 方法的具体实现：
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
            QString ext = rule.value.trimmed().replace("'", "''");
            if (ext.startsWith('.')) ext.remove(0, 1);
            if (rule.op == SmartRuleOperator::NotContains) {
                whereConditions << QString("extension != '%1'").arg(ext);
            } else {
                whereConditions << QString("extension = '%1'").arg(ext);
            }
        }
        else if (rule.type == SmartRuleType::Color) {
            QString val = rule.value.trimmed().replace("'", "''");
            if (rule.op == SmartRuleOperator::NotContains) {
                whereConditions << QString("color_hex != '%1'").arg(val);
            } else {
                whereConditions << QString("color_hex = '%1'").arg(val);
            }
        }
        else if (rule.type == SmartRuleType::Tag) {
            QString tagVal = rule.value.trimmed().replace("'", "''");
            if (rule.op == SmartRuleOperator::NotContains) {
                whereConditions << QString("file_id NOT IN (SELECT file_id FROM file_tags WHERE tag_name = '%1')")
                                   .arg(tagVal);
            } else {
                whereConditions << QString("file_id IN (SELECT file_id FROM file_tags WHERE tag_name = '%1')")
                                   .arg(tagVal);
            }
        }
        else if (rule.type == SmartRuleType::Keyword) {
            QString kw = rule.value.trimmed().replace("'", "''");
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
1. 执行 `cmake --build build` 编译项目。
2. 验证智能文件夹在 OR 模式及 ALL 模式下计数与左侧匹配数量完全一致。
3. 验证智能文件夹包含单引号特殊字符规则时查询不报错。

---

## Chapter 5: SSOT API Reuse Self-Check
- 恪守数据单源原则：`SmartFolderDao::getMatchingPaths` 与 `SmartFolderDao::countMatchingItems` 统一查询 `library_items` 表，计数与结果集合逻辑保持 100% 同步。

---

## Chapter 6: Header API Signature Verification
- 在 `SmartFolderDao.h` 中新增：
  `static QStringList getMatchingPaths(const SmartFolderRecord& rec);`

---

## Chapter 7: Header Inclusion Chain & Type Completeness Check
- `SmartFolderDao.h` 已包含 `<QStringList>` 与 `SmartFolderRecord`。

---

## Chapter 8: Compliance Self-Check Table
| 检查项 | 状态 | 备注 |
|---|---|---|
| 1. 不直接编辑源文件 | 合规 | 实施方案保存在隔离目录 |
| 2. 方案采用 Git Merge Diff 格式 | 合规 | 使用 Standard Git Merge Diff |
| 3. OR 模式 SQL 拼接修复 | 合规 | 保证 `sync_state != 2` 不介入 OR join |
| 4. 字符串安全转义 | 合规 | 包含 `.replace("'", "''")` |

---

## Chapter 9: Zero-Value-Alteration Check
- 硬编码及 UI 默认参数零变动。
