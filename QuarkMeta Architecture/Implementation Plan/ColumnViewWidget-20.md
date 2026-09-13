# ColumnViewWidget & ColumnItemDelegate Rendering Fix Plan

## 1. Overview
This plan fixes two UI issues reported in Column View mode:
1. **Missing Column Separator Line**: `QWidget#ColumnViewPane` has QSS rule `border-right: 1px solid #2D2D2D;`. However, standard QWidget subclasses in Qt do not automatically paint QSS background and borders unless `setAttribute(Qt::WA_StyledBackground, true);` is set or `paintEvent` handles style painting via `QStyleOption`. Setting `setAttribute(Qt::WA_StyledBackground, true);` in `ColumnViewPane` constructor ensures Qt paints the right border divider line configured in `resources/style.qss`.
2. **Item Rendering Alignment & Star Rating Layout**: Verifies and ensures proper drawing of items in `ColumnItemDelegate.cpp`, including background hover/selection highlight, file/folder icon positioning (18x18px), star rating `★` rendering (`#FFC107`, bold Segoe UI font), and folder chevron arrow (`chevron_right`) on the far right.

## 2. Modified Files List
- `src/ui/ColumnViewWidget.cpp`

## 3. Detailed Line-by-Line Changes

### Change 1: Enable QSS Styled Background on `ColumnViewPane` (`src/ui/ColumnViewWidget.cpp`)
```gitsaml
<<<<<<< SEARCH
ColumnViewPane::ColumnViewPane(const QString& path, ContentPanel* contentPanel, QWidget* parent)
    : QWidget(parent), m_path(path), m_contentPanel(contentPanel)
{
    setObjectName("ColumnViewPane");
    setMinimumWidth(220);
=======
ColumnViewPane::ColumnViewPane(const QString& path, ContentPanel* contentPanel, QWidget* parent)
    : QWidget(parent), m_path(path), m_contentPanel(contentPanel)
{
    setObjectName("ColumnViewPane");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(220);
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Rebuild the application using CMake.
2. Launch QuarkMeta and switch to Column View (列视图).
3. Verify that each directory column pane (`ColumnViewPane`) displays a 1px solid `#2D2D2D` vertical separator line on its right border.
4. Verify that items with star ratings display `★<rating>` correctly formatted with yellow text on the right side.
