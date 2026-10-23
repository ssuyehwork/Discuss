# FavoriteAutoRecursive-2.md - 收藏夹自动递归信号覆盖冲突根治实施方案

- **Base Commit Hash**: `fea3877403076754476d110247feb42e36035c01`
- **Prerequisite Plans**: 无
- **Supersedes**: `QuarkMeta Architecture/Implementation Plan/FavoriteAutoRecursive-1.md`

---

## 已读物理证据清单 (Gate 1 Evidence)
1. **物理读取文件列表**：
   - `src/core/NavigationService.h`
   - `src/core/NavigationService.cpp`
   - `src/ui/FavoritePanel.h`
   - `src/ui/FavoritePanel.cpp`
   - `src/ui/PanelMediator.cpp`
2. **头文件原样签名核查**：
   - `void navigateTo(const QString& rawUrl, bool recordHistory = true);` (`src/core/NavigationService.h:16`)
   - `void currentUrlChanged(const QString& url, const QString& displayPath);` (`src/core/NavigationService.h:44`)
   - `void directorySelected(const QString& path);` (`src/ui/FavoritePanel.h:43`)
3. **既有 API 检索日志**：
   - `grep -rn "isAutoRecursive" src/`：确认 `FavoriteDao` 和 `FavoriteRecord` 中 `isAutoRecursive` 数据库读写与模型绑定逻辑已在当前代码库中落地，本次方案仅针对导航信号透传进行精准修复。

---

## Chapter 1: Overview（概述与根因分析）

### 1. 根因分析（物理死因）
在当前的仓库代码中，`FavoriteDao`、`FavoriteRecord` 与 `FavoritePanel` 已具备 `isAutoRecursive`（自动递归）的数据库存取、图标渲染与右键菜单切换逻辑。
但是，当用户点击开启了自动递归的收藏项（如 `src`）时，发生了如下**信号竞争与递归标志丢失冲突**：

1. 用户点击收藏项 `src` ➔ `FavoritePanel` 依次发射了两个信号：
   - `directorySelectedWithRecursive(path, true)`
   - `directorySelected(path)`
2. `directorySelectedWithRecursive` 触发 `target->loadDirectory(path, true)`，使得主内容区极其短暂地开启了递归；
3. 但紧接着 `directorySelected(path)` 触发 `NavigationService::instance().navigateTo(path)` ➔ 发射全局导航广播 `currentUrlChanged(url, displayPath)`；
4. `PanelMediator` 接收到 `currentUrlChanged` 广播，执行了统一导航逻辑：
   ```cpp
   targetPanel->loadDirectory(url); // ❌ 默认 recursive = false！
   ```
5. **覆盖冲突结果**：`loadDirectory(url, false)` 瞬间覆盖重置了先前设置的 `recursive = true`。
   最终表现为：左侧收藏项图标显示成功（标记 1），但右侧主区未能递归展平显示、右上角工具栏 layers 按钮亦未能高亮绿灯（标记 2）。

### 2. 重构设计目标
1. 扩展 `NavigationService::navigateTo` 和 `currentUrlChanged` 信号，增加可选参数 `bool recursive = false`。
2. 简化 `FavoritePanel` 信号，将 `directorySelected` 修改为支持传递 `recursive` 标志：`directorySelected(const QString& path, bool recursive = false)`，不再使用冗余的双信号连接。
3. `PanelMediator` 响应 `currentUrlChanged(url, displayPath, recursive)` 时，将 `recursive` 标志透传给 `targetPanel->loadDirectory(url, recursive)`。
4. 彻底消除状态覆盖冲突，保证标记 1（收藏项图标）与标记 2（工具栏 layers 高亮及子文件夹展平显示）100% 同步。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/core/NavigationService.h` （修改）
2. `src/core/NavigationService.cpp` （修改）
3. `src/ui/FavoritePanel.h` （修改）
4. `src/ui/FavoritePanel.cpp` （修改）
5. `src/ui/PanelMediator.cpp` （修改）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 1. `src/core/NavigationService.h`
```
<<<<<<< SEARCH
    void navigateTo(const QString& rawUrl, bool recordHistory = true);
=======
    void navigateTo(const QString& rawUrl, bool recordHistory = true, bool recursive = false);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    void currentUrlChanged(const QString& url, const QString& displayPath);
=======
    void currentUrlChanged(const QString& url, const QString& displayPath, bool recursive = false);
>>>>>>> REPLACE
```

### 2. `src/core/NavigationService.cpp`
```
<<<<<<< SEARCH
void NavigationService::navigateTo(const QString& rawUrl, bool recordHistory) {
=======
void NavigationService::navigateTo(const QString& rawUrl, bool recordHistory, bool recursive) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    emit currentUrlChanged(m_currentUrl, currentDisplayPath());
    emitNavState();
}
=======
    emit currentUrlChanged(m_currentUrl, currentDisplayPath(), recursive);
    emitNavState();
}
>>>>>>> REPLACE
```

### 3. `src/ui/FavoritePanel.h`
```
<<<<<<< SEARCH
    void directorySelected(const QString& path);
    void directorySelectedWithRecursive(const QString& path, bool recursive);
=======
    void directorySelected(const QString& path, bool recursive = false);
>>>>>>> REPLACE
```

### 4. `src/ui/FavoritePanel.cpp`
```
<<<<<<< SEARCH
    if (fi.isDir()) {
        bool isAutoRec = index.data(Qt::UserRole + 9).toBool();
        emit directorySelectedWithRecursive(path, isAutoRec);
        emit directorySelected(path);
    } else {
=======
    if (fi.isDir()) {
        bool isAutoRec = index.data(Qt::UserRole + 9).toBool();
        emit directorySelected(path, isAutoRec);
    } else {
>>>>>>> REPLACE
```

### 5. `src/ui/PanelMediator.cpp`
```
<<<<<<< SEARCH
    // 1. 路径变更与导航驱动
    connect(&NavigationService::instance(), &NavigationService::currentUrlChanged, this,
            [this, contentPanel, addressBar, navPanel, filterPanel, searchController, titleBar](const QString& url, const QString& displayPath) {
=======
    // 1. 路径变更与导航驱动
    connect(&NavigationService::instance(), &NavigationService::currentUrlChanged, this,
            [this, contentPanel, addressBar, navPanel, filterPanel, searchController, titleBar](const QString& url, const QString& displayPath, bool recursive) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
            if (url == "computer://") {
                targetPanel->loadDirectory("computer://");
            } else if (url == "trash://") {
                targetPanel->loadCategory("trash");
            } else {
                targetPanel->loadDirectory(url);
            }
=======
            if (url == "computer://") {
                targetPanel->loadDirectory("computer://");
            } else if (url == "trash://") {
                targetPanel->loadCategory("trash");
            } else {
                targetPanel->loadDirectory(url, recursive);
            }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    if (favoritePanel) {
        connect(favoritePanel, &FavoritePanel::directorySelectedWithRecursive, this, [this, contentPanel](const QString& path, bool recursive) {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                target->loadDirectory(path, recursive);
            }
        });

        connect(favoritePanel, &FavoritePanel::directorySelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });
=======
    if (favoritePanel) {
        connect(favoritePanel, &FavoritePanel::directorySelected, &NavigationService::instance(), [](const QString& path, bool recursive) {
            NavigationService::instance().navigateTo(path, true, recursive);
        });
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 1. 构建步骤
```bash
cmake --build build --config Debug
```

### 2. 运行时验收标准
1. **收藏项视觉标识（标记 1）**：开启“自动递归”的收藏文件夹项目右侧精确绘制出绿色的 `layers` 层次图标。
2. **工具栏按钮与内容区同步（标记 2）**：鼠标点击开启了“自动递归”的收藏文件夹：
   - 右上角工具栏上的 `layers` 按钮同步变为高亮绿色；
   - 右侧主内容区递归展开并展示所有子文件夹中的项目。
3. **状态持久化**：重启软件或重新加载收藏夹，该收藏项的自动递归设置与图标依然生效。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- 复用 `NavigationService::navigateTo` 和 `currentUrlChanged` 全局导航主干通道，彻底消除信号竞争与重复调用。
- 复用 `ContentPanel::loadDirectory(url, recursive)` 官方 API 加载数据。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 自动递归配置 | `favorites` 表 `is_auto_recursive` | QStandardItem `UserRole + 9` | 导航时透传给 `ContentPanel::m_isRecursive` |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

| 外部调用接口 | 物理 .h 中的声明行 | 访问级别 |
| :--- | :--- | :--- |
| `NavigationService::navigateTo` | `src/core/NavigationService.h:16` | `public` |
| `NavigationService::currentUrlChanged` | `src/core/NavigationService.h:44` | `public signal` |
| `FavoritePanel::directorySelected` | `src/ui/FavoritePanel.h:43` | `public signal` |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- 所有涉及文件包含链完备闭合，类型无需新增头文件依赖。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项 | 结论 | 关联位置/说明 |
| :--- | :--- | :--- | :--- |
| 1 | 方案位于物理隔离目录 | 满足 | `FavoriteAutoRecursive-2.md` |
| 2 | SEARCH 块原文存在且唯一 | 满足 | 精准校验 5 个修改文件 |
| 3 | 10 大章节完整 | 满足 | Chapter 1 ~ 10 全齐 |
| 4 | 无 Win32 / Qt::Popup Hack | 满足 | 遵循 Qt 原生信号槽机制 |
| 5 | 无主/后台线程 UI 违规 | 满足 | 主线程同步 UI 刷新 |
| 6 | 样式与数值 100% 原样保持 | 满足 | 遵守原有 QPainter 几何逻辑 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

- **交互行为**：普通导航与点击行为完全保持不变，仅扩展可选的 `recursive` 状态传递。
- **外观数值**：右侧 `layers` 图标尺寸与原工具栏图标 100% 保持相同颜色与大小。
