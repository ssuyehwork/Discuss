# Implementation Plan - Step 1: Data Chain Unification & Legacy Cleanup (Single View Fix)

This implementation plan details unifying ContentPanel and ColumnViewPane data chains into a single `m_proxyModel` -> `m_sectionModel` pipeline, eliminating duplicate proxy models and remaining legacy references.

## 1. Overview
Currently, `ContentPanel` contains remnant members (`m_folderProxyModel`, `m_fileProxyModel`, `m_gridFolderProxyModel`, `m_gridFileProxyModel`, etc.).
This step consolidates `ContentPanel` to use exactly one `FilterProxyModel` (`m_proxyModel`) and one `SectionProxyModel` (`m_sectionModel`), which are shared by Grid/Justified (`m_gridView`) and List (`m_treeView`) modes.

In `ColumnViewPane`, individual panes retain a single `m_proxyModel` and `m_sectionProxyModel`.

---

## 2. Modified Files List
1. `src/ui/ContentPanel.h` & `src/ui/ContentPanel.cpp`
2. `src/ui/ColumnViewPane.h` & `src/ui/ColumnViewPane.cpp`
3. `src/ui/controllers/ContentViewCoordinator.h` & `src/ui/controllers/ContentViewCoordinator.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ContentPanel.h`
<<<<<<< SEARCH
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;
    FilterProxyModel* m_listProxyModel = nullptr;
    SectionProxyModel* m_listSectionProxyModel = nullptr;
    FilterProxyModel* m_gridProxyModel = nullptr;
    SectionProxyModel* m_gridSectionProxyModel = nullptr;
=======
    FilterProxyModel* m_proxyModel = nullptr;
    SectionProxyModel* m_sectionModel = nullptr;
>>>>>>> REPLACE

---

### 3.2 `src/ui/ContentPanel.cpp`
<<<<<<< SEARCH
    m_listProxyModel = new FilterProxyModel(this);
    m_listSectionProxyModel = new SectionProxyModel(this);
    m_listSectionProxyModel->setSourceModel(m_listProxyModel);

    m_gridProxyModel = new FilterProxyModel(this);
    m_gridSectionProxyModel = new SectionProxyModel(this);
    m_gridSectionProxyModel->setSourceModel(m_gridProxyModel);
=======
    m_proxyModel = new FilterProxyModel(this);
    m_sectionModel = new SectionProxyModel(this);
    m_sectionModel->setSourceModel(m_proxyModel);

    m_gridView->setModel(m_sectionModel);
    m_gridView->setSectionModel(m_sectionModel);

    m_treeView->setModel(m_sectionModel);
    m_treeView->setSectionModel(m_sectionModel);
>>>>>>> REPLACE

---

### 3.3 `src/ui/ColumnViewPane.h`
<<<<<<< SEARCH
    FilterProxyModel* folderProxyModel() const { return m_proxyModel; }
    FilterProxyModel* fileProxyModel() const { return m_proxyModel; }
    FolderSectionHeaderBar* folderHeader() const { return nullptr; }
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;
=======
    SectionProxyModel* sectionProxyModel() const { return m_sectionProxyModel; }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Apply changes to `ContentPanel.h/.cpp`, `ColumnViewPane.h/.cpp`, `ContentViewCoordinator.h/.cpp`.
2. Compile project using CMake.
3. Verify zero compilation errors and confirm only one `m_proxyModel` and `m_sectionModel` exist in `ContentPanel`.
