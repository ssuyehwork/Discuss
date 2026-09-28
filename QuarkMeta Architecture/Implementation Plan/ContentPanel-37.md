# Implementation Plan - ContentPanel List View File Header Removal (`ContentPanel-37.md`)

## Overview
This implementation plan physically removes the redundant `m_listFileHeader` (`FileSectionHeaderBar`) from `ContentPanel`'s List View mode. In the single `m_treeView` architecture, placing `m_listFileHeader` above `m_treeView` falsely rendered all folders inside `m_treeView` underneath the file header label. Removing `m_listFileHeader` restores clean List View hierarchy with only `m_listFolderHeader` on top of `m_treeView`.

---

## Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.h`

```
<<<<<<< SEARCH
    QWidget* m_listContainerWidget = nullptr;
    FolderSectionHeaderBar* m_listFolderHeader = nullptr;
    FileSectionHeaderBar* m_listFileHeader = nullptr;

    FilterProxyModel* m_proxyModel = nullptr;
=======
    QWidget* m_listContainerWidget = nullptr;
    FolderSectionHeaderBar* m_listFolderHeader = nullptr;

    FilterProxyModel* m_proxyModel = nullptr;
>>>>>>> REPLACE
```

---

### 2. `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
    m_listFolderHeader = new FolderSectionHeaderBar(m_listContainerWidget);
    m_listFolderHeader->hide();
    layout->addWidget(m_listFolderHeader);

    m_listFileHeader = new FileSectionHeaderBar(m_listContainerWidget);
    m_listFileHeader->hide();
    layout->addWidget(m_listFileHeader);

    m_treeView = new DropTreeView(m_listContainerWidget);
=======
    m_listFolderHeader = new FolderSectionHeaderBar(m_listContainerWidget);
    m_listFolderHeader->hide();
    layout->addWidget(m_listFolderHeader);

    m_treeView = new DropTreeView(m_listContainerWidget);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    auto updateListSectionCounts = [this]() {
        if (!m_model) return;
        int folderCount = 0;
        int fileCount = 0;
        const auto& records = m_model->allRecords();
        for (const auto& rec : records) {
            if (rec.isDir) folderCount++;
            else fileCount++;
        }

        if (m_listFolderHeader) {
            m_listFolderHeader->setCount(folderCount);
            m_listFolderHeader->setVisible(folderCount > 0);
        }
        if (m_listFileHeader) {
            m_listFileHeader->setCount(fileCount);
            m_listFileHeader->setVisible(fileCount > 0);
        }
    };
=======
    auto updateListSectionCounts = [this]() {
        if (!m_model) return;
        int folderCount = 0;
        const auto& records = m_model->allRecords();
        for (const auto& rec : records) {
            if (rec.isDir) folderCount++;
        }

        if (m_listFolderHeader) {
            m_listFolderHeader->setCount(folderCount);
            m_listFolderHeader->setVisible(folderCount > 0);
        }
    };
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. Verify `QuarkMeta Architecture/Implementation Plan/ContentPanel-37.md` created.
2. Verify that `m_listFileHeader` references are completely deleted in `ContentPanel.h` and `ContentPanel.cpp`.

---

## SSOT API Reuse & Anti-Redundancy Self-Check

- Clean single `m_treeView` view architecture without misplaced outer section widgets.

---

## Header API Signature Verification Table

| File | Class / Struct | Exact Physical Signature | Verification Result |
| :--- | :--- | :--- | :--- |
| `src/ui/ContentPanel.h` | `ContentPanel` | `FolderSectionHeaderBar* m_listFolderHeader = nullptr;` | Verified 100% Match |
