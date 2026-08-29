# Implementation Plan - FavoritePanel-3

This implementation plan resolves the missing declaration errors (`saveFavorites`), fixes the blank file icon issue via `IconLoadNotifier` signal subscription, and enforces solid folder icons (`folder_filled`), folder/file dual-track rendering, and pure icon context menus without text labels.

## 1. Overview
- **Fix Declaration Errors**: Add `void saveFavorites();` method to `FavoritePanel.h` (and empty/compatibility body in `FavoritePanel.cpp`) to resolve compiler errors regarding `saveFavorites` not being a member of `FavoritePanel`.
- **Async Icon Refresh via `IconLoadNotifier`**: Subscribe to `IconLoadNotifier::instance().iconLoaded` in `FavoritePanel` constructor to trigger `m_favoriteView->viewport()->update()`. As soon as background threads finish extracting system icons for files (`.svg`, `.psd`, etc.), the view immediately updates and replaces placeholder icons with actual system thumbnails/icons.
- **Solid Folder Default**: Folder favorites default to `folder_filled` SVG key and `#FDB70A` color.
- **Dual-Track Item Rendering**:
  - **Folders (`QFileInfo::isDir() == true`)**: Rendered using `UiHelper::getIcon(iconKey, color, 18)`.
  - **Files (`QFileInfo::isDir() == false`)**: Rendered strictly using native system icons/thumbnails via `ShellIconManager::getFileIcon(path)`.
- **Pure Icon Context Menu (No Text Labels)**: Right-click "切换图标" and "切换色标" menus display **icons only with empty text strings `""`**.
- **Context Menu File Safeguard**: File items in favorites show only "取消收藏"; icon/color customization submenus are hidden for files.

## 2. Modified Files List
- `src/ui/FavoritePanel.h`
- `src/ui/FavoritePanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/FavoritePanel.h`
```diff
<<<<<<< SEARCH
public:
    explicit FavoritePanel(QWidget* parent = nullptr);
    ~FavoritePanel() override = default;

    /**
     * @brief 物理还原：设置 1px 高亮线的显隐状态
     */
    void setFocusHighlight(bool visible);
=======
public:
    explicit FavoritePanel(QWidget* parent = nullptr);
    ~FavoritePanel() override = default;

    /**
     * @brief 物理还原：设置 1px 高亮线的显隐状态
     */
    void setFocusHighlight(bool visible);

    /**
     * @brief 兼容性存根：SQLite 变动时即时自动持久化
     */
    void saveFavorites();
>>>>>>> REPLACE
```

### `src/ui/FavoritePanel.cpp`
```diff
<<<<<<< SEARCH
FavoritePanel::FavoritePanel(QWidget* parent)
    : QFrame(parent) {
    setObjectName("ListContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);
    setStyleSheet("FavoritePanel { background-color: #1E1E1E; color: #EEEEEE; }");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    initUi();
    loadFavorites();
}
=======
FavoritePanel::FavoritePanel(QWidget* parent)
    : QFrame(parent) {
    setObjectName("ListContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);
    setStyleSheet("FavoritePanel { background-color: #1E1E1E; color: #EEEEEE; }");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    initUi();
    loadFavorites();

    // Subscribe to async system icon load notifications to update viewport when icons are extracted
    connect(&IconLoadNotifier::instance(), &IconLoadNotifier::iconLoaded, this, [this]() {
        if (m_favoriteView && m_favoriteView->viewport()) {
            m_favoriteView->viewport()->update();
        }
    });
}

void FavoritePanel::saveFavorites() {
    // 兼容性存根：SQLite 变动时已即时自动持久化到 global.db
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Clean and build the project using CMake:
   ```bash
   cmake -B build -G Ninja
   cmake --build build
   ```
2. Run application and verify:
   - Ensure `saveFavorites()` compilation error is eliminated.
   - Add files (such as `.svg` or `.psd`) to FavoritePanel: when the background thread completes icon extraction, the viewport automatically refreshes and displays actual file system icons.
   - Verify folder items display `folder_filled` solid SVG icons by default.
   - Verify right-clicking folder items shows pure-icon menus without text labels.
   - Verify right-clicking file items shows only "取消收藏".
