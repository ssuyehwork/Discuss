# ColumnViewShortcuts-1.md — 列视图全局快捷键行为精准对齐实施方案

---

## 🔒 架构三问 (Architecture 3-Questions Answer)

1. **第一问（真理源溯源 SSOT）**：
   全软件当前激活作用域目录路径的唯一真理源是谁？
   - 答：在网格/列表视图下为 `ContentPanel::m_currentPath`；在列视图（Column View）下为当前获焦激活列的目录路径 `m_columnView->activePane()->currentPath()`。统一收敛通过 `ContentPanel::activePath()` 接口对外暴露。

2. **第二问（黑盒完整性 Black-box Integrity）**：
   本次修复是否符合【契约锁】规范？
   - 答：完全符合。在 `ContentPanel.h` 中扩展 `activePath()` 辅助 Getter 接口，对外签名与既有逻辑保持向后兼容；不修改任何外部只读/冻结的 `.h` 物理签名。

3. **第三问（根因 vs 症状 Root Cause Analysis）**：
   为什么在列视图的第 2 列/第 3 列按 `Ctrl+V`（粘贴）、`Ctrl+Shift+N`（新建文件夹）或 `Backspace`（退格）时行为异常？
   - 答：根本原因为 `ContentKeyHandler` 与 `ContentFileOpsHandler` 硬编码了静态根路径 `m_panel->currentPath()`（即第 1 列根目录），且 `Backspace` 错误触发了面板级的全局 `directorySelected` 导航退回。修正方法是统一切换至 `m_panel->activePath()` 动态作用域，并为 `Backspace` 接入列视图原生的 `goUpColumn()` 裁撤退回逻辑。

---

## 1. Overview（概述与解决的问题）

本方案精准修正列视图（Column View）下常用热键（`Ctrl+V` 粘贴、`Ctrl+Shift+N` 新建文件夹、`Backspace` 退格返回）的作用域定位错位问题，实现热键对当前高亮激活列（Active Column Pane）的精准响应。

### 核心改动：
1. **暴露动态激活路径接口 `activePath()`**：在 `ContentPanel` 中提供 `activePath()` 接口，若当前处于列视图则自动提取 `m_columnView->activePane()->currentPath()`，否则退回 `m_currentPath`。
2. **修正新建与粘贴目标路径**：在 `ContentFileOpsHandler::createNewItem` 和 `ContentKeyHandler` 的 `Ctrl+V` 处理流程中，统一使用 `activePath()` 作为目标目录，确保新文件夹或粘贴项落入用户当前获焦的子列中。
3. **适配 `Backspace` 退格逻辑**：在列视图下按 `Backspace` 时，优先调用 `m_columnView->goUpColumn()` 收拢/退回当前列，避免整个视图直接跳回顶层根目录。

---

## 2. Modified Files List（影响文件清单）

- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/controllers/ContentFileOpsHandler.cpp`
- `src/ui/controllers/ContentKeyHandler.cpp`

---

## 3. Detailed Line-by-Line Changes（包含 CMakeLists.txt 在内的精准替换块）

### 修改点 1：`src/ui/ContentPanel.h`
在 `ContentPanel` 头文件中声明 `activePath()` 接口。

```
<<<<<<< SEARCH
    SectionedScrollCanvas* gridCanvas() const { return m_gridCanvas; }
    SectionedScrollCanvas* listCanvas() const { return m_listCanvas; }

    // 5. 业务操作分发
=======
    SectionedScrollCanvas* gridCanvas() const { return m_gridCanvas; }
    SectionedScrollCanvas* listCanvas() const { return m_listCanvas; }

    QString activePath() const;

    // 5. 业务操作分发
>>>>>>> REPLACE
```

### 修改点 2：`src/ui/ContentPanel.cpp`
实现 `ContentPanel::activePath()` 动态作用域提取方法。

```
<<<<<<< SEARCH
ContentPanel::DataSourceType ContentPanel::dataSourceType() const {
    return (m_currentCategoryType == "path_list" || m_currentCategoryType == "search") ? DataSourceType::PathList : DataSourceType::DiskNav;
}
=======
ContentPanel::DataSourceType ContentPanel::dataSourceType() const {
    return (m_currentCategoryType == "path_list" || m_currentCategoryType == "search") ? DataSourceType::PathList : DataSourceType::DiskNav;
}

QString ContentPanel::activePath() const {
    if (m_currentViewMode == ColumnView && m_columnView) {
        ColumnViewPane* pane = m_columnView->activePane();
        if (pane && !pane->currentPath().isEmpty()) {
            return pane->currentPath();
        }
    }
    return m_currentPath;
}
>>>>>>> REPLACE
```

### 修改点 3：`src/ui/controllers/ContentFileOpsHandler.cpp`
在新建文件/文件夹逻辑中使用 `activePath()` 替换静态 `currentPath()`。

```
<<<<<<< SEARCH
void ContentFileOpsHandler::createNewItem(const QString& type) {
    if (!m_panel) return;
    QString currentPath = m_panel->currentPath();
    if (currentPath.isEmpty() || currentPath == "computer://") return;
=======
void ContentFileOpsHandler::createNewItem(const QString& type) {
    if (!m_panel) return;
    QString currentPath = m_panel->activePath();
    if (currentPath.isEmpty() || currentPath == "computer://") return;
>>>>>>> REPLACE
```

### 修改点 4：`src/ui/controllers/ContentKeyHandler.cpp`
修正 `Ctrl+V` 粘贴与 `Backspace` 导航退回在列视图下的快捷键响应行为。

```
<<<<<<< SEARCH
        if (keyEvent->key() == Qt::Key_V) {
            if (m_panel->canPaste()) {
                ClipboardService::instance().executePaste(m_panel->currentPath(), m_panel);
            }
            return true;
        }
=======
        if (keyEvent->key() == Qt::Key_V) {
            QString targetPath = m_panel->activePath();
            if (m_panel->canPaste(targetPath)) {
                ClipboardService::instance().executePaste(targetPath, m_panel);
            }
            return true;
        }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 8. 导航键
    if (keyEvent->key() == Qt::Key_Backspace) {
        QDir dir(m_panel->currentPath());
        if (dir.cdUp()) emit m_panel->directorySelected(dir.absolutePath());
        return true;
    }
=======
    // 8. 导航键
    if (keyEvent->key() == Qt::Key_Backspace) {
        if (m_panel->currentViewMode() == ContentPanel::ColumnView && m_panel->columnView()) {
            m_panel->columnView()->goUpColumn();
            return true;
        }
        QDir dir(m_panel->currentPath());
        if (dir.cdUp()) emit m_panel->directorySelected(dir.absolutePath());
        return true;
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 编译步骤：
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

### 验证步骤：
1. **新建文件夹热键验证（Ctrl+Shift+N）**：
   - 在列视图中依次点击展开至第 3 列（例如 `G:\T图片\J截图`）。
   - 按下 `Ctrl+Shift+N` 组合键。
   - **预期结果**：新建文件夹成功创建在第 3 列（`G:\T图片\J截图\新建文件夹`），而不是弹出创建在第 1 列根目录。
2. **粘贴热键验证（Ctrl+V）**：
   - 复制任意文件后，点击选中第 3 列的空白区域。
   - 按下 `Ctrl+V`。
   - **预期结果**：文件成功粘贴至第 3 列文件夹中。
3. **Backspace 退格验证**：
   - 在列视图第 3 列激活状态下按下 `Backspace`。
   - **预期结果**：第 3 列被裁撤收拢，视角焦点回退至第 2 列，软件不跳回根目录。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）

- **SSOT API 复用**：复用了 `ColumnViewWidget::activePane()` 与 `ColumnViewWidget::goUpColumn()` 既有官方接口。
- **防另起炉灶自查**：未重复编写路径提取逻辑，统一归一化引入 `ContentPanel::activePath()` 单一暴露接口。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用的成员函数 / API | 物理声明文件 | 精确物理签名 | 核查状态 |
| :--- | :--- | :--- | :--- |
| `ContentPanel::activePath` | `src/ui/ContentPanel.h` | `QString activePath() const` | ✅ 匹配一致 |
| `ContentPanel::canPaste` | `src/ui/ContentPanel.h` | `bool canPaste(const QString& targetOverride = QString()) const` | ✅ 匹配一致 |
| `ColumnViewWidget::activePane` | `src/ui/ColumnViewWidget.h` | `ColumnViewPane* activePane() const` | ✅ 匹配一致 |
| `ColumnViewWidget::goUpColumn` | `src/ui/ColumnViewWidget.h` | `void goUpColumn()` | ✅ 匹配一致 |
| `ColumnViewPane::currentPath` | `src/ui/ColumnViewWidget.h` | `QString currentPath() const` | ✅ 匹配一致 |
