# 侧边栏分类及盘符栏自定义图标与外观设计方案 —— Modification_Plan-6.md

## 1. 任务背景
在当前版本中：
- 侧边栏自定义分类（“我的分类”及子树）默认一律渲染为深灰色（`#555555`）的普通文件夹图标（`folder_filled`），或者在未解锁时渲染为锁定图标（`lock`）。
- 顶部盘符栏的自定义监控文件夹按钮（`FolderButton`）右键菜单仅有 “新建自动导入” 与 “移除”，不支持颜色、随机颜色和自定义图标设定（对应用户原话：“期望的是图中箭头指向的这个右键菜单也有“设置颜色”和“随机颜色”这三个选项”）。

为了进一步打通系统的视觉配置个性化能力，并保持顶部盘符栏与侧边栏的交互一致性，用户希望：
1. **侧边栏分类**：右键菜单新增主选项：“**文件夹图标**”（对应用户原话），支持平铺选择内置的所有 SVG 矢量图标并持久化。
2. **盘符栏自定义监控目录**：其右键菜单也平移对齐，同样支持 **“设置颜色”**、**“随机颜色”** 以及 **“文件夹图标”** 这三个选项（对应用户原话），并完美应用个性化渲染与重绘。

Jules 将在此文档中对该双重外观自定义功能进行深度数据库升级设计、AppConfig 持久层拓展、API 接口定制以及 UI 动态交互体系的全方位分析。

---

## 2. 问题定位
要实现分类及盘符栏自定义文件夹图标与外观设定，需要将逻辑打通至以下模块：
1. **侧边栏数据库结构升级与实体读写**：
   - 文件：`src/meta/DatabaseManager.cpp`、`src/meta/CategoryRepo.h` / `CategoryRepo.cpp`
   - 原因：SQLite 中的 `categories` 表和内存中的 `struct Category` 需要扩展 `icon` 字段，并在持久层读写接口（`getAll`、`getById`、`add`、`update`）中完成 SQL 绑定。
2. **侧边栏树模型动态加载与渲染**：
   - 文件：`src/ui/CategoryModel.cpp`、`src/ui/CategoryPanel.cpp`
   - 原因：`CategoryModel::refresh()` 在渲染分类树节点和快速访问镜像时需要优先读取 `cat.icon`。而 `CategoryPanel::setupContextMenu` 则需要新增主菜单“**文件夹图标**”并挂载矢量图标子菜单，触发点击时完成更新与重刷。
   - *特别优先级*：当分类处于加密锁定状态时，锁定图标（`lock`）依然需要覆盖自定义图标进行优先展现，以满足安全性语境。
3. **盘符栏自定义目录的外观属性持久化拓展**：
   - 文件：`src/ui/MainWindow.cpp` 或对应的 `DriveBar` 控制区域
   - 原因：原本的自定义监控目录仅在 `AppConfig` 的 `"DriveBar/CustomMonitoredFolders"` 中存储为一个 `QStringList` 的扁平物理路径数组。为了绑定颜色与图标，我们需要在 `AppConfig` 中增加以物理路径为关联标识的动态 KV 映射。
4. **盘符栏自定义按钮（FolderButton）的外观渲染与交互**：
   - 文件：`src/ui/DriveButton.cpp` / `src/ui/DriveButton.h`
   - 原因：`FolderButton` 绘制逻辑需要动态读取其绑定的物理路径的外观配置；在其右键 `contextMenuEvent` 或信号响应中新增这三个选项，点击后更新 `AppConfig` 对应属性并触发重绘更新。

---

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 在侧边栏右键菜单中新增主选项“文件夹图标”（对应用户原话） | 在 `CategoryPanel::setupContextMenu` 中添加名称为 `"文件夹图标"` 的二级菜单项。 | ✅ 一致 |
| 2    | 数据库存储，启动自动升级迁移 | 在 `DatabaseManager::loadDb` 函数中自动探测 categories 表，执行 `ALTER TABLE categories ADD COLUMN icon TEXT DEFAULT 'folder_filled'` 字段迁移。 | ✅ 一致 |
| 3    | 盘符栏自定义文件夹右键也希望有“设置颜色”、“随机颜色”和“文件夹图标”这三个选项（对应用户原话） | 在 `FolderButton` 的上下文右键菜单中，在原有的“新建自动导入”与“移除”下平移追加这三个外观自定义选项。 | ✅ 一致 |
| 4    | 盘符栏属性持久化与无缝实时重绘更新 | 采用 AppConfig 针对路径的去中心化属性绑定：`FolderColor_[Path]` 与 `FolderIcon_[Path]`，变更后立即触发 `DriveBar` 重新绘制。 | ✅ 一致 |

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

### 4.2 侧边栏内存模型与读写接口适配（CategoryRepo.h / CategoryRepo.cpp）
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

### 4.3 侧边栏树节点与镜像渲染（CategoryModel.cpp）
在树模型重构分类节点和快速访问镜像时，如果是普通分类或快速访问分类，在非锁定状态下采用 `cat.icon` 重新载入。
```cpp
// CategoryModel::refresh() 循环加载部分：
if (cat.encrypted && !m_unlockedIds.contains(id)) {
    // 安全红线：加密且未解锁时强制采用 lock 图标展示
    item->setIcon(UiHelper::getIcon("lock", QColor("#aaaaaa"), 16));
} else {
    // 无锁/已解锁状态：读取数据库中自定义的 icon 键名
    QString iconKey = QString::fromStdWString(cat.icon).isEmpty() ? "folder_filled" : QString::fromStdWString(cat.icon);
    item->setIcon(UiHelper::getIcon(iconKey, QColor(color), 16));
}

// 快速访问分类镜像 (Mirror Nodes) 加载部分：
if (cat.encrypted && !m_unlockedIds.contains(id)) {
    mirror->setIcon(UiHelper::getIcon("lock", QColor("#aaaaaa"), 16));
} else {
    // 物理同步快速访问中的镜像图标
    QString iconKey = QString::fromStdWString(cat.icon).isEmpty() ? "folder_filled" : QString::fromStdWString(cat.icon);
    mirror->setIcon(UiHelper::getIcon(iconKey, QColor(color), 16));
}
```

### 4.4 侧边栏右键自定义图标菜单（CategoryPanel.cpp）
```cpp
// CategoryPanel::setupContextMenu() 的普通分类节点条件分支内：
if (itemType == "category" || itemType == "file" || itemType == "folder") {
    // 1. 创建主选项“文件夹图标”
    QMenu* iconMenu = menu.addMenu(UiHelper::getIcon("folder_filled", WarningOrange, 18), "文件夹图标");
    UiHelper::applyMenuStyle(iconMenu);

    // 2. 内置平铺的备选矢量图标映射表
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

    QString colorStr = index.data(ColorRole).toString();
    QColor catColor = colorStr.isEmpty() ? QColor("#555555") : QColor(colorStr);

    for (auto it = builtInIconMap.begin(); it != builtInIconMap.end(); ++it) {
        QString label = it.key();
        QString iconKey = it.value();

        QAction* iconAct = iconMenu->addAction(UiHelper::getIcon(iconKey, catColor, 18), label);
        connect(iconAct, &QAction::triggered, this, [this, id, iconKey]() {
            auto cats = CategoryRepo::getAll();
            for (auto& cat : cats) {
                if (cat.id == id) {
                    cat.icon = iconKey.toStdWString();
                    CategoryRepo::update(cat);
                    break;
                }
            }
            m_categoryModel->refresh();
        });
    }
}
```

---

### 4.5 盘符栏自定义文件夹外观设定与持久化（DriveButton.cpp / DriveButton.h）

#### 4.5.1 属性持久化扩展
为了对自定义监控目录物理路径建立去中心化外观设定存储：
```cpp
// 获取路径对应颜色的辅助方法
QString getFolderColor(const QString& path) {
    QString key = QString("DriveBar/FolderColor_%1").arg(path);
    return AppConfig::instance().getValue(key, "#FFFFFF").toString(); // 默认白色/常规高亮
}

// 获取路径对应图标的辅助方法
QString getFolderIcon(const QString& path) {
    QString key = QString("DriveBar/FolderIcon_%1").arg(path);
    return AppConfig::instance().getValue(key, "folder_filled").toString(); // 默认文件夹
}

// 物理移除时的静默清理逻辑
void removeFolderConfig(const QString& path) {
    AppConfig::instance().setValue(QString("DriveBar/FolderColor_%1").arg(path), QVariant());
    AppConfig::instance().setValue(QString("DriveBar/FolderIcon_%1").arg(path), QVariant());
}
```

#### 4.5.2 FolderButton 右键菜单及重绘刷新扩展
在 `FolderButton` 的事件响应或信号连接中补齐菜单：
```cpp
// FolderButton 右键菜单弹出处（DriveButton.cpp 中对 contextMenuEvent 的处理）：
void FolderButton::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    // 原有选项
    menu.addAction("新建自动导入", this, &FolderButton::onNewAutoImport);
    menu.addAction("移除", this, &FolderButton::onRemoveFolder);
    menu.addSeparator();

    // 1. 新增 “设置颜色”
    menu.addAction(UiHelper::getIcon("palette", WarningOrange, 18), "设置颜色", this, [this]() {
        FramelessColorPicker dlg("选择文件夹颜色", this);
        QString currentClr = getFolderColor(m_path);
        dlg.setCurrentColor(QColor(currentClr));
        if (dlg.exec() == QDialog::Accepted) {
            QColor selected = dlg.selectedColor();
            AppConfig::instance().setValue(QString("DriveBar/FolderColor_%1").arg(m_path), selected.name().toUpper());
            AppConfig::instance().sync();

            // 触发 FolderButton 的更新重绘
            updateButtonAppearance();
        }
    });

    // 2. 新增 “随机颜色”
    menu.addAction(UiHelper::getIcon("random_color", QColor("#e91e63"), 18), "随机颜色", this, [this]() {
        static const QStringList palette = {
            "#FF6B6B", "#4ECDC4", "#45B7D1", "#96CEB4", "#FFEEAD",
            "#D4A5A5", "#9B59B6", "#3498DB", "#E67E22", "#2ECC71"
        };
        QString chosenColor = palette.at(QRandomGenerator::global()->bounded(palette.size()));
        AppConfig::instance().setValue(QString("DriveBar/FolderColor_%1").arg(m_path), chosenColor);
        AppConfig::instance().sync();

        updateButtonAppearance();
    });

    // 3. 新增 “文件夹图标” 二级子菜单
    QMenu* iconMenu = menu.addMenu(UiHelper::getIcon("folder_filled", PrimaryBlue, 18), "文件夹图标");
    UiHelper::applyMenuStyle(iconMenu);

    static const QMap<QString, QString> builtInIconMap = {
        {"默认文件夹", "folder_filled"},
        {"层级分类", "category"},
        {"照片媒体", "image_filled"},
        {"时钟历史", "clock_filled"},
        {"星标收藏", "star_filled"},
        {"爱心常用", "heart_filled"},
        {"加密安全", "lock_filled"},
        {"图书文档", "book"},
        {"配置管理", "settings_filled"}
    };

    QColor catColor = QColor(getFolderColor(m_path));

    for (auto it = builtInIconMap.begin(); it != builtInIconMap.end(); ++it) {
        QString label = it.key();
        QString iconKey = it.value();

        QAction* iconAct = iconMenu->addAction(UiHelper::getIcon(iconKey, catColor, 18), label);
        connect(iconAct, &QAction::triggered, this, [this, iconKey]() {
            AppConfig::instance().setValue(QString("DriveBar/FolderIcon_%1").arg(m_path), iconKey);
            AppConfig::instance().sync();

            updateButtonAppearance();
        });
    }

    menu.exec(event->globalPos());
}

// 动态重画更新 Appearance 的核心方法实现
void FolderButton::updateButtonAppearance() {
    QString colorStr = getFolderColor(m_path);
    QString iconKey = getFolderIcon(m_path);

    // 加载自定义图标与颜色
    QIcon icon = UiHelper::getIcon(iconKey, QColor(colorStr), 16);
    this->setIcon(icon);

    // 更新样式颜色
    this->setStyleSheet(QString(
        "QPushButton { color: %1; }"
        "QPushButton:hover { background-color: #3E3E42; }"
    ).arg(colorStr));
}
```

---

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/meta/DatabaseManager.cpp` （数据库载入与自动升级 ALTER TABLE）
- [ ] 模块/文件：`src/meta/CategoryRepo.h` / `CategoryRepo.cpp` （Category 结构体扩展，SQL 语句及参数绑定）
- [ ] 模块/文件：`src/ui/CategoryModel.cpp` （CategoryModel 节点图标动态加载与无锁渲染逻辑）
- [ ] 模块/文件：`src/ui/CategoryPanel.cpp` （CategoryPanel 上下文右键二级子菜单构建与更新信号分发）
- [ ] 模块/文件：`src/ui/DriveButton.h` / `src/ui/DriveButton.cpp` （FolderButton 的事件接收，QMenu 动态选项拓展与 Appearance 重绘刷新）

**明确禁止越界修改的范围：**
- [ ] 严禁修改其他非本任务相关的元数据表、USN 监控及物理扫描机制。
- [ ] 严禁删除、重命名 `SvgIcons.h` 中的现有 SVG 属性或篡改其键名。

---

## 6. 实现准则与预警【核心】
1. **重绘渲染与样式对齐**：
   - 盘符栏按钮在悬停时需完全遵守 `AGENTS.md` 规定的 Hover 规范色 `#3E3E42`（Style::HoverBackground）。
2. **清除移除逻辑一齐清除**：
   - 当点击 “移除” 时，需确保调用上文设计的 `removeFolderConfig(m_path)` 将属性配置静默剔除，防止由于长久累积导致 `AppConfig` 的 ini 文件极具膨胀。

---

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 侧边栏分类模式 | 属于 SQLite DB-driven 内存镜像，完全符合从内存加载与持久化要求。 | ✅ 符合 |
| 直连磁盘数据库模式 | 满足 `DatabaseManager::loadDb` 秒退架构的高性能 PRAGMA journal_mode 与直连规范。 | ✅ 符合 |
| 样式与 hover 背景 | 顶栏按钮悬停完全对齐 `#3E3E42` 规范。 | ✅ 符合 |
| UI考古原则 | 本方案的菜单 Action 以及 `getIcon` 调用均严格考古并对齐于 `CategoryPanel.cpp` 原有的设置颜色及设置预设标签。 | ✅ 符合 |
