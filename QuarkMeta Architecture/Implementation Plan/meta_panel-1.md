# Implementation Plan - MetaPanel Link Edit Internal Action & Star Spacing Fix (meta_panel-1.md)

This implementation plan refactors `MetaPanel` UI components to move the link action button exclusively inside the `QLineEdit` input control, enforces leading-edge text visibility for long URLs, reduces star rating spacing, and unifies clear rating and no-color icon buttons.

## Overview
1. **Move Open Link Button Inside LineEdit**: Remove external `m_btnOpenLink` from `linkL` layout. Keep only `m_actOpenLink` inside `m_linkEdit` using `QLineEdit::TrailingPosition`.
2. **Display Leading Edge for Long URLs**: Call `m_linkEdit->setCursorPosition(0)` after setting URL text so long link strings begin at the head (left).
3. **Tighten Star Rating Spacing**: Change `starLayout` spacing from `6px` to `2px`.
4. **Unify Clear Rating & No-Color Buttons**: Standardize `btnClearStar` and `btnNoColor` fixed size to `22x22` with `16x16` SVG icons.

---

## Modified Files List
1. `src/ui/MetaPanel.h`
2. `src/ui/MetaPanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/MetaPanel.h`
Remove `m_btnOpenLink` declaration as it is no longer placed outside `m_linkEdit`.

```git
<<<<<<< SEARCH
    // 6. 关联网址区
    QWidget* m_linkBox = nullptr;
    ElasticEdit* m_linkEdit = nullptr;
    QPushButton* m_btnOpenLink = nullptr;
=======
    // 6. 关联网址区
    QWidget* m_linkBox = nullptr;
    QLineEdit* m_linkEdit = nullptr;
    QAction* m_actOpenLink = nullptr;
>>>>>>> REPLACE
```

---

### 2. `src/ui/MetaPanel.cpp`

#### 2.1 Remove External `m_btnOpenLink` and Retain Internal `m_actOpenLink`
Remove creation, layout adding, and signal connections of external `m_btnOpenLink`.

```git
<<<<<<< SEARCH
    m_actOpenLink = m_linkEdit->addAction(UiHelper::getIcon("link", QColor("#378ADD"), 14), QLineEdit::TrailingPosition);
    m_actOpenLink->setVisible(false);

    auto handleOpenLink = [this]() {
        QString urlStr = m_linkEdit->text().trimmed();
        if (!urlStr.isEmpty()) {
            if (!urlStr.startsWith("http://") && !urlStr.startsWith("https://")) {
                urlStr = "https://" + urlStr;
            }
            QDesktopServices::openUrl(QUrl(urlStr));
        }
    };

    connect(m_actOpenLink, &QAction::triggered, this, handleOpenLink);

    m_btnOpenLink = new QPushButton(m_linkBox);
    m_btnOpenLink->setFixedSize(28, 28);
    m_btnOpenLink->setCursor(Qt::PointingHandCursor);
    m_btnOpenLink->setIcon(UiHelper::getIcon("link", QColor("#378ADD"), 14));
    m_btnOpenLink->setIconSize(QSize(14, 14));
    m_btnOpenLink->setProperty("tooltipText", "打开链接");
    m_btnOpenLink->installEventFilter(this);
    m_btnOpenLink->setStyleSheet(
        "QPushButton { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; }"
        "QPushButton:hover { background: #333333; border-color: #378ADD; }"
    );
    m_btnOpenLink->setVisible(false);
    connect(m_btnOpenLink, &QPushButton::clicked, this, handleOpenLink);

    connect(m_linkEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        bool hasText = !text.trimmed().isEmpty();
        if (m_actOpenLink) m_actOpenLink->setVisible(hasText);
        if (m_btnOpenLink) m_btnOpenLink->setVisible(hasText);
    });

    linkL->addWidget(m_linkEdit);
    linkL->addWidget(m_btnOpenLink);
=======
    m_actOpenLink = m_linkEdit->addAction(UiHelper::getIcon("link", QColor("#378ADD"), 14), QLineEdit::TrailingPosition);
    m_actOpenLink->setVisible(false);

    connect(m_actOpenLink, &QAction::triggered, this, [this]() {
        QString urlStr = m_linkEdit->text().trimmed();
        if (!urlStr.isEmpty()) {
            if (!urlStr.startsWith("http://") && !urlStr.startsWith("https://")) {
                urlStr = "https://" + urlStr;
            }
            QDesktopServices::openUrl(QUrl(urlStr));
        }
    });

    connect(m_linkEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        bool hasText = !text.trimmed().isEmpty();
        if (m_actOpenLink) m_actOpenLink->setVisible(hasText);
    });

    linkL->addWidget(m_linkEdit);
>>>>>>> REPLACE
```

#### 2.2 Reduce Star Rating Spacing
Change `starLayout->setSpacing(6)` to `starLayout->setSpacing(2)`:

```git
<<<<<<< SEARCH
    // 星级行 (清除 ⊘ + 5 星)
    QWidget* ratingRow = new QWidget(m_ratingColorBox);
    ratingRow->setStyleSheet("QWidget { background: transparent; border: none; }");
    QHBoxLayout* starLayout = new QHBoxLayout(ratingRow);
    starLayout->setContentsMargins(0, 2, 0, 2);
    starLayout->setSpacing(6);
=======
    // 星级行 (清除 + 5 星)
    QWidget* ratingRow = new QWidget(m_ratingColorBox);
    ratingRow->setStyleSheet("QWidget { background: transparent; border: none; }");
    QHBoxLayout* starLayout = new QHBoxLayout(ratingRow);
    starLayout->setContentsMargins(0, 2, 0, 2);
    starLayout->setSpacing(2);
>>>>>>> REPLACE
```

#### 2.3 Force URL Text to Display Head/Left First
Call `m_linkEdit->setCursorPosition(0)` in `MetaPanel::setURL`:

```git
<<<<<<< SEARCH
void MetaPanel::setURL(const QString& url) {
    m_isInternalUpdating = true;
    m_linkEdit->setText(url);
    if (m_actOpenLink) {
        m_actOpenLink->setVisible(!url.trimmed().isEmpty());
    }
    if (m_container) m_container->adjustSize();
    m_isInternalUpdating = false;
}
=======
void MetaPanel::setURL(const QString& url) {
    m_isInternalUpdating = true;
    m_linkEdit->setText(url);
    m_linkEdit->setCursorPosition(0);
    if (m_actOpenLink) {
        m_actOpenLink->setVisible(!url.trimmed().isEmpty());
    }
    if (m_container) m_container->adjustSize();
    m_isInternalUpdating = false;
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. Verify `meta_panel-1.md` adheres to strict Git Merge Diff syntax.
2. Confirm no modification of source files directly during the planning phase.
