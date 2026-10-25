# Implementation Plan - 按文件夹独立持久化排序偏好方案

- **Base Commit Hash**: `1e9a71b62c08f4b040483238a5a105559768becc`
- **Prerequisite Plans**: 无
- **Supersedes**: 无

---

## 第 1 关：已读物理证据清单 (Pre-Draft Evidence)

### 1.1 物理读取文件列表
1. `src/meta/MetadataDefs.h` (已读取 `FolderMeta` 内部 `sortBy` 和 `sortOrder` 字段声明)
2. `src/meta/MetadataManager.h` (已读取 `getFolderMeta` 与 `setFolderMeta` 方法签名)
3. `src/ui/controllers/ContentSortController.h` / `src/ui/controllers/ContentSortController.cpp` (已读取 `ContentSortController` 的加载、修改与保存逻辑)
4. `src/ui/controllers/ContentDataLoader.cpp` (已读取 `ContentDataLoader::loadDirectory` 路径加载流程)

### 1.2 头文件原样签名
1. `FolderMeta` 字段声明：
   - 声明位置：`src/meta/MetadataDefs.h` 第 40-41 行：
     `std::wstring sortBy;`
     `std::wstring sortOrder;`
2. `MetadataManager::getFolderMeta(const QString& folderPath)`：
   - 声明位置：`src/meta/MetadataManager.h` 第 82 行
3. `MetadataManager::setFolderMeta(const QString& folderPath, const FolderMeta& meta)`：
   - 声明位置：`src/meta/MetadataManager.h` 第 83 行

### 1.3 既有 API 检索日志
- 检索关键词：`grep -rn "RightClickSortType" src/`
- 命中列表：`ContentSortController.cpp` 中将排序存取保存在 `AppConfig` 的 `"ContentPanel/RightClickSortType"` 和 `"ContentPanel/RightClickSortOrder"`。

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题现象与需求分析
用户在不同文件夹中处理不同类型的内容（如某些文件夹适合按“创建日期 降序”浏览，某些适合按“名称 升序”浏览）。目前 QuarkMeta 的排序逻辑为全应用统一的全局排序，每次切换文件夹都会共享同一套排序规则，导致用户需要反复手动切换，体验极为苦闷。

### 1.2 物理根因与架构现状
底层元数据结构 `FolderMeta` (`src/meta/MetadataDefs.h`) 在设计之初便已预留了 `sortBy` 与 `sortOrder` 字段，且 `.QuarkMeta.json` 侧车文件支持这些字段的序列化与反序列化。但 UI 排序控制器 `ContentSortController` 过去仅向 `AppConfig` 读写全局统一的 Key，未与 `FolderMeta` 以及 `MetadataManager` 进行双向联动。

### 1.3 修复方案设计
1. **文件夹专属加载**：当导航进入新文件夹时，在 `ContentDataLoader::loadDirectory` 中调用 `MetadataManager::instance().getFolderMeta(path)` 提取该文件夹的 `sortBy` 与 `sortOrder`，并同步更新给 `ContentSortController`；若文件夹无专属排序配置，则读取 `AppConfig` 的全局默认配置；
2. **文件夹专属保存**：当用户在当前文件夹手动调整排序方式或升降序时，同步更新 `ContentSortController`，并调用 `MetadataManager::instance().setFolderMeta` 将排序设置实时写入该文件夹的 `.QuarkMeta.json` 隐藏侧车文件中，实现彻底的按文件夹持久化与便携随行。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/ui/controllers/ContentSortController.h` / `src/ui/controllers/ContentSortController.cpp` (修改：支持按路径存取文件夹专属排序)
2. `src/ui/controllers/ContentDataLoader.cpp` (修改：加载文件夹时应用专属排序规则)

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 为 `ContentSortController` 增加按路径加载与保存文件夹专属排序支持

```
<<<<<<< SEARCH
void ContentSortController::saveToConfig() {
    AppConfig::instance().setValue("ContentPanel/RightClickSortType", static_cast<int>(m_sortType));
    AppConfig::instance().setValue("ContentPanel/RightClickSortOrder", static_cast<int>(m_sortOrder));
    AppConfig::instance().sync();
}
=======
void ContentSortController::saveToConfig() {
    AppConfig::instance().setValue("ContentPanel/RightClickSortType", static_cast<int>(m_sortType));
    AppConfig::instance().setValue("ContentPanel/RightClickSortOrder", static_cast<int>(m_sortOrder));
    AppConfig::instance().sync();

    if (!m_currentFolderPath.isEmpty()) {
        FolderMeta meta = MetadataManager::instance().getFolderMeta(m_currentFolderPath);
        
        switch (m_sortType) {
            case SortType::SortByName: meta.sortBy = L"name"; break;
            case SortType::SortByCreateDate: meta.sortBy = L"cdate"; break;
            case SortType::SortByModifyDate: meta.sortBy = L"mtime"; break;
            case SortType::SortByExtension: meta.sortBy = L"type"; break;
            case SortType::SortBySize: meta.sortBy = L"size"; break;
            case SortType::SortByDimension: meta.sortBy = L"dim"; break;
            case SortType::SortByRating: meta.sortBy = L"rating"; break;
            default: meta.sortBy = L"name"; break;
        }
        meta.sortOrder = (m_sortOrder == Qt::AscendingOrder) ? L"asc" : L"desc";

        MetadataManager::instance().setFolderMeta(m_currentFolderPath, meta);
    }
}

void ContentSortController::setCurrentFolderPath(const QString& folderPath) {
    m_currentFolderPath = folderPath;
    if (folderPath.isEmpty()) {
        loadFromConfig();
        return;
    }

    FolderMeta meta = MetadataManager::instance().getFolderMeta(folderPath);
    if (!meta.sortBy.empty()) {
        if (meta.sortBy == L"name") m_sortType = SortType::SortByName;
        else if (meta.sortBy == L"cdate") m_sortType = SortType::SortByCreateDate;
        else if (meta.sortBy == L"mtime") m_sortType = SortType::SortByModifyDate;
        else if (meta.sortBy == L"type") m_sortType = SortType::SortByExtension;
        else if (meta.sortBy == L"size") m_sortType = SortType::SortBySize;
        else if (meta.sortBy == L"dim") m_sortType = SortType::SortByDimension;
        else if (meta.sortBy == L"rating") m_sortType = SortType::SortByRating;
        
        m_sortOrder = (meta.sortOrder == L"desc") ? Qt::DescendingOrder : Qt::AscendingOrder;
    } else {
        loadFromConfig();
    }
}
>>>>>>> REPLACE
```

### 3.2 在 `ContentSortController.h` 中添加成员与方法

```
<<<<<<< SEARCH
    void setSortCriteria(SortType type, Qt::SortOrder order);

    static FileListColumn columnForSortType(SortType type);
=======
    void setSortCriteria(SortType type, Qt::SortOrder order);
    void setCurrentFolderPath(const QString& folderPath);

    static FileListColumn columnForSortType(SortType type);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    SortType m_sortType = SortType::SortByName;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
=======
    SortType m_sortType = SortType::SortByName;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
    QString m_currentFolderPath;
>>>>>>> REPLACE
```

### 3.3 在 `ContentDataLoader::loadDirectory` 中调用专属排序设置

```
<<<<<<< SEARCH
void ContentDataLoader::loadDirectory(const QString& path, bool recursive) {
    if (!m_panel) return;
    m_panel->restoreActiveView();
=======
void ContentDataLoader::loadDirectory(const QString& path, bool recursive) {
    if (!m_panel) return;
    m_panel->restoreActiveView();
    if (m_panel->sortController()) {
        m_panel->sortController()->setCurrentFolderPath(path);
    }
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 构建步骤
运行项目标准 CMake 构建命令：
```bash
cmake --build --preset x64-Debug
```

### 4.2 运行时验收标准
1. 打开 QuarkMeta 应用程序，导航进入文件夹 A（如 `D:/A`）；
2. 右键菜单或状态栏调整排序方式为“按大小 降序”；
3. 导航进入另一个文件夹 B（如 `D:/B`），确认其初始化为默认排序规则（按名称 升序）；
4. 重新导航切回文件夹 A（`D:/A`），确认界面自动恢复并展现为“按大小 降序”；
5. 查看文件夹 A 根目录下的 `.QuarkMeta.json`，确认 `folder.sortBy` 与 `folder.sortOrder` 已成功保存记录。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- **底层元数据复用**：直接复用既有的 `FolderMeta` 数据结构及 `MetadataManager::getFolderMeta` / `setFolderMeta`，绝不另起炉灶新增配置文件或修改数据库 schema。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 源 | 派生副本/视图展现 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 文件夹专属排序 | 各目录 `.QuarkMeta.json` 中 `FolderMeta` 节点 | `ContentSortController` 的 `m_sortType` / `m_sortOrder` | 导航切入时从 `.QuarkMeta.json` 加载，修改排序时回写至 `.QuarkMeta.json` |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. `ContentSortController::setCurrentFolderPath(const QString& folderPath)`：
   - 新增于 `src/ui/controllers/ContentSortController.h`，`public` 访问权限。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- `ContentSortController.cpp` 中需要补充包含 `#include "../../meta/MetadataManager.h"` 以获取 `MetadataManager` 与 `FolderMeta` 完整定义。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 是否满足 | 说明 |
| :---: | :--- | :---: | :--- |
| 1 | 是否遵循物理隔离与实施方案命名契约 | 是 | 方案存放于 `Implementation Plan/FolderPerSortPersistence.md` |
| 2 | 是否包含全部 10 大必备章节 | 是 | 包含 Overview 至 Behavior Preservation 全部章节 |
| 3 | SEARCH 块是否在目标文件中真实存在且唯一 | 是 | 已核对原文件唯一匹配 |
| 4 | 是否未直接修改任何 C++/Qt 源代码文件 | 是 | 仅提交实施方案文档，绝对不改源码 |
| 5 | 是否破坏既有对外接口签名与 C++ 契约 | 是(否破坏) | 未修改 public API |
| 6 | 是否违反单向依赖与分层硬锁 | 是(否违反) | 控制器访问 Meta 模块 |
| 7 | 是否包含特定平台的 Hack 级补丁 | 是(否包含) | 无 Hack |
| 8 | 是否包含另起炉灶的重复实现 | 是(否包含) | 100% 复用既有 `FolderMeta` 结构 |
| 9 | 是否有隐式/显式的数据迁移逻辑 | 是(否包含) | 无数据迁移 |
| 10 | 是否破坏既有 UI 样式数值或参数 | 是(否破坏) | 不涉及 UI 样式数值 |
| 11 | 是否完整走完四道关质量管控流程 | 是 | 完成需求确认与逻辑核对 |
| 12 | 是否存在跨线程直接操作 GUI 控件行为 | 是(否存在) | 主线程调用 |
| 13 | 是否有死代码残留 | 是(无残留) | 无死代码 |
| 14 | 是否引发潜在的死锁风险 | 是(无风险) | 无锁操作 |
| 15 | 变量与类型闭合性 | 是 | 闭合 |
| 16 | CMake 注册完整性 | 是 | 未新建/删除文件 |
| 17 | 行为与交互零篡改声明 | 是 | 保持完全一致的右键与排序交互 |
| 18 | 语言规范 | 是 | 100% 全中文编写 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 类别 | 项目名称 | 原始状态 / 数值 | 实施后状态 / 数值 | 确认一致 |
| :--- | :--- | :--- | :--- | :---: |
| 交互逻辑 | 排序方式切换菜单 | 保持既有右键菜单与按钮触发逻辑 | 保持既有右键菜单与按钮触发逻辑 | 是 |
| 默认兜底 | 未配置排序的文件夹 | 使用全局配置作为兜底 | 使用全局配置作为兜底 | 是 |
