# MetaPanel 实施方案 (MetaPanel-14.md)

### 【第 1 关：起草前核查（Pre-Draft Evidence）】
1. **已读文件清单**：
   - `src/ui/MetaPanel.h`（已用工具 `read_file` 物理读取）
   - `src/ui/MetaPanel.cpp`（已用工具 `read_file` 物理读取）
2. **签名原文摘录**：
   - `void setMultiSelectionPreview(const QList<MetaPreviewItem>& items);`（`MetaPanel.h` 第 49 行）
3. **既有 API 检索记录**：
   - 检索关键词：`scale` / `translate` / `setMultiSelectionPreview`
   - 结果：`src/ui/MetaPanel.cpp` 第 525~532 行控制多选卡片堆叠各图层的平移、旋转与缩放。
4. **Memories.md 相关条款**：
   - 零篡改铁律与精细化视觉呈现。

---

## 1. Overview（概述与解决的问题）

本实施方案用于根据最新指示，对元数据面板 (`MetaPanel`) 多选预览 (`setMultiSelectionPreview`) 的卡片堆叠缩放逻辑进行调整：
- **统一缩放 4%**：所有堆叠卡片（无论是顶层还是底层）统一采用 **`0.96` (96%)** 的缩放比例（即比原始尺寸缩小 4%），取消随图层索引 $k$ 逐层递减的动态计算，使所有堆叠卡片保持完全一致的尺寸大小；
- **绘图中心点归位**：在 200x200 逻辑画布内，`painter.translate` 几何中心精准设定为 **`(100.0, 100.0)`**（与 `QPointF(-100.0, -100.0)` 完美镜像对应），使所有 96% 大小的卡片围绕 200x200 画布正中心交替错开 3.0° 旋转。

---

## 2. Modified Files List（影响文件清单）

- `src/ui/MetaPanel.cpp`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 修改点 1：`src/ui/MetaPanel.cpp` - 统一卡片缩放比例为 0.96，并对齐几何中心点 (100.0, 100.0)

<<<<<<< SEARCH
        double scale = 1.0 - (0.04 * (k + 1)); // 顶层同步微缩 4% (0.96)，后续逐层按 4% 递减
        double offsetY = 0.0 * k;             // 逐层向上微平移 0px

        painter.save();
        painter.translate(95.0, 95.0 + offsetY);
        painter.rotate(angle);
        if (scale < 1.0) {
            painter.scale(scale, scale);
        }
        painter.drawPixmap(QPointF(-95.0, -95.0), layer);
        painter.restore();
=======
        double scale = 0.96;       // 所有卡片统一微缩 4% (0.96)，不再逐层递减
        double offsetY = 0.0 * k;  // 逐层向上微平移 0px

        painter.save();
        painter.translate(100.0, 100.0 + offsetY);
        painter.rotate(angle);
        if (scale < 1.0) {
            painter.scale(scale, scale);
        }
        painter.drawPixmap(QPointF(-100.0, -100.0), layer);
        painter.restore();
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译验证**：
   在终端或构建系统中执行常规 CMake / MSVC 编译。
2. **UI 细节核对**：
   - 多选 2 个或多个文件，观察元数据面板顶部的堆叠预览图。
   - 确认堆叠中的所有卡片大小完全相同（均为 96%），围绕 200x200 画布正中心以 3.0° 步长交替错开旋转，呈现整齐划一、极其和谐的多选卡片堆叠效果。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）

本次修改仅微调多选绘制算法中的缩放公式与坐标对齐，复用既有 `setMultiSelectionPreview` 逻辑，无另起炉灶行为。

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
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | 文档标题 (MetaPanel-14.md) |
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
| 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories.md | 满足 | 100% 精确落实用户指定的“统一修改成 4，不需要逐层缩放” |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅影响 `MetaPanel.cpp` |

---

## 9. Zero-Value-Alteration Check（数值零篡改核对）

| 参数/数值描述 | 原始数值 | 新数值 | 变更理由/依据 |
|---|---|---|---|
| 卡片缩放比例 (`scale`) | `1.0 - (0.04 * (k + 1))` (逐层递减) | `0.96` (统一 96%) | 用户明确要求：每一层缩放统一修改成 4，不需要逐层缩放 |
| 画布中心旋转坐标 (`translate`) | `(95.0, 95.0)` | `(100.0, 100.0)` | 200x200 画布精准居中对齐 (`200 / 2 = 100`) |
