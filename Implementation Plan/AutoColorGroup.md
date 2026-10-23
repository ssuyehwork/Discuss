# 实施方案：AutoColorGroup 色码输入框集成与颜色标记组前移 (AutoColorGroup.md)

## 0. 起草前核查（Pre-Draft Evidence）

根据 AGENTS.md 3.6 第 1 关规范，实施本方案前物理核查证据如下：

### 0.1 已读文件清单
- `src/ui/AutoColorGroup.h`：已物理阅读，确认公共接口 `populate` 与 `pushRecent`，签名保持冻结只读。
- `src/ui/AutoColorGroup.cpp`：已物理阅读（行 1~220），定位到头文件包含与 `populate` 容器顶部布局。
- `src/ui/FilterPanel.cpp`：已物理阅读（行 470~520），定位到 `rebuildGroups()` 中各组的物理插入顺序。

### 0.2 签名原文摘录
本方案在 `AutoColorGroup.cpp` 中调用的外部 Qt 成员函数声明如下：
- `void QLineEdit::setClearButtonEnabled(bool enable);`
- `void QLineEdit::setPlaceholderText(const QString &);`
- `void QLineEdit::setText(const QString &);`
- `QString QLineEdit::text() const;`
- `bool QColor::isValid() const noexcept;`
- `QString QColor::name(QColor::NameFormat format = HexRgb) const;`
- `int QColor::hsvHue() const noexcept;`

### 0.3 既有 API 检索记录
- 检索关键词：`FilterSearchEdit`
- 检索结果：全工程搜索输入框均统一采用 `#FilterSearchEdit` 作为 `objectName`，高度统一定为 `22px`，外边距为 `(5, 6, 5, 4)`，本方案 100% 沿用该标准规范。

### 0.4 Memories.md 相关条款
- 遵守【UI 标准与规范】：所有搜索输入控件复用全局 QSS 框架（`#FilterSearchEdit`），严禁 C++ 内联样式硬编码。
- 遵守【契约锁】：外部公开的 `.h` 接口签名一律只读冻结。

---

## 1. Overview（概述与解决的问题）

### 1.1 背景与解决的问题
1. **增加色码输入框**：色轮上方缺少精确输入色码的文本框，用户在需要精准按 Hex 筛选时只能靠肉眼在色轮上寻找；同时需支持带 `#`（如 `#350D0D`）与不带 `#`（如 `350D0D`）的输入容错；
2. **组位置前移**：当前“颜色标记”排在第 3 位（位于标签与评级之后），用户期望将其上移至首位（“标签”组上方），优先呈现视觉色彩筛选。

### 1.2 解决措施
1. 在 `AutoColorGroup.cpp` 的 `populate` 函数顶部、色轮正上方，插入标准的 `FilterSearchEdit` 风格单行输入框，监听回车与清空事件，实现合法十六进制色码的双向联动与筛选；
2. 在 `FilterPanel.cpp` 的 `rebuildGroups` 中，将“颜色标记”组的构造与添加代码挪移至“标签”组的前面。

---

## 2. Modified Files List（影响文件清单）

- `src/ui/AutoColorGroup.cpp`
- `src/ui/FilterPanel.cpp`

---

## 3. Detailed Line-by-Line Changes（包含 CMakeLists.txt 在内的精准替换块）

### 修改文件 1：`src/ui/AutoColorGroup.cpp`

#### 修改点 1.1：补充 `#include <QLineEdit>`

<<<<<<< SEARCH
#include <QLabel>
#include <QPushButton>
=======
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
>>>>>>> REPLACE

#### 修改点 1.2：在色轮上方插入色码输入框并绑定回车/清空筛选

<<<<<<< SEARCH
    const FilterState currentState = filterModel->state();

    // 1. 色轮 ColorWheel 居中布局
    QWidget* wheelContainer = new QWidget(container);
=======
    const FilterState currentState = filterModel->state();

    // 0. 色码输入框行 (位于色轮上方)
    QWidget* editRow = new QWidget(container);
    QHBoxLayout* editLayout = new QHBoxLayout(editRow);
    editLayout->setContentsMargins(5, 6, 5, 4);
    editLayout->setSpacing(0);

    QLineEdit* editColorHex = new QLineEdit(editRow);
    editColorHex->setObjectName("FilterSearchEdit");
    editColorHex->setFixedHeight(22);
    editColorHex->setClearButtonEnabled(true);
    editColorHex->setPlaceholderText("例: #350D0D 或 350D0D");

    if (!currentState.colors.isEmpty()) {
        editColorHex->setText(currentState.colors.first());
    }

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

    editLayout->addWidget(editColorHex);
    groupLayout->addWidget(editRow);

    // 1. 色轮 ColorWheel 居中布局
    QWidget* wheelContainer = new QWidget(container);
>>>>>>> REPLACE

---

### 修改文件 2：`src/ui/FilterPanel.cpp`

#### 修改点 2.1：将“颜色标记”组挪移至“标签”组的前面

<<<<<<< SEARCH
    FilterState currentSt = m_filterModel->state();

    // ── 1. 标签 ──────────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("标签", gl);

        TagStatusGroup::populate(g, gl, m_filterModel, m_currentStats, currentSt,
            [this](QVBoxLayout* layout, const QString& label, int count, const QString& rowKey) {
                return addFilterRow(layout, label, count, QColor(), rowKey);
            });

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }

    // ── 2. 评级 ──────────────────────────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("评级", gl);

        RatingGroup::populate(g, gl, m_filterModel, m_ratingCounts, currentSt,
            [this](QVBoxLayout* layout, const QString& label, int count) {
                return addFilterRow(layout, label, count);
            });

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }

    // ── 3. 颜色标记 ────────────
    {
        QVBoxLayout* gl = nullptr;
        QHBoxLayout* hdrLayout = nullptr;
        QWidget* g = buildGroup("颜色标记", gl, &hdrLayout);

        AutoColorGroup::populate(g, gl, m_filterModel, [this]() {
            rebuildGroups();
        });

        ColorLabelGroup::populate(g, gl, m_filterModel, m_colorCounts, currentSt,
            [this](QVBoxLayout* layout, const QString& label, int count, const QColor& color, const QString& rowKey) {
                return addFilterRow(layout, label, count, color, rowKey);
            });

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
=======
    FilterState currentSt = m_filterModel->state();

    // ── 1. 颜色标记 (移至首位，在标签组上方) ────────────
    {
        QVBoxLayout* gl = nullptr;
        QHBoxLayout* hdrLayout = nullptr;
        QWidget* g = buildGroup("颜色标记", gl, &hdrLayout);

        AutoColorGroup::populate(g, gl, m_filterModel, [this]() {
            rebuildGroups();
        });

        ColorLabelGroup::populate(g, gl, m_filterModel, m_colorCounts, currentSt,
            [this](QVBoxLayout* layout, const QString& label, int count, const QColor& color, const QString& rowKey) {
                return addFilterRow(layout, label, count, color, rowKey);
            });

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }

    // ── 2. 标签 ──────────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("标签", gl);

        TagStatusGroup::populate(g, gl, m_filterModel, m_currentStats, currentSt,
            [this](QVBoxLayout* layout, const QString& label, int count, const QString& rowKey) {
                return addFilterRow(layout, label, count, QColor(), rowKey);
            });

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }

    // ── 3. 评级 ──────────────────────────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("评级", gl);

        RatingGroup::populate(g, gl, m_filterModel, m_ratingCounts, currentSt,
            [this](QVBoxLayout* layout, const QString& label, int count) {
                return addFilterRow(layout, label, count);
            });

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 4.1 编译验证
在项目根目录下编译：
```powershell
cmake --build "G:\C++\QuarkMeta\QuarkMeta_Build\x64-release"
```
验证 `AutoColorGroup.cpp.obj` 与 `FilterPanel.cpp.obj` 无任何语法报错。

### 4.2 运行验证
1. 打开筛选侧边栏：
   - 验证“颜色标记”组是否已成功排列在最顶部（“标签”组上方）；
   - 验证色盘正上方是否呈现统一高度 22px 的单行输入框；
2. 输入色码测试：
   - 输入 `350D0D`（不带 `#`），按回车：验证列表正确筛选该色码素材，色盘同步旋转联动；
   - 输入 `#FF0400`（带 `#`），按回车：验证正常按鲜亮大红筛选；
   - 点击输入框右侧清除按钮：验证颜色过滤被重置恢复。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）

- 输入框样式 100% 复用全局 QSS `#FilterSearchEdit`；
- 色码过滤状态 100% 流向既有的 `FilterStateModel` 与 `FilterProxyModel`，无另外起炉灶。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用成员 / API | 所在头文件 | 物理精确签名 | 核查状态 |
|---|---|---|---|
| `setClearButtonEnabled` | `<QLineEdit>` | `void QLineEdit::setClearButtonEnabled(bool enable);` | 100% 吻合 |
| `setPlaceholderText` | `<QLineEdit>` | `void QLineEdit::setPlaceholderText(const QString &);` | 100% 吻合 |
| `isValid` | `<QColor>` | `bool QColor::isValid() const noexcept;` | 100% 吻合 |
| `name` | `<QColor>` | `QString QColor::name(QColor::NameFormat format = HexRgb) const;` | 100% 吻合 |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

- 在 `AutoColorGroup.cpp` 中显式补充 `#include <QLineEdit>`，防止因缺失前置声明导致编译失败；
- 其他类型均在原包含链中闭合。

---

## 8. Compliance Self-Check Table（合规核对表）

根据 AGENTS.md 3.6 第 2 关规范，交付前自检结果如下：

| 检查项 | 来源条款 | 自检结果 | 对应方案位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | `AutoColorGroup.md` 映射 `AutoColorGroup` |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/AutoColorGroup.md` |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 | 第 3 节 SEARCH 块与实际代码 100% 吻合 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册，且有配套 .cpp | 3.3 | 不涉及 | 未新建 Q_OBJECT 类 |
| 第 1~9 章齐全 | 3.4 | 满足 | 方案包含 0~9 所有必备章节 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 见第 0.2 节与第 6 节核查表 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 满足 | 显式增加 `<QLineEdit>`，见第 7 节 |
| 已复用既有 SSOT 入口，无另起炉灶；旧分散实现已列入删除 | 2.4 | 满足 | 见第 5 节 |
| 无内联 `setStyleSheet`，样式走 style.qss | 4.1 | 满足 | 使用 `#FilterSearchEdit` 接入全局 QSS |
| 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 纯标准 Qt 控件布局与信号槽机制 |
| 批量操作为一次聚合广播 | 4.3 | 不涉及 | 单输入框单值事件 |
| 迁移的所有数值原样照抄 | 4.4 | 满足 | 输入框尺寸 22px 与内边距照抄既有标准 |
| 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories.md | 满足 | 严格按共识设计 |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅涉及 `AutoColorGroup.cpp` 与 `FilterPanel.cpp` |

---

## 9. Zero-Value-Alteration Check（数值零篡改核对）

- 输入框高度：`22 px`（与 `FileTypeGroup.cpp` 中 `#FilterSearchEdit` 100% 相同）；
- 输入框容器外边距：`(5, 6, 5, 4)`（与既有输入框标准 100% 相同）；
- 无任何主观私自添加的数值参数。
