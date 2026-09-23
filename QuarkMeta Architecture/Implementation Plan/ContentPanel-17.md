# ContentPanel-17: Double Top Border Fix in Split View Mode

## Overview
In multi-pane / split view mode, the outer `ContentPanel` host object (`setObjectName("EditorContainer")`) and the inner pane containers / sub-`ContentPanel`s (`setObjectName("EditorContainer")` or `#EditorContainer`) both apply a 1px solid border (`#333333` or `#555555`). Because the outer container wraps `QSplitter` and inner pane containers with 0 margins, the outer top border and inner top borders overlay directly, causing the top border above `ContentHeaderWidget` to appear 2px thick (double border).

To fix this issue:
1. When split mode is activated (`m_isSplit = true`), set `setProperty("isHostPanel", true)` on the outer host `ContentPanel`, so QSS can target `#EditorContainer[isHostPanel="true"]` to set `border: none;`.
2. When split mode is exited/closed (`m_isSplit = false`), update `setProperty("isHostPanel", false)` on the outer host `ContentPanel` so single-pane mode restores its normal 1px border.
3. Add `#EditorContainer[isHostPanel="true"] { border: none; }` to `resources/style.qss`.

## Modified Files List
- `resources/style.qss`
- `src/ui/ContentPanel.cpp`

## Detailed Line-by-Line Changes

### 1. `resources/style.qss`

```
<<<<<<< SEARCH
#EditorContainer[activePane="true"], ContentPanel[activePane="true"] {
    border: 1px solid #555555;
}
=======
#EditorContainer[activePane="true"], ContentPanel[activePane="true"] {
    border: 1px solid #555555;
}

#EditorContainer[isHostPanel="true"] {
    border: none;
}
>>>>>>> REPLACE
```

### 2. `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
    if (!m_paneSplitter) {
        m_isSplit = true;

        m_paneSplitter = new QSplitter(m_splitOrientation, this);
=======
    if (!m_paneSplitter) {
        m_isSplit = true;
        setProperty("isHostPanel", "true");
        style()->unpolish(this);
        style()->polish(this);

        m_paneSplitter = new QSplitter(m_splitOrientation, this);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    } else {
        m_isSplit = true;
        m_paneSplitter->setOrientation(m_splitOrientation);
=======
    } else {
        m_isSplit = true;
        setProperty("isHostPanel", "true");
        style()->unpolish(this);
        style()->polish(this);
        m_paneSplitter->setOrientation(m_splitOrientation);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    if (m_panes.isEmpty()) {
        m_isSplit = false;
        if (m_paneSplitter) {
=======
    if (m_panes.isEmpty()) {
        m_isSplit = false;
        setProperty("isHostPanel", "false");
        style()->unpolish(this);
        style()->polish(this);
        if (m_paneSplitter) {
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Verify `resources/style.qss` contains `#EditorContainer[isHostPanel="true"] { border: none; }`.
2. Verify `ContentPanel.cpp` correctly updates the `isHostPanel` property during `splitPane` and `closePane`.
3. Verify single-pane mode retains its 1px border while split mode eliminates the redundant outer border.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Uses existing Qt dynamic property dynamic re-polishing mechanisms (`setProperty`, `style()->unpolish`, `style()->polish`) without creating parallel styling classes or custom painting code.
- Fully preserves `#EditorContainer` single-source styling rules in `resources/style.qss`.

## Header API Signature Verification
- `QWidget::setProperty(const char *name, const QVariant &value)`
- `QWidget::style() const`
- `QStyle::unpolish(QWidget *widget)`
- `QStyle::polish(QWidget *widget)`
