# Implementation Plan - Replace Breadcrumb Text Separators with Vector SVG Icons

## 1. Overview
This implementation plan addresses the blurriness of the address bar breadcrumb separator text `">"` (`QLabel`). It replaces low-contrast text characters with crisp 12x12 vector SVG arrow icons (`chevron_right`) and higher contrast color styling (`#AAAAAA`).

## 2. Modified Files List
- `src/ui/BreadcrumbBar.cpp`

## 3. Detailed Line-by-Line Changes

```diff
<<<<<<< SEARCH
    for (qsizetype i = 0; i < parts.size(); ++i) {
        // 添加箭头/分隔符
        QLabel* sep = new QLabel(">", this);
        sep->setStyleSheet("color: #555; font-size: 10px; padding: 0 2px;");
        m_layout->addWidget(sep);
=======
    for (qsizetype i = 0; i < parts.size(); ++i) {
        // 添加箭头/分隔符 (统一采用矢量 SVG 箭头图标)
        QLabel* sep = new QLabel(this);
        sep->setPixmap(UiHelper::getIcon("chevron_right", QColor("#AAAAAA"), 12).pixmap(12, 12));
        sep->setStyleSheet("background: transparent; border: none; padding: 0 1px;");
        m_layout->addWidget(sep);
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Rebuild application using CMake & Qt build pipeline.
2. Navigate through folders in the address bar.
3. Observe the breadcrumb separator between folder levels. Verify that vector SVG arrow icons are displayed sharply with clear contrast.
