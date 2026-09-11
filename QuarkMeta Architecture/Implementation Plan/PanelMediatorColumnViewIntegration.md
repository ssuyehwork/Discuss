# PanelMediatorColumnViewIntegration Implementation Plan

This implementation plan details the exact changes required in `PanelMediator.cpp` and `ContentPanel` to ensure seamless bidirectional synchronization between `ColumnViewWidget` (Miller Columns) and `MetaPanel`.

## Overview
Currently, `PanelMediator.cpp` binds `metaPanel` updates exclusively to `contentPanel->model()->dataChanged`. Since `ColumnViewWidget` uses private `DiskItemModel` instances for each column, changes originating from Column View do not emit through `contentPanel->model()`, leaving `MetaPanel` unaffected. Furthermore, when selections change in Column View, `PanelMediator` reads raw un-decorated model data, resulting in empty/zero rating, color, note, and URL properties being passed to `MetaPanel`.

This plan:
1. Adds an aggregated signal `itemDataChanged` on `ContentPanel` that emits whenever either the main model or any column view private model changes.
2. Updates `PanelMediator.cpp` to listen to `ContentPanel::itemDataChanged` instead of `contentPanel->model()->dataChanged`.
3. Adds `MetadataManager::instance().getMeta(path)` fallback in `PanelMediator.cpp` during `selectionChanged` handling so `MetaPanel` always receives accurate SSOT metadata for Column View selections.

---

## Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/PanelMediator.cpp`

---

## Detailed Line-by-Line Changes

### File: `src/ui/ContentPanel.h`

```git
<<<<<<< SEARCH
signals:
    void zoomLevelChanged(int level);
    void viewModeChanged(ViewMode mode);
=======
signals:
    void itemDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    void zoomLevelChanged(int level);
    void viewModeChanged(ViewMode mode);
>>>>>>> REPLACE
```

---

### File: `src/ui/ContentPanel.cpp`

```git
<<<<<<< SEARCH
    // 核心架构闭环：监听底层模型元数据变更（卡片点击、列表点击、快捷键赋予、F4重复等），自动防抖驱动统计重算与筛选器同步
    connect(m_diskModel, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex&, const QModelIndex&, const QList<int>& roles) {
        if (roles.isEmpty() || roles.contains(RatingRole) || roles.contains(ColorRole) || roles.contains(TagsRole)) {
            if (m_statsDebounceTimer) {
                m_statsDebounceTimer->start();
            }
        }
    });
=======
    // 核心架构闭环：监听底层模型元数据变更（卡片点击、列表点击、快捷键赋予、F4重复等），自动防抖驱动统计重算与筛选器同步
    connect(m_diskModel, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles) {
        if (roles.isEmpty() || roles.contains(RatingRole) || roles.contains(ColorRole) || roles.contains(TagsRole)) {
            if (m_statsDebounceTimer) {
                m_statsDebounceTimer->start();
            }
        }
        emit itemDataChanged(topLeft, bottomRight, roles);
    });
>>>>>>> REPLACE
```

---

### File: `src/ui/PanelMediator.cpp`

```git
<<<<<<< SEARCH
    // 2. 内容面板选中项改变 -> 元数据面板 0 毫秒极速同步
    if (contentPanel && metaPanel) {
        // 监听卡片/列表上的就地修改，0 毫秒同步右侧 MetaPanel
        connect(contentPanel->model(), &QAbstractItemModel::dataChanged, metaPanel,
                [contentPanel, metaPanel](const QModelIndex& topLeft, const QModelIndex&, const QVector<int>& roles) {
=======
    // 2. 内容面板选中项改变 -> 元数据面板 0 毫秒极速同步
    if (contentPanel && metaPanel) {
        // 监听卡片/列表上的就地修改，0 毫秒同步右侧 MetaPanel
        connect(contentPanel, &ContentPanel::itemDataChanged, metaPanel,
                [contentPanel, metaPanel](const QModelIndex& topLeft, const QModelIndex&, const QVector<int>& roles) {
>>>>>>> REPLACE
```

```git
<<<<<<< SEARCH
                metaPanel->updateInfo(
                    name, type, sizeStr, "-", mtimeStr, "-",
                    path, idx.data(EncryptedRole).toBool(), 0, 0
                );
                metaPanel->setRating(idx.data(RatingRole).toInt(), false);
                metaPanel->setColor(idx.data(ColorRole).toString(), false);
                metaPanel->setTags(idx.data(TagsRole).toStringList());
                metaPanel->setNote(idx.data(NoteRole).toString());
                metaPanel->setURL(idx.data(UrlRole).toString());
=======
                int rating = idx.data(RatingRole).toInt();
                QString colorStr = idx.data(ColorRole).toString();
                QStringList tags = idx.data(TagsRole).toStringList();
                QString note = idx.data(NoteRole).toString();
                QString url = idx.data(UrlRole).toString();

                if (rating == 0 && colorStr.isEmpty() && tags.isEmpty() && note.isEmpty() && url.isEmpty()) {
                    RuntimeMeta meta = MetadataManager::instance().getMeta(path.toStdWString());
                    rating = meta.rating;
                    colorStr = QString::fromStdWString(meta.manualColor);
                    tags = meta.tags;
                    note = QString::fromStdWString(meta.note);
                    url = QString::fromStdWString(meta.url);
                }

                metaPanel->updateInfo(
                    name, type, sizeStr, "-", mtimeStr, "-",
                    path, idx.data(EncryptedRole).toBool(), 0, 0
                );
                metaPanel->setRating(rating, false);
                metaPanel->setColor(colorStr, false);
                metaPanel->setTags(tags);
                metaPanel->setNote(note);
                metaPanel->setURL(url);
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **CMake Build Verification**:
   ```bash
   cmake --build build --config Debug
   ```
2. **Functional Verification**:
   - Launch QuarkMeta application and switch to Column View mode (Miller Columns).
   - Select any item in any active pane.
   - Verify that `MetaPanel` immediately displays the accurate rating stars, color tag, notes, and URL from `MetadataManager` memory SSOT.
   - Modify rating stars or color tag in Column View (or in `MetaPanel`).
   - Verify that `MetaPanel` and Column View pane both update seamlessly in real-time.
