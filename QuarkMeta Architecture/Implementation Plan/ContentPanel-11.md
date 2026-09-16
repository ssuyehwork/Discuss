# ContentPanel-11.md: Active Pane Index Normalization & Selection Hijacking Prevention

## 1. Overview
Runtime diagnostics confirmed the exact sequence causing Column View multi-selection loss:
1. When switching to Column View, the rightmost pane (index 2) asynchronously finishes scanning and selects 39 Markdown files.
2. Shortly after, parent directory column (index 1) finishes its asynchronous scan and programmatically highlights the parent directory item (`selectItemByPath("H:/测试/测试-删除")`).
3. The parent column emits `selectionChanged()`, which executed `m_activePaneIndex = pane->property("paneIndex").toInt()`.
4. This hijacked `m_activePaneIndex` to 1 (the parent column), causing `activePane()` and `getSelectedPaths()` to query pane 1 (containing only the parent folder) instead of pane 2 (containing the 39 selected Markdown files).

This implementation plan resolves the active pane index hijacking bug by:
1. Ensuring `m_activePaneIndex` is updated during `selectionChanged()` **only if** the selection signal originated from user interaction or from the rightmost pane (`pane == rightmostPane()`).
2. Cleaning up temporary `qDebug()` log statements from `ColumnViewWidget.cpp` and `ContentPanel.cpp`.

---

## 2. Modified Files List
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/ColumnViewWidget.cpp`

<<<<<<< SEARCH
void ColumnViewPane::selectItemByPath(const QString& targetPath) {
    m_pendingSelectPath = targetPath;
    if (m_listView && m_listView->selectionModel()) {
        m_listView->selectionModel()->blockSignals(true);
        tryPendingSelection();
        m_listView->selectionModel()->blockSignals(false);
    } else {
        tryPendingSelection();
    }
}

void ColumnViewPane::setPendingSelectNames(const QSet<QString>& names) {
    m_pendingSelectNames = names;
    qDebug() << "[ColumnViewPane Debug] setPendingSelectNames count:" << names.size() << "path:" << m_path;
    tryPendingSelection();
}
=======
void ColumnViewPane::selectItemByPath(const QString& targetPath) {
    m_pendingSelectPath = targetPath;
    tryPendingSelection();
}

void ColumnViewPane::setPendingSelectNames(const QSet<QString>& names) {
    m_pendingSelectNames = names;
    tryPendingSelection();
}
>>>>>>> REPLACE

<<<<<<< SEARCH
void ColumnViewPane::tryPendingSelection() {
    if (!m_proxyModel || !m_listView) return;

    qDebug() << "[ColumnViewPane Debug] tryPendingSelection rowCount:" << m_proxyModel->rowCount()
             << "pendingNames:" << m_pendingSelectNames.size()
             << "pendingPath:" << m_pendingSelectPath;

    if (!m_pendingSelectNames.isEmpty() && m_proxyModel->rowCount() > 0) {
=======
void ColumnViewPane::tryPendingSelection() {
    if (!m_proxyModel || !m_listView) return;

    if (!m_pendingSelectNames.isEmpty() && m_proxyModel->rowCount() > 0) {
>>>>>>> REPLACE

<<<<<<< SEARCH
                if (!weakSelf->m_pendingSelectNames.isEmpty()) {
                    qDebug() << "[ColumnViewPane Debug] Async load finished, triggering tryPendingSelection";
                    weakSelf->tryPendingSelection();
                }
=======
                if (!weakSelf->m_pendingSelectNames.isEmpty()) {
                    weakSelf->tryPendingSelection();
                }
>>>>>>> REPLACE

<<<<<<< SEARCH
    connect(pane, &ColumnViewPane::selectionChanged, this, [this, pane]() {
        m_activePaneIndex = pane->property("paneIndex").toInt();
        emit selectionChanged();
        if (rightmostPane() && rightmostPane()->model()) {
            emit activeColumnRecordsChanged(rightmostPane()->model()->allRecords());
        }
    });
=======
    connect(pane, &ColumnViewPane::selectionChanged, this, [this, pane]() {
        if (pane == rightmostPane() || (pane->listView() && pane->listView()->hasFocus())) {
            m_activePaneIndex = pane->property("paneIndex").toInt();
        }
        emit selectionChanged();
        if (rightmostPane() && rightmostPane()->model()) {
            emit activeColumnRecordsChanged(rightmostPane()->model()->allRecords());
        }
    });
>>>>>>> REPLACE

---

### File 2: `src/ui/ContentPanel.cpp`

<<<<<<< SEARCH
    if (m_currentViewMode == ColumnView) {
        qDebug() << "[ContentPanel Debug] restoreSelections in ColumnView, count:" << m_pendingSelectNames.size();
        if (m_columnView && m_columnView->rightmostPane()) {
            m_columnView->rightmostPane()->setPendingSelectNames(m_pendingSelectNames);
        }
        m_pendingSelectNames.clear();
        return;
    }
=======
    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->rightmostPane()) {
            m_columnView->rightmostPane()->setPendingSelectNames(m_pendingSelectNames);
        }
        m_pendingSelectNames.clear();
        return;
    }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps

### Verification Commands
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Verification Checklist
1. Select 39 Markdown files in Grid View or List View.
2. Switch to Column View (`ColumnView`).
3. Verify that parent directory asynchronous loading does NOT hijack `m_activePaneIndex`.
4. Verify that MetaPanel and ContentPanel accurately show 39 files selected.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Prevents active pane index mutation when non-focused parent columns emit background selection signals.
- Preserves `rightmostPane()` as the SSOT active pane during view navigation.

---

## 6. Header API Signature Verification
- `ColumnViewWidget::rightmostPane()` -> `src/ui/ColumnViewWidget.h`
- `ColumnViewWidget::activePane()` -> `src/ui/ColumnViewWidget.h`
