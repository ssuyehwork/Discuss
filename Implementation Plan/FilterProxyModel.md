# Implementation Plan - FilterProxyModel

## 1. Overview（概述与解决的问题）
本实施方案旨在彻底根治“此电脑” (`computer://`) 模式下，驱动器盘符在 `SectionedScrollCanvas` 视图中同时出现在“文件夹”区与“文件”区被重复渲染两次的架构缺陷。

### 根因分析：
1. `FilterProxyModel` 针对文件夹区与文件区分别持有 `showFolders` 和 `showFiles` 关断开关。
2. 此前在 `FilterProxyModel::filterAcceptsRow` 头部，针对 `computer://` 路径加入了全盲豁免分支：
   ```cpp
   if (sourceModelPtr->currentPath() == "computer://") {
       return true;
   }
   ```
   这导致无论当前 FilterProxyModel 是文件区代理（`showFolders=false, showFiles=true`）还是文件夹区代理（`showFolders=true, showFiles=false`），在 `computer://` 场景下均无脑返回 `true`！
3. 驱动器盘符在系统底层 `isDir` 为 `true`。因此盘符既通过了 `m_gridFolderProxyModel`，又被 `m_gridFileProxyModel` 全无脑放行，从而在界面上被重复渲染到了“文件 (5)”区域。

### 解决办法：
修改 `FilterProxyModel::filterAcceptsRow` 中针对 `computer://` 的处理逻辑：
- 驱动器盘符仅在 `showFolders == true`（即“文件夹/驱动器”分区）时被放行展示；
- 在 `showFiles == true`（即“文件”分区）时直接拦截并返回 `false`，彻底消除盘符在“文件”区重复出现的问题。

---

## 2. Modified Files List（影响文件清单）
- `src/ui/models/FilterProxyModel.cpp`

---

## 3. Detailed Line-by-Line Changes（包含 CMakeLists.txt 在内的精准替换块）

### `src/ui/models/FilterProxyModel.cpp`
```cpp
<<<<<<< SEARCH
    // 🚀【此电脑根路径豁免准则】：当加载“此电脑”(computer://)盘符列表时，盘符属于系统硬件层介质，100% 必须始终放行显示，不受常规文件夹/文件显隐或星级筛选器的过滤关断！
    if (sourceModelPtr->currentPath() == "computer://") {
        return true;
    }
=======
    // 🚀【此电脑根路径准则】：当加载“此电脑”(computer://)盘符列表时，盘符仅归属于驱动器/文件夹区，在“文件”区代理中严禁重复显示！
    if (sourceModelPtr->currentPath() == "computer://") {
        return currentFilter.showFolders;
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）
1. **静态逻辑审查**：
   - 检查 `m_gridFolderProxyModel` (设置了 `showFolders=true, showFiles=false`) 和 `m_gridFileProxyModel` (设置了 `showFolders=false, showFiles=true`) 的运行表现。
   - 当 `currentPath == "computer://"` 时，`m_gridFolderProxyModel` 校验 `currentFilter.showFolders` 返回 `true`，成功渲染 5 个盘符；`m_gridFileProxyModel` 校验 `currentFilter.showFolders` 为 `false` 返回 `false`，“文件”区数量归零并自动隐藏。
2. **运行与视图验证**：
   - 启动程序进入“此电脑”，确认页面仅显示唯一的“文件夹 (5)”驱动器盘符列表，“文件 (5)”区域完全消失，且双击盘符可正常进入对应磁盘。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）
- 完美契合 `FilterProxyModel` 的过滤责任边界，不修改任何上层 View 和 Controller 的结构。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用的成员函数 / 属性 | 所在的头文件 | 精准物理签名 | 核查状态 |
| :--- | :--- | :--- | :--- |
| `FilterState::showFolders` | `src/ui/FilterPanel.h` | `bool showFolders = true;` |  物理核实通过 |
| `ItemModelBase::currentPath` | `src/ui/models/ItemModelBase.h` | `const QString& currentPath() const;` |  物理核实通过 |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

- `src/ui/models/FilterProxyModel.cpp`:
  - 已包含 `#include "FilterProxyModel.h"`
  - `FilterState` 在 `FilterProxyModel.h` 中完整引入。
