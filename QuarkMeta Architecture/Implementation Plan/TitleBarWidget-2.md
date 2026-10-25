# TitleBar Name Label Removal Implementation Plan

## 1. Overview
This implementation plan removes the app name text label (`m_appNameLabel`, "QuarkMeta") from the `TitleBarWidget`, keeping only the brand logo label (`m_logoLabel`) at the top-left of the title bar.

## 2. Modified Files List
- `src/ui/TitleBarWidget.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 Update `TitleBarWidget.cpp` to remove `m_appNameLabel` initialization and layout addition

```
<<<<<<< SEARCH
    m_appNameLabel = new QLabel("QuarkMeta", this);
    m_appNameLabel->setObjectName("AppNameLabel");
    m_layout->addWidget(m_appNameLabel);
    m_layout->addStretch();
=======
    m_layout->addStretch();
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. **Compilation**:
   ```bash
   cmake --build build --config Debug
   ```
2. **Visual Verification**:
   - Run the application.
   - Inspect the title bar at the top left corner.
   - Confirm that the FERREX brand logo is visible and the "QuarkMeta" text label is gone.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- No new APIs or duplicate widgets were introduced.
- Existing `TitleBarWidget` structure is preserved cleanly.

## 6. Header API Signature Verification
- `TitleBarWidget.h` is unmodified (contract frozen).
