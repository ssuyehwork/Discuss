# Implementation Plan - ContentPanel (Fix Missing Border on Primary Pane Split View Container)

## 1. Overview
When double-pane split mode is enabled (`splitPane()`), the content panel splits into a primary pane and a secondary pane inside a `QSplitter`.
Currently:
- The secondary pane container contains a `m_secondaryContentPanel` (`ContentPanel` instance), which inherits from `QFrame`. Qt QSS natively renders `border: 1px solid #333333` for `#EditorContainer` on `QFrame`.
- The primary pane container (`m_primaryPaneContainer`) was instantiated as a base `QWidget*`. Qt QSS engine by default does NOT render borders (`border: 1px solid #333333`) on plain `QWidget` instances unless `QStyleOption` painting is explicitly implemented.
This results in the primary (left) pane missing its left/right border and vertical divider line, while the secondary (right) pane displays its border properly.

To fix this issue:
We change `m_primaryPaneContainer` from `QWidget*` to `QFrame*` (`new QFrame(m_paneSplitter)`). Since `QFrame` naturally supports Qt QSS border rendering, both left and right split panes will render identical 1px solid `#333333` border dividers.

---

## 2. Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/ContentPanel.h`
<<<<<<< SEARCH
    QSplitter* m_paneSplitter = nullptr;
    QWidget* m_primaryPaneContainer = nullptr;
    QWidget* m_secondaryPaneContainer = nullptr;
=======
    QSplitter* m_paneSplitter = nullptr;
    QFrame* m_primaryPaneContainer = nullptr;
    QWidget* m_secondaryPaneContainer = nullptr;
>>>>>>> REPLACE

---

### File 2: `src/ui/ContentPanel.cpp`
<<<<<<< SEARCH
        // 1. Primary pane container
        m_primaryPaneContainer = new QWidget(m_paneSplitter);
        m_primaryPaneContainer->setObjectName("EditorContainer");
        m_primaryPaneContainer->setAttribute(Qt::WA_StyledBackground, true);
=======
        // 1. Primary pane container
        m_primaryPaneContainer = new QFrame(m_paneSplitter);
        m_primaryPaneContainer->setObjectName("EditorContainer");
        m_primaryPaneContainer->setAttribute(Qt::WA_StyledBackground, true);
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Execute CMake build:
   ```bash
   cmake --build build --config Release --target QuarkMeta
   ```
2. Run the application or test build output to verify clean compilation with no warnings or missing symbols.
3. Open dual-pane split view (`Ctrl+Alt+S` or split button) and verify that both primary (left) and secondary (right) content panels display clear 1px solid `#333333` borders on both sides.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **QSS Style SSOT**: Uses existing `#EditorContainer` selector in `resources/style.qss` (`border: 1px solid #333333`). No inline styling or hardcoded borders added in C++.
- **Layout Integrity**: Modifies container type from `QWidget` to `QFrame` without introducing redundant layout wrappers or duplicating splitting logic.

---

## 6. Header API Signature Verification
- `QFrame` is declared in `<QFrame>` (already included in `ContentPanel.h`).
- `QFrame::setObjectName(const QString& name)` is inherited from `QObject`.
- `QFrame::setAttribute(Qt::WidgetAttribute attribute, bool on = true)` is inherited from `QWidget`.
- `QFrame::setLayout(QLayout* layout)` is inherited from `QWidget`.
- All methods called on `m_primaryPaneContainer` exist and are standard Qt API signatures.
