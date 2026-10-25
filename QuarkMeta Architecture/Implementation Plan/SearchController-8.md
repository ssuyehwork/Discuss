# SearchController-8.md

## Overview
提升搜索框 (`m_searchEdit`) 交互体验，使其占位提示文本 (`placeholderText`) 能够根据当前选择的搜索范围（“搜索：当前文件夹”或“搜索：库”）进行响应式动态切换，而非硬编码固定展示“搜索...”。

具体改造点：
1. 在 `SearchController` 中增加私有辅助方法 `updatePlaceholderText()`。
2. 构造函数初始化时依据默认搜索范围设置初始占位文本。
3. 在切换搜索范围菜单项的回调中统一触发 `updatePlaceholderText()`，保证搜索框 Placeholder 实时感知当前范围。

---

## Modified Files List
1. `src/ui/SearchController.h`
2. `src/ui/SearchController.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/SearchController.h`
```
<<<<<<< SEARCH
    void doSearch(const QString& keyword);
    void showSearchMenu();
=======
    void doSearch(const QString& keyword);
    void showSearchMenu();
    void updatePlaceholderText();
>>>>>>> REPLACE
```

### 2. `src/ui/SearchController.cpp`
```
<<<<<<< SEARCH
    m_searchEdit = new QLineEdit(m_searchContainer);
    m_searchEdit->setPlaceholderText("搜索...");
    m_searchEdit->setObjectName("SearchEdit");
=======
    m_searchEdit = new QLineEdit(m_searchContainer);
    m_searchEdit->setObjectName("SearchEdit");
    updatePlaceholderText();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    connect(actFolder, &QAction::triggered, this, [this]() {
        m_searchScope = SearchScope::CurrentFolder;
    });

    connect(actLibrary, &QAction::triggered, this, [this]() {
        m_searchScope = SearchScope::Library;
    });
=======
    connect(actFolder, &QAction::triggered, this, [this]() {
        m_searchScope = SearchScope::CurrentFolder;
        updatePlaceholderText();
    });

    connect(actLibrary, &QAction::triggered, this, [this]() {
        m_searchScope = SearchScope::Library;
        updatePlaceholderText();
    });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void SearchController::showSearchMenu() {
=======
void SearchController::updatePlaceholderText() {
    if (!m_searchEdit) return;
    if (m_searchScope == SearchScope::Library) {
        m_searchEdit->setPlaceholderText("搜索：库");
    } else {
        m_searchEdit->setPlaceholderText("搜索：当前文件夹");
    }
}

void SearchController::showSearchMenu() {
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### 1. 编译步骤：
运行 CMake 构建：
```bash
cmake --build build --config Debug
```

### 2. 验证步骤：
1. 启动应用，查看顶部搜索框默认提示文字是否为“搜索：当前文件夹”。
2. 点击搜索图标按钮调出搜索范围菜单，切换为“搜索：库”。
3. **验证**：搜索输入框为空时，占位提示文字是否同步变为“搜索：库”。
4. 再次切换回“搜索：当前文件夹”，**验证**提示文字恢复为“搜索：当前文件夹”。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- 使用 Qt 原生 `QLineEdit::setPlaceholderText` 统一控制提示文本，未创建冗余遮罩 Label。
- 状态变化统一由 `updatePlaceholderText()` 集中维护，消除硬编码重复。

---

## Header API Signature Verification
- `SearchController::updatePlaceholderText()` — `.h` 私有公约一致。
- `QLineEdit::setPlaceholderText(const QString&)` — Qt 原生 API。

---

## Header Inclusion Chain & Type Completeness Check
- `SearchController.h` 包含 `<QLineEdit>`。
- 类型完整，无头文件包含断裂隐患。
