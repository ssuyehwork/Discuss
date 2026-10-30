# Implementation Plan - 拖拽遮罩标记文件夹名称列表显示与右键菜单【一键分发】新增方案

## 1. 方案概述 (Overview)
本方案旨在进一步提升 QuarkMeta 标记目的地（Marked Folders）功能的人性化视觉反馈与快捷操作体验：
1. **拖拽遮罩标记文件夹名称多行展示**：当用户将文件/文件夹拖拽至右侧“元数据属性面板”（`MetaPanel`）或“筛选器面板”（`FilterPanel`）时，遮罩上除了显示“迁移到 N 个标记文件夹”主标题外，在下方逐行展示所有已激活标记文件夹的真实名称（一个名称一行），让用户在拖拽停留瞬间对分发目的地一目了然。
2. **右键菜单【移动到】新增【一键分发】选项**：在右键上下文菜单（`ContentContextMenu`）的“移动到...”级联子菜单最顶部，新增快捷选项“一键分发”。点击后直接调用 `MarkedFolderService::instance().executeTransferToMarked(sourcePaths)`，与快捷键 `Alt+V` 保持相同的中央分发搬运逻辑与状态通知。

---

## 2. 问题分析与根因溯源 (Analysis & Root Cause)
1. **遮罩信息盲区**：目前拖拽遮罩仅显示抽象的数量统计（如“迁移到 1 个标记文件夹”），用户无法直观确认究竟会分发搬运到哪几个具体的目标文件夹，存在操作误解风险。
2. **分发入口单一**：目前一键分发主要依赖键盘快捷键 `Alt+V` 或拖拽到“标记”面板空白处，缺乏在右键“移动到...”主功能菜单中的显式入口。

---

## 3. 修改文件范围 (Scope of Changes)

### 核心代码文件
- `src/ui/MetaPanel.cpp`：升级 `showDragOverlay`，读取 `MarkedFolderService::instance().getActiveMarked()`，提取文件夹名称拼接为多行 HTML/Text 文本展示在遮罩 Label 上。
- `src/ui/FilterPanel.cpp`：升级 `showDragOverlay`，同步支持多行已激活标记文件夹名称展示。
- `src/ui/controllers/ContentContextMenu.cpp`：在 `moveMenu` 最顶部添加“一键分发”菜单项（绑定 `Folder-add.svg` 或 `Destination.svg` 图标与 `#EEEEEE` 中性色），触发时执行 `executeTransferToMarked`。

---

## 4. 架构契约与规范核查 (Architecture Checks)
- **【契约锁】规范统一入口**：统一路由至 `MarkedFolderService::instance().executeTransferToMarked(sourcePaths)`，绝对禁止自写局部搬运逻辑。
- **【依赖锁】单向依赖**：UI 面板通过公开服务层 `MarkedFolderService` 查询状态，黑盒解耦。
- **【门禁锁】图标与颜色契约**：右键菜单项图标统一采用 SVG 格式与 `#EEEEEE` 中性单色，严格守住红线。

---

## 5. 详细实现方案 (Implementation Details)

### 5.1 遮罩文本动态拼接逻辑 (MetaPanel & FilterPanel)
```cpp
auto activeFolders = MarkedFolderService::instance().getActiveMarked();
if (!activeFolders.isEmpty()) {
    QStringList lines;
    lines << QString("迁移到 %1 个标记文件夹").arg(activeFolders.size());
    lines << "----------------";
    for (const auto& folder : activeFolders) {
        QString name = folder.name;
        if (name.isEmpty()) {
            name = QFileInfo(folder.path).fileName();
        }
        lines << name;
    }
    m_dragOverlayLabel->setText(lines.join("\n"));
} else {
    m_dragOverlayLabel->setText("尚未标记目的地");
}
```

---

## 6. 代码修改 Blocks (Search & Replace Blocks)

### Block 1: `src/ui/MetaPanel.cpp`
<<<<<<< SEARCH
void MetaPanel::showDragOverlay(int activeCount) {
    if (!m_dragOverlayWidget) {
        m_dragOverlayWidget = new QWidget(this);
        m_dragOverlayWidget->setObjectName("DragOverlayWidget");
        m_dragOverlayWidget->setAttribute(Qt::WA_TransparentForMouseEvents);

        QVBoxLayout* layout = new QVBoxLayout(m_dragOverlayWidget);
        layout->setContentsMargins(0, 0, 0, 0);
        m_dragOverlayLabel = new QLabel(m_dragOverlayWidget);
        m_dragOverlayLabel->setAlignment(Qt::AlignCenter);
        m_dragOverlayLabel->setStyleSheet("color: #FFFFFF; font-size: 14px; font-weight: bold;");
        layout->addWidget(m_dragOverlayLabel);
    }

    if (activeCount > 0) {
        m_dragOverlayLabel->setText(QString("迁移到 %1 个标记文件夹").arg(activeCount));
    } else {
        m_dragOverlayLabel->setText("尚未标记目的地");
    }

    m_dragOverlayWidget->setGeometry(rect());
    m_dragOverlayWidget->show();
    m_dragOverlayWidget->raise();
}
=======
void MetaPanel::showDragOverlay(int activeCount) {
    if (!m_dragOverlayWidget) {
        m_dragOverlayWidget = new QWidget(this);
        m_dragOverlayWidget->setObjectName("DragOverlayWidget");
        m_dragOverlayWidget->setAttribute(Qt::WA_TransparentForMouseEvents);

        QVBoxLayout* layout = new QVBoxLayout(m_dragOverlayWidget);
        layout->setContentsMargins(10, 10, 10, 10);
        m_dragOverlayLabel = new QLabel(m_dragOverlayWidget);
        m_dragOverlayLabel->setAlignment(Qt::AlignCenter);
        m_dragOverlayLabel->setStyleSheet("color: #FFFFFF; font-size: 13px; font-weight: bold; line-height: 1.4;");
        layout->addWidget(m_dragOverlayLabel);
    }

    auto activeFolders = MarkedFolderService::instance().getActiveMarked();
    if (!activeFolders.isEmpty()) {
        QStringList lines;
        lines << QString("迁移到 %1 个标记文件夹").arg(activeFolders.size());
        lines << ""; // 空行打散间距
        for (const auto& folder : activeFolders) {
            QString name = folder.name;
            if (name.isEmpty()) {
                name = QFileInfo(folder.path).fileName();
            }
            if (name.isEmpty()) name = folder.path;
            lines << name;
        }
        m_dragOverlayLabel->setText(lines.join("\n"));
    } else {
        m_dragOverlayLabel->setText("尚未标记目的地");
    }

    m_dragOverlayWidget->setGeometry(rect());
    m_dragOverlayWidget->show();
    m_dragOverlayWidget->raise();
}
>>>>>>> REPLACE

---

### Block 2: `src/ui/FilterPanel.cpp`
<<<<<<< SEARCH
void FilterPanel::showDragOverlay(int activeCount) {
    if (!m_dragOverlayWidget) {
        m_dragOverlayWidget = new QWidget(this);
        m_dragOverlayWidget->setObjectName("DragOverlayWidget");
        m_dragOverlayWidget->setAttribute(Qt::WA_TransparentForMouseEvents);

        QVBoxLayout* layout = new QVBoxLayout(m_dragOverlayWidget);
        layout->setContentsMargins(0, 0, 0, 0);
        m_dragOverlayLabel = new QLabel(m_dragOverlayWidget);
        m_dragOverlayLabel->setAlignment(Qt::AlignCenter);
        m_dragOverlayLabel->setStyleSheet("color: #FFFFFF; font-size: 14px; font-weight: bold;");
        layout->addWidget(m_dragOverlayLabel);
    }

    if (activeCount > 0) {
        m_dragOverlayLabel->setText(QString("迁移到 %1 个标记文件夹").arg(activeCount));
    } else {
        m_dragOverlayLabel->setText("尚未标记目的地");
    }

    m_dragOverlayWidget->setGeometry(rect());
    m_dragOverlayWidget->show();
    m_dragOverlayWidget->raise();
}
=======
void FilterPanel::showDragOverlay(int activeCount) {
    if (!m_dragOverlayWidget) {
        m_dragOverlayWidget = new QWidget(this);
        m_dragOverlayWidget->setObjectName("DragOverlayWidget");
        m_dragOverlayWidget->setAttribute(Qt::WA_TransparentForMouseEvents);

        QVBoxLayout* layout = new QVBoxLayout(m_dragOverlayWidget);
        layout->setContentsMargins(10, 10, 10, 10);
        m_dragOverlayLabel = new QLabel(m_dragOverlayWidget);
        m_dragOverlayLabel->setAlignment(Qt::AlignCenter);
        m_dragOverlayLabel->setStyleSheet("color: #FFFFFF; font-size: 13px; font-weight: bold; line-height: 1.4;");
        layout->addWidget(m_dragOverlayLabel);
    }

    auto activeFolders = MarkedFolderService::instance().getActiveMarked();
    if (!activeFolders.isEmpty()) {
        QStringList lines;
        lines << QString("迁移到 %1 个标记文件夹").arg(activeFolders.size());
        lines << ""; // 空行打散间距
        for (const auto& folder : activeFolders) {
            QString name = folder.name;
            if (name.isEmpty()) {
                name = QFileInfo(folder.path).fileName();
            }
            if (name.isEmpty()) name = folder.path;
            lines << name;
        }
        m_dragOverlayLabel->setText(lines.join("\n"));
    } else {
        m_dragOverlayLabel->setText("尚未标记目的地");
    }

    m_dragOverlayWidget->setGeometry(rect());
    m_dragOverlayWidget->show();
    m_dragOverlayWidget->raise();
}
>>>>>>> REPLACE

---

### Block 3: `src/ui/controllers/ContentContextMenu.cpp`
<<<<<<< SEARCH
                QMenu* moveMenu = menu.addMenu(UiHelper::getIcon("Export-1.svg", QColor("#EEEEEE"), 18), "移动到");
                UiHelper::applyMenuStyle(moveMenu);

                auto performMoveTo = [this](const QString& targetDir) {
=======
                QMenu* moveMenu = menu.addMenu(UiHelper::getIcon("Export-1.svg", QColor("#EEEEEE"), 18), "移动到");
                UiHelper::applyMenuStyle(moveMenu);

                QAction* actDistribute = moveMenu->addAction(UiHelper::getIcon("Destination.svg", QColor("#EEEEEE"), 16), "一键分发 (Alt+V)");
                connect(actDistribute, &QAction::triggered, this, [this]() {
                    QStringList selectedPaths = m_panel->getSelectedPaths();
                    MarkedFolderService::instance().executeTransferToMarked(selectedPaths);
                });
                moveMenu->addSeparator();

                auto performMoveTo = [this](const QString& targetDir) {
>>>>>>> REPLACE

---

## 7. 三道关自检与验证步骤 (Verification)

### 1. 第一道关：静态编译核查
确保修改后无编译语法错误与 Warning。

### 2. 第二道关：自检脚本验证
使用方案自检脚本 `tools/plan_verify/verify_plan.py` 验证本文件的完整性：
```bash
python3 tools/plan_verify/verify_plan.py "Implementation Plan/MarkedFolder_DragOverlay_DistributeMenu_Plan.md"
```

---

## 8. 总结 (Summary)
本实施方案完整涵盖了拖拽遮罩多行文件夹名称展示与右键菜单【一键分发】选项添加的全部实现，符合全软件一致的架构原则与中性色契约，为后续上架合并提供精确标准。
