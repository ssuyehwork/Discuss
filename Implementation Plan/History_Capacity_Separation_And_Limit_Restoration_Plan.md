# 历史记录容量边界隔离与上限恢复实施方案 (History Capacity Separation And Limit Restoration Plan)

**方案版本**：v1.0
**日期**：2026-11-09
**关联 Memories_QuarkMeta.md 条目**：`## 41. 侧边栏目录导航树“回收站”置顶与“最近访问”记录扩容设计理念` 中 `41.1.2 “最近访问”与历史记录容量边界严格分离契约`

---

## 已读物理证据清单（第 1 关：Gate 1）

| 文件 | 确认已读 | 关键物理签名 |
|---|---|---|
| `QuarkMeta_Source/src/core/NavigationHistoryService.h` | ✅ | `const int m_maxLimit = 30;`（第 29 行） |
| `QuarkMeta_Source/src/core/NavigationHistoryService.cpp` | ✅ | `while (list.size() > 29) list.removeLast();`（第 73 行） |
| `QuarkMeta_Source/src/ui/AddressHistoryPanel.cpp` | ✅ | `for (const QString& path : m_history)`（第 78 行，遍历全量列表构建历史行） |
| `QuarkMeta_Source/src/ui/controllers/ContentContextMenu.cpp` | ✅ | `QStringList recentFolders = NavigationHistoryService::getRecentVisitedFolders(volSerial);`（第 362 行） |
| `QuarkMeta_Source/src/ui/NavPanel.cpp` | ✅ | `if (count >= 30) break;`（第 281 行，左侧导航栏最近访问限制） |

**防重复造轮子与根因检索记录**：
- 用户要求扩充到 30 个的仅限侧边栏目录导航树（`NavPanel`）中的“最近访问”专区；
- “地址栏下拉历史”与右键上下文“移动到”子菜单并未被要求扩容，应当严格保持 15 个上限并执行旧项淘汰；
- 方案不增加重复类或冗余存储，通过职责边界隔离消除功能耦合。

---

## Chapter 1: Overview（概述与根因分析）

### 问题根因
在先前的迭代中，存在过度发散与不当扩展：
1. `NavigationHistoryService` 底层的 `m_maxLimit` 由 15 条扩充到了 30 条，直接导致读取该服务的地址栏下拉历史面板（`AddressHistoryPanel`）也随之展示多达 30 条历史，破坏了原定“最多 15 个记录且淘汰旧的”设计理念；
2. 关联驱动器卷的 `recordRecentVisitedFolder` 淘汰阈值也盲目从 15 条改为了 30 条（`while (list.size() > 29) list.removeLast();`），导致右键菜单中的“移动到”子菜单下拉项过度膨胀；
3. 未将“全局历史存储池 / 侧边栏 30 项高频访问”与“地址栏 15 项轻量弹窗”、“右键移动到 15 项操作”进行容量隔离。

### 设计目标与隔离契约
1. **地址栏历史面板 (`AddressHistoryPanel`)**：
   - 严格恢复 **15 项** 视觉呈现上限；
   - 在 `AddressHistoryPanel::rebuild()` 循环渲染前，截取前 15 条（`m_history.mid(0, 15)`），或循环计数 `count >= 15` 自动截断，坚决杜绝弹出列表过长；
2. **右键菜单“移动到”子菜单 (`ContentContextMenu`)**：
   - 恢复驱动器卷最近访问记录的上限为 **15 项**（`while (list.size() > 14) list.removeLast();`），同时在读取时截取最多 15 项；
3. **侧边栏“最近访问” (`NavPanel::m_recentRootItem`)**：
   - 保持用户明确要求的 **30 项** 深度扩容；底层历史池保留 30 项以供侧边栏异步扫描。

---

## Chapter 2: Modified Files List（影响文件清单）

| 操作类型 | 文件相对路径 |
|---|---|
| **修改** | `QuarkMeta_Source/src/core/NavigationHistoryService.cpp` |
| **修改** | `QuarkMeta_Source/src/ui/AddressHistoryPanel.cpp` |
| **修改** | `QuarkMeta_Source/src/ui/controllers/ContentContextMenu.cpp` |
| 不涉及 | `CMakeLists.txt`（无文件增删） |

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `QuarkMeta_Source/src/core/NavigationHistoryService.cpp` — 恢复关联卷记录上限为 15

```
<<<<<<< SEARCH
    QString qPath = QString::fromStdWString(MetadataManager::normalizePath(path));
    list.removeAll(qPath);
    list.prepend(qPath);
    while (list.size() > 29) list.removeLast();

    AppConfig::instance().setValue(key, list);
=======
    QString qPath = QString::fromStdWString(MetadataManager::normalizePath(path));
    list.removeAll(qPath);
    list.prepend(qPath);
    while (list.size() > 14) list.removeLast();

    AppConfig::instance().setValue(key, list);
>>>>>>> REPLACE
```

### 3.2 `QuarkMeta_Source/src/ui/AddressHistoryPanel.cpp` — 严格限制地址栏下拉历史面板展示最多 15 项

```
<<<<<<< SEARCH
        for (const QString& path : m_history) {
            QWidget* row = new QWidget(this);
            row->setObjectName("HistoryRow");
            row->setCursor(Qt::PointingHandCursor);
            row->setFixedHeight(30);
=======
        int displayCount = 0;
        for (const QString& path : m_history) {
            if (++displayCount > 15) break;

            QWidget* row = new QWidget(this);
            row->setObjectName("HistoryRow");
            row->setCursor(Qt::PointingHandCursor);
            row->setFixedHeight(30);
>>>>>>> REPLACE
```

### 3.3 `QuarkMeta_Source/src/ui/controllers/ContentContextMenu.cpp` — 限制“移动到”子菜单展示最多 15 项

```
<<<<<<< SEARCH
                std::wstring volSerial = MetadataManager::getVolumeSerialNumber(path.toStdWString());
                QStringList recentFolders = NavigationHistoryService::getRecentVisitedFolders(volSerial);
                recentFolders.removeAll(currentPath);

                QMenu* moveMenu = menu.addMenu(UiHelper::getIcon("Export-1.svg", QColor("#EEEEEE"), 18), "移动到");
=======
                std::wstring volSerial = MetadataManager::getVolumeSerialNumber(path.toStdWString());
                QStringList recentFolders = NavigationHistoryService::getRecentVisitedFolders(volSerial);
                recentFolders.removeAll(currentPath);
                while (recentFolders.size() > 15) {
                    recentFolders.removeLast();
                }

                QMenu* moveMenu = menu.addMenu(UiHelper::getIcon("Export-1.svg", QColor("#EEEEEE"), 18), "移动到");
>>>>>>> REPLACE
```

---

## Chapter 4: Architecture & Design Decisions（架构与设计决策）

1. **容量边界解耦**：
   - 全局数据服务 `NavigationHistoryService` 提供足够容量（30 项）以支撑侧边栏大容量并发扫描需求；
   - 视图层（`AddressHistoryPanel` 与 `ContentContextMenu`）根据各自 UI 人机工程学与用户最初设计理念，坚守 15 项边界；
2. **零副作用**：
   - 地址栏历史的单项删除（`removePath`）与全量清除（`clearAll`）接口签名与行为保持 100% 契约不变；
   - 侧边栏“最近访问”30 条探测逻辑不受任何影响。

---

## Chapter 5: Verification & Safety Precautions（验证与安全防护）

1. **编译契约验证**：
   - 现有公开接口 `NavigationHistoryService::getHistory()` 与 `getRecentVisitedFolders()` 签名完全保持不变，调用方不受编译破坏。
2. **界面尺寸自适应**：
   - `AddressHistoryPanel::adjustSize()` 在行数被限定为最多 15 行后，弹出菜单高度可控，避免超出屏幕。

---

## Chapter 6: Rollback Plan（回滚方案）

若回滚，仅需执行 `git checkout` 恢复 `NavigationHistoryService.cpp`、`AddressHistoryPanel.cpp` 以及 `ContentContextMenu.cpp`。

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

| 调用 | 物理签名位置 | 访问级别 |
|---|---|---|
| `NavigationHistoryService::getRecentVisitedFolders(const std::wstring&)` | `NavigationHistoryService.h`，public static | ✅ public |
| `NavigationHistoryService::getHistory()` | `NavigationHistoryService.h`，public | ✅ public |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

| 类型/API | `#include` 状态 |
|---|---|
| `NavigationHistoryService` | 已包含于 `AddressHistoryPanel.cpp` 与 `ContentContextMenu.cpp` ✅ |
| `MetadataManager` | 已包含于 `NavigationHistoryService.cpp` 与 `ContentContextMenu.cpp` ✅ |

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 编号 | 检查项 | 状态 | 位置/说明 |
|---|---|---|---|
| 1 | 方案已通过第 0 关需求确认 | ✅ 满足 | 用户明确表示“达成共识，批准创建方案” |
| 2 | 物理读取所有涉及文件（Gate 1） | ✅ 满足 | 见已读物理证据清单 |
| 3 | SEARCH 块原文真实存在且唯一匹配 | ✅ 满足 | 逐行核对真实存在且唯一匹配 |
| 4 | 新增接口声明、定义、调用点签名一致 | ✅ 满足 | 无新增外部签名，内部容量控制 |
| 5 | 被修改的 `#include` 间接包含链闭合 | ✅ 满足 | 无 `#include` 增删 |
| 6 | 新建/删除文件已在 CMakeLists.txt 同步 | ✅ 不涉及 | 无文件增删 |
| 7 | 跨类调用访问级别均为 public | ✅ 满足 | 均为 public |
| 8 | 方案未触及范围外文件 | ✅ 满足 | 严格限制为指定的三个文件 |
| 9 | 未采用笨拙浪费计算资源的手法 | ✅ 满足 | 视图截断与精准截取，零额外开销 |
| 10 | 代码中无打补丁行为 | ✅ 满足 | 从显示层源头与循环处收敛限制 |
| 11 | 遵循 Memories.md 规范 | ✅ 满足 | 已同步更新核心记忆第 41 章并纠偏 |
| 12 | ToolTipOverlay 不追随鼠标 | ✅ 不涉及 | 未修改 ToolTipOverlay 逻辑 |
| 13 | 100% 语义化矢量图标 | ✅ 满足 | 保持原有矢量图标 |
| 14 | 异步回调按值捕获 | ✅ 不涉及 | 无新增异步回调 |
| 15 | 无 Lambda 循环引用 | ✅ 满足 | 无循环引用 |
| 16 | 后台线程 UI 操作回归主线程 | ✅ 不涉及 | 无后台线程操作变更 |
| 17 | Qt 信号槽使用 C++11 函数指针语法 | ✅ 不涉及 | 无修改信号槽连接 |
| 18 | 对外 `.h` 签名冻结 | ✅ 满足 | 对外公开接口签名 100% 冻结 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 行为零篡改声明

| 既有行为 | 本方案影响 | 结论 |
|---|---|---|
| 地址栏历史下拉点击跳转 | 点击事件与路由完全不变，条目数恢复为用户期望的最多 15 条 | ✅ 100% 原样保留 |
| 右键菜单“移动到”目标移动 | 移动执行、一键分发及自定义浏览完全一致，关联项恢复为最多 15 条 | ✅ 100% 原样保留 |
| 侧边栏“最近访问” | 依然保持展示最多 30 个 | ✅ 100% 原样保留 |

### 数值零篡改声明

| 数值类型 | 值 | 来源对齐 |
|---|---|---|
| 地址栏历史展示上限 | `15` | 恢复用户最初设定的 15 条契约 |
| 移动到子选项展示上限 | `15` | 恢复 15 条契约 |
| 侧边栏“最近访问”展示上限 | `30` | 保持用户要求的 30 条契约 |
