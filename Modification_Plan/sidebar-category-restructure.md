# 侧边栏分类层级平铺与控制按钮美化 —— sidebar-category-restructure.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
在 ArcMeta 应用的侧边栏中，目前用户自定义创建的分类文件夹在视觉层级上存在一定的缩进，且“文件夹 (N)”组标识的设计较偏向普通的文本，缺乏按钮的质感和职能，这容易给用户带来层级嵌套的误解。
为了让界面更加直观和现代化，需要将所有自定义创建的分类升级为无缩进的一等公民（一级分类），并将“文件夹 (N)”彻底美化为一个拥有独立按钮质感、职能精晰（专用来隐藏或显示自定义文件夹）的专用按钮。
同时，我们需要修复目前隐藏行逻辑的 Bug：目前的隐藏行逻辑会粗暴地将所有 `ID > 0` 的项（包括系统托管库根分类 `arcmeta.library_g`）一并隐藏。本方案将利用 `CategoryKindRole` 角色进行精确过滤，确保按钮仅隐藏和显示“用户自定义创建的分类文件夹”，保持托管库入口不受影响。

## 2. 问题定位
- **模块 1：** `src/ui/CategoryModel.cpp`
  - **位置：** `CategoryModel::refresh()`
  - **原因：** 目前向 QStandardItem 写入属性时，未将 `cat.kind` 写入 Mime 绑定的 Role。我们需要在初始化 Item 时，将 `static_cast<int>(cat.kind)` 写入 `CategoryKindRole` 属性中，以便外部进行精确区分。
- **模块 2：** `src/ui/CategoryPanel.cpp`
  - **位置：** `initUi()` 中 `m_btnFolderGroup` 的 QSS 样式表及按钮构建逻辑
  - **原因：** 原 QSS 样式表过于扁平简陋，缺乏圆角、内边距对齐及美观的悬浮背景。我们需要将其重构，增加符合 Rule 3.4 规范的左内边距（对齐全部数据等顶级树节点的图标位置），增加半透明悬停圆角背景 `#2D2D30`，以及按下背景色 `#3E3E40`，使其具有鲜明的“隐藏/显示切换控制按钮”质感。
- **模块 3：** `src/ui/CategoryPanel.cpp`
  - **位置：** `m_btnFolderGroup` 点击回调函数中的隐藏行逻辑
  - **原因：** 目前直接判定 `IdRole > 0` 会错误地隐藏系统托管库节点（其 ID 也大于 0）。我们应当读取 `CategoryKindRole` 角色，只有当它不是 `SystemLibrary` 时，才执行行隐藏，从而实现高保真的完美双轨物理隔离。
- **模块 4：** `src/ui/CategoryPanel.cpp`
  - **位置：** `updateFolderGroupButtonText()`
  - **原因：** 双态图标和文本的呈现需要微调，确保箭头与图标布局美观，不带假树节点的拖沓感。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 0    | Step 1 中确认的"核心问题"：侧边栏自定义分类的层级重构与隐藏控制按钮优化 | 本方案核心事件名：侧边栏分类层级平铺与控制按钮美化 | ✅ |
| 1    | 将标记为①的主分类““文件夹 (N)” (▼ / ▶)”变成按钮，该按钮是专用来隐藏或显示自定义创建的分类文件夹（对应用户原话：“将标记为①的主分类““文件夹 (N)” (▼ / ▶)”变成按钮，该按钮是专用来隐藏或显示自定义创建的分类文件夹”） | 重新设计并美化 `m_btnFolderGroup` 的 QSS，使之呈现极佳的圆角、悬浮高亮、对齐间距按钮特性。并精确过滤只折叠/隐藏用户自定义分类，不伤及托管库根分类。 | ✅ |
| 2    | 将标记为②二级分类升级为一级分类（一等公民）（对应用户原话：“将标记为②二级分类升级为一级分类（一等公民）”） | 在底层数据结构中，自定义分类节点已完全作为平铺的一级节点挂载在 `root` 根部。由于按钮 `m_btnFolderGroup` 重新设计了 15px 的左侧内边距，且其完全抽离自 QTreeView 的原生缩进层级，树中所有的自定义文件夹会在视觉层级上与系统分类保持完美的左对齐，完全升级为无缩进的一等公民一级分类。 | ✅ |

## 4. 详细解决方案

本部分由执行者 AI 角色直接读取，并严格、机械地按照给出的 Git merge diff 代码块进行物理替换，不得做逆向脑补或二次修改。

### 4.1 修改 `src/ui/CategoryModel.cpp` 存储 `CategoryKindRole` 属性

```
<<<<<<< SEARCH
            item->setData("category", TypeRole);
            item->setData(id, IdRole);
            item->setData(color, ColorRole);
            item->setData(name, NameRole);
            item->setData(cat.pinned, PinnedRole);
            item->setData(cat.encrypted, EncryptedRole);
            item->setData(QString::fromStdWString(cat.encryptHint), EncryptHintRole);
            item->setFlags(item->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
=======
            item->setData("category", TypeRole);
            item->setData(id, IdRole);
            item->setData(color, ColorRole);
            item->setData(name, NameRole);
            item->setData(cat.pinned, PinnedRole);
            item->setData(cat.encrypted, EncryptedRole);
            item->setData(QString::fromStdWString(cat.encryptHint), EncryptHintRole);
            item->setData(static_cast<int>(cat.kind), CategoryKindRole); // 精确记录分类类别，便于隐藏行过滤区分
            item->setFlags(item->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
>>>>>>> REPLACE
```

### 4.2 修改 `src/ui/CategoryPanel.cpp` 中的按钮 QSS 样式重构

```
<<<<<<< SEARCH
    // 1. 构造“文件夹 (N)”专用组按钮
    m_btnFolderGroup = new QPushButton(this);
    m_btnFolderGroup->setFixedHeight(28);
    m_btnFolderGroup->setCursor(Qt::PointingHandCursor);
    m_btnFolderGroup->setStyleSheet(
        "QPushButton { "
        "  background: transparent; "
        "  border: none; "
        "  color: #FFFFFF; "
        "  font-weight: bold; "
        "  font-size: 12px; "
        "  text-align: left; "
        "  padding-left: 4px; "
        "} "
        "QPushButton:hover { background-color: #2A2A2A; border-radius: 4px; }"
    );
=======
    // 1. 构造“文件夹 (N)”专用组按钮（对应用户原话：“将标记为①的主分类““文件夹 (N)” (▼ / ▶)”变成按钮，该按钮是专用来隐藏或显示自定义创建的分类文件夹”）
    m_btnFolderGroup = new QPushButton(this);
    m_btnFolderGroup->setFixedHeight(28);
    m_btnFolderGroup->setCursor(Qt::PointingHandCursor);
    m_btnFolderGroup->setStyleSheet(
        "QPushButton { "
        "  background: transparent; "
        "  border: none; "
        "  color: #EEEEEE; "
        "  font-weight: bold; "
        "  font-size: 12px; "
        "  text-align: left; "
        "  padding-left: 15px; " // 左侧对齐边距：使其图标和文字完美对齐系统分类
        "  margin-right: 5px; "
        "} "
        "QPushButton:hover { background-color: #2D2D30; border-radius: 4px; color: #FFFFFF; }"
        "QPushButton:pressed { background-color: #3E3E40; border-radius: 4px; }"
    );
>>>>>>> REPLACE
```

### 4.3 修改 `src/ui/CategoryPanel.cpp` 隐藏行回调逻辑（过滤系统托管库）

```
<<<<<<< SEARCH
    // 2. 点击按钮：无缝切换下方自定义分类列表的隐藏/显示（折叠/展开）
    connect(m_btnFolderGroup, &QPushButton::clicked, this, [this]() {
        m_isFolderGroupExpanded = !m_isFolderGroupExpanded;

        // 控制 TreeView 中顶级分类节点的展开/收起状态
        if (m_categoryTree && m_proxyModel) {
            for (int i = 0; i < m_proxyModel->rowCount(); ++i) {
                QModelIndex proxyIdx = m_proxyModel->index(i, 0);
                if (proxyIdx.data(IdRole).toInt() > 0) { // 顶级自定义分类
                    m_categoryTree->setRowHidden(i, QModelIndex(), !m_isFolderGroupExpanded);
                }
            }
        }
        // 动态更新箭头图标 (▼ / ▶)
        int count = m_categoryModel ? m_categoryModel->allUserFolderCount() : 0;
        updateFolderGroupButtonText(count);
    });
=======
    // 2. 点击按钮：无缝切换下方自定义分类列表的隐藏/显示（折叠/展开）（对应用户原话：“该按钮是专用来隐藏或显示自定义创建的分类文件夹”）
    connect(m_btnFolderGroup, &QPushButton::clicked, this, [this]() {
        m_isFolderGroupExpanded = !m_isFolderGroupExpanded;

        // 控制 TreeView 中顶级分类节点的展开/收起状态
        if (m_categoryTree && m_proxyModel) {
            for (int i = 0; i < m_proxyModel->rowCount(); ++i) {
                QModelIndex proxyIdx = m_proxyModel->index(i, 0);
                int id = proxyIdx.data(IdRole).toInt();
                int kind = proxyIdx.data(CategoryKindRole).toInt();

                // 精确阻断：仅当是正数数据库 ID 且不是 SystemLibrary 托管库根分类时，才进行隐藏/显示切换
                if (id > 0 && kind != static_cast<int>(CategoryKind::SystemLibrary)) {
                    m_categoryTree->setRowHidden(i, QModelIndex(), !m_isFolderGroupExpanded);
                }
            }
        }
        // 动态更新箭头图标 (▼ / ▶)
        int count = m_categoryModel ? m_categoryModel->allUserFolderCount() : 0;
        updateFolderGroupButtonText(count);
    });
>>>>>>> REPLACE
```

### 4.4 修改 `src/ui/CategoryPanel.cpp` 中的双态文本更新方法

```
<<<<<<< SEARCH
void CategoryPanel::updateFolderGroupButtonText(int count) {
    if (!m_btnFolderGroup) return;
    QString arrow = m_isFolderGroupExpanded ? "▼ " : "▶ ";
    m_btnFolderGroup->setText(QString("%1文件夹 (%2)").arg(arrow).arg(count));
    m_btnFolderGroup->setIcon(UiHelper::getIcon("folder_filled", QColor("#378ADD"), 16));
}
=======
void CategoryPanel::updateFolderGroupButtonText(int count) {
    if (!m_btnFolderGroup) return;
    // 使用统一美观的双态符号（对应用户原话：“(▼ / ▶)”）
    QString arrow = m_isFolderGroupExpanded ? "▼  " : "▶  ";
    m_btnFolderGroup->setText(QString("%1文件夹 (%2)").arg(arrow).arg(count));
    m_btnFolderGroup->setIcon(UiHelper::getIcon("folder_filled", QColor("#378ADD"), 16));
}
>>>>>>> REPLACE
```

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [x] 模块/文件：`src/ui/CategoryModel.cpp`，将分类实体类别 `cat.kind` 绑定存入 `CategoryKindRole` 数据。
- [x] 模块/文件：`src/ui/CategoryPanel.cpp` 的 `initUi()` 阶段关于 `m_btnFolderGroup` 控件样式与文字初始化逻辑。
- [x] 模块/文件：`src/ui/CategoryPanel.cpp` 隐藏行回调逻辑（过滤系统托管库 `SystemLibrary`）。
- [x] 模块/文件：`src/ui/CategoryPanel.cpp` 的 `updateFolderGroupButtonText(int)` 刷新机制。

**明确禁止越界修改的范围：**
- [x] 树视图中其他系统桶、快速访问节点刷新及拖拽响应——不修改。
- [x] 分类持久化（`CategoryRepo.cpp`）及数据库底层操作——不修改。

## 6. 实现准则与预警【核心】
- **完美对齐与无警告编译**：
  在应用此 QSS 变更时，必须确保不凭空捏造任何成员变量或私有静态锁，严格尊重已声明的 `m_btnFolderGroup`、`m_isFolderGroupExpanded`、`m_categoryTree` 及 `m_proxyModel` 变量生命周期。
  修改仅在 `CategoryPanel.cpp` 内安全、独立、不污染其他面板执行。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 输入框清除功能 | 所有可编辑的输入框一键清除，一律且仅允许采用 Qt 原生的 setClearButtonEnabled(true)，不涉及本方案 | ✅ |
| 窗口置顶 | 窗口置顶状态一律使用 Win32 原生 SetWindowPos 并搭配 SWP_NOSENDCHANGING 标志，不涉及本方案 | ✅ |
| 标题栏悬停与按下色值 | Hover 状态背景色 #3E3E42（Style::HoverBackground），Pressed 状态 #4E4E52（Style::PressedBackground），不涉及本方案 | ✅ |

## 8. 待确认事项（可选）
暂无。
