# MetaPanel Redesign Implementation Plan

## 1. Overview
This implementation plan details the complete refactoring of `MetaPanel` (`src/ui/MetaPanel.h` and `src/ui/MetaPanel.cpp`) into 8 distinct modular sections to improve metadata editing, responsiveness, visualization, and physical disk state synchronization.

The 8 modules consist of:
1. Top Preview & Palette Area (`m_topPreviewBox`) - Hides automatically when no thumbnail/palette exists to eliminate blank box padding.
2. File Name Heading & Edit (`m_nameEdit`) - Highlighted bold name edit box, validating illegal Windows characters on focus out/enter and triggering rename.
3. Rating Stars & Color Marker Bar (`m_ratingColorBox`) - 5-star rating with clear button (⊘), plus 8 color marker buttons + no-color circle.
4. Tag Management Section (`m_tagSection`) - Collapsible section with adaptive full-width `[+ 添加标签]` or trailing `[+]` capsule button triggering `TagSelectorOverlay`.
5. Note Description Section (`m_noteSection`) - Collapsible section with multiline `ElasticEdit` syncing to `.QuarkMeta.json` on focus out.
6. Link Section (`m_linkSection`) - Collapsible section with single-line `ElasticEdit`, displaying `link.svg` trigger button when non-empty to open in default browser.
7. Basic Physical Attributes Section (`m_infoSection`) - Collapsible section showing Type, Size, Dimensions, Creation/Modification/Access time, and Encryption state.
8. Physical Path Section (`m_pathSection`) - Collapsible section showing read-only full native path with `[ 复制路径 ]` and `[ 打开位置 ]` action buttons.

## 2. Modified Files List
- `src/ui/MetaPanel.h`
- `src/ui/MetaPanel.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/MetaPanel.h`
Re-structure UI member variables, collapsible section widgets, and signals/slots for the 8 modules.

### 3.2 `src/ui/MetaPanel.cpp`
Implement layout logic, dynamic collapsing behavior, overlay anchoring, URL opening via `QDesktopServices`, path copying via `QClipboard`, and explorer navigation via `ShellHelper::openInExplorer`.

## 4. Build & Verification Steps
1. Verify that selecting a file without image/palette hides the top preview box without leaving extra whitespace.
2. Verify that clicking rating stars and color buttons updates `.QuarkMeta.json`.
3. Verify that clicking `[ 复制路径 ]` copies path to clipboard and `[ 打开位置 ]` opens Explorer.
4. Verify collapsible section headers expand/collapse cleanly.
