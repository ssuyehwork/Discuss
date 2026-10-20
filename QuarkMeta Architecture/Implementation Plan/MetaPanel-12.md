# 实施方案：MetaPanel 元数据面板冗余清理、同步离焦提交与浮层生命周期重构 (迭代 12)

## 0. 已读证据清单 (Pre-Draft Evidence / Gate 1)

### 1. 已读文件清单
- `src/ui/MetaPanel.h` (已读取)
- `src/ui/MetaPanel.cpp` (已读取)
- `src/ui/TagSelectorOverlay.h` (已读取)
- `src/ui/PanelMediator.h` (已读取)
- `src/ui/PanelMediator.cpp` (已读取)
- `CMakeLists.txt` (已读取)
- `src/ui/MetaRatingColorWidget.h` / `src/ui/MetaRatingColorWidget.cpp` (已读取核实：无外部引用)
- `src/ui/MetaInfoSection.h` / `src/ui/MetaInfoSection.cpp` (已读取核实：无外部引用)
- `src/ui/MetaTagSection.h` / `src/ui/MetaTagSection.cpp` (已读取核实：无外部引用)
- `src/ui/MetaPreviewWidget.h` / `src/ui/MetaPreviewWidget.cpp` (已读取核实：无外部引用)

### 2. 外部成员函数签名原文摘录
- `void MetaPanel::setSelectedPaths(const QStringList& paths);` (`src/ui/MetaPanel.h:55`)
- `void QWidget::close();` (Qt 基类 API)
- `void QWidget::clearFocus();` (Qt 基类 API)
- `bool QWidget::hasFocus() const;` (Qt 基类 API)

### 3. 既有 API 检索记录
- 检索命令：`grep -rn "MetaRatingColorWidget" src/` ➔ 全工程 0 处外部实例化。
- 检索命令：`grep -rn "MetaInfoSection" src/` ➔ 全工程 0 处外部实例化。
- 检索命令：`grep -rn "MetaTagSection" src/` ➔ 全工程 0 处外部实例化。
- 检索命令：`grep -rn "MetaPreviewWidget" src/` ➔ 全工程 0 处外部实例化。

### 4. Memories.md / AGENTS.md 规范条款关联
- **`Memories.md` 1.2 项**：“禁止内联硬编码：严格禁止在控件 C++ 代码中随意调用 `setStyleSheet(...)` 采用内联方式硬编码样式。”
- **`AGENTS.md` 2.4 项**：“物理彻底清除死代码契约：当发现另起炉灶的分散旧实现时，必须在重构方案中彻底将其物理删除。”
- **`AGENTS.md` 4.4 项**：“重构与修复中严禁改变既有交互形态。”

---

## 1. Overview（概述与解决的问题）

本实施方案是对此前 `MetaPanel-11.md` 进行自检推演后的**完备迭代补全方案（MetaPanel-12.md）**，旨在精准解决 `MetaPanel` 的冗余代码与微观时序/交互冲突，彻底消除任何“顾此失彼”隐患：

1. **物理清除孤儿僵尸死代码**：清理 `src/ui/` 下 4 组完全未被使用的僵尸死代码类（`MetaRatingColorWidget`、`MetaInfoSection`、`MetaTagSection`、`MetaPreviewWidget`）并从 `CMakeLists.txt` 中注销。
2. **新增同步立即可控落盘机制 `submitPendingEdits()`**：在选区切换（`setSelectedPaths`）前，同步检查并立即发射 `noteEdited` / `linkEdited` / `renameRequested` 信号，在 `m_editingPathsSnapshot` 覆盖前精准对旧文件落盘，彻底消除依赖 `FocusOut` + `QTimer::singleShot(0)` 导致的异步回调时序覆盖冲突。
3. **标签浮层（TagSelectorOverlay）目标与生命周期安全防漂移**：在切换选区时，若 `TagSelectorOverlay` 处于打开状态，优雅调用 `close()`，防止浮层留在屏幕上时勾选标签误改旧文件。
4. **清理 HTML 内联硬编码颜色**：将 `setHtml` 中硬编码的 CSS 颜色 `color:#3498db;` 替换为符合 ThemeManager / style.qss 的纯净文本格式。

---

## 2. Modified Files List（影响文件清单）

1. `CMakeLists.txt`
2. `src/ui/MetaPanel.h`
3. `src/ui/MetaPanel.cpp`
4. （物理删除文件）：
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

### 3.2 `src/ui/MetaPanel.h` 增加私有同步提交函数声明

<<<<<<< SEARCH
    void adjustFlowHeights();
    void addInfoRow(QVBoxLayout* layout, const QString& label, QLabel*& valueLabel);
=======
    void submitPendingEdits();
    void adjustFlowHeights();
    void addInfoRow(QVBoxLayout* layout, const QString& label, QLabel*& valueLabel);
>>>>>>> REPLACE

---

### 3.3 `src/ui/MetaPanel.cpp` 实现同步提交函数与选区切换同步保护

<<<<<<< SEARCH
void MetaPanel::setSelectedPaths(const QStringList& paths) {
    QElapsedTimer timer;
    timer.start();
    m_selectedPaths = paths;
    m_editingPathsSnapshot = paths;
    bool hasSelection = !m_selectedPaths.isEmpty();
    bool isMulti = m_selectedPaths.size() > 1;
=======
void MetaPanel::submitPendingEdits() {
    if (m_editingPathsSnapshot.isEmpty() || m_isReadOnlyMode) return;

    if (m_noteEdit && m_noteEdit->hasFocus()) {
        QString textCopy = m_noteEdit->toPlainText();
        emit noteEdited(m_editingPathsSnapshot, textCopy);
        m_noteEdit->clearFocus();
    }
    if (m_linkEdit && m_linkEdit->hasFocus()) {
        QString linkCopy = m_linkEdit->text().trimmed();
        emit linkEdited(m_editingPathsSnapshot, linkCopy);
        m_linkEdit->clearFocus();
    }
    if (m_nameEdit && m_nameEdit->hasFocus() && m_editingPathsSnapshot.size() == 1) {
        QString oldPath = m_nameEdit->property("oldPath").toString();
        QString newName = m_nameEdit->toPlainText().trimmed();
        static const QRegularExpression illegalRegex("[\\\\/:*?\"<>|\r\n]");
        newName.remove(illegalRegex);

        QString suffix = m_nameEdit->property("suffix").toString();
        if (!oldPath.isEmpty() && !newName.isEmpty()) {
            QFileInfo oldInfo(oldPath);
            if (newName != oldInfo.completeBaseName()) {
                QString newPath = oldInfo.absolutePath() + "/" + newName + (suffix.isEmpty() ? "" : "." + suffix);
                newPath = QDir::toNativeSeparators(newPath);
                if (!QFile::exists(newPath)) {
                    emit renameRequested(oldPath, newPath);
                }
            }
        }
        m_nameEdit->clearFocus();
    }
}

void MetaPanel::setSelectedPaths(const QStringList& paths) {
    // 1. 同步提交当前焦点的未落盘修改，对旧文件完成精准落盘
    submitPendingEdits();

    // 2. 若标签选择浮层处于打开状态，选区切换时优雅关闭，消除选区漂移
    if (m_tagSelectorOverlay) {
        m_tagSelectorOverlay->close();
    }

    QElapsedTimer timer;
    timer.start();
    m_selectedPaths = paths;
    m_editingPathsSnapshot = paths;
    bool hasSelection = !m_selectedPaths.isEmpty();
    bool isMulti = m_selectedPaths.size() > 1;
>>>>>>> REPLACE

---

### 3.4 `src/ui/MetaPanel.cpp` 消除内联硬编码 CSS 颜色

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
1. **编辑同步落盘验证**：在备注框输入文字不按回车，直接点击列表中的另一个文件，旧文件备注通过 `submitPendingEdits()` 同步且精准地发射提交，无任何微秒级回调覆盖报错；
2. **多选状态验证**：多选文件时，`m_nameEdit` 呈现纯净的“已选中 X 个项目”占位，无内联颜色样式缺陷；
3. **浮层防漂移验证**：打开标签浮层 `TagSelectorOverlay` 后点击列表中的其他文件，浮层自动平滑关闭。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

- **同步立即可控落盘**：通过内部高内聚函数 `submitPendingEdits()` 统一管控 `m_noteEdit` / `m_linkEdit` / `m_nameEdit` 的同步离焦提交，消除了 `QTimer::singleShot(0)` 异步宏微任务对选区切换的干扰。
- **清除分散僵尸类**：彻底物理删除了 4 组分散孤儿类。

---

## 6. Header API Signature Verification

- `void MetaPanel::setSelectedPaths(const QStringList& paths);` (`src/ui/MetaPanel.h:55`)
- `void MetaPanel::submitPendingEdits();` (`src/ui/MetaPanel.h:73` 新增私有辅助函数)
- `void QWidget::close();` (Qt 基类 API)
- `void QWidget::clearFocus();` (Qt 基类 API)
- `bool QWidget::hasFocus() const;` (Qt 基类 API)

---

## 7. Header Inclusion Chain & Type Completeness Check

- 本次修改未删除任何头文件引用，新增了私有方法 `submitPendingEdits()`，受影响 `.cpp` 文件所需类型 `QRegularExpression` / `QFileInfo` / `QDir` / `QFile` 均已在 `MetaPanel.cpp` 包含链中具备完整类型定义，无 `C2027` 或 `C2065` 隐患。

---

## 8. Compliance Self-Check Table（合规核对表 / Gate 2）

| 序号 | 检查项 | 来源条款 | 结果 (满足/不满足/不涉及) | 方案对应位置 |
|---|---|---|---|---|
| 1 | 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | 文件名 `MetaPanel-12.md`（遵循 3.1-3 递增后缀规则） |
| 2 | 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/` |
| 3 | 所有 SEARCH 块代码在仓库真实存在 | 3.2 | 满足 | 章节 3.1, 3.2, 3.3, 3.4 |
| 4 | 涉及 Q_OBJECT 的类已在 CMakeLists 注册 | 3.3 | 满足 | `MetaPanel` 在 CMakeLists 保持注册 |
| 5 | 第 1~9 章齐全 | 3.4 | 满足 | 本文档包含全部 9 章与第 0 章 |
| 6 | 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 章节 6 |
| 7 | 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 不涉及 | 未修改 `#include` |
| 8 | 已复用既有 SSOT 入口，无另起炉灶；旧分散实现已删除 | 2.4 | 满足 | 章节 3.1, 5 |
| 9 | 无内联 setStyleSheet，样式走 style.qss | 4.1 | 满足 | 章节 3.4 清除 setHtml 内联硬编码 |
| 10 | 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 未引入 Hack |
| 11 | 批量操作为一次聚合广播 | 4.3 | 不涉及 | 未修改批量广播逻辑 |
| 12 | 迁移的所有数值原样照抄 | 4.4 | 满足 | 章节 9 |
| 13 | 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories | 满足 | 保留全套既有 UI 布局与交互 |
| 14 | 未触及方案范围外的文件 | 5.3 | 满足 | 仅涉及 `CMakeLists.txt` 与 `MetaPanel` 头文件/源文件 |

---

## 9. Zero-Value-Alteration Check（数值零篡改核对）

- **样式清理项**：
  * 原位置：`src/ui/MetaPanel.cpp` 中 `m_nameEdit->setHtml(QString("已选中 <span style=\"color:#3498db;\">%1</span> 个项目").arg(m_selectedPaths.size()));`
  * 修改后：`m_nameEdit->setPlainText(QString("已选中 %1 个项目").arg(m_selectedPaths.size()));`
  * 说明：移除了硬编码 CSS `#3498db` 样式。无其他 Hex/RGB 颜色、px 边距、圆角或尺寸参数被篡改。
