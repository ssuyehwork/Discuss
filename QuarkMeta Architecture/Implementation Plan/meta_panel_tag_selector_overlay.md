# 元数据面板标签按钮化与 TagSelectorOverlay 悬浮选择器实时联动无脑实施方案 (MetaPanel Tag Button & Selector Overlay Implementation Plan)

## 1. Overview（概述与解决的问题）

本实施方案旨在彻底提升元数据属性面板（`MetaPanel`）中的打标签交互体验：
1. **输入框按钮化**：将 `MetaPanel` 中原有的单行文本输入框（`m_tagEdit` "输入标签..."）替换为精美的 **“+ 添加标签”** 矢量按钮（`m_btnAddTag`），按钮严格使用 SVG 矢量图标 `UiHelper::getIcon("add", QColor("#AAAAAA"), 14)` 渲染，契合暗黑主题。
2. **弹出 TagSelectorOverlay 悬浮界面**：点击“+ 添加标签”按钮时，自动在按钮下方弹出全功能标签悬浮选择器（`TagSelectorOverlay`），并传入当前选中项目已有的标签作为初始选中状态。
3. **可持续选择与实时数据联动**：监听 `TagSelectorOverlay::selectionChanged` 信号，在用户勾选或取消勾选标签时，**元数据面板实时同步更新**胶囊气泡（`TagPill`），并向 `CoreEngine` 提交增量标签指令，实时持久化至磁盘 `.QuarkMeta.json`。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/MetaPanel.h`
2. `src/ui/MetaPanel.cpp`
3. `QuarkMeta Architecture/QuarkMeta-Architecture-Planning.md`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/MetaPanel.h`
在 `MetaPanel.h` 中，将 `m_tagEdit` 替换为 `QPushButton* m_btnAddTag` 与 `QPointer<TagSelectorOverlay> m_tagSelectorOverlay` 成员变量声明。

```
<<<<<<< SEARCH
#include "components/ElasticEdit.h"
=======
#include "components/ElasticEdit.h"
#include "TagSelectorOverlay.h"
#include <QPointer>
#include <QPushButton>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    ElasticEdit* m_tagEdit = nullptr;
=======
    QPushButton* m_btnAddTag = nullptr;
    QPointer<TagSelectorOverlay> m_tagSelectorOverlay;
>>>>>>> REPLACE
```

---

### 3.2 `src/ui/MetaPanel.cpp`
在 `MetaPanel.cpp` 中将 `m_tagEdit` 的创建逻辑替换为 `m_btnAddTag` 按钮，绑定点击弹窗 `TagSelectorOverlay` 及 `selectionChanged` 实时增量更新逻辑。

```
<<<<<<< SEARCH
    // 1. 上半部分：输入框
    m_tagEdit = new ElasticEdit(m_tagBox);
    m_tagEdit->setPlaceholderText("输入标签...");
    m_tagEdit->setStyleSheet("QTextEdit { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; padding: 4px 10px; font-size: 12px; color: #AAAAAA; font-weight: normal; }");
    connect(m_tagEdit, &ElasticEdit::returnPressed, this, &MetaPanel::onTagAdded);
    m_tagEdit->installEventFilter(this);
    tagL->addWidget(m_tagEdit);
=======
    // 1. 上半部分：添加标签矢量按钮
    m_btnAddTag = new QPushButton(UiHelper::getIcon("add", QColor("#AAAAAA"), 14), " 添加标签", m_tagBox);
    m_btnAddTag->setFixedHeight(28);
    m_btnAddTag->setCursor(Qt::PointingHandCursor);
    m_btnAddTag->setStyleSheet(QString(
        "QPushButton { background-color: #252526; border: 1px solid #3c3c3c; border-radius: 4px; padding: 0 10px; color: #AAAAAA; font-size: 12px; text-align: left; }"
        "QPushButton:hover { background-color: #2a2d2e; border-color: #1abc9c; color: #FFFFFF; }"
        "QPushButton:pressed { background-color: #333333; }"
    ));

    connect(m_btnAddTag, &QPushButton::clicked, this, [this]() {
        if (m_tagSelectorOverlay) {
            m_tagSelectorOverlay->close();
            return;
        }

        // 收集当前已有标签
        QStringList currentTags;
        for (int i = 0; i < m_tagFlowLayout->count(); ++i) {
            TagPill* pill = qobject_cast<TagPill*>(m_tagFlowLayout->itemAt(i)->widget());
            if (pill) {
                QString tagStr = pill->property("tagText").toString();
                if (!tagStr.isEmpty()) currentTags.append(tagStr);
            }
        }

        m_tagSelectorOverlay = new TagSelectorOverlay(currentTags, this->topLevelWidget());
        QPoint globalPos = m_btnAddTag->mapToGlobal(QPoint(0, m_btnAddTag->height() + 4));
        QPoint parentPos = this->topLevelWidget()->mapFromGlobal(globalPos);
        m_tagSelectorOverlay->move(parentPos);
        m_tagSelectorOverlay->resize(320, 360);
        m_tagSelectorOverlay->show();

        connect(m_tagSelectorOverlay, &TagSelectorOverlay::selectionChanged, this, [this](const QStringList& selectedTags) {
            // 实时对比与渲染更新
            QStringList oldTags;
            for (int i = 0; i < m_tagFlowLayout->count(); ++i) {
                TagPill* pill = qobject_cast<TagPill*>(m_tagFlowLayout->itemAt(i)->widget());
                if (pill) oldTags.append(pill->property("tagText").toString());
            }

            // 找出新增与删除的标签
            for (const QString& tag : selectedTags) {
                if (!oldTags.contains(tag)) {
                    TagPill* pill = new TagPill(tag, m_tagContainer);
                    pill->setProperty("tagText", tag);
                    connect(pill, &TagPill::deleteRequested, this, &MetaPanel::onTagDeleted);
                    m_tagFlowLayout->addWidget(pill);
                    emit tagAddRequested(m_selectedPaths, tag);
                }
            }
            for (const QString& oldTag : oldTags) {
                if (!selectedTags.contains(oldTag)) {
                    onTagDeleted(oldTag);
                }
            }
            adjustTagContainerHeight();
        });
    });
    tagL->addWidget(m_btnAddTag);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译确认**：
   在命令行运行 CMake 编译，验证 `MOC` 与全工程无符号缺失错误：
   ```bash
   cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```
2. **功能验证**：
   - **界面变更为按钮确认**：展开右侧第 4 栏元数据面板，确认原本的“输入标签...”编辑框变为了精美的“+ 添加标签”矢量按钮。
   - **点击弹窗验证**：点击“+ 添加标签”按钮，确认弹出 `TagSelectorOverlay` 悬浮选择器。
   - **可持续选择与实时联动验证**：在悬浮选择器中点击/勾选标签，观察元数据面板中的标签气泡（`TagPill`）实时增减展现，且磁盘离散 JSON 元数据实时同步保存。
