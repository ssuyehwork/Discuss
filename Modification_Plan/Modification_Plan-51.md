# 磁盘模式独占批量创建功能设计 —— Modification_Plan-51.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
为了提升系统对批量文件及文件夹生成的支持，用户期望在应用中引入一个“批量创建”功能模块。该功能可以让用户一次性输入多个名称（按行分隔），并智能、自动地批量生成文件夹或带后缀名的物理文件。

为了贯彻本项目核心的“双轨路由物理隔离”与“单一职责原则（SRP）”，该功能在设计与实施上必须遵循以下红线约束：
1. **磁盘目录模式独占**：本模块只在“目录导航模式”（物理磁盘 I/O 驱动）下可用。当应用切换至“侧边栏分类模式”（托管内存模式）时，右键空白处的对应创建菜单项必须完全置灰或隐藏。
2. **文本按行解析与智能分流**：支持文件夹与物理空白文件自适应混合创建。
3. **安全自愈避让**：在遇到同名文件或子目录时，系统应自动进行尾部自增序号避让。

## 2. 问题定位
- 新增功能。需要新增专有轻量化对话框 `BatchCreateDialog.h` 与 `BatchCreateDialog.cpp`。
- 联动入口：`src/ui/ContentPanel.cpp` 中的空白处右键菜单构建逻辑。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 期望添加一个模块，专门用来批量创建文件夹或某种后缀名文件 (用户原话) | 创建支持多行文本驱动、智能解析后缀名的全新批量创建对话框类 | ✅ |
| 2    | 仅可在磁盘目录模式下使用 (用户原话) | 在 `ContentPanel` 右键菜单中根据 `isMirrorSource()` 结果硬阻断/隐藏该操作 | ✅ |
| 3    | 彻底根除 -Wunused-variable / -Wunused-parameter (我的理解) | 在新增类的所有声明与槽连接中，完美设计变量生存周期，形参做屏蔽注释处理 | ✅ |

## 4. 详细解决方案
本部分由执行者 AI 角色直接读取，并严格、机械地按照给出的 Git merge diff 代码块进行物理替换，不得做任何自由发挥或脑补改动。

### 4.1 新增批量创建对话框
在 `src/ui/` 目录下新增两个文件：`BatchCreateDialog.h` 与 `BatchCreateDialog.cpp`。

#### 4.1.1 创建 `src/ui/BatchCreateDialog.h`
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

#### 4.1.2 创建 `src/ui/BatchCreateDialog.cpp`
```cpp
#include "BatchCreateDialog.h"
#include "ToolTipOverlay.h"
#include "UiHelper.h"
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
    ToolTipOverlay::instance()->showText(QCursor::pos(), finishMsg, 2000, UiHelper::SuccessGreen);
    accept();
}

} // namespace ArcMeta
```

### 4.2 联动注册菜单
在内容区空白处的右键菜单中增加入口并进行严格的磁盘模式激活限制。

```
<<<<<<< SEARCH
    QAction* actNewFolder = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建文件夹");
    QAction* actNewMd     = menu.addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "新建 Markdown");

    connect(actNewFolder, &QAction::triggered, this, [this]() { createNewItem("folder"); });
    connect(actNewMd,     &QAction::triggered, this, [this]() { createNewItem("md"); });

    menu.exec(globalPos);
=======
    QAction* actNewFolder = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建文件夹");
    QAction* actNewMd     = menu.addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "新建 Markdown");

    menu.addSeparator();

    QAction* actBatchCreate = menu.addAction(UiHelper::getIcon("add", QColor("#EEEEEE")), "批量创建项目...");
    // 6.1 磁盘目录模式独占
    if (isMirrorSource()) {
        actBatchCreate->setEnabled(false);
        actBatchCreate->setToolTip("批量创建仅支持在物理磁盘模式下使用");
    }

    connect(actNewFolder, &QAction::triggered, this, [this]() { createNewItem("folder"); });
    connect(actNewMd,     &QAction::triggered, this, [this]() { createNewItem("md"); });
    connect(actBatchCreate, &QAction::triggered, this, [this]() {
        // 打开批量创建对话框
        BatchCreateDialog dlg(m_currentPath, this);
        if (dlg.exec() == QDialog::Accepted) {
            refreshAll();
        }
    });

    menu.exec(globalPos);
>>>>>>> REPLACE
```

## 5. 修改边界声明【范围】
**本次方案涉及范围：**
- 新增：`src/ui/BatchCreateDialog.h`
- 新增：`src/ui/BatchCreateDialog.cpp`
- 修改：`src/ui/ContentPanel.cpp` —— 空白右键菜单联动注入

**明确禁止越界修改的范围：**
- 侧边栏托管分类树双击、拖拽逻辑 —— 不修改
- 数据库底层写路由 —— 不修改

## 6. 实现准则与预警【核心】
1. **防止 -Wunused-variable / -Wunused-parameter 警告**：
   在所有继承或实现类的槽函数连接和 Lambda 表达式中，绝对不要引入未引用或空闲的局部变量，确保 C++ 编译器在 `-Werror` 严格模式下一键完美编译通过。
2. **头文件依赖预警**：
   在 `src/ui/ContentPanel.cpp` 顶部引入新组件时，必须添加 `#include "BatchCreateDialog.h"`，防止出现未知类定义等阻碍性编译断裂。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 批量重命名/创建非模态通知 | 任何批量操作结束后，绝不可弹出导致线程阻塞的模态通知（MessageBox）。必须使用 ToolTipOverlay 做平滑、轻量化通知。 | 符合。批量创建完成后采用 ToolTipOverlay 进行就地气泡提示。 |

## 8. 待确认事项（可选）
无。
