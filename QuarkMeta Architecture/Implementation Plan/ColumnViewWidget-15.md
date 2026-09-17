# ColumnViewWidget-15.md: Blue Section Headers & Overflow Blue Indicator Line

## 1. Overview
This update implements the UI styling refinements requested for section header bars and Column View folder overflow handling:
1. **Blue Collapsible Section Headers**: Updated `FolderSectionHeaderBar` and `FileSectionHeaderBar` label color and SVG arrow icon color from gray (`#CCCCCC`/`#888888`) to QuarkMeta brand blue (`#3498db`).
2. **Column View Folder Overflow Indicator**: In `ColumnViewPane`, overridden `paintEvent` and `resizeEvent` to check if `m_folderListView` is visible and its bottom edge (`m_folderListView->y() + m_folderListView->height()`) reaches or exceeds the column height (`height()`). If it overflows, a 1-pixel blue indicator line (`#3498db`) is drawn along the bottom edge of the pane; when there is no overflow, the line is not drawn.

---

## 2. Modified Files List
- `src/ui/FolderSectionWidget.cpp`
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/FolderSectionWidget.cpp`

```
<<<<<<< SEARCH
    m_titleLabel->setStyleSheet("color: #CCCCCC; font-size: 12px; font-weight: bold;");
=======
    m_titleLabel->setStyleSheet("color: #3498db; font-size: 12px; font-weight: bold;");
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    if (m_arrowLabel) {
        m_arrowLabel->setPixmap(UiHelper::getIcon(m_collapsed ? "chevron_right" : "chevron_down", QColor("#888888"), 12).pixmap(12, 12));
    }
=======
    if (m_arrowLabel) {
        m_arrowLabel->setPixmap(UiHelper::getIcon(m_collapsed ? "chevron_right" : "chevron_down", QColor("#3498db"), 12).pixmap(12, 12));
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_titleLabel = new QLabel("文件 (0)", this);
    m_titleLabel->setStyleSheet("color: #888888; font-size: 12px; font-weight: bold;");
=======
    m_titleLabel = new QLabel("文件 (0)", this);
    m_titleLabel->setStyleSheet("color: #3498db; font-size: 12px; font-weight: bold;");
>>>>>>> REPLACE
```

---

### File 2: `src/ui/ColumnViewWidget.h`

```
<<<<<<< SEARCH
signals:
    void folderSelected(const QString& folderPath, int paneIndex);
    void fileSelected(const QString& filePath, int paneIndex);
    void selectionChanged();
    void recordsLoaded(const std::vector<ItemRecord>& records);
    void blankSpaceDoubleClicked(int paneIndex);

private slots:
    void tryPendingSelection();
=======
signals:
    void folderSelected(const QString& folderPath, int paneIndex);
    void fileSelected(const QString& filePath, int paneIndex);
    void selectionChanged();
    void recordsLoaded(const std::vector<ItemRecord>& records);
    void blankSpaceDoubleClicked(int paneIndex);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void tryPendingSelection();
>>>>>>> REPLACE
```

---

### File 3: `src/ui/ColumnViewWidget.cpp`

```
<<<<<<< SEARCH
void ColumnViewPane::setFilterState(const FilterState& state) {
=======
void ColumnViewPane::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
    if (m_folderListView && m_folderListView->isVisible()) {
        int folderBottom = m_folderListView->y() + m_folderListView->height();
        if (folderBottom >= height()) {
            QPainter painter(this);
            painter.setPen(QPen(QColor("#3498db"), 1));
            painter.drawLine(0, height() - 1, width(), height() - 1);
        }
    }
}

void ColumnViewPane::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    update();
}

void ColumnViewPane::setFilterState(const FilterState& state) {
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### Verification Commands
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Verification Checklist
1. Open QuarkMeta and check collapsible section headers ("文件夹 (N)", "文件 (N)") in Grid/List/Column views to confirm text and expand/collapse icons are rendered in blue (`#3498db`).
2. Switch to Column View and open a folder containing a small number of subfolders (does not exceed column height). Verify that NO blue line appears at the bottom.
3. Open a folder containing a large number of subfolders (exceeding column height). Verify that a 1-pixel blue line (`#3498db`) is drawn at the bottom edge of the column pane.
