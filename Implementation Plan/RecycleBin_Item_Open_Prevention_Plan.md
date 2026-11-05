# 回收站项目打开防护实施方案 (RecycleBin Item Open Prevention Plan)

**方案版本**：v1.0  
**日期**：2026-11-06  
**关联 Memories.md 条目**：`[新增记录] 内容面板回收站项目打开保护设计理念 (Recycle Bin Item Open Prevention)`

---

## 已读物理证据清单（第 1 关：Gate 1）

| 文件 | 确认已读 | 关键物理签名 |
|---|---|---|
| `src/ui/ContentPanel.cpp` | ✅ | `void ContentPanel::activateIndex(const QModelIndex& index)`（第 839-856 行） |
| `src/ui/ContentPanel.cpp` | ✅ | `#include "ToolTipOverlay.h"` 已在第 25 行包含 |
| `src/ui/ContentPanel.cpp` | ✅ | 回收站识别判定 `m_currentCategoryType == "trash" || m_currentPath == "trash://"`（第 1001 行）及 `IsDiskTrashRole`（第 1337 行） |
| `src/ui/controllers/ContentKeyHandler.cpp` | ✅ | 空格键/回车键统一委托 `m_panel->activateIndex(nameIdx)`（第 646 行） |
| `src/ui/PanelMediator.cpp` | ✅ | 响应 `directorySelected` 与 `fileActivated` 触发打开与 QuickLook（第 764 行） |

**防重复造轮子与根因检索记录**：
- `ContentPanel::activateIndex` 为所有视图双击、键盘回车与空格激活的唯一官方 SSOT 通道；在入口收口即可 100% 阻断所有打开路径，无需在各视图打补丁。

---

## Chapter 1: Overview（概述与根因分析）

### 问题根因
当用户进入回收站后，双击或按回车/空格激活项目时，`ContentPanel::activateIndex` 未对当前是否处于回收站进行感知拦截，直接发出了 `directorySelected`（试图进入已删除文件夹）或 `fileActivated`（试图通过外部程序或 QuickLook 打开废弃文件），违背了操作系统回收站的安全隔离原则。

### 设计目标
1. 在 `ContentPanel::activateIndex` 入口设置回收站守卫；
2. 只要当前处于回收站视图（`m_currentCategoryType == "trash"`、`m_currentPath == "trash://"`）或当前项目具备回收站数据（`IsDiskTrashRole` 为 true），立即阻断退出；
3. 弹出 Toast 提示：“回收站中的项目无法直接打开，请先还原后再访问”（持续 2000ms，黄色警示色 `#f39c12`），严格不跟随鼠标；
4. 保证单一变量控制与架构纯洁度，不触碰无关文件。

---

## Chapter 2: Modified Files List（影响文件清单）

| 操作类型 | 文件相对路径 |
|---|---|
| **修改** | `src/ui/ContentPanel.cpp` |
| 不涉及 | `CMakeLists.txt`（无文件增删） |

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/ContentPanel.cpp` — `activateIndex` 增加回收站安全拦截守卫

```
<<<<<<< SEARCH
void ContentPanel::activateIndex(const QModelIndex& index) {
    if (!index.isValid()) return;
    if (index.data(SectionHeaderRole).toBool()) return;
    QString path = index.data(PathRole).toString();
    if (path.isEmpty()) return;

    if (QFileInfo(path).isDir()) {
=======
void ContentPanel::activateIndex(const QModelIndex& index) {
    if (!index.isValid()) return;
    if (index.data(SectionHeaderRole).toBool()) return;
    QString path = index.data(PathRole).toString();
    if (path.isEmpty()) return;

    // 回收站安全守卫：回收站中的文件和文件夹均禁止直接打开或进入
    bool isTrash = (m_currentCategoryType == "trash" || m_currentPath == "trash://" || index.data(IsDiskTrashRole).toBool());
    if (isTrash) {
        ToolTipOverlay::instance()->showToast("回收站中的项目无法直接打开，请先还原后再访问", 2000, QColor("#f39c12"));
        return;
    }

    if (QFileInfo(path).isDir()) {
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 编译验证步骤
```bash
cmake --build . --config Release --target QuarkMeta
```
预期：**0 errors, 0 warnings**

### 运行时验收标准

| 场景 | 操作步骤 | 预期结果 |
|---|---|---|
| 回收站双击文件 | 进入回收站，双击任意文件 | **不打开**文件，不弹出 QuickLook，弹出 Toast 提示“回收站中的项目无法直接打开，请先还原后再访问” |
| 回收站双击文件夹 | 进入回收站，双击任意文件夹 | **不进入**文件夹，弹出 Toast 提示“回收站中的项目无法直接打开，请先还原后再访问” |
| 回收站按回车键 | 选中项目按 Enter / Return | **不打开/不进入**，弹出 Toast 提示 |
| 回收站按空格键 | 选中项目按 Space | **不调起 QuickLook**，弹出 Toast 提示 |
| 正常目录双击 | 切换至普通物理文件夹，双击文件/文件夹 | 正常打开文件或展开文件夹，行为完全不受影响 |

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

| 功能 | 复用的官方 SSOT 入口 | 位置 |
|---|---|---|
| Toast 提示 | `ToolTipOverlay::instance()->showToast(...)` | 全局单例，居中弹出，不追随鼠标 |
| 回收站标识读取 | `index.data(IsDiskTrashRole).toBool()` | `ModelContract.h` 官方数据角色 |
| 分类状态读取 | `m_currentCategoryType` / `m_currentPath` | `ContentPanel` 既有属性 |

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | SSOT 来源 | 派生/消费点 | 同步方向 |
|---|---|---|---|
| 回收站视图状态 | `m_currentCategoryType` / `m_currentPath` | `ContentPanel::activateIndex` | 只读判定 |
| 条目回收站角色 | `index.data(IsDiskTrashRole)` | `ContentPanel::activateIndex` | 只读判定 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

| 调用 | 物理签名位置 | 访问级别 |
|---|---|---|
| `ToolTipOverlay::instance()->showToast(...)` | `ToolTipOverlay.h`，public static | ✅ public |
| `index.data(...)` | `QModelIndex::data(int role)`，Qt 标准接口 | ✅ public |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

| 类型/API | `#include` 状态 |
|---|---|
| `ToolTipOverlay` | `#include "ToolTipOverlay.h"` ✅（`ContentPanel.cpp` 第 25 行已有） |
| `IsDiskTrashRole` | 通过 `#include "../core/ModelContract.h"` ✅（`ContentPanel.h` 第 19 行已有） |

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 编号 | 检查项 | 状态 | 位置/说明 |
|---|---|---|---|
| 1 | 方案已通过第 0 关需求确认 | ✅ 满足 | 用户明确表示“达成共识，理解用户意图后可直接修改” |
| 2 | 物理读取所有涉及文件（Gate 1） | ✅ 满足 | 见已读物理证据清单 |
| 3 | SEARCH 块原文真实存在且唯一匹配 | ✅ 满足 | 第 3 章块通过脚本唯一校验 |
| 4 | 新增接口声明、定义、调用点签名一致 | ✅ 满足 | 无新增对外接口，纯内聚逻辑增强 |
| 5 | 被修改的 `#include` 间接包含链闭合 | ✅ 满足 | Chapter 8 全量核查 |
| 6 | 新建/删除文件已在 CMakeLists.txt 同步 | ✅ 不涉及 | 纯内部逻辑修改 |
| 7 | 跨类调用访问级别均为 public | ✅ 满足 | 调用的 ToolTipOverlay 均为 public |
| 8 | 方案未触及范围外文件 | ✅ 满足 | 严格限制为单一文件 `src/ui/ContentPanel.cpp` |
| 9 | 未采用笨拙浪费计算资源的手法 | ✅ 满足 | 仅增加 1 条 O(1) 逻辑或条件判断与 return |
| 10 | 代码中无打补丁行为 | ✅ 满足 | 直接在入口门禁（SSOT）收敛，不搞下层分散过滤 |
| 11 | 遵循 Memories.md 规范 | ✅ 满足 | 已同步写入核心记忆 |
| 12 | ToolTipOverlay 不追随鼠标 | ✅ 满足 | `showToast` 不传入鼠标坐标 |
| 13 | 100% 语义化矢量图标 | ✅ 不涉及 | 纯行为阻断与 Toast，无菜单新增 |
| 14 | 异步回调按值捕获 | ✅ 不涉及 | 纯同步门禁 |
| 15 | 无 Lambda 循环引用 | ✅ 不涉及 | 无 Lambda |
| 16 | 后台线程 UI 操作回归主线程 | ✅ 满足 | `activateIndex` 为 UI 主线程执行 |
| 17 | Qt 信号槽使用 C++11 函数指针语法 | ✅ 不涉及 | 无新增信号槽 |
| 18 | 对外 `.h` 签名冻结 | ✅ 满足 | 零修改 `.h`，100% 冻结 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 行为零篡改声明

| 既有行为 | 本方案影响 | 结论 |
|---|---|---|
| 正常非回收站目录下的双击/回车/空格激活 | 条件为 false，完全直通后续原有逻辑 | ✅ 100% 原样保留 |
| 回收站内的右键菜单（还原、剪切、永久删除等） | 无任何修改 | ✅ 100% 原样保留 |
| 视图选择集、多选逻辑 | 无任何修改 | ✅ 100% 原样保留 |

### 数值零篡改声明

| 数值类型 | 值 | 来源对齐 |
|---|---|---|
| Toast 持续时间 | `2000`（ms） | 与系统标准警告 Toast 一致 |
| Toast 颜色 | `QColor("#f39c12")` | 系统统一警示橙黄色（WarningOrange） |
