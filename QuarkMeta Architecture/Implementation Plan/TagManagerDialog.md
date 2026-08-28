# TagManagerDialog 对话框与悬浮遮罩纯化及全量 SVG 图标化实施方案

## 1. Overview（概述与解决的问题）
本实施方案旨在全面纯化 `TagManagerDialog` 与 `TagSelectorOverlay`：
1. **1:1 标准 API 对齐**：全面使用 `TagLexiconService` 标准接口（`getAllTagGroups()`、`getAllTagNames()`、`TagGroup`、`moveTagToGroup()` 等），废除旧有的 `TagLexiconGroup` 和 `getAllMasterTags()` 调用。
2. **纯 SVG 图标化与符号 0 容忍**：物理移除文本/Emoji 符号（如 `"📁 "`、`"• "`、`"+ "` 等），统一采用 `UiHelper::getIcon(...)` 加载标准的矢量 SVG 图标。
3. **悬浮遮罩防护与自闭环**：加固 `TagSelectorOverlay` 视口碰撞反折与失焦自动销毁机制。

---

## 2. Modified Files List（影响文件清单）
- `src/ui/TagManagerDialog.h`
- `src/ui/TagManagerDialog.cpp`
- `src/ui/TagSelectorOverlay.h`
- `src/ui/TagSelectorOverlay.cpp`
- `CMakeLists.txt`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/TagManagerDialog.h`
<<<<<<< SEARCH
    QList<TagLexiconGroup> m_allGroups;
    QStringList m_masterTags;
=======
    // 🚀【类型对齐】：采用标准 TagGroup 数据结构
    QList<TagGroup> m_allGroups;
    QStringList m_masterTags;
>>>>>>> REPLACE

### 3.2 `src/ui/TagManagerDialog.cpp`
<<<<<<< SEARCH
    createSideBtn(0, "all_data", "全部标签");
    createSideBtn(-1, "uncategorized", "未分类标签");
    createSideBtn(-2, "star_filled", "常用标签");

    m_allGroups = TagLexiconService::instance().getAllGroups();
    for (const auto& grp : m_allGroups) {
        if (grp.id <= 0) continue;
        QPushButton* btn = createSideBtn(grp.id, "folder_filled", grp.name);
        btn->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(btn, &QWidget::customContextMenuRequested, this, [this, grp](const QPoint& pos) {
            QPushButton* b = qobject_cast<QPushButton*>(sender());
            if (b) showGroupContextMenu(grp.id, grp.name, b->mapToGlobal(pos));
        });
    }
=======
    createSideBtn(0, "all_data", "全部标签");
    createSideBtn(-1, "uncategorized", "未分类标签");
    createSideBtn(-2, "star_filled", "常用标签");

    // 🚀【API 对齐】：调用 getAllTagGroups()
    m_allGroups = TagLexiconService::instance().getAllTagGroups();
    for (const auto& grp : m_allGroups) {
        if (grp.id <= 0) continue; // 排除默认组
        QPushButton* btn = createSideBtn(grp.id, "folder_filled", grp.name);
        btn->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(btn, &QWidget::customContextMenuRequested, this, [this, grp](const QPoint& pos) {
            QPushButton* b = qobject_cast<QPushButton*>(sender());
            if (b) showGroupContextMenu(grp.id, grp.name, b->mapToGlobal(pos));
        });
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
        QPushButton* btn = new QPushButton("• " + tag, flowContainer);
=======
        QPushButton* btn = new QPushButton(tag, flowContainer);
        btn->setIcon(UiHelper::getIcon("tag_pill", QColor("#888888"), 12));
        btn->setIconSize(QSize(12, 12));
>>>>>>> REPLACE

### 3.3 `src/ui/TagSelectorOverlay.h`
<<<<<<< SEARCH
    QList<TagLexiconGroup> m_lexiconGroups;
=======
    // 🚀【类型对齐】：对齐标准 TagGroup
    QList<TagGroup> m_lexiconGroups;
>>>>>>> REPLACE

### 3.4 `src/ui/TagSelectorOverlay.cpp`
<<<<<<< SEARCH
void TagSelectorOverlay::loadTagsAndGroups() {
    m_lexiconGroups = TagLexiconService::instance().getAllGroups();
    
    m_groupList->clear();
    m_groupList->addItem("全部");
    m_groupList->addItem("未分类");

    for (const auto& grp : m_lexiconGroups) {
        if (grp.id > 0) {
            m_groupList->addItem("📁 " + grp.name);
        }
    }

    m_groupList->setCurrentRow(0);
}
=======
void TagSelectorOverlay::loadTagsAndGroups() {
    m_lexiconGroups = TagLexiconService::instance().getAllTagGroups();
    
    m_groupList->clear();
    
    auto addGroupItem = [this](const QString& name, const QString& iconKey) {
        QListWidgetItem* item = new QListWidgetItem(name, m_groupList);
        item->setIcon(UiHelper::getIcon(iconKey, QColor("#AAAAAA"), 14));
        return item;
    };

    addGroupItem("全部", "all_data");
    addGroupItem("未分类", "uncategorized");

    for (const auto& grp : m_lexiconGroups) {
        if (grp.id > 0) {
            addGroupItem(grp.name, "folder_filled");
        }
    }

    m_groupList->setCurrentRow(0);
}
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）
1. 检查构建注册：确保 `CMakeLists.txt` 已包含所有对话框源文件。
2. 静态符号检查：代码中不存在 `"📁 "`、`"• "` 等硬编码文本字符，全量通过 `UiHelper::getIcon` 注入 SVG。
3. 动态验证：测试呼出 `TagManagerDialog` 与 `TagSelectorOverlay`，确认标签分组列表渲染清晰且 SVG 图标高保真对齐。
