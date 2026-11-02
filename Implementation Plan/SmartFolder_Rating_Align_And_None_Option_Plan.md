# SmartFolder 星级下拉靠左对齐与“无”评分选项实施方案 (SmartFolder Rating Align & None Option Plan)

语言：正文与注释只允许使用中文（标识符、代码块、标准 Git Merge Diff 标记与固定章节英文标题除外）。

---

## 物理证据清单 (Pre-Draft Evidence)
1. **物理读取文件列表**：
   - `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.h`：已读取，确认 `StarRatingValueWidget` 类定义。
   - `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.cpp`：已读取，确认 `StarRatingValueWidget` 构造函数第 33~63 行循环构建 `m_combo` 选项，原循环仅生成 1~5 星，且 pixmap 宽度可变未贴齐画布左侧。
   - `QuarkMeta_Source/src/ui/dialogs/SmartFolderRuleRow.cpp`：已读取，确认第 200~215 行当 `testRule.value.toInt() == 0` 时自动锁定操作符为 `Equals`。
   - `QuarkMeta_Source/src/meta/SmartFolderDao.cpp`：已读取，确认第 281~289 行 `buildRatingCondition` 已完美支持 `r == 0` 生成 `rating = 0`。
2. **头文件原样签名核验**：
   - `StarRatingValueWidget::StarRatingValueWidget(QWidget* parent = nullptr);`
   - `virtual void getRuleValues(SmartFolderRule& rule) const override;`
   - `virtual void setRuleValues(const SmartFolderRule& rule) override;`
3. **既有 API 检索日志**：
   - `grep -rn "UiHelper::getIcon(\"star_filled\"" QuarkMeta_Source/` -> 确认直接复用 `UiHelper::getIcon("star_filled", QColor("#C5D1DE"))`。

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题现象与根因分析
1. **排版松散与未靠左对齐缺陷根因**：
   - 在 `StarRatingValueWidget` 中，`m_combo->setIconSize(QSize(100, 18))` 设置了 100 像素宽度的图标渲染槽位；
   - 原代码在为 1 星至 5 星生成 Pixmap 时，其画布宽度计算为 `maxW = stars * starSize + (stars - 1) * spacing`（如 1 星宽度为 14px，2 星宽度为 32px）；
   - Qt 默认的下拉框样式与项代理在将一个较窄的 Pixmap（如 14px）绘制到 100px 的图标槽位中时，会自动采取**居中对齐**策略（`(100 - 14) / 2 = 43px` 偏移），导致 1 星在视觉上严重向右偏移了约 43 像素，呈现出离散、居中未靠齐左边界的丑陋排版；
   - **架构修复**：将 Pixmap 的画布宽度统一规范为满槽宽度（固定宽度 `iconWidth = 5 * starSize + 4 * spacing = 86px`），所有星级图标均从画布起点 `x = 0` 开始向右绘制，右侧空白区域透明填充，从而使 Qt 在图标槽位中精准贴齐左边缘，实现绝对靠左对齐。
2. **缺少“无”评级选项缺陷根因**：
   - 原代码 `for (int stars = 1; stars <= 5; ++stars)` 仅提供了 1 星至 5 星，缺少未评分项目的筛选能力；
   - 底层数据库检索层（`SmartFolderDao::buildRatingCondition`）与规则行数据监听早已完备支持 `r == 0`（返回 `rating = 0` SQL 条件），但前端 UI 下拉框缺失首项“无”；
   - **架构修复**：在下拉列表首项新增“无”选项，关联数值 `0`。选项文本显示为 `"无"`，为用户提供极其清晰直观的未评分项目筛选入口。

### 1.2 实施目标
1. 规范 `StarRatingValueWidget` 下拉框项渲染，所有星级图标严格从最左侧对齐；
2. 在下拉框首项补充“无”选项，赋值为 `0`；
3. 确保选择“无”时操作符自动联动为“等于”，且底层精确检索 `rating = 0` 的未评级项目。

---

## Chapter 2: Modified Files List（影响文件清单）

本次方案仅优化智能文件夹星级值选择控件的绘制与选项生成，涉及文件如下：
1. `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.cpp`（修改：统一星级 Pixmap 画布宽度实现严格靠左对齐，并在首项添加“无”选项）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.cpp`

```diff
<<<<<<< SEARCH
    const int starSize = 14;
    const int spacing = 4;
    for (int stars = 1; stars <= 5; ++stars) {
        int maxW = stars * starSize + (stars - 1) * spacing;
        QPixmap pixmap(maxW, starSize);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        int x = 0;
        for (int i = 0; i < stars; ++i) {
            QIcon icon = UiHelper::getIcon("star_filled", QColor("#C5D1DE"));
            icon.paint(&painter, x, 0, starSize, starSize);
            x += starSize + spacing;
        }

        m_combo->addItem(QIcon(pixmap), "", stars);
    }
=======
    // 添加“无”选项（对应 rating = 0），专门筛选无星级/评分的项目
    m_combo->addItem("无", 0);

    const int starSize = 14;
    const int spacing = 4;
    const int fullIconWidth = 5 * starSize + 4 * spacing; // 86px 固定画布宽度，保证图标严格靠左对齐

    for (int stars = 1; stars <= 5; ++stars) {
        QPixmap pixmap(fullIconWidth, starSize);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        int x = 0;
        for (int i = 0; i < stars; ++i) {
            QIcon icon = UiHelper::getIcon("star_filled", QColor("#C5D1DE"));
            icon.paint(&painter, x, 0, starSize, starSize);
            x += starSize + spacing;
        }

        m_combo->addItem(QIcon(pixmap), "", stars);
    }
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 构建命令
在构建工作区运行：
```bash
cmake -B build -S QuarkMeta_Source
cmake --build build --target QuarkMeta -j4
```

### 4.2 运行时验收标准
1. **下拉框首项包含“无”**：
   - 打开智能文件夹规则配置对话框，规则类型选择“星级”，点击第三列星级下拉框；
   - 下拉列表的第一项显示为“无”，后跟 1 星至 5 星。
2. **星级图标严格靠左对齐**：
   - 展开下拉列表，1 星至 5 星的所有星星图标均贴齐列表项最左边缘垂直对齐，彻底消除原先 1 星悬空在中央、排版散乱的现象。
3. **“无”选项与操作符自动联动**：
   - 当在星级下拉框中切换到“无”时，中间的操作符下拉框自动切换并锁定为“等于”；
   - 点击保存后，规则生成的 SQL 条件为 `rating = 0`，准确筛选出所有未评分的文件。
4. **既有规则兼容性**：
   - 编辑原有已保存的 1~5 星智能文件夹规则，加载规则时正确选中对应星数，既有配置 100% 正常回显。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **复用 `UiHelper::getIcon("star_filled", ...)`**：严格复用既有矢量图标加载机制，杜绝引入任何 Unicode 字符或多余依赖；
2. **复用 `SmartFolderDao::buildRatingCondition`**：完全复用已支持 `r == 0` 的底层 DAO 架构，不增加冗余分支；
3. **复用 `SmartFolderRuleRow` 既有联动**：利用既有的 `testRule.value.toInt() == 0` 监听逻辑自动同步操作符。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | SSOT 唯一数据源 | 派生副本/UI表现 | 同步与刷新方式 |
| :--- | :--- | :--- | :--- |
| 星级规则数值 | `SmartFolderRule::value` / `SmartFolderRule::num1` | `StarRatingValueWidget::m_combo` (currentData) | 双向同步：`getRuleValues` 导出数值，`setRuleValues` 驱动下拉框匹配回显 |
| 规则类型 | `SmartFolderRule::type` (枚举值 `SmartRuleType::Rating`) | `SmartFolderRuleRow::m_typeCombo` | 切换时动态挂载 `StarRatingValueWidget` |
| 筛选条件 | SQLite 数据库 `library_items.rating` | `SmartFolderDao::buildRatingCondition` | `rating = 0` (对应“无”) 或 `rating >= / <= / = N` |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. **外部调用接口核查**：
   - `StarRatingValueWidget` 构造函数与虚函数签名保持完全一致，对外接口签名 100% 冻结不变。
2. **跨类访问权限核查**：
   - 本次修改仅在 `StarRatingValueWidget` 构造函数内部调整局部 Pixmap 生成逻辑与添加数据项，不涉及任何跨类私有成员访问。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

1. `SmartFolderValueWidgets.cpp` 头部已显式包含：
   - `#include "SmartFolderValueWidgets.h"`
   - `#include "../../ui/UiHelper.h"`
   - `#include <QPainter>`
   - `#include <QPixmap>`
2. 未引入任何新的外部类型或前置声明，包含链完全闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 检查项 | 状态 | 说明 |
| :--- | :--- | :--- |
| 1. SEARCH 块原文真实存在且唯一匹配 | 满足 | 经 `tools/plan_verify/verify_plan.py` 自动化检测，唯一匹配且仅匹配 1 处 |
| 2. 外部成员函数签名一致 | 满足 | `m_combo->addItem` 与 `UiHelper::getIcon` 签名完全匹配 |
| 3. 新增接口全链核查 | 不涉及 | 本次不新增对外公开接口 |
| 4. #include 完整闭合 | 满足 | 头文件包含链完备 |
| 5. CMakeLists.txt 同步注册 | 不涉及 | 无新建或删除源码文件 |
| 6. 跨类访问控制权限合法 | 满足 | 无越权访问 |
| 7. 方案未触及范围外文件 | 满足 | 仅涉及目标控件文件 |
| 8. 杜绝盲目修改/架构警告核查 | 满足 | 遵循 Memories.md 第 27 与 34 章节设计规范 |
| 9. 异步并发安全与生命周期 | 不涉及 | 本次不涉及多线程并发 |
| 10. 10 大必备章节完整性 | 满足 | 完整包含 Chapter 1 至 Chapter 10 |

### 自动化验证执行输出日志：
```text
=== [Plan Verification] Starting verification on: Implementation Plan/SmartFolder_Rating_Align_And_None_Option_Plan.md ===
[+] Chapter completeness check (All 10 chapters present): PASSED
[+] Hallucinated API / Macro check: PASSED (None found)
[+] Total SEARCH/REPLACE blocks found: 1
[+] SEARCH block in QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.cpp: Exactly 1 unique match.
[+] Physical source code matching: ALL PASSED
=== [Plan Verification] SUCCESS: Implementation Plan/SmartFolder_Rating_Align_And_None_Option_Plan.md satisfies all verification rules! ===
```

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 10.1 交互与行为零篡改核查
- 1 星至 5 星的选择、过滤、持久化及反序列化逻辑保持 100% 行为一致；
- 规则行的高度、删除逻辑、保存逻辑保持不变。

### 10.2 样式数值零篡改核查
- `starSize = 14` 像素保持严格不变；
- `spacing = 4` 像素保持严格不变；
- 星星图标颜色 `#C5D1DE` 保持严格不变；
- 图标总槽位高度 18 像素保持严格不变。
