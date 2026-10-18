# MetaPanel 实施方案 (MetaPanel-11.md)

### 【第 1 关：起草前核查（Pre-Draft Evidence）】
1. **已读文件清单**：
   - `src/ui/MetaPanel.h`（已用工具 `read_file` 物理读取）
   - `src/ui/MetaPanel.cpp`（已用工具 `read_file` 物理读取）
2. **签名原文摘录**：
   - `void setMultiSelectionPreview(const QList<MetaPreviewItem>& items);`（`MetaPanel.h` 第 49 行）
3. **既有 API 检索记录**：
   - 检索关键词：`0.04` / `kMultiPreviewAngleStep` / `setMultiSelectionPreview`
   - 结果：`src/ui/MetaPanel.cpp` 第 35 行定义了 `constexpr double kMultiPreviewAngleStep = 4.0;`，第 525 行定义了 `double scale = 1.0 - (0.04 * k);`。
4. **Memories.md 相关条款**：
   - 零篡改铁律与精细化数值呈现。

---

## 1. Overview（概述与解决的问题）

本实施方案用于对元数据面板 (`MetaPanel`) 中多选预览 (`setMultiSelectionPreview`) 的两项堆叠视觉参数进行精细化微调：
1. 将多选卡片堆叠的**逐层微缩比例**从 **`4%` (`0.04`)** 调整为 **`10%` (`0.10`)**；
2. 将多选卡片堆叠的**逐层倾斜角步长**从 **`4.0°`** 调整为 **`3.0°` (`3`)**。

---

## 2. Modified Files List（影响文件清单）

- `src/ui/MetaPanel.cpp`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 修改点 1：`src/ui/MetaPanel.cpp` - 修改逐层倾斜角步长常量 (4.0 -> 3.0)

<<<<<<< SEARCH
constexpr double kMultiPreviewAngleStep = 4.0;
=======
constexpr double kMultiPreviewAngleStep = 3.0;
>>>>>>> REPLACE

### 修改点 2：`src/ui/MetaPanel.cpp` - 修改多选卡片堆叠逐层微缩比例 (4% -> 10%)

<<<<<<< SEARCH
        double scale = 1.0 - (0.04 * k); // 逐层微缩 4%，露出下层边角
=======
        double scale = 1.0 - (0.10 * k); // 逐层微缩 10%，露出下层边角
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译验证**：
   在终端或构建系统中执行常规 CMake / MSVC 编译。
2. **UI 细节核对**：
   - 多选 2 个或多个文件，观察元数据面板顶部的堆叠预览图。
   - 确认卡片逐层微缩比例已变为 10%，倾斜角旋转步长调整为 3.0°，展现更柔和且层次分明的多选卡片堆叠效果。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）

本次修改仅微调多选绘制算法中的倾斜角度与缩放系数常量，复用既有 `setMultiSelectionPreview` 逻辑，无另起炉灶行为。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

- `void setMultiSelectionPreview(const QList<MetaPreviewItem>& items);`（`MetaPanel.h` 第 49 行）

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

不增加、删除或修改任何 `#include` 头文件包含行。

---

## 8. Compliance Self-Check Table（合规核对表）

| 检查项 | 来源条款 | 结论 | 对应方案位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | 文档标题 (MetaPanel-11.md) |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/` |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 | 第 3 章全部 SEARCH 块 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册，且有配套 .cpp | 3.3 | 不涉及 | 未新建或修改 Q_OBJECT 声明 |
| 第 1~9 章齐全 | 3.4 | 满足 | 第 1~9 章完整覆盖 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 第 6 章 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 不涉及 | 未修改头文件包含 |
| 已复用既有 SSOT 入口，无另起炉灶；旧分散实现已列入删除 | 2.4 | 满足 | 第 5 章 |
| 无内联 `setStyleSheet`，样式走 style.qss | 4.1 | 不涉及 | 不涉及 CSS 修改 |
| 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 均未采用 |
| 批量操作为一次聚合广播 | 4.3 | 不涉及 | 不涉及文件批量 IO |
| 迁移的所有数值原样照抄 | 4.4 | 满足 | 第 9 章 |
| 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories.md | 满足 | 仅精确落实用户指定的微缩 10% 与倾斜角 3.0° |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅影响 `MetaPanel.cpp` |

---

## 9. Zero-Value-Alteration Check（数值零篡改核对）

| 参数/数值描述 | 原始数值 | 新数值 | 变更理由/依据 |
|---|---|---|---|
| 多选卡片逐层微缩比例 (`scale`) | `0.04` (4%) | `0.10` (10%) | 用户明确要求：将微缩 4% 修改成微缩 10% |
| 多选卡片逐层倾斜角步长 (`kMultiPreviewAngleStep`) | `4.0°` | `3.0°` (`3`) | 用户明确要求：逐层倾斜角步长 4.0° 修改成 3 |
