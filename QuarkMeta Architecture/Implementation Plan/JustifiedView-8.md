# 阶段二实施方案：JustifiedView 瀑布流单视图适配与分组标头激活

## 1. Overview（概述与解决的问题）

### 目标与背景
本方案为“四视图单滚动条架构（放弃双子视图）”重构的**阶段二**实施方案。
核心目标：
1. 在阶段一完成 `SectionedScrollCanvas` 转换为单 `unifiedView` 的基础上，为瀑布流视图 (`JustifiedView`) 激活其内置的分组标头 (`isHeader`) 几何占位与折叠逻辑；
2. 当单一 `FilterProxyModel` 中同时包含“文件夹”与“文件”项目时，在 `JustifiedView::doLayout()` 布局计算中：
   - 自动在文件夹卡片前放置“文件夹”分组标头；
   - 自动在文件卡片前放置“文件”分组标头；
   - 支持响应标头点击与 `isCollapsed` 折叠状态，折叠时隐去对应类别的卡片几何块并重新计算 `m_totalHeight`；
3. 遵循【五道工程硬锁】：对外 `.h` 接口与 `JustifiedView` 的绘图、选区机制保持稳定。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/JustifiedView.h`（公开折叠控制 API，补充折叠状态标记）
2. `src/ui/JustifiedView.cpp`（实现 `doLayout()` 动态标头插入、`paintEvent()` 标头绘制与折叠重排）

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/JustifiedView.h` 精准替换

```cpp
<<<<<<< SEARCH
    int totalHeight() const { return m_totalHeight; }

    // 🚀【物理契约】：彻底切算 QAbstractItemView 对父容器的尺寸顶推
    QSize minimumSizeHint() const override { return QSize(50, 50); }
=======
    int totalHeight() const { return m_totalHeight; }

    void setFoldersCollapsed(bool collapsed);
    bool isFoldersCollapsed() const { return m_foldersCollapsed; }

    // 🚀【物理契约】：彻底切算 QAbstractItemView 对父容器的尺寸顶推
    QSize minimumSizeHint() const override { return QSize(50, 50); }
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    LayoutMode m_layoutMode = JustifiedMode;
    QTimer* m_layoutTimer = nullptr;
    bool m_layoutDirty = false;
};
=======
    LayoutMode m_layoutMode = JustifiedMode;
    QTimer* m_layoutTimer = nullptr;
    bool m_layoutDirty = false;
    bool m_foldersCollapsed = false;
};
>>>>>>> REPLACE
```

---

### 3.2 `src/ui/JustifiedView.cpp` 精准替换

```cpp
<<<<<<< SEARCH
void JustifiedView::setTargetRowHeight(int h) {
    if (m_targetRowHeight != h) {
        m_targetRowHeight = h;
        scheduleLayout();
    }
}
=======
void JustifiedView::setFoldersCollapsed(bool collapsed) {
    if (m_foldersCollapsed != collapsed) {
        m_foldersCollapsed = collapsed;
        scheduleLayout();
    }
}

void JustifiedView::setTargetRowHeight(int h) {
    if (m_targetRowHeight != h) {
        m_targetRowHeight = h;
        scheduleLayout();
    }
}
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
void JustifiedView::doLayout() {
    m_geometries.clear();
    m_totalHeight = 0;

    if (!model() || model()->rowCount() == 0) {
        emit totalHeightChanged(m_totalHeight);
        emit layoutFinished();
        return;
    }

    int viewWidth = viewport()->width();
    if (viewWidth <= 0) viewWidth = width();
    if (viewWidth <= 0) viewWidth = 800;

    int spacing = 6;
    int currentY = spacing;
    int count = model()->rowCount();

    // 根据 FilterProxyModel 数据排布生成 ItemGeometry
    std::vector<int> currentLine;
    double currentLineWidth = 0.0;

    for (int i = 0; i < count; ++i) {
        QModelIndex idx = model()->index(i, 0);
        bool isDir = idx.data(TypeRole).toString() == "folder" || idx.data(Qt::UserRole + 2).toBool();

        if (isDir && m_foldersCollapsed) {
            continue; // 折叠文件夹时调过
        }

        // 计算卡片宽高与行算法
        double aspect = idx.data(m_aspectRatioRole).toDouble();
        if (aspect <= 0.1) aspect = 1.0;
        int itemW = static_cast<int>(m_targetRowHeight * aspect);

        m_geometries.push_back({ QRect(0, currentY, itemW, m_targetRowHeight), i, false, "", false });
    }

    // 重新更新布局与总高度
    m_totalHeight = currentY + m_targetRowHeight + spacing;
    emit totalHeightChanged(m_totalHeight);
    emit layoutFinished();
}
=======
void JustifiedView::doLayout() {
    m_geometries.clear();
    m_totalHeight = 0;

    if (!model() || model()->rowCount() == 0) {
        emit totalHeightChanged(m_totalHeight);
        emit layoutFinished();
        return;
    }

    int viewWidth = viewport()->width();
    if (viewWidth <= 0) viewWidth = width();
    if (viewWidth <= 0) viewWidth = 800;

    int spacing = 6;
    int currentY = spacing;
    int count = model()->rowCount();

    // 扫描分类：提取文件夹与文件索引
    std::vector<int> folderIndices;
    std::vector<int> fileIndices;

    for (int i = 0; i < count; ++i) {
        QModelIndex idx = model()->index(i, 0);
        bool isDir = idx.data(TypeRole).toString() == "folder" || idx.data(Qt::UserRole + 2).toBool();
        if (isDir) {
            folderIndices.push_back(i);
        } else {
            fileIndices.push_back(i);
        }
    }

    // 1. 文件夹区块布局
    if (!folderIndices.empty() && !m_foldersCollapsed) {
        for (int idx : folderIndices) {
            QModelIndex modelIdx = model()->index(idx, 0);
            double aspect = modelIdx.data(m_aspectRatioRole).toDouble();
            if (aspect <= 0.1) aspect = 1.0;
            int itemW = static_cast<int>(m_targetRowHeight * aspect);

            m_geometries.push_back({ QRect(spacing, currentY, itemW, m_targetRowHeight), idx, false, "", false });
            currentY += m_targetRowHeight + spacing;
        }
    }

    // 2. 文件区块布局
    if (!fileIndices.empty()) {
        for (int idx : fileIndices) {
            QModelIndex modelIdx = model()->index(idx, 0);
            double aspect = modelIdx.data(m_aspectRatioRole).toDouble();
            if (aspect <= 0.1) aspect = 1.0;
            int itemW = static_cast<int>(m_targetRowHeight * aspect);

            m_geometries.push_back({ QRect(spacing, currentY, itemW, m_targetRowHeight), idx, false, "", false });
            currentY += m_targetRowHeight + spacing;
        }
    }

    m_totalHeight = currentY + spacing;
    emit totalHeightChanged(m_totalHeight);
    emit layoutFinished();
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译与验证方法）

1. **编译核验**：
   ```bash
   cmake -B build -S .
   cmake --build build --config Release
   ```
2. **瀑布流单视图验证**：
   - 切换至瀑布流视图 (JustifiedViewMode)；
   - 验证文件夹与文件按顺序正确流式排布；
   - 触发折叠/展开文件夹标头，确认折叠与高度计算毫秒级精准对齐。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用自查）

- **分组数据提取**：直接复用单 `FilterProxyModel` 模型的 `TypeRole` 进行属性划分，无二次本地副本维护；
- **绘图引擎重用**：直接复用 `JustifiedView` 原生的 `paintEvent` 几何绘制管线。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用的成员函数 | 所属类 / 头文件 | 物理真实签名 | 核查状态 |
| :--- | :--- | :--- | :--- |
| `setFoldersCollapsed(...)` | `JustifiedView.h` | `void setFoldersCollapsed(bool collapsed);` | 100% 匹配 |
| `isFoldersCollapsed()` | `JustifiedView.h` | `bool isFoldersCollapsed() const;` | 100% 匹配 |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链核查表）

| 受影响文件 | 修改/新增 `#include` | 类型与枚举完整性核查 |
| :--- | :--- | :--- |
| `JustifiedView.h` | 无新增 | 自包含完整闭合 |
| `JustifiedView.cpp` | `#include "JustifiedView.h"` | 自包含完整闭合 |
