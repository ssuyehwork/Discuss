# Implementation Plan - 资源管理器交互场景语义归一化修正方案

## 1. 方案概述 (Overview)
本方案旨在修正全软件右键上下文菜单中调用 Windows 资源管理器时的场景语义与动作行为，彻底解决右键空白处误用“在“资源管理器”中显示”与 `/select` 选中参数的问题。
修正后：
1. **选中文件或文件夹项**（项右键菜单）：菜单文案保持为 **`在“资源管理器”中显示`**，调用 `ShellHelper::openInExplorer(path)` 执行 `explorer /select, <路径>`（高亮选中该项）；
2. **右键空白处**（空白处右键菜单）：菜单文案修正为 **`在“资源管理器”中打开`**，调用 `ShellHelper::openDirectoryInExplorer(path)` 执行 `explorer <当前目录路径>`（直接打开进入当前目录，不使用 `/select` 选中参数）；
3. 严格遵循 `Memories.md` Section 1.3 规则 10 **“在资源管理器中显示/打开”场景语义契约**，由 `ContextMenuFactory::buildShowInExplorerAction` 统一控制。

---

## 2. 问题分析与根因溯源 (Analysis & Root Cause)
1. **空白处场景文案与参数混淆**：在 `ContentContextMenu.cpp` 第 492 行（空白处右键场景）中，调用了默认的 `ContextMenuFactory::buildShowInExplorerAction(&menu, currentPath, m_panel)`，导致空白处右键弹出的菜单显示为“在“资源管理器”中显示”，且底层调用了 `explorer /select, currentPath`，在资源管理器中试图高亮当前文件夹自身（父级界面），与 Windows 用户直接打开当前目录的心理预期严重不符。
2. **缺少直接打开目录的系统 API 封装**：`ShellHelper` 仅提供了带 `/select,` 的 `openInExplorer`，缺少不带 `/select` 参数直接运行 `explorer <dirPath>` 的 `openDirectoryInExplorer` 接口。

---

## 3. 修改文件范围 (Scope of Changes)

### 核心代码文件
- `src/util/ShellHelper.h` & `src/util/ShellHelper.cpp`：新增 `openDirectoryInExplorer(const QString& dirPath)` 接口，专门用于直接打开目录（运行 `explorer <dirPath>`）。
- `src/ui/controllers/ContextMenuFactory.h` & `src/ui/controllers/ContextMenuFactory.cpp`：升级 `buildShowInExplorerAction(QMenu* menu, const QString& path, bool isBlankSpace = false, QObject* receiver = nullptr)` 接口，当 `isBlankSpace == true` 时使用文案“在“资源管理器”中打开”并触发 `openDirectoryInExplorer`。
- `src/ui/controllers/ContentContextMenu.cpp`：在空白处右键场景（第 492 行）中将 `isBlankSpace` 设为 `true`。

---

## 4. 架构契约与规范核查 (Architecture Checks)
- **【契约锁】场景语义契约**：严格遵守 `Memories.md` 规则 1.3.10，区分项定位与空白处打开目录语义。
- **【图标锁】矢量图标统一**：无论显示还是打开，图标统一使用中性单色 `#EEEEEE` 的 `folder_search` 图标。
- **【依赖锁】单向依赖**：`ContextMenuFactory` 调度 `ShellHelper` 平台接口。

---

## 5. 详细实现方案 (Implementation Details)

### 5.1 ShellHelper 新增 `openDirectoryInExplorer`
在 `ShellHelper.cpp` 中实现：
```cpp
void ShellHelper::openDirectoryInExplorer(const QString& dirPath) {
#ifdef Q_OS_WIN
    QStringList args;
    args << QDir::toNativeSeparators(dirPath);
    QProcess::startDetached("explorer", args);
#else
    Q_UNUSED(dirPath);
#endif
}
```

### 5.2 ContextMenuFactory 工厂升级
在 `ContextMenuFactory.cpp` 中升级：
```cpp
QAction* ContextMenuFactory::buildShowInExplorerAction(QMenu* menu, const QString& path, bool isBlankSpace, QObject* receiver) {
    if (!menu || path.isEmpty()) return nullptr;

    QString text = isBlankSpace ? "在“资源管理器”中打开" : "在“资源管理器”中显示";
    QAction* action = menu->addAction(UiHelper::getIcon("folder_search", QColor("#EEEEEE"), 18), text);
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [path, isBlankSpace]() {
        if (isBlankSpace) {
            ShellHelper::openDirectoryInExplorer(path);
        } else {
            ShellHelper::openInExplorer(path);
        }
    });
    return action;
}
```

### 5.3 ContentContextMenu 空白处场景绑定
在 `ContentContextMenu.cpp` 的空白处右键分支中：
```cpp
QAction* actShowInExp = ContextMenuFactory::buildShowInExplorerAction(&menu, currentPath, true, m_panel);
```

---

## 6. 代码修改 (Search & Replace Blocks)

### Block 1: `src/util/ShellHelper.h`
<<<<<<< SEARCH
    /**
     * @brief 在资源管理器中定位
     */
    static void openInExplorer(const QString& path);
=======
    /**
     * @brief 在资源管理器中定位
     */
    static void openInExplorer(const QString& path);

    /**
     * @brief 在资源管理器中打开当前目录
     */
    static void openDirectoryInExplorer(const QString& dirPath);
>>>>>>> REPLACE

---

### Block 2: `src/util/ShellHelper.cpp`
<<<<<<< SEARCH
void ShellHelper::openInExplorer(const QString& path) {
#ifdef Q_OS_WIN
    QStringList args;
    args << "/select," << QDir::toNativeSeparators(path);
    QProcess::startDetached("explorer", args);
#else
    Q_UNUSED(path);
#endif
}
=======
void ShellHelper::openInExplorer(const QString& path) {
#ifdef Q_OS_WIN
    QStringList args;
    args << "/select," << QDir::toNativeSeparators(path);
    QProcess::startDetached("explorer", args);
#else
    Q_UNUSED(path);
#endif
}

void ShellHelper::openDirectoryInExplorer(const QString& dirPath) {
#ifdef Q_OS_WIN
    QStringList args;
    args << QDir::toNativeSeparators(dirPath);
    QProcess::startDetached("explorer", args);
#else
    Q_UNUSED(dirPath);
#endif
}
>>>>>>> REPLACE

---

### Block 3: `src/ui/controllers/ContextMenuFactory.h`
<<<<<<< SEARCH
    static QAction* buildShowInExplorerAction(QMenu* menu, const QString& path, QObject* receiver = nullptr);
=======
    static QAction* buildShowInExplorerAction(QMenu* menu, const QString& path, bool isBlankSpace = false, QObject* receiver = nullptr);
>>>>>>> REPLACE

---

### Block 4: `src/ui/controllers/ContextMenuFactory.cpp`
<<<<<<< SEARCH
QAction* ContextMenuFactory::buildShowInExplorerAction(QMenu* menu, const QString& path, QObject* receiver) {
    if (!menu || path.isEmpty()) return nullptr;

    QAction* action = menu->addAction(UiHelper::getIcon("folder_search", QColor("#EEEEEE"), 18), "在“资源管理器”中显示");
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [path]() {
        ShellHelper::openInExplorer(path);
    });
    return action;
}
=======
QAction* ContextMenuFactory::buildShowInExplorerAction(QMenu* menu, const QString& path, bool isBlankSpace, QObject* receiver) {
    if (!menu || path.isEmpty()) return nullptr;

    QString text = isBlankSpace ? "在“资源管理器”中打开" : "在“资源管理器”中显示";
    QAction* action = menu->addAction(UiHelper::getIcon("folder_search", QColor("#EEEEEE"), 18), text);
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [path, isBlankSpace]() {
        if (isBlankSpace) {
            ShellHelper::openDirectoryInExplorer(path);
        } else {
            ShellHelper::openInExplorer(path);
        }
    });
    return action;
}
>>>>>>> REPLACE

---

### Block 5: `src/ui/controllers/ContentContextMenu.cpp`
<<<<<<< SEARCH
            QAction* actShowInExp = ContextMenuFactory::buildShowInExplorerAction(&menu, currentPath, m_panel);
=======
            QAction* actShowInExp = ContextMenuFactory::buildShowInExplorerAction(&menu, currentPath, true, m_panel);
>>>>>>> REPLACE

---

## 7. 三道关自检与验证步骤 (Verification)

### 1. 第一道关：静态编译核查
通过自检校验，确保 `ShellHelper::openDirectoryInExplorer` 声明/定义与 `ContextMenuFactory::buildShowInExplorerAction` 参数更新 100% 匹配无误。

### 2. 第二道关：自检脚本验证
运行方案合规性校验工具：
```bash
python3 tools/plan_verify/verify_plan.py "Implementation Plan/ShowInExplorer_Semantics_Normalization_Plan.md"
```

---

## 8. 总结 (Summary)
本实施方案精准归一化了“在资源管理器中显示/打开”的场景语义，使选中项定位（`/select`）与空白处直接打开当前目录行为彻底解耦，完美对齐 Windows 用户习惯与应用品质契约。
