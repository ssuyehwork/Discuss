# Implementation Plan - Fix Header Alignment and Section Squeezing (`contentpanel-1.md`)

## 1. Overview
This implementation plan addresses two critical UI defects in `ContentPanel`:
1. **Header Squeezing under Low Panel Width**: High fixed column widths (total 590px) cause section 0 ("名称") to be squeezed out in initial small window sizes. Compacting fixed sections 1–6 to 480px total and setting minimum section size ensures section 0 retains ample layout space.
2. **Header Text Alignment & Vertical Alignment with Filename**: `setDefaultAlignment(Qt::AlignCenter)` causes text alignment issues for section 0. Changing default alignment to `Qt::AlignLeft | Qt::AlignVCenter` and dynamically applying `QHeaderView::section:first { padding-left: %1px; text-align: left; }` in `updateGridSize()` ensures exact pixel-level alignment between section 0 header text ("名称") and filenames drawn in `TreeItemDelegate`.

---

## 2. Modified Files List
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### `src/ui/ContentPanel.cpp`

```git
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
    m_treeView->header()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_treeView->header()->setStyleSheet( 
        "QHeaderView::section { background-color: #252525; color: #B0B0B0; border: none; border-right: 1px solid #333333; height: 32px; font-size: 11px; text-align: center; }" 
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

    // 2. 压缩精简固定列宽，释放更多空间给第 0 列（名称列），防止小窗口下被挤爆
    header->resizeSection(1, 40);   // 状态 (40px)
    header->resizeSection(2, 90);   // 星级 (90px)
    header->resizeSection(3, 90);   // 尺寸 (90px)
    header->resizeSection(4, 70);   // 类型 (70px)
    header->resizeSection(5, 80);   // 大小 (80px)
    header->resizeSection(6, 110);  // 修改日期 (110px)

    // 3. 锁定模式：第 0 列自适应，且设定最小宽度防护
    header->setMinimumSectionSize(80);
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int i = 1; i <= 6; ++i) {
        header->setSectionResizeMode(i, QHeaderView::Fixed);
    }
>>>>>>> REPLACE
```

```git
<<<<<<< SEARCH
    } else if (m_viewStack->currentWidget() == m_treeView) {
        // 列表模式：动态计算安全图标尺寸（最低不小于 16px）
        int iconSize = qMax(16, m_zoomLevel - 8);
        m_treeView->setIconSize(QSize(iconSize, iconSize));

        // 动态设置列表项的物理行高为 m_zoomLevel (范围：30px ~ 230px)
        static int lastTreeHeight = -1;
        if (lastTreeHeight != m_zoomLevel) {
            m_treeView->setStyleSheet( 
                QString("QTreeView { background-color: transparent; border: none; outline: none; font-size: 12px; }" 
                        "QTreeView::item { height: %1px; color: #EEEEEE; padding-left: 0px; }" 
                        "QTreeView::item:alternate { background-color: #252526; }" 
                        "QTreeView::item:selected { background-color: rgba(52, 152, 219, 0.2); border-left: 2px solid #3498db; }"
                        "QTreeView::item:hover { background-color: #2A2A2A; }"
                        "QTreeView QLineEdit { background-color: #2D2D2D; color: #FFFFFF; border: 1px solid #378ADD; border-radius: 6px; padding: 2px; selection-background-color: #378ADD; selection-color: #FFFFFF; }")
                .arg(m_zoomLevel)
            );
            lastTreeHeight = m_zoomLevel;
        }
    }
=======
    } else if (m_viewStack->currentWidget() == m_treeView) {
        // 列表模式：动态计算安全图标尺寸（最低不小于 16px）
        int iconSize = qMax(16, m_zoomLevel - 8);
        m_treeView->setIconSize(QSize(iconSize, iconSize));

        // 计算微卡片占用的总宽度：卡片宽度(m_zoomLevel - 6) + 左右留白(16px)
        int textOffset = (m_zoomLevel - 6) + 16;

        // 动态设置表头样式表：第 0 列表头向左对齐并施加 padding-left，其他列居中
        m_treeView->header()->setStyleSheet(QString(
            "QHeaderView::section { background-color: #252525; color: #B0B0B0; border: none; border-right: 1px solid #333333; height: 32px; font-size: 11px; text-align: center; }"
            "QHeaderView::section:first { padding-left: %1px; text-align: left; }"
        ).arg(textOffset));

        // 动态设置列表项的物理行高为 m_zoomLevel (范围：30px ~ 230px)
        static int lastTreeHeight = -1;
        if (lastTreeHeight != m_zoomLevel) {
            m_treeView->setStyleSheet( 
                QString("QTreeView { background-color: transparent; border: none; outline: none; font-size: 12px; }" 
                        "QTreeView::item { height: %1px; color: #EEEEEE; padding-left: 0px; }" 
                        "QTreeView::item:alternate { background-color: #252526; }" 
                        "QTreeView::item:selected { background-color: rgba(52, 152, 219, 0.2); border-left: 2px solid #3498db; }"
                        "QTreeView::item:hover { background-color: #2A2A2A; }"
                        "QTreeView QLineEdit { background-color: #2D2D2D; color: #FFFFFF; border: 1px solid #378ADD; border-radius: 6px; padding: 2px; selection-background-color: #378ADD; selection-color: #FFFFFF; }")
                .arg(m_zoomLevel)
            );
            lastTreeHeight = m_zoomLevel;
        }
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### Verification Steps
1. Verify `src/ui/ContentPanel.cpp` has been modified correctly using git merge diff/read_file.
2. Ensure column 1-6 width optimizations are applied (40px, 90px, 90px, 70px, 80px, 110px).
3. Ensure dynamic section 0 header padding and alignment logic is updated in `updateGridSize()`.
