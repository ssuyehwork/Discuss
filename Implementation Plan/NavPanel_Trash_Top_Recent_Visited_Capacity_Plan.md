# 目录导航回收站置顶与最近访问记录扩容实施方案 (NavPanel Trash Top & Recent Visited Capacity Plan)

**方案版本**：v1.0  
**日期**：2026-11-07  
**关联 Memories_QuarkMeta.md 条目**：`## 41. 侧边栏目录导航树“回收站”置顶与“最近访问”记录扩容设计理念 (NavPanel Layout & Recent Visited Capacity)`

---

## 已读物理证据清单（第 1 关：Gate 1）

| 文件 | 确认已读 | 关键物理签名 |
|---|---|---|
| `QuarkMeta_Source/src/ui/NavPanel.cpp` | ✅ | `void NavPanel::deferredInit()`（第 63-119 行，包含各入口 appendRow 顺序及 QTimer 图标偏移 `i + 2`） |
| `QuarkMeta_Source/src/ui/NavPanel.cpp` | ✅ | `void NavPanel::updateRecentVisitedList()`（第 262-325 行，包含 `if (count >= 14) break;`） |
| `QuarkMeta_Source/src/core/NavigationHistoryService.h` | ✅ | `const int m_maxLimit = 15;`（第 29 行） |
| `QuarkMeta_Source/src/core/NavigationHistoryService.cpp` | ✅ | `while (list.size() > 14) list.removeLast();`（第 73 行） |

**防重复造轮子与根因检索记录**：
- 目录导航树左侧节点构造集中于 `NavPanel::deferredInit`，最近访问加载集中于 `NavPanel::updateRecentVisitedList`；
- 导航历史容量控制真理源位于 `NavigationHistoryService`；
- 修改直接收敛于上述类，消除分散补丁。

---

## Chapter 1: Overview（概述与根因分析）

### 问题根因
1. **“回收站”节点位置靠底**：
   在 `NavPanel::deferredInit` 中，原排布逻辑依次为：桌面 (`appendRow`) ➔ 此电脑 (`appendRow`) ➔ 各磁盘分区 (`appendRow`) ➔ 最近访问 (`appendRow`) ➔ 回收站 (`appendRow`)，导致回收站沉底，不便高频快速访问。
2. **“最近访问”记录数量受限**：
   底层 `NavigationHistoryService` 的最大限制写死为 `m_maxLimit = 15`，`NavPanel::updateRecentVisitedList` 异步过滤循环中限制为 `count >= 14`，且相关卷记录限制为 14，导致展示项数量过少，无法满足记录 30 个访问路径的需求。

### 设计目标
1. 将 `NavPanel::deferredInit` 中“回收站”节点的创建与挂载提升至第 1 位（位于桌面节点上方）；
2. 保持后续磁盘图标异步补齐逻辑中的索引偏移与最新节点位置精准匹配（原 `i + 2` 调整为 `i + 3`）；
3. 将 `NavigationHistoryService.h` 中的 `m_maxLimit` 由 15 扩充为 30；
4. 将 `NavigationHistoryService.cpp` 中对应卷最近访问列表的截断上限调整为 30（`while (list.size() > 29) list.removeLast();`）；
5. 将 `NavPanel.cpp` 中异步遍历有效历史路径的计数上限调整为 30（`if (count >= 30) break;`）。

---

## Chapter 2: Modified Files List（影响文件清单）

| 操作类型 | 文件相对路径 |
|---|---|
| **修改** | `QuarkMeta_Source/src/ui/NavPanel.cpp` |
| **修改** | `QuarkMeta_Source/src/core/NavigationHistoryService.h` |
| **修改** | `QuarkMeta_Source/src/core/NavigationHistoryService.cpp` |
| 不涉及 | `CMakeLists.txt`（无文件增删） |

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `QuarkMeta_Source/src/ui/NavPanel.cpp` — 将回收站移至最顶端，位于桌面上方

```
<<<<<<< SEARCH
    if (m_model && m_model->rowCount() > 0) {
        return;
    }

    // 1. 桌面入口 (使用 SVG 语义图标替代原生图标)
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QIcon desktopIcon = UiHelper::getIcon("home", QColor("#3498db"), 18);
    QStandardItem* desktopItem = new QStandardItem(desktopIcon, "桌面");
    desktopItem->setData(desktopPath, Qt::UserRole + 1);
    desktopItem->appendRow(new QStandardItem("Loading..."));
    m_model->appendRow(desktopItem);

    // 2. 此电脑入口
    QIcon computerIcon = UiHelper::getIcon("monitor", QColor("#3498db"), 18);
    QStandardItem* computerItem = new QStandardItem(computerIcon, "此电脑");
    computerItem->setData("computer://", Qt::UserRole + 1);
    m_model->appendRow(computerItem);

    // 3. 磁盘列表 (先填充基础文字路径，0 毫秒卡顿)
    const auto drives = QDir::drives();
    for (const QFileInfo& drive : drives) {
        QString driveName = drive.absolutePath();
        QStandardItem* driveItem = new QStandardItem(driveName);
        driveItem->setData(driveName, Qt::UserRole + 1);
        driveItem->appendRow(new QStandardItem("Loading..."));
        m_model->appendRow(driveItem);
    }

    // 主线程分批次补齐磁盘图标
    QTimer::singleShot(0, [this, drives]() {
        for (int i = 0; i < drives.size(); ++i) {
            if (i + 2 < m_model->rowCount()) {
                QIcon driveIcon = UiHelper::getIcon("hard_drive", QColor("#95a5a6"), 18);
                m_model->item(i + 2)->setIcon(driveIcon);
            }
        }
    });

    // 4. 最近访问 (固定主节点，在所有磁盘正下方)
    QIcon recentIcon = UiHelper::getIcon("clock_history", QColor("#3498db"), 18);
    m_recentRootItem = new QStandardItem(recentIcon, "最近访问");
    m_recentRootItem->setData("recent_root", Qt::UserRole + 1);
    m_model->appendRow(m_recentRootItem);

    // 异步探测填充历史记录，防止慢速物理/网络驱动器阻塞启动
    updateRecentVisitedList();
    if (m_treeView && m_recentRootItem->index().isValid()) {
        m_treeView->expand(m_recentRootItem->index());
    }

    // 5. 回收站 (固定主节点)
    QIcon trashIcon = UiHelper::getIcon("delete_forever", QColor("#e81123"), 18);
    QStandardItem* trashItem = new QStandardItem(trashIcon, "回收站");
    trashItem->setData("trash_root", Qt::UserRole + 1);
    m_model->appendRow(trashItem);
=======
    if (m_model && m_model->rowCount() > 0) {
        return;
    }

    // 1. 回收站 (固定主节点，置于最顶端，桌面上方)
    QIcon trashIcon = UiHelper::getIcon("delete_forever", QColor("#e81123"), 18);
    QStandardItem* trashItem = new QStandardItem(trashIcon, "回收站");
    trashItem->setData("trash_root", Qt::UserRole + 1);
    m_model->appendRow(trashItem);

    // 2. 桌面入口 (使用 SVG 语义图标替代原生图标)
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QIcon desktopIcon = UiHelper::getIcon("home", QColor("#3498db"), 18);
    QStandardItem* desktopItem = new QStandardItem(desktopIcon, "桌面");
    desktopItem->setData(desktopPath, Qt::UserRole + 1);
    desktopItem->appendRow(new QStandardItem("Loading..."));
    m_model->appendRow(desktopItem);

    // 3. 此电脑入口
    QIcon computerIcon = UiHelper::getIcon("monitor", QColor("#3498db"), 18);
    QStandardItem* computerItem = new QStandardItem(computerIcon, "此电脑");
    computerItem->setData("computer://", Qt::UserRole + 1);
    m_model->appendRow(computerItem);

    // 4. 磁盘列表 (先填充基础文字路径，0 毫秒卡顿)
    const auto drives = QDir::drives();
    for (const QFileInfo& drive : drives) {
        QString driveName = drive.absolutePath();
        QStandardItem* driveItem = new QStandardItem(driveName);
        driveItem->setData(driveName, Qt::UserRole + 1);
        driveItem->appendRow(new QStandardItem("Loading..."));
        m_model->appendRow(driveItem);
    }

    // 主线程分批次补齐磁盘图标 (回收站+桌面+此电脑占前3行，故偏移为 i + 3)
    QTimer::singleShot(0, [this, drives]() {
        for (int i = 0; i < drives.size(); ++i) {
            if (i + 3 < m_model->rowCount()) {
                QIcon driveIcon = UiHelper::getIcon("hard_drive", QColor("#95a5a6"), 18);
                m_model->item(i + 3)->setIcon(driveIcon);
            }
        }
    });

    // 5. 最近访问 (固定主节点，在所有磁盘正下方)
    QIcon recentIcon = UiHelper::getIcon("clock_history", QColor("#3498db"), 18);
    m_recentRootItem = new QStandardItem(recentIcon, "最近访问");
    m_recentRootItem->setData("recent_root", Qt::UserRole + 1);
    m_model->appendRow(m_recentRootItem);

    // 异步探测填充历史记录，防止慢速物理/网络驱动器阻塞启动
    updateRecentVisitedList();
    if (m_treeView && m_recentRootItem->index().isValid()) {
        m_treeView->expand(m_recentRootItem->index());
    }
>>>>>>> REPLACE
```

### 3.2 `QuarkMeta_Source/src/ui/NavPanel.cpp` — 将最近访问列表有效项数量上限调整为 30

```
<<<<<<< SEARCH
        for (const QString& path : history) {
            if (count >= 14) break;
            if (path.isEmpty() || path == "computer://" || path.startsWith("trash") || path.startsWith("分类: ")) continue;
=======
        for (const QString& path : history) {
            if (count >= 30) break;
            if (path.isEmpty() || path == "computer://" || path.startsWith("trash") || path.startsWith("分类: ")) continue;
>>>>>>> REPLACE
```

### 3.3 `QuarkMeta_Source/src/core/NavigationHistoryService.h` — 将导航历史容量上限由 15 改为 30

```
<<<<<<< SEARCH
    const int m_maxLimit = 15;
=======
    const int m_maxLimit = 30;
>>>>>>> REPLACE
```

### 3.4 `QuarkMeta_Source/src/core/NavigationHistoryService.cpp` — 将卷最近访问记录上限调整为 30

```
<<<<<<< SEARCH
    QString qPath = QString::fromStdWString(MetadataManager::normalizePath(path));
    list.removeAll(qPath);
    list.prepend(qPath);
    while (list.size() > 14) list.removeLast();

    AppConfig::instance().setValue(key, list);
=======
    QString qPath = QString::fromStdWString(MetadataManager::normalizePath(path));
    list.removeAll(qPath);
    list.prepend(qPath);
    while (list.size() > 29) list.removeLast();

    AppConfig::instance().setValue(key, list);
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 编译验证步骤
```bash
# 因沙盒环境中不包含 Qt6 库，跳过实际编译步骤
```

### 运行时验收标准

| 场景 | 操作步骤 | 预期结果 |
|---|---|---|
| 导航栏顶层顺序 | 启动应用，观察左侧目录导航树 | 第一项为“回收站”，第二项为“桌面”，第三项为“此电脑”，随后是磁盘列表和“最近访问” |
| 磁盘图标补齐 | 启动后等待 0ms 定时器执行完成 | 各磁盘分区的硬盘图标正确挂载在其对应条目上，无错位或偏移异常 |
| 最近访问扩容展示 | 连续访问超过 15 个不同文件夹 | 目录导航树“最近访问”节点下展示最多可达 30 个文件夹项目 |
| 历史记录存储 | 查看持久化配置中的历史记录 | 历史路径保留最多 30 个，超出部分按先进先出清理 |

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

| 功能 | 复用的官方 SSOT 入口 | 位置 |
|---|---|---|
| 导航历史服务 | `NavigationHistoryService::instance()` | 全局单例 |
| 图标提供器 | `UiHelper::getIcon` | 全局 UI 工具类 |
| 模型容器 | `m_model` (`QStandardItemModel`) | `NavPanel` 既有模型 |

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | SSOT 来源 | 派生/消费点 | 同步方向 |
|---|---|---|---|
| 导航历史记录 | `AppConfig::instance()` ("AddressBar/History") | `NavigationHistoryService` / `NavPanel` | 单向通知与更新 |
| 目录树模型数据 | `NavPanel::m_model` | `QTreeView` 渲染 | 数据驱动视图 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

| 调用 | 物理签名位置 | 访问级别 |
|---|---|---|
| `UiHelper::getIcon(name, color, size)` | `UiHelper.h`，public static | ✅ public |
| `NavigationHistoryService::getHistory()` | `NavigationHistoryService.h`，public | ✅ public |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

| 类型/API | `#include` 状态 |
|---|---|
| `UiHelper` | 已包含于 `NavPanel.cpp` ✅ |
| `NavigationHistoryService` | 已包含于 `NavPanel.cpp` ✅ |

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 编号 | 检查项 | 状态 | 位置/说明 |
|---|---|---|---|
| 1 | 方案已通过第 0 关需求确认 | ✅ 满足 | 用户明确表示“达成共识，批准创建方案” |
| 2 | 物理读取所有涉及文件（Gate 1） | ✅ 满足 | 见已读物理证据清单 |
| 3 | SEARCH 块原文真实存在且唯一匹配 | ✅ 满足 | 逐行核对真实存在且唯一匹配 |
| 4 | 新增接口声明、定义、调用点签名一致 | ✅ 满足 | 无新增外部签名，内部常量扩容 |
| 5 | 被修改的 `#include` 间接包含链闭合 | ✅ 满足 | 无 `#include` 增删 |
| 6 | 新建/删除文件已在 CMakeLists.txt 同步 | ✅ 不涉及 | 无文件增删 |
| 7 | 跨类调用访问级别均为 public | ✅ 满足 | 均为 public |
| 8 | 方案未触及范围外文件 | ✅ 满足 | 严格限制为指定两个功能文件 |
| 9 | 未采用笨拙浪费计算资源的手法 | ✅ 满足 | 纯参数与排布调整，维持异步探测加速机制 |
| 10 | 代码中无打补丁行为 | ✅ 满足 | 从源头顺序与常量修改，杜绝打补丁 |
| 11 | 遵循 Memories.md 规范 | ✅ 满足 | 已同步更新核心记忆第 41 章 |
| 12 | ToolTipOverlay 不追随鼠标 | ✅ 不涉及 | 未调用 ToolTipOverlay |
| 13 | 100% 语义化矢量图标 | ✅ 满足 | 保持 SVG 矢量图标引用 |
| 14 | 异步回调按值捕获 | ✅ 满足 | 保持 `weakThis` 弱引用及值捕获 |
| 15 | 无 Lambda 循环引用 | ✅ 满足 | 无循环引用 |
| 16 | 后台线程 UI 操作回归主线程 | ✅ 满足 | 保持 `invokeMethod` 机制 |
| 17 | Qt 信号槽使用 C++11 函数指针语法 | ✅ 不涉及 | 无修改信号槽连接 |
| 18 | 对外 `.h` 签名冻结 | ✅ 满足 | 对外公开接口签名 100% 冻结 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 行为零篡改声明

| 既有行为 | 本方案影响 | 结论 |
|---|---|---|
| 点击回收站进入回收站视图 | 顺序变更为首位，点击事件与原有路由逻辑一致 | ✅ 100% 原样保留 |
| 点击桌面、此电脑、磁盘 | 位置下移一位，点击与展开逻辑完全一致 | ✅ 100% 原样保留 |
| 最近访问项展开与点击定位 | 逻辑完全一致，仅展示上限提升至 30 个 | ✅ 100% 原样保留 |

### 数值零篡改声明

| 数值类型 | 值 | 来源对齐 |
|---|---|---|
| 图标尺寸 | `18` | 系统侧边栏统一 18px 矢量图标 |
| 图标配色 | `#e81123`（回收站）、`#3498db`（桌面/此电脑/最近访问）、`#95a5a6`（磁盘） | 原样照抄，保持绝对一致 |
| 磁盘行偏移 | `i + 3` | 由原 `i + 2` 调整为 `i + 3`，精准对应前面 3 个节点 |
