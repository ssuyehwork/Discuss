# 实施方案：FilterProxyModel-6 - 自动色彩筛选 RGB ±20 范围精准匹配

## Pre-Draft Evidence（起草前核查）

### 1. 已读文件清单
- `G:/C++/QuarkMeta/QuarkMeta/src/ui/models/FilterProxyModel.cpp`（已物理读取）
- `G:/C++/QuarkMeta/QuarkMeta/src/ui/FilterStateModel.h`（已物理读取）
- `G:/C++/QuarkMeta/QuarkMeta/src/ui/FilterStateModel.cpp`（已物理读取）
- `G:/C++/QuarkMeta/QuarkMeta/src/ui/AutoColorGroup.cpp`（已物理读取）
- `G:/C++/QuarkMeta/QuarkMeta/src/ui/UiHelper.h`（已物理读取）
- `G:/C++/QuarkMeta/QuarkMeta/src/util/ColorPaletteEngine.h`（已物理读取）

### 2. 外部签名原文摘录
- `QColor::red()`: `int red() const noexcept;` (Qt 源码 / qcolor.h)
- `QColor::green()`: `int green() const noexcept;` (Qt 源码 / qcolor.h)
- `QColor::blue()`: `int blue() const noexcept;` (Qt 源码 / qcolor.h)
- `QColor::isValid()`: `bool isValid() const noexcept;` (Qt 源码 / qcolor.h)
- `UiHelper::parseColorName`: `static inline QColor parseColorName(const QString& colorName)` (UiHelper.h L39)
- `AppConfig::getValue`: `QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;` (AppConfig.h L33)

### 3. 既有 API 检索记录
- 检索关键词：`calculateDeltaE`，命中 `UiHelper::calculateDeltaE` / `ColorPaletteEngine::calculateDeltaE`。原逻辑采用 CIEDE2000 色差模型，因非线性映射导致 RGB 色差过大仍被召回。
- 检索关键词：`ColorSimilarity` / `colorTolerance`，命中 `FilterStateModel`、`FilterProxyModel`、`AutoColorGroup`，为现有容差控制 SSOT 通道。

### 4. Memories.md 相关条款
- **筛选联动与性能契约**：筛选器状态变动驱动 `FilterProxyModel::updateFilter()` 原位重刷，保证过滤算法轻量、无内存分配泄漏。
- **零篡改红线**：UI 控件布局、滑条范围 (0~100)、占比滑条及标准色块保持既有状态不变，仅修正容差匹配计算准则与默认基准值。

---

## 1. Overview（概述与解决的问题）

当前版本在进行自动提取色彩筛选时，采用 CIEDE2000 色差感知公式（`UiHelper::calculateDeltaE`）与容差阈值比较。由于 CIEDE2000 基于 Lab 空间且具备复杂的非线性加权，在处理饱和度较高的颜色（如青蓝 `#007FFF`）时，部分在屏幕视觉和 RGB 数值上已经显著偏色的项依然落入容差圈内，与用户直觉的“相近色”严重脱节。

本方案将色彩过滤从非线性的 Lab DeltaE 改造为直接、精准的 **RGB 分量多维容差匹配**：
1. 默认容差设定为 `20`（即 RGB 三通道分量变动均在基准色 $\pm 20$ 之内）；
2. 过滤算法对目标色与调色板色/主色进行通道级核验：$|R_1 - R_2| \le \text{tol} \land |G_1 - G_2| \le \text{tol} \land |B_1 - B_2| \le \text{tol}$；
3. 以 `#007FFF` ($R=0, G=127, B=255$) 为例，默认容差下 $G$ 通道允许范围精准收敛于 $107 \sim 147$，超出即判定偏离；
4. 容差无缝对接 `AutoColorGroup` 的精准度滑条，用户微调滑条时实时按通道容差平滑缩放。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/FilterStateModel.h` - 更新 `FilterState::colorTolerance` 默认值为 20；
2. `src/ui/FilterStateModel.cpp` - 更新从 `AppConfig` 获取 `Filter/ColorSimilarity` 的默认后备值为 20；
3. `src/ui/models/FilterProxyModel.cpp` - 将色彩过滤匹配逻辑替换为 RGB 三通道容差比较算法。

---

## 3. Detailed Line-by-Line Changes（精准代码替换块）

### 修改文件 1：`src/ui/FilterStateModel.h`

<<<<<<< SEARCH
    QList<int>   ratings;
    QStringList  colors;          // 自动提取色彩过滤色 (Hex 字符串，如 #E24B4A)
    QStringList  manualColors;    // 手动标注颜色过滤 (红、橙、黄、绿、青、蓝、紫、灰、无色标)
    int          colorTolerance = 33; // 相似度滑条值 0~100，默认 33
    int          minColorArea = 0;    // 占比 (0~100，默认 0%)
=======
    QList<int>   ratings;
    QStringList  colors;          // 自动提取色彩过滤色 (Hex 字符串，如 #E24B4A)
    QStringList  manualColors;    // 手动标注颜色过滤 (红、橙、黄、绿、青、蓝、紫、灰、无色标)
    int          colorTolerance = 20; // 相似度/容差滑条值 0~100，默认 20 (RGB各通道±20)
    int          minColorArea = 0;    // 占比 (0~100，默认 0%)
>>>>>>> REPLACE

---

### 修改文件 2：`src/ui/FilterStateModel.cpp`

<<<<<<< SEARCH
FilterStateModel::FilterStateModel(QObject* parent) : QObject(parent) {
    m_state.colorTolerance = AppConfig::instance().getValue("Filter/ColorSimilarity", 33).toInt();
    m_state.minColorArea = AppConfig::instance().getValue("Filter/MinColorArea", 0).toInt();
}
=======
FilterStateModel::FilterStateModel(QObject* parent) : QObject(parent) {
    m_state.colorTolerance = AppConfig::instance().getValue("Filter/ColorSimilarity", 20).toInt();
    m_state.minColorArea = AppConfig::instance().getValue("Filter/MinColorArea", 0).toInt();
}
>>>>>>> REPLACE

---

### 修改文件 3：`src/ui/models/FilterProxyModel.cpp`

<<<<<<< SEARCH
    // 3.5 🚨 物理隔离：自动提取色彩过滤 (只针对 record.autoColor & record.palettes)
    if (!currentFilter.colors.isEmpty()) {
        bool matchAuto = false;
        double maxDeltaE = (currentFilter.colorTolerance / 100.0) * FilterState::kMaxSimilarityDeltaE;

        auto calculateMatchedArea = [&](const QColor& targetCol) -> float {
            if (!targetCol.isValid()) return 0.0f;
            float totalArea = 0.0f;
            if (!record.palettes.empty()) {
                for (const auto& pe : record.palettes) {
                    if (UiHelper::calculateDeltaE(targetCol, pe.first) <= maxDeltaE) {
                        totalArea += pe.second;
                    }
                }
            } else if (!record.autoColor.isEmpty()) {
                QColor recordCol = UiHelper::parseColorName(record.autoColor);
                if (UiHelper::calculateDeltaE(targetCol, recordCol) <= maxDeltaE) {
                    totalArea = 1.0f;
                }
            }
            return totalArea;
        };

        for (const QString& fc : currentFilter.colors) {
            QColor targetCol = UiHelper::parseColorName(fc);
            float area = calculateMatchedArea(targetCol);
            if (area > 0.0f && (area * 100.0f >= static_cast<float>(currentFilter.minColorArea))) {
                matchAuto = true;
                break;
            }
        }
        if (!matchAuto) return false;
    }
=======
    // 3.5 🚨 物理隔离：自动提取色彩过滤 (只针对 record.autoColor & record.palettes)
    if (!currentFilter.colors.isEmpty()) {
        bool matchAuto = false;
        const int tol = currentFilter.colorTolerance;

        auto isColorSimilar = [tol](const QColor& c1, const QColor& c2) -> bool {
            if (!c1.isValid() || !c2.isValid()) return false;
            return std::abs(c1.red() - c2.red()) <= tol &&
                   std::abs(c1.green() - c2.green()) <= tol &&
                   std::abs(c1.blue() - c2.blue()) <= tol;
        };

        auto calculateMatchedArea = [&](const QColor& targetCol) -> float {
            if (!targetCol.isValid()) return 0.0f;
            float totalArea = 0.0f;
            if (!record.palettes.empty()) {
                for (const auto& pe : record.palettes) {
                    if (isColorSimilar(targetCol, pe.first)) {
                        totalArea += pe.second;
                    }
                }
            } else if (!record.autoColor.isEmpty()) {
                QColor recordCol = UiHelper::parseColorName(record.autoColor);
                if (isColorSimilar(targetCol, recordCol)) {
                    totalArea = 1.0f;
                }
            }
            return totalArea;
        };

        for (const QString& fc : currentFilter.colors) {
            QColor targetCol = UiHelper::parseColorName(fc);
            float area = calculateMatchedArea(targetCol);
            if (area > 0.0f && (area * 100.0f >= static_cast<float>(currentFilter.minColorArea))) {
                matchAuto = true;
                break;
            }
        }
        if (!matchAuto) return false;
    }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 编译步骤
在项目根目录执行：
```powershell
cmake --build build --config Debug
```
或执行发布版编译验证无任何告警与未解析外部符号。

### 验证方法
1. 启动应用程序，定位到包含多种颜色图片/素材的目录；
2. 展开左侧/筛选面板，在自动色彩中选择青蓝色（如 `#007FFF`）；
3. 检查默认精准度（20）下筛选出的文件，其主色/调色板 RGB 各通道与 `#007FFF` 比较，分量差值不超过 20；
4. 验证原偏色样本（如 $G$ 小于 107 或大于 147 的项）已被彻底过滤排除；
5. 拖动精准度滑条，验证当容差缩小时匹配更加严苛，扩大时平滑放宽，交互顺畅无抖动。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）

- **FilterState 数据流**：完全沿用现有的 `FilterState::colorTolerance` 字段及 `Filter/ColorSimilarity` 配置项，无另立字段或冗余结构；
- **色彩解析入口**：继续使用 `UiHelper::parseColorName` 作为 Hex/颜色名转 `QColor` 的唯一入口；
- **物理死代码清除**：`FilterProxyModel` 内部原 Lab DeltaE 计算调用点被完全替换，杜绝两套匹配逻辑共存。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用的成员/API | 所属类 / 头文件 | 物理精确签名 |
|---|---|---|
| `red()` | `QColor` (`<QColor>`) | `int red() const noexcept` |
| `green()` | `QColor` (`<QColor>`) | `int green() const noexcept` |
| `blue()` | `QColor` (`<QColor>`) | `int blue() const noexcept` |
| `isValid()` | `QColor` (`<QColor>`) | `bool isValid() const noexcept` |
| `parseColorName(...)` | `UiHelper` (`UiHelper.h`) | `static inline QColor parseColorName(const QString& colorName)` |
| `getValue(...)` | `AppConfig` (`AppConfig.h`) | `QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const` |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

- `src/ui/models/FilterProxyModel.cpp`：
  - `<cmath>` 已包含（用于 `std::abs`）；
  - `QColor` 已通过 `UiHelper.h` 间接完整包含；
  - 本次未移除任何头文件包含，不存在间接断裂风险。
- `src/ui/FilterStateModel.h` / `src/ui/FilterStateModel.cpp`：
  - 仅修改结构体内置整型初始默认值，不涉及新类型引入。

---

## 8. Compliance Self-Check Table（合规核对表）

| 检查项 | 来源条款 | 结论 | 对应方案位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | `FilterProxyModel-6.md` 对应核心类 `FilterProxyModel` |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/` |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 | 第 3 章代码块均通过实物查看完全对应 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册，且有配套 .cpp | 3.3 | 不涉及 | 未新建任何包含 Q_OBJECT 的类 |
| 第 1~9 章齐全 | 3.4 | 满足 | 涵盖全部 9 个章节 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 见第 6 章签名核查表 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 满足 | 见第 7 章包含链检查 |
| 已复用既有 SSOT 入口，无另起炉灶；旧分散实现已列入删除 | 2.4 | 满足 | 见第 5 章，完全复用既有通道 |
| 无内联 `setStyleSheet`，样式走 style.qss | 4.1 | 不涉及 | 本方案不涉及 UI 样式变动 |
| 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 纯数据模型与过滤判定优化，无 Hack |
| 批量操作为一次聚合广播 | 4.3 | 不涉及 | 代理模型行级接受过滤，不涉及批量广播 |
| 迁移的所有数值原样照抄 | 4.4 | 不涉及 | 本方案不涉及样式属性数值迁移 |
| 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories.md | 满足 | 严格贴合用户提出的 RGB 通道 ±20 匹配要求 |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅涉及必要的 3 个模型过滤文件 |

---

## 9. Zero-Value-Alteration Check（数值零篡改核对）

- **不涉及**：本方案属于过滤算法与匹配准则的逻辑优化，不涉及任何 UI 控件尺寸、边框、颜色值等样式的迁移与改动。
