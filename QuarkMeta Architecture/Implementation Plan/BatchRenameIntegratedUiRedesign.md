# 批量重命名单窗双栏与实时预览界面重构无脑实施方案 —— BatchRenameIntegratedUiRedesign

本实施方案旨在将原本分离的“批量重命名”设置窗口与“批量重命名预览”窗口合并为一个单窗双栏集成界面，实现实时无感自动预览、左侧固定/最大 500px 布局、右侧 700px 预览列表、行高不超过 30px、最左侧微型缩略图/图标展示以及统一的深色暗黑斑马纹（`#1E1E1E` / `#252526`）。

同时彻底废除“取消”、“预览”及“关闭预览”等冗余按钮，将“重命名”主按钮移动并水平居中置于左侧栏底部。

---

## 修改文件清单

1. `src/ui/BatchRenameDialog.h`
2. `src/ui/BatchRenameDialog.cpp`
3. `src/ui/BatchRenamePreviewDialog.h` (作废)
4. `src/ui/BatchRenamePreviewDialog.cpp` (作废)
5. `CMakeLists.txt`

---

## 阶段一：重构对话框头文件 `src/ui/BatchRenameDialog.h`

**修改文件**：`src/ui/BatchRenameDialog.h`
**修改目的**：新增右侧对比预览表格组件 `QTableWidget* m_table`，剔除废弃的 `m_btnCancel` 与 `m_btnPreview` 按钮指针，添加动态规则变动信号绑定函数。

**精准替换 Diff**：
```cpp
<<<<<<< SEARCH
    // 动作按钮 (右侧栏)
    QPushButton* m_btnExecute = nullptr;
    QPushButton* m_btnCancel = nullptr;
    QPushButton* m_btnPreview = nullptr;
=======
    // 右侧实时预览表格 (700px 宽度)
    QTableWidget* m_table = nullptr;

    // 左侧底部主执行按钮 (水平居中)
    QPushButton* m_btnExecute = nullptr;
>>>>>>> REPLACE
```

---

## 阶段二：重构对话框实现文件 `src/ui/BatchRenameDialog.cpp`

**修改文件**：`src/ui/BatchRenameDialog.cpp`
**修改目的**：
1. 窗口尺寸调整为横向 1220×650 像素（左侧最大 500px，右侧 700px）。
2. 在左侧栏底部水平居中放置“重命名”主按钮，彻底废除“取消”与“预览”按钮。
3. 右侧包含 `QTableWidget` 对比列表，设置 `verticalHeader()->setDefaultSectionSize(28)`（确保行高不超过 30 像素），启用 `setAlternatingRowColors(true)` 并配置 `#1E1E1E` 与 `#252526` 深色暗黑斑马纹。
4. 表格第 0 列单元格带有文件微型缩略图/关联图标。
5. 绑定规则变动事件，只要用户修改/新增规则，立即调用 `updatePreview()` 在右侧实时刷出“重命名后名称”。

**精准替换 Diff**：
```cpp
<<<<<<< SEARCH
    m_table = new QTableWidget(this);
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({"当前文件名", "重命名后名称"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
=======
    // 右侧预览面板初始化 (700px 宽度)
    m_table = new QTableWidget(this);
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({"当前文件名", "重命名后名称"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(28); // 🚨 物理红线：行高限制不超过 30 像素 (设定为 28px)
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true); // 开启斑马纹
    m_table->setShowGrid(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setIconSize(QSize(20, 20)); // 最左侧微型缩略图/图标尺寸
>>>>>>> REPLACE
```

**QSS 暗黑斑马纹配色替换 Diff**：
```cpp
<<<<<<< SEARCH
        "QTableWidget { background-color: #252526; border: 1px solid #333; gridline-color: transparent; selection-background-color: rgba(55, 138, 221, 0.2); }"
=======
        "QTableWidget { background-color: #1E1E1E; alternate-background-color: #252526; color: #EEEEEE; border: 1px solid #333333; gridline-color: transparent; selection-background-color: rgba(52, 152, 219, 0.2); }"
        "QHeaderView::section { background-color: #2D2D2D; color: #888888; border: none; height: 30px; font-weight: bold; font-size: 11px; }"
>>>>>>> REPLACE
```

**实时更新预览列表 Diff**：
```cpp
<<<<<<< SEARCH
void BatchRenameDialog::updatePreview() {
    // 旧预览调用
}
=======
void BatchRenameDialog::updatePreview() {
    std::vector<RenameRule> rules;
    for (auto* row : m_ruleRows) {
        if (row) rules.push_back(row->getRule());
    }

    auto newNames = BatchRenameEngine::instance().preview(m_originalPaths, rules);
    m_table->setRowCount(static_cast<int>(m_originalPaths.size()));

    for (int i = 0; i < static_cast<int>(m_originalPaths.size()); ++i) {
        QString oldPath = QString::fromStdWString(m_originalPaths[static_cast<size_t>(i)]);
        QFileInfo info(oldPath);

        // 1. 左侧原文件名（带微型缩略图/文件关联图标）
        QIcon fileIcon = ShellIconManager::getFileIcon(oldPath, 20);
        auto* itemOld = new QTableWidgetItem(fileIcon, info.fileName());
        itemOld->setForeground(QColor("#B0B0B0"));
        m_table->setItem(i, 0, itemOld);

        // 2. 右侧重命名后新名称 (绿色高亮显示新名称)
        QString newName = QString::fromStdWString(newNames[static_cast<size_t>(i)]);
        auto* itemNew = new QTableWidgetItem(newName);
        itemNew->setForeground(QColor("#2ecc71"));
        m_table->setItem(i, 1, itemNew);
    }
}
>>>>>>> REPLACE
```

---

## 阶段三：清理 CMakeLists.txt 构建列表

从 `CMakeLists.txt` 中移除已作废的独立预览弹窗源文件 `BatchRenamePreviewDialog.cpp` / `.h`：

```cmake
<<<<<<< SEARCH
    src/ui/BatchRenamePreviewDialog.cpp
    src/ui/BatchRenamePreviewDialog.h
=======
>>>>>>> REPLACE
```

---

## 验证与测试步骤

1. **界面尺寸与布局验证**：
   在文件视图中选中多个文件按快捷键 `Ctrl+Shift+R` 打开批量重命名窗口，确认呈现为单窗双栏：左侧设置栏宽度 $\le 500\text{px}$，右侧预览列表宽度 $700\text{px}$。
2. **按钮定位与精简验证**：
   确认原本的“取消”和“预览”按钮已消失；确认“重命名”主按钮置于左侧栏底部并水平居中。
3. **实时无感自动预览验证**：
   在左侧添加、删除或修改任意文本/序列规则，确认右侧列表中“重命名后名称”即时自动更新刷新，无需手动点击预览。
4. **表格行高与斑马纹验证**：
   检查右侧表格行高不超过 30 像素（实际为 28px），最左侧每个文件名旁正确显示微型图标；表项交替显示 `#1E1E1E` 与 `#252526` 深色暗黑斑马纹配色。
