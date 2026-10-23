# ColumnViewWidget-22.md Implementation Plan

## Overview
本实施方案旨在解决在 `src/ui/ColumnViewWidget.cpp` 中引用 `PanelLayoutManager::kBasePanelWidth` 报 `C2248: 无法访问 private 成员` 的编译报错。

### 根因与修复策略
1. **访问权限受限**：
   `PanelLayoutManager.h` 中，将全系统物理基准常量 `kBasePanelWidth`、`kContentBaseWidth`、`kSplitterHandleWidth`、`kWindowAbsoluteMinWidth` 放置在了 `private:` 访问区段内，导致外部视图无法合法引用此权威物理常量。
2. **公开物理基准真理源**：
   将这组物理基准静态常量从 `private:` 区域迁移至 `PanelLayoutManager` 的 `public:` 区域，赋予全局合法公开访问权限，既不破坏任何既有调用，又确立了全局唯一的物理基准访问契约。

---

## Modified Files List
- `src/ui/PanelLayoutManager.h`

---

## Detailed Line-by-Line Changes

### `src/ui/PanelLayoutManager.h`
```diff
<<<<<<< SEARCH
    void saveLayoutState();

private:
    void savePreImmersiveState();
    void restorePreImmersiveState();

signals:
    void layoutResetCompleted();
    void panelVisibilityChanged(const QString& panelId, bool visible);

private:
    QPointer<QMainWindow> m_mainWindow;
    QPointer<QSplitter> m_mainSplitter;

    QPointer<NavPanel> m_navPanel;
    QPointer<FavoritePanel> m_favoritePanel;
    QPointer<ContentPanel> m_contentPanel;
    QPointer<MetaPanel> m_metaPanel;
    QPointer<FilterPanel> m_filterPanel;

    // 🚀【物理基准】：面板基准 230px，QSplitter 句柄宽度 5px（旧版本原始机制，不叠加额外 margin）
    static constexpr int kBasePanelWidth = 230;
    static constexpr int kContentBaseWidth = 230;
    static constexpr int kSplitterHandleWidth = 5;
    static constexpr int kWindowAbsoluteMinWidth = 710; // 顶栏与三栏视界物理绝对下限 (3 * 230px + 10px + 10px = 710px)
};
=======
    void saveLayoutState();

    // 🚀【物理基准】：面板基准 230px，QSplitter 句柄宽度 5px（全系统唯一权威物理真理源）
    static constexpr int kBasePanelWidth = 230;
    static constexpr int kContentBaseWidth = 230;
    static constexpr int kSplitterHandleWidth = 5;
    static constexpr int kWindowAbsoluteMinWidth = 710; // 顶栏与三栏视界物理绝对下限 (3 * 230px + 10px + 10px = 710px)

private:
    void savePreImmersiveState();
    void restorePreImmersiveState();

signals:
    void layoutResetCompleted();
    void panelVisibilityChanged(const QString& panelId, bool visible);

private:
    QPointer<QMainWindow> m_mainWindow;
    QPointer<QSplitter> m_mainSplitter;

    QPointer<NavPanel> m_navPanel;
    QPointer<FavoritePanel> m_favoritePanel;
    QPointer<ContentPanel> m_contentPanel;
    QPointer<MetaPanel> m_metaPanel;
    QPointer<FilterPanel> m_filterPanel;
};
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. 运行构建：
   ```powershell
   cmake --build build --config Release
   ```
2. 确认 `C2248` 编译错误彻底消除，编译顺利通过。
