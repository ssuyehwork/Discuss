# 实施方案：色彩输入无损过滤与明度严格约束 (FilterPanel-6.md)

## 0. 起草前核查（Pre-Draft Evidence）

根据 AGENTS.md 3.6 第 1 关规范，实施本方案前物理核查证据如下：

### 0.1 已读文件清单
- `src/ui/AutoColorGroup.cpp`：已物理阅读（行 190~255），定位到色码输入框的事件响应与 `requestRebuild` 触发点。
- `src/ui/FilterPanel.cpp`：已物理阅读（行 440~530），定位到 `rebuildGroups()` 容器销毁与滚动条容器。
- `src/ui/models/FilterProxyModel.cpp`：已物理阅读（行 85~120），定位到自动颜色匹配函数 `isMatch` 的明度与色相容差计算。

### 0.2 签名原文摘录
本方案调用的外部 Qt 成员函数声明如下：
- `int QScrollBar::value() const;`
- `void QScrollBar::setValue(int);`
- `QScrollBar* QAbstractScrollArea::verticalScrollBar() const;`
- `int QColor::value() const noexcept;`
- `int qRound(double d);`
- `int std::abs(int n);`

### 0.3 既有 API 检索记录
- 检索关键词：`verticalScrollBar`、`requestRebuild`、`isMatch`
- 检索结果：滚动条位置由 `m_scrollArea->verticalScrollBar()` 统管；过滤状态由 `filterModel->setState(st)` 发送信号驱动，无需调用 `rebuildGroups()` 销毁全部 UI 树。

### 0.4 Memories.md 相关条款
- 遵守【零篡改与交互体验保障】：消除输入触发的画面跳动，保持光标与滚动位置平稳。
- 遵守【单向依赖流动】：UI 视图仅下发过滤状态至 FilterStateModel，模型自动分发通知代理模型，禁止冗余全树重构。

---

## 1. Overview（概述与解决的问题）

### 1.1 背景与解决的问题
1. **滚动条跳动问题**：用户在色码输入框输入内容并回车后，代码无故调用 `requestRebuild()` 触发了 `FilterPanel::rebuildGroups()`，导致侧边栏全部组件被销毁并重新构造，输入框失去焦点，滚动条因焦点转移自动滚到下方，色轮与输入框脱离可视范围；
2. **暗色筛选混入亮色问题**：在输入极暗红（如 `#350D0D`，V=53）时，由于算法明度差容差过宽（150~60），导致明度在 150 左右的中大亮红依然被放行，且色相容差固定为 20° 较为宽松。

### 1.2 解决措施
1. **消除跳动**：在 `AutoColorGroup.cpp` 中移除色码输入框对 `requestRebuild()` 的调用，纯粹通过 `filterModel->setState(st)` 触发数据层更新；同时在 `FilterPanel::rebuildGroups()` 中增加滚动条位置现场保护与恢复机制；
2. **算法严格收敛**：在 `FilterProxyModel.cpp` 中将明度容差大幅收窄至 `40 ~ 15`（默认 50% 精准度时仅允许 27 的明度差），并将色相容差收紧至 `15° ~ 6°`，彻底阻断大亮红（V=255）与深暗褐色（V=53）互相串色。

---

## 2. Modified Files List（影响文件清单）

- `src/ui/AutoColorGroup.cpp`
- `src/ui/FilterPanel.cpp`
- `src/ui/models/FilterProxyModel.cpp`

---

## 3. Detailed Line-by-Line Changes（包含 CMakeLists.txt 在内的精准替换块）

### 修改文件 1：`src/ui/AutoColorGroup.cpp`

#### 修改点 1.1：移除色码输入回车与清空时的全局重建调用

<<<<<<< SEARCH
    auto applyHexFilter = [filterModel, editColorHex, requestRebuild]() {
        QString raw = editColorHex->text().trimmed();
        if (raw.isEmpty()) {
            FilterState st = filterModel->state();
            if (!st.colors.isEmpty()) {
                st.colors.clear();
                filterModel->setState(st);
                if (requestRebuild) requestRebuild();
            }
            return;
        }

        QString hex = raw;
        if (!hex.startsWith('#') && (hex.length() == 6 || hex.length() == 3)) {
            hex.prepend('#');
        }

        QColor col(hex);
        if (col.isValid()) {
            QString formattedHex = col.name().toUpper();
            pushRecent(formattedHex);

            FilterState st = filterModel->state();
            st.colors = QStringList{ formattedHex };
            int h = col.hsvHue();
            if (h >= 0) {
                st.colorHue = h;
                AppConfig::instance().setValue("Filter/ColorHue", h);
            }
            filterModel->setState(st);
            if (requestRebuild) requestRebuild();
        }
    };

    QObject::connect(editColorHex, &QLineEdit::returnPressed, container, applyHexFilter);
    QObject::connect(editColorHex, &QLineEdit::textChanged, container, [filterModel, requestRebuild](const QString& text) {
        if (text.trimmed().isEmpty()) {
            FilterState st = filterModel->state();
            if (!st.colors.isEmpty()) {
                st.colors.clear();
                filterModel->setState(st);
                if (requestRebuild) requestRebuild();
            }
        }
    });
=======
    auto applyHexFilter = [filterModel, editColorHex]() {
        QString raw = editColorHex->text().trimmed();
        if (raw.isEmpty()) {
            FilterState st = filterModel->state();
            if (!st.colors.isEmpty()) {
                st.colors.clear();
                filterModel->setState(st);
            }
            return;
        }

        QString hex = raw;
        if (!hex.startsWith('#') && (hex.length() == 6 || hex.length() == 3)) {
            hex.prepend('#');
        }

        QColor col(hex);
        if (col.isValid()) {
            QString formattedHex = col.name().toUpper();
            pushRecent(formattedHex);

            FilterState st = filterModel->state();
            st.colors = QStringList{ formattedHex };
            int h = col.hsvHue();
            if (h >= 0) {
                st.colorHue = h;
                AppConfig::instance().setValue("Filter/ColorHue", h);
            }
            filterModel->setState(st);
        }
    };

    QObject::connect(editColorHex, &QLineEdit::returnPressed, container, applyHexFilter);
    QObject::connect(editColorHex, &QLineEdit::textChanged, container, [filterModel](const QString& text) {
        if (text.trimmed().isEmpty()) {
            FilterState st = filterModel->state();
            if (!st.colors.isEmpty()) {
                st.colors.clear();
                filterModel->setState(st);
            }
        }
    });
>>>>>>> REPLACE

---

### 修改文件 2：`src/ui/FilterPanel.cpp`

#### 修改点 2.1：在 `rebuildGroups()` 中增加滚动条位置保存与原位恢复

<<<<<<< SEARCH
void FilterPanel::rebuildGroups() {
    updateHeaderStatus();
    m_groupHeaders.clear();

    m_editType = nullptr;
=======
void FilterPanel::rebuildGroups() {
    int savedScrollY = (m_scrollArea && m_scrollArea->verticalScrollBar()) 
                           ? m_scrollArea->verticalScrollBar()->value() : 0;

    updateHeaderStatus();
    m_groupHeaders.clear();

    m_editType = nullptr;
>>>>>>> REPLACE

<<<<<<< SEARCH
        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
}
=======
        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }

    if (m_scrollArea && m_scrollArea->verticalScrollBar()) {
        m_scrollArea->verticalScrollBar()->setValue(savedScrollY);
    }
}
>>>>>>> REPLACE

---

### 修改文件 3：`src/ui/models/FilterProxyModel.cpp`

#### 修改点 3.1：大幅收敛明度差与色相差容差，杜绝亮暗互混

<<<<<<< SEARCH
                int d = std::abs(pcHue - targetHue);
                if (d > 180) d = 360 - d;
                if (d > FilterState::kHueTolerance) return false;

                double minSat = 255.0 * (1.0 - static_cast<double>(currentFilter.colorPrecision) / 100.0);
                if (static_cast<double>(pcSat) < minSat) return false;

                // 明度约束：避免亮色误匹配极暗色（如 #FF0400 误匹配极暗红 #350D0D）
                // 允许的最大明度差随精准度动态收敛：精准度 0~100 对应明度差容差 150~60
                double precisionFactor = static_cast<double>(currentFilter.colorPrecision) / 100.0;
                int maxValDiff = qRound(150.0 - 90.0 * precisionFactor);
                return std::abs(pc.value() - targetCol.value()) <= maxValDiff;
=======
                int d = std::abs(pcHue - targetHue);
                if (d > 180) d = 360 - d;

                double precisionFactor = static_cast<double>(currentFilter.colorPrecision) / 100.0;

                // 1. 动态收紧色相差容差：精准度 0~100 对应色相差 15° ~ 6° (50时为 10°)
                int maxHueDiff = qRound(15.0 - 9.0 * precisionFactor);
                if (d > maxHueDiff) return false;

                // 2. 纯度要求：必须符合精准度最低饱和度阈值
                double minSat = 255.0 * (1.0 - precisionFactor);
                if (static_cast<double>(pcSat) < minSat) return false;

                // 3. 严格明度约束：精准度 0~100 对应最大明度差 40 ~ 15 (50时仅为 27)
                // 彻底阻断大亮红（V=255）与深暗褐色（V=53）之间的混淆匹配
                int maxValDiff = qRound(40.0 - 25.0 * precisionFactor);
                return std::abs(pc.value() - targetCol.value()) <= maxValDiff;
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 4.1 编译验证
在项目根目录下构建：
```powershell
cmake --build "G:\C++\QuarkMeta\QuarkMeta_Build\x64-release"
```
验证三处文件编译无报错。

### 4.2 运行验证
1. **滚动条防跳动验证**：
   - 在顶部的色码输入框输入 `#350D0D` 并按回车；
   - 验证：输入框焦点不丢失，滚动条绝对不向下滚动跳动，色码框与色轮稳居顶部原位。
2. **暗色精准过滤验证**：
   - 验证列表结果：所有鲜艳亮红色（如截图中的亮红花朵、红礼服、Sale 70% 亮红标）被 100% 拦截剔除；
   - 仅保留真正属于深暗褐色、极暗红棕色的素材，搜索结果纯正精准。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）

- 移除冗余的 `requestRebuild()`，让过滤变动直接走唯一的 `FilterStateModel -> FilterProxyModel` SSOT 通道；
- 复用 `verticalScrollBar()` 既有接口做位置保护，无另起炉灶。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用成员 / API | 所在头文件 | 物理精确签名 | 核查状态 |
|---|---|---|---|
| `verticalScrollBar` | `<QAbstractScrollArea>` | `QScrollBar *verticalScrollBar() const;` | 100% 吻合 |
| `value` | `<QScrollBar>` | `int value() const;` | 100% 吻合 |
| `setValue` | `<QScrollBar>` | `void setValue(int);` | 100% 吻合 |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

- `FilterPanel.cpp` 原有已包含 `<QScrollArea>` 与 `<QScrollBar>`，头文件包含链完备闭合；
- 本方案未新增、删除 `#include`，不存在类型缺失。

---

## 8. Compliance Self-Check Table（合规核对表）

根据 AGENTS.md 3.6 第 2 关规范，交付前自检结果如下：

| 检查项 | 来源条款 | 自检结果 | 对应方案位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | `FilterPanel-6.md` 映射 `FilterPanel` |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/FilterPanel-6.md` |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 | 第 3 节 SEARCH 块与真实代码 100% 吻合 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册，且有配套 .cpp | 3.3 | 不涉及 | 未新建类 |
| 第 1~9 章齐全 | 3.4 | 满足 | 方案包含 0~9 所有必备章节 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 见第 0.2 节与第 6 节核查表 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 不涉及 | 未变动任何 `#include` 行，见第 7 节 |
| 已复用既有 SSOT 入口，无另起炉灶；旧分散实现已列入删除 | 2.4 | 满足 | 见第 5 节 |
| 无内联 `setStyleSheet`，样式走 style.qss | 4.1 | 不涉及 | 纯逻辑与算法调整 |
| 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 采用标准 QScrollBar 位置记忆机制 |
| 批量操作为一次聚合广播 | 4.3 | 不涉及 | 单次状态更新 |
| 迁移的所有数值原样照抄 | 4.4 | 不涉及 | 本次为容差阈值精准收紧，见第 9 节 |
| 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories.md | 满足 | 解决界面跳动并提升搜索精准度 |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅涉及本次关联的 3 个文件 |

---

## 9. Zero-Value-Alteration Check（数值零篡改核对）

- 本方案不涉及任何 UI 颜色、边框或圆角数值迁移；
- 色彩过滤算法容差按共识精确收紧（明度差由 `150~60` 收紧至 `40~15`，色相差由 `20°` 收紧至 `15°~6°`）。
