# 实施方案：色彩过滤增加明度约束消除暗色混入 (FilterProxyModel-7.md)

## 0. 起草前核查（Pre-Draft Evidence）

根据 AGENTS.md 3.6 第 1 关规范，实施本方案前物理核查证据如下：

### 0.1 已读文件清单
- `src/ui/models/FilterProxyModel.h`：已物理阅读，确认类声明及成员函数，接口完全只读冻结。
- `src/ui/models/FilterProxyModel.cpp`：已物理阅读（行 80~125），定位到自动颜色过滤比对逻辑 `isMatch`。

### 0.2 签名原文摘录
本方案在 `isMatch` 闭包中调用的外部 Qt / C++ 成员函数声明如下：
- `int QColor::value() const noexcept;`
- `int qRound(double d);`
- `int std::abs(int n);`

### 0.3 既有 API 检索记录
- 检索关键词：`isMatch`、`currentFilter.colors`、`colorPrecision`
- 检索结果：自动提取色彩的判定算法集中于 `FilterProxyModel.cpp` 的 `filterAcceptsRow` 局部闭包中，无分散另起炉灶。

### 0.4 Memories.md 相关条款
- 遵守【契约锁】：对外公开的 `.h` 接口签名一律只读冻结，不破坏既有调用契约。
- 遵守【真理源溯源 (SSOT)】：沿用 `currentFilter.colorPrecision` 作为容差动态计算的基准真理源。

---

## 1. Overview（概述与解决的问题）

### 1.1 背景与解决的问题
用户在筛选鲜亮大红（如 `#FF0400`，V=255）时，极暗黑红（如 `#350D0D`，V=53）被同时筛选出来。
**根因定位**：现有 `isMatch` 算法仅比对了色相差（$\le 20^\circ$）和饱和度最低阈值，完全缺失对明度（Value）差异的校验，导致虽然色相相同但亮度极端悬殊的暗褐色/深黑红被误判为同类色放行。

### 1.2 解决措施
在 `isMatch` 有彩色分支中增加明度差（Value）动态约束：
- 提取目标色与被测色的 HSV 明度值 `targetCol.value()` 与 `pc.value()`；
- 根据用户设置的精准度（`colorPrecision`：0~100）动态计算允许的最大明度容差范围（`150 ~ 60`）；
- 阻断明度差异过大的暗色（如 `#350D0D` 相对 `#FF0400` 的明度差高达 202，远超容差阈值，将被精准排除）。

---

## 2. Modified Files List（影响文件清单）

- `src/ui/models/FilterProxyModel.cpp`

---

## 3. Detailed Line-by-Line Changes（包含 CMakeLists.txt 在内的精准替换块）

### 修改文件：`src/ui/models/FilterProxyModel.cpp`

<<<<<<< SEARCH
                double minSat = 255.0 * (1.0 - static_cast<double>(currentFilter.colorPrecision) / 100.0);
                return static_cast<double>(pcSat) >= minSat;
            } else {
=======
                double minSat = 255.0 * (1.0 - static_cast<double>(currentFilter.colorPrecision) / 100.0);
                if (static_cast<double>(pcSat) < minSat) return false;

                // 明度约束：避免亮色误匹配极暗色（如 #FF0400 误匹配极暗红 #350D0D）
                // 允许的最大明度差随精准度动态收敛：精准度 0~100 对应明度差容差 150~60
                double precisionFactor = static_cast<double>(currentFilter.colorPrecision) / 100.0;
                int maxValDiff = qRound(150.0 - 90.0 * precisionFactor);
                return std::abs(pc.value() - targetCol.value()) <= maxValDiff;
            } else {
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 4.1 编译验证
在项目根目录构建：
```powershell
cmake --build "G:\C++\QuarkMeta\QuarkMeta_Build\x64-release"
```
确保 `FilterProxyModel.cpp.obj` 编译通过。

### 4.2 筛选验证
1. 在色盘选择鲜亮大红（如 `#FF0400`）：
   - 包含亮红花卉、鲜红图案的文件正常展示；
   - 极暗红/暗褐色的素材（如 `#350D0D`）被成功排除。
2. 在暗色色块或色盘偏暗区域选择较暗红色：
   - 目标为暗红时，暗红图片正常展示，鲜亮红色图片被合理隔离。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）

- 复用既有 `currentFilter.colorPrecision` 状态与 `isMatch` 闭包判定管道，零冗余新增，未破坏单向依赖流。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用成员 / API | 所在头文件 | 物理精确签名 | 核查状态 |
|---|---|---|---|
| `QColor::value` | `<QColor>` | `int QColor::value() const noexcept;` | 100% 吻合 |
| `qRound` | `<QtMath>` | `int qRound(double d);` | 100% 吻合 |
| `std::abs` | `<cmath>` | `int std::abs(int n);` | 100% 吻合 |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

- `<cmath>` 已在 `FilterProxyModel.cpp` 第 6 行显式包含；
- `<QColor>` 由 `ItemModelBase.h` 与 `FilterProxyModel.h` 完整引入；
- 不修改任何 `#include`，无隐式断裂风险。

---

## 8. Compliance Self-Check Table（合规核对表）

根据 AGENTS.md 3.6 第 2 关规范，交付前自检结果如下：

| 检查项 | 来源条款 | 自检结果 | 对应方案位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | `FilterProxyModel-7.md` 映射 `FilterProxyModel` 类 |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/FilterProxyModel-7.md` |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 | 第 3 节 SEARCH 块与 `FilterProxyModel.cpp` 行 102~104 真实代码 100% 吻合 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册，且有配套 .cpp | 3.3 | 不涉及 | 未新建类 |
| 第 1~9 章齐全 | 3.4 | 满足 | 方案包含 0~9 所有必备章节 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 见第 0.2 节与第 6 节核查表 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 不涉及 | 未变动任何 `#include` 行，见第 7 节 |
| 已复用既有 SSOT 入口，无另起炉灶；旧分散实现已列入删除 | 2.4 | 满足 | 见第 5 节 |
| 无内联 `setStyleSheet`，样式走 style.qss | 4.1 | 不涉及 | 纯数据过滤模型逻辑 |
| 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 标准 Qt 代理模型数据过滤 |
| 批量操作为一次聚合广播 | 4.3 | 不涉及 | 纯行过滤匹配判定 |
| 迁移的所有数值原样照抄 | 4.4 | 不涉及 | 本次为色彩明度匹配补全，见第 9 节 |
| 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories.md | 满足 | 仅补全色彩过滤中的明度算法判定 |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅影响 `FilterProxyModel.cpp`，无多余文件触碰 |

---

## 9. Zero-Value-Alteration Check（数值零篡改核对）

- 本方案不涉及任何 UI 样式参数迁移。
- 色相容差 `kHueTolerance = 20`、饱和度有效阈值 `38` 保持 100% 不变。
