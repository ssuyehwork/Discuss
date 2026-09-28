# Implementation Plan - ContentPanel List View File Section Header Integration (`ContentPanel-36.md`)

## Overview
This implementation plan adds the `FileSectionHeaderBar` (`m_listFileHeader`) into `ContentPanel`'s List View container (`m_listContainerWidget`), ensuring both folder and file section headers are displayed appropriately in List View mode, with live counts for folders (`文件夹 (N)`) and files (`文件 (M)`).

---

## Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.h`

```
<<<<<<< SEARCH
    // UI 组件指针
    QVBoxLayout* m_mainLayout = nullptr;
    class ContentHeaderWidget* m_headerWidget = nullptr;
    QWidget* m_listContainerWidget = nullptr;
    FolderSectionHeaderBar* m_listFolderHeader = nullptr;

    FilterProxyModel* m_proxyModel = nullptr;
=======
    // UI 组件指针
    QVBoxLayout* m_mainLayout = nullptr;
    class ContentHeaderWidget* m_headerWidget = nullptr;
    QWidget* m_listContainerWidget = nullptr;
    FolderSectionHeaderBar* m_listFolderHeader = nullptr;
    FileSectionHeaderBar* m_listFileHeader = nullptr;

    FilterProxyModel* m_proxyModel = nullptr;
>>>>>>> REPLACE
```

---

### 2. `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
void ContentPanel::initListView() {
    m_listContainerWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(m_listContainerWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_listFolderHeader = new FolderSectionHeaderBar(m_listContainerWidget);
    m_listFolderHeader->hide();
    layout->addWidget(m_listFolderHeader);

    m_treeView = new DropTreeView(m_listContainerWidget);
=======
void ContentPanel::initListView() {
    m_listContainerWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(m_listContainerWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_listFolderHeader = new FolderSectionHeaderBar(m_listContainerWidget);
    m_listFolderHeader->hide();
    layout->addWidget(m_listFolderHeader);

    m_listFileHeader = new FileSectionHeaderBar(m_listContainerWidget);
    m_listFileHeader->hide();
    layout->addWidget(m_listFileHeader);

    m_treeView = new DropTreeView(m_listContainerWidget);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
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
=======
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
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. Verify that `QuarkMeta Architecture/Implementation Plan/ContentPanel-36.md` exists and contains Git Merge Diff blocks.
2. Verify header signatures in `FolderSectionWidget.h` match `FileSectionHeaderBar` calls.

---

## SSOT API Reuse & Anti-Redundancy Self-Check

- **`FolderSectionWidget` Component Reuse**: Reuses `FileSectionHeaderBar` from `FolderSectionWidget.h` / `FolderSectionWidget.cpp`.
- **Single Source Model**: Counts records directly from `m_model->allRecords()`.

---

## Header API Signature Verification Table

| File | Class / Struct | Exact Physical Signature | Verification Result |
| :--- | :--- | :--- | :--- |
| `src/ui/FolderSectionWidget.h` | `FileSectionHeaderBar` | `explicit FileSectionHeaderBar(QWidget* parent = nullptr);` | Verified 100% Match |
| `src/ui/FolderSectionWidget.h` | `FileSectionHeaderBar` | `void setCount(int count);` | Verified 100% Match |
