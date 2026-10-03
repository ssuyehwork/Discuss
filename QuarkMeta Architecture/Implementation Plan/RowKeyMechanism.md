# RowKeyMechanism Implementation Plan

## 1. Overview
在 `FilterPanel` 中新增“行键” (`rowKey`) 机制，用固定内部标识代替界面显示文字，实现勾选状态同步与数量调用的精确匹配。
本步骤只新增机制，不删除任何现有的文字判断，12 个已抽离的分组类文件 100% 一行不动。

## 2. Modified Files List
- **修改**：`src/ui/FilterPanel.h`
- **修改**：`src/ui/FilterPanel.cpp`
- **物理隔离**：`QuarkMeta Architecture/Implementation Plan/RowKeyMechanism.md`

## 3. Detailed Line-by-Line Changes

### `src/ui/FilterPanel.h`
```diff
<<<<<<< SEARCH
    QCheckBox* addFilterRow(QVBoxLayout* layout, const QString& label,
                            int count, const QColor& dotColor = Qt::transparent);
=======
    QCheckBox* addFilterRow(QVBoxLayout* layout, const QString& label,
                            int count, const QColor& dotColor = Qt::transparent,
                            const QString& rowKey = QString());

    bool isRowKeyChecked(const QString& key, const FilterState& st) const;
    int countForRowKey(const QString& key) const;
>>>>>>> REPLACE
```

### `src/ui/FilterPanel.cpp`

**Change 1: Update `addFilterRow` implementation**
```diff
<<<<<<< SEARCH
QCheckBox* FilterPanel::addFilterRow(QVBoxLayout* layout, const QString& label,
                                      int count, const QColor& dotColor) {
    ClickableRow* row = new ClickableRow();
=======
QCheckBox* FilterPanel::addFilterRow(QVBoxLayout* layout, const QString& label,
                                      int count, const QColor& dotColor,
                                      const QString& rowKey) {
    ClickableRow* row = new ClickableRow();
    if (!rowKey.isEmpty()) {
        row->setProperty("rowKey", rowKey);
    }
>>>>>>> REPLACE
```

**Change 2: Add `isRowKeyChecked` and `countForRowKey` implementations**
```cpp
bool FilterPanel::isRowKeyChecked(const QString& key, const FilterState& st) const {
    int colonIdx = key.indexOf(':');
    if (colonIdx == -1) return false;

    QString prefix = key.left(colonIdx);
    QString value = key.mid(colonIdx + 1);

    if (prefix == "rating") {
        return st.ratings.contains(value.toInt());
    } else if (prefix == "color") {
        if (st.colors.contains(value)) return true;
        for (const auto& item : Style::getColorPalette()) {
            if (item.hex == value && st.colors.contains(item.name)) return true;
        }
        return false;
    } else if (prefix == "type") {
        return st.types.contains(value);
    } else if (prefix == "createDate") {
        return st.createDates.contains(value);
    } else if (prefix == "modifyDate") {
        return st.modifyDates.contains(value);
    } else if (prefix == "tag") {
        if (value == "yes") return st.tagPresence == FilterState::Yes;
        if (value == "no") return st.tagPresence == FilterState::No;
    } else if (prefix == "link") {
        if (value == "yes") return st.linkPresence == FilterState::Yes;
        if (value == "no") return st.linkPresence == FilterState::No;
    } else if (prefix == "note") {
        if (value == "yes") return st.notePresence == FilterState::Yes;
        if (value == "no") return st.notePresence == FilterState::No;
    } else if (prefix == "ratio") {
        if (value == "h") return st.ratio == FilterState::Horizontal;
        if (value == "v") return st.ratio == FilterState::Vertical;
        if (value == "sq") return st.ratio == FilterState::Square;
        if (value == "169") return st.ratio == FilterState::Ratio169;
    } else if (prefix == "dup") {
        if (value == "only") return st.duplicatePresence == FilterState::DuplicateOnly;
        if (value == "unique") return st.duplicatePresence == FilterState::UniqueOnly;
    } else if (prefix == "thumb") {
        if (value == "has") return st.thumbnailPresence == FilterState::HasThumbnail;
        if (value == "none") return st.thumbnailPresence == FilterState::NoThumbnail;
    }
    return false;
}

int FilterPanel::countForRowKey(const QString& key) const {
    int colonIdx = key.indexOf(':');
    if (colonIdx == -1) return 0;

    QString prefix = key.left(colonIdx);
    QString value = key.mid(colonIdx + 1);

    if (prefix == "rating") {
        return m_ratingCounts.value(value.toInt(), 0);
    } else if (prefix == "color") {
        QString name;
        for (const auto& item : Style::getColorPalette()) {
            if (item.hex == value) {
                name = item.name;
                break;
            }
        }
        return m_colorCounts.value(value, m_colorCounts.value(name, 0));
    } else if (prefix == "type") {
        if (value == "空文件夹") return m_emptyFolderCount;
        return m_typeCounts.value(value, 0);
    } else if (prefix == "createDate") {
        return m_createDateCounts.value(value, 0);
    } else if (prefix == "modifyDate") {
        return m_modifyDateCounts.value(value, 0);
    } else if (prefix == "tag") {
        if (value == "yes") return m_currentStats.hasTagCount;
        if (value == "no") return m_currentStats.noTagCount;
    } else if (prefix == "link") {
        if (value == "yes") return m_currentStats.hasLinkCount;
        if (value == "no") return m_currentStats.noLinkCount;
    } else if (prefix == "note") {
        if (value == "yes") return m_currentStats.hasNoteCount;
        if (value == "no") return m_currentStats.noNoteCount;
    } else if (prefix == "ratio") {
        if (value == "h") return m_currentStats.ratioHorizontalCount;
        if (value == "v") return m_currentStats.ratioVerticalCount;
        if (value == "sq") return m_currentStats.ratioSquareCount;
        if (value == "169") return m_currentStats.ratio169Count;
    } else if (prefix == "dup") {
        if (value == "only") return m_currentStats.duplicateCount;
        if (value == "unique") return m_currentStats.uniqueCount;
    } else if (prefix == "thumb") {
        if (value == "has") return m_currentStats.hasThumbnailCount;
        if (value == "none") return m_currentStats.noThumbnailCount;
    }
    return 0;
}
```

**Change 3: Insert rowKey check at top of loop in `syncUIFromFilterState()`**
```diff
<<<<<<< SEARCH
    QList<StyledCheckBox*> allCheckBoxes = findChildren<StyledCheckBox*>();
    for (auto* cb : allCheckBoxes) {
        ClickableRow* row = qobject_cast<ClickableRow*>(cb->parentWidget());
        if (!row) continue;
        
        QLabel* labelWidget = row->findChild<QLabel*>();
        if (!labelWidget) continue;
=======
    QList<StyledCheckBox*> allCheckBoxes = findChildren<StyledCheckBox*>();
    for (auto* cb : allCheckBoxes) {
        ClickableRow* row = qobject_cast<ClickableRow*>(cb->parentWidget());
        if (!row) continue;

        QVariant keyProp = row->property("rowKey");
        if (keyProp.isValid() && !keyProp.toString().isEmpty()) {
            bool shouldCheck = isRowKeyChecked(keyProp.toString(), currentSt);
            cb->blockSignals(true);
            cb->setChecked(shouldCheck);
            cb->blockSignals(false);
            continue;
        }
        
        QLabel* labelWidget = row->findChild<QLabel*>();
        if (!labelWidget) continue;
>>>>>>> REPLACE
```

**Change 4: Insert rowKey count update at top of loop in `populate()`**
```diff
<<<<<<< SEARCH
        QList<ClickableRow*> rows = m_container->findChildren<ClickableRow*>();
        for (auto* row : rows) {
             QList<QLabel*> labels = row->findChildren<QLabel*>();
             if (labels.size() >= 2) {
=======
        QList<ClickableRow*> rows = m_container->findChildren<ClickableRow*>();
        for (auto* row : rows) {
             QVariant keyProp = row->property("rowKey");
             if (keyProp.isValid() && !keyProp.toString().isEmpty()) {
                 QLabel* cntLabel = row->findChild<QLabel*>("FilterItemCountLabel");
                 if (cntLabel) {
                     cntLabel->setText(QString::number(countForRowKey(keyProp.toString())));
                 }
                 continue;
             }

             QList<QLabel*> labels = row->findChildren<QLabel*>();
             if (labels.size() >= 2) {
>>>>>>> REPLACE
```

**Change 5: Add rowKey parameter call in `rebuildDateCheckboxes()`**
```diff
<<<<<<< SEARCH
    for (const QString& d : dates) {
        QCheckBox* cb = addFilterRow(layout, d, counts[d]);
=======
    for (const QString& d : dates) {
        QString rowKey = isCreateDate ? ("createDate:" + d) : ("modifyDate:" + d);
        QCheckBox* cb = addFilterRow(layout, d, counts[d], Qt::transparent, rowKey);
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. 编译验证，确认 12 个分组类文件完全未修改且能正常编译。
2. 验证创建日期与修改日期行，核对 `setProperty("rowKey", ...)` 是否成功生效。
3. 验证点击测试：勾选与切换创建日期/修改日期，确认独立匹配互不干扰。

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 所有的状态提取与修改依然统领于 `FilterStateModel` 与 `FilterState` 基础设施；
- 保持原文字判断路径做兜底兼容防护。

## 6. Header API Signature Verification
- `addFilterRow(...)`: `QCheckBox* addFilterRow(QVBoxLayout* layout, const QString& label, int count, const QColor& dotColor = Qt::transparent, const QString& rowKey = QString())`
- `isRowKeyChecked(const QString&, const FilterState&)`: `bool isRowKeyChecked(const QString& key, const FilterState& st) const`
- `countForRowKey(const QString&)`: `int countForRowKey(const QString& key) const`

## 7. Header Inclusion Chain & Type Completeness Check
- `FilterPanel.h` 保留原有包含关系，所有 `FilterState`、`ClickableRow`、`Style` 相关类型闭合无缺失。
