# NavPanel-2.md Implementation Plan: “最近访问”分类节点与 14 条历史路径动态联动

## 1. 架构原则评估 (3 Questions Check)

### 1.1 单一真理源 (Single Source of Truth)
- **最近访问历史数据源 SSOT**：`NavigationHistoryService` 是全局最近访问历史记录的唯一真理源，数据持久化归属于 `AppConfig` (`AddressBar/History`)。
- **导航视图展示 SSOT**：`NavPanel` 中的 `m_treeView` / `m_model` 是第 1 栏目录导航区域的唯一 UI 展示载体。

### 1.2 封装完整性 (Encapsulation Integrity)
- `NavPanel` 通过连接 `NavigationHistoryService::historyChanged` 信号响应历史路径变更，解耦不直接硬编码其他组件的私有状态。
- `NavigationService::navigateTo` 在处理磁盘路径导航时转调 `NavigationHistoryService::instance().appendPath(normalized)`，将导航历史写入与广播收敛于服务层。
- 点击“最近访问”子节点时发出 `directorySelected(path)` 信号，维持 `NavPanel` 原有的公共 API 契约不变。

### 1.3 根因与表面现象分析 (Root Cause vs Surface Phenomenon)
- **需求分析**：
  1. 位置：位于第 1 栏“目录导航栏 (`NavPanel`)”中，紧跟在所有物理磁盘驱动器（C:/、G:/、H:/ 等）正下方；
  2. 结构：固定分类主节点名为“最近访问”，使用专属 `clock_history.svg` 图标，其展开子列表显示至多 14 条最近访问过的文件夹；
  3. 交互：点击子项触发 `directorySelected(path)` 跳转内容面板，随用户浏览其他文件夹实时更新并打顶刷新。

---

## 2. 详细实现方案

### 2.1 `NavigationService` 完善历史记录触发
在 `src/core/NavigationService.cpp` 的 `navigateTo()` 函数中，在非虚拟协议路径处理逻辑内加入：
```cpp
if (!isVirtualProtocol()) {
    NavigationHistoryService::instance().appendPath(normalized);
    NavigationHistoryService::recordRecentVisitedFolder(QDir::toNativeSeparators(m_currentUrl).toStdWString());
}
```
确保每次成功导航至本地物理文件夹时，自动将路径追加/置顶到历史列表并广播 `historyChanged` 信号。

### 2.2 `NavPanel` 添加“最近访问”主节点与更新槽函数
1. **`src/ui/NavPanel.h`**：
   - 增加成员变量 `QStandardItem* m_recentRootItem = nullptr;`；
   - 增加私有槽函数 `void updateRecentVisitedList();`；

2. **`src/ui/NavPanel.cpp`**：
   - 在 `initUi()` 中，建立信号连接：
     `connect(&NavigationHistoryService::instance(), &NavigationHistoryService::historyChanged, this, &NavPanel::updateRecentVisitedList);`
   - 在 `deferredInit()` 中，在磁盘驱动器列表添加完毕后，追加“最近访问”固定主节点：
     ```cpp
     QIcon recentIcon = UiHelper::getIcon("clock_history", QColor("#3498db"), 18);
     m_recentRootItem = new QStandardItem(recentIcon, "最近访问");
     m_recentRootItem->setData("recent_root", Qt::UserRole + 1);
     m_model->appendRow(m_recentRootItem);
     updateRecentVisitedList();
     ```
   - 实现 `updateRecentVisitedList()`：
     - 清空 `m_recentRootItem` 现有子行；
     - 从 `NavigationHistoryService::instance().getHistory()` 读取历史路径；
     - 校验路径有效性（`QFileInfo(path).exists() && isDir()`）；
     - 提取文件夹名称（根目录时展示原生驱动器路径），配以 `ShellIconManager::getFileIcon` 或 `folder_filled` 图标，设置 `Qt::ToolTipRole` 完整路径；
     - 限制最多添加 14 条最常用的有效历史记录；
     - 调用 `m_treeView->expand(m_recentRootItem->index())` 保持展开呈现。
   - 在 `onTreeClicked()` 中：
     - 拦截 `recent_root` 主节点避免无效导航；
     - 点击子项文件夹路径时，发送 `emit directorySelected(path)` 触发跳转。

---

## 3. 验证与测试计划

1. **位置与视觉结构校验**：
   - 启动应用，检查“最近访问”节点是否精准位于所有物理磁盘驱动器正下方，且图标为专属 `clock_history.svg`。
   - 检查子列表是否在“最近访问”节点下方展开显示。

2. **点击导航校验**：
   - 点击“最近访问”下的任意文件夹项，验证中间内容面板（`ContentPanel`）立即跳转并加载对应文件夹。

3. **动态置顶与 14 条限制校验**：
   - 在应用中连续导航访问不同的文件夹，验证“最近访问”列表实时刷新置顶，且最多保持 14 条记录。

---

## 4. 检查与合规声明
- 公开 API 签名冻结：`NavPanel` 所有既有公共接口和信号保持 100% 稳定。
- 主题与 QSS 规范：统一使用 `UiHelper` 及 `resources/style.qss` 主题配置，不硬编码内联样式。
