# Implementation Plan - Zombie Code Cleanup

## Overview
Purge zombie code, ghost declarations, unreachable code, and obsolete architecture remnants identified in Task 1 from `ContentPanel.h` and `ContentPanel.cpp`.

## Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.h`

#### Cleanup unused headers and ghost forward declaration
<<<<<<< SEARCH
#include <unordered_map>
#include <deque>
#include <vector>
#include <QCache>
#include <QList>
#include <QStringList>
#include <QTimer>
#include <QWidget>
#include <QListView>
#include <QTreeView>
#include <QStackedWidget>
#include <QPushButton>
#include <QTextBrowser>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
=======
#include <unordered_map>
#include <vector>
#include <QList>
#include <QStringList>
#include <QTimer>
#include <QWidget>
#include <QListView>
#include <QTreeView>
#include <QStackedWidget>
#include <QPushButton>
#include <QSortFilterProxyModel>
>>>>>>> REPLACE

<<<<<<< SEARCH
namespace QuarkMeta {

struct RuntimeMeta;

/**
=======
namespace QuarkMeta {

/**
>>>>>>> REPLACE

#### Remove obsolete DataSourceType::SystemCategory
<<<<<<< SEARCH
    enum class DataSourceType {
        DiskNav,        // 物理磁盘导航模式
        SystemCategory, // 系统逻辑桶 (全部数据, 未标记, 回收站, 最近访问)
        PathList        // 临时路径列表 (搜索结果, 标签筛选)
    };
=======
    enum class DataSourceType {
        DiskNav,        // 物理磁盘导航模式
        PathList        // 临时路径列表 (搜索结果, 标签筛选)
    };
>>>>>>> REPLACE

#### Remove ghost method declaration `setupContextMenu()`
<<<<<<< SEARCH
    void initListView();
    void setupContextMenu();
    void updateLayersButtonState();
=======
    void initListView();
    void updateLayersButtonState();
>>>>>>> REPLACE

#### Clean parameter signature in `resolvePasteDestination`
<<<<<<< SEARCH
    bool resolvePasteDestination(int& outCatId);
=======
    bool resolvePasteDestination();
>>>>>>> REPLACE


### 2. `src/ui/ContentPanel.cpp`

#### Remove duplicate includes
<<<<<<< SEARCH
#include "../meta/MetadataManager.h"
#include "../meta/TagManager.h"
=======
#include "../meta/MetadataManager.h"
#include "../meta/TagManager.h"
>>>>>>> REPLACE

<<<<<<< SEARCH
#include <QAbstractItemView>
#include <QHeaderView>
=======
#include <QHeaderView>
>>>>>>> REPLACE

<<<<<<< SEARCH
#include <QTextBrowser>
#include <QAbstractItemView>
#include <QJsonDocument>
=======
#include <QJsonDocument>
>>>>>>> REPLACE

<<<<<<< SEARCH
#include "../meta/MetadataManager.h"
#include "StyleLibrary.h"
=======
#include "StyleLibrary.h"
>>>>>>> REPLACE

#### Fix unreachable code in `FilterProxyModel::lessThan`
<<<<<<< SEARCH
    // 5. 物理第三权重：具体的排序类型逻辑，平局时统一追加二级决胜键 (localeAwareCompare 拼音/文件名)
    auto* contentPanel = qobject_cast<ContentPanel*>(parent());
    ContentPanel::SortType sType = contentPanel ? contentPanel->currentSortType() : ContentPanel::SortByName;

    if (sType == ContentPanel::SortByAddedDate) {
        sType = ContentPanel::SortByName;
    }

    auto compareNames = [](const ItemRecord& l, const ItemRecord& r) {
=======
    // 5. 物理第三权重：具体的排序类型逻辑，平局时统一追加二级决胜键 (localeAwareCompare 拼音/文件名)
    auto* contentPanel = qobject_cast<ContentPanel*>(parent());
    ContentPanel::SortType sType = contentPanel ? contentPanel->currentSortType() : ContentPanel::SortByName;

    auto compareNames = [](const ItemRecord& l, const ItemRecord& r) {
>>>>>>> REPLACE

#### Update `resolvePasteDestination` call & signature
<<<<<<< SEARCH
    int targetCatId = 0;
    if (!resolvePasteDestination(targetCatId)) return; // 内部已完成提示/取消处理
=======
    if (!resolvePasteDestination()) return; // 内部已完成提示/取消处理
>>>>>>> REPLACE

<<<<<<< SEARCH
bool ContentPanel::resolvePasteDestination(int& outCatId) {
    Q_UNUSED(outCatId);
    if (m_currentCategoryType == "trash") {
=======
bool ContentPanel::resolvePasteDestination() {
    if (m_currentCategoryType == "trash") {
>>>>>>> REPLACE

## Build & Verification Steps
1. Verify `ContentPanel.h` and `ContentPanel.cpp` code changes.
2. Build the project using CMake if environment permits.
