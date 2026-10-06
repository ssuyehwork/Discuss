# Implementation Plan - TabBarWidget Responsive Width & Dynamic Text Eliding Refactoring

## 1. Overview
This implementation plan addresses the tab width behavior requirement in `TabBarWidget`:
- **When tab count is small**: Tabs stretch wider (up to a maximum width of 240px) to display longer directory paths without artificial early truncation. Title text eliding dynamically adapts to the actual component width (`width() - 56px`), replacing the hardcoded 110px limit.
- **When tab count is large**: Tabs automatically squeeze down towards the minimum width (80px), maintaining icon and close button visibility.

---

## 2. Modified Files List
- `src/ui/TabBarWidget.h`
- `src/ui/TabBarWidget.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Changes in `src/ui/TabBarWidget.h`

```
<<<<<<< SEARCH
#include <QString>
#include <QEvent>

namespace QuarkMeta {
=======
#include <QString>
#include <QEvent>
#include <QResizeEvent>

namespace QuarkMeta {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    int m_index = -1;
    QPoint m_dragStartPos;
    bool m_isDragging = false;
    QLabel* m_iconLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QPushButton* m_btnClose = nullptr;
};
=======
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateElidedTitle();

    int m_index = -1;
    QPoint m_dragStartPos;
    bool m_isDragging = false;
    QString m_rawTitle;
    QLabel* m_iconLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QPushButton* m_btnClose = nullptr;
};
>>>>>>> REPLACE
```

---

### 3.2 Changes in `src/ui/TabBarWidget.cpp`

```
<<<<<<< SEARCH
TabItemButton::TabItemButton(int index, QWidget* parent)
    : QPushButton(parent), m_index(index) {
    setObjectName("TabItem");
    setFocusPolicy(Qt::NoFocus);
    setFixedHeight(28);
    setMaximumWidth(180);
    setMinimumWidth(80);
    setCursor(Qt::PointingHandCursor);
=======
TabItemButton::TabItemButton(int index, QWidget* parent)
    : QPushButton(parent), m_index(index) {
    setObjectName("TabItem");
    setFocusPolicy(Qt::NoFocus);
    setFixedHeight(28);
    setMaximumWidth(240);
    setMinimumWidth(80);
    setCursor(Qt::PointingHandCursor);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void TabItemButton::setTabTitle(const QString& title) {
    if (m_titleLabel) {
        QFontMetrics fm(m_titleLabel->font());
        QString elided = fm.elidedText(title, Qt::ElideRight, 110);
        m_titleLabel->setText(elided);
    }
}
=======
void TabItemButton::setTabTitle(const QString& title) {
    m_rawTitle = title;
    updateElidedTitle();
}

void TabItemButton::updateElidedTitle() {
    if (!m_titleLabel || m_rawTitle.isEmpty()) return;
    int availWidth = width() - 56;
    if (availWidth < 10) availWidth = 10;
    QFontMetrics fm(m_titleLabel->font());
    QString elided = fm.elidedText(m_rawTitle, Qt::ElideRight, availWidth);
    m_titleLabel->setText(elided);
}

void TabItemButton::resizeEvent(QResizeEvent* event) {
    QPushButton::resizeEvent(event);
    updateElidedTitle();
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. Verify `src/ui/TabBarWidget.h` and `src/ui/TabBarWidget.cpp` syntax and headers.
2. Ensure `CMakeLists.txt` includes `src/ui/TabBarWidget.h` and `src/ui/TabBarWidget.cpp` for MOC and compilation.
3. Validate that `TabItemButton::resizeEvent` triggers `updateElidedTitle()` whenever tab item dimensions change due to layout stretching or squeezing.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

- **SSOT Reuse**: Reuses the existing `TabBarWidget` and `TabItemButton` class structure and Qt `QHBoxLayout` layout mechanics without inventing separate tab widgets or duplicating tab management logic.
- **No Anti-Pattern / Redundancy**: Eliminates static 110px title text elision and replaces hardcoded maximum width limits with dynamic responsive width evaluation.

---

## 6. Header API Signature Verification

| Class / Struct | Method / Member Signature | Status |
| :--- | :--- | :--- |
| `TabItemButton` | `void setTabTitle(const QString& title)` | Modified existing method |
| `TabItemButton` | `void resizeEvent(QResizeEvent* event) override` | Added QWidget virtual override |
| `TabItemButton` | `void updateElidedTitle()` | Added private helper |
| `TabItemButton` | `QString m_rawTitle` | Added private member |

---

## 7. Header Inclusion Chain & Type Completeness Check

- Added `#include <QResizeEvent>` in `src/ui/TabBarWidget.h` to supply the complete definition of `QResizeEvent` for the `resizeEvent` parameter.
- Verified `<QFontMetrics>` and `<QLabel>` inclusions in `src/ui/TabBarWidget.cpp`.
