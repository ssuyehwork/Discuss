# Implementation Plan - Remove Empty Filter Hint Label (DualSectionPanel-RemoveEmptyHint.md)

## 1. Overview
本实施方案响应用户需求，彻底物理移除 `DualSectionPanel` 中“所有内容已被筛选隐藏”的提示 Label（`m_emptyFilterHintLabel`）及其判定函数 `updateEmptyFilterHint()`。

---

## 2. Modified Files List
- `src/ui/DualSectionPanel.h`
- `src/ui/DualSectionPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/DualSectionPanel.h`
```
<<<<<<< SEARCH
    int computeFileViewMinHeight(int hostViewportHeight) const;
    int computeFolderViewMinHeight(int hostViewportHeight) const;
    void updateEmptyFilterHint();

    QVBoxLayout* m_layout = nullptr;
    FolderSectionHeaderBar* m_folderHeader = nullptr;
    FileSectionHeaderBar* m_fileHeader = nullptr;
    QAbstractItemView* m_folderView = nullptr;
    QAbstractItemView* m_fileView = nullptr;
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;
    QLabel* m_emptyFilterHintLabel = nullptr;
    int m_lastHostViewportHeight = 0;
=======
    int computeFileViewMinHeight(int hostViewportHeight) const;
    int computeFolderViewMinHeight(int hostViewportHeight) const;

    QVBoxLayout* m_layout = nullptr;
    FolderSectionHeaderBar* m_folderHeader = nullptr;
    FileSectionHeaderBar* m_fileHeader = nullptr;
    QAbstractItemView* m_folderView = nullptr;
    QAbstractItemView* m_fileView = nullptr;
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;
    int m_lastHostViewportHeight = 0;
>>>>>>> REPLACE
```

### File 2: `src/ui/DualSectionPanel.cpp`
```
<<<<<<< SEARCH
    if (m_fileView) {
        m_fileView->setParent(this);
        m_layout->addWidget(m_fileView, 1);
    }

    // 🚀 筛选后全隐藏提示（原 ColumnViewPane 独有，现统一给三种视图）
    m_emptyFilterHintLabel = new QLabel(this);
    m_emptyFilterHintLabel->setAlignment(Qt::AlignCenter);
    m_emptyFilterHintLabel->setWordWrap(true);
    m_emptyFilterHintLabel->setStyleSheet("color: #888888; font-size: 12px; padding: 16px;");
    m_emptyFilterHintLabel->hide();
    m_layout->addWidget(m_emptyFilterHintLabel, 0);

    connect(m_folderHeader, &FolderSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
=======
    if (m_fileView) {
        m_fileView->setParent(this);
        m_layout->addWidget(m_fileView, 1);
    }

    connect(m_folderHeader, &FolderSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
int DualSectionPanel::computeFolderViewMinHeight(int hostViewportHeight) const {
    int used = 0;
    if (m_folderHeader && m_folderHeader->isVisible()) used += m_folderHeader->height();
    return qMax(0, hostViewportHeight - used);
}

void DualSectionPanel::updateEmptyFilterHint() {
    if (!m_emptyFilterHintLabel || !m_folderProxyModel || !m_fileProxyModel) return;
    bool folderEmpty = m_folderProxyModel->rowCount() == 0;
    bool fileEmpty = m_fileProxyModel->rowCount() == 0;

    if (folderEmpty && fileEmpty) {
        m_emptyFilterHintLabel->setText("所有内容已被筛选隐藏");
        m_emptyFilterHintLabel->show();
    } else {
        m_emptyFilterHintLabel->hide();
    }
}

void DualSectionPanel::updateSectionCounts(int hostViewportHeight) {
=======
int DualSectionPanel::computeFolderViewMinHeight(int hostViewportHeight) const {
    int used = 0;
    if (m_folderHeader && m_folderHeader->isVisible()) used += m_folderHeader->height();
    return qMax(0, hostViewportHeight - used);
}

void DualSectionPanel::updateSectionCounts(int hostViewportHeight) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    if (m_fileView) {
        if (fileCount == 0) {
            m_fileView->hide();
        } else {
            m_fileView->show();
        }
    }

    updateEmptyFilterHint();
}
=======
    if (m_fileView) {
        if (fileCount == 0) {
            m_fileView->hide();
        } else {
            m_fileView->show();
        }
    }
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. 编译代码并启动程序。
2. 筛选使所有项目隐藏，验证视口中不再出现“所有内容已被筛选隐藏”的文字提示 Label。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 彻底物理删除了已废弃的提示 Label 代码，无死代码残留。

---

## 6. Header API Signature Verification
- `DualSectionPanel` 构造函数与公有 Getter 保持完全一致。

---

## 7. Header Inclusion Chain & Type Completeness Check
- `DualSectionPanel.h` / `DualSectionPanel.cpp` 类型完整且编译安全。
