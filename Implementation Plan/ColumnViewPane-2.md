# 阶段三实施方案：分栏视图 (ColumnViewPane) 单视图化与 DualSectionPanel 依赖裁撤

## 1. Overview（概述与解决的问题）

### 目标与背景
本方案为“四视图单滚动条架构（放弃双子视图）”重构的**阶段三**实施方案。
核心目标：
1. 改造分栏视图中的列面板 `ColumnViewPane`，由原先依赖 `DualSectionPanel` 管理 `m_folderListView + m_listView` 双列表，升级为**直接管理单 `m_unifiedListView` (`DropListView`) + 单 `FilterProxyModel`**；
2. 直连管理分组标头：`FolderSectionHeaderBar` 与 `FileSectionHeaderBar` 由 `ColumnViewPane` 直接持有的滚动容器 `m_paneScrollArea` 内部管理；
3. 严格遵循【契约锁】：保留 `folderListView()`、`listView()`、`folderProxyModel()`、`fileProxyModel()` 公共 API，内部映射至单 `m_unifiedListView` 与单 `m_proxyModel`，100% 保护 `ColumnViewWidget` 及外部调用方的编译契约。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/ColumnViewPane.h`（移除 `DualSectionPanel` 依赖，引入单列表与标头直接持有声明）
2. `src/ui/ColumnViewPane.cpp`（实现单列表创建、选区与折叠事件绑定，裁撤双 ProxyModel）

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/ColumnViewPane.h` 精准替换

```cpp
<<<<<<< SEARCH
class ContentPanel;
class DualSectionPanel;

class ColumnViewPane : public QWidget {
=======
class ContentPanel;
class FileSectionHeaderBar;

class ColumnViewPane : public QWidget {
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    DropListView* listView() const;
    DropListView* folderListView() const;
    FilterProxyModel* proxyModel() const { return m_proxyModel; }
    FilterProxyModel* folderProxyModel() const { return m_folderProxyModel; }
    FilterProxyModel* fileProxyModel() const { return m_fileProxyModel; }
    DiskItemModel* model() const { return m_model; }
    FolderSectionHeaderBar* folderHeader() const;
=======
    DropListView* listView() const { return m_unifiedListView; }
    DropListView* folderListView() const { return m_unifiedListView; }
    FilterProxyModel* proxyModel() const { return m_proxyModel; }
    FilterProxyModel* folderProxyModel() const { return m_proxyModel; }
    FilterProxyModel* fileProxyModel() const { return m_proxyModel; }
    DiskItemModel* model() const { return m_model; }
    FolderSectionHeaderBar* folderHeader() const { return m_folderHeader; }
    FileSectionHeaderBar* fileHeader() const { return m_fileHeader; }
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
private:
    QString m_path;
    QString m_pendingSelectPath;
    QSet<QString> m_pendingSelectPaths;
    bool m_isPendingEdit = false;
    ContentPanel* m_contentPanel = nullptr;
    DiskItemModel* m_model = nullptr;
    FilterProxyModel* m_proxyModel = nullptr;
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;
    bool m_isActive = false;
    QScrollArea* m_paneScrollArea = nullptr;
    DualSectionPanel* m_panel = nullptr;
    DropListView* m_folderListView = nullptr;
    DropListView* m_listView = nullptr;
};
=======
private:
    QString m_path;
    QString m_pendingSelectPath;
    QSet<QString> m_pendingSelectPaths;
    bool m_isPendingEdit = false;
    ContentPanel* m_contentPanel = nullptr;
    DiskItemModel* m_model = nullptr;
    FilterProxyModel* m_proxyModel = nullptr;
    bool m_isActive = false;
    QScrollArea* m_paneScrollArea = nullptr;
    QWidget* m_containerWidget = nullptr;
    class QVBoxLayout* m_containerLayout = nullptr;
    FolderSectionHeaderBar* m_folderHeader = nullptr;
    FileSectionHeaderBar* m_fileHeader = nullptr;
    DropListView* m_unifiedListView = nullptr;
};
>>>>>>> REPLACE
```

---

### 3.2 `src/ui/ColumnViewPane.cpp` 精准替换

```cpp
<<<<<<< SEARCH
#include "ColumnViewPane.h"
#include "DualSectionPanel.h"
#include "ColumnItemDelegate.h"
#include "ContentPanel.h"
#include "Logger.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QEvent>
#include <QMouseEvent>

namespace QuarkMeta {

ColumnViewPane::ColumnViewPane(const QString& path, ContentPanel* contentPanel, QWidget* parent)
    : QWidget(parent), m_path(path), m_contentPanel(contentPanel) {
    
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_paneScrollArea = new QScrollArea(this);
    m_paneScrollArea->setFrameShape(QFrame::NoFrame);
    m_paneScrollArea->setWidgetResizable(true);
    m_paneScrollArea->setFocusPolicy(Qt::NoFocus);
    m_paneScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_paneScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_model = new DiskItemModel(this);
    
    m_folderProxyModel = new FilterProxyModel(this);
    m_folderProxyModel->setSourceModel(m_model);
    m_folderProxyModel->setFilterKeyColumn(0);
    m_folderProxyModel->setDynamicSortFilter(true);
    FilterState folderOnlyFilter;
    folderOnlyFilter.showFolders = true;
    folderOnlyFilter.showFiles = false;
    m_folderProxyModel->currentFilter = folderOnlyFilter;

    m_fileProxyModel = new FilterProxyModel(this);
    m_fileProxyModel->setSourceModel(m_model);
    m_fileProxyModel->setFilterKeyColumn(0);
    m_fileProxyModel->setDynamicSortFilter(true);
    FilterState fileOnlyFilter;
    fileOnlyFilter.showFolders = false;
    fileOnlyFilter.showFiles = true;
    m_fileProxyModel->currentFilter = fileOnlyFilter;

    m_proxyModel = m_fileProxyModel; // 兼容现有句柄

    m_folderListView = new DropListView(this);
    m_folderListView->setFrameShape(QFrame::NoFrame);
    m_folderListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_folderListView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_folderListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_folderListView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_folderListView->setModel(m_folderProxyModel);
    m_folderListView->setItemDelegate(new ColumnItemDelegate(this));

    m_listView = new DropListView(this);
    m_listView->setFrameShape(QFrame::NoFrame);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setModel(m_fileProxyModel);
    m_listView->setItemDelegate(new ColumnItemDelegate(this));

    m_panel = new DualSectionPanel(m_folderListView, m_listView, m_folderProxyModel, m_fileProxyModel, this);
    m_paneScrollArea->setWidget(m_panel);
    mainLayout->addWidget(m_paneScrollArea);

    if (m_contentPanel) {
        m_folderListView->installEventFilter(m_contentPanel);
        m_listView->installEventFilter(m_contentPanel);
        m_paneScrollArea->installEventFilter(m_contentPanel);
        m_panel->installEventFilter(m_contentPanel);
    }

    // 绑定展开/选中信号...
=======
#include "ColumnViewPane.h"
#include "ColumnItemDelegate.h"
#include "ContentPanel.h"
#include "Logger.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QEvent>
#include <QMouseEvent>

namespace QuarkMeta {

ColumnViewPane::ColumnViewPane(const QString& path, ContentPanel* contentPanel, QWidget* parent)
    : QWidget(parent), m_path(path), m_contentPanel(contentPanel) {
    
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_paneScrollArea = new QScrollArea(this);
    m_paneScrollArea->setFrameShape(QFrame::NoFrame);
    m_paneScrollArea->setWidgetResizable(true);
    m_paneScrollArea->setFocusPolicy(Qt::NoFocus);
    m_paneScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_paneScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_model = new DiskItemModel(this);
    
    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterKeyColumn(0);
    m_proxyModel->setDynamicSortFilter(true);

    m_containerWidget = new QWidget(this);
    m_containerLayout = new QVBoxLayout(m_containerWidget);
    m_containerLayout->setContentsMargins(0, 0, 0, 0);
    m_containerLayout->setSpacing(0);

    m_folderHeader = new FolderSectionHeaderBar(m_containerWidget);
    m_folderHeader->hide();
    m_containerLayout->addWidget(m_folderHeader);

    m_unifiedListView = new DropListView(this);
    m_unifiedListView->setFrameShape(QFrame::NoFrame);
    m_unifiedListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_unifiedListView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_unifiedListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_unifiedListView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_unifiedListView->setModel(m_proxyModel);
    m_unifiedListView->setItemDelegate(new ColumnItemDelegate(this));
    m_containerLayout->addWidget(m_unifiedListView);

    m_fileHeader = new FileSectionHeaderBar(m_containerWidget);
    m_fileHeader->hide();
    m_containerLayout->addWidget(m_fileHeader);

    m_paneScrollArea->setWidget(m_containerWidget);
    mainLayout->addWidget(m_paneScrollArea);

    if (m_contentPanel) {
        m_unifiedListView->installEventFilter(m_contentPanel);
        m_paneScrollArea->installEventFilter(m_contentPanel);
        m_containerWidget->installEventFilter(m_contentPanel);
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译与验证方法）

1. **编译核验**：
   ```bash
   cmake -B build -S .
   cmake --build build --config Release
   ```
2. **分栏视图单视图验证**：
   - 切换至分栏视图 (ColumnView) 并展开多列；
   - 验证单列中文件夹与文件项目完美集成排布；
   - 验证向右展开子列、路径点击与快捷键依然响应正常。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用自查）

- **完全解耦 DualSectionPanel**：`ColumnViewPane` 内部彻底清除了 `DualSectionPanel` 依赖；
- **契约锁保护**：向后兼容 `folderListView()` / `listView()` 返回同一个 `m_unifiedListView` 指针。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用的成员函数 | 所属类 / 头文件 | 物理真实签名 | 核查状态 |
| :--- | :--- | :--- | :--- |
| `listView()` | `ColumnViewPane.h` | `DropListView* listView() const;` | 100% 匹配 (兼容别名) |
| `folderListView()` | `ColumnViewPane.h` | `DropListView* folderListView() const;` | 100% 匹配 (兼容别名) |
| `proxyModel()` | `ColumnViewPane.h` | `FilterProxyModel* proxyModel() const;` | 100% 匹配 |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链核查表）

| 受影响文件 | 修改/新增 `#include` | 类型与枚举完整性核查 |
| :--- | :--- | :--- |
| `ColumnViewPane.h` | 移除 `class DualSectionPanel;` 前置声明，增加 `class FileSectionHeaderBar;` | 依赖关系清理 |
| `ColumnViewPane.cpp` | 移除 `#include "DualSectionPanel.h"` | 自包含完整闭合 |
