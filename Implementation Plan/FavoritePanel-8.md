# Implementation Plan - FavoritePanel-8

## 1. Overview（概述与解决的问题）

### 解决的问题
在收藏栏（`FavoritePanel`）中，当用户收藏普通物理文件（如 `.docx`, `.pdf`, `.zip`, `.exe`, `.mp4` 等非图形文件，或未生成自定义缩略图的文件）时，这些文件没有正确显示其在 Windows 操作系统中对应的程序图标与类型图标。

### 根因分析
1. **异步图标加载后缺乏通知刷新**：
   - 当 `FavoritePanel::loadFavorites()` 加载文件条目时，调用 `ShellIconManager::getFileIcon(nativePath)` 获取图标；
   - 内部实现（`WindowsShellThumbnailProvider::getFileIcon`）为避免主线程卡顿，首次请求会返回默认占位图标，并在后台线程异步提取真实系统文件图标后发射 `IconLoadNotifier::instance().iconLoaded()` 全局信号；
   - 但 `FavoritePanel` **从未连接和监听该信号**！导致文件项图标永久停留在最初的占位状态，永远无法更新为已缓存的真实系统图标。
2. **缺乏异步图标批量刷新函数**：
   - 当系统图标加载完成或缩略图完成时，未提供一个针对树形结构各级节点中所有非自定义缩略图文件的图标刷新方法。
3. **层级项（如收藏夹子项文件）绘制的横向内边距计算**：
   - `DropTreeView` 中 `setIndentation(0)`，但 `FavoriteItemDelegate::paint` 绘制时采用的是固定 `leftMargin = 10`；
   - 当文件作为子节点被收纳在“虚拟分类/收藏夹”下方时，若存在展开缩进（或通过父级深度计算层次），需要根据节点深度保留适度层次，同时确保系统图标正确定位绘制。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/FavoritePanel.h`
2. `src/ui/FavoritePanel.cpp`

---

## 3. Detailed Line-by-Line Changes（包含精准替换块）

### 3.1 `src/ui/FavoritePanel.h`
在 `FavoritePanel` 类中声明 `refreshItemIcons()` 私有方法。

```
<<<<<<< SEARCH
    void initUi();
    void updateItemThumbnail(const QString& path, const QPixmap& pix);
    void createAndEditCategory(int parentId = 0);
=======
    void initUi();
    void refreshItemIcons();
    void updateItemThumbnail(const QString& path, const QPixmap& pix);
    void createAndEditCategory(int parentId = 0);
>>>>>>> REPLACE
```

---

### 3.2 `src/ui/FavoritePanel.cpp`
包含 `WindowsShellThumbnailProvider.h` 头文件，并在 `initUi()` 中连接 `IconLoadNotifier::iconLoaded` 信号，实现 `refreshItemIcons()`，并在代理绘制时支持基于层级深度的图标自适应布局。

#### 步骤 1：补全包含头文件
```
<<<<<<< SEARCH
#include "ShellIconManager.h"
#include "../util/DiskMediaExtractor.h"
=======
#include "ShellIconManager.h"
#include "WindowsShellThumbnailProvider.h"
#include "../util/DiskMediaExtractor.h"
>>>>>>> REPLACE
```

#### 步骤 2：优化 `FavoriteItemDelegate::paint` 绘制几何计算（支持子项层级偏移）
```
<<<<<<< SEARCH
    // 几何布局
    int leftMargin = 10;
    int iconSize = 18;
    int spacing = 8;

    QRect iconRect(opt.rect.left() + leftMargin, opt.rect.top() + (opt.rect.height() - iconSize) / 2, iconSize, iconSize);
    QRect textRect(iconRect.right() + spacing, opt.rect.top(), opt.rect.width() - leftMargin - iconSize - spacing, opt.rect.height());
=======
    // 几何布局（根据树层级深度计算缩进，保证子节点图标与文本排版层次清晰）
    int depth = 0;
    QModelIndex parentIdx = index.parent();
    while (parentIdx.isValid()) {
        depth++;
        parentIdx = parentIdx.parent();
    }

    int leftMargin = 10 + depth * 14;
    int iconSize = 18;
    int spacing = 8;

    QRect iconRect(opt.rect.left() + leftMargin, opt.rect.top() + (opt.rect.height() - iconSize) / 2, iconSize, iconSize);
    QRect textRect(iconRect.right() + spacing, opt.rect.top(), opt.rect.width() - leftMargin - iconSize - spacing, opt.rect.height());
>>>>>>> REPLACE
```

#### 步骤 3：在 `FavoritePanel::initUi` 中连接 `IconLoadNotifier::iconLoaded` 信号
```
<<<<<<< SEARCH
    connect(&FavoriteService::instance(), &FavoriteService::favoritesReloaded, this, [this]() {
        loadFavorites();
    });
}
=======
    connect(&FavoriteService::instance(), &FavoriteService::favoritesReloaded, this, [this]() {
        loadFavorites();
    });

    // 🚀【系统图标异步回写】：当后台线程完成文件真实扩展名/程序图标提取时，即时刷新收藏项图标
    connect(&IconLoadNotifier::instance(), &IconLoadNotifier::iconLoaded, this, &FavoritePanel::refreshItemIcons, Qt::QueuedConnection);
}
>>>>>>> REPLACE
```

#### 步骤 4：在 `FavoritePanel.cpp` 中实现 `refreshItemIcons`
```
<<<<<<< SEARCH
void FavoritePanel::updateItemThumbnail(const QString& path, const QPixmap& pix) {
=======
void FavoritePanel::refreshItemIcons() {
    if (!m_favoriteModel || m_isLoading) return;

    std::function<void(QStandardItem*)> updateIconsRecursive = [&](QStandardItem* parentItem) {
        int count = parentItem ? parentItem->rowCount() : m_favoriteModel->rowCount();
        for (int i = 0; i < count; ++i) {
            QStandardItem* item = parentItem ? parentItem->child(i) : m_favoriteModel->item(i);
            if (!item) continue;

            bool isDir = item->data(Qt::UserRole + 4).toBool();
            bool hasCustomThumb = item->data(Qt::UserRole + 5).toBool();
            bool isVirtual = item->data(Qt::UserRole + 7).toBool();

            // 仅对非文件夹、且尚未被自定义大缩略图接管的普通文件更新真实系统文件图标
            if (!isDir && !isVirtual && !hasCustomThumb) {
                QString itemPath = item->data(Qt::UserRole + 1).toString();
                if (!itemPath.isEmpty()) {
                    QIcon newIcon = ShellIconManager::getFileIcon(itemPath);
                    if (!newIcon.isNull()) {
                        item->setIcon(newIcon);
                    }
                }
            }

            if (item->hasChildren()) {
                updateIconsRecursive(item);
            }
        }
    };

    updateIconsRecursive(nullptr);
    if (m_favoriteView && m_favoriteView->viewport()) {
        m_favoriteView->viewport()->update();
    }
}

void FavoritePanel::updateItemThumbnail(const QString& path, const QPixmap& pix) {
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. 执行工程编译：
   ```powershell
   cmake --build . --config Release --target QuarkMeta
   ```
2. 验证场景：
   - 在主界面将各类文件（如 `.docx`, `.pdf`, `.zip`, `.exe`, `.txt` 等）拖放或收藏至收藏夹下方；
   - 检查收藏夹中的文件是否正确且即时显示出对应的 Windows 系统文件类型图标；
   - 检查将文件放入收藏夹虚拟分组（子级）时，子项文件的图标与文本是否具有清晰的层次缩进，且图标显示完好。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）
- **复用官方既有机制**：统一通过 `ShellIconManager::getFileIcon(nativePath)` 与 `IconLoadNotifier::instance().iconLoaded` 通道，不重复编写底层图标抓取。
- **无破坏性修改**：对既有的虚拟文件夹颜色、自定义缩略图提取通道均保持 100% 兼容。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用点 | 头文件路径 | 物理精确签名 |
| :--- | :--- | :--- |
| `IconLoadNotifier::instance` | `src/ui/WindowsShellThumbnailProvider.h` | `static IconLoadNotifier& instance();` |
| `IconLoadNotifier::iconLoaded` | `src/ui/WindowsShellThumbnailProvider.h` | `signals: void iconLoaded();` |
| `ShellIconManager::getFileIcon` | `src/ui/ShellIconManager.h` | `static inline QIcon getFileIcon(const QString& filePath, int size = 18);` |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

- 在 `FavoritePanel.cpp` 中补充了 `#include "WindowsShellThumbnailProvider.h"`，以保证 `IconLoadNotifier` 具有完整类型定义，杜绝 `C2027` 不完整类型错误；
- `FavoritePanel.h` 仅新增了私有成员函数签名 `void refreshItemIcons();`，无需引入新的头文件。
