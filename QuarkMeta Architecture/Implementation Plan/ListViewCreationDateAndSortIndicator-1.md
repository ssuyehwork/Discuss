# Implementation Plan - ListViewCreationDateAndSortIndicator-1.md

## 1. Overview
This updated implementation plan refines the List View (`ListView` / `DropTreeView`) column responsiveness thresholds to support **Multi-Pane Split View Mode**:
1. **Multi-Pane Responsive Column Threshold Fix**: In multi-pane split view mode, each individual `ContentPanel` container width is halved (typically ~400px–650px). Setting `minContainerWidth = 850px` for `CreatedDate` caused it to be automatically hidden in multi-pane mode. Adjusting `minContainerWidth` to `480px` for both `CreatedDate` and `ModifiedDate` ensures that the "创建日期" column remains visible and functional across all split panes (Primary and Secondary panes).
2. **Creation Date Column Addition**: Expands list view standard columns from 7 to 8 by adding `FileListColumn::CreatedDate = 7` (placed right after `ModifiedDate = 6`). Column titles and data in `DiskItemModel` use `switch (static_cast<FileListColumn>(...))` without magic numbers, formatting creation time (`ItemRecord::ctime`) as `dd-MM-yyyy HH:mm` (or `"-"` when `ctime == 0`).
3. **Sort Indicator Painting**: Refactors `ContentHeaderView::paintSection` in `DropTreeView.h` to paint a clean custom sort arrow (▲ for ascending, ▼ for descending) using `QPainter` on the currently sorted column (`sortIndicatorSection()`). Non-active column titles remain gray (`#B0B0B0`), while the active sort column title and arrow are highlighted in bright white (`#FFFFFF`). For centered columns, "text + arrow" is drawn as a centered group.
4. **Sort Mapping Centralization**: Consolidates `SortType ↔ FileListColumn` bi-directional conversion into static helper methods `ContentSortController::columnForSortType` and `ContentSortController::sortTypeForColumn`. Duplicate hand-written switches in `ContentPanel.cpp` are replaced with calls to these SSOT functions. Selecting a sort type without a matching header column (e.g. `SortByAddedDate`) clears the header arrow via `header()->setSortIndicator(-1, ...)`.

---

## 2. Modified Files List
- `src/core/ModelContract.h` (Add `CreatedDate = 7` and update `Count = 8` in `FileListColumn`)
- `src/ui/models/DiskItemModel.cpp` (Use `switch (FileListColumn)` in `headerData` and `data`, adding `CreatedDate` and `formatDateTime` helper)
- `src/ui/DropTreeView.h` (Refactor `ContentHeaderView::paintSection` to paint custom sort arrows and highlight active sort column)
- `src/ui/DropTreeView.cpp` (Update `kFileListColumnPolicies` to add `CreatedDate` with `minContainerWidth = 480` for multi-pane split view support)
- `src/ui/controllers/ContentSortController.h` (Declare `columnForSortType` and `sortTypeForColumn` conversion methods)
- `src/ui/controllers/ContentSortController.cpp` (Implement `columnForSortType` and `sortTypeForColumn` conversion methods)
- `src/ui/ContentPanel.cpp` (Unify sort mapping using `ContentSortController`, sync initial sort indicator, remove duplicate `m_columnView->applySort` call)

---

## 3. Detailed Line-by-Line Changes

### Change 1: `src/core/ModelContract.h`
<<<<<<< SEARCH
enum class FileListColumn : int {
    Name = 0,        // 名称 (微卡片 + 文本)
    Status = 1,      // 状态 (固定 40px，默认常态隐藏)
    Rating = 2,      // 评分 (固定 100px)
    Dimension = 3,   // 尺寸 (固定 100px)
    Type = 4,        // 类型 (固定 60px)
    Size = 5,        // 大小 (固定 80px)
    ModifiedDate = 6,// 修改日期 (固定 130px)
    Count = 7
};
=======
enum class FileListColumn : int {
    Name = 0,        // 名称 (微卡片 + 文本)
    Status = 1,      // 状态 (固定 40px，默认常态隐藏)
    Rating = 2,      // 评分 (固定 100px)
    Dimension = 3,   // 尺寸 (固定 100px)
    Type = 4,        // 类型 (固定 60px)
    Size = 5,        // 大小 (固定 80px)
    ModifiedDate = 6,// 修改日期 (固定 130px)
    CreatedDate = 7, // 创建日期 (固定 130px)
    Count = 8
};
>>>>>>> REPLACE

### Change 2: `src/ui/models/DiskItemModel.cpp`
<<<<<<< SEARCH
QVariant DiskItemModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return QString("名称");
            case 1: return QString("状态");
            case 2: return QString("评分");
            case 3: return QString("尺寸");
            case 4: return QString("类型");
            case 5: return QString("大小");
            case 6: return QString("修改日期");
            default: break;
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}
=======
QVariant DiskItemModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (static_cast<FileListColumn>(section)) {
            case FileListColumn::Name: return QString("名称");
            case FileListColumn::Status: return QString("状态");
            case FileListColumn::Rating: return QString("评分");
            case FileListColumn::Dimension: return QString("尺寸");
            case FileListColumn::Type: return QString("类型");
            case FileListColumn::Size: return QString("大小");
            case FileListColumn::ModifiedDate: return QString("修改日期");
            case FileListColumn::CreatedDate: return QString("创建日期");
            default: break;
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
            case 0: {
                int lastSlash = std::max(path.lastIndexOf('\\'), path.lastIndexOf('/'));
                if (lastSlash == -1) return path;
                QString name = path.mid(lastSlash + 1);
                if (name.isEmpty() && path.length() >= 2 && path[1] == ':') return path;
                return name;
            }
            case 3: {
                if (record.isDir) return "-";
                if (record.width > 0 && record.height > 0) {
                    return QString("%1 x %2").arg(record.width).arg(record.height);
                }
                return "-";
            }
            case 4: {
                if (record.isDir) return "文件夹";
                int lastDot = path.lastIndexOf('.');
                return (lastDot != -1) ? path.mid(lastDot + 1).toUpper() : "";
            }
            case 5: {
                if (record.isDir) return "-";
                if (record.size < 1024) return QString::number(record.size) + " B";
                if (record.size < 1024 * 1024) return QString::number(record.size / 1024.0, 'f', 1) + " KB";
                return QString::number(record.size / (1024.0 * 1024.0), 'f', 1) + " MB";
            }
            case 6: {
                return QDateTime::fromMSecsSinceEpoch(record.mtime).toString("dd-MM-yyyy HH:mm");
            }
        }
    }
=======
    static auto formatDateTime = [](long long msecs) -> QString {
        if (msecs <= 0) return "-";
        return QDateTime::fromMSecsSinceEpoch(msecs).toString("dd-MM-yyyy HH:mm");
    };

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (static_cast<FileListColumn>(index.column())) {
            case FileListColumn::Name: {
                int lastSlash = std::max(path.lastIndexOf('\\'), path.lastIndexOf('/'));
                if (lastSlash == -1) return path;
                QString name = path.mid(lastSlash + 1);
                if (name.isEmpty() && path.length() >= 2 && path[1] == ':') return path;
                return name;
            }
            case FileListColumn::Dimension: {
                if (record.isDir) return "-";
                if (record.width > 0 && record.height > 0) {
                    return QString("%1 x %2").arg(record.width).arg(record.height);
                }
                return "-";
            }
            case FileListColumn::Type: {
                if (record.isDir) return "文件夹";
                int lastDot = path.lastIndexOf('.');
                return (lastDot != -1) ? path.mid(lastDot + 1).toUpper() : "";
            }
            case FileListColumn::Size: {
                if (record.isDir) return "-";
                if (record.size < 1024) return QString::number(record.size) + " B";
                if (record.size < 1024 * 1024) return QString::number(record.size / 1024.0, 'f', 1) + " KB";
                return QString::number(record.size / (1024.0 * 1024.0), 'f', 1) + " MB";
            }
            case FileListColumn::ModifiedDate: {
                return formatDateTime(record.mtime);
            }
            case FileListColumn::CreatedDate: {
                return formatDateTime(record.ctime);
            }
            default: break;
        }
    }
>>>>>>> REPLACE

### Change 3: `src/ui/DropTreeView.h`
<<<<<<< SEARCH
    void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override {
        if (!rect.isValid()) return;

        painter->save();
        painter->fillRect(rect, QColor("#252525"));
        painter->setPen(QColor("#333333"));
        painter->drawLine(rect.topRight(), rect.bottomRight());

        QString title = model() ? model()->headerData(logicalIndex, orientation(), Qt::DisplayRole).toString() : QString();
        painter->setPen(QColor("#B0B0B0"));
        painter->setFont(font());

        if (logicalIndex == 0) {
            int textStartX = rect.left() + RowLayoutEngine::calculateHeaderTextStartX(m_zoomLevel);
            QRect textRect = rect;
            textRect.setLeft(textStartX);
            painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, title);
        } else {
            painter->drawText(rect, Qt::AlignCenter, title);
        }
        painter->restore();
    }
=======
    void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override {
        if (!rect.isValid()) return;

        painter->save();
        painter->fillRect(rect, QColor("#252525"));
        painter->setPen(QColor("#333333"));
        painter->drawLine(rect.topRight(), rect.bottomRight());

        QString title = model() ? model()->headerData(logicalIndex, orientation(), Qt::DisplayRole).toString() : QString();
        bool isSorted = (sortIndicatorSection() == logicalIndex && logicalIndex >= 0);
        QColor textColor = isSorted ? QColor("#FFFFFF") : QColor("#B0B0B0");
        painter->setPen(textColor);
        painter->setFont(font());

        QFontMetrics fm(font());
        int textWidth = fm.horizontalAdvance(title);

        if (logicalIndex == 0) {
            int textStartX = rect.left() + RowLayoutEngine::calculateHeaderTextStartX(m_zoomLevel);
            QRect textRect = rect;
            textRect.setLeft(textStartX);
            painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, title);

            if (isSorted) {
                int arrowX = textStartX + textWidth + 6;
                drawSortArrow(painter, arrowX, rect.center().y(), sortIndicatorOrder(), textColor);
            }
        } else {
            if (isSorted) {
                int arrowWidth = 7;
                int gap = 6;
                int totalWidth = textWidth + gap + arrowWidth;
                int startX = rect.left() + (rect.width() - totalWidth) / 2;

                QRect textRect(startX, rect.top(), textWidth + 2, rect.height());
                painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, title);

                int arrowX = startX + textWidth + gap;
                drawSortArrow(painter, arrowX, rect.center().y(), sortIndicatorOrder(), textColor);
            } else {
                painter->drawText(rect, Qt::AlignCenter, title);
            }
        }
        painter->restore();
    }

private:
    static void drawSortArrow(QPainter* painter, int x, int centerY, Qt::SortOrder order, const QColor& color) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(Qt::NoPen);
        painter->setBrush(color);

        QPolygonF triangle;
        if (order == Qt::AscendingOrder) {
            triangle << QPointF(x, centerY + 2.5)
                     << QPointF(x + 7.0, centerY + 2.5)
                     << QPointF(x + 3.5, centerY - 3.5);
        } else {
            triangle << QPointF(x, centerY - 2.5)
                     << QPointF(x + 7.0, centerY - 2.5)
                     << QPointF(x + 3.5, centerY + 3.5);
        }
        painter->drawPolygon(triangle);
        painter->restore();
    }
>>>>>>> REPLACE

### Change 4: `src/ui/DropTreeView.cpp`
<<<<<<< SEARCH
static const std::vector<ColumnPolicy> kFileListColumnPolicies = {
    { FileListColumn::Name,         0,   QHeaderView::Stretch, 0,   false }, // 始终显示并拉伸
    { FileListColumn::Status,       40,  QHeaderView::Fixed,   0,   true  }, // 恒定隐藏
    { FileListColumn::Rating,       100, QHeaderView::Fixed,   350, false }, // >=350px
    { FileListColumn::Dimension,    100, QHeaderView::Fixed,   480, false }, // >=480px
    { FileListColumn::Type,         60,  QHeaderView::Fixed,   600, false }, // >=600px
    { FileListColumn::Size,         80,  QHeaderView::Fixed,   600, false }, // >=600px
    { FileListColumn::ModifiedDate, 130, QHeaderView::Fixed,   720, false }, // >=720px
};
=======
static const std::vector<ColumnPolicy> kFileListColumnPolicies = {
    { FileListColumn::Name,         0,   QHeaderView::Stretch, 0,   false }, // 始终显示并拉伸
    { FileListColumn::Status,       40,  QHeaderView::Fixed,   0,   true  }, // 恒定隐藏
    { FileListColumn::Rating,       100, QHeaderView::Fixed,   350, false }, // >=350px
    { FileListColumn::Dimension,    100, QHeaderView::Fixed,   480, false }, // >=480px
    { FileListColumn::Type,         60,  QHeaderView::Fixed,   480, false }, // >=480px (分栏适应)
    { FileListColumn::Size,         80,  QHeaderView::Fixed,   480, false }, // >=480px (分栏适应)
    { FileListColumn::ModifiedDate, 130, QHeaderView::Fixed,   480, false }, // >=480px (分栏适应)
    { FileListColumn::CreatedDate,  130, QHeaderView::Fixed,   480, false }, // >=480px (分栏适应)
};
>>>>>>> REPLACE

### Change 5: `src/ui/controllers/ContentSortController.h`
<<<<<<< SEARCH
    void applySortToModel(QSortFilterProxyModel* proxyModel);
    void loadFromConfig();
    void saveToConfig();
=======
    static FileListColumn columnForSortType(SortType type);
    static SortType sortTypeForColumn(FileListColumn col);

    void applySortToModel(QSortFilterProxyModel* proxyModel);
    void loadFromConfig();
    void saveToConfig();
>>>>>>> REPLACE

### Change 6: `src/ui/controllers/ContentSortController.cpp`
<<<<<<< SEARCH
void ContentSortController::saveToConfig() {
=======
FileListColumn ContentSortController::columnForSortType(SortType type) {
    switch (type) {
        case SortType::SortByName: return FileListColumn::Name;
        case SortType::SortByRating: return FileListColumn::Rating;
        case SortType::SortByDimension: return FileListColumn::Dimension;
        case SortType::SortByExtension: return FileListColumn::Type;
        case SortType::SortBySize: return FileListColumn::Size;
        case SortType::SortByModifyDate: return FileListColumn::ModifiedDate;
        case SortType::SortByCreateDate: return FileListColumn::CreatedDate;
        case SortType::SortByAddedDate: default: break;
    }
    return static_cast<FileListColumn>(-1);
}

SortType ContentSortController::sortTypeForColumn(FileListColumn col) {
    switch (col) {
        case FileListColumn::Name: return SortType::SortByName;
        case FileListColumn::Rating: return SortType::SortByRating;
        case FileListColumn::Dimension: return SortType::SortByDimension;
        case FileListColumn::Type: return SortType::SortByExtension;
        case FileListColumn::Size: return SortType::SortBySize;
        case FileListColumn::ModifiedDate: return SortType::SortByModifyDate;
        case FileListColumn::CreatedDate: return SortType::SortByCreateDate;
        case FileListColumn::Status: case FileListColumn::Count: default: break;
    }
    return static_cast<SortType>(-1);
}

void ContentSortController::saveToConfig() {
>>>>>>> REPLACE

### Change 7: `src/ui/ContentPanel.cpp`
<<<<<<< SEARCH
    m_sortController = new ContentSortController(this);
    connect(m_sortController, &ContentSortController::sortCriteriaChanged, this, [this](SortType type, Qt::SortOrder order) {
        applySort();
        if (m_treeView && m_treeView->header()) {
            int col = -1;
            switch (type) {
                case SortType::SortByName: col = static_cast<int>(FileListColumn::Name); break;
                case SortType::SortByRating: col = static_cast<int>(FileListColumn::Rating); break;
                case SortType::SortByDimension: col = static_cast<int>(FileListColumn::Dimension); break;
                case SortType::SortByExtension: col = static_cast<int>(FileListColumn::Type); break;
                case SortType::SortBySize: col = static_cast<int>(FileListColumn::Size); break;
                case SortType::SortByModifyDate: col = static_cast<int>(FileListColumn::ModifiedDate); break;
                default: break;
            }
            if (col >= 0) {
                m_treeView->header()->setSortIndicator(col, order);
            }
        }
        if (m_columnView) {
            m_columnView->applySort(static_cast<int>(type), order);
        }
    });
=======
    m_sortController = new ContentSortController(this);
    connect(m_sortController, &ContentSortController::sortCriteriaChanged, this, [this](SortType type, Qt::SortOrder order) {
        applySort();
        if (m_treeView && m_treeView->header()) {
            FileListColumn colEnum = ContentSortController::columnForSortType(type);
            m_treeView->header()->setSortIndicator(static_cast<int>(colEnum), order);
        }
    });
>>>>>>> REPLACE

<<<<<<< SEARCH
    connect(tree->header(), &QHeaderView::sectionClicked, this, [this](int logicalIndex) {
        if (logicalIndex == static_cast<int>(FileListColumn::Status)) return;
        SortType newType = SortType::SortByName;
        switch (static_cast<FileListColumn>(logicalIndex)) {
            case FileListColumn::Name: newType = SortType::SortByName; break;
            case FileListColumn::Rating: newType = SortType::SortByRating; break;
            case FileListColumn::Dimension: newType = SortType::SortByDimension; break;
            case FileListColumn::Type: newType = SortType::SortByExtension; break;
            case FileListColumn::Size: newType = SortType::SortBySize; break;
            case FileListColumn::ModifiedDate: newType = SortType::SortByModifyDate; break;
            default: return;
        }
        if (currentSortType() == newType) {
            setSortOrder(currentSortOrder() == Qt::AscendingOrder ? Qt::DescendingOrder : Qt::AscendingOrder);
        } else {
            setSortCriteria(newType, Qt::AscendingOrder);
        }
    });
}
=======
    connect(tree->header(), &QHeaderView::sectionClicked, this, [this](int logicalIndex) {
        if (logicalIndex == static_cast<int>(FileListColumn::Status)) return;
        FileListColumn col = static_cast<FileListColumn>(logicalIndex);
        SortType newType = ContentSortController::sortTypeForColumn(col);
        if (static_cast<int>(newType) < 0) return;

        if (currentSortType() == newType) {
            setSortOrder(currentSortOrder() == Qt::AscendingOrder ? Qt::DescendingOrder : Qt::AscendingOrder);
        } else {
            setSortCriteria(newType, Qt::AscendingOrder);
        }
    });

    if (m_sortController && tree->header()) {
        FileListColumn initCol = ContentSortController::columnForSortType(m_sortController->sortType());
        tree->header()->setSortIndicator(static_cast<int>(initCol), m_sortController->sortOrder());
    }
}
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Clean build directory and compile using CMake & Ninja / MSVC:
   ```bash
   cmake -B build -G Ninja
   cmake --build build --config Release
   ```
2. Verify Creation Date column display in single pane vs multi-pane split view mode:
   - In single-pane mode (width ≥ 480px), verify "创建日期" header column is displayed after "修改日期".
   - Split window into multi-pane view mode (Primary & Secondary panes, pane width ~480px–650px).
   - Verify "创建日期" and "修改日期" header columns remain visible in BOTH primary and secondary split panes.
3. Verify sort indicator arrows:
   - Click "创建日期" header column: verify ▲ arrow appears and active column text turns white (`#FFFFFF`).
   - Click again: verify ▼ arrow flips direction.
   - Switch sort type to "添加日期" (no header column): verify header arrow is cleared (`setSortIndicator(-1, ...)`).

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Multi-Pane Responsive Adaptation**: Adjusted `kFileListColumnPolicies` in `DropTreeView.cpp` to use `minContainerWidth = 480px` for date columns, avoiding premature hiding when split panes shrink.
- **Sort Mapping SSOT**: Centralized `SortType ↔ FileListColumn` mapping in `ContentSortController::columnForSortType` and `ContentSortController::sortTypeForColumn`.

---

## 6. Header API Signature Verification
- `static FileListColumn ContentSortController::columnForSortType(SortType type)`
- `static SortType ContentSortController::sortTypeForColumn(FileListColumn col)`
- `void QHeaderView::setSortIndicator(int logicalIndex, Qt::SortOrder order)`

---

## 7. Header Inclusion Chain & Type Completeness Check
- `DropTreeView.cpp` includes `DropTreeView.h` and `ModelContract.h`.
- `ContentPanel.cpp` includes `ContentSortController.h` and `ModelContract.h`.
