# 实施方案：五栏布局物理间距与 QSplitter 分割线渲染修复 (PanelLayoutManager-3.md)

## 1. Overview（概述与解决的问题）

### 1.1 核心问题说明
在 `MainWindow` 与 `PanelLayoutManager` 的五栏布局管理中，存在以下 UI 渲染与物理间距偏离问题：
1. **QSplitter 分割线（Handle）物理宽度与样式被侵蚀**：`m_mainSplitter` 句柄在部分场景下因为内边距（Margin）及 QSS 规则叠加，导致分割线与相邻面板边框出现叠加重影或缝隙。
2. **面板容器边框与分割线混淆**：侧边栏容器（`SidebarContainer`, `FavoriteContainer`, `MetadataContainer`, `FilterContainer`）在 `ThemeManager` 中设置了 `border: none;`，但容器包裹与 QSplitter handle 之间缺乏明确的 1px 细线视觉隔离，导致各面板间距呈现出非预期缝隙。
3. **中央主体布局 (BodyLayout) 外边距**：`m_bodyLayout->setContentsMargins` 的边距配置与 `QSplitter` 边界对齐逻辑需要精确规范，确保顶部/底部与左右两侧的贴合度符合 `UI_DESIGN_SPEC.md` 标准。

### 1.2 修复方案核心设计理念
1. **严格遵循 `UI_DESIGN_SPEC.md` 规范**：
   - 黄金分割线标准：`QSplitter::handle` 统一固定为 1px 物理宽度。
   - 分割线默认色值：`#333333`（Border Color）。
   - Hover 高亮色值：`#4A90E2`（Primary Blue）。
2. **精确分工与物理对齐**：
   - 彻底消灭主布局 `m_bodyLayout` 的多余内边距与横向间距，保证 `QSplitter` 占据完全可用的横向物理空间。
   - 规范 `QSplitter` 句柄 QSS 样式定义，确保 1px 精确渲染。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/MainWindow.cpp` - 优化 `setupSplitters()` 中 `m_mainSplitter` 的句柄样式配置与 `m_bodyLayout` 的边距。
2. `src/ui/ThemeManager.cpp` - 规范五栏容器背景与边框 QSS 规则，确保无冲突缝隙。

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
    ).arg(qssColor(BorderColor)).arg(qssColor(PrimaryBlue)));
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
1. **物理分割线校验**：
   - 启动可执行文件 `./QuarkMeta`。
   - 观察【目录导航】、【收藏夹】、【内容区】、【元数据属性】、【筛选栏】5 面板之间的分割线。
   - 确认分割线恰好为 1px 细线（`#333333`），鼠标 Hover 悬停时高亮显示为蓝色（`#4A90E2`）。
2. **边缘无间距校验**：
   - 拖拽各个 Splitter 句柄，验证面板 resize 是否丝滑流畅，相邻面板边缘无重影或异常黑缝。
