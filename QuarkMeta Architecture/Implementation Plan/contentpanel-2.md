# Implementation Plan - Refactor List View Header Geometry and Remove QSS Padding Patch (`contentpanel-2.md`)

## 1. Overview
This implementation plan addresses header text disappearing and clipping in `ContentPanel` when panel width is small (e.g. 300-350px):
1. **Remove QSS `padding-left` Patch**: Remove the QSS `padding-left` manipulation from `updateGridSize()` that pushed the "名称" text out of view when column 0 width became smaller than the padding value.
2. **Reverse Header Column Resize Strategy**: Change all column resize modes to `QHeaderView::Interactive` and set initial section sizes (section 0 to 260px, sections 1-6 to 40, 90, 90, 70, 75, 110px). Section 0 ("名称") is guaranteed a base width of 260px to display thumbnail and filename properly without being squeezed out.

---

## 2. Modified Files List
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### `src/ui/ContentPanel.cpp`

```git
<<<<<<< SEARCH
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
=======
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
>>>>>>> REPLACE
```

```git
<<<<<<< SEARCH
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
=======
    // 统一表头样式：干净、左对齐、无任何破坏性 padding
    m_treeView->header()->setStyleSheet( 
        "QHeaderView::section { background-color: #252525; color: #B0B0B0; border: none; border-right: 1px solid #333333; height: 32px; font-size: 11px; padding: 0 4px; }" 
    ); 
    
    auto* header = m_treeView->header();
    header->setStretchLastSection(false);
    header->setCascadingSectionResizes(true);
    header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    for (int i = 0; i <= 6; ++i) {
        header->setSectionHidden(i, false);
    }
    header->setSectionHidden(7, true);

    header->setMinimumSectionSize(35);
    
    header->resizeSection(0, 260);  // 【第 0 列名称】：保底 260px，确保缩略图和文件名完美并存！
    header->resizeSection(1, 40);   // 【第 1 列状态】：40px
    header->resizeSection(2, 90);   // 【第 2 列星级】：90px
    header->resizeSection(3, 90);   // 【第 3 列尺寸】：90px
    header->resizeSection(4, 70);   // 【第 4 列类型】：70px
    header->resizeSection(5, 75);   // 【第 5 列大小】：75px
    header->resizeSection(6, 110);  // 【第 6 列修改日期】：110px

    for (int i = 0; i <= 6; ++i) {
        header->setSectionResizeMode(i, QHeaderView::Interactive);
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### Verification Steps
1. Verify `src/ui/ContentPanel.cpp` has been modified using git merge diff/read_file.
2. Ensure QSS padding patch has been completely purged from `updateGridSize()`.
3. Confirm section resize modes are set to `Interactive` and initial column widths are set cleanly.
