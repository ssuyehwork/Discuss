# “创建资源库”与“新建文件夹”解耦与隔离重构方案 —— create-library-remediation.md

## 一、 方案背景与隔离原则

为了彻底消除之前逻辑上的模糊与混淆，必须对“创建资源库”与“新建文件夹”进行严格的解耦与概念隔离：

1. **零后台脑补与零静默联动**：
   - 系统绝不在后台静默推导、自动创建或强制绑定任何物理文件夹与侧边栏节点。
   - 所有逻辑必须由用户在 UI 上的显式交互（如右键菜单点击）明确触发。

2. **入口与逻辑完全隔离**：
   - **“创建资源库”**：仅响应用户在盘符栏（`DriveBar`）右键菜单上的显式点击触发。
   - **“新建文件夹”**：仅响应用户在侧边栏或内容区右键菜单上的显式点击触发。
   - 两条管道在代码模型、触发源与响应函数上彻底独立，互不调用，绝不产生隐藏关联。

3. **半静态托管库分类的保护属性**：
   - 属于半静态分类组的托管库节点（`CategoryKind::SystemLibrary`）具备受保护属性。
   - 侧边栏（`CategoryPanel`）拦截对该类节点的重命名、删除等非法编辑指令，确保结构稳定。

---

## 二、 模块化重构与修改指引

### 2.1 模块一：`src/ui/MainWindow.cpp`（盘符栏右键交互）

* **修改位置**：`MainWindow::onDriveButtonContextMenu`
* **目标逻辑**：
  当且仅当用户在盘符按钮上点击右键“创建资源库”菜单项时，执行该菜单绑定的指定动作：

```cpp
// 盘符右键菜单“创建资源库”处理函数
void MainWindow::onCreateLibraryForDrive(const QString& driveRoot)
{
    // 严格由用户显式点击触发，创建指定盘符下的托管库结构并刷新UI
    std::wstring managedPath = getManagedLibraryPathForDrive(driveRoot);
    if (QDir().mkpath(QString::fromStdWString(managedPath))) {
        // 注册半静态托管库节点
        Category cat;
        cat.name = QFileInfo(QString::fromStdWString(managedPath)).fileName().toStdWString();
        cat.parentId = 0;
        cat.kind = CategoryKind::SystemLibrary;
        cat.physicalPath = managedPath;

        if (CategoryRepo::add(cat)) {
            MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
        }
    }
}
```

---

### 2.2 模块二：`src/ui/CategoryPanel.cpp`（侧边栏右键与编辑隔离）

* **修改位置**：`CategoryPanel::setupContextMenu`、`onRenameCategory`、`onDeleteCategory`
* **目标逻辑**：
  在侧边栏交互中，拦截对半静态托管库节点（`CategoryKind::SystemLibrary`）的修改操作，弹出提示，防止误操作：

```cpp
// 在侧边栏右键菜单及按键响应中增加判别防护
void CategoryPanel::onDeleteCategory()
{
    QModelIndex index = getCurrentSelectedIndex();
    if (!index.isValid()) return;

    CategoryKind kind = static_cast<CategoryKind>(index.data(CategoryKindRole).toInt());
    if (kind == CategoryKind::SystemLibrary) {
        ToolTipOverlay::instance()->showText(QCursor::pos(),
            "<b style='color:#e81123;'>受保护的托管库节点禁止在侧边栏删除！</b>", 2000, QColor("#e81123"));
        return;
    }

    // 正常的自定义分类/文件夹删除逻辑...
}
```

---

### 2.3 模块三：`src/ui/CategoryModel.cpp`（分类树挂载分流）

* **修改位置**：`CategoryModel::refresh`
* **目标逻辑**：
  分类树加载时，依据 `kind` 字段进行组装展示，确保 `CategoryKind::SystemLibrary` 正确归类于半静态分类组（第二组）。

```cpp
// 分类节点数据绑定
item->setData(static_cast<int>(cat.kind), CategoryKindRole);
```

---

## 三、 验证与测试用例

1. **测试用例 1：盘符栏显式创建触发**
   - **步骤**：用户右键点击盘符按钮，点击“创建资源库”。
   - **预期结果**：仅在用户点击后执行创建逻辑，侧边栏第二组刷新显示对应的托管库节点。

2. **测试用例 2：“新建文件夹”隔离**
   - **步骤**：用户在侧边栏“文件夹”分组或内容区空白处右键“新建文件夹”。
   - **预期结果**：仅创建用户自定义分类或常规子文件夹，完全不影响磁盘根目录托管库结构。

3. **测试用例 3：受保护节点防护**
   - **步骤**：用户在侧边栏对托管库节点进行右键删除或按 `Delete` 键。
   - **预期结果**：弹框/气泡红字提示警告并拦截操作。

---

> **文档状态**：方案已修正并彻底剔除脑补/自动生成逻辑，等待代码重构落实。
