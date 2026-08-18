# QuarkMeta 五栏界面布局与拉伸系数无脑修复方案 (FiveColumnLayoutFix)

## 1. 问题背景与根因分析

在进行内存模式代码清理和收藏夹剥离为独立第二栏（`FavoritePanel`）后，`MainWindow.cpp` 中关于拆分条（`m_mainSplitter`）的控件组装顺序与 `setStretchFactor` 设置产生了严重不匹配，导致了极其傻逼的布局错乱现象：

### 根本原因：
1. **拉伸系数索引打错位**：
   原代码中：
   ```cpp
   m_mainSplitter->setStretchFactor(0, 0); // 目录导航
   m_mainSplitter->setStretchFactor(1, 1); // 错误：把索引 1（第二栏 FavoritePanel 收藏夹）设为了 1 (主拉伸区)！
   m_mainSplitter->setStretchFactor(2, 0); // 错误：把索引 2（第三栏 ContentPanel 内容区）设为了 0 (固定不拉伸)！
   m_mainSplitter->setStretchFactor(3, 0); // 元数据
   ```
   **后果**：窗口一最大化或拉伸，第二栏“收藏夹”瞬间无限膨胀膨胀占满中间巨大的黑色区域，而真正的内容展示区反被压缩在最右边小角落！

2. **默认尺寸分配数组未包含 5 栏规范**：
   `setSizes` 和重置布局里的尺寸分配仍使用了旧逻辑或不精准比例。

3. **样式表 QSS 缺失 `#FavoriteContainer` 控件选择器**：
   全局 `style.qss` 统一边框和背景选择器中缺少 `#FavoriteContainer`，导致第二栏在特定情况下出现边框缺失或分割感不清晰的问题。

---

## 2. 涉及修改文件路径与精确代码行号

| 文件路径 | 精确代码行号/位置 | 修改说明 |
| :--- | :--- | :--- |
| `src/ui/MainWindow.cpp` | **Line 314 - Line 334** | 修正 `m_mainSplitter->setStretchFactor` 与初始 `setSizes` 的对应索引 |
| `src/ui/MainWindow.cpp` | **Line 1801 - Line 1806** | 修正 `resetSplitterLayout()` 重置分栏的 `setSizes` 默认数组 |
| `resources/style.qss` | **Line 15** | 在 `#SidebarContainer, #ListContainer` 选择器组中补全 `#FavoriteContainer` |

---

## 3. 无脑修复实施方案步骤（步骤与代码落地）

### 步骤 1：修复 `src/ui/MainWindow.cpp` 中的 Splitter 拉伸系数与初始宽度

打开 `src/ui/MainWindow.cpp`，定位到 `MainWindow::initUi()` 内部（约第 314 行 - 第 334 行）：

#### 修改前代码：
```cpp
    // 2026-04-11 按照用户要求：物理锁定侧边栏宽度，最大化时仅“内容”区拉伸
    m_mainSplitter->setStretchFactor(0, 0); // 目录导航
    m_mainSplitter->setStretchFactor(1, 1); // 内容 (主拉伸区)
    m_mainSplitter->setStretchFactor(2, 0); // 元数据
    m_mainSplitter->setStretchFactor(3, 0); // 筛选

    // 1. 先应用面板显隐状态
    loadPanelVisibility();

    // 2. 延迟至下一个事件循环（等窗口 geometry 稳定后）再恢复 SplitterState
    QByteArray state = AppConfig::instance().getValue("MainWindow/SplitterState").toByteArray();
    if (!state.isEmpty()) {
        QTimer::singleShot(0, this, [this, state]() {
            m_mainSplitter->restoreState(state);
        });
    } else {
        // 初始默认分配: 200 << 200 << 550 << 200 << 200
        QList<int> sizes;
        sizes << 200 << 200 << 550 << 200 << 200;
        m_mainSplitter->setSizes(sizes);
    }
```

#### 替换为精确修复后代码：
```cpp
    // 物理锁定：主界面从左到右共 5 栏（索引 0:目录导航, 1:收藏夹, 2:内容展示区, 3:元数据, 4:筛选）
    // 严格确保仅有第三栏“内容展示区”（索引 2）具备拉伸系数 1，其余 4 栏全部锁定为 0！
    m_mainSplitter->setStretchFactor(0, 0); // 第一栏：目录导航 (NavPanel) -> 固定不拉伸
    m_mainSplitter->setStretchFactor(1, 0); // 第二栏：收藏夹 (FavoritePanel) -> 严格固定不拉伸！
    m_mainSplitter->setStretchFactor(2, 1); // 第三栏：内容展示区 (ContentPanel) -> 全界面唯一核心主拉伸区！
    m_mainSplitter->setStretchFactor(3, 0); // 第四栏：元数据属性栏 (MetaPanel) -> 固定不拉伸
    m_mainSplitter->setStretchFactor(4, 0); // 第五栏：条件筛选栏 (FilterPanel) -> 固定不拉伸

    // 1. 先应用面板显隐状态
    loadPanelVisibility();

    // 2. 延迟至下一个事件循环（等窗口 geometry 稳定后）再恢复 SplitterState
    QByteArray state = AppConfig::instance().getValue("MainWindow/SplitterState").toByteArray();
    if (!state.isEmpty()) {
        QTimer::singleShot(0, this, [this, state]() {
            m_mainSplitter->restoreState(state);
        });
    } else {
        // 5 栏标准初始默认分配: 200px | 200px | 550px | 200px | 200px
        QList<int> sizes;
        sizes << 200 << 200 << 550 << 200 << 200;
        m_mainSplitter->setSizes(sizes);
    }
```

---

### 步骤 2：修复 `src/ui/MainWindow.cpp` 中的 `resetSplitterLayout()` 重置逻辑

定位到 `src/ui/MainWindow.cpp` 中的 `resetSplitterLayout()` 函数（约第 1801 行）：

#### 修改前代码：
```cpp
    // 2. 物理恢复尺寸比例
    QList<int> sizes;
    sizes << 200 << 200 << 550 << 200 << 200;
    if (m_mainSplitter->count() > 5) sizes << 0;

    m_mainSplitter->setSizes(sizes);
```

#### 替换为精确修复后代码：
```cpp
    // 2. 物理恢复 5 栏尺寸比例 (Index 0: NavPanel, Index 1: FavoritePanel, Index 2: ContentPanel, Index 3: MetaPanel, Index 4: FilterPanel)
    QList<int> sizes;
    sizes << 200 << 200 << 550 << 200 << 200;
    if (m_mainSplitter->count() > 5) sizes << 0; // 索引 5 为隐藏的 TagManagerView

    m_mainSplitter->setSizes(sizes);

    // 重新强制刷新 stretchFactor，确保无脑切回标准
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 0);
    m_mainSplitter->setStretchFactor(2, 1);
    m_mainSplitter->setStretchFactor(3, 0);
    m_mainSplitter->setStretchFactor(4, 0);
```

---

### 步骤 3：补全 `resources/style.qss` 中的选择器名称

打开 `resources/style.qss`，定位到第 15 行：

#### 修改前代码：
```qss
#SidebarContainer, #ListContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
    background-color: #1E1E1E;
    border: 1px solid #2D2D30;
    border-radius: 0px;
}
```

#### 替换为精确修复后代码：
```qss
#SidebarContainer, #FavoriteContainer, #ListContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
    background-color: #1E1E1E;
    border: 1px solid #2D2D30;
    border-radius: 0px;
}
```

---

## 4. 修复效果预期与验证方法

1. **界面布局对齐**：
   启动 QuarkMeta 后，主视图从左到右依次平铺：
   - 1 栏（目录导航：200px）
   - 2 栏（收藏夹：200px，垂直贯通）
   - 3 栏（内容展示区：自适应拉伸占满其余空间）
   - 4 栏（元数据属性栏：200px）
   - 5 栏（条件筛选栏：200px）
2. **窗口拉伸验证**：
   拖拽窗口边框或点击右上角最大化，**仅第三栏（内容展示区）随之放大**，第二栏“收藏夹”绝对不会出现大片黑色空白膨胀的情况。
3. **分割线验证**：
   各栏之间均拥有 5px 宽的深色物理分割手柄（`handle`），鼠标悬停可正常拖拽调节各栏宽度。
