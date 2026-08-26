# 全代码库僵尸 / 幽灵 / 孤儿代码彻底清退无脑实施方案 (Zombie Code Purge Implementation Plan)

## Overview（概述与解决的问题）
本实施方案依据 `Zombie Code.md` 中归档的诊断结果，对全代码库中确认的僵尸代码、幽灵状态标志、孤儿定时器、空桩函数与废弃数据字段进行一次性彻底物理清退：
1. **清退 `MainWindow` 僵尸**：彻底剔除幽灵状态标志 `m_isTagManagerMode`、孤儿定时器与进度条变量 (`m_topProgressBar`, `m_elapsedTimer`, `m_syncStartTime`, `m_totalBatchCount`)、协议常量 `kProtocolSystem`、空事件槽 `onDriveBarContextMenu` 以及重复事件过滤器安装。
2. **清退 `ContentPanel` 与 Delegate 僵尸**：移除 `ContentPanel` 对越界列索引 7 的隐藏调用，修复 `m_isPendingEdit` 状态复位遗漏，清理 `ThumbnailDelegate::helpEvent` 中的悬空计算，清理 `CardPainterHelper::drawRatingStars` 中被 `Q_UNUSED` 的废弃参数 `starSpacing`。
3. **清退数据模型与服务层僵尸**：彻底清理 `ItemRecord::isManaged`、`ItemRecord::thumbStatus`、`RuntimeMeta::thumbStatus` 僵尸字段，移除 `MetadataManager::initFromDatabase` 中的打桩虚假逻辑，并清理 `DiskItemModel::m_iconCache` 冗余缓存。

---

## Modified Files List（影响文件清单）

### 影响的源文件 (Modified Files)
- `src/core/ItemRecord.h`
- `src/meta/MetadataDefs.h`
- `src/meta/MetadataManager.h`
- `src/meta/MetadataManager.cpp`
- `src/ui/CardPainterHelper.h`
- `src/ui/CardPainterHelper.cpp`
- `src/ui/ContentPanel.cpp`
- `src/ui/MainWindow.h`
- `src/ui/MainWindow.cpp`
- `src/ui/ThumbnailDelegate.cpp`
- `src/ui/models/DiskItemModel.h`
- `src/ui/models/DiskItemModel.cpp`

---

## Detailed Line-by-Line Changes（包含 Precise Git Merge Diff 替换块）

### 1. `src/ui/MainWindow.h` 清理幽灵标志、孤儿变量与协议常量
```
<<<<<<< SEARCH
    // 常量定义
    static constexpr const char* kProtocolSystem = "system://";

    // 界面与模式状态
    bool m_isTagManagerMode = false;

    // 进度与时间管理
    QProgressBar* m_topProgressBar = nullptr;
    QElapsedTimer m_elapsedTimer;
    qint64 m_syncStartTime = 0;
    int m_totalBatchCount = 0;
=======
    // 界面与模式状态已物理纯化，消除 m_isTagManagerMode、m_topProgressBar 等幽灵变量
>>>>>>> REPLACE
```

### 2. `src/ui/MainWindow.cpp` 清理空槽函数与重复过滤器安装
```
<<<<<<< SEARCH
void MainWindow::onDriveBarContextMenu(const QPoint& pos) {
    // 空桩函数
}
=======
// 空桩函数 onDriveBarContextMenu 已彻底清理
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 2. 依次安装通用悬停过滤器
    m_btnBack->installEventFilter(m_hoverFilter);
    m_btnForward->installEventFilter(m_hoverFilter);
    m_btnUp->installEventFilter(m_hoverFilter);
=======
    // 2. 移除重复安装的 m_hoverFilter，统一由基类进行事件拦截
>>>>>>> REPLACE
```

### 3. `src/ui/ContentPanel.cpp` 清理越界隐藏列与复位遗漏
```
<<<<<<< SEARCH
    // 隐藏无需展示的列
    header->setSectionHidden(7, true);
=======
    // 索引 7 越界隐藏已清理（模型最大列数硬编码为 6）
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    if (m_isPendingEdit) {
        view->edit(lastProxyIdx);
    }
=======
    if (m_isPendingEdit) {
        view->edit(lastProxyIdx);
        m_isPendingEdit = false; // 严密复位状态，防止后续意外误触发编辑态
    }
>>>>>>> REPLACE
```

### 4. `src/ui/ThumbnailDelegate.cpp` 清理悬空计算
```
<<<<<<< SEARCH
bool ThumbnailDelegate::helpEvent(QHelpEvent* event, QAbstractItemView* view,
                                 const QStyleOptionViewItem& option, const QModelIndex& index) {
    Metrics m = getMetrics(option, index);
    QRect statusRect = m.statusRect;
    return QStyledItemDelegate::helpEvent(event, view, option, index);
}
=======
bool ThumbnailDelegate::helpEvent(QHelpEvent* event, QAbstractItemView* view,
                                 const QStyleOptionViewItem& option, const QModelIndex& index) {
    // 移除无用局部计算，直接透传
    return QStyledItemDelegate::helpEvent(event, view, option, index);
}
>>>>>>> REPLACE
```

### 5. `src/core/ItemRecord.h` 清理僵尸字段
```
<<<<<<< SEARCH
    bool isManaged = false;        // 历史托管库时代标志
    int thumbStatus = 0;           // 0:正常, 1:失败
=======
    // 彻底清除 isManaged 与 thumbStatus 历史僵尸字段
>>>>>>> REPLACE
```

### 6. `src/meta/MetadataManager.cpp` 清理打桩虚假逻辑
```
<<<<<<< SEARCH
void MetadataManager::initFromDatabase() {
    DatabaseManager::instance().init();
    m_loaded = true;
}
=======
void MetadataManager::initFromDatabase() {
    DatabaseManager::instance().init();
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps（编译命令与验证方法）

1. **编译验证**：
   在 sandbox 中运行 CMake 编译命令，验证移除无用变量与桩函数后代码 100% 编译通过：
   ```bash
   cmake -B build
   cmake --build build --config Release
   ```

2. **功能与状态验证**：
   - 验证 `MainWindow` 顶栏与面板显隐切换无任何异常，控制台不再出现越界 Section 隐藏提示。
   - 验证选中视图回复后不会错误偶发留存 `m_isPendingEdit` 编辑态。
   - 检查全工程构建无任何警告。
