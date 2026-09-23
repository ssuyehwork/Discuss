# Implementation Plan - ViewSectionHeaderNormalization.md

## 1. Overview
This implementation plan normalizes view section headers across all view modes in QuarkMeta (`DropTreeView`, `DropListView`, `JustifiedView`, and `ColumnViewWidget`):
1. **Unified Text**: Unify folder header label to `文件夹 (N)` across all views, eliminating `子文件夹 (N)` inconsistency.
2. **Permanent Expansion for Files**: Remove folding arrow (`▼`/`▶`) and click-collapse interaction for the file section header (`文件 (N)`). Keep it as a static header text, remaining permanently expanded.
3. **Eliminate Row Overlay Bug via Streamlined Offset**:
   - Enable `TreeItemDelegate`'s built-in 26px padding offset when rendering first item in section across all columns.
   - Remove overlay drawing (`fillRect("#222222")`) in `DropTreeView::paintEvent` and `DropListView::paintEvent` that previously obscured list items.
   - Ensure items flow naturally without overlap or clipped text.
4. **Adaptive Text Width & Transparent Background**:
   - Replace full-width background bar fill (`#222222`) with transparent background (`Qt::transparent`).
   - Draw text within adaptive capsule width calculated via `QFontMetrics::horizontalAdvance`.
5. **Architectural Responsibilities**:
   - `TreeItemDelegate`: Handles inline item section headers for `DropTreeView` and `DropListView`.
   - `ColumnViewWidget`: Uses top layout widget (`m_folderHeaderLabel`) for section headers. `ColumnItemDelegate` stays lean without `m_enableSectionHeaders` flags, preventing MSVC `C2511`/`C2065`/`C2550` compiler signature mismatches.

## 2. Modified Files List
- `src/ui/ContentPanel.cpp`
- `src/ui/TreeItemDelegate.h`
- `src/ui/DropTreeView.cpp`
- `src/ui/DropListView.cpp`
- `src/ui/JustifiedView.cpp`
- `src/ui/ColumnViewWidget.cpp`

## 3. Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.cpp`
Enable `enableSectionHeaders = true` in `TreeItemDelegate`.

```
<<<<<<< SEARCH
    m_treeView->setItemDelegate(new TreeItemDelegate(this, true, true));
=======
    m_treeView->setItemDelegate(new TreeItemDelegate(this, true, true, true));
>>>>>>> REPLACE
```

### 2. `src/ui/TreeItemDelegate.h`
Adjust `paint()` to apply vertical 26px shift across all columns for header rows, draw `文件 (N)` without collapsible arrows, and calculate text width dynamically.

```
<<<<<<< SEARCH
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        if (!index.isValid()) return;

        // ── Section Header 内嵌绘制 ──────────────────────────────────────────────
        QStyleOptionViewItem opt = option; // 提前声明，后续统一使用
        if (m_enableSectionHeaders && index.column() == 0) {
            const QAbstractItemModel* m = index.model();
            bool isFolder = (index.data(TypeRole).toString() == "folder");
            bool isSectionFirstRow = false;
            QString sectionText;

            if (isFolder && index.row() == 0) {
                // 计算文件夹总数
                int folderCount = 0, fileFirstRow = -1;
                countSections(m, folderCount, fileFirstRow);
                isSectionFirstRow = true;
                sectionText = QString("▼  文件夹 (%1)").arg(folderCount);
            } else if (!isFolder && m) {
                bool prevIsFolder = (index.row() > 0)
                    ? (m->index(index.row() - 1, 0).data(TypeRole).toString() == "folder")
                    : true;
                if (prevIsFolder) {
                    // 计算文件总数
                    int total = m->rowCount();
                    int fileCount = 0;
                    for (int i = index.row(); i < total; ++i) {
                        if (m->index(i, 0).data(TypeRole).toString() != "folder") fileCount++;
                    }
                    isSectionFirstRow = true;
                    sectionText = QString("文件 (%1)").arg(fileCount);
                }
            }

            if (isSectionFirstRow) {
                // 上方 26px：透明背景，绘制分组标题
                QRect headerArea(option.rect.left(), option.rect.top(), option.rect.width(), 26);
                QRect contentArea(option.rect.left(), option.rect.top() + 26,
                                  option.rect.width(), option.rect.height() - 26);

                painter->save();
                QFont headerFont("Microsoft YaHei", 9, QFont::Bold);
                painter->setFont(headerFont);
                painter->setPen(QColor("#A0A0A0"));
                QFontMetrics fm(headerFont);
                int textW = fm.horizontalAdvance(sectionText) + 16;
                QRect textRect(headerArea.left() + 8, headerArea.top(), textW, headerArea.height());
                painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, sectionText);
                painter->restore();

                // 将 opt.rect 收缩到内容区域，后续所有绘制都在内容区域进行
                opt.rect = contentArea;
            }
        }
        // ── End Section Header ───────────────────────────────────────────────────
=======
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        if (!index.isValid()) return;

        // ── Section Header 内嵌绘制与多列统一向下偏移 ──────────────────────────────
        QStyleOptionViewItem opt = option;
        if (m_enableSectionHeaders) {
            const QAbstractItemModel* m = index.model();
            bool isFolder = (index.data(TypeRole).toString() == "folder");
            bool hasHeaderAbove = false;
            QString sectionText;

            if (isFolder && index.row() == 0) {
                hasHeaderAbove = true;
                if (index.column() == 0) {
                    int folderCount = 0, fileFirstRow = -1;
                    countSections(m, folderCount, fileFirstRow);
                    sectionText = QString("▼  文件夹 (%1)").arg(folderCount);
                }
            } else if (!isFolder && m) {
                bool prevIsFolder = (index.row() > 0)
                    ? (m->index(index.row() - 1, 0).data(TypeRole).toString() == "folder")
                    : true;
                if (prevIsFolder) {
                    hasHeaderAbove = true;
                    if (index.column() == 0) {
                        int total = m->rowCount();
                        int fileCount = 0;
                        for (int i = index.row(); i < total; ++i) {
                            if (m->index(i, 0).data(TypeRole).toString() != "folder") fileCount++;
                        }
                        sectionText = QString("文件 (%1)").arg(fileCount);
                    }
                }
            }

            if (hasHeaderAbove) {
                // 上方 26px 腾空为 Header 区域，所有列的内容统一避让下沉 26px
                QRect headerArea(option.rect.left(), option.rect.top(), option.rect.width(), 26);
                opt.rect = QRect(option.rect.left(), option.rect.top() + 26,
                                 option.rect.width(), option.rect.height() - 26);

                if (index.column() == 0 && !sectionText.isEmpty()) {
                    painter->save();
                    QFont headerFont("Microsoft YaHei", 9, QFont::Bold);
                    painter->setFont(headerFont);
                    painter->setPen(QColor("#A0A0A0"));
                    QFontMetrics fm(headerFont);
                    int textW = fm.horizontalAdvance(sectionText) + 8;
                    QRect textRect(headerArea.left() + 8, headerArea.top(), textW, headerArea.height());
                    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, sectionText);
                    painter->restore();
                }
            }
        }
        // ── End Section Header ───────────────────────────────────────────────────
>>>>>>> REPLACE
```

### 3. `src/ui/DropTreeView.cpp`
Remove overlay header fill block from `paintEvent`.

```
<<<<<<< SEARCH
void DropTreeView::paintEvent(QPaintEvent* event) {
    updateFolderHiding();

    QTreeView::paintEvent(event);

    if (m_folderCount > 0) {
        QPainter painter(viewport());
        painter.save();

        m_folderHeaderRect = QRect(0, 0, viewport()->width(), 26);
        painter.fillRect(m_folderHeaderRect, QColor("#222222"));

        painter.setPen(QColor("#A0A0A0"));
        QFont headerFont("Microsoft YaHei", 9, QFont::Bold);
        painter.setFont(headerFont);

        QString arrow = m_foldersCollapsed ? "▶" : "▼";
        QString headerText = QString("  %1  文件夹 (%2)").arg(arrow).arg(m_folderCount);
        painter.drawText(m_folderHeaderRect, Qt::AlignLeft | Qt::AlignVCenter, headerText);

        painter.restore();
    }

    if (!m_emptyHint.isEmpty() && model() && model()->rowCount() == 0) {
        QPainter painter(viewport());
        painter.save();
        painter.setPen(QColor("#888888"));
        painter.setFont(QFont("Microsoft YaHei", 12));
        painter.drawText(viewport()->rect(), Qt::AlignCenter, m_emptyHint);
        painter.restore();
    }
}
=======
void DropTreeView::paintEvent(QPaintEvent* event) {
    updateFolderHiding();

    QTreeView::paintEvent(event);

    if (!m_emptyHint.isEmpty() && model() && model()->rowCount() == 0) {
        QPainter painter(viewport());
        painter.save();
        painter.setPen(QColor("#888888"));
        painter.setFont(QFont("Microsoft YaHei", 12));
        painter.drawText(viewport()->rect(), Qt::AlignCenter, m_emptyHint);
        painter.restore();
    }
}
>>>>>>> REPLACE
```

### 4. `src/ui/DropListView.cpp`
Remove overlay header block from `DropListView::paintEvent`.

```
<<<<<<< SEARCH
void DropListView::paintEvent(QPaintEvent* event) {
    updateFolderHiding();

    QListView::paintEvent(event);

    if (m_folderCount > 0) {
        QPainter painter(viewport());
        painter.save();

        m_folderHeaderRect = QRect(0, 0, viewport()->width(), 26);
        painter.fillRect(m_folderHeaderRect, QColor("#222222"));

        painter.setPen(QColor("#A0A0A0"));
        QFont headerFont("Microsoft YaHei", 9, QFont::Bold);
        painter.setFont(headerFont);

        QString arrow = m_foldersCollapsed ? "▶" : "▼";
        QString headerText = QString("  %1  文件夹 (%2)").arg(arrow).arg(m_folderCount);
        painter.drawText(m_folderHeaderRect, Qt::AlignLeft | Qt::AlignVCenter, headerText);

        painter.restore();
    }
}
=======
void DropListView::paintEvent(QPaintEvent* event) {
    updateFolderHiding();
    QListView::paintEvent(event);
}
>>>>>>> REPLACE
```

### 5. `src/ui/ColumnViewWidget.cpp`
Update header text to `文件夹 (N)`, transparent background, and adaptive size policy. Do NOT modify `ColumnItemDelegate` signatures or constructor parameters.

```
<<<<<<< SEARCH
    m_folderHeaderLabel = new QLabel(this);
    m_folderHeaderLabel->setStyleSheet("color: #DDDDDD; font-weight: bold; font-size: 12px; padding: 4px 8px;");
    m_folderHeaderLabel->setCursor(Qt::PointingHandCursor);
    m_folderHeaderLabel->hide();
    m_folderHeaderLabel->installEventFilter(this);
    layout->addWidget(m_folderHeaderLabel);
=======
    m_folderHeaderLabel = new QLabel(this);
    m_folderHeaderLabel->setStyleSheet("color: #A0A0A0; font-weight: bold; font-size: 12px; padding: 4px 8px; background: transparent;");
    m_folderHeaderLabel->setCursor(Qt::PointingHandCursor);
    m_folderHeaderLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    m_folderHeaderLabel->hide();
    m_folderHeaderLabel->installEventFilter(this);
    layout->addWidget(m_folderHeaderLabel);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        if (folderCount > 0) {
            m_folderHeaderLabel->setText(QString("子文件夹 (%1) %2").arg(folderCount).arg(m_foldersCollapsed ? "▶" : "▼"));
            m_folderHeaderLabel->show();
        } else {
            m_folderHeaderLabel->hide();
        }
=======
        if (folderCount > 0) {
            m_folderHeaderLabel->setText(QString("%1  文件夹 (%2)").arg(m_foldersCollapsed ? "▶" : "▼").arg(folderCount));
            m_folderHeaderLabel->show();
        } else {
            m_folderHeaderLabel->hide();
        }
>>>>>>> REPLACE
```

### 6. `src/ui/JustifiedView.cpp`
Update `JustifiedView` header rendering to remove background fills, use adaptive text bounds, and remove file collapsing interaction.

```
<<<<<<< SEARCH
    if (m_folderCount > 0 && !m_folderHeaderRect.isEmpty()) {
        painter.save();
        painter.translate(0, -scrollY);

        QRect headerRect = m_folderHeaderRect;
        painter.fillRect(headerRect, QColor("#222222"));

        painter.setPen(QColor("#A0A0A0"));
        QFont headerFont("Microsoft YaHei", 9, QFont::Bold);
        painter.setFont(headerFont);

        QString arrow = m_foldersCollapsed ? "▶" : "▼";
        QString headerText = QString("  %1  文件夹 (%2)").arg(arrow).arg(m_folderCount);
        painter.drawText(headerRect, Qt::AlignLeft | Qt::AlignVCenter, headerText);

        painter.restore();
    }

    if (m_fileCount > 0 && !m_fileHeaderRect.isEmpty()) {
        painter.save();
        painter.translate(0, -scrollY);

        QRect headerRect = m_fileHeaderRect;
        painter.fillRect(headerRect, QColor("#222222"));

        painter.setPen(QColor("#A0A0A0"));
        QFont headerFont("Microsoft YaHei", 9, QFont::Bold);
        painter.setFont(headerFont);

        QString arrow = m_filesCollapsed ? "▶" : "▼";
        QString headerText = QString("  %1  文件 (%2)").arg(arrow).arg(m_fileCount);
        painter.drawText(headerText, Qt::AlignLeft | Qt::AlignVCenter, headerText);

        painter.restore();
    }
=======
    if (m_folderCount > 0 && !m_folderHeaderRect.isEmpty()) {
        painter.save();
        painter.translate(0, -scrollY);

        painter.setPen(QColor("#A0A0A0"));
        QFont headerFont("Microsoft YaHei", 9, QFont::Bold);
        painter.setFont(headerFont);

        QString arrow = m_foldersCollapsed ? "▶" : "▼";
        QString headerText = QString("  %1  文件夹 (%2)").arg(arrow).arg(m_folderCount);

        QFontMetrics fm(headerFont);
        int textW = fm.horizontalAdvance(headerText) + 12;
        QRect textRect(m_folderHeaderRect.left(), m_folderHeaderRect.top(), textW, m_folderHeaderRect.height());
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, headerText);

        painter.restore();
    }

    if (m_fileCount > 0 && !m_fileHeaderRect.isEmpty()) {
        painter.save();
        painter.translate(0, -scrollY);

        painter.setPen(QColor("#A0A0A0"));
        QFont headerFont("Microsoft YaHei", 9, QFont::Bold);
        painter.setFont(headerFont);

        // “文件”分组无箭头，永久伸展
        QString headerText = QString("  文件 (%1)").arg(m_fileCount);

        QFontMetrics fm(headerFont);
        int textW = fm.horizontalAdvance(headerText) + 12;
        QRect textRect(m_fileHeaderRect.left(), m_fileHeaderRect.top(), textW, m_fileHeaderRect.height());
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, headerText);

        painter.restore();
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        if (m_fileCount > 0 && m_fileHeaderRect.contains(contentPos)) {
            m_filesCollapsed = !m_filesCollapsed;
            doLayout();
            viewport()->update();
            event->accept();
            return;
        }
=======
        // “文件”分组永久伸展，忽略点击折叠交互
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. **Compilation Verification**:
   Run `cmake --build build --config Release` to ensure clean build without MSVC `C2511`/`C2065`/`C2550` errors.
2. **Behavioral Check**:
   - Check List View (`DropTreeView`): Verify first row in sections is cleanly shifted down 26px without overlapping or being obscured by a floating block.
   - Check Column View (`ColumnViewWidget`): Verify header displays `文件夹 (N)` without `子文件夹` wording and `ColumnItemDelegate` stays intact.
   - Check Grid View (`JustifiedView`): Verify `文件 (N)` header is static (no collapse arrow, permanent expansion) and drawn over a transparent background with dynamic text bounds.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Single Source of Truth**: Reuses `TreeItemDelegate`'s delegate-based inline header drawing architecture for list view section rendering, avoiding hardcoded overlay painting inside view classes, while keeping `ColumnItemDelegate` signatures strictly frozen.

## 6. Header API Signature Verification
- Verified `TreeItemDelegate` constructor in `src/ui/TreeItemDelegate.h`.
- Verified `ColumnItemDelegate` signature in `src/ui/ColumnItemDelegate.h` (`ColumnItemDelegate(QObject* parent = nullptr)` frozen).
- Verified `DropTreeView::paintEvent` in `src/ui/DropTreeView.h`.
- Verified `ColumnViewPane` label setup in `src/ui/ColumnViewWidget.h`.
