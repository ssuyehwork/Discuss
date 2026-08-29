# 实施方案：彻底根除 NavPanel 与 FavoritePanel 顶部 1px 蓝色焦点线 (NavPanel-1.md)

## 1. Overview（概述与解决的问题）

### 1.1 核心问题说明
在【目录导航】（`NavPanel`）与【收藏夹】（`FavoritePanel`）顶部，先前实现放置了 1 像素高、背景色为 `#007ACC` 的蓝色焦点提示线（`m_focusLine`）。根据最新的视觉设计与布局要求，该蓝色线不再需要，必须彻底清除，使得面板顶部与 Header 直接自然贴合。

### 1.2 修复方案核心设计理念
1. **彻底物理拔除 1px 蓝色焦点线**：
   - 从 `NavPanel.cpp` 和 `FavoritePanel.cpp` 的 `initUi()` 中删除 `m_focusLine` 控件的 `new` 创建、样式设置及向 `m_mainLayout` 的 `addWidget` 添加逻辑。
   - 删除 `NavPanel.h` 与 `FavoritePanel.h` 中的 `QWidget* m_focusLine` 成员变量。
2. **严禁破坏公共契约（契约锁）**：
   - `setFocusHighlight(bool)` 接口作为既有 Public API 予以保留，内部改为空实现（No-op），防止任何外部调用触发编译错误或空指针崩溃。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/NavPanel.h` - 移除 `m_focusLine` 成员变量。
2. `src/ui/NavPanel.cpp` - 移除 `initUi()` 中 `m_focusLine` 创建与 `setFocusHighlight` 显隐控制逻辑。
3. `src/ui/FavoritePanel.h` - 移除 `m_focusLine` 成员变量。
4. `src/ui/FavoritePanel.cpp` - 移除 `initUi()` 中 `m_focusLine` 创建与 `setFocusHighlight` 显隐控制逻辑。

---

## 3. Detailed Line-by-Line Changes（精准代码替换块）

### 3.1 修改 `src/ui/NavPanel.h`

```
<<<<<<< SEARCH
    QVBoxLayout* m_mainLayout = nullptr;
    QWidget* m_focusLine = nullptr;
};
=======
    QVBoxLayout* m_mainLayout = nullptr;
};
>>>>>>> REPLACE
```

---

### 3.2 修改 `src/ui/NavPanel.cpp`

```
<<<<<<< SEARCH
void NavPanel::setFocusHighlight(bool visible) {
    if (m_focusLine) m_focusLine->setVisible(visible);
}

void NavPanel::initUi() {
    // 2026-05-07 按照用户要求：修改焦点线颜色为蓝色
    m_focusLine = new QWidget(this);
    m_focusLine->setFixedHeight(1);
    m_focusLine->setStyleSheet("background-color: #007ACC;");
    m_focusLine->hide(); // 初始隐藏
    m_mainLayout->addWidget(m_focusLine);

    // 面板标题 (2026-xx-xx 按照 Plan-96：作为顶层固定标题)
=======
void NavPanel::setFocusHighlight(bool visible) {
    Q_UNUSED(visible);
}

void NavPanel::initUi() {
    // 面板标题 (2026-xx-xx 按照 Plan-96：作为顶层固定标题)
>>>>>>> REPLACE
```

---

### 3.3 修改 `src/ui/FavoritePanel.h`

```
<<<<<<< SEARCH
    QVBoxLayout* m_mainLayout = nullptr;
    QWidget* m_focusLine = nullptr;

    DropTreeView* m_favoriteView = nullptr;
=======
    QVBoxLayout* m_mainLayout = nullptr;

    DropTreeView* m_favoriteView = nullptr;
>>>>>>> REPLACE
```

---

### 3.4 修改 `src/ui/FavoritePanel.cpp`

```
<<<<<<< SEARCH
void FavoritePanel::setFocusHighlight(bool visible) {
    if (m_focusLine) m_focusLine->setVisible(visible);
}

void FavoritePanel::initUi() {
    // 顶部 1px 焦点线
    m_focusLine = new QWidget(this);
    m_focusLine->setFixedHeight(1);
    m_focusLine->setStyleSheet("background-color: #007ACC;");
    m_focusLine->hide();
    m_mainLayout->addWidget(m_focusLine);

    // 固定顶栏 Header
=======
void FavoritePanel::setFocusHighlight(bool visible) {
    Q_UNUSED(visible);
}

void FavoritePanel::initUi() {
    // 固定顶栏 Header
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 4.1 编译验证
在沙盒 Bash 环境中执行构建命令：

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --parallel 4
```

### 4.2 视觉与功能验证步骤
1. **顶部焦点线根除校验**：
   - 启动可执行文件 `./QuarkMeta`。
   - 检查【目录导航】及【收藏夹】顶部。
   - 确认无任何蓝色 1px 细线存在，面板 Header 直接贴合容器顶边缘。
