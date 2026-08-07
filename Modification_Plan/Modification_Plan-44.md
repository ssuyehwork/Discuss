# 恢复侧边栏“分类”外壳容器与手动分类计数 —— Modification_Plan-44.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
本方案承接自 `Modification_Plan-43.md`，因用户提出补充要求：
- 将原本设计的“我的分类”名称替换为简练的“分类”；
- “分类”顶级容器分组上需要显示当前的计数，且该计数统计的是所有手动创建的分类数量（即非托管库 `ArcMeta.Library_` 及其下属项的自定义分类的总个数）。
旧方案 `Modification_Plan-43.md` 永久作为历史只读保留，所有的技术演进与施工方案在本篇 `Modification_Plan-44.md` 中进行完整细化与最终规范，确保功能时序高内聚，数据流纯净不污染数据库。

---

## 2. 问题定位
- 模型构建端 `CategoryModel::refresh()` 需要在构建“分类”根节点容器前，先遍历 `categories` 列表。
- 通过深度回溯父节点判定各分类是否隶属于顶级托管库 `ArcMeta.Library_`（名称以该前缀开头且 parentId == 0）。不隶属托管库的所有分类计入 `userTotalCount`。
- 构建 `QStandardItem("分类")` 时，使用 `QString("分类 (%1)").arg(userTotalCount)` 动态渲染标题，绑定 `NameRole = "分类"`。
- 在 `CategoryPanel.cpp` 以及 `CategoryFilterProxyModel.h` 中，将所有原定对 `"我的分类"` 的硬编码匹配，物理升级对齐为对 `"分类"` 的匹配保护。

---

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
| :---: | --- | --- | :---: |
| 1 | 我期望将侧边栏分类“我的分类”从旧版本恢复到当前版本的侧边栏分类 | 恢复顶级容器分组并在 `refresh` 重新收拢用户分类挂载。 | ✅ |
| 2 | 将“我的分类”替换成“分类” | 全局采用 `"分类"` 字符串进行 NameRole 匹配和容器重置。 | ✅ |
| 3 | 分类上显示的计数计的是手动创建的分类数量 | 计算所有非托管库属性的自定义分类总数，动态刷新到分类标题上。 | ✅ |

---

## 4. 详细解决方案

本部分由执行者 AI 角色直接读取，并严格、机械地按照给出的 Git merge diff 代码块进行物理替换，不得做任何自由发挥或脑补改动。

### 4.1 修改：`src/ui/CategoryModel.cpp`
在 `refresh` 函数中，先构建物理分类缓存映射并统计所有非托管库归属（手动创建）的分类总数量 `userTotalCount`，然后用 `分类 (数量)` 作为节点名称创建 userGroup 顶级项。并在 `dropMimeData` 校验中放行对该节点的拖入动作。

```merge-diff
<<<<<<< SEARCH
    QStandardItem* favGroup = nullptr;
    if (m_type == Both || m_type == User) {
        favGroup = new QStandardItem("快速访问");
        favGroup->setData("快速访问", NameRole);
        favGroup->setSelectable(false);
        favGroup->setEditable(false);
        favGroup->setIcon(UiHelper::getIcon("folder_filled", QColor("#FFFFFF"), 16));

        QFont font = favGroup->font();
        font.setBold(true);
        favGroup->setFont(font);
        favGroup->setForeground(QColor("#FFFFFF"));
    }

    if (m_type == User || m_type == Both) {
        auto categories = CategoryRepo::getAll();
        QMap<int, QStandardItem*> itemMap;
        QMap<int, Category> catMap;

        for (const auto& cat : categories) {
=======
    QStandardItem* favGroup = nullptr;
    if (m_type == Both || m_type == User) {
        favGroup = new QStandardItem("快速访问");
        favGroup->setData("快速访问", NameRole);
        favGroup->setSelectable(false);
        favGroup->setEditable(false);
        favGroup->setIcon(UiHelper::getIcon("folder_filled", QColor("#FFFFFF"), 16));

        QFont font = favGroup->font();
        font.setBold(true);
        favGroup->setFont(font);
        favGroup->setForeground(QColor("#FFFFFF"));
    }

    QStandardItem* userGroup = nullptr;
    if (m_type == User || m_type == Both) {
        auto categories = CategoryRepo::getAll();
        QMap<int, QStandardItem*> itemMap;
        QMap<int, Category> catMap;

        // 1. 建立基础 ID 到 Category 的快速寻溯缓存
        for (const auto& cat : categories) {
            catMap[cat.id] = cat;
        }

        // 2. 统计手动创建的自定义分类总数 (排除托管库及其下级的所有分类)
        int userTotalCount = 0;
        for (const auto& cat : categories) {
            int tempId = cat.id;
            bool isLibrary = false;
            while (tempId > 0) {
                if (catMap.contains(tempId)) {
                    Category currentCat = catMap[tempId];
                    if (currentCat.id == 0) break;
                    if (currentCat.parentId == 0) {
                        if (QString::fromStdWString(currentCat.name).startsWith("ArcMeta.Library_", Qt::CaseInsensitive)) {
                            isLibrary = true;
                        }
                        break;
                    }
                    tempId = currentCat.parentId;
                } else {
                    break;
                }
            }
            if (!isLibrary) {
                userTotalCount++;
            }
        }

        // 3. 构建“分类”根节点容器，动态注入计算好的手动分类数量
        userGroup = new QStandardItem(QString("分类 (%1)").arg(userTotalCount));
        userGroup->setData("分类", NameRole);
        userGroup->setSelectable(false);
        userGroup->setEditable(false);
        userGroup->setFlags(userGroup->flags() | Qt::ItemIsDropEnabled);
        userGroup->setIcon(UiHelper::getIcon("folder_filled", QColor("#FFFFFF"), 16));

        QFont font = userGroup->font();
        font.setBold(true);
        userGroup->setFont(font);
        userGroup->setForeground(QColor("#FFFFFF"));

        for (const auto& cat : categories) {
>>>>>>> REPLACE
```

```merge-diff
<<<<<<< SEARCH
        // 1. 优先渲染托管库根分类 (parentId == 0 && ArcMeta.Library_) 到 root 中间位置
        for (const auto& cat : categories) {
            int id = cat.id;
            QStandardItem* item = itemMap[id];
            int parentId = cat.parentId;

            if (parentId == 0) {
                QString name = QString::fromStdWString(cat.name);
                if (name.startsWith("ArcMeta.Library_", Qt::CaseInsensitive)) {
                    root->appendRow(item);
                }
            } else if (parentId > 0 && itemMap.contains(parentId)) {
                itemMap[parentId]->appendRow(item);
            }
        }

        // 2. 渲染“快速访问”分组节点
        if (favGroup) {
            root->appendRow(favGroup);
        }

        // 3. 渲染用户自定义分类树 (将非 ArcMeta.Library_ 的顶级自定义分类作为“快速访问”的子树展示)
        for (const auto& cat : categories) {
            int id = cat.id;
            QStandardItem* item = itemMap[id];
            int parentId = cat.parentId;

            if (parentId == 0) {
                QString name = QString::fromStdWString(cat.name);
                if (!name.startsWith("ArcMeta.Library_", Qt::CaseInsensitive)) {
                    if (favGroup) {
                        favGroup->appendRow(item);
                    } else {
                        root->appendRow(item);
                    }
                }
            }
        }
=======
        if (userGroup) {
            root->appendRow(userGroup);
        }

        // 1. 优先渲染托管库根分类 (parentId == 0 && ArcMeta.Library_) 到 root 中间位置
        for (const auto& cat : categories) {
            int id = cat.id;
            QStandardItem* item = itemMap[id];
            int parentId = cat.parentId;

            if (parentId > 0 && itemMap.contains(parentId)) {
                itemMap[parentId]->appendRow(item);
            } else {
                if (QString::fromStdWString(cat.name).startsWith("ArcMeta.Library_", Qt::CaseInsensitive)) {
                    root->appendRow(item);
                } else if (userGroup) {
                    userGroup->appendRow(item);
                }
            }
        }

        // 2. 渲染“快速访问”分组节点
        if (favGroup) {
            root->appendRow(favGroup);
        }
>>>>>>> REPLACE
```

```merge-diff
<<<<<<< SEARCH
bool CategoryModel::dropMimeData(const QMimeData* mimeData, Qt::DropAction action, int row, int column, const QModelIndex& parent) {
    if (mimeData->hasUrls() || mimeData->hasFormat("text/plain")) {
        return true;
    }

    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(column);

    QModelIndex actualParent = parent;
    if (actualParent.isValid()) {
        QStandardItem* parentItem = itemFromIndex(actualParent);
        if (!parentItem) return false;

        QString type = parentItem->data(TypeRole).toString();
        QString name = parentItem->data(NameRole).toString();

        if (type != "category" && type != "bookmark") {
            return false;
        }

        // 🚨 阻止拖拽子分类嵌套入托管库根分类 (parentId == 0 && !physicalPath.empty())
=======
bool CategoryModel::dropMimeData(const QMimeData* mimeData, Qt::DropAction action, int row, int column, const QModelIndex& parent) {
    if (mimeData->hasUrls() || mimeData->hasFormat("text/plain")) {
        return true;
    }

    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(column);

    QModelIndex actualParent = parent;
    if (actualParent.isValid()) {
        QStandardItem* parentItem = itemFromIndex(actualParent);
        if (!parentItem) return false;

        QString type = parentItem->data(TypeRole).toString();
        QString name = parentItem->data(NameRole).toString();

        if (type != "category" && type != "bookmark" && name != "分类") {
            return false;
        }

        // 🚨 阻止拖拽子分类嵌套入托管库根分类 (parentId == 0 && !physicalPath.empty())
>>>>>>> REPLACE
```

---

### 4.2 修改：`src/ui/CategoryPanel.cpp`
调整 contextMenu 弹出时的选中排除逻辑，并修改 `rowsMoved` 数据同步中关于“分类”子树内部节点的数据库更新映射，实现仅对“分类”子树下的变动回写 parentId，并在空白区域或“分类”根分组上右键时支持创建自定义顶级分类（归入“分类”）。

```merge-diff
<<<<<<< SEARCH
        // 基于规范逻辑：如果没有选中项
        QString itemName = index.data(NameRole).toString();
        QString itemType = index.data(TypeRole).toString();

        if (itemType == "trash") {
            // 2026-06-xx 物理级 1:1 还原：回收站专属右键菜单
            menu.addAction(UiHelper::getIcon("trash", ErrorRed, 18), "清空回收站", this, &CategoryPanel::onEmptyTrash);
            menu.addAction(UiHelper::getIcon("sync", PrimaryBlue, 18), "还原全部项目", this, &CategoryPanel::onRestoreAllFromTrash);
        } else if (!index.isValid()) {
            menu.addAction(UiHelper::getIcon("folder_filled", QColor("#aaaaaa"), 18), "新建分类", this, &CategoryPanel::onCreateCategory);

            auto* sortMenu = menu.addMenu(UiHelper::getIcon("list_ul", QColor("#aaaaaa"), 18), "排列");
=======
        // 基于规范逻辑：如果没有选中项，或者选中了“分类”根节点
        QString itemName = index.data(NameRole).toString();
        QString itemType = index.data(TypeRole).toString();

        if (itemType == "trash") {
            // 2026-06-xx 物理级 1:1 还原：回收站专属右键菜单
            menu.addAction(UiHelper::getIcon("trash", ErrorRed, 18), "清空回收站", this, &CategoryPanel::onEmptyTrash);
            menu.addAction(UiHelper::getIcon("sync", PrimaryBlue, 18), "还原全部项目", this, &CategoryPanel::onRestoreAllFromTrash);
        } else if (!index.isValid() || itemName == "分类") {
            menu.addAction(UiHelper::getIcon("folder_filled", QColor("#aaaaaa"), 18), "新建分类", this, &CategoryPanel::onCreateCategory);

            auto* sortMenu = menu.addMenu(UiHelper::getIcon("list_ul", QColor("#aaaaaa"), 18), "排列");
>>>>>>> REPLACE
```

```merge-diff
<<<<<<< SEARCH
    // 2026-03-xx 物理记忆：连接展开/折叠信号，实时持久化
    connect(m_categoryTree, &QTreeView::expanded, this, &CategoryPanel::saveExpandedStateToSettings);
    connect(m_categoryTree, &QTreeView::collapsed, this, &CategoryPanel::saveExpandedStateToSettings);
    // 2026-06-xx 物理同步：支持内部拖拽重排持久化
    connect(m_categoryModel, &QAbstractItemModel::rowsMoved, this, [this](const QModelIndex&, int, int, const QModelIndex&, int) {
        // 核心逻辑：深度优先遍历分类树，根据 UI 层级物理同步 DB 中的 parent_id 与 sort_order
        std::function<void(const QModelIndex&, int)> syncSubtree;
        syncSubtree = [&](const QModelIndex& parentIdx, int parentIdInDb) {
            for (int i = 0; i < m_categoryModel->rowCount(parentIdx); ++i) {
                QModelIndex childIdx = m_categoryModel->index(i, 0, parentIdx);
                int id = childIdx.data(IdRole).toInt();
                QString type = childIdx.data(TypeRole).toString();
                bool isPinned = childIdx.data(PinnedRole).toBool();

                // 物理阻断：严禁处理“镜像节点”（即 Pinned 为 true 的节点）。
                // 理由：镜像节点仅作为 UI 快捷方式，其移动不应改写原始数据库中的 parentId 关系。
                if (isPinned) {
                    continue;
                }

                if (type == "category" && id > 0) {
                    int actualParentId = parentIdx.isValid() ? parentIdInDb : 0;
                    // 只有在数据真正发生位移时才触发数据库 UPDATE，优化性能
                    auto all = CategoryRepo::getAll();
                    for (auto& cat : all) {
                        if (cat.id == id) {
                            if (cat.parentId != actualParentId || cat.sortOrder != i) {
                                cat.parentId = actualParentId;
                                cat.sortOrder = i;
                                CategoryRepo::update(cat);
                            }
                            break;
                        }
                    }
                    // 递归同步子分类
                    syncSubtree(childIdx, id);
                }
            }
        };
        syncSubtree(QModelIndex(), 0); // 从隐式根开始，0 表示顶层
    });
=======
    // 2026-03-xx 物理记忆：连接展开/折叠信号，实时持久化
    connect(m_categoryTree, &QTreeView::expanded, this, &CategoryPanel::saveExpandedStateToSettings);
    connect(m_categoryTree, &QTreeView::collapsed, this, &CategoryPanel::saveExpandedStateToSettings);
    // 2026-06-xx 物理同步：支持内部拖拽重排持久化
    connect(m_categoryModel, &QAbstractItemModel::rowsMoved, this, [this](const QModelIndex&, int, int, const QModelIndex&, int) {
        // 核心逻辑：深度优先遍历“分类”子树，根据 UI 层级物理同步 DB 中的 parent_id 与 sort_order
        std::function<void(const QModelIndex&, int)> syncSubtree;
        syncSubtree = [&](const QModelIndex& parentIdx, int parentIdInDb) {
            for (int i = 0; i < m_categoryModel->rowCount(parentIdx); ++i) {
                QModelIndex childIdx = m_categoryModel->index(i, 0, parentIdx);
                int id = childIdx.data(IdRole).toInt();
                QString type = childIdx.data(TypeRole).toString();
                bool isPinned = childIdx.data(PinnedRole).toBool();

                // 物理阻断：严禁处理“镜像节点”（即 Pinned 为 true 且其父项不是“分类”的节点）。
                // 理由：镜像节点仅作为 UI 快捷方式，其移动不应改写原始数据库中的 parentId 关系。
                if (parentIdInDb != -1 && isPinned && parentIdx.data(NameRole).toString() != "分类" && parentIdx.data(TypeRole).toString() != "category") {
                    continue;
                }

                if (type == "category" && id > 0) {
                    // 只有在数据真正发生位移时才触发数据库 UPDATE，优化性能
                    auto all = CategoryRepo::getAll();
                    for (auto& cat : all) {
                        if (cat.id == id) {
                            if (cat.parentId != parentIdInDb || cat.sortOrder != i) {
                                cat.parentId = parentIdInDb;
                                cat.sortOrder = i;
                                CategoryRepo::update(cat);
                            }
                            break;
                        }
                    }
                    // 递归同步子分类
                    syncSubtree(childIdx, id);
                } else if (childIdx.data(NameRole).toString() == "分类") {
                    // 进入“分类”根容器
                    syncSubtree(childIdx, 0);
                }
            }
        };
        syncSubtree(QModelIndex(), -1); // 从隐式根开始，-1 表示尚未进入有效分类区
    });
>>>>>>> REPLACE
```

---

### 4.3 修改：`src/ui/CategoryFilterProxyModel.h`
对递归过滤代理模型进行适配，确保当用户在“筛选分类...”输入框搜索时，“分类”和“快速访问”这两个根容器节点能够根据其子项的匹配状态正常地展开/收起和显示。

```merge-diff
<<<<<<< SEARCH
        // 2. 根容器处理
        if (name == "快速访问") {
            return hasMatchingChild(index);
        }
=======
        // 2. 根容器处理
        if (name == "快速访问" || name == "分类") {
            return hasMatchingChild(index);
        }
>>>>>>> REPLACE
```

---

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] `src/ui/CategoryModel.cpp`：添加 `userGroup` 作为“分类”创建并动态计算手动分类计数 `userTotalCount`，并在挂载和拖放逻辑中对齐使用 `"分类"`。
- [ ] `src/ui/CategoryPanel.cpp`：右键菜单、拖拽 rowsMoved 等同步中对齐使用 `"分类"` 匹配。
- [ ] `src/ui/CategoryFilterProxyModel.h`：增加 `filterAcceptsRow` 中针对 `"分类"` 的匹配过滤支持。

**明确禁止越界修改的范围：**
- [ ] 数据库驱动层 `CategoryRepo` 文件操作——不修改
- [ ] 磁盘物理导航面板 `NavPanel` 架构——不修改

---

## 6. 实现准则与预警【核心】
1. **精准计数安全**：在统计手动创建的分类数量时，必须回溯判断其顶层祖先是否包含托管库（`ArcMeta.Library_` 前缀）。排除了托管库链条的所有自定义分类才计入 `userTotalCount`，确保计数百分之百精准。
2. **避免多线程冲突**：由于分类统计是在模型中触发，必须确保获取和过滤逻辑使用的是同一快照（来自 `CategoryRepo::getAll()`），避免在加载或并发写入时发生死锁或野指针。
3. **过滤支持对齐**：注意 `CategoryFilterProxyModel` 的 `NameRole` 会匹配到 `"分类 (%1)"`，为了匹配保护，应该使用 `childIdx.data(NameRole).toString() == "分类"` 这种纯粹的 Role 做匹配。因为 `NameRole` 我们已经将其设为了 `"分类"`，所以这一层判定是完美无误、强壮安全的。

---

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 输入框清除功能 | 一律使用 Qt 原生 `setClearButtonEnabled(true)`。严禁通过 Action 或自定义按钮模拟。本方案未新增输入框。 | ✅ 符合 |
| 双轨模式物理隔离 | 托管分类模式与磁盘导航模式运行时各自独立。磁盘导航模式下的设色等任何写操作绝不向 SQLite 发生溢流和倒灌。本方案涉及分类层级调整，100% 保持在托管数据库分类模式下处理。 | ✅ 符合 |
| 统一数据来源判断 | 跨模块调用一律复用 `isMirrorSource()` 判别当前视图数据是来源于“侧边栏分类模式”还是“目录导航模式”。本方案未修改该判定接口。 | ✅ 符合 |

---

## 8. 待确认事项（可选）
暂无。
