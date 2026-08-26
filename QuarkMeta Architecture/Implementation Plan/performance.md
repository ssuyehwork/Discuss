# Implementation Plan - Performance Optimization for Selection Change & Large Dataset Ops

## 1. Overview
When performing "Select All" (Ctrl+A) or "Deselect All" (Ctrl+D / Esc) on folders containing 5,000+ items in `ContentPanel`, the UI experiences severe lag (3~5s freezing).
This plan addresses the root causes:
- Excessive signal generation and deep-copying `QStringList` containing thousands of paths across UI threads.
- Lack of debouncing when selection changes rapid fire.
- Redundant selection index lookups across mediators and status updates.

To resolve this:
1. Introduce a 30ms debouncing timer `m_selectionDebounceTimer` in `ContentPanel` to batch rapid selection signals into a single update.
2. Introduce a circuit-breaker threshold (50 items) in `ContentPanel::emitSelectionChangedSignal()`: when selection size exceeds 50, avoid deep copying thousands of string paths and only pass the first item path for `MetaPanel` preview.
3. Eliminate duplicate `getSelectedIndexes()` calls in `PanelMediator.cpp`.

## 2. Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/PanelMediator.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ContentPanel.h`
Add `QTimer* m_selectionDebounceTimer = nullptr;` and private slot `onSelectionDebounced()`.

```
<<<<<<< SEARCH
    QPoint m_dragStartPosition;
    bool m_isInternalDrag = false;

    void updateStatusBarSelectionStats();
=======
    QPoint m_dragStartPosition;
    bool m_isInternalDrag = false;

    QTimer* m_selectionDebounceTimer = nullptr;
    void emitSelectionChangedSignal();

    void updateStatusBarSelectionStats();
>>>>>>> REPLACE
```

### 3.2 `src/ui/ContentPanel.cpp`
Initialize `m_selectionDebounceTimer` in `ContentPanel` constructor and update `onSelectionChanged()` to restart timer, plus implement `emitSelectionChangedSignal()`.

```
<<<<<<< SEARCH
ContentPanel::ContentPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}
=======
ContentPanel::ContentPanel(QWidget *parent)
    : QWidget(parent)
{
    m_selectionDebounceTimer = new QTimer(this);
    m_selectionDebounceTimer->setSingleShot(true);
    m_selectionDebounceTimer->setInterval(30);
    connect(m_selectionDebounceTimer, &QTimer::timeout, this, &ContentPanel::emitSelectionChangedSignal);

    setupUI();
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(selected);
    Q_UNUSED(deselected);

    QList<QModelIndex> indexes = getSelectedIndexes();
    QStringList paths;
    for (const auto &idx : indexes) {
        if (idx.isValid()) {
            paths.append(idx.data(Qt::UserRole + 1).toString());
        }
    }

    emit selectionChanged(paths);
    updateStatusBarSelectionStats();
}
=======
void ContentPanel::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(selected);
    Q_UNUSED(deselected);

    if (m_selectionDebounceTimer) {
        m_selectionDebounceTimer->start();
    } else {
        emitSelectionChangedSignal();
    }
}

void ContentPanel::emitSelectionChangedSignal()
{
    QList<QModelIndex> indexes = getSelectedIndexes();
    QStringList paths;

    // Circuit breaker for large selection (> 50 items) to prevent memory & UI thread freeze
    if (indexes.size() > 50) {
        if (!indexes.isEmpty() && indexes.first().isValid()) {
            paths.append(indexes.first().data(Qt::UserRole + 1).toString());
        }
    } else {
        paths.reserve(indexes.size());
        for (const auto &idx : indexes) {
            if (idx.isValid()) {
                paths.append(idx.data(Qt::UserRole + 1).toString());
            }
        }
    }

    emit selectionChanged(paths);
    updateStatusBarSelectionStats();
}
>>>>>>> REPLACE
```

### 3.3 `src/ui/PanelMediator.cpp`
Avoid redundant `getSelectedIndexes()` in `onContentSelectionChanged`.

```
<<<<<<< SEARCH
void PanelMediator::onContentSelectionChanged(const QStringList &selectedPaths)
{
    if (!m_metaPanel || !m_contentPanel) return;

    QList<QModelIndex> indexes = m_contentPanel->getSelectedIndexes();
    if (indexes.isEmpty()) {
        m_metaPanel->clear();
        return;
    }

    QModelIndex firstIdx = indexes.first();
    QString path = firstIdx.data(Qt::UserRole + 1).toString();
    ItemRecord rec = m_contentPanel->getItemRecord(firstIdx);

    m_metaPanel->setItem(rec);
}
=======
void PanelMediator::onContentSelectionChanged(const QStringList &selectedPaths)
{
    if (!m_metaPanel || !m_contentPanel) return;

    if (selectedPaths.isEmpty()) {
        m_metaPanel->clear();
        return;
    }

    QString firstPath = selectedPaths.first();
    ItemRecord rec = m_contentPanel->getItemRecordByPath(firstPath);
    if (!rec.filePath.isEmpty()) {
        m_metaPanel->setItem(rec);
    } else {
        QList<QModelIndex> indexes = m_contentPanel->getSelectedIndexes();
        if (!indexes.isEmpty() && indexes.first().isValid()) {
            m_metaPanel->setItem(m_contentPanel->getItemRecord(indexes.first()));
        } else {
            m_metaPanel->clear();
        }
    }
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build the target with `cmake --build build` or `ninja -C build`.
2. Verify zero warnings and errors during compilation.
3. Test Ctrl+A / Select All and Esc / Deselect All on 5000+ items to confirm immediate 0ms UI responsiveness.
