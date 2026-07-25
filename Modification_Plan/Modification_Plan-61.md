# 托管库分类与物理文件夹 1:1 双向同步清退方案 —— Modification_Plan-61.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
在 ArcMeta 托管库设计中，“ArcMeta.Library_[盘符]”下的物理文件夹和侧边栏分类树应当是 1:1 镜像并实时同步的。
当前版本中存在两个严重违背初衷的同步盲点：
1. **单向断裂**：在外部将库内某个物理文件夹移出或删除后，侧边栏分类依然残留。
2. **缺乏逆向物理同步**：在侧边栏手动删除某个映射到物理文件夹的分类时，未能同步清退/删除其对应的物理文件夹及映射状态。

本方案旨在针对该双向同步盲点，制定完整的双向清退逻辑与修改方案，确保双向对应的彻底映射和同步更新。

## 2. 问题定位

### 2.1 物理文件夹移出/删除 -> 侧边栏分类同步清理
`NativeFolderWatcher` 会捕获物理删除和重命名移出。它会调用 `MetadataManager::removeMetadataSync(fullPath)`。
虽然 `removeMetadataSync` 底部有清理镜像分类树的循环：
```cpp
auto allCats = CategoryRepo::getAll();
for (const auto& cat : allCats) {
    if (cat.physicalPath == nPath || cat.physicalPath.find(nPath + L"\\") == 0 || cat.physicalPath.find(nPath + L"/") == 0) {
        CategoryRepo::remove(cat.id);
    }
}
```
但当用户在磁盘上直接删除某个文件夹时：
1. 该文件夹下的子文件会首先产生一波 IOCP 的 `FILE_ACTION_REMOVED`。
2. 文件夹本身随后也会被删除，产生一个 `FILE_ACTION_REMOVED`。
3. 此时由于 `fullPath` 处理、物理路径归一化可能与分类的 `physicalPath` 存在格式差异（如大小写、盘符反斜杠细节），或者 `CategoryRepo::remove` 执行后 UI 没有得到即时、专门的 `CategoryModel` 的刷新通知，导致分类未能彻底清退并在视觉上残留。

### 2.2 侧边栏删除分类 -> 同步更新并删除物理文件夹
在 `CategoryPanel::onDeleteCategory()` 中，系统通过 `ArcMeta::CategoryRepo::remove(id)` 执行数据库分类及关联的异步物理清洗：
1. 此时它仅仅清理了数据库分类、分类下的关联，并把项置入回收站。
2. 核心漏洞：它完全没有检测该分类是否具备 `physicalPath`。如果该分类拥有 `physicalPath`，理应将其对应的物理文件夹在磁盘上同步移出/删除（或移动到系统回收站/ArcMeta 特殊托管回收站中），从而达到双向强同步的要求。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 当我把某个文件夹移出/删除之后，分类仍然存在 | 加固物理删除逻辑：确保 `MetadataManager::removeMetadataSync` 在处理路径时精准进行 1:1 分类匹配和清退，并在清理完毕后通过 `CategoryModel::refresh()` 主动重刷分类树视图。 | ✅ 一致 |
| 2    | 当用户删除某个分类 / 文件夹，相应的映射也要同步更新 | 重构 `CategoryPanel::onDeleteCategory` 及 `CategoryRepo::remove`：删除具有物理映射的分类时，同步在物理磁盘上删除/移出对应的物理文件夹。 | ✅ 一致 |

## 4. 详细解决方案

### 4.1 逆向同步：手动删除分类时，同步删除其映射的物理文件夹
在删除分类的核心逻辑 `CategoryRepo::remove(int id)` 内，追加逆向物理同步判断（对应用户原话：“当用户删除某个分类 / 文件夹，相应的映射也要同步更新”）：
```cpp
// 伪代码参考
bool CategoryRepo::remove(int id) {
    // ... 原有收集 toDelete 逻辑保持 ...

    // 收集所有关联了物理路径的文件夹分类
    std::vector<std::wstring> physicalDirsToDelete;
    for (int delId : toDelete) {
        Category cat = getById(delId);
        if (cat.id > 0 && !cat.physicalPath.empty()) {
            physicalDirsToDelete.push_back(cat.physicalPath);
        }
    }

    // ... 原有清理子项及 categories 数据库记录逻辑不变 ...

    // 对磁盘上的对应物理文件夹执行物理清理/移出
    for (const auto& wPath : physicalDirsToDelete) {
        QString qPath = QString::fromStdWString(wPath);
        QDir dir(qPath);
        if (dir.exists()) {
            // 安全物理删除磁盘对应的物理文件夹（可使用 QFile::moveToTrash 放入系统回收站，或直接递归物理删除）
            qDebug() << "[DB_TRACE] 同步清理物理文件夹:" << qPath;
            dir.removeRecursively();
        }
    }

    // 派发刷新信号
    MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
    return true;
}
```

### 4.2 正向同步加固：物理移出/删除时彻底清退分类
在 `MetadataManager::removeMetadataSync` 底部，当由于物理删除触发 `CategoryRepo::remove(cat.id)` 后，
增加专门的信号机制或通过 `CategoryModel::refresh` 对分类模型进行显式重刷通知：
1. 确保对路径比较时一律进行标准化归一比对（采用 `normalizePath` 进行归一化路径匹配，防止盘符大小写或反斜杠格式不一致导致 `physicalPath == nPath` 匹配失效）。
2. 在 `MetadataManager::removeMetadataSync` 清理分类后，通过 `CategoryModel` 或者全局事件通知 UI 瞬间重新加载分类面板。

### 4.3 UI 层同步刷新保障
当 `MetadataManager::notifyUI(MetadataManager::RefreshLevel::FullRebuild)` 或其他物理改变信号发生时，确保 `CategoryPanel` 实时捕获并重刷 `CategoryModel`：
在 `CategoryPanel` 初始化时：
```cpp
// 确保关联 MetadataManager 的刷新通知，刷新分类树
connect(&MetadataManager::instance(), &MetadataManager::uiRefreshRequested, this, [this](MetadataManager::RefreshLevel level){
    if (level == MetadataManager::RefreshLevel::FullRebuild) {
        m_categoryModel->refresh();
    }
});
```

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/meta/CategoryRepo.cpp`（重构 `CategoryRepo::remove` 逻辑，识别并同步清理物理映射路径）
- [ ] 模块/文件：`src/meta/MetadataManager.cpp`（加固 `removeMetadataSync` 内部的分类路径标准化比对）
- [ ] 模块/文件：`src/ui/CategoryPanel.cpp`（连接物理改变信号与分类模型的主动重刷）

**明确禁止越界修改的范围：**
- [ ] 模块/文件：`src/mft/MftReader.cpp` —— 禁止触碰 MFT 底层底层物理缓存清理逻辑。
- [ ] 模块/文件：`src/core/NativeFolderWatcher.cpp` —— 原有的 IOCP 异步接收与消息分发逻辑禁止改变。

## 6. 实现准则与预警【核心】
1. **防止文件占用引发的物理删除失败**：磁盘上的文件夹删除可能因文件被占用而失败。为此，方案将调用 `QDir::removeRecursively()`，如果删除失败，需在后台输出警告日志，但不能影响数据库记录的清退。
2. **递归安全**：分类树删除为级联删除（递归收集所有子分类）。物理文件夹删除时同样需小心，若存在子映射关系，确保只对顶级映射的物理根目录调用一次删除即可，防止对同一个子目录产生多次无效的 I/O。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 输入框清除功能 | 一律使用 Qt 原生 setClearButtonEnabled(true)，严禁自定义按钮模拟 | ✅ 本方案不涉及输入框清除功能 |
| 窗口置顶 | 一律使用 Win32 原生 SetWindowPos 并配合 SWP_NOSENDCHANGING 标志，严禁使用 Qt 重建 | ✅ 本方案不涉及任何窗口置顶功能 |
| 标题栏按钮样式 | 悬停：#3E3E42（Style::HoverBackground），按下：#4E4E52，严禁 rgba 蒙版 | ✅ 本方案不涉及新标题栏按钮样式 |
| 物理/逻辑源 Focus 对齐 | 所有具备作用域功能执行范围必须与 UI 顶部蓝色 Focus Line 实时对齐 | ✅ 本方案不涉及 Focus Line 的改变 |

## 8. 待确认事项（可选）
- **删除物理文件夹时的安全策略**：是否直接调用 `QDir::removeRecursively()`（强力彻底物理抹除），还是推荐调用 `QFile::moveToTrash()` 将其更安全地置入系统的物理回收站？这可以在后续执行前由用户做最终裁夺。
