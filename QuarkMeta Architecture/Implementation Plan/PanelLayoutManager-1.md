# Implementation Plan: PanelLayoutManager-1

## 1. Overview
将全应用五大面板（`NavPanel`, `FavoritePanel`, `ContentPanel`, `MetaPanel`, `FilterPanel`）的物理基准宽度与最小宽度限制从 230px 统一修改为 228px，确保各栏区在默认状态下呈现精确的 228 像素宽度。

## 2. Modified Files List
- `src/ui/NavPanel.cpp`
- `src/ui/FavoritePanel.cpp`
- `src/ui/ContentPanel.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/MetaPanel.cpp`
- `src/ui/FilterPanel.cpp`
- `src/ui/MainWindow.cpp`
- `src/ui/PanelLayoutManager.h`

## 3. Detailed Line-by-Line Changes

### `src/ui/NavPanel.cpp`
<<<<<<< SEARCH
    setMinimumWidth(230);
=======
    setMinimumWidth(228);
>>>>>>> REPLACE

### `src/ui/FavoritePanel.cpp`
<<<<<<< SEARCH
    setMinimumWidth(230);
=======
    setMinimumWidth(228);
>>>>>>> REPLACE

### `src/ui/ContentPanel.cpp`
<<<<<<< SEARCH
    setMinimumWidth(230);
=======
    setMinimumWidth(228);
>>>>>>> REPLACE

### `src/ui/ContentPanel.h`
<<<<<<< SEARCH
    // 🚀【物理沙盒契约】：硬性向外报告 230px 下限，切断内部组件尺寸反向渗透
    QSize minimumSizeHint() const override { return QSize(230, 100); }
=======
    // 🚀【物理沙盒契约】：硬性向外报告 228px 下限，切断内部组件尺寸反向渗透
    QSize minimumSizeHint() const override { return QSize(228, 100); }
>>>>>>> REPLACE

### `src/ui/MetaPanel.cpp`
<<<<<<< SEARCH
    setMinimumWidth(230);
=======
    setMinimumWidth(228);
>>>>>>> REPLACE

### `src/ui/FilterPanel.cpp`
<<<<<<< SEARCH
    setMinimumWidth(230);
=======
    setMinimumWidth(228);
>>>>>>> REPLACE

### `src/ui/MainWindow.cpp`
<<<<<<< SEARCH
        sizes << 230 << 230 << 230 << 230 << 230;
=======
        sizes << 228 << 228 << 228 << 228 << 228;
>>>>>>> REPLACE

### `src/ui/PanelLayoutManager.h`
<<<<<<< SEARCH
    // 🚀【紧凑物理基准】：内容面板基准设为 230，总和严格等于 1180px
    static constexpr int kBasePanelWidth = 230;
    static constexpr int kContentBaseWidth = 230;
=======
    // 🚀【物理基准】：面板基准设为 228px
    static constexpr int kBasePanelWidth = 228;
    static constexpr int kContentBaseWidth = 228;
>>>>>>> REPLACE

## 4. Build & Verification Steps
1. 核对所有 8 个修改文件，确认 minimumWidth 与 basePanelWidth 均已精确更新为 228。
2. 确认 API 契约与公开接口无任何破坏。
