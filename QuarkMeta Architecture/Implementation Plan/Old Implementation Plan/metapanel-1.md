# Implementation Plan - Restoring MetaPanel UI and Component Rendering Sequence (`metapanel-1.md`)

## 1. Overview
This implementation plan restores `MetaPanel` to align 100% with the authoritative `Version-5` baseline design.
It eliminates illegal text symbols (`[+]`), restores missing `no_color` clearance vector SVG icons (`⊘`) for star ratings and color markers, fixes the title bar branding ("元数据属性" with `#4a90e2` database icon style), and reinstates the strictly defined top-to-bottom layout hierarchy:

1. Top Preview & Palette Box (`m_topPreviewBox`)
2. Filename Editor (`m_nameEdit`)
3. Collapsible Note Section ("备注说明")
4. Collapsible Link Section ("关联网址", with external link trigger icon)
5. Star Rating & Color Marker Box (with `no_color` clearance icons and full SVG stars/dots)
6. Collapsible Tag Management Section ("标签管理", with vector SVG `add` button)
7. Collapsible Basic Physical Properties Section ("基础属性", 7 rows including Encrypted Status)
8. Collapsible Physical Path Section ("物理路径", with dedicated "复制路径" and "打开位置" buttons)

---

## 2. Modified Files List
- `src/ui/MetaPanel.h`
- `src/ui/MetaPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/MetaPanel.h`
```
<<<<<<< SEARCH
    QPushButton* m_btnAddTag = nullptr;
=======
    QPushButton* m_btnAddTag = nullptr;
    QPushButton* m_btnNoColor = nullptr;

---

## 4. Build & Verification Steps

### Verification Steps
1. Launch application and select any file in `ContentPanel`.
2. Observe `MetaPanel` on the right:
   - Header shows "元数据属性" in blue `#4a90e2`.
   - Layout strictly follows: Preview -> Filename -> 备注说明 -> 关联网址 -> Rating & Color (with `no_color` ⊘ icons) -> 标签管理 (with pure vector `add` icon) -> 基础属性 -> 物理路径 (with 2 big buttons).
3. Verify no text symbols (`[+]`) are present and all clearance/color actions work seamlessly.
