# Implementation Plan - MetaPanelColorStrip-1.md

## 1. Overview
Fixes visual alignment and scale discrepancies between the Star Rating row and Color Strip row in `MetaPanel` (`src/ui/MetaPanel.cpp` / `src/ui/ColorPicker.cpp`):
1. **Left Alignment Fix**:
   - The Star Rating row (`starLayout`) has zero left margin (`contentsMargins(0, 2, 0, 2)`), with `btnClearStar` starting at `x = 0px` (center `x = 11px`).
   - `ColorStripPicker` previously hardcoded `startX = 12px`, shifting the first color circle 12px to the right and causing noticeable left-alignment misalignment.
   - Adjusts `ColorStripPicker` rendering so that the first color circle aligns with the left margin (`startX = m_circleRadius + 2` / `startX = 2px` or center `x = 11px`), matching the left alignment of `btnClearStar`.
2. **Visual Scale Harmonization**:
   - Increases color circle diameter in `ColorStripPicker` from $14\text{px}$ ($r = 7\text{px}$) to $16\text{px}$–$18\text{px}$ ($r = 8\text{px}$–$9\text{px}$), harmonizing the visual weight of color dots with the $20\text{px} \times 20\text{px}$ star buttons ($16\text{px}$ icons).

---

## 2. Modified Files List
- `src/ui/ColorPicker.h`
- `src/ui/ColorPicker.cpp`

---

## 3. Detailed Line-by-Line Changes

```path
src/ui/ColorPicker.h
```

<<<<<<< SEARCH
    int m_circleRadius = 7;
    int m_spacing = 5;
=======
    int m_circleRadius = 9;
    int m_spacing = 4;
>>>>>>> REPLACE

```path
src/ui/ColorPicker.cpp
```

<<<<<<< SEARCH
ColorStripPicker::ColorStripPicker(const QString& currentColorHex, QWidget* parent)
    : QWidget(parent), m_selectedColor(currentColorHex) {
    // 9个直径为14像素的圆，间距为5像素。
    // 总宽度：左侧预留12像素 + 9 * 14像素圆 + 8 * 5像素间隔 + 右侧预留12像素 = 12 + 126 + 40 + 12 = 190像素
    setFixedSize(190, 26);
=======
ColorStripPicker::ColorStripPicker(const QString& currentColorHex, QWidget* parent)
    : QWidget(parent), m_selectedColor(currentColorHex) {
    // 9个直径为18像素的圆，间距为4像素，起始边距2像素（使圆心位于 x=11，与 22px 宽的清除按钮完全靠左线对齐）。
    setFixedSize(210, 26);
>>>>>>> REPLACE

<<<<<<< SEARCH
    int startX = 12; // 起始左边距
    int y = rect().height() / 2;
    
    for (int i = 0; i < m_items.size(); ++i) {
        int cx = startX + i * (14 + m_spacing) + m_circleRadius;
=======
    int startX = 2; // 起始左边距（圆心位于 2 + 9 = 11px，与星级行首个按钮 22px 宽度完美轴向对齐）
    int y = rect().height() / 2;
    int diameter = m_circleRadius * 2;
    
    for (int i = 0; i < m_items.size(); ++i) {
        int cx = startX + i * (diameter + m_spacing) + m_circleRadius;
>>>>>>> REPLACE

<<<<<<< SEARCH
void ColorStripPicker::mouseMoveEvent(QMouseEvent* event) {
    int newHovered = -1;
    int cy = rect().height() / 2;
    int startX = 12;
    for (int i = 0; i < m_items.size(); ++i) {
        int cx = startX + i * (14 + m_spacing) + m_circleRadius;
=======
void ColorStripPicker::mouseMoveEvent(QMouseEvent* event) {
    int newHovered = -1;
    int cy = rect().height() / 2;
    int startX = 2;
    int diameter = m_circleRadius * 2;
    for (int i = 0; i < m_items.size(); ++i) {
        int cx = startX + i * (diameter + m_spacing) + m_circleRadius;
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Run CMake compilation to verify `ColorPicker.cpp` compiles cleanly.
2. Launch QuarkMeta and observe `MetaPanel`:
   - Verify that the first color circle (`no_color`) in the Color Strip row aligns perfectly left with the `btnClearStar` button in the Star Rating row above it.
   - Verify that color circles ($18\text{px}$ diameter) visually balance in scale with the $20\text{px} \times 20\text{px}$ star buttons ($16\text{px}$ icons).

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `Style::getColorPalette()` as the SSOT for color palette items.
- Preserved existing `ColorStripPicker` signals and public methods.

---

## 6. Header API Signature Verification
- `ColorStripPicker` public signature remains 100% untouched.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `ColorPicker.cpp` includes `"ColorPicker.h"`, `"UiHelper.h"`, `"StyleLibrary.h"`.
- Type completeness verified.
