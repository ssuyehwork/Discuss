# Implementation Plan: Expand "Show in QuarkMeta" to Library Scene (ContentContextMenuShowInQuarkMeta-1.md)

## 1. Overview
本实施方案旨在将内容面板右键菜单中的 **“在 QuarkMeta 中显示”** (`ActionShowInQuarkMeta`) 菜单项的启用条件进行扩展。

原条件仅在递归平铺模式 (`m_panel->isRecursive()`) 下显示。现扩展为：在**“显示子文件夹中的项目”**模式或**“库”分类视图** (`m_panel->getCurrentCategoryType() == "library"`) 下均予以展示，使用户在浏览“库”分类中的跨文件夹文件时，也能通过右键菜单直接跳转至文件实际存放的物理文件夹视角，并自动定位高亮该文件。

---

## 2. Modified Files List
* `src/ui/controllers/ContentContextMenu.cpp`

---

## 3. Detailed Line-by-Line Changes

### File: `src/ui/controllers/ContentContextMenu.cpp`

<<<<<<< SEARCH
            if (m_panel && m_panel->isRecursive()) {
                menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE"), 18), "在 QuarkMeta 中显示")->setData(ContentPanel::ActionShowInQuarkMeta);
            }
=======
            if (m_panel && (m_panel->isRecursive() || m_panel->getCurrentCategoryType() == "library")) {
                menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE"), 18), "在 QuarkMeta 中显示")->setData(ContentPanel::ActionShowInQuarkMeta);
            }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps

### 构建步骤
使用 CMake 增量编译项目：
```bash
cmake --build build --config Debug --target QuarkMeta
```

### 验证步骤
1. 启动 QuarkMeta 应用程序；
2. 切换至侧边栏 **“库”** 视图（如选择任意自定义或内置库分类）；
3. 右键点击“库”视图中的任意文件项目，唤出右键菜单；
4. 验证右键菜单中出现 **“在 QuarkMeta 中显示”** 选项；
5. 点击 **“在 QuarkMeta 中显示”**；
6. 验证应用程序成功平滑跳转至该文件所属的真实物理磁盘目录，且当前视图中自动导航定位并高亮选中了该文件；
7. 切换至普通的硬盘文件夹视图（非递归模式、非库视图），确认右键菜单中不出现该选项，门禁逻辑符合预期。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

| 自查项 | 结论 | 说明 |
| :--- | :--- | :--- |
| **是否复用既有 SSOT 通道？** | **是** | 复用既有 `ContentPanel::ActionShowInQuarkMeta` 与 `NavigationService::instance().navigateTo` 通道 |
| **是否存在另起炉灶行为？** | **否** | 仅扩展条件判断 `m_panel->getCurrentCategoryType() == "library"`，无任何冗余重复代码 |
| **底层功能完整性** | **100% 保持** | 点击后的跳转与高亮选中逻辑已在 `ActionShowInQuarkMeta` 槽函数中完全自治 |

---

## 6. Header API Signature Verification

| 被调用的类/函数 | `.h` 文件真实物理签名 | 核查状态 |
| :--- | :--- | :--- |
| `ContentPanel::getCurrentCategoryType` | `QString getCurrentCategoryType() const;` | 物理存在，匹配 |
| `ContentPanel::isRecursive` | `bool isRecursive() const;` | 物理存在，匹配 |
| `NavigationService::navigateTo` | `void navigateTo(const QString& path);` | 物理存在，匹配 |

---

## 7. Header Inclusion Chain & Type Completeness Check

| 修改文件 | 新增/引用的类与枚举 | 头文件包含状态 |
| :--- | :--- | :--- |
| `ContentContextMenu.cpp` | `ContentPanel` | 已包含 `"ContentPanel.h"`，类型定义完整 |
