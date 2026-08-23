### 空白处右键菜单增加“在“资源管理器”中显示”实施方案

---

### 一、 改造目标

在内容面板（`ContentPanel`）空白处右键单击弹出的菜单中，新增 **“在“资源管理器”中显示”** 选项。
- **作用**：直接在 Windows 原生资源管理器中打开并定位当前正在浏览的物理文件夹（`m_currentPath`）。
- **智能可用性**：当处于真实物理文件夹时可用；当处于“此电脑”（`computer://`）或“回收站”（`trash://`）等虚拟视图时自动置灰或隐藏。

---

### 二、 具体代码实施细节

**文件：`src/ui/ContentPanel.cpp`**

#### 1. 在 `onCustomContextMenuRequested` 的空白处菜单（`else` 分支）中注入该选项：

找到 `ContentPanel.cpp` 中 `else`（空白处菜单）的代码块，在“新建...”组的下方或“刷新”组中添加：

```cpp
    } else { 
        // ==================== [空白处菜单] ==================== 
        QMenu* newMenu = menu.addMenu("新建..."); 
        UiHelper::applyMenuStyle(newMenu); 
        newMenu->addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "创建文件夹")->setData(ActionNewFolder); 
        newMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建 Markdown")->setData(ActionNewMd); 
        newMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建纯文本文件 (txt)")->setData(ActionNewTxt); 
 
        menu.addSeparator(); 

        QAction* actBatchCreate = menu.addAction(UiHelper::getIcon("add", QColor("#EEEEEE")), "批量创建项目...");
        actBatchCreate->setData(ActionBatchCreate);
        if (m_currentCategoryType == "trash") {
            actBatchCreate->setEnabled(false);
            actBatchCreate->setToolTip("回收站中不支持批量创建");
        }

        menu.addSeparator(); 
        QAction* actPaste = menu.addAction("粘贴"); 
        actPaste->setData(ActionPaste); 
        actPaste->setEnabled(canPaste()); // 绑定动态粘贴有效性判定
 
        menu.addSeparator(); 

        // 🚨 核心新增：在空白处右键菜单增加“在“资源管理器”中显示”
        bool isPhysicalPath = !m_currentPath.isEmpty() && !m_currentPath.contains("://") && QDir(m_currentPath).exists();
        QAction* actShowInExp = menu.addAction("在“资源管理器”中显示");
        actShowInExp->setData(ActionShowInExplorer);
        actShowInExp->setEnabled(isPhysicalPath); // 仅在有效物理路径下启用

        menu.addAction("刷新")->setData(ActionRefresh);
    } 
```

---

#### 2. 完善 `ActionShowInExplorer` 执行逻辑（`ShellHelper::openInExplorer`）：

在 `switch (action)` 的 `case ActionShowInExplorer:` 中，确保无论是选中项目还是对准空白处，都能正确唤起 Windows Explorer：

```cpp
        case ActionShowInExplorer: { 
            // 如果是在项目上右键，定位选中该文件；如果是在空白处右键，直接打开当前所在文件夹
            QString targetPath = onItem ? path : m_currentPath;
            if (!targetPath.isEmpty() && !targetPath.contains("://")) {
                ShellHelper::openInExplorer(targetPath); 
            }
            break; 
        } 
```

---

### 三、 预期效果

1. 用户在任意物理文件夹的内容区空白处点击右键时，菜单中清晰出现 **“在“资源管理器”中显示”**。
2. 点击后立即调用 Windows Explorer 打开当前物理目录，与在文件上右键的行为完全一致、平滑闭环。