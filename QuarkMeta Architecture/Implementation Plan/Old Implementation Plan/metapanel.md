# Implementation Plan - MetaPanel Decoupling (`metapanel.md`)

## 1. Overview
`MetaPanel` currently combines preview thumbnail rendering, star rating/color markers, tag flow layouts, note/link editing, and file path attributes in a single container.
This plan refactors `MetaPanel` into modular section widgets:
1. `MetaPreviewWidget`: Manages image/thumbnail preview displaying.
2. `MetaRatingColorWidget`: Manages star ratings and color tag buttons.
3. `MetaTagSection`: Handles tag pill display and `TagSelectorOverlay` interactions.
4. `MetaInfoSection`: Handles file size, timestamps, path copy, and Explorer location opening.

---

## 2. Modified Files List
- `CMakeLists.txt`
- `src/ui/MetaPreviewWidget.h`
- `src/ui/MetaPreviewWidget.cpp`
- `src/ui/MetaRatingColorWidget.h`
- `src/ui/MetaRatingColorWidget.cpp`
- `src/ui/MetaTagSection.h`
- `src/ui/MetaTagSection.cpp`
- `src/ui/MetaInfoSection.h`
- `src/ui/MetaInfoSection.cpp`
- `src/ui/MetaPanel.h`
- `src/ui/MetaPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### `CMakeLists.txt`
```git
<<<<<<< SEARCH
    src/ui/MetaPanel.cpp
    src/ui/MetaPanel.h
=======
    src/ui/MetaPreviewWidget.h
    src/ui/MetaPreviewWidget.cpp
    src/ui/MetaRatingColorWidget.h
    src/ui/MetaRatingColorWidget.cpp
    src/ui/MetaTagSection.h
    src/ui/MetaTagSection.cpp
    src/ui/MetaInfoSection.h
    src/ui/MetaInfoSection.cpp
    src/ui/MetaPanel.cpp
    src/ui/MetaPanel.h
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### Verification Steps
1. Select files in `ContentPanel` and verify attribute rendering in `MetaPanel`.
2. Test rating/color changes, tag modifications, and link jumps across sub-sections.
