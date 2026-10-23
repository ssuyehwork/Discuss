# StatusBarNumberColor.md - 底部状态栏数量值高亮着色实施方案

- **Base Commit Hash**: `ce50d267821c0f24e36ca3bafd847bc1b9e3df08`
- **Prerequisite Plans**: 无
- **Supersedes**: 无

---

## 已读物理证据清单 (Gate 1 Evidence)
1. **物理读取文件列表**：
   - `src/ui/ContentPanel.h`
   - `src/ui/ContentPanel.cpp`
2. **头文件原样签名核查**：
   - `void updateStatusBarStats(int cachedSelectedCount = -1);` (`src/ui/ContentPanel.h:247`)
3. **既有 API 检索日志**：
   - `grep -rn "updateStatusBarStats" src/ui/ContentPanel.cpp`：发现 `emit statusBarMessageReady(statusText)` 广播状态栏文案。

---

## Chapter 1: Overview（概述与根因分析）

### 1. 需求分析
用户期望将底部状态栏左侧统计文本中的**数量值**（例如项目总数、已隐藏数、选中数等数字）高亮显示为鲜艳的橘红色 **`#ff551c`**，以增强深色状态栏下的数据可读性与视觉引导。

### 2. 方案设计
在 `ContentPanel::updateStatusBarStats` 中，将原有的纯文本格式化字符串改造为包含富文本 HTML 标签的格式化字符串，将具体的数字包裹在 `<span style="color:#ff551c; font-weight:bold;">...</span>` 中。
状态栏控件 `QLabel#StatusBarLeft` 原生支持富文本渲染，接收信号后即可自动把数量数字高亮渲染为 `#ff551c` 橘红色。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/ui/ContentPanel.cpp` （修改）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 1. `src/ui/ContentPanel.cpp`
```
<<<<<<< SEARCH
    QString statusText = (hiddenCount > 0)
        ? QString("%1 个项目，%2 个已隐藏，选中了 %3 个").arg(visibleCount).arg(hiddenCount).arg(selectedCount)
        : QString("%1 个项目，选中了 %2 个").arg(visibleCount).arg(selectedCount);

    emit statusBarMessageReady(statusText);
=======
    QString numStyle = "color:#ff551c; font-weight:bold;";
    QString statusText = (hiddenCount > 0)
        ? QString("<span style=\"%1\">%2</span> 个项目，<span style=\"%1\">%3</span> 个已隐藏，选中了 <span style=\"%1\">%4</span> 个")
              .arg(numStyle).arg(visibleCount).arg(hiddenCount).arg(selectedCount)
        : QString("<span style=\"%1\">%2</span> 个项目，选中了 <span style=\"%1\">%3</span> 个")
              .arg(numStyle).arg(visibleCount).arg(selectedCount);

    emit statusBarMessageReady(statusText);
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 1. 构建步骤
```bash
cmake --build build --config Debug
```

### 2. 运行时验收标准
1. **数字高亮展示**：打开任意文件夹，观察底部状态栏左侧，数字部分（如 `7`、`0`）精确高亮渲染为 `#ff551c` 橘红色。
2. **筛选与选中响应**：对项目进行选择或筛选，隐藏数字与选中数字同步刷新，高亮着色不变。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- 复用 `ContentPanel::updateStatusBarStats` 官方统计更新入口，未另起炉灶。
- 复用 Qt QLabel 原生 HTML 富文本渲染引擎。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 状态栏统计数字 | `SectionProxyModel` / `DiskItemModel` 统计结果 | `ContentPanel::updateStatusBarStats` 富文本文案 | 变化时由 `statusBarMessageReady` 广播给 `MainWindow` |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

| 外部调用接口 | 物理 .h 中的声明行 | 访问级别 |
| :--- | :--- | :--- |
| `updateStatusBarStats` | `src/ui/ContentPanel.h:247` | `private` |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- 仅修改 `ContentPanel.cpp` 内部格式化逻辑，无需新增头文件。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项 | 结论 | 关联位置/说明 |
| :--- | :--- | :--- | :--- |
| 1 | 方案位于物理隔离目录 | 满足 | `StatusBarNumberColor.md` |
| 2 | SEARCH 块原文存在且唯一 | 满足 | 精准匹配 `ContentPanel.cpp:948-953` |
| 3 | 10 大章节完整 | 满足 | Chapter 1 ~ 10 全齐 |
| 4 | 无 Win32 / Qt::Popup Hack | 满足 | 标准 Qt 富文本支持 |
| 5 | 无主/后台线程 UI 违规 | 满足 | GUI 主线程同步刷新 |
| 6 | 样式与数值 100% 照抄 | 满足 | 指定数值 `#ff551c` 原样照抄 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

- **交互行为**：状态栏显示与统计交互保持 100% 完全一致。
- **外观数值**：颜色恰好使用用户指定的 `#ff551c`，未作任何私自修改。
