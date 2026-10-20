# 实施方案：MetaPanel 元数据面板冗余清理与上下文时序冲突修复

## 0. 已读证据清单 (Pre-Draft Evidence / Gate 1)

### 1. 已读文件清单
- `src/ui/MetaPanel.h` (已读取)
- `src/ui/MetaPanel.cpp` (已读取)
- `src/ui/PanelMediator.h` (已读取)
- `src/ui/PanelMediator.cpp` (已读取)
- `CMakeLists.txt` (已读取)
- `src/ui/MetaRatingColorWidget.h` / `src/ui/MetaRatingColorWidget.cpp` (已读取核实：无外部引用)
- `src/ui/MetaInfoSection.h` / `src/ui/MetaInfoSection.cpp` (已读取核实：无外部引用)
- `src/ui/MetaTagSection.h` / `src/ui/MetaTagSection.cpp` (已读取核实：无外部引用)
- `src/ui/MetaPreviewWidget.h` / `src/ui/MetaPreviewWidget.cpp` (已读取核实：无外部引用)

### 2. 外部成员函数签名原文摘录
- `void MetaPanel::setSelectedPaths(const QStringList& paths);` (`src/ui/MetaPanel.h:55`)
- `void QWidget::close();` (Qt 基类 API，用于关闭浮层 `m_tagSelectorOverlay`)
- `void QWidget::clearFocus();` (Qt 基类 API，用于在切换选区前使编辑框离焦提交)

### 3. 既有 API 检索记录
- 检索命令：`grep -rn "MetaRatingColorWidget" src/` ➔ 结果仅在该类自身 `.h` / `.cpp` 中出现，全工程 0 处外部实例化。
- 检索命令：`grep -rn "MetaInfoSection" src/` ➔ 结果仅在自身出现，全工程 0 处外部实例化。
- 检索命令：`grep -rn "MetaTagSection" src/` ➔ 结果仅在自身出现，全工程 0 处外部实例化。
- 检索命令：`grep -rn "MetaPreviewWidget" src/` ➔ 结果仅在自身出现，全工程 0 处外部实例化。

### 4. Memories.md / AGENTS.md 规范条款关联
- **`Memories.md` 1.2 项**：“禁止内联硬编码：在后续开发与重构中，严格禁止在控件 C++ 代码中随意调用 `setStyleSheet(...)` 采用内联方式硬编码样式。”
- **`AGENTS.md` 2.4 项**：“物理彻底清除死代码契约：当发现另起炉灶的分散旧实现时，必须在重构方案中彻底将其物理删除。”

---

## 1. Overview（概述与解决的问题）

本实施方案旨在解决对 `MetaPanel`（右侧元数据属性面板）审计中发现的三大核心问题：
1. **物理清除孤儿僵尸死代码**：清理 `src/ui/` 下 4 组完全未被使用的死代码类（`MetaRatingColorWidget`、`MetaInfoSection`、`MetaTagSection`、`MetaPreviewWidget`）并从 `CMakeLists.txt` 中注销，消除冗余编译与隐患。
2. **消除标签浮层（TagSelectorOverlay）选区漂移冲突**：当用户切换选区（调用 `setSelectedPaths`）时，若 `TagSelectorOverlay` 处于打开状态，强制将其关闭，杜绝“在新选区视图下勾选标签却修改了旧文件”的上下文错位问题。
3. **消除编辑框离焦（FocusOut）与选区切换的时序竞态冲突**：在 `setSelectedPaths` 中，切换新选区前主动收回当前焦点（`clearFocus`），驱动编辑框在选区快照更新前同步完成落盘，消除数据交织冲突。
4. **清理 HTML 内联硬编码颜色**：将 `setHtml` 中硬编码的 CSS 颜色 `color:#3498db;` 替换为样式表友好的标准文本格式，无缝遵循 `Memories.md` 零内联硬编码红线。

---

## 2. Modified Files List（影响文件清单）

1. `CMakeLists.txt`
2. `src/ui/MetaPanel.cpp`
3. （删除文件）：
   - `src/ui/MetaRatingColorWidget.h`
   - `src/ui/MetaRatingColorWidget.cpp`
   - `src/ui/MetaInfoSection.h`
   - `src/ui/MetaInfoSection.cpp`
   - `src/ui/MetaTagSection.h`
   - `src/ui/MetaTagSection.cpp`
   - `src/ui/MetaPreviewWidget.h`
   - `src/ui/MetaPreviewWidget.cpp`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `CMakeLists.txt` 删除僵尸文件注册

<<<<<<< SEARCH
    src/ui/MetaPreviewWidget.h
    src/ui/MetaPreviewWidget.cpp
    src/ui/MetaRatingColorWidget.h
    src/ui/MetaRatingColorWidget.cpp
    src/ui/MetaTagSection.h
    src/ui/MetaTagSection.cpp
    src/ui/MetaInfoSection.h
    src/ui/MetaInfoSection.cpp
    src/ui/MetaPanel.cpp
    src/ui/MetaPanel.h
=======
    src/ui/MetaPanel.cpp
    src/ui/MetaPanel.h
>>>>>>> REPLACE

---

### 3.2 `src/ui/MetaPanel.cpp` 选区切换保护与离焦竞态消除

<<<<<<< SEARCH
void MetaPanel::setSelectedPaths(const QStringList& paths) {
    QElapsedTimer timer;
    timer.start();
    m_selectedPaths = paths;
    m_editingPathsSnapshot = paths;
    bool hasSelection = !m_selectedPaths.isEmpty();
    bool isMulti = m_selectedPaths.size() > 1;
=======
void MetaPanel::setSelectedPaths(const QStringList& paths) {
    // 1. 选区切换防漂移：若标签选择浮层处于打开状态，选区变更时强制关闭，消除上下文错位
    if (m_tagSelectorOverlay) {
        m_tagSelectorOverlay->close();
    }

    // 2. 离焦竞态保护：选区切换前驱使当前焦点编辑框收回焦点，确保旧文件的编辑在路径快照更新前落盘
    if (m_nameEdit && m_nameEdit->hasFocus()) {
        m_nameEdit->clearFocus();
    }
    if (m_noteEdit && m_noteEdit->hasFocus()) {
        m_noteEdit->clearFocus();
    }
    if (m_linkEdit && m_linkEdit->hasFocus()) {
        m_linkEdit->clearFocus();
    }

    QElapsedTimer timer;
    timer.start();
    m_selectedPaths = paths;
    m_editingPathsSnapshot = paths;
    bool hasSelection = !m_selectedPaths.isEmpty();
    bool isMulti = m_selectedPaths.size() > 1;
>>>>>>> REPLACE

---

### 3.3 `src/ui/MetaPanel.cpp` 消除内联硬编码 CSS 颜色

<<<<<<< SEARCH
        if (m_nameEdit) {
            m_nameEdit->setHtml(QString("已选中 <span style=\"color:#3498db;\">%1</span> 个项目").arg(m_selectedPaths.size()));
        }
=======
        if (m_nameEdit) {
            m_nameEdit->setPlainText(QString("已选中 %1 个项目").arg(m_selectedPaths.size()));
        }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 1. 删除物理僵尸文件
运行 bash 命令删除 8 个僵尸文件：
```bash
rm src/ui/MetaRatingColorWidget.h src/ui/MetaRatingColorWidget.cpp \
   src/ui/MetaInfoSection.h src/ui/MetaInfoSection.cpp \
   src/ui/MetaTagSection.h src/ui/MetaTagSection.cpp \
   src/ui/MetaPreviewWidget.h src/ui/MetaPreviewWidget.cpp
```

### 2. 编译工程
在 sandbox 环境运行 CMake 构建命令：
```bash
cmake -B build -S .
cmake --build build --config Release
```
确认构建成功且 0 编译错误。

### 3. 功能交互验证
1. **多选状态验证**：多选文件时，`m_nameEdit` 呈现纯净的“已选中 X 个项目”占位，无内联颜色样式缺陷；
2. **浮层防漂移验证**：打开标签浮层 `TagSelectorOverlay` 后点击列表中的其他文件，浮层自动关闭；
3. **编辑离焦落盘验证**：在备注框输入文字但不按回车，直接点击列表中的另一个文件，编辑内容正确落盘至前一个文件，且无时序竞态报错。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

- **复用既有离焦提交通道**：通过 `clearFocus()` 触发 `eventFilter` 内既有的 `FocusOut` 逻辑，复用已验证的 `noteEdited` / `linkEdited` 信号链路，未新增重复提交代码。
- **清除分散僵尸类**：彻底物理删除了 `MetaRatingColorWidget`、`MetaInfoSection`、`MetaTagSection`、`MetaPreviewWidget` 等 4 组分散孤儿类。

---

## 6. Header API Signature Verification

- `void MetaPanel::setSelectedPaths(const QStringList& paths);` (`src/ui/MetaPanel.h:55`) ➔ 签名完全一致。
- `void QWidget::close();` (Qt 基类 API) ➔ 物理签名符合。
- `void QWidget::clearFocus();` (Qt 基类 API) ➔ 物理签名符合。

---

## 7. Header Inclusion Chain & Type Completeness Check

- 本次修改未删除或新增任何 `#include` 行，仅删除了 `CMakeLists.txt` 中的死代码项，包含链完整性受控。

---

## 8. Compliance Self-Check Table（合规核对表 / Gate 2）

| 序号 | 检查项 | 来源条款 | 结果 (满足/不满足/不涉及) | 方案对应位置 |
|---|---|---|---|---|
| 1 | 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | 文件名 `MetaPanel-11.md`（遵循 3.1-3 递增后缀规则） |
| 2 | 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/` |
| 3 | 所有 SEARCH 块代码在仓库真实存在 | 3.2 | 满足 | 章节 3.1, 3.2, 3.3 |
| 4 | 涉及 Q_OBJECT 的类已在 CMakeLists 注册 | 3.3 | 满足 | `MetaPanel` 在 CMakeLists 保持注册 |
| 5 | 第 1~9 章齐全 | 3.4 | 满足 | 本文档包含全部 9 章与第 0 章 |
| 6 | 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 章节 6 |
| 7 | 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 不涉及 | 未修改 `#include` |
| 8 | 已复用既有 SSOT 入口，无另起炉灶；旧分散实现已删除 | 2.4 | 满足 | 章节 3.1, 5 |
| 9 | 无内联 setStyleSheet，样式走 style.qss | 4.1 | 满足 | 章节 3.3 清除 setHtml 内联硬编码 |
| 10 | 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 未引入 Hack |
| 11 | 批量操作为一次聚合广播 | 4.3 | 不涉及 | 未修改批量广播逻辑 |
| 12 | 迁移的所有数值原样照抄 | 4.4 | 满足 | 章节 9 |
| 13 | 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories | 满足 | 保留全套既有 UI 布局与交互 |
| 14 | 未触及方案范围外的文件 | 5.3 | 满足 | 仅涉及 `CMakeLists.txt` 与 `MetaPanel.cpp` |

---

## 9. Zero-Value-Alteration Check（数值零篡改核对）

- **样式清理项**：
  * 原位置：`src/ui/MetaPanel.cpp` 中 `m_nameEdit->setHtml(QString("已选中 <span style=\"color:#3498db;\">%1</span> 个项目").arg(m_selectedPaths.size()));`
  * 修改后：`m_nameEdit->setPlainText(QString("已选中 %1 个项目").arg(m_selectedPaths.size()));`
  * 说明：移除了硬编码的 CSS `#3498db` 样式，改为继承 QSS 全局暗色字体。无其他 Hex/RGB 颜色、px 边距、圆角或尺寸参数被篡改。
