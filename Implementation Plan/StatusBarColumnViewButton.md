# Implementation Plan: Status Bar Column View Button (StatusBarColumnViewButton.md)

## 1. Overview
Add a new **"列视图" (Column View)** button (`m_btnToggleColumn`) in the status bar of `MainWindow`, positioned immediately to the left of the **"自适应" (Justified View)** button (`m_btnToggleJustified`).
The button uses the icon key `"column_view"`, has a tooltip `"列视图(C)"`, and is initially hooked up to a TODO placeholder handler upon click.

## 2. Modified Files List
- `src/ui/SvgIcons.h` (Add `"column_view"` SVG icon definition)
- `src/ui/MainWindow.h` (Declare `QPushButton* m_btnToggleColumn`)
- `src/ui/MainWindow.cpp` (Instantiate `m_btnToggleColumn`, add to status bar layout before `m_btnToggleJustified`, and handle signal blocking / click callback)

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/SvgIcons.h`

<<<<<<< SEARCH
        {"resize2", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="2"/><path d="M15 3v18"/><path d="M9 3v18"/></svg>)svg"},
=======
        {"column_view", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="2"/><line x1="9" y1="3" x2="9" y2="21"/><line x1="15" y1="3" x2="15" y2="21"/></svg>)svg"},
        {"resize2", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="2"/><path d="M15 3v18"/><path d="M9 3v18"/></svg>)svg"},
>>>>>>> REPLACE

### 3.2 `src/ui/MainWindow.h`

<<<<<<< SEARCH
    QPushButton* m_btnToggleJustified = nullptr;
=======
    QPushButton* m_btnToggleColumn   = nullptr;
    QPushButton* m_btnToggleJustified = nullptr;
>>>>>>> REPLACE

### 3.3 `src/ui/MainWindow.cpp`

<<<<<<< SEARCH
    m_btnToggleJustified = createSquareStatusBtn("resize2", "自适应(A)");
=======
    m_btnToggleColumn    = createSquareStatusBtn("column_view", "列视图(C)");
    m_btnToggleJustified = createSquareStatusBtn("resize2", "自适应(A)");
>>>>>>> REPLACE

<<<<<<< SEARCH
    statusL->setSpacing(4);
    statusL->addWidget(m_btnToggleJustified);
=======
    statusL->setSpacing(4);
    statusL->addWidget(m_btnToggleColumn);
    statusL->addWidget(m_btnToggleJustified);
>>>>>>> REPLACE

<<<<<<< SEARCH
    connect(m_btnToggleJustified, &QPushButton::clicked, this, [this]() {
=======
    connect(m_btnToggleColumn, &QPushButton::clicked, this, [this]() {
        // TODO: Implement column view mode switching when backend logic is ready
        if (m_statusLeft) {
            m_statusLeft->setText("列视图 (功能尚未实现 - TODO)");
        }
    });

    connect(m_btnToggleJustified, &QPushButton::clicked, this, [this]() {
>>>>>>> REPLACE

<<<<<<< SEARCH
    QSignalBlocker b8(m_btnToggleJustified);
=======
    QSignalBlocker b_col(m_btnToggleColumn);
    QSignalBlocker b8(m_btnToggleJustified);
>>>>>>> REPLACE

<<<<<<< SEARCH
        if (m_btnToggleJustified) m_btnToggleJustified->setChecked(mode == ContentPanel::JustifiedViewMode);
=======
        if (m_btnToggleColumn) m_btnToggleColumn->setChecked(false);
        if (m_btnToggleJustified) m_btnToggleJustified->setChecked(mode == ContentPanel::JustifiedViewMode);
>>>>>>> REPLACE

## 4. Build & Verification Steps
1. Verify files modified via `git status` or `read_file`.
2. Run build via `cmake --build build` or equivalent.
3. Launch application and inspect the status bar to verify the "列视图" button appears immediately to the left of "自适应".
