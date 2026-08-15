# “创建资源库”逻辑架构重构与混淆隔离修复方案 —— create-library-remediation.md

## 一、 方案背景与核心问题

在之前的版本演进中，盘符栏右键菜单中的“创建资源库”逻辑与侧边栏中的“新建文件夹”逻辑发生了混淆：
1. **逻辑未闭环**：盘符栏执行“创建资源库”时，仅在物理磁盘上创建了 `arcmeta.library_X` 文件夹，未同步在侧边栏第二类“半静态分类（物理托管库）”中注册并生成对应的分类节点。
2. **逻辑交织混淆**：“创建资源库”（物理盘符层最高级容器）与“新建文件夹”（用户业务组织层分类）在触发管道与模型处理上未彻底解耦，导致两者的生命周期与受保护属性互相撕裂。

本方案旨在提供精准的代码修复指引，确保两套逻辑在代码层面彻底解耦、各自独立执行。

---

## 二、 核心修复原则

1. **“创建资源库”双重原子动作**：
   - **物理磁盘层**：在指定盘符根目录下创建 `arcmeta.library_X` 真实物理文件夹及 SQLite 数据库基础结构。
   - **侧边栏分类层**：同步在 SQLite `categories` 表中注册 `CategoryKind::SystemLibrary` 分类记录（`parentId = 0`），并在侧边栏第二区域（半静态分类/物理托管库组）即时挂载展示。
2. **受保护属性与交互隔离**：
   - 托管库分类（`SystemLibrary`）具备系统保护属性，绝对禁止用户在侧边栏中对其进行手动重命名、删除、设色或改变层级。
3. **“新建文件夹”管道独立**：
   - “新建文件夹”（`onCreateCategory` / `onCreateSubCategory`）仅负责创建 `CategoryKind::User` 类型的用户自定义逻辑分类（或磁盘模式下的常规物理子文件夹），100% 禁绝触发或篡改 `arcmeta.library_*` 托管库节点。

---

## 三、 模块化代码修改指引

### 3.1 模块一：`src/ui/MainWindow.cpp`（盘符栏右键创建触发源）

* **修改位置**：`MainWindow::onDriveButtonContextMenu`
* **目标逻辑**：
  在物理创建 `managedPath` 成功后，同步将该托管库注册至 `CategoryRepo` 数据库表，并发布刷新信号：

```cpp
// 示例修改逻辑
if (QDir().mkpath(managedPath)) {
    btn->setState(DriveButton::Active);

    std::wstring wPath = QDir::toNativeSeparators(managedPath).toStdWString();

    // 1. 构造半静态托管库分类记录
    Category cat;
    cat.name = QFileInfo(managedPath).fileName().toStdWString(); // 如 "arcmeta.library_g"
    cat.parentId = 0;
    cat.kind = CategoryKind::SystemLibrary; // 标记为半静态托管库分类
    cat.physicalPath = wPath;
    cat.color = CategoryRepo::getDefaultColor();

    // 2. 尝试锚定 Win32 物理 FRN (如果可用)
    std::string fid;
    std::wstring frnStr;
    if (MetadataManager::fetchWinApiMetadataDirect(wPath, fid, &frnStr)) {
        try { cat.physicalFrn = std::stoull(frnStr, nullptr, 16); } catch (...) {}
    }

    // 3. 写入分类表并通知侧边栏 UI 1:1 重建刷新
    if (CategoryRepo::add(cat)) {
        MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
    }

    ToolTipOverlay::instance()->showText(QCursor::pos(), "资源库创建成功", 1500, Style::SuccessGreen);
}
```

---

### 3.2 模块二：`src/ui/CategoryModel.cpp`（侧边栏分类树挂载与隔离）

* **修改位置**：`CategoryModel::refresh` 与 `CategoryModel::setupCategoryTree`
* **目标逻辑**：
  在遍历组装分类树时，严格通过 `cat.kind == CategoryKind::SystemLibrary` 进行判别分流：
  - 将 `SystemLibrary` 分类节点强制挂载于**第二区域（半静态分类/物理托管库组）**；
  - 绑定 `CategoryKindRole` 属性，确保 Delegate 和 View 能准确识别其受保护属性。

```cpp
// 在构建 Item 节点时设置 CategoryKindRole
item->setData(static_cast<int>(cat.kind), CategoryKindRole);
```

---

### 3.3 模块三：`src/ui/CategoryPanel.cpp`（右键菜单与编辑交互隔离）

* **修改位置**：`CategoryPanel::setupContextMenu`、`onRenameCategory`、`onDeleteCategory`
* **目标逻辑**：
  在用户触发右键菜单或按下快捷键（如 `F2`、`Delete`）时，拦截 `CategoryKind::SystemLibrary` 节点：

```cpp
// 在重命名 / 删除函数头部增加防护拦截
int catId = getTargetCategoryId(index);
if (catId > 0) {
    Category cat = CategoryRepo::getById(catId);
    if (cat.kind == CategoryKind::SystemLibrary || (!cat.physicalPath.empty() && cat.parentId == 0)) {
        ToolTipOverlay::instance()->showText(QCursor::pos(),
            "<b style='color:#e81123;'>受保护的托管库分类禁止编辑/删除！</b>", 2000, QColor("#e81123"));
        return;
    }
}
```

---

## 四、 验收与测试用例

1. **测试用例 1：盘符栏“创建资源库”双重挂载测试**
   - **步骤**：在盘符栏（`DriveBar`）右键点击未初始化的盘符（如 H 盘），选择“创建资源库”。
   - **预期结果**：
     1) H 盘根目录下生成 `H:\ArcMeta.Library_H` 物理文件夹；
     2) 侧边栏“半静态分类”区域瞬间出现 `arcmeta.library_h` 分类节点，且初始计数为 `(0)`。

2. **测试用例 2：“新建文件夹”隔离测试**
   - **步骤**：在侧边栏“文件夹”分组右键点击“新建文件夹”，或在内容区空白右键“新建文件夹”。
   - **预期结果**：
     仅在用户自定义分类树中生成普通逻辑分类（`CategoryKind::User`），绝不触发磁盘根目录下 `ArcMeta.Library_*` 的物理建包动作。

3. **测试用例 3：受保护属性拦截测试**
   - **步骤**：在侧边栏右键点击 `arcmeta.library_h` 节点，尝试选择重命名或删除，或按 `F2` / `Delete` 键。
   - **预期结果**：
     系统弹出红字警告“受保护的托管库分类禁止编辑/删除！”，拦截修改操作。

---

> **文档状态**：已根据架构共识生成完毕，等待后续代码重构执行。
