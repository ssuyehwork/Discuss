# Implementation Plan - ContentPanel Trash Refresh Routing Fix

## 1. Overview
修补 `ContentPanel::refreshAll()` 在回收站视角（`trash://`）下触发原位刷新时，误将其当作磁盘路径调用 `loadDirectory("trash://")` 导致物理磁盘扫描失败、回收站剩余项目被清空的致命 Bug。
在 `ContentPanel::refreshAll()` 中增加对 `m_currentCategoryType == "trash"` 的分类拦截分支，确保回收站刷新时始终正确路由至 `loadCategory("trash")` 从 SQLite 数据库读取最新暂存列表。

## 2. Modified Files List
- `src/ui/ContentPanel.cpp`

## 3. Detailed Line-by-Line Changes

### File: `src/ui/ContentPanel.cpp`
在 `ContentPanel::refreshAll()` 函数中，增加对 `m_currentCategoryType == "trash"` 的逻辑路由分支。

```
<<<<<<< SEARCH
void ContentPanel::refreshAll() {
    if (m_currentViewMode == ColumnView) {
        if (m_columnView) m_columnView->refreshAllColumns();
        return;
    }
    if (!m_currentPath.isEmpty() && m_currentPath != "computer://") loadDirectory(m_currentPath, m_isRecursive);
    else loadDirectory("computer://");
}
=======
void ContentPanel::refreshAll() {
    if (m_currentViewMode == ColumnView) {
        if (m_columnView) m_columnView->refreshAllColumns();
        return;
    }
    if (m_currentCategoryType == "trash") {
        loadCategory("trash");
        return;
    }
    if (!m_currentPath.isEmpty() && m_currentPath != "computer://") loadDirectory(m_currentPath, m_isRecursive);
    else loadDirectory("computer://");
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
```bash
# 1. 配置并构建 CMake 项目
cmake -B build -S .
cmake --build build --config Release

# 2. 验证路径与测试
# - 进入回收站视图 (`trash://`)，确保列表中有多个回收站项目（如 10 个）
# - 选中其中 1 个项目点击右键“还原”
# - 校验还原完成后，回收站列表是否平滑刷新并保留剩余的 9 个项目，不再出现“没有可显示的项目”与空白画面
```

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **SSOT 入口复用**：`refreshAll()` 正确分发并复用 `ContentDataLoader::loadCategory("trash")` 作为回收站列表装载的唯一 SSOT 通道。
- **无重复实现**：消除误调 `loadDirectory` 引起的假磁盘扫描。

## 6. Header API Signature Verification
| 类名 / 模块名 | 调用的成员/数据角色 | 物理头文件签名 |
| :--- | :--- | :--- |
| `ContentPanel` | `loadCategory` | `void loadCategory(const QString& categoryType);` |
| `ContentPanel` | `m_currentCategoryType` | `QString m_currentCategoryType;` |
