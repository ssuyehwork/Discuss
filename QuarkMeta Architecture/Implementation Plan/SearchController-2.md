# SearchController 搜索框高度修复实施方案 (SearchController-2.md)

## 1. Overview（概述与解决的问题）
为恢复与同排地址栏（`AddressBar`，高度 30px）及顶部导航栏全局控件的像素级一致对齐，解决搜索容器（`SearchContainer`）高度（32px）与输入框布局拉伸导致的不对齐问题。本方案将 `m_searchContainer` 尺寸严格调至 `(230, 30)`，搜索按钮 `m_btnSearch` 尺寸调至 `(28, 30)`，且对 `QLineEdit#SearchEdit` 进行标准 30px 高度约束与 QSS 样式匹配，确保界面视觉对齐与交互规范。

## 2. Modified Files List（影响文件清单）
1. `src/ui/SearchController.cpp`
2. `resources/style.qss`

## 3. Detailed Line-by-Line Changes（精准替换块）

### `src/ui/SearchController.cpp`
```
<<<<<<< SEARCH
    m_searchContainer = new QWidget(parent);
    m_searchContainer->setObjectName("SearchContainer");
    m_searchContainer->setFixedSize(230, 32);

    QHBoxLayout* searchLayout = new QHBoxLayout(m_searchContainer);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(0);

    m_searchEdit = new QLineEdit(m_searchContainer);
    m_searchEdit->setPlaceholderText("搜索...");
    m_searchEdit->setObjectName("SearchEdit");
=======
    m_searchContainer = new QWidget(parent);
    m_searchContainer->setObjectName("SearchContainer");
    m_searchContainer->setFixedSize(230, 30);

    QHBoxLayout* searchLayout = new QHBoxLayout(m_searchContainer);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(0);

    m_searchEdit = new QLineEdit(m_searchContainer);
    m_searchEdit->setPlaceholderText("搜索...");
    m_searchEdit->setObjectName("SearchEdit");
    m_searchEdit->setFixedHeight(30);
>>>>>>> REPLACE
```

### `resources/style.qss`
```
<<<<<<< SEARCH
QLineEdit#SearchEdit {
    background-color: #1E1E1E;
    border: 1px solid #333333;
    border-radius: 6px;
    color: #EEEEEE;
    padding-left: 4px;
    padding-right: 24px;
    font-size: 12px;
}
=======
QLineEdit#SearchEdit {
    background-color: #1E1E1E;
    border: 1px solid #333333;
    border-radius: 6px;
    color: #EEEEEE;
    padding-left: 4px;
    padding-right: 24px;
    font-size: 12px;
    height: 30px;
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps（编译命令与验证方法）
1. 在 build 目录运行构建命令：
   ```bash
   cmake --build build --config Release
   ```
2. 运行应用，观察顶部栏搜索输入框与地址栏 `AddressBar` 的物理像素高度，确认两者均呈 30px 高度水平完美对齐。

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）
- 维持既有 `SearchController` 控件架构与事件机制，无另起炉灶。

## 6. Header API Signature Verification（头文件 API 物理签名核查表）
- `QWidget::setFixedSize(int w, int h)`
- `QWidget::setFixedHeight(int h)`
- 均为 Qt 标准 API。

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）
- `SearchController.cpp` 中包含了 `<QLineEdit>`、`<QPushButton>`、`<QWidget>`、`<QHBoxLayout>`，类型完整无断裂。
