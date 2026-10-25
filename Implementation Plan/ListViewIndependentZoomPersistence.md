# Implementation Plan - 列表视图独立宽/高缩放持久化记忆方案

- **Base Commit Hash**: `1e9a71b62c08f4b040483238a5a105559768becc`
- **Prerequisite Plans**: 无
- **Supersedes**: 无

---

## 第 1 关：已读物理证据清单 (Pre-Draft Evidence)

### 1.1 物理读取文件列表
1. `src/ui/ContentPanel.cpp` (已读取 `ContentPanel` 构造、`setViewMode`、`setZoomLevel` 与 `updateGridSize` 实现)
2. `src/ui/PanelMediator.cpp` (已读取标题栏/工具栏与 `ContentPanel` 的 `zoomLevel` 信号槽通信)

### 1.2 头文件原样签名
1. `ContentPanel::setViewMode(ViewMode mode)`：
   - 声明位置：`src/ui/ContentPanel.h` 第 211 行
2. `ContentPanel::setZoomLevel(int level)`：
   - 声明位置：`src/ui/ContentPanel.h` 第 215 行
3. `ContentPanel::updateGridSize()`：
   - 声明位置：`src/ui/ContentPanel.h` 第 265 行

### 1.3 既有 API 检索日志
- 检索关键词：`grep -rn "GridZoomLevel" src/ui/`
- 命中列表：`ContentPanel.cpp` 第 113、881 行以及 `PanelMediator.cpp` 第 173、181、186 行使用 `"UI/GridZoomLevel"` 读取与保存网格缩放等级。

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题现象与需求分析
由于网格视图（Grid View / Justified View）与列表视图（List View）对项目项目单元格与行高的显示缩放需求完全不同（网格视图卡片尺寸较大，列表视图行高相对精紧），目前全应用共享同一个缩放 Key `"UI/GridZoomLevel"`，导致在网格视图调整好尺寸后，切换到列表视图时尺寸错乱或不符合用户对列表行高的期望。

### 1.2 物理根因分析
在 `src/ui/ContentPanel.cpp` 中：
1. `ContentPanel` 在构造与初始化时，统一读取 `"UI/GridZoomLevel"` 作为初始 `m_zoomLevel`；
2. 当在网格视图把尺寸调节变大（如 160px）时，`m_zoomLevel` 被改写；切换至 `ListView` 时，依然直接使用了该 `m_zoomLevel`，导致列表行高过于臃肿；
3. 在调整缩放比例时，`updateGridSize()` 无条件写回 `"UI/GridZoomLevel"`，缺乏按视图模式隔离存储的机制。

### 1.3 修复方案设计
1. **持久化 Key 隔离**：
   - 网格视图 / 瀑布流视图使用 `"UI/GridZoomLevel"`（默认 96px）；
   - 列表视图启用专属 Key `"UI/ListZoomLevel"`（默认 30px，取值范围 30~230px）。
2. **`setViewMode` 自动恢复专属尺寸**：
   在 `setViewMode` 切换视图时，自动从 `AppConfig` 读取当前目标视图对应的专属缩放保存值，并更新 UI；
3. **`updateGridSize` 按视图模式写入**：
   调整缩放等级时，根据 `m_currentViewMode` 自动写回对应的 Key，实现列表视图与网格视图宽/高缩放尺寸的彻底独立记忆与恢复。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/ui/ContentPanel.cpp` (修改：`setViewMode` 与 `updateGridSize` 支持按视图类型区分缩放 Persistence Key)

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 在 `ContentPanel.cpp` 中更新 `setViewMode` 与 `updateGridSize` 的缩放记忆逻辑

```
<<<<<<< SEARCH
    ViewMode oldMode = m_currentViewMode;
    m_currentViewMode = mode;
    int minZoom = (mode == ListView) ? 30 : 93;
    m_zoomLevel = qBound(minZoom, m_zoomLevel, 230);

    if (mode == ListView) {
=======
    ViewMode oldMode = m_currentViewMode;
    m_currentViewMode = mode;

    if (mode == ListView) {
        int savedListZoom = AppConfig::instance().getValue("UI/ListZoomLevel", 30).toInt();
        m_zoomLevel = qBound(30, savedListZoom, 230);
    } else {
        int savedGridZoom = AppConfig::instance().getValue("UI/GridZoomLevel", 96).toInt();
        m_zoomLevel = qBound(93, savedGridZoom, 230);
    }

    if (mode == ListView) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::updateGridSize() {
    if (m_viewCoordinator) {
        m_viewCoordinator->updateGridSize(m_zoomLevel);
    }
    AppConfig::instance().setValue("UI/GridZoomLevel", m_zoomLevel);
}
=======
void ContentPanel::updateGridSize() {
    if (m_viewCoordinator) {
        m_viewCoordinator->updateGridSize(m_zoomLevel);
    }
    if (m_currentViewMode == ListView) {
        AppConfig::instance().setValue("UI/ListZoomLevel", m_zoomLevel);
    } else {
        AppConfig::instance().setValue("UI/GridZoomLevel", m_zoomLevel);
    }
}
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 构建步骤
运行项目标准 CMake 构建命令：
```bash
cmake --build --preset x64-Debug
```

### 4.2 运行时验收标准
1. 打开 QuarkMeta 应用程序，默认处于网格视图，将卡片缩放调整至 160px；
2. 切换至“列表视图” (List View Mode)，将列表行高缩放调整至 36px；
3. 在视图模式之间来回切换（网格 $\to$ 列表 $\to$ 网格），确认：
   - 切换到列表视图时，自动恢复为列表专属的 36px；
   - 切换到网格视图时，自动恢复为网格专属的 160px；
4. 重启应用程序，再次进入列表视图，确认列表视图依然稳定维持 36px 独立记忆。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- **AppConfig 入口复用**：复用全应用统一的 `AppConfig::instance().getValue / setValue` 进行键值持久化，仅通过 Key 区分隔离，零冗余。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 源 | 派生副本/视图展现 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 列表视图缩放级别 | `AppConfig` 的 `"UI/ListZoomLevel"` | `ContentPanel::m_zoomLevel` | 切换到列表视图时读取，列表缩放调整时写入 |
| 网格视图缩放级别 | `AppConfig` 的 `"UI/GridZoomLevel"` | `ContentPanel::m_zoomLevel` | 切换到网格视图时读取，网格缩放调整时写入 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. 不增删修改任何 `.h` 物理接口声明，私有实现零扩散。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- `ContentPanel.cpp` 已包含 `#include "../core/AppConfig.h"`，头文件闭合完备。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 是否满足 | 说明 |
| :---: | :--- | :---: | :--- |
| 1 | 是否遵循物理隔离与实施方案命名契约 | 是 | 方案存放于 `Implementation Plan/ListViewIndependentZoomPersistence.md` |
| 2 | 是否包含全部 10 大必备章节 | 是 | 包含 Overview 至 Behavior Preservation 全部章节 |
| 3 | SEARCH 块是否在目标文件中真实存在且唯一 | 是 | 已核对原文件唯一匹配 |
| 4 | 是否未直接修改任何 C++/Qt 源代码文件 | 是 | 仅提交实施方案文档，绝对不改源码 |
| 5 | 是否破坏既有对外接口签名与 C++ 契约 | 是(否破坏) | 零 `.h` 修改 |
| 6 | 是否违反单向依赖与分层硬锁 | 是(否违反) | 保持在 UI 视图与配置层分权 |
| 7 | 是否包含特定平台的 Hack 级补丁 | 是(否包含) | 无 Hack |
| 8 | 是否包含另起炉灶的重复实现 | 是(否包含) | 复用统一 `AppConfig` |
| 9 | 是否有隐式/显式的数据迁移逻辑 | 是(否包含) | 无数据变动 |
| 10 | 是否破坏既有 UI 样式数值或参数 | 是(否破坏) | 保持默认缩放参数完全一致 |
| 11 | 是否完整走完四道关质量管控流程 | 是 | 完成需求确认与逻辑核对 |
| 12 | 是否存在跨线程直接操作 GUI 控件行为 | 是(否存在) | 主线程调用 |
| 13 | 是否有死代码残留 | 是(无残留) | 无死代码 |
| 14 | 是否引发潜在的死锁风险 | 是(无风险) | 无锁操作 |
| 15 | 变量与类型闭合性 | 是 | 闭合 |
| 16 | CMake 注册完整性 | 是 | 未新建/删除文件 |
| 17 | 行为与交互零篡改声明 | 是 | 仅增强维度独立记忆，无交互退化 |
| 18 | 语言规范 | 是 | 100% 全中文编写 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 类别 | 项目名称 | 原始状态 / 数值 | 实施后状态 / 数值 | 确认一致 |
| :--- | :--- | :--- | :--- | :---: |
| 默认数值 | 列表缩放默认值 | `30px` | `30px` | 是 |
| 默认数值 | 网格缩放默认值 | `96px` | `96px` | 是 |
