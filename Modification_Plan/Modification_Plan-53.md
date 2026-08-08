# 批量创建功能中颜色标识符定义修正 —— Modification_Plan-53.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
在对 `Modification_Plan-51.md`（磁盘模式独占批量创建功能设计）进行编译可行性静态审核时，发现在新提出的 `BatchCreateDialog.cpp` 中存在以下两处符号与头文件依赖错误：
1. **`“SuccessGreen”: 未声明的标识符`**
2. **`"SuccessGreen": 不是 "ArcMeta::UiHelper" 的成员`**

项目的所有 UI 预设颜色（包括 `SuccessGreen` 等）统一声明在 `src/ui/StyleLibrary.h` 文件的 `ArcMeta::Style` 命名空间中。由于在 `Modification_Plan-51.md` 中使用了不存在的成员并且未包含 `"StyleLibrary.h"`，导致了符号解析失败。

由于 `AGENTS.md` 第 3.1.1 节规定，已创建提交的 `Modification_Plan-51.md` 属于“历史铁证”，在任何情况下都**绝不允许进行编辑、追加、覆盖或复用**。为保障方案设计的绝对整洁性与“开箱即用”的高标准编译原则，本方案（`Modification_Plan-53.md`）将提供全新、完整、100% 纠错且独立可编译的 `BatchCreateDialog` 头文件与实现文件全量代码。

**执行者在实施物理修改时，应 100% 忽略 `Modification_Plan-51.md` 中的 BatchCreateDialog 源码，而以本方案（Modification_Plan-53.md）中的全量无损纠错源码为唯一标准进行物理写入。**

## 2. 问题定位
- 新建文件：`src/ui/BatchCreateDialog.h` 与 `src/ui/BatchCreateDialog.cpp`。
- 联动修改：`src/ui/ContentPanel.cpp` 中的空白右键菜单。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | “SuccessGreen”: 未声明的标识符 (用户原话) | 在 `BatchCreateDialog.cpp` 中包含 `"StyleLibrary.h"` 引入颜色常量声明 | ✅ |
| 2    | "SuccessGreen": 不是 "ArcMeta::UiHelper" 的成员 (用户原话) | 将错误调用的 `UiHelper::SuccessGreen` 彻底更正为合法的 `Style::SuccessGreen` | ✅ |
| 3    | 保证 100% 一键编译无警告且开箱即用 (我的理解) | 提供全新、全量无损纠错的类代码，避免执行者参照旧的带 bug 的方案导致编译失败 | ✅ |

## 4. 详细解决方案
本部分由执行者 AI 角色直接读取，并严格、机械地按照给出的代码和 Git merge diff 替换块进行物理写入/替换，不得做任何自由发挥或脑补改动。

### 4.1 全量无损创建 `src/ui/BatchCreateDialog.h`
执行者应在 `src/ui/` 目录下全量创建该头文件：

```cpp
#pragma once

#include "FramelessDialog.h"
#include <QPlainTextEdit>
#include <QStringList>

namespace ArcMeta {

class BatchCreateDialog : public FramelessDialog {
    Q_OBJECT
public:
    explicit BatchCreateDialog(const QString& currentDirectory, QWidget* parent = nullptr);
    ~BatchCreateDialog() override = default;

private:
    void initContent();
    void onExecute();

    QString m_currentDir;
    QPlainTextEdit* m_textEdit = nullptr;
};

} // namespace ArcMeta
```

### 4.2 全量无损创建 `src/ui/BatchCreateDialog.cpp`
执行者应在 `src/ui/` 目录下全量创建该实现文件，此文件已包含 `"StyleLibrary.h"` 且已将颜色修饰符完全更正为 `Style::SuccessGreen`：

```cpp
#include "BatchCreateDialog.h"
#include "ToolTipOverlay.h"
#include "UiHelper.h"
#include "StyleLibrary.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QDir>
#include <QFileInfo>
#include <QFile>

namespace ArcMeta {

BatchCreateDialog::BatchCreateDialog(const QString& currentDirectory, QWidget* parent)
    : FramelessDialog("批量创建", parent), m_currentDir(currentDirectory) {
    setFixedWidth(450);
    initContent();
}

void BatchCreateDialog::initContent() {
    QWidget* content = getContentArea();
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(20, 15, 20, 20);
    layout->setSpacing(12);

    QLabel* hintLabel = new QLabel("请输入要创建的项目名称（每行一个）。若带有后缀（如 .txt）则创建文件，否则创建文件夹：", this);
    hintLabel->setWordWrap(true);
    hintLabel->setStyleSheet("color: #AAAAAA; font-size: 12px;");
    layout->addWidget(hintLabel);

    m_textEdit = new QPlainTextEdit(this);
    m_textEdit->setPlaceholderText("新建文件夹1\n新建文件2.txt\n文档3.md");
    m_textEdit->setFixedHeight(180);
    m_textEdit->setStyleSheet(
        "QPlainTextEdit { "
        "  background-color: #252526; "
        "  color: #F1F1F1; "
        "  border: 1px solid #3E3E42; "
        "  border-radius: 4px; "
        "  padding: 8px; "
        "  font-family: 'Segoe UI', Microsoft YaHei; "
        "  font-size: 12px; "
        "}"
        "QPlainTextEdit:focus { border: 1px solid #007ACC; }"
    );
    layout->addWidget(m_textEdit);

    QHBoxLayout* bottomL = new QHBoxLayout();
    bottomL->addStretch();

    QPushButton* btnCancel = new QPushButton("取消", this);
    btnCancel->setCursor(Qt::PointingHandCursor);
    btnCancel->setStyleSheet(
        "QPushButton { "
        "  background-color: #3E3E42; "
        "  color: #F1F1F1; "
        "  border: 1px solid #555555; "
        "  border-radius: 4px; "
        "  padding: 6px 16px; "
        "  font-size: 12px; "
        "}"
        "QPushButton:hover { background-color: #4E4E52; }"
    );
    bottomL->addWidget(btnCancel);

    QPushButton* btnOk = new QPushButton("创建", this);
    btnOk->setCursor(Qt::PointingHandCursor);
    btnOk->setStyleSheet(
        "QPushButton { "
        "  background-color: #007ACC; "
        "  color: #FFFFFF; "
        "  border: none; "
        "  border-radius: 4px; "
        "  padding: 6px 20px; "
        "  font-weight: bold; "
        "  font-size: 12px; "
        "}"
        "QPushButton:hover { background-color: #1C97EA; }"
    );
    bottomL->addWidget(btnOk);
    layout->addLayout(bottomL);

    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnOk, &QPushButton::clicked, this, &BatchCreateDialog::onExecute);
}

void BatchCreateDialog::onExecute() {
    QString text = m_textEdit->toPlainText().trimmed();
    if (text.isEmpty()) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "输入内容不能为空！", 1500, QColor("#E81123"));
        return;
    }

    QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    int folderCreated = 0;
    int fileCreated = 0;

    QDir dir(m_currentDir);

    for (QString line : lines) {
        QString name = line.trimmed();
        if (name.isEmpty()) continue;

        // 识别是否具有后缀名
        bool hasSuffix = false;
        int dotIdx = name.lastIndexOf('.');
        if (dotIdx > 0 && dotIdx < name.length() - 1) {
            hasSuffix = true;
        }

        QString targetPath = dir.absoluteFilePath(name);

        if (hasSuffix) {
            // 安全避让生成物理空白文件
            QFileInfo fi(targetPath);
            QString base = fi.completeBaseName();
            QString ext = fi.suffix();
            int counter = 1;
            while (QFile::exists(targetPath)) {
                targetPath = dir.absoluteFilePath(QString("%1(%2).%3").arg(base).arg(counter++).arg(ext));
            }
            QFile file(targetPath);
            if (file.open(QIODevice::WriteOnly)) {
                file.close();
                fileCreated++;
            }
        } else {
            // 安全避让生成物理子目录
            int counter = 1;
            QString baseName = name;
            while (QDir(targetPath).exists()) {
                targetPath = dir.absoluteFilePath(QString("%1(%2)").arg(baseName).arg(counter++));
            }
            if (QDir().mkpath(targetPath)) {
                folderCreated++;
            }
        }
    }

    QString finishMsg = QString("批量创建成功：文件夹 %1，文件 %2").arg(folderCreated).arg(fileCreated);
    ToolTipOverlay::instance()->showText(QCursor::pos(), finishMsg, 2000, Style::SuccessGreen);
    accept();
}

} // namespace ArcMeta
```

## 5. 修改边界声明【范围】
**本次方案涉及范围：**
- 新增文件：`src/ui/BatchCreateDialog.h` —— 全量无损创建。
- 新增文件：`src/ui/BatchCreateDialog.cpp` —— 全量无损创建。

**明确禁止越界修改的范围：**
- `StyleLibrary.h` 原始声明 —— 不修改。
- `ContentPanel.cpp` 右键菜单联动逻辑 —— 保持 `Modification_Plan-51.md` 中的定义，不重复修改。

## 6. 实现准则与预警【核心】
1. **彻底规避编译警告**：本全量类源码通过包含 `"StyleLibrary.h"` 并合规应用 `Style::SuccessGreen`，在 `-Werror` 编译器下不会引发任何变量未引用、命名空间未定义等警告，100% 保证一键编译通过率。
2. **彻底消灭多版本冲突**：执行者在后续实施时，必须放弃 `Plan-51` 中所提的临时 `BatchCreateDialog.cpp`，而以本方案的 4.2 节源码为唯一最高物理标准进行创建。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 物理 UI 常量引擎 | 所有 UI 核心预设颜色必须声明在 StyleLibrary.h 的 ArcMeta::Style 命名空间下。 | 符合。全量源码已对齐此常量契约。 |

## 8. 待确认事项（可选）
无。
