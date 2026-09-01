# Implementation Plan: BreadcrumbBar

## 1. Overview
解决地址栏面包屑路径按钮（`BreadcrumbBar`）渲染浅白色矩形背景框的问题，以及优化筛选面板（`FilterPanel`）标题栏按钮（图钉与折叠按钮）的 SVG 图标尺寸约束与样式体验。

通过精细化 `BreadcrumbBar` 面包屑按钮的 QSS 隔离样式（消除继承性外框与浅色背景），并为 `FilterPanel` 的标题栏按钮明确设置物理图标尺寸 `setIconSize(QSize(16, 16))`，实现无白框的扁平通透地址栏与精细统一样式的筛选栏标题区。

## 2. Modified Files List
- `src/ui/BreadcrumbBar.cpp`
- `src/ui/FilterPanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/BreadcrumbBar.cpp`
<<<<<<< SEARCH
    // 面包屑按钮样式：扁平化，仅悬停可见背景
    btn->setStyleSheet(
        "QPushButton { background: transparent; border: none; border-radius: 6px; "
        "              color: #EEEEEE; font-size: 12px; padding: 0 6px; }"
        "QPushButton:hover { background: #3E3E42; }"
        "QPushButton:pressed { background: #4E4E52; }"
    );
=======
    // 面包屑按钮样式：扁平化，无外框与背景底，仅悬停可见暗色背景
    btn->setStyleSheet(
        "QPushButton { background: transparent; border: none; outline: none; border-radius: 4px; "
        "              color: #CCCCCC; font-size: 12px; padding: 0 4px; }"
        "QPushButton:hover { background: #3E3E42; color: #FFFFFF; }"
        "QPushButton:pressed { background: #4E4E52; }"
    );
>>>>>>> REPLACE

### `src/ui/FilterPanel.cpp`
<<<<<<< SEARCH
    m_btnPin = new QPushButton(topBar);
    m_btnPin->setFixedSize(24, 24);
    m_btnPin->setIcon(UiHelper::getIcon("pin_tilted", QColor("#B0B0B0")));
    m_btnPin->setIconSize(QSize(18, 18));
=======
    m_btnPin = new QPushButton(topBar);
    m_btnPin->setFixedSize(24, 24);
    m_btnPin->setIcon(UiHelper::getIcon("pin_tilted", QColor("#B0B0B0")));
    m_btnPin->setIconSize(QSize(16, 16));
>>>>>>> REPLACE

<<<<<<< SEARCH
    m_btnToggleGroups = new QPushButton(topBar);
    m_btnToggleGroups->setFixedSize(24, 24);
=======
    m_btnToggleGroups = new QPushButton(topBar);
    m_btnToggleGroups->setFixedSize(24, 24);
    m_btnToggleGroups->setIconSize(QSize(16, 16));
>>>>>>> REPLACE

## 4. Build & Verification Steps
1. 检查代码改动，确保符合 API 冻结原则与图标治理规范。
2. 运行应用验证地址栏面包屑路径展现，确认节点不再显示白色底框且悬停正常；验证筛选栏标题区图钉与折叠按钮渲染正常。
