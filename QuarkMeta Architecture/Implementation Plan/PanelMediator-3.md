# Implementation Plan - PanelMediator (Equal-Weight Active Panel Focus Routing)

## 1. Overview
Currently, in dual-pane split mode (`splitPane()`), `PanelMediator` binds global controllers (AddressBar, FilterPanel, TitleBar view mode/zoom, SearchController) directly to the primary `ContentPanel` instance (`contentPanel`).
When the user clicks or navigates inside the secondary `ContentPanel`, the AddressBar, FilterPanel, and TitleBar continue to operate solely on the primary pane, creating a weight imbalance where the secondary pane is ignored by top-level controls.

To make Primary and Secondary panels **100% equal in weight**:
1. Add `panelActivated(ContentPanel* panel)` signal to `ContentPanel`, emitted whenever a panel receives a mouse press, focus, or selection event.
2. Maintain `m_activeContentPanel` in `PanelMediator` (defaulting to the primary panel).
3. Whenever `panelActivated(panel)` is emitted by either panel, update `m_activeContentPanel = panel`, sync the AddressBar text to `panel->currentPath()`, populate `FilterPanel` with `panel`'s stats, and route TitleBar view mode / zoom / create item commands directly to `m_activeContentPanel`.

---

## 2. Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/PanelMediator.h`
- `src/ui/PanelMediator.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/ContentPanel.h`
```cpp
<<<<<<< SEARCH
signals:
    void secondaryPaneCreated(ContentPanel* pane);
=======
signals:
    void panelActivated(ContentPanel* panel);
    void secondaryPaneCreated(ContentPanel* pane);
>>>>>>> REPLACE
```

---

### File 2: `src/ui/ContentPanel.cpp`
```cpp
<<<<<<< SEARCH
bool ContentPanel::eventFilter(QObject* obj, QEvent* event) {
=======
bool ContentPanel::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::FocusIn) {
        emit panelActivated(this);
    }
>>>>>>> REPLACE
```

---

### File 3: `src/ui/PanelMediator.h`
```cpp
<<<<<<< SEARCH
    ContentPanel* m_contentPanel = nullptr;
=======
    ContentPanel* m_contentPanel = nullptr;
    ContentPanel* m_activeContentPanel = nullptr;
>>>>>>> REPLACE
```

---

### File 4: `src/ui/PanelMediator.cpp`
Connect `panelActivated` for primary panel and any `secondaryPaneCreated` panels to update `m_activeContentPanel` and route AddressBar, FilterPanel, TitleBar, and Search commands to `m_activeContentPanel`.

---

## 4. Build & Verification Steps
1. Verify clean compilation via CMake.
2. Open dual-pane split view. Click on the secondary pane. Verify that AddressBar breadcrumb updates to the secondary pane's path, FilterPanel reflects secondary pane's file stats, and view mode buttons control the active secondary pane.
