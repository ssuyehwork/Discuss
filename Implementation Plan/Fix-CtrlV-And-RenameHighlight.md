# 实施方案：修复 Ctrl+V 偶发失效 & 重命名后高亮丢失

> **方案编号**：Fix-CtrlV-And-RenameHighlight  
> **涉及文件**：2 个（ContentKeyHandler.cpp、DiskItemModel.cpp）  
> **架构层定位**：视图层（View）→ 业务域层（Domain）单向，无逆向依赖  

---

## 一、门禁锁三问

### 1.1 SSOT 溯源
- **Ctrl+V 的 SSOT 执行入口**：`ContentPanel::performPaste()` → `ClipboardService::instance().executePaste()`，这是唯一合法粘贴入口，本方案不另起炉灶，仍走此路径。
- **重命名完成后的选中恢复 SSOT 入口**：`ContentPanel::setPendingSelectPath(newPath, false)` + 在合适时机调用 `restoreSelections()`，这是全项目统一的"刷新后选中指定项"机制。

### 1.2 黑盒完整性
- 本方案的两处修改均在各自模块内部，不新增跨模块依赖。
- `DiskItemModel::setData` 会新增一个 `emit itemRenamed(oldPath, newPath)` 信号发射，`ContentPanel` 监听此信号并调用 `setPendingSelectPath`，符合单向信号通知架构，无逆向包含。

### 1.3 根因溯源
- **Ctrl+V**：根因是 `ContentKeyHandler::handleKeyPress` 对 Ctrl+V 的处理位于所有 `view` 判断之后（L675），而 L474 的 `if (!view) return false` 会在焦点不在 View 时提前退出。
- **重命名高亮**：根因是 `DiskItemModel::setData` 重命名成功后只 `emit dataChanged`（刷新显示），但没有触发"重新选中新路径"的机制，导致 Qt 编辑器关闭后选中态自然消失。

---

## 二、能力对照表

| 场景 | 修复前 | 修复后 |
|------|--------|--------|
| 焦点在 View 上，按 Ctrl+V | ✅ 正常粘贴 | ✅ 正常粘贴（不变）|
| 焦点在 SearchBar/FilterPanel 等非 View 控件，按 Ctrl+V | ❌ 静默丢失 | ✅ 正常粘贴 |
| 焦点在 QLineEdit（行内编辑/地址栏）中，按 Ctrl+V | ✅ 正常文本粘贴（LocalShortcutInterceptor 已让渡，ContentKeyHandler L443 也 return false）| ✅ 保持原有行为，不干扰（本修复不影响此路径）|
| 重命名完成（单个文件/文件夹）后 | ❌ 高亮消失，焦点游离 | ✅ 新名称项保持高亮选中 |
| 重命名未改名（ESC 取消）后 | ✅ 维持原有（DataChanged 不 emit） | ✅ 保持原有（新信号仅在 success 时发出，不影响取消场景）|

---

## 三、修改一：修复 Ctrl+V 偶发失效

**文件**：`G:\C++\Project\Source_Code\QuarkMeta_Source\src\ui\controllers\ContentKeyHandler.cpp`

### 修改位置

**修改前**（L666-L680）：
```cpp
    // 6. Ctrl + C / X / V
    if (keyEvent->modifiers() & Qt::ControlModifier) {
        if (keyEvent->key() == Qt::Key_C && !(keyEvent->modifiers() & Qt::ShiftModifier)) {
            m_panel->performCopy(false);
            return true;
        }
        if (keyEvent->key() == Qt::Key_X) {
            m_panel->performCopy(true);
            return true;
        }
        if (keyEvent->key() == Qt::Key_V) {
            m_panel->setActivePane(true);
            m_panel->performPaste();
            return true;
        }
    }
```

**根因分析**：这段代码要在 L474 的 `if (!view) return false` **之后**才能到达，而当 `obj` 不是 View 时（焦点在 FilterPanel/SearchBar 等处），L474 已提前 return false，Ctrl+V 的处理根本不会被执行。

**修改方案**：将 Ctrl+V 单独提前到 `view` 判断之前处理。Ctrl+C 和 Ctrl+X 在焦点不在 View 时不应执行（无选中项），仅提前 Ctrl+V。

**修改后**（在 L443 `if (qobject_cast<QLineEdit*>...return false` 之后、L466 `if (keyEvent->key() == Qt::Key_Backspace)` 之前，插入 Ctrl+V 的前置提前处理）：

```cpp
bool ContentKeyHandler::handleKeyPress(QObject* obj, QEvent* event) {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    if (qobject_cast<QLineEdit*>(QApplication::focusWidget())) return false;

    if (keyEvent->key() == Qt::Key_Escape && keyEvent->modifiers() == Qt::NoModifier) {
        if (!ClipboardService::instance().cutPaths().isEmpty()) {
            ClipboardService::instance().clearCutState();
            return true;
        }
    }

    // 局部 F5 刷新当前聚焦窗格，坚决阻断按键冒泡至全局引发的主窗格错误刷新
    if (keyEvent->key() == Qt::Key_F5 && keyEvent->modifiers() == Qt::NoModifier) {
        m_panel->setActivePane(true);
        m_panel->refreshAll();
        return true;
    }

    // ══════════════════════════════════════════════════════════════
    // [修复] Ctrl+V 提前处理：解除对 obj 是 View 的强依赖。
    // 粘贴行为与当前焦点控件无关，只要面板可粘贴即执行。
    // Ctrl+C/X 不提前（无选中项时执行无意义），仅提前 V。
    // ══════════════════════════════════════════════════════════════
    if ((keyEvent->modifiers() & Qt::ControlModifier) &&
        !(keyEvent->modifiers() & Qt::ShiftModifier) &&
        keyEvent->key() == Qt::Key_V) {
        m_panel->setActivePane(true);
        m_panel->performPaste();
        return true;
    }

    // 面板级全局快捷键（独立于 QAbstractItemView 焦点状态）
    if (keyEvent->modifiers() & Qt::ControlModifier) {
        if (keyEvent->key() == Qt::Key_S) {
            m_panel->toggleFolderSectionCollapse();
            return true;
        }
    }
    if (keyEvent->key() == Qt::Key_Backspace) {
        QDir dir(m_panel->currentPath());
        if (dir.cdUp()) emit m_panel->directorySelected(dir.absolutePath());
        return true;
    }

    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(obj);
    if (!view) view = qobject_cast<QAbstractItemView*>(obj->parent());
    if (!view) return false;

    // ... 以下所有 view 相关逻辑保持原样不变 ...
    // （原 L476 起的所有代码，包括 Ctrl+C/X 以及最后的 Ctrl+V 分支）
    // 注意：原 L675-679 的 Ctrl+V 分支在 view 存在时依然会被触发，
    //       但因提前分支已 return true，该分支永远不会被重复触发。
    //       将原 L675-679 的 Ctrl+V 分支【删除】，避免冗余（见下方说明）。
```

### 配套：删除原来 view 判断后的 Ctrl+V 冗余分支

**原 L665-680 中的 Ctrl+V 部分**（删除 `if (keyEvent->key() == Qt::Key_V)` 这一块，保留 Ctrl+C 和 Ctrl+X）：

```cpp
    // 6. Ctrl + C / X / V
    if (keyEvent->modifiers() & Qt::ControlModifier) {
        if (keyEvent->key() == Qt::Key_C && !(keyEvent->modifiers() & Qt::ShiftModifier)) {
            m_panel->performCopy(false);
            return true;
        }
        if (keyEvent->key() == Qt::Key_X) {
            m_panel->performCopy(true);
            return true;
        }
        // ← Ctrl+V 已移至 view 判断之前处理，此处删除，不保留
    }
```

---

## 四、修改二：修复重命名后高亮丢失

这个问题需要两步：**① 在 DiskItemModel 中发出重命名完成信号；② 在 ContentPanel 中监听信号并触发选中恢复**。

### 修改 2a：DiskItemModel.h ——新增信号声明

**文件**：`G:\C++\Project\Source_Code\QuarkMeta_Source\src\ui\models\DiskItemModel.h`

在现有 `signals:` 区块（L62-64）中，紧随 `thumbnailLoaded` 信号之后，新增一行：

```cpp
signals:
    void thumbnailLoaded(int rowIndex);
    // [新增] 用于重命名完成后通知上层恢复选中态（SSOT: ContentPanel::setPendingSelectPath）
    void itemRenamed(const QString& oldPath, const QString& newPath);
```

### 修改 2b：DiskItemModel.cpp ——在 setData 成功后发射信号

**文件**：`G:\C++\Project\Source_Code\QuarkMeta_Source\src\ui\models\DiskItemModel.cpp`

在 `DiskItemModel::setData` 的 `if (success)` 块中（当前 L529-538），紧在 `emit dataChanged(...)` 之后新增信号发射：

**修改前**（L529-538）：
```cpp
        if (success) {
            QString newPath = QDir(oldInfo.absolutePath()).filePath(newName);
            record.path = newPath;
            record.filename = newName;
            
            m_pathToIndex.erase(oldPath);
            m_pathToIndex[newPath] = index.row();
            
            emit dataChanged(this->index(index.row(), 0), this->index(index.row(), columnCount() - 1), {Qt::EditRole, Qt::DisplayRole});
            return true;
        }
```

**修改后**：
```cpp
        if (success) {
            QString newPath = QDir(oldInfo.absolutePath()).filePath(newName);
            record.path = newPath;
            record.filename = newName;
            
            m_pathToIndex.erase(oldPath);
            m_pathToIndex[newPath] = index.row();
            
            emit dataChanged(this->index(index.row(), 0), this->index(index.row(), columnCount() - 1), {Qt::EditRole, Qt::DisplayRole});
            // [新增] 通知上层（ContentPanel）重命名已完成，请用新路径恢复选中高亮
            emit itemRenamed(oldPath, newPath);
            return true;
        }
```

### 修改 2c：ContentPanel.cpp ——监听信号并触发选中恢复

**文件**：`G:\C++\Project\Source_Code\QuarkMeta_Source\src\ui\ContentPanel.cpp`

在构造函数中现有的 `m_diskModel` 信号连接区块（当前 L80-87 的 `dataChanged` 连接之后），新增对 `itemRenamed` 的监听：

**修改前**（L80-87，仅展示上下文，不修改此段）：
```cpp
    connect(m_diskModel, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex&, const QModelIndex&, const QList<int>& roles) {
        if (roles.isEmpty() || roles.contains(RatingRole) || roles.contains(ColorRole) || roles.contains(TagsRole) || roles.contains(HasThumbnailRole)) {
            if (m_statsDebounceTimer) {
                m_statsDebounceTimer->start();
            }
        }
    });
```

**在此之后插入**（约在 L87 之后）：
```cpp
    // [新增] 监听 DiskItemModel 的重命名完成信号，使用 QueuedConnection 确保
    // 在 Qt 编辑器（QAbstractItemDelegate::closeEditor）完成关闭、选区被系统清除之后，
    // 再触发新路径的选中恢复，避免与 Qt 内部的编辑器关闭流程产生竞争。
    connect(m_diskModel, &DiskItemModel::itemRenamed, this, [this](const QString& /*oldPath*/, const QString& newPath) {
        setPendingSelectPath(newPath, false);
        // 必须用 QueuedConnection 语义（singleShot(0)）延迟执行，
        // 因为此时 closeEditor 尚未完成，立即调用 restoreSelections 会被 Qt 清除覆盖。
        QTimer::singleShot(0, this, &ContentPanel::restoreSelections);
    }, Qt::QueuedConnection);
```

> **注意**：`Qt::QueuedConnection` 本身已保证异步（下一个事件循环轮次），等效于 `QTimer::singleShot(0)`。两者择其一即可。推荐直接使用 `Qt::QueuedConnection`，代码更简洁：

```cpp
    connect(m_diskModel, &DiskItemModel::itemRenamed, this,
            [this](const QString& /*oldPath*/, const QString& newPath) {
                setPendingSelectPath(newPath, false);
                QMetaObject::invokeMethod(this, &ContentPanel::restoreSelections, Qt::QueuedConnection);
            }, Qt::DirectConnection);
```

---

## 五、三道关自检

### 关 1：物理签名核查
- `DiskItemModel.h` 新增 `itemRenamed(const QString&, const QString&)` 信号——信号无调用方编译契约约束，无破坏既有调用。
- `ContentKeyHandler.cpp` 无签名变动，只是调整函数体内部逻辑顺序。
- `DiskItemModel.cpp` 无签名变动，仅在已有 `if (success)` 块内追加一行 `emit`。
- `ContentPanel.cpp` 无签名变动，仅在构造函数内追加 `connect`。

### 关 2：行为变化醒目标注

| 变化项 | 变化描述 | 影响评估 |
|--------|----------|---------|
| ⚠️ Ctrl+V 提前处理 | 原来：焦点不在 View 时 Ctrl+V 静默失效；修复后：只要 `m_panel->canPaste()` 为 true 就执行 | 可接受，符合用户预期。`canPaste()` 内部已有路径合法性判断保护 |
| ⚠️ 重命名后多一次选中动作 | 原来：重命名后无选中态；修复后：重命名完成后异步触发一次 select | 可接受，属于缺失行为的补全，无副作用风险 |
| ✅ 无行为变化 | Ctrl+C、Ctrl+X、ESC、F5、Backspace、所有星级/色标快捷键等均不受影响 | — |

### 关 3：编译依赖核查
- `ContentPanel.cpp` 已包含 `DiskItemModel.h`（通过 `m_diskModel` 成员可见），`itemRenamed` 信号可直接连接，无需新增 `#include`。
- `ContentKeyHandler.cpp` 已包含 `ContentPanel.h` 和 `ClipboardService.h`，新增的 Ctrl+V 提前处理无需额外头文件。

---

## 六、修改范围汇总

| 文件 | 修改性质 | 行范围 |
|------|----------|--------|
| `src/ui/controllers/ContentKeyHandler.cpp` | 在 `handleKeyPress` 中：①新增 Ctrl+V 前置处理块（view 判断之前）；②删除原 view 判断之后的 Ctrl+V 分支 | 约 L441-480（插入），L675-679（删除）|
| `src/ui/models/DiskItemModel.h` | 在 `signals:` 区块新增 `itemRenamed` 信号声明 | L64（插入一行）|
| `src/ui/models/DiskItemModel.cpp` | 在 `setData` 的 `if (success)` 块末尾，`emit dataChanged` 之后追加 `emit itemRenamed` | L538（插入一行）|
| `src/ui/ContentPanel.cpp` | 在构造函数中追加 `connect(m_diskModel, &DiskItemModel::itemRenamed, ...)` | 约 L87（插入 ~5 行）|
