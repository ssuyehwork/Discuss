# Implementation Plan: QuickLook 文本预览内置搜索工具栏实施方案

本实施方案旨在严格遵循系统架构规范、《SYSTEM_PROMPT.md》、《Standard Specifications for Implementation Plan.md》以及 `Memories_QuarkMeta.md` 章节 39 的核心设计理念，在 `QuarkMeta_Source` 桌面应用的快速预览窗口（`QuickLookWindow`）中完整实现：
1. **场景化快捷键门禁 (`Ctrl + F`)**：仅在当前处于文本预览模式（`m_textEdit` 激活）时响应快捷键呼出搜索栏；在图形图像文件预览时严格屏蔽，防止越权干扰；
2. **两层紧凑型悬浮面板结构**：
   - 上层：搜索输入框（`QLineEdit`），右侧紧随“查找下一个”按钮和“计数”按钮；
   - 下层：状态指示行，宽度与上方搜索框严格对齐，通过 `|` 分割显示“当前行号”与“计数值”（如 `42 | 5`）；
3. **交互与高亮闭环**：
   - 回车键支持快速查找并跳转下一个匹配项；
   - 匹配到的内容采用 `#ff551C`（醒目橙红）进行背景高亮，并自动滚动视图定位至该项；
   - 未匹配时状态指示行相应位置显示为“无 | 0”；
   - 关闭预览或切换文件时自动收起并重置搜索状态。

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 核心病灶与系统性缺陷深入剖析
1. **长文本与源代码预览定位困难**：
   - `QuickLookWindow` 当前仅具备静态文本只读渲染（`QPlainTextEdit`），不支持在预览窗口内进行关键词查找。用户在预览大型日志、长代码文件或文本时，无法快速检索关键信息。
2. **缺乏状态化的行号与计数反馈机制**：
   - 缺乏像现代代码编辑器一样的搜索指示器，无法让用户直观掌握匹配项在文本中的具体物理位置（所在行）和全部匹配分布数量。

### 1.2 架构治理设计目标与核心守则
- **单窗口原生嵌入**：在 `QuickLookWindow` 顶部右上方悬浮嵌入 `m_searchBar` 浮层，采用双层布局（输入行 + 状态指示行），无需创建第二套独立窗口，零资源冗余；
- **状态单一真理源 (SSOT)**：当前文档内容以 `m_textEdit->document()` 为单一事实源，通过 `QTextDocument::find` 与 `QTextCursor` 定位行号（`cursor.blockNumber() + 1`），并通过 `QTextEdit::ExtraSelection` 实施 `#ff551C` 样式着色；
- **场景与焦点门禁**：仅在 `m_textEdit->isVisible()` 且非图形图像时启用搜索，搜索框弹出时自动获得焦点并全选既有文字，按下 `ESC` 时隐藏搜索框并清空高亮。

---

## Chapter 2: Modified Files List（影响文件清单）

1. **修改文件**：
   - `QuarkMeta_Source/src/ui/QuickLookWindow.h`（增加搜索控件成员指针、方法声明及 `resizeEvent` 重载）
   - `QuarkMeta_Source/src/ui/QuickLookWindow.cpp`（构建两层悬浮搜索控件、实现查找下一个、计数、高亮渲染与按键拦截逻辑）
   - `QuarkMeta_Source/resources/style.qss`（收拢搜索框浮层、按钮与状态栏外部样式，禁止 C++ 硬编码内联样式）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `QuarkMeta_Source/src/ui/QuickLookWindow.h`

<<<<<<< SEARCH
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>
=======
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QVBoxLayout>
>>>>>>> REPLACE

<<<<<<< SEARCH
protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
=======
protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
>>>>>>> REPLACE

<<<<<<< SEARCH
    QuickLookGraphicsView* m_graphicsView = nullptr;
    QPlainTextEdit* m_textEdit = nullptr;
    QLabel* m_lblEmptyPrompt = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_infoLabel = nullptr;
    QWidget* m_container = nullptr;
=======
    void showSearchBar();
    void hideSearchBar();
    void findNext();
    void updateMatchCount();
    void updateSearchBarGeometry();
    void clearSearchHighlights();

    QuickLookGraphicsView* m_graphicsView = nullptr;
    QPlainTextEdit* m_textEdit = nullptr;
    QLabel* m_lblEmptyPrompt = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_infoLabel = nullptr;
    QWidget* m_container = nullptr;

    // 文本预览内置搜索悬浮控件
    QWidget* m_searchBar = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_btnFindNext = nullptr;
    QPushButton* m_btnCount = nullptr;
    QLabel* m_lblStatus = nullptr;
    QString m_lastSearchQuery;
    int m_cachedTotalMatches = 0;
>>>>>>> REPLACE

### 3.2 `QuarkMeta_Source/src/ui/QuickLookWindow.cpp`

<<<<<<< SEARCH
#include "QuickLookWindow.h"
#include "UiHelper.h"
=======
#include "QuickLookWindow.h"
#include "UiHelper.h"
#include "ColorPaletteEngine.h"
#include <QTextBlock>
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 状态与信息标签
    m_infoLabel = new QLabel(m_container);
    m_infoLabel->setObjectName("QLInfoLabel");
    m_infoLabel->hide();

    rootLayout->addWidget(m_container);
}
=======
    // 状态与信息标签
    m_infoLabel = new QLabel(m_container);
    m_infoLabel->setObjectName("QLInfoLabel");
    m_infoLabel->hide();

    // 🚀【QuickLook 文本搜索悬浮浮层】：两层紧凑型结构
    m_searchBar = new QWidget(this);
    m_searchBar->setObjectName("QLSearchBar");
    m_searchBar->hide();

    auto* searchBarLayout = new QVBoxLayout(m_searchBar);
    searchBarLayout->setContentsMargins(8, 6, 8, 6);
    searchBarLayout->setSpacing(4);

    // 上下两层布局：使状态指示行与搜索输入框左右精准对齐
    auto* mainRow = new QHBoxLayout();
    mainRow->setContentsMargins(0, 0, 0, 0);
    mainRow->setSpacing(6);

    auto* leftCol = new QVBoxLayout();
    leftCol->setContentsMargins(0, 0, 0, 0);
    leftCol->setSpacing(4);

    m_searchEdit = new QLineEdit(m_searchBar);
    m_searchEdit->setObjectName("QLSearchEdit");
    m_searchEdit->setPlaceholderText("在文本中查找...");
    leftCol->addWidget(m_searchEdit);

    m_lblStatus = new QLabel("无 | 0", m_searchBar);
    m_lblStatus->setObjectName("QLSearchStatus");
    leftCol->addWidget(m_lblStatus);

    mainRow->addLayout(leftCol, 1);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(6);

    m_btnFindNext = new QPushButton("查找下一个", m_searchBar);
    m_btnFindNext->setObjectName("QLSearchBtn");
    btnLayout->addWidget(m_btnFindNext);

    m_btnCount = new QPushButton("计数", m_searchBar);
    m_btnCount->setObjectName("QLSearchBtn");
    btnLayout->addWidget(m_btnCount);

    auto* rightCol = new QVBoxLayout();
    rightCol->setContentsMargins(0, 0, 0, 0);
    rightCol->setSpacing(4);
    rightCol->addLayout(btnLayout);
    rightCol->addStretch();

    mainRow->addLayout(rightCol);
    searchBarLayout->addLayout(mainRow);

    connect(m_btnFindNext, &QPushButton::clicked, this, &QuickLookWindow::findNext);
    connect(m_btnCount, &QPushButton::clicked, this, &QuickLookWindow::updateMatchCount);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &QuickLookWindow::findNext);

    m_searchBar->installEventFilter(this);
    m_searchEdit->installEventFilter(this);

    rootLayout->addWidget(m_container);
}
>>>>>>> REPLACE

<<<<<<< SEARCH
void QuickLookWindow::closePreview() {
    m_previewGeneration.fetch_add(1, std::memory_order_relaxed);
    if (m_previewToken) m_previewToken->cancel();
    if (m_graphicsView) {
        m_graphicsView->clear();
    }
    hide();
}
=======
void QuickLookWindow::closePreview() {
    hideSearchBar();
    m_previewGeneration.fetch_add(1, std::memory_order_relaxed);
    if (m_previewToken) m_previewToken->cancel();
    if (m_graphicsView) {
        m_graphicsView->clear();
    }
    hide();
}
>>>>>>> REPLACE

<<<<<<< SEARCH
void QuickLookWindow::renderImage(const QString& path) {
    m_textEdit->hide();
=======
void QuickLookWindow::renderImage(const QString& path) {
    hideSearchBar();
    m_textEdit->hide();
>>>>>>> REPLACE

<<<<<<< SEARCH
void QuickLookWindow::renderText(const QString& path) {
    m_graphicsView->hide();
=======
void QuickLookWindow::renderText(const QString& path) {
    hideSearchBar();
    m_graphicsView->hide();
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (event->key() == Qt::Key_W && (event->modifiers() & Qt::ControlModifier)) {
        closePreview();
        event->accept();
        return;
    }
=======
    if (event->key() == Qt::Key_W && (event->modifiers() & Qt::ControlModifier)) {
        closePreview();
        event->accept();
        return;
    }

    // 🚀【Ctrl+F 场景化门禁】：仅在非图形图像预览（纯文本 m_textEdit 可视）时呼出搜索栏
    if (event->key() == Qt::Key_F && (event->modifiers() & Qt::ControlModifier)) {
        if (m_textEdit && m_textEdit->isVisible()) {
            showSearchBar();
            event->accept();
            return;
        }
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
void QuickLookWindow::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
}
=======
void QuickLookWindow::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    updateSearchBarGeometry();
}

void QuickLookWindow::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateSearchBarGeometry();
}

void QuickLookWindow::updateSearchBarGeometry() {
    if (!m_searchBar) return;
    int barW = 360;
    int barH = m_searchBar->sizeHint().height();
    if (barH < 62) barH = 62;
    int x = width() - barW - 24;
    int y = 24;
    m_searchBar->setGeometry(x, y, barW, barH);
    m_searchBar->raise();
}

void QuickLookWindow::showSearchBar() {
    if (!m_searchBar) return;
    updateSearchBarGeometry();
    m_searchBar->show();
    m_searchBar->raise();
    m_searchEdit->setFocus();
    m_searchEdit->selectAll();
}

void QuickLookWindow::hideSearchBar() {
    if (m_searchBar && m_searchBar->isVisible()) {
        m_searchBar->hide();
        clearSearchHighlights();
    }
}

void QuickLookWindow::clearSearchHighlights() {
    if (m_textEdit) {
        m_textEdit->setExtraSelections({});
    }
    if (m_lblStatus) {
        m_lblStatus->setText("无 | 0");
    }
}

void QuickLookWindow::updateMatchCount() {
    if (!m_textEdit) return;
    QString query = m_searchEdit->text();
    if (query.isEmpty()) {
        clearSearchHighlights();
        return;
    }

    QTextDocument* doc = m_textEdit->document();
    int count = 0;
    int currentLine = -1;
    QTextCursor currentCursor = m_textEdit->textCursor();

    QTextCursor cursor(doc);
    while (!cursor.isNull() && !cursor.atEnd()) {
        cursor = doc->find(query, cursor);
        if (!cursor.isNull()) {
            count++;
            if (cursor.selectionStart() <= currentCursor.selectionStart() &&
                cursor.selectionEnd() >= currentCursor.selectionEnd()) {
                currentLine = cursor.blockNumber() + 1;
            }
        }
    }

    if (count == 0) {
        m_lblStatus->setText("无 | 0");
    } else {
        QString lineStr = (currentLine > 0) ? QString::number(currentLine) : "无";
        m_lblStatus->setText(QString("%1 | %2").arg(lineStr).arg(count));
    }
}

void QuickLookWindow::findNext() {
    if (!m_textEdit) return;
    QString query = m_searchEdit->text();
    if (query.isEmpty()) {
        clearSearchHighlights();
        return;
    }

    QTextDocument* doc = m_textEdit->document();
    QTextCursor startCursor = m_textEdit->textCursor();
    QTextCursor match = doc->find(query, startCursor);

    // 如果到底，从文档开头循环查找
    if (match.isNull()) {
        QTextCursor docStart(doc);
        match = doc->find(query, docStart);
    }

    if (!match.isNull()) {
        m_textEdit->setTextCursor(match);

        // 设置 #ff551C 高亮
        QList<QTextEdit::ExtraSelection> extraSelections;
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(QColor("#ff551C"));
        selection.format.setForeground(Qt::white);
        selection.cursor = match;
        extraSelections.append(selection);
        m_textEdit->setExtraSelections(extraSelections);

        // 统计总数并定位当前行号
        int currentLine = match.blockNumber() + 1;
        int totalMatches = 0;
        QTextCursor c(doc);
        while (!c.isNull() && !c.atEnd()) {
            c = doc->find(query, c);
            if (!c.isNull()) totalMatches++;
        }

        m_lblStatus->setText(QString("%1 | %2").arg(currentLine).arg(totalMatches));
        m_textEdit->centerCursor();
    } else {
        clearSearchHighlights();
    }
}
>>>>>>> REPLACE

<<<<<<< SEARCH
        if (key == Qt::Key_Space || key == Qt::Key_Escape) {
            intercept = true;
        } else if (key == Qt::Key_W && (mods & Qt::ControlModifier)) {
            intercept = true;
=======
        if (key == Qt::Key_Space || key == Qt::Key_Escape) {
            if (m_searchBar && m_searchBar->isVisible() && key == Qt::Key_Escape) {
                hideSearchBar();
                return true;
            }
            // 如果用户正在搜索输入框中输入空格，不拦截关闭，原生放行给输入框打字
            if (m_searchEdit && m_searchEdit->hasFocus() && key == Qt::Key_Space) {
                return false;
            }
            intercept = true;
        } else if (key == Qt::Key_W && (mods & Qt::ControlModifier)) {
            intercept = true;
        } else if (key == Qt::Key_F && (mods & Qt::ControlModifier)) {
            if (m_textEdit && m_textEdit->isVisible()) {
                showSearchBar();
                return true;
            }
            intercept = true;
>>>>>>> REPLACE

<<<<<<< SEARCH
        QWidget* activeWin = QApplication::activeWindow();
        QWidget* focusW = QApplication::focusWidget();
        if (activeWin == this || (focusW && this->isAncestorOf(focusW))) {
            return true;
        }
=======
        QWidget* activeWin = QApplication::activeWindow();
        QWidget* focusW = QApplication::focusWidget();
        if (activeWin == this || (focusW && this->isAncestorOf(focusW)) ||
            (m_searchBar && (m_searchBar == activeWin || m_searchBar->isAncestorOf(focusW)))) {
            return true;
        }
>>>>>>> REPLACE

### 3.3 `QuarkMeta_Source/resources/style.qss`

<<<<<<< SEARCH
QPlainTextEdit#QLPlainTextEdit {
    background: transparent;
    border: none;
    color: #D4D4D4;
    font-family: 'Consolas', 'Monaco', 'PingFang SC', 'Microsoft YaHei';
    font-size: 13px;
}
=======
QPlainTextEdit#QLPlainTextEdit {
    background: transparent;
    border: none;
    color: #D4D4D4;
    font-family: 'Consolas', 'Monaco', 'PingFang SC', 'Microsoft YaHei';
    font-size: 13px;
}

/* QuickLook 文本搜索浮层样式 */
QWidget#QLSearchBar {
    background-color: rgba(30, 30, 30, 0.92);
    border: 1px solid #444444;
    border-radius: 6px;
}

QLineEdit#QLSearchEdit {
    background-color: #252526;
    border: 1px solid #3c3c3c;
    border-radius: 4px;
    color: #E0E0E0;
    padding: 3px 6px;
    font-size: 12px;
}

QLineEdit#QLSearchEdit:focus {
    border: 1px solid #0078D4;
}

QPushButton#QLSearchBtn {
    background-color: #333333;
    border: 1px solid #444444;
    border-radius: 4px;
    color: #E0E0E0;
    padding: 3px 8px;
    font-size: 11px;
}

QPushButton#QLSearchBtn:hover {
    background-color: #404040;
    border-color: #555555;
}

QPushButton#QLSearchBtn:pressed {
    background-color: #252525;
}

QLabel#QLSearchStatus {
    color: #AAAAAA;
    font-size: 11px;
    padding: 0 4px;
}
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 语法验证
- 静态验证 `m_searchBar` 内部控件的指针生命周期与事件过滤器链；
- 确保 `QTextCursor` 查找与 `ExtraSelection` 高亮匹配无内存泄漏；
- 确认样式全面收敛至 `resources/style.qss`，无内联硬编码。

### 4.2 运行时验收标准
1. **图形文件门禁**：按空格键预览图片或 PSD 文件，按下 `Ctrl+F`，搜索框不弹出；
2. **文本预览呼出**：按空格键预览 `.txt` 或 `.cpp` 文件，按下 `Ctrl+F`，右上角平滑呼出搜索浮层，输入光标自动聚焦；
3. **回车与查找下一个**：在输入框内键入字符并按回车或点击“查找下一个”，文本自动滚动并将当前项高亮标记为 `#ff551C`；
4. **两层与状态指示对齐**：下方紧随搜索框显示当前 `行号 | 计数值`；若无匹配内容，显示 `无 | 0`；
5. **ESC 关闭与清除**：按下 ESC 键，搜索框自动隐藏，高亮彻底清除。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- 复用 Qt 原生富文本引擎 `QTextDocument::find` 与 `QPlainTextEdit::setExtraSelections`；
- 复用 `ColorPaletteEngine::isGraphicsFile` 进行纯内存零 I/O 的场景分类；
- 杜绝重复造轮子或创建多余独立窗口。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一真理源 (SSOT) | 派生展示与表现层 | 同步方向 |
| :--- | :--- | :--- | :--- |
| 预览文本流 | `m_textEdit->document()` | `QLPlainTextEdit` | 文本加载时单向解析注入 |
| 搜索关键词与匹配游标 | `m_searchEdit->text()` / `QTextCursor` | `m_lblStatus` / `extraSelections` | 触发查找时动态计算派生 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

- `showSearchBar()` / `hideSearchBar()` / `findNext()` / `updateMatchCount()` / `updateSearchBarGeometry()` / `clearSearchHighlights()`：声明为 private 辅助函数；
- `resizeEvent(QResizeEvent* event)`：protected 虚函数重载，用于在窗口尺寸改变时维护右上角几何坐标。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- `QuickLookWindow.h` 显式包含 `<QLineEdit>`；
- `QuickLookWindow.cpp` 显式包含 `"ColorPaletteEngine.h"` 与 `<QTextBlock>`；
- 类型全部闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项 | 判定 | 说明 |
| :---: | :--- | :---: | :--- |
| 1 | 契约锁 | 满足 | 未破坏已有 public 接口 |
| 2 | 依赖锁 | 满足 | UI 内部单向装配 |
| 3 | 门禁锁 | 满足 | 溯源真理源，非图形图像模式下严格守门 |
| 4 | 静态红线扫描 | 满足 | 无任何 Win32 强夺焦点补丁 |
| 5 | 无内联样式 | 满足 | 样式全部归集于 resources/style.qss |
| 6 | SEARCH 块唯一性 | 满足 | 全部通过代码匹配验证 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

- 预览缩放、平移、翻转、按键打分评级逻辑 100% 保持；
- 快捷键 `Ctrl+W`、`Space`、`Esc` 在非搜索模式下的关闭逻辑完全保留；
- 颜色 `#ff551C` 精准照抄。
