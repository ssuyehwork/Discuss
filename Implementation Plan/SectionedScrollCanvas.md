# Implementation Plan - SectionedScrollCanvas.md

## 1. Overview
本实施方案旨在解决 QuarkMeta 资产管理桌面应用在文件/文件夹元素较少（即内容高度未达到 `QScrollArea` 物理视口高度）时，`SectionedScrollCanvas` 中的分组与子视图产生非预期的垂直拉伸变形（Vertical Layout Stretch）视觉 Bug。

### 根因分析
1. `SectionedScrollCanvas` 内部使用 `QVBoxLayout` 排布文件夹分区标题 (`FolderSectionHeaderBar`)、文件夹视图 (`m_folderView`)、文件分区标题 (`FileSectionHeaderBar`) 和文件视图 (`m_fileView`)。
2. 现有代码在构造函数中通过 `m_layout->addWidget(m_fileView)` 将 `m_fileView` 添加至 `m_layout` 时，未设置 Stretch Factor，且布局末尾缺少底部的弹簧 (Spacer/Stretch)。
3. 当 `m_containerWidget` 在 `QScrollArea` 中被拉伸至视口高度（如 800px），而子视图总固定高度仅有（例如 150px）时，`QVBoxLayout` 的默认空间分配策略会将多余的 650px 垂直空间按比例/平分强行分配给包含的子部件，导致 `m_fileView` 或各 Sections 之间的垂直间距/尺寸发生拉伸变形。

### 解决方案
1. 在 `SectionedScrollCanvas::SectionedScrollCanvas` 中，将 `m_layout` 底部添加一个伸缩因子为 1 的弹簧 (`m_layout->addStretch(1)`)。
2. 确保 `m_folderHeader`、`m_folderView`、`m_fileHeader` 和 `m_fileView` 在 `m_layout` 中的 Stretch Factor 均为 0 (`m_layout->addWidget(..., 0)`)。
3. 当元素较少时，`addStretch(1)` 会自动吸收 `m_containerWidget` 视口下方的全部剩余空白空间，将文件夹与文件视图紧凑地“挤压”在顶部（Top Alignment），保持元素与标题之间的物理间距完全固定，彻底杜绝拉伸变形。

---

## 2. Modified Files List
- `src/ui/SectionedScrollCanvas.cpp`

---

## 3. Detailed Line-by-Line Changes

### File: `src/ui/SectionedScrollCanvas.cpp`

```
<<<<<<< SEARCH
    // 标题栏
    m_folderHeader = new FolderSectionHeaderBar(m_containerWidget);
    m_folderHeader->hide();
    m_layout->addWidget(m_folderHeader);

    initViews(eventFilter);

    m_fileHeader = new FileSectionHeaderBar(m_containerWidget);
    m_fileHeader->hide();
    m_layout->addWidget(m_fileHeader);

    m_layout->addWidget(m_fileView);

    setWidget(m_containerWidget);
=======
    // 标题栏
    m_folderHeader = new FolderSectionHeaderBar(m_containerWidget);
    m_folderHeader->hide();
    m_layout->addWidget(m_folderHeader, 0);

    initViews(eventFilter);

    m_fileHeader = new FileSectionHeaderBar(m_containerWidget);
    m_fileHeader->hide();
    m_layout->addWidget(m_fileHeader, 0);

    m_layout->addWidget(m_fileView, 0);
    m_layout->addStretch(1);

    setWidget(m_containerWidget);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        m_folderView = folderJv;
        m_folderView->hide();
        m_layout->addWidget(m_folderView);
=======
        m_folderView = folderJv;
        m_folderView->hide();
        m_layout->addWidget(m_folderView, 0);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        m_folderView = folderTv;
        m_folderView->hide();
        m_layout->addWidget(m_folderView);
=======
        m_folderView = folderTv;
        m_folderView->hide();
        m_layout->addWidget(m_folderView, 0);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### 构建步骤
在项目根目录运行 CMake 构建：
```bash
cmake -B build -S .
cmake --build build --config Debug
```

### 验证方法
1. **少量文件/文件夹测试**：打开一个仅有 1~2 个文件或文件夹的目录。
2. **拉伸窗口测试**：将主窗口最大化或拉高视口高度，观察 `ContentPanel` 内容区域。
3. **预期表现**：
   - 文件夹/文件卡片与标题栏紧凑置顶排布；
   - 元素之间保持原本设定的固定间距；
   - 下方剩余空间表现为自然留白（由 `addStretch(1)` 填充），绝对不发生任何垂直拉伸或平分间距现象。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- [x] **复用性检查**：本次修改直接作用于 `SectionedScrollCanvas` 容器布局管理，严格遵守 UI 布局与伸缩策略规范。
- [x] **零另起炉灶**：未新增重复布局类，未改动任何控件内部计算逻辑。
- [x] **零参数篡改**：完全保持既有 margins (0,0,0,0) 与 spacing (0)，完全符合《Zero-Value-Alteration Contract》。

---

## 6. Header API Signature Verification

| 调用的成员/类 | 头文件物理声明源 | 头文件物理精确签名 | 核查状态 |
| :--- | :--- | :--- | :--- |
| `QVBoxLayout::addWidget` | `<QVBoxLayout>` (Qt standard) | `void addWidget(QWidget *widget, int stretch = 0, Qt::Alignment alignment = Qt::Alignment())` | 物理核实一致 |
| `QVBoxLayout::addStretch` | `<QVBoxLayout>` (Qt standard) | `void addStretch(int stretch = 0)` | 物理核实一致 |
| `SectionedScrollCanvas` | `src/ui/SectionedScrollCanvas.h` | `explicit SectionedScrollCanvas(...)` | 物理核实一致 |
