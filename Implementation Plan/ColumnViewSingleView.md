# Implementation Plan - Step 4: Single View Transformation for ColumnViewPane (Column View)

This plan details transforming `ColumnViewPane` (in Column View mode) to replace the dual `folderListView` + `listView` split with a single `DropListView` attached to `SectionProxyModel`.

## 1. Overview
Currently, `ColumnViewPane` relies on `DualSectionPanel` (`m_panel`) holding two `DropListView` instances (`m_folderListView` and `m_listView`), backed by `m_folderProxyModel` and `m_fileProxyModel`.
This refactoring removes `DualSectionPanel`, `m_folderListView`, `m_folderProxyModel`, and `m_fileProxyModel` from `ColumnViewPane`.
A single `DropListView` (`m_listView`) is hosted inside `ColumnViewPane`, backed by a `FilterProxyModel` (`m_proxyModel`) and a `SectionProxyModel` (`m_sectionProxyModel`).

Key behavior retention:
- Fixed column width (230px) and right-side blank canvas behavior remain unchanged.
- Single-click folder expansion, double-click actions, parent column highlights (`IsParentExpandedRole`), and blank space double-click handling remain intact.
- Multi-selection (Ctrl+A, Ctrl+Click, Shift+Click) operates across folders and files within the single view.
- Section headers for folders and files are non-selectable and non-draggable.

---

## 2. Modified Files List
1. `src/ui/ColumnViewPane.h` (Update class members and remove dual view getters)
2. `src/ui/ColumnViewPane.cpp` (Refactor layout to single `DropListView`, bind `SectionProxyModel`, handle folder collapse)
3. `src/ui/ColumnViewWidget.h` & `src/ui/ColumnViewWidget.cpp` (Update selection and path traversal methods for single view)
4. `src/ui/controllers/ContentContextMenu.cpp` (Update context menu targeting for `ColumnViewPane`)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ColumnViewPane.h`
<<<<<<< SEARCH
    DropListView* listView() const;
    DropListView* folderListView() const;
    FilterProxyModel* proxyModel() const { return m_proxyModel; }
    FilterProxyModel* folderProxyModel() const { return m_folderProxyModel; }
    FilterProxyModel* fileProxyModel() const { return m_fileProxyModel; }
    DiskItemModel* model() const { return m_model; }
    FolderSectionHeaderBar* folderHeader() const;
=======
    DropListView* listView() const { return m_listView; }
    FilterProxyModel* proxyModel() const { return m_proxyModel; }
    SectionProxyModel* sectionProxyModel() const { return m_sectionProxyModel; }
    DiskItemModel* model() const { return m_model; }
>>>>>>> REPLACE

<<<<<<< SEARCH
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;
    QScrollArea* m_paneScrollArea = nullptr;
    DualSectionPanel* m_panel = nullptr;
    DropListView* m_folderListView = nullptr;
    DropListView* m_listView = nullptr;
=======
    SectionProxyModel* m_sectionProxyModel = nullptr;
    DropListView* m_listView = nullptr;
>>>>>>> REPLACE

---

### 3.2 `src/ui/ColumnViewPane.cpp`
<<<<<<< SEARCH
    // DualSectionPanel initialization in ColumnViewPane constructor
=======
ColumnViewPane::ColumnViewPane(const QString& path, ContentPanel* contentPanel, QWidget* parent)
    : QWidget(parent), m_path(path), m_contentPanel(contentPanel) {
    setObjectName("ColumnViewPane");
    setFixedWidth(230);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_model = new DiskItemModel(this);
    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);

    m_sectionProxyModel = new SectionProxyModel(this);
    m_sectionProxyModel->setSourceModel(m_proxyModel);

    m_listView = new DropListView(this);
    m_listView->setObjectName("ColumnViewPaneListView");
    m_listView->setModel(m_sectionProxyModel);
    m_listView->setItemDelegate(new ColumnItemDelegate(m_listView));
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    layout->addWidget(m_listView);
}
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Apply changes to `ColumnViewPane.h/.cpp` and `ColumnViewWidget.h/.cpp`.
2. Compile project using CMake.
3. Verify Column View mode creates 230px column panes each hosting a single `DropListView`.
4. Test cascading column expansion on folder clicks and verify section headers do not break selection or column navigation.
