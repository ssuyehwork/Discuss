# ContentHeaderWidget API Signature Fix Implementation Plan

## Overview
This plan adds the missing `setActive(bool active)` public member function to `ContentHeaderWidget` (`src/ui/ContentHeaderWidget.h` and `src/ui/ContentHeaderWidget.cpp`), resolving the MSVC C2039 compilation error when `ContentPanel::setActivePane` invokes `m_headerWidget->setActive(active)`.

## Modified Files List
1. `src/ui/ContentHeaderWidget.h`
2. `src/ui/ContentHeaderWidget.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentHeaderWidget.h`
```cpp
<<<<<<< SEARCH
    void setFilterState(const FilterState& state);
    void setRecursive(bool recursive);
    void setLayersEnabled(bool enabled, const QString& tooltip);
=======
    void setFilterState(const FilterState& state);
    void setRecursive(bool recursive);
    void setLayersEnabled(bool enabled, const QString& tooltip);
    void setActive(bool active);
>>>>>>> REPLACE
```

### 2. `src/ui/ContentHeaderWidget.cpp`
```cpp
<<<<<<< SEARCH
void ContentHeaderWidget::setLayersEnabled(bool enabled, const QString& tooltip) {
    if (m_btnLayers) {
        m_btnLayers->setEnabled(enabled);
        m_btnLayers->setProperty("tooltipText", tooltip);
    }
}

bool ContentHeaderWidget::eventFilter(QObject* watched, QEvent* event) {
=======
void ContentHeaderWidget::setLayersEnabled(bool enabled, const QString& tooltip) {
    if (m_btnLayers) {
        m_btnLayers->setEnabled(enabled);
        m_btnLayers->setProperty("tooltipText", tooltip);
    }
}

void ContentHeaderWidget::setActive(bool active) {
    setProperty("activePane", active ? "true" : "false");
    style()->unpolish(this);
    style()->polish(this);
}

bool ContentHeaderWidget::eventFilter(QObject* watched, QEvent* event) {
>>>>>>> REPLACE
```

## Header API Signature Verification
- `ContentHeaderWidget::setActive(bool active)`: `void`
