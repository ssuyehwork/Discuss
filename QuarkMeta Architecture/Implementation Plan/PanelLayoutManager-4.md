# 实施方案：五栏布局物理间距与 QSplitter 缝隙背景色融合修复 (PanelLayoutManager-4.md)

## 1. Overview（概述与解决的问题）

### 1.1 核心问题说明
在 `MainWindow` 与 `PanelLayoutManager` 的五栏布局管理中，存在以下 UI 渲染与物理间距偏离问题：
1. **QSplitter 分割线（Handle）默认背景色被误配为灰色 BorderColor**：`m_mainSplitter` 句柄在未悬停状态下渲染了 `#333333`（灰色），导致【目录导航】、【收藏夹】、【内容区】、【元数据属性】、【筛选栏】各面板之间暴露出了明显的灰色分割线条/灰色缝隙。
2. **与标杆版本 (`Dual-mode version`) 偏离**：标杆版本中，`QSplitter::handle` 默认背景色设为主深色背景 `BackgroundDeep`（即 `#1E1E1E`），因此句柄颜色与左右面板背景完美融为一体，无任何硬质灰色线条。悬停时高亮，保持切割感与交互性。

### 1.2 修复方案核心设计理念
1. **完全对标 `Dual-mode version` 的物理融合标准**：
   - 黄金分割线默认背景色：`#1E1E1E`（`BackgroundDeep`），使句柄缝隙与两侧面板背景无缝贴合融入。
   - Hover 高亮色值：`#4A90E2`（`Primary Blue`）或悬停高亮色。
   - Handle 宽度：固定 1px 物理宽度，零外边距。
2. **消灭间距与边框冲突**：
   - 规范 `QSplitter` 句柄 QSS 样式定义，添加 `margin: 0px; padding: 0px;`。
   - 规范 `ThemeManager.cpp` 面板容器样式，避免 QSS 外边距叠加引起缝隙泄漏。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/MainWindow.cpp` - 修改 `setupSplitters()` 中 `m_mainSplitter` 的 QSS 句柄默认背景色为 `BackgroundDeep`（`#1E1E1E`）。
2. `src/ui/ThemeManager.cpp` - 规范五栏容器背景与边框 QSS 规则，增加 `margin: 0px; padding: 0px;`。

---

## 3. Detailed Line-by-Line Changes（精准代码替换块）

### 3.1 修改 `src/ui/MainWindow.cpp`

```
<<<<<<< SEARCH
    QWidget* bodyWrapper = new QWidget(centralC);
    bodyWrapper->setStyleSheet("background: transparent;");
    m_bodyLayout = new QVBoxLayout(bodyWrapper);
    m_bodyLayout->setContentsMargins(kEdgeMargin, 0, kEdgeMargin, kEdgeMargin); 
    m_bodyLayout->setSpacing(0);

    m_mainSplitter = new QSplitter(Qt::Horizontal, bodyWrapper);
    m_mainSplitter->setHandleWidth(1); 
    m_mainSplitter->setChildrenCollapsible(false);
    m_mainSplitter->setStyleSheet(QString(
        "QSplitter { background: transparent; border: none; }"
        "QSplitter::handle { background-color: %1; width: 1px; }"
        "QSplitter::handle:hover { background-color: %2; }"
    ).arg(qssColor(BorderColor)).arg(qssColor(PrimaryBlue)));
=======
    QWidget* bodyWrapper = new QWidget(centralC);
    bodyWrapper->setStyleSheet("background: transparent;");
    m_bodyLayout = new QVBoxLayout(bodyWrapper);
    m_bodyLayout->setContentsMargins(0, 0, 0, 0); 
    m_bodyLayout->setSpacing(0);

    m_mainSplitter = new QSplitter(Qt::Horizontal, bodyWrapper);
    m_mainSplitter->setHandleWidth(1); 
    m_mainSplitter->setChildrenCollapsible(false);
    m_mainSplitter->setStyleSheet(QString(
        "QSplitter { background: transparent; border: none; spacing: 0px; }"
        "QSplitter::handle { background-color: %1; width: 1px; margin: 0px; padding: 0px; }"
        "QSplitter::handle:hover { background-color: %2; }"
    ).arg(qssColor(BackgroundDeep)).arg(qssColor(PrimaryBlue)));
>>>>>>> REPLACE
```

---

### 3.2 修改 `src/ui/ThemeManager.cpp`

```
<<<<<<< SEARCH
        #SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
            background-color: #1E1E1E; border: none; border-radius: 0px;
        }
=======
        #SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
            background-color: #1E1E1E; border: none; border-radius: 0px; margin: 0px; padding: 0px;
        }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 4.1 编译验证
在沙盒 Bash 环境中依次执行以下构建命令：

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --parallel 4
```

### 4.2 视觉与功能验证步骤
1. **物理无缝背景融合校验**：
   - 启动可执行文件 `./QuarkMeta`。
   - 观察【目录导航】、【收藏夹】、【内容区】、【元数据属性】、【筛选栏】5 面板之间的分割线。
   - 确认分割线在未悬停状态下背景完全融入面板的主背景色（`#1E1E1E`），无灰色分割线条或灰色缝隙露色。
2. **Hover 高亮与 Resize 交互校验**：
   - 鼠标悬停到 Splitter 句柄位置时，背景高亮显示为蓝色（`#4A90E2`），且拖拽调整面板宽度流畅无卡顿。
