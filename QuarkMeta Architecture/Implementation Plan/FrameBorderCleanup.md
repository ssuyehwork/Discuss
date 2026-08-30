# 内容视图面板 QFrame 1px 原生白边框彻底根除实施方案

## 1. Overview（概述与解决的问题）

在 Windows 平台下，Qt 的 `QStackedWidget`、`QAbstractItemView` 与 `QTreeView` 继承自 `QFrame`，默认带有 Windows 原生 `QFrame::StyledPanel` 或 `Sunken` 凸起/暗陷 1px 亮色高光边框，导致内容面板标题栏下方悬挂一条 1px 白色亮线。
本方案通过在 C++ 初始化时显式调用 `setFrameShape(QFrame::NoFrame)`，并在 CSS/QSS 中兜底置空 border 与 outline，彻底物理打死并根除该边框残留。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/ContentPanel.cpp`
2. `src/ui/JustifiedView.cpp`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/ContentPanel.cpp`

在初始化 `m_viewStack`、`initGridView()` 和 `initListView()` 中添加 `setFrameShape(QFrame::NoFrame)`：

```
<<<<<<< SEARCH
    m_viewStack = new QStackedWidget(this);
    initGridView();
    initListView();
    m_viewStack->addWidget(m_gridView);
    m_viewStack->addWidget(m_treeView);
    m_viewStack->setCurrentWidget(m_gridView);
=======
    m_viewStack = new QStackedWidget(this);
    m_viewStack->setFrameShape(QFrame::NoFrame);
    initGridView();
    initListView();
    m_viewStack->addWidget(m_gridView);
    m_viewStack->addWidget(m_treeView);
    m_viewStack->setCurrentWidget(m_gridView);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::initGridView() {
    m_gridView = new DropJustifiedView(this);
    m_gridView->setSelectionMode(QAbstractItemView::ExtendedSelection);
=======
void ContentPanel::initGridView() {
    m_gridView = new DropJustifiedView(this);
    m_gridView->setFrameShape(QFrame::NoFrame);
    m_gridView->setSelectionMode(QAbstractItemView::ExtendedSelection);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::initListView() {
    m_treeView = new DropTreeView(this);
    m_treeView->setAlternatingRowColors(true);
=======
void ContentPanel::initListView() {
    m_treeView = new DropTreeView(this);
    m_treeView->setFrameShape(QFrame::NoFrame);
    m_treeView->setAlternatingRowColors(true);
>>>>>>> REPLACE
```

### 3.2 `src/ui/JustifiedView.cpp`

构造函数显式强制 `setFrameShape(QFrame::NoFrame)`：

```
<<<<<<< SEARCH
JustifiedView::JustifiedView(QWidget* parent) : QAbstractItemView(parent) {
    m_layoutTimer = new QTimer(this);
=======
JustifiedView::JustifiedView(QWidget* parent) : QAbstractItemView(parent) {
    setFrameShape(QFrame::NoFrame);
    m_layoutTimer = new QTimer(this);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译确认**：
   ```bash
   cmake --build build --config Release
   ```
2. **UI 边框验证**：
   运行应用，检查 `ContentPanel` 顶部标题栏下方与网格/列表视图四周，确认原生的 1px 白色/灰色凸起高光边框彻底消失，界面纯净一体化。
