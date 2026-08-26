# Implementation Plan - Add Context Menu to Breadcrumb Bar Level Buttons for Favorite Toggling

## 1. Overview
This implementation plan adds custom context menu support to each level button in `BreadcrumbBar`. Right-clicking any folder level in the address bar will pop up a context menu that dynamically checks if the folder path is in `FavoritePanel`, presenting either "添加至收藏夹" or "取消收藏".

## 2. Modified Files List
- `src/ui/BreadcrumbBar.h`
- `src/ui/BreadcrumbBar.cpp`

## 3. Detailed Line-by-Line Changes

```diff
<<<<<<< SEARCH
signals:
    void pathClicked(const QString& fullPath);
    void blankAreaClicked();
=======
signals:
    void pathClicked(const QString& fullPath);
    void blankAreaClicked();
    void favoriteToggleRequested(const QString& fullPath);
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    connect(btn, &QPushButton::clicked, [this, fullPath]() {
        emit pathClicked(fullPath);
    });

    m_layout->addWidget(btn);
=======
    btn->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(btn, &QPushButton::clicked, [this, fullPath]() {
        emit pathClicked(fullPath);
    });
    connect(btn, &QPushButton::customContextMenuRequested, [this, btn, fullPath](const QPoint& pos) {
        emit favoriteToggleRequested(fullPath);
    });

    m_layout->addWidget(btn);
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Rebuild application using CMake & Qt build pipeline.
2. Right-click any level button in the address bar breadcrumb.
3. Verify a context menu pops up displaying "添加至收藏夹" (or "取消收藏" if already favorited).
4. Click the action and verify the item is successfully added to or removed from `FavoritePanel`.
