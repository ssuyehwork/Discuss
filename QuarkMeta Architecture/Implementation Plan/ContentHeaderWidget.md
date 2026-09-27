# ContentHeaderWidget Implementation Plan (移除文件夹与文件切换按钮)

## Overview
本实施方案旨在按照最新架构与 UI 需求，物理移除 `ContentHeaderWidget` 中 redundant 的 **`m_btnToggleFolders`（显示/隐藏文件夹按钮）** 与 **`m_btnToggleFiles`（显示/隐藏文件按钮）** 成员及相关 UI 初始化/事件响应代码，仅保留 **`m_btnToggleHidden`** 与 **`m_btnLayers`**。

---

## Modified Files List
1. `src/ui/ContentHeaderWidget.h`
2. `src/ui/ContentHeaderWidget.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentHeaderWidget.h`

<<<<<<< SEARCH
    QPushButton* m_btnLayers = nullptr;
    QPushButton* m_btnToggleHidden = nullptr;
    QPushButton* m_btnToggleFolders = nullptr;
    QPushButton* m_btnToggleFiles = nullptr;
=======
    QPushButton* m_btnLayers = nullptr;
    QPushButton* m_btnToggleHidden = nullptr;
>>>>>>> REPLACE

---

### 2. `src/ui/ContentHeaderWidget.cpp`

<<<<<<< SEARCH
    setupToggleBtn(m_btnToggleHidden, "eye", QColor("#3498db"), m_filterState.showHidden, "显示/隐藏隐藏项目");
    connect(m_btnToggleHidden, &QPushButton::clicked, this, [this]() {
        m_filterState.showHidden = m_btnToggleHidden->isChecked();
        m_btnToggleHidden->setIcon(UiHelper::getIcon("eye", m_filterState.showHidden ? QColor("#3498db") : QColor("#888888"), 16));
        emit filterStateChanged(m_filterState);
    });

    setupToggleBtn(m_btnToggleFolders, "folder_filled", QColor("#FDB70A"), m_filterState.showFolders, "显示/隐藏文件夹");
    connect(m_btnToggleFolders, &QPushButton::clicked, this, [this]() {
        m_filterState.showFolders = m_btnToggleFolders->isChecked();
        m_btnToggleFolders->setIcon(UiHelper::getIcon("folder_filled", m_filterState.showFolders ? QColor("#FDB70A") : QColor("#B0B0B0"), 16));
        emit filterStateChanged(m_filterState);
    });

    setupToggleBtn(m_btnToggleFiles, "file", QColor("#2ecc71"), m_filterState.showFiles, "显示/隐藏文件");
    connect(m_btnToggleFiles, &QPushButton::clicked, this, [this]() {
        m_filterState.showFiles = m_btnToggleFiles->isChecked();
        m_btnToggleFiles->setIcon(UiHelper::getIcon("file", m_filterState.showFiles ? QColor("#2ecc71") : QColor("#B0B0B0"), 16));
        emit filterStateChanged(m_filterState);
    });

    m_btnLayers = new QPushButton(this);
=======
    setupToggleBtn(m_btnToggleHidden, "eye", QColor("#3498db"), m_filterState.showHidden, "显示/隐藏隐藏项目");
    connect(m_btnToggleHidden, &QPushButton::clicked, this, [this]() {
        m_filterState.showHidden = m_btnToggleHidden->isChecked();
        m_btnToggleHidden->setIcon(UiHelper::getIcon("eye", m_filterState.showHidden ? QColor("#3498db") : QColor("#888888"), 16));
        emit filterStateChanged(m_filterState);
    });

    m_btnLayers = new QPushButton(this);
>>>>>>> REPLACE

<<<<<<< SEARCH
void ContentHeaderWidget::setFilterState(const FilterState& state) {
    m_filterState = state;
    if (m_btnToggleHidden) {
        m_btnToggleHidden->setChecked(state.showHidden);
        m_btnToggleHidden->setIcon(UiHelper::getIcon("eye", state.showHidden ? QColor("#3498db") : QColor("#888888"), 16));
    }
    if (m_btnToggleFolders) {
        m_btnToggleFolders->setChecked(state.showFolders);
        m_btnToggleFolders->setIcon(UiHelper::getIcon("folder_filled", state.showFolders ? QColor("#FDB70A") : QColor("#B0B0B0"), 16));
    }
    if (m_btnToggleFiles) {
        m_btnToggleFiles->setChecked(state.showFiles);
        m_btnToggleFiles->setIcon(UiHelper::getIcon("file", state.showFiles ? QColor("#2ecc71") : QColor("#B0B0B0"), 16));
    }
}
=======
void ContentHeaderWidget::setFilterState(const FilterState& state) {
    m_filterState = state;
    if (m_btnToggleHidden) {
        m_btnToggleHidden->setChecked(state.showHidden);
        m_btnToggleHidden->setIcon(UiHelper::getIcon("eye", state.showHidden ? QColor("#3498db") : QColor("#888888"), 16));
    }
}
>>>>>>> REPLACE

---

## Build & Verification Steps
1. 检查物理文件内容无冗余引用。
2. 运行应用，确认内容面板标题栏右上侧仅保留“显示/隐藏隐藏项目”与“递归穿透”两个按钮，界面布局整洁美观。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- 移除了不需要的旧按钮及冗余字段映射，彻底防止按钮功能重叠。

---

## Header API Signature Verification
- `ContentHeaderWidget::setFilterState(const FilterState&)`: 准确匹配
- `ContentHeaderWidget::setRecursive(bool)`: 准确匹配
- `ContentHeaderWidget::setLayersEnabled(bool, const QString&)`: 准确匹配