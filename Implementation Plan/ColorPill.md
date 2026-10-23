# Implementation Plan - ColorPill (Monochrome SVG Icons)

## 1. Overview
This implementation plan specifies the changes required to equip context menu actions in `ColorPill.cpp` with neutral monochrome (`#EEEEEE`) SVG icons.

---

## 2. Modified Files List
- `src/ui/components/ColorPill.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Neutral Monochrome Icons in ColorPill Context Menu (`src/ui/components/ColorPill.cpp`)

```
<<<<<<< SEARCH
        QMenu menu(this);
        UiHelper::applyMenuStyle(&menu);
        QColor color = m_color;
        menu.addAction("搜索相似颜色的项目", [this, color]() { emit colorSelected(color); });
        menu.addSeparator();
        QString hex = color.name().toUpper();
        menu.addAction(QString("复制 %1").arg(hex), [hex]() { QApplication::clipboard()->setText(hex); });
        menu.addSeparator();
        menu.addAction("设置为自定义主色", [this, color]() { emit requestSetAsPrimary(color); });
        menu.exec(event->globalPosition().toPoint());
=======
        QMenu menu(this);
        UiHelper::applyMenuStyle(&menu);
        QColor color = m_color;
        menu.addAction(UiHelper::getIcon("search", QColor("#EEEEEE"), 18), "搜索相似颜色的项目", [this, color]() { emit colorSelected(color); });
        menu.addSeparator();
        QString hex = color.name().toUpper();
        menu.addAction(UiHelper::getIcon("copy", QColor("#EEEEEE"), 18), QString("复制 %1").arg(hex), [hex]() { QApplication::clipboard()->setText(hex); });
        menu.addSeparator();
        menu.addAction(UiHelper::getIcon("star_filled", QColor("#EEEEEE"), 18), "设置为自定义主色", [this, color]() { emit requestSetAsPrimary(color); });
        menu.exec(event->globalPosition().toPoint());
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Visual Verification**:
   - Left-click on a color pill in MetaPanel or FilterPanel to open the color context menu.
   - Confirm that all three context menu actions feature neutral monochrome (`#EEEEEE`) SVG icons.
