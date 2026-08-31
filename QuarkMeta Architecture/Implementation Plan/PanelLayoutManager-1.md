# QuarkMeta 架构重构方案：PanelLayoutManager 分栏间距复原

## 一、 Overview
彻底复原 Version-1/2 标准设计的 5 像素分栏间距（`kSplitterHandleWidth = 5`），纠正硬编码为 1 像素导致布局紧凑混乱的问题。

## 二、 Modified Files List
- `src/ui/MainWindow.cpp`
- `src/ui/PanelLayoutManager.h`

## 三、 Detailed Line-by-Line Changes

### 1. `src/ui/MainWindow.cpp`
```git
<<<<<<< SEARCH
    m_mainSplitter = new QSplitter(Qt::Horizontal, bodyWrapper);
    m_mainSplitter->setHandleWidth(1); 
    m_mainSplitter->setChildrenCollapsible(false);
    m_mainSplitter->setStyleSheet(QString(
        "QSplitter { background: transparent; border: none; spacing: 0px; }"
        "QSplitter::handle { background-color: %1; width: 1px; margin: 0px; padding: 0px; }"
        "QSplitter::handle:hover { background-color: %2; }"
    ).arg(qssColor(BackgroundDeep)).arg(qssColor(PrimaryBlue)));
=======
    m_mainSplitter = new QSplitter(Qt::Horizontal, bodyWrapper);
    m_mainSplitter->setHandleWidth(5); 
    m_mainSplitter->setChildrenCollapsible(false);
    m_mainSplitter->setStyleSheet(QString(
        "QSplitter { background: transparent; border: none; }"
        "QSplitter::handle { background-color: transparent; width: 5px; }"
        "QSplitter::handle:hover { background-color: %1; }"
    ).arg(qssColor(PrimaryBlue)));
>>>>>>> REPLACE
```

### 2. `src/ui/PanelLayoutManager.h`
```git
<<<<<<< SEARCH
    static constexpr int kSplitterHandleWidth = 1;
=======
    static constexpr int kSplitterHandleWidth = 5;
>>>>>>> REPLACE
```

## 四、 Build & Verification Steps
1. 确认 `src/ui/MainWindow.cpp` 与 `src/ui/PanelLayoutManager.h` 已复原 5px 间距。
2. 确认主窗口分割条宽为 5px，悬停高亮展示 PrimaryBlue 提示框。
