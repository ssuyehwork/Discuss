# Implementation Plan - DiskItemModel Trash Roles Extension

## 1. Overview
修补 `DiskItemModel::data()` 对回收站扩展 Role（`IsDiskTrashRole` 与 `DiskTrashIdRole`）未暴露绑定的严重 Bug。
解决回收站视图（`trash://`）中选定项目还原时，`ContentPanel::getSelectedTrashIds()` 因读取不到模型 `IsDiskTrashRole` / `DiskTrashIdRole` 数据而获取空 ID 列表、导致还原流程在最上游关卡拦截退出的断裂问题。

## 2. Modified Files List
- `src/ui/models/DiskItemModel.cpp`

## 3. Detailed Line-by-Line Changes

### File: `src/ui/models/DiskItemModel.cpp`
在 `DiskItemModel::data()` 的角色分支中，补全对 `IsDiskTrashRole` 与 `DiskTrashIdRole` 的映射处理，直接返回记录中的 `record.isDiskTrash` 和 `record.diskTrashId`。

```
<<<<<<< SEARCH
    } else if (role == IsDropTargetRole) {
        return record.isDropTarget;
    } else if (role == HasThumbnailRole) {
=======
    } else if (role == IsDropTargetRole) {
        return record.isDropTarget;
    } else if (role == IsDiskTrashRole) {
        return record.isDiskTrash;
    } else if (role == DiskTrashIdRole) {
        return record.diskTrashId;
    } else if (role == HasThumbnailRole) {
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
```bash
# 1. 配置并构建 CMake 项目
cmake -B build -S .
cmake --build build --config Release

# 2. 验证路径与测试
# - 进入回收站视图 (`trash://`)
# - 选中某一项或多项回收站文件，右键点击“还原”
# - 校验 ContentPanel::getSelectedTrashIds() 是否能正常取得对应的整数 ID 列表
# - 校验 TrashService::restoreItems 能否被成功驱动并调用底层 DiskTrashService 还原物理文件与清理数据库记录
```

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **SSOT 入口复用**：`DiskItemModel` 作为统一主视图模型的角色数据暴露源，直接映射 `ItemRecord` 的权威模型数据。
- **无重复实现**：无需在 UI 层或 Controller 层做额外的类型转接，维持标准 Qt Model/View 数据绑定契约。

## 6. Header API Signature Verification
| 类名 / 模块名 | 调用的成员/数据角色 | 物理头文件签名 |
| :--- | :--- | :--- |
| `ModelContract` | `IsDiskTrashRole` | `IsDiskTrashRole = Qt::UserRole + 208` |
| `ModelContract` | `DiskTrashIdRole` | `DiskTrashIdRole = Qt::UserRole + 209` |
| `ItemRecord` | `isDiskTrash` | `bool isDiskTrash = false;` |
| `ItemRecord` | `diskTrashId` | `int diskTrashId = 0;` |
