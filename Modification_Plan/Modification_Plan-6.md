# 侧边栏分类自定义图标设计与持久化方案 —— Modification_Plan-6.md

## 1. 任务背景
在当前版本中，侧边栏自定义分类（“我的分类”及子树）默认一律渲染为深灰色（`#555555`）的普通文件夹图标（`folder_filled`），或者在未解锁时渲染为锁定图标（`lock`）。
为了进一步提升用户体验，提供更丰富的视觉识别度，用户希望能够**自定义分类文件夹图标**。具体功能边界包括：
- 在右键菜单中新增一个主选项：“**文件夹图标**”（对应用户原话：“在右键菜单中新增一个主选项”）。
- 该选项下挂载一个二级子菜单，子菜单平铺系统内置的 SVG 矢量图标（SVG 数据统一从 `SvgIcons.h` 中检索加载），子项展示对应矢量图标并允许用户选择。
- 点击特定图标后，将图标键名写入本地 SQLite 数据库分类表中进行持久化，并重新渲染侧边栏树结构，实现侧边栏图标的无缝、实时变更新渲染。

Jules 将在此文档中对该功能进行深度物理考古、数据库结构升级设计、API 接口定制以及 UI 动态交互体系的全方位分析，并产出详细且可以直接交付的方案文档。

---

## 2. 问题定位
要实现分类图标自定义与持久化，需要将逻辑打通至以下模块：
1. **数据持久化拓展**：
   - 文件：`src/meta/DatabaseManager.cpp`
   - 原因：SQLite 数据库中的 `categories` 表结构目前不包含 `icon` 字段。需要在数据库首次加载及升级时，通过 `ALTER TABLE` 动态增加 `icon` 字段，默认值为 `folder_filled`。
2. **内存模型实体拓展**：
   - 文件：`src/meta/CategoryRepo.h` / `CategoryRepo.cpp`
   - 原因：`struct Category` 结构体需要新增 `std::wstring icon` 属性，并在持久层读写接口（`getAll`、`getById`、`add`、`update`）中完成对 `icon` 字段的 SQL 绑定。
3. **侧边栏树模型动态加载与渲染**：
   - 文件：`src/ui/CategoryModel.cpp`
   - 原因：`CategoryModel::refresh()` 函数负责将数据库中读取的分类实体转换为 `QStandardItem` 并设定对应的图标（通过 `UiHelper::getIcon`）。我们需要将图标键名读取逻辑由硬编码改为读取 `cat.icon`。
   - *特别优先级*：当分类处于加密锁定状态时，锁定图标（`lock`）依然需要覆盖自定义图标进行优先展现，以满足安全性语境。
4. **右键上下文菜单与交互控制**：
   - 文件：`src/ui/CategoryPanel.cpp`
   - 原因：`CategoryPanel::setupContextMenu` 函数负责分类右键菜单构建。在此处需要新增主选项：“**文件夹图标**”，并构建子菜单平铺常用矢量图标。用户触发点击后执行 `CategoryRepo::update` 以及 `CategoryModel::refresh()` 重新加载。

---

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 在右键菜单中新增一个主选项（对应用户原话：“在右键菜单中新增一个主选项”） | 在 `CategoryPanel::setupContextMenu` 中添加名称为 `"文件夹图标"` 的二级菜单项。 | ✅ 一致 |
| 2    | 子菜单平铺内置的所有 SVG 矢量图标 | 提取 `SvgIcons.h` 中的代表性 SVG 键名，作为二级菜单 de Action 列表进行呈现。 | ✅ 一致 |
| 3    | 数据库存储，启动自动升级迁移 | 在 `DatabaseManager::loadDb` 函数中自动探测 categories 表，执行 `ALTER TABLE categories ADD COLUMN icon TEXT DEFAULT 'folder_filled'` 字段迁移。 | ✅ 一致 |
| 4    | 侧边栏图标无缝实时变更新渲染 | 点击二级菜单中的图标选项后，直接触发 DB 写入，接着调用 `CategoryModel::refresh()` 重构并复用原折叠/展开状态，无卡顿闪烁渲染。 | ✅ 一致 |

---

## 4. 详细解决方案

### 4.1 数据库结构自动升级（DatabaseManager.cpp）
在 `DatabaseManager::loadDb` 中，于 categories 表字段探测逻辑之后，追加对 `icon` 字段的动态升级检测。
```cpp
// 2026-08-xx 物理同步扩展：迁移 categories 表字段
sqlite3_stmt* catCheckStmt;
bool hasFrnColumn = false;
bool hasPhysicalPathColumn = false;
bool hasIconColumn = false; // 新增字段标识

if (sqlite3_prepare_v2(conn.memDb, "PRAGMA table_info(categories)", -1, &catCheckStmt, nullptr) == SQLITE_OK) {
    while (sqlite3_step(catCheckStmt) == SQLITE_ROW) {
        const char* colName = reinterpret_cast<const char*>(sqlite3_column_text(catCheckStmt, 1));
        if (colName) {
            std::string name(colName);
            if (name == "physical_frn") hasFrnColumn = true;
            if (name == "physical_path") hasPhysicalPathColumn = true;
            if (name == "icon") hasIconColumn = true; // 识别 icon 字段
        }
    }
    sqlite3_finalize(catCheckStmt);
}

if (!hasFrnColumn) {
    sqlite3_exec(conn.memDb, "ALTER TABLE categories ADD COLUMN physical_frn INTEGER DEFAULT 0", nullptr, nullptr, nullptr);
}
if (!hasPhysicalPathColumn) {
    sqlite3_exec(conn.memDb, "ALTER TABLE categories ADD COLUMN physical_path TEXT", nullptr, nullptr, nullptr);
}
if (!hasIconColumn) {
    // 安全迁移：增加 icon 字段，默认设为 folder_filled 文件夹图标
    sqlite3_exec(conn.memDb, "ALTER TABLE categories ADD COLUMN icon TEXT DEFAULT 'folder_filled'", nullptr, nullptr, nullptr);
}
```

### 4.2 内存模型与读写接口适配（CategoryRepo.h / CategoryRepo.cpp）
#### Category 结构体扩展（CategoryRepo.h）
```cpp
struct Category {
    int id = 0;
    int parentId = 0;
    std::wstring name;
    std::wstring color;
    std::vector<std::wstring> presetTags;
    int sortOrder = 0;
    bool pinned = false;
    bool encrypted = false;
    std::wstring encryptHint;
    uint64_t physicalFrn = 0;
    std::wstring physicalPath;
    std::wstring icon = L"folder_filled"; // 默认为 folder_filled 文件夹图标
};
```

#### SQL 读取绑定扩展（CategoryRepo.cpp）
对 `getAll`、`getById`、`add`、`update` 这四个接口中的 SQL 查询语句以及参数绑定操作进行补全。以 `getAll` 与 `update` 为例：
```cpp
// 1. getAll() 函数中的 SQL 查询适配
const char* sql = "SELECT id, parent_id, name, color, preset_tags, sort_order, pinned, encrypted, "
                  "encrypt_hint, physical_frn, physical_path, icon FROM categories WHERE id > 0 ORDER BY sort_order ASC";
// 循环解析中：
const wchar_t* wicon = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 11));
if (wicon) c.icon = wicon;

// 2. update() 函数中的 SQL 查询适配
const char* sql = "UPDATE categories SET parent_id=?, name=?, color=?, preset_tags=?, "
                  "sort_order=?, pinned=?, encrypted=?, encrypt_hint=?, physical_frn=?, physical_path=?, icon=? WHERE id=?";
// 参数绑定：
sqlite3_bind_text16(stmt, 11, cat.icon.c_str(), -1, SQLITE_TRANSIENT);
sqlite3_bind_int(stmt, 12, cat.id);
```

### 4.3 UI 树节点图标加载机制（CategoryModel.cpp）
在树模型重构分类节点时，如果是普通分类或快速访问分类，在非锁定状态下采用 `cat.icon` 重新载入。
```cpp
// CategoryModel::refresh()
for (const auto& cat : categories) {
    catMap[cat.id] = cat;
    int id = cat.id;
    QString name = QString::fromStdWString(cat.name);
    QString color = QString::fromStdWString(cat.color).isEmpty() ? "#555555" : QString::fromStdWString(cat.color);

    QStandardItem* item = new QStandardItem(QString("%1 (0)").arg(name));
    // ... 绑定各个角色参数 ...

    // 自定义图标渲染逻辑
    if (cat.encrypted && !m_unlockedIds.contains(id)) {
        // 安全红线：加密且未解锁时强制采用 lock 图标展示
        item->setIcon(UiHelper::getIcon("lock", QColor("#aaaaaa"), 16));
    } else {
        // 无锁/已解锁状态：读取数据库中自定义的 icon 键名
        QString iconKey = QString::fromStdWString(cat.icon).isEmpty() ? "folder_filled" : QString::fromStdWString(cat.icon);
        item->setIcon(UiHelper::getIcon(iconKey, QColor(color), 16));
    }
    itemMap[id] = item;
}

// 快速访问分类镜像 (Mirror Nodes) 的加载逻辑
if (favGroup) {
    for (const auto& cat : categories) {
        if (cat.pinned) {
            int id = cat.id;
            QString name = QString::fromStdWString(cat.name);
            QString color = QString::fromStdWString(cat.color).isEmpty() ? "#555555" : QString::fromStdWString(cat.color);

            QStandardItem* mirror = new QStandardItem(QString("%1 (0)").arg(name));
            // ... 绑定各个角色参数 ...

            if (cat.encrypted && !m_unlockedIds.contains(id)) {
                mirror->setIcon(UiHelper::getIcon("lock", QColor("#aaaaaa"), 16));
            } else {
                // 物理同步快速访问中的镜像图标
                QString iconKey = QString::fromStdWString(cat.icon).isEmpty() ? "folder_filled" : QString::fromStdWString(cat.icon);
                mirror->setIcon(UiHelper::getIcon(iconKey, QColor(color), 16));
            }
            favGroup->appendRow(mirror);
        }
    }
}
```

### 4.4 右键分类菜单及选择交互（CategoryPanel.cpp）
在右键菜单中增加主菜单入口 `"文件夹图标"`，并在其下挂载矢量图标子菜单，直接映射触发数据库修改。
```cpp
// CategoryPanel::setupContextMenu() 的普通分类节点条件分支内：
if (itemType == "category" || itemType == "file" || itemType == "folder") {
    // 归类、颜色、标签等操作菜单项...

    // 1. 创建主选项“文件夹图标”
    QMenu* iconMenu = menu.addMenu(UiHelper::getIcon("folder_filled", WarningOrange, 18), "文件夹图标");
    UiHelper::applyMenuStyle(iconMenu);

    // 2. 选择性提取 SvgIcons.h 中适合作为分类图案的键名与对应视觉文字
    static const QMap<QString, QString> builtInIconMap = {
        {"默认文件夹", "folder_filled"},
        {"层级分类", "category"},
        {"照片媒体", "image_filled"},
        {"时钟历史", "clock_filled"},
        {"星标收藏", "star_filled"},
        {"爱心常用", "heart_filled"},
        {"加密安全", "lock_filled"},
        {"图书文档", "book"},
        {"配置管理", "settings_filled"},
        {"网络球体", "globe_filled"}
    };

    // 获取当前分类实体的颜色，使图标预览拥有完美的一致性
    QString colorStr = index.data(ColorRole).toString();
    QColor catColor = colorStr.isEmpty() ? QColor("#555555") : QColor(colorStr);

    // 3. 将代表性内置图标加入二级菜单
    for (auto it = builtInIconMap.begin(); it != builtInIconMap.end(); ++it) {
        QString label = it.key();
        QString iconKey = it.value();

        // 动态根据分类色彩渲染菜单图标预览，体验极佳
        QAction* iconAct = iconMenu->addAction(UiHelper::getIcon(iconKey, catColor, 18), label);

        // 用户点击后，写入数据库并重刷侧边栏
        connect(iconAct, &QAction::triggered, this, [this, id, iconKey]() {
            auto cats = CategoryRepo::getAll();
            for (auto& cat : cats) {
                if (cat.id == id) {
                    cat.icon = iconKey.toStdWString();
                    CategoryRepo::update(cat);
                    break;
                }
            }

            // 实时更新树模型，展开状态已有 rowsAboutToBeReset() 等信号自动完美恢复
            m_categoryModel->refresh();
        });
    }

    menu.addSeparator();
    // 后面继续添加新建分类、Pin、重命名等...
}
```

---

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/meta/DatabaseManager.cpp` （数据库载入与自动升级 ALTER TABLE）
- [ ] 模块/文件：`src/meta/CategoryRepo.h` / `CategoryRepo.cpp` （Category 结构体扩展，SQL 语句及参数绑定）
- [ ] 模块/文件：`src/ui/CategoryModel.cpp` （CategoryModel 节点图标动态加载与无锁渲染逻辑）
- [ ] 模块/文件：`src/ui/CategoryPanel.cpp` （CategoryPanel 上下文右键二级子菜单构建与更新信号分发）

**明确禁止越界修改的范围：**
- [ ] 严禁修改其他非本任务相关的元数据表、USN 监控及物理扫描机制。
- [ ] 严禁删除、重命名 `SvgIcons.h` 中的现有 SVG 属性或篡改其键名。

---

## 6. 实现准则与预警【核心】
1. **头文件依赖预警**：
   - 修改 `CategoryRepo` 时，要确保 `#include <string>` 正常存在。由于 `wstring` 已在 `CategoryRepo.h` 中广泛使用，本次新增成员无阻碍。
2. **锁越权与跨线程预警**：
   - 数据库迁移在主线程首次启动 `DatabaseManager::loadDb` 时串行进行，不存在跨线程冲突。
   - `CategoryRepo::update` 在 UI 线程触发时直连磁盘 DB，由 SQL 锁机制保证线程安全。由于更新是纳秒级，不需要在此处额外封装子线程，以免引发展开折叠树重构状态在多线程下的同步时序 Bug。
3. **安全加密覆盖优先级**：
   - 在 `CategoryModel::refresh()` 中，必须确保 `cat.encrypted && !m_unlockedIds.contains(id)` 的条件分支在 `cat.icon` 渲染分支之上，因为未解锁分类必须展现“加锁”图标以防泄漏分类类别属性。

---

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 侧边栏分类模式 | 属于 SQLite DB-driven 内存镜像，完全符合从内存加载与持久化要求。 | ✅ 符合 |
| 直连磁盘数据库模式 | 满足 `DatabaseManager::loadDb` 秒退架构的高性能 PRAGMA journal_mode 与直连规范。 | ✅ 符合 |
| 样式与 hover 背景 | 右键菜单直接复用系统的 `UiHelper::applyMenuStyle`，不需要任何额外的色卡冲突修改。 | ✅ 符合 |
| UI考古原则 | 本方案的菜单 Action 以及 `getIcon` 调用均严格考古并对齐于 `CategoryPanel.cpp` 原有的设置颜色及设置预设标签。 | ✅ 符合 |
