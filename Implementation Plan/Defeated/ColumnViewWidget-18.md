# ColumnViewWidget-18.md Implementation Plan

## Overview
本实施方案旨在全面修复列视图 (Column View) 丢失的功能特性，并彻底解决列分割线 (Separator Line) 上下截断的视觉缺陷：
1. **彻底解决分割线截断问题**：
   - 将 `ColumnViewWidget` 主布局、`m_containerLayout` 以及 `ColumnViewPane` 内部布局的 `contentsMargins` 严格重置为 `(0, 0, 0, 0)`、`spacing` 重置为 `0`；
   - 将列右侧分割线定义转移到 `ColumnViewPane` 容器层或 `QListView#ColumnViewPaneListView` 的 100% 贯通高度上，防止滚动条或 Margin 挤压导致分割线上下中断。
2. **恢复 FilterState 过滤同步与透传**：
   - 为 `ColumnViewWidget` 与 `ColumnViewPane` 恢复 `applyFilterState(const FilterState& state)` 与 `setFilterState(const FilterState& state)`，确保筛选面板修改过滤条件时列视图可同步过滤。
3. **恢复平滑向右自动滚动 (`scrollToRightmostPane`)**：
   - 当向列视图中追加新列 (`appendColumn`) 时，自动将水平滚动条异步滑动至最右侧，并确保最右侧列完整可见。
4. **恢复选区与路径检索 API**：
   - 恢复 `getSelectedPaths()`、`getSelectedIndexes()`、`containsPath(const QString& path)` 接口及 `selectionChanged` 信号，供右侧 MetaPanel 及状态栏精准获取列选区状态。
5. **恢复 Delegate 行内元数据渲染 (色标圆点与星级 ★N)**：
   - 在 `ColumnItemDelegate::paint()` 中恢复对 `ColorRole`（色标圆点）与 `RatingRole`（星级文本）的精致绘制，与 Version-6 表现完全一致。

---

## Modified Files List
- `resources/style.qss`
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ColumnItemDelegate.h`
- `src/ui/ColumnItemDelegate.cpp`
- `src/ui/ContentPanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `resources/style.qss`

```diff
<<<<<<< SEARCH
QListView#ColumnViewPaneListView {
    background: #1E1E1E;
    border: none;
    border-right: 1px solid #2D2D2D;
    color: #CCCCCC;
    outline: none;
}
=======
QWidget#ColumnViewPane {
    background: #1E1E1E;
    border-right: 1px solid #2D2D2D;
}

QListView#ColumnViewPaneListView {
    background: transparent;
    border: none;
    color: #CCCCCC;
    outline: none;
}
>>>>>>> REPLACE
```

---

### 2. `src/ui/ColumnViewWidget.h`

```diff
<<<<<<< SEARCH
    void loadDirectory();
    void selectItemByPath(const QString& itemPath);
    void clearSelection();

    signals:
=======
    void loadDirectory();
    void selectItemByPath(const QString& itemPath);
    void clearSelection();
    void setFilterState(const FilterState& state);

signals:
    void selectionChanged();
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    ColumnViewPane* activePane() const;
    void refreshActiveColumn();
    void updateMetadataForPath(const QString& path);
    void clearOtherSelections(ColumnViewPane* currentPane);
    void clearAllColumns();

    QList<ColumnViewPane*> panes() const { return m_panes; }

signals:
    void activeColumnRecordsChanged(const std::vector<QuarkMeta::ItemRecord>& records);
=======
    ColumnViewPane* activePane() const;
    bool containsPath(const QString& path) const;
    void refreshActiveColumn();
    void updateMetadataForPath(const QString& path);
    void clearOtherSelections(ColumnViewPane* currentPane);
    void clearAllColumns();
    void scrollToRightmostPane();
    QStringList getSelectedPaths() const;
    QModelIndexList getSelectedIndexes() const;
    void applyFilterState(const FilterState& state);

    QList<ColumnViewPane*> panes() const { return m_panes; }

signals:
    void pathNavigated(const QString& path);
    void selectionChanged();
    void activeColumnRecordsChanged(const std::vector<QuarkMeta::ItemRecord>& records);
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    ContentPanel* m_contentPanel = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_container = nullptr;
    QHBoxLayout* m_containerLayout = nullptr;
    QList<ColumnViewPane*> m_panes;
    QString m_rootPath;
=======
    ContentPanel* m_contentPanel = nullptr;
    FilterState m_currentFilter;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_container = nullptr;
    QHBoxLayout* m_containerLayout = nullptr;
    QList<ColumnViewPane*> m_panes;
    QString m_rootPath;
>>>>>>> REPLACE
```

---

### 3. `src/ui/ColumnViewWidget.cpp`

```diff
<<<<<<< SEARCH
ColumnViewPane::ColumnViewPane(const QString& path, ContentPanel* contentPanel, QWidget* parent)
    : QWidget(parent), m_path(path), m_contentPanel(contentPanel) {
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
=======
ColumnViewPane::ColumnViewPane(const QString& path, ContentPanel* contentPanel, QWidget* parent)
    : QWidget(parent), m_path(path), m_contentPanel(contentPanel) {
    setObjectName("ColumnViewPane");
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    m_listView = new DropListView(this);
    m_listView->setObjectName("ColumnViewPaneListView");
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->setModel(m_proxyModel);

    auto* delegate = new ColumnItemDelegate(this);
    m_listView->setItemDelegate(delegate);
    layout->addWidget(m_listView);

    connect(m_listView, &QListView::clicked, this, &ColumnViewPane::onClicked);
    connect(m_listView, &QListView::doubleClicked, this, &ColumnViewPane::onDoubleClicked);
=======
    m_listView = new DropListView(this);
    m_listView->setObjectName("ColumnViewPaneListView");
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->setModel(m_proxyModel);

    auto* delegate = new ColumnItemDelegate(this);
    m_listView->setItemDelegate(delegate);
    layout->addWidget(m_listView);

    if (m_listView->selectionModel()) {
        connect(m_listView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ColumnViewPane::selectionChanged);
    }

    connect(m_listView, &QListView::clicked, this, &ColumnViewPane::onClicked);
    connect(m_listView, &QListView::doubleClicked, this, &ColumnViewPane::onDoubleClicked);
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void ColumnViewPane::clearSelection() {
    if (m_listView && m_listView->selectionModel()) {
        m_listView->clearSelection();
    }
}
=======
void ColumnViewPane::clearSelection() {
    if (m_listView && m_listView->selectionModel()) {
        m_listView->clearSelection();
    }
}

void ColumnViewPane::setFilterState(const FilterState& state) {
    if (m_proxyModel) {
        m_proxyModel->setFilterState(state);
    }
}
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
ColumnViewWidget::ColumnViewWidget(ContentPanel* contentPanel, QWidget* parent)
    : QWidget(parent), m_contentPanel(contentPanel) {
    
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_container = new QWidget(m_scrollArea);
    m_containerLayout = new QHBoxLayout(m_container);
    m_containerLayout->setContentsMargins(0, 0, 0, 0);
    m_containerLayout->setSpacing(0);
    m_containerLayout->addStretch(1);

    m_container->setLayout(m_containerLayout);
    m_scrollArea->setWidget(m_container);

    mainLayout->addWidget(m_scrollArea);
}
=======
ColumnViewWidget::ColumnViewWidget(ContentPanel* contentPanel, QWidget* parent)
    : QWidget(parent), m_contentPanel(contentPanel) {
    
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setObjectName("ColumnViewScrollArea");
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_container = new QWidget(m_scrollArea);
    m_containerLayout = new QHBoxLayout(m_container);
    m_containerLayout->setContentsMargins(0, 0, 0, 0);
    m_containerLayout->setSpacing(0);
    m_containerLayout->addStretch(1);

    m_container->setLayout(m_containerLayout);
    m_scrollArea->setWidget(m_container);

    mainLayout->addWidget(m_scrollArea);
}

void ColumnViewWidget::scrollToRightmostPane() {
    QMetaObject::invokeMethod(this, [this]() {
        if (m_scrollArea && m_scrollArea->horizontalScrollBar()) {
            m_scrollArea->horizontalScrollBar()->setValue(m_scrollArea->horizontalScrollBar()->maximum());
        }
        if (!m_panes.isEmpty() && m_panes.last()) {
            m_scrollArea->ensureWidgetVisible(m_panes.last(), 0, 0);
        }
    }, Qt::QueuedConnection);
}

bool ColumnViewWidget::containsPath(const QString& path) const {
    if (path.isEmpty()) return false;
    QString cleanTarget = QDir::toNativeSeparators(QDir::cleanPath(path));
    for (auto* pane : m_panes) {
        if (pane) {
            QString panePath = QDir::toNativeSeparators(QDir::cleanPath(pane->path()));
            if (QString::compare(panePath, cleanTarget, Qt::CaseInsensitive) == 0) {
                return true;
            }
        }
    }
    return false;
}

QStringList ColumnViewWidget::getSelectedPaths() const {
    ColumnViewPane* pane = activePane();
    if (!pane || !pane->listView() || !pane->listView()->selectionModel()) return {};
    QStringList paths;
    for (const auto& idx : pane->listView()->selectionModel()->selectedIndexes()) {
        if (idx.column() == 0) {
            QString p = idx.data(PathRole).toString();
            if (!p.isEmpty()) paths << p;
        }
    }
    return paths;
}

QModelIndexList ColumnViewWidget::getSelectedIndexes() const {
    ColumnViewPane* pane = activePane();
    if (!pane || !pane->listView() || !pane->listView()->selectionModel()) return {};
    return pane->listView()->selectionModel()->selectedIndexes();
}

void ColumnViewWidget::applyFilterState(const FilterState& state) {
    m_currentFilter = state;
    for (auto* pane : m_panes) {
        pane->setFilterState(state);
    }
}
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void ColumnViewWidget::appendColumn(const QString& path) {
    ColumnViewPane* pane = new ColumnViewPane(path, m_contentPanel, m_container);
    
    connect(pane, &ColumnViewPane::folderSelected, this, &ColumnViewWidget::onFolderSelected);
    connect(pane, &ColumnViewPane::fileSelected, this, &ColumnViewWidget::onFileSelected);

    if (pane->model()) {
        emit activeColumnRecordsChanged(pane->model()->allRecords());
    }

    // 插入到 layout Stretch 之前
    m_containerLayout->insertWidget(m_containerLayout->count() - 1, pane);
    m_panes.append(pane);

    // 自动向右滚动到底
    QMetaObject::invokeMethod(this, [this]() {
        if (m_scrollArea && m_scrollArea->horizontalScrollBar()) {
            m_scrollArea->horizontalScrollBar()->setValue(m_scrollArea->horizontalScrollBar()->maximum());
        }
    }, Qt::QueuedConnection);
}
=======
void ColumnViewWidget::appendColumn(const QString& path) {
    ColumnViewPane* pane = new ColumnViewPane(path, m_contentPanel, m_container);
    pane->setFixedWidth(240);
    pane->setFilterState(m_currentFilter);
    pane->loadDirectory();

    connect(pane, &ColumnViewPane::selectionChanged, this, [this]() {
        emit selectionChanged();
    });
    
    connect(pane, &ColumnViewPane::folderSelected, this, &ColumnViewWidget::onFolderSelected);
    connect(pane, &ColumnViewPane::fileSelected, this, &ColumnViewWidget::onFileSelected);

    if (pane->model()) {
        emit activeColumnRecordsChanged(pane->model()->allRecords());
    }

    // 插入到 layout Stretch 之前
    m_containerLayout->insertWidget(m_containerLayout->count() - 1, pane);
    m_panes.append(pane);

    scrollToRightmostPane();
}
>>>>>>> REPLACE
```

---

### 4. `src/ui/ColumnItemDelegate.cpp`

```diff
<<<<<<< SEARCH
    // 4. 文件/文件夹名称文本 (纯净单行渲染，不做行内星级/颜色标识绘制)
    int textLeft = iconRect.right() + 8;
    int textWidth = rect.width() - (textLeft - rect.left()) - rightReserved;
    QRect textRect(textLeft, rect.top(), qMax(10, textWidth), rect.height());

    QString name = index.data(Qt::DisplayRole).toString();
    QColor textColor = selected ? QColor("#FFFFFF") : QColor("#EEEEEE");
    painter->setPen(textColor);
    painter->setFont(option.font);

    QString elidedText = option.fontMetrics.elidedText(name, Qt::ElideRight, textRect.width());
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);
=======
    // 3.5 读取色标与星级元数据
    int rating = index.data(RatingRole).toInt();
    QString colorName = index.data(ColorRole).toString();

    if (!colorName.isEmpty()) {
        static const QMap<QString, QString> s_colorHexMap = {
            {"红色", "#E24B4A"}, {"橙色", "#EF9F27"}, {"黄色", "#FECF0E"},
            {"绿色", "#639922"}, {"青色", "#1D9E75"}, {"蓝色", "#378ADD"},
            {"紫色", "#7F77DD"}, {"灰色", "#5F5E5A"}
        };
        QString hexColor = s_colorHexMap.value(colorName, colorName);
        if (hexColor.startsWith("#")) {
            painter->setBrush(QColor(hexColor));
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(option.rect.left() + 2, option.rect.top() + (option.rect.height() - 6) / 2, 6, 6);
        }
    }

    // 4. 文件/文件夹名称文本
    int extraRightMargin = rightReserved;
    if (rating > 0) extraRightMargin += 32;

    int textLeft = iconRect.right() + 8;
    int textWidth = rect.width() - (textLeft - rect.left()) - extraRightMargin;
    QRect textRect(textLeft, rect.top(), qMax(10, textWidth), rect.height());

    QString name = index.data(Qt::DisplayRole).toString();
    QColor textColor = selected ? QColor("#FFFFFF") : QColor("#EEEEEE");
    painter->setPen(textColor);
    painter->setFont(option.font);

    QString elidedText = option.fontMetrics.elidedText(name, Qt::ElideRight, textRect.width());
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

    // 5. 绘制星级标示 (若 rating > 0)
    if (rating > 0) {
        int starRight = rect.right() - rightReserved;
        QRect starRect(starRight - 30, rect.top() + (rect.height() - 12) / 2, 30, 12);
        painter->setPen(QColor("#FFC107"));
        painter->setFont(QFont("Segoe UI", 9, QFont::Bold));
        painter->drawText(starRect, Qt::AlignRight | Qt::AlignVCenter, QString("★%1").arg(rating));
    }
>>>>>>> REPLACE
```

---

### 5. `src/ui/ContentPanel.cpp`

```diff
<<<<<<< SEARCH
    m_columnView = new ColumnViewWidget(this, this);
    connect(m_columnView, &ColumnViewWidget::selectionChanged, this, &ContentPanel::onSelectionChanged);
=======
    m_columnView = new ColumnViewWidget(this, this);
    connect(m_columnView, &ColumnViewWidget::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_columnView, &ColumnViewWidget::pathNavigated, this, [this](const QString& path) {
        if (!QFileInfo(path).isDir()) {
            emit fileActivated(path);
        }
    });
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### 1. 编译验证
使用 CMake 构建应用程序，确保编译零 Warning / Error：
```bash
cmake --build build --config Release
```

### 2. 功能及 UI 验证
1. **分割线贯通性**：观察列与列之间的垂直分割线，确认从列容器最顶端垂直贯穿至最底端，无任何上下截断或悬空。
2. **平滑向右滚动**：深度展开多层子文件夹，确认视口自动向右平滑滚动，新展开的列自动保持居中/可见。
3. **筛选联动**：在筛选面板切换类型或星级过滤条件，确认列视图中的数据按条件实时更新。
4. **元数据绘制**：为某些资产设置颜色标记与星级（如 ★5），确认列视图列表项左侧出现色标圆点，右侧出现 `★5` 黄金星级标示。
