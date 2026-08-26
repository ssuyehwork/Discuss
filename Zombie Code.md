# QuarkMeta 全代码库僵尸 / 幽灵 / 孤儿代码归档报告 (Zombie Code)

本文档专门提取并归档自 `Refactoring Implementation Plan translation.md` 及全量源码诊断中确认的**僵尸代码、幽灵状态标志、孤儿定时器、空桩函数与废弃数据字段**。所有条目均具备确凿的物理源码行凭证，留待后续集中清理与架构净化。

---

## 一、 `MainWindow` 模块僵尸与孤儿代码

1. **幽灵状态标志 `m_isTagManagerMode`**
   - **源码位置**：`src/ui/MainWindow.h`、`src/ui/MainWindow.cpp`
   - **描述**：在头文件中声明，在 `resetSplitterLayout` 和 `savePanelVisibility` 中被读取与重置，但全工程没有任何业务逻辑将其设为 `true`，属于典型的幽灵变量。

2. **孤儿定时器与进度条驱动变量**
   - **源码位置**：`src/ui/MainWindow.h` (`m_topProgressBar`, `m_elapsedTimer`, `m_syncStartTime`, `m_totalBatchCount`)
   - **描述**：存在初始化和扫描倒计时算法计算代码，但没有外部信号或业务方法对其进行启动、赋值与显示控制，属于悬空孤儿变量。

3. **未使用的协议常量 `kProtocolSystem`**
   - **源码位置**：`src/ui/MainWindow.h`
   - **描述**：头文件中定义的 `kProtocolSystem = "system://"` 在实现文件及全工程中无任何引用。

4. **空事件槽 `onDriveBarContextMenu`**
   - **源码位置**：`src/ui/MainWindow.cpp`
   - **描述**：`onDriveBarContextMenu(const QPoint& pos)` 为完全没有任何实现的空桩函数。

5. **重复事件过滤器安装**
   - **源码位置**：`src/ui/MainWindow.cpp` 中的 `initToolbar()`
   - **描述**：创建按钮时通过 Lambda 安装了 `this` 作为事件过滤器，紧接着又对 `m_btnBack` / `m_btnForward` / `m_btnUp` 重复安装了 `m_hoverFilter`，造成重复事件拦截开销。

---

## 二、 `ContentPanel` 与 Delegate 模块僵尸与废弃代码

1. **越界隐藏不存在的列索引 7**
   - **源码位置**：`src/ui/ContentPanel.cpp` 中的 `initListView()`
   - **描述**：调用了 `header->setSectionHidden(7, true);`，但关联数据模型 `DiskItemModel::columnCount()` 返回值硬编码为 7（即仅有 0~6 列），索引 7 为越界无效调用。

2. **`m_isPendingEdit` 状态未复位**
   - **源码位置**：`src/ui/ContentPanel.cpp` 中的 `restoreSelections()`
   - **描述**：使用 `if (m_isPendingEdit) view->edit(lastProxyIdx);` 进入编辑态后，未重置 `m_isPendingEdit = false;`，导致后续偶发意外触发编辑态。

3. **`ThumbnailDelegate::helpEvent` 空局部变量与悬空计算**
   - **源码位置**：`src/ui/ThumbnailDelegate.cpp`
   - **描述**：计算了 `Metrics m` 和 `statusRect`，但未进行任何逻辑判定即直接透传基类调用，属于无效占位代码。

4. **`CardPainterHelper::drawRatingStars` 废弃参数**
   - **源码位置**：`src/ui/CardPainterHelper.cpp`
   - **描述**：函数入参包含 `starSpacing`，但内部使用 `Q_UNUSED(starSpacing)` 强行忽略，并硬编码使用 `unifiedSpacing = -4`。

---

## 三、 数据模型与持久化服务层僵尸字段与打桩代码

1. **`ItemRecord::isManaged` 僵尸字段**
   - **源码位置**：`src/core/ItemRecord.h`
   - **描述**：历史托管库时代遗留字段，全生命周期无任何业务逻辑读取或赋值。

2. **`ItemRecord::thumbStatus` 与 `RuntimeMeta::thumbStatus` 幽灵字段**
   - **源码位置**：`src/core/ItemRecord.h`、`src/meta/MetadataDefs.h`
   - **描述**：定义为“0:正常, 1:失败”，实际工程全量走 `DiskMediaExtractor` 实时与磁盘缓存体系，该字段已沦为幽灵字段。

3. **`MetadataManager::initFromDatabase` 虚假初始化打桩函数**
   - **源码位置**：`src/meta/MetadataManager.cpp`
   - **描述**：函数内仅调用 `DatabaseManager::instance().init()` 并将 `m_loaded = true`，并未实际从数据库加载元数据，属于纯打桩代码。

4. **模型层与视图层双重图标缓存冗余**
   - **源码位置**：`src/ui/models/DiskItemModel.h` (`m_iconCache`) 与 `src/ui/ShellIconManager.h`
   - **描述**：`DiskItemModel` 维护了 `m_iconCache`（`QCache<QString, QIcon>`），而 `ShellIconManager` 内部同时维护了系统图标缓存，造成内存重复占用与多头缓存失效问题。
