# List View Architecture Fix Implementation Plan

This implementation plan fixes the list view layout architecture anomaly where the header ("名称", "状态", "评分") is incorrectly shown and column 0 is squeezed on initial startup (Image ①), enforcing the correct single-column list view layout architecture (Image ②).

## Overview
Currently, in `ContentPanel::initListView()`, `m_treeView->header()` is initialized with 7 sections enabled and visible. When the application launches, `QTreeView` renders a multi-column table header ("名称", "状态", "评分", etc.) and compresses column 0 ("名称"). Upon maximizing/restoring, the view stretches column 0 to display the clean single-column item list.
To align 100% with the intended design architecture (Image ②):
1. The table header must be hidden via `m_treeView->setHeaderHidden(true)`.
2. Section 0 (Name / Preview Card) must occupy 100% stretch width without being squeezed by hidden sub-columns.

## Modified Files List
- `src/ui/ContentPanel.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.cpp`
Hide `QTreeView` header in `initListView()` and ensure single-column stretch behavior.

<<<<<<< SEARCH
    m_treeView->header()->setDefaultAlignment(Qt::AlignCenter);
    m_treeView->header()->setStyleSheet(
        "QHeaderView::section { background-color: #252525; color: #B0B0B0; border: none; border-right: 1px solid #333333; height: 32px; font-size: 11px; }"
    );

    // --- 列表表头（Header）列宽固定化重构 ---
    auto* header = m_treeView->header();
    header->setStretchLastSection(false); // 禁止末端强行拉伸
    header->setCascadingSectionResizes(false);

    // 1. 确保所有 7 列均可见，并且彻底隐藏或移除多余的第 7 列（原本的第 7 列已被前移）
    for (int i = 0; i <= 6; ++i) {
        header->setSectionHidden(i, false);
    }
    header->setSectionHidden(7, true);

    // 2. 精确设置各列固定像素宽度（彻底移除“颜色”列，平移后续所有列宽度）
    header->resizeSection(1, 50);   // 状态 (固定 50px 图标区)
    header->resizeSection(2, 120);  // 星级 (固定 120px 图标区)
    header->resizeSection(3, 120);  // 尺寸 (固定 120px)
    header->resizeSection(4, 80);   // 类型 (固定 80px)
    header->resizeSection(5, 100);  // 大小 (固定 100px)
    header->resizeSection(6, 120);  // 修改日期 (固定 120px)

    // 3. 锁定调整模式：第 0 列（名称）弹性自适应拉伸，第 1~6 列物理固定禁止拖拽
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int i = 1; i <= 6; ++i) {
        header->setSectionResizeMode(i, QHeaderView::Fixed);
    }
=======
    // 隐藏列表头部，收拢为单列纯净列表架构 (图②)
    m_treeView->setHeaderHidden(true);

    auto* header = m_treeView->header();
    header->setStretchLastSection(false);
    header->setCascadingSectionResizes(false);

    // 第 0 列弹性自适应拉伸充满视口，第 1~6 列隐去
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int i = 1; i <= 6; ++i) {
        header->setSectionHidden(i, true);
    }
>>>>>>> REPLACE

## Build & Verification Steps
1. Configure and build the project:
   ```bash
   cmake -B build -G Ninja
   cmake --build build
   ```
2. Verify that `m_treeView` displays without the top header bar upon application startup and that items in column 0 fill the view width smoothly without name truncation or column squeezing.
