# 实施方案：SvgIcons 图标库扩展与筛选面板日期排序图标更新 (SvgIcons-1.md)

## 1. Overview（概述与解决的问题）

### 1.1 核心问题说明
1. **星级与滚动箭头 SVG 图标集扩充**：需要在全软件 SVG 图标映射字典 `src/ui/SvgIcons.h` 中注册新增的 `star-001.svg`、`star-002.svg` 及 `scroll-003.svg` 至 `scroll-010.svg` 系列矢量图标。
2. **"star" / "star_filled" 图标替换**：将现有的空心星 `"star"` 映射替换为 `star-002.svg` 的矢量内容，将实心星 `"star_filled"` 映射替换为 `star-001.svg` 的矢量内容。
3. **筛选面板日期排序图标更换**：在 `FilterPanel.cpp` 中，“创建日期”与“修改日期”分组标题右侧的升降序排序按钮，需要将原有的 `arrow_down` / `arrow_up` 统一更换为标准的新矢量图标 `scroll-010.svg`（降序）与 `scroll-007.svg`（升序）。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/SvgIcons.h` - 注册 10 个新 SVG 图标并更新 `"star"` / `"star_filled"` 映射。
2. `src/ui/FilterPanel.cpp` - 将创建日期与修改日期右侧的排序图标更新为 `scroll-010.svg` / `scroll-007.svg`。

---

## 3. Detailed Line-by-Line Changes（精准代码替换块）

### 3.1 修改 `src/ui/SvgIcons.h`

```
<<<<<<< SEARCH
        {"star", R"svg(<svg viewBox="0 0 64 64" fill="currentColor"><path d="M37.675,26.643l18.335,0l-14.834,10.777l5.666,17.438l-14.833,-10.777l-14.834,10.777l5.666,-17.438l-14.833,-10.777l18.335,0l5.666,-17.438c1.888,5.813 3.777,11.625 5.666,17.438Zm-8.407,4.026l-8.869,0l7.175,5.213l-2.74,8.435l7.175,-5.213l7.175,5.213l-2.741,-8.435l7.175,-5.213l-8.869,0l-2.74,-8.434c-0.914,2.811 -1.827,5.623 -2.741,8.434Z" fill-rule="nonzero"/></svg>)svg"},
        {"star_filled", R"svg(<svg viewBox="0 0 32 32" fill="currentColor"><path d="M16 4.588l2.833 8.719H28l-7.416 5.387 2.832 8.719L16 22.023l-7.417 5.389 2.833-8.719L4 13.307h9.167L16 4.588z"/></svg>)svg"},
=======
        {"star", R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16" fill="currentColor"><path d="M8 .25a.75.75 0 0 1 .673.418l1.882 3.815 4.21.612a.75.75 0 0 1 .416 1.279l-3.046 2.97.719 4.192a.751.751 0 0 1-1.088.791L8 12.347l-3.766 1.98a.75.75 0 0 1-1.088-.79l.72-4.194L.818 6.374a.75.75 0 0 1 .416-1.28l4.21-.611L7.327.668A.75.75 0 0 1 8 .25Zm0 2.445L6.615 5.5a.75.75 0 0 1-.564.41l-3.097.45 2.24 2.184a.75.75 0 0 1 .216.664l-.528 3.084 2.769-1.456a.75.75 0 0 1 .698 0l2.77 1.456-.53-3.084a.75.75 0 0 1 .216-.664l2.24-2.183-3.096-.45a.75.75 0 0 1-.564-.41L8 2.694Z"/></svg>)svg"},
        {"star_filled", R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16" fill="currentColor"><path d="M8 .25a.75.75 0 0 1 .673.418l1.882 3.815 4.21.612a.75.75 0 0 1 .416 1.279l-3.046 2.97.719 4.192a.751.751 0 0 1-1.088.791L8 12.347l-3.766 1.98a.75.75 0 0 1-1.088-.79l.72-4.194L.818 6.374a.75.75 0 0 1 .416-1.28l4.21-.611L7.327.668A.75.75 0 0 1 8 .25Z"/></svg>)svg"},
        {"star-001.svg", R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16" fill="currentColor"><path d="M8 .25a.75.75 0 0 1 .673.418l1.882 3.815 4.21.612a.75.75 0 0 1 .416 1.279l-3.046 2.97.719 4.192a.751.751 0 0 1-1.088.791L8 12.347l-3.766 1.98a.75.75 0 0 1-1.088-.79l.72-4.194L.818 6.374a.75.75 0 0 1 .416-1.28l4.21-.611L7.327.668A.75.75 0 0 1 8 .25Z"/></svg>)svg"},
        {"star-002.svg", R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16" fill="currentColor"><path d="M8 .25a.75.75 0 0 1 .673.418l1.882 3.815 4.21.612a.75.75 0 0 1 .416 1.279l-3.046 2.97.719 4.192a.751.751 0 0 1-1.088.791L8 12.347l-3.766 1.98a.75.75 0 0 1-1.088-.79l.72-4.194L.818 6.374a.75.75 0 0 1 .416-1.28l4.21-.611L7.327.668A.75.75 0 0 1 8 .25Zm0 2.445L6.615 5.5a.75.75 0 0 1-.564.41l-3.097.45 2.24 2.184a.75.75 0 0 1 .216.664l-.528 3.084 2.769-1.456a.75.75 0 0 1 .698 0l2.77 1.456-.53-3.084a.75.75 0 0 1 .216-.664l2.24-2.183-3.096-.45a.75.75 0 0 1-.564-.41L8 2.694Z"/></svg>)svg"},
        {"scroll-003.svg", R"svg(<svg version="1.1" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 648 648" fill="currentColor"><polygon points="162.5,613.5 162.5,42.5 486.5,328 "/></svg>)svg"},
        {"scroll-004.svg", R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 648 648" fill="currentColor"><path d="M610 504H39l285.5-324z"/></svg>)svg"},
        {"scroll-005.svg", R"svg(<svg version="1.1" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 648 648" fill="currentColor"><polygon points="486.5,42.5 486.5,613.5 162.5,328 "/></svg>)svg"},
        {"scroll-006.svg", R"svg(<svg version="1.1" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 648 648" fill="currentColor"><polygon points="28,180 619,180 323.5,504 "/></svg>)svg"},
        {"scroll-007.svg", R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 60 60" fill="currentColor"><path d="M56.5 46.7H3.6l26.4-30z"/></svg>)svg"},
        {"scroll-008.svg", R"svg(<svg version="1.1" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 648 648" fill="currentColor"><polygon points="162.5,613.5 162.5,42.5 486.5,328 "/></svg>)svg"},
        {"scroll-009.svg", R"svg(<svg version="1.1" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 648 648" fill="currentColor"><polygon points="486.5,42.5 486.5,613.5 162.5,328 "/></svg>)svg"},
        {"scroll-010.svg", R"svg(<svg version="1.1" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 60 60" fill="currentColor"><polygon points="3.6,16.7 56.5,16.7 30,46.7 "/></svg>)svg"},
>>>>>>> REPLACE
```

---

### 3.2 修改 `src/ui/FilterPanel.cpp`

```
<<<<<<< SEARCH
        QPushButton* btnSort = new QPushButton(g);
        btnSort->setFixedSize(16, 16);
        btnSort->setIcon(UiHelper::getIcon(m_createDateDesc ? "arrow_down" : "arrow_up", QColor("#B0B0B0")));
        btnSort->setFlat(true);
        btnSort->setCursor(Qt::PointingHandCursor);
        btnSort->setStyleSheet("QPushButton { background: transparent; border: none; } QPushButton:hover { background: #3E3E42; border-radius: 2px; }");
        hdrLayout->addWidget(btnSort);
        connect(btnSort, &QPushButton::clicked, this, [this, btnSort]() {
            m_createDateDesc = !m_createDateDesc;
            btnSort->setIcon(UiHelper::getIcon(m_createDateDesc ? "arrow_down" : "arrow_up", QColor("#B0B0B0")));
            rebuildDateCheckboxes(true, m_createDateDesc);
        });
=======
        QPushButton* btnSort = new QPushButton(g);
        btnSort->setFixedSize(16, 16);
        btnSort->setIcon(UiHelper::getIcon(m_createDateDesc ? "scroll-010.svg" : "scroll-007.svg", QColor("#B0B0B0")));
        btnSort->setFlat(true);
        btnSort->setCursor(Qt::PointingHandCursor);
        btnSort->setStyleSheet("QPushButton { background: transparent; border: none; } QPushButton:hover { background: #3E3E42; border-radius: 2px; }");
        hdrLayout->addWidget(btnSort);
        connect(btnSort, &QPushButton::clicked, this, [this, btnSort]() {
            m_createDateDesc = !m_createDateDesc;
            btnSort->setIcon(UiHelper::getIcon(m_createDateDesc ? "scroll-010.svg" : "scroll-007.svg", QColor("#B0B0B0")));
            rebuildDateCheckboxes(true, m_createDateDesc);
        });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        QPushButton* btnSort = new QPushButton(g);
        btnSort->setFixedSize(16, 16);
        btnSort->setIcon(UiHelper::getIcon(m_modifyDateDesc ? "arrow_down" : "arrow_up", QColor("#B0B0B0")));
        btnSort->setFlat(true);
        btnSort->setCursor(Qt::PointingHandCursor);
        btnSort->setStyleSheet("QPushButton { background: transparent; border: none; } QPushButton:hover { background: #3E3E42; border-radius: 2px; }");
        hdrLayout->addWidget(btnSort);
        connect(btnSort, &QPushButton::clicked, this, [this, btnSort]() {
            m_modifyDateDesc = !m_modifyDateDesc;
            btnSort->setIcon(UiHelper::getIcon(m_modifyDateDesc ? "arrow_down" : "arrow_up", QColor("#B0B0B0")));
            rebuildDateCheckboxes(false, m_modifyDateDesc);
        });
=======
        QPushButton* btnSort = new QPushButton(g);
        btnSort->setFixedSize(16, 16);
        btnSort->setIcon(UiHelper::getIcon(m_modifyDateDesc ? "scroll-010.svg" : "scroll-007.svg", QColor("#B0B0B0")));
        btnSort->setFlat(true);
        btnSort->setCursor(Qt::PointingHandCursor);
        btnSort->setStyleSheet("QPushButton { background: transparent; border: none; } QPushButton:hover { background: #3E3E42; border-radius: 2px; }");
        hdrLayout->addWidget(btnSort);
        connect(btnSort, &QPushButton::clicked, this, [this, btnSort]() {
            m_modifyDateDesc = !m_modifyDateDesc;
            btnSort->setIcon(UiHelper::getIcon(m_modifyDateDesc ? "scroll-010.svg" : "scroll-007.svg", QColor("#B0B0B0")));
            rebuildDateCheckboxes(false, m_modifyDateDesc);
        });
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
1. **星级图标矢量刷新校验**：
   - 启动 `./QuarkMeta`。
   - 检查内容面板及属性栏中的 5 星评级图样与打分按钮，确认空星与实心星呈现为 `star-002.svg` 和 `star-001.svg` 的最新精致矢量图样。
2. **筛选面板日期图标校验**：
   - 展开【筛选】面板中的“创建日期”与“修改日期”分组。
   - 确认标题右侧箭头按钮分别使用了 `scroll-010.svg`（向下实心三角）与 `scroll-007.svg`（向上实心三角），点击切换降序/升序流畅。
