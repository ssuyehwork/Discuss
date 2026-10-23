# Implementation Plan - Step 2: Contract Addition & SectionProxyModel Fixes

This implementation plan covers adding `SectionKindRole` to `ModelContract.h` and refactoring `SectionProxyModel` to use binary search mapping, incremental row updates, and persistent index migration on sorting to maintain selection state.

## 1. Overview
Currently, `SectionProxyModel` relies on full `beginResetModel()` calls on incremental updates and string matching (`startsWith("文件夹")`) to determine section header types.
This step introduces `SectionKindRole` in `ModelContract.h` (0 = normal, 1 = folder header, 2 = file header) and optimizes `SectionProxyModel`:
- Fast binary search for `mapFromSource`.
- Granular `beginInsertRows`/`beginRemoveRows` instead of reset model on incremental changes and collapse toggling.
- Migration of persistent indexes (`changePersistentIndexList`) on `layoutAboutToBeChanged`/`layoutChanged` so sorting preserves item selection.

---

## 2. Modified Files List
1. `src/core/ModelContract.h` (Add `SectionKindRole = Qt::UserRole + 215`)
2. `src/ui/models/SectionProxyModel.h`
3. `src/ui/models/SectionProxyModel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/core/ModelContract.h`
<<<<<<< SEARCH
    SectionHeaderRole    = Qt::UserRole + 212, // 是否为分区标头行
    SectionHeaderTextRole = Qt::UserRole + 213,// 分区标头显示文本
    SectionCollapsedRole = Qt::UserRole + 214, // 文件夹分区折叠状态
=======
    SectionHeaderRole    = Qt::UserRole + 212, // 是否为分区标头行
    SectionHeaderTextRole = Qt::UserRole + 213,// 分区标头显示文本
    SectionCollapsedRole = Qt::UserRole + 214, // 文件夹分区折叠状态
    SectionKindRole      = Qt::UserRole + 215, // 分区行类型 (0=普通, 1=文件夹标头, 2=文件标头)
    SectionRowRole       = Qt::UserRole + 216, // 分区内相对序号 (用于斑马纹)
>>>>>>> REPLACE

---

### 3.2 `src/ui/models/SectionProxyModel.cpp`
<<<<<<< SEARCH
void SectionProxyModel::setFolderCollapsed(bool collapsed) {
    if (m_folderCollapsed == collapsed) return;
    m_folderCollapsed = collapsed;
    rebuildMapping();
}
=======
void SectionProxyModel::setFolderCollapsed(bool collapsed) {
    if (m_folderCollapsed == collapsed) return;
    
    int folderItemCount = m_folderSourceRows.size();
    if (folderItemCount == 0) {
        m_folderCollapsed = collapsed;
        return;
    }

    if (collapsed) {
        // Remove folder item rows [1, folderItemCount]
        beginRemoveRows(QModelIndex(), 1, folderItemCount);
        m_folderCollapsed = collapsed;
        rebuildMappingInternalWithoutReset();
        endRemoveRows();
    } else {
        // Insert folder item rows [1, folderItemCount]
        beginInsertRows(QModelIndex(), 1, folderItemCount);
        m_folderCollapsed = collapsed;
        rebuildMappingInternalWithoutReset();
        endInsertRows();
    }
}
>>>>>>> REPLACE

<<<<<<< SEARCH
void SectionProxyModel::onSourceLayoutAboutToBeChanged() {
    emit layoutAboutToBeChanged();
}

void SectionProxyModel::onSourceLayoutChanged() {
    rebuildMapping();
    emit layoutChanged();
}
=======
void SectionProxyModel::onSourceLayoutAboutToBeChanged() {
    emit layoutAboutToBeChanged();
    m_persistentSrcIndexes = persistentIndexList();
}

void SectionProxyModel::onSourceLayoutChanged() {
    QModelIndexList oldProxyIndexes = m_persistentSrcIndexes;
    QModelIndexList newProxyIndexes;
    
    rebuildMappingInternalWithoutReset();

    for (const QModelIndex& oldProxyIdx : oldProxyIndexes) {
        QModelIndex srcIdx = mapToSource(oldProxyIdx);
        QModelIndex newProxyIdx = mapFromSource(srcIdx);
        newProxyIndexes.append(newProxyIdx);
    }

    changePersistentIndexList(oldProxyIndexes, newProxyIndexes);
    emit layoutChanged();
}
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Apply changes to `ModelContract.h`, `SectionProxyModel.h/.cpp`.
2. Compile project using CMake.
3. Test folder collapse and item sorting: verify selections in file section remain intact when collapsing folder section or sorting columns.
