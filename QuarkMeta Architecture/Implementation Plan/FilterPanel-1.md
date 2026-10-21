# Implementation Plan - FilterPanel MSVC C2027 Incomplete Type Fix

## Pre-Draft Evidence (Gate 1)

### 1. Read Files List
- `src/ui/FilterPanel.h` (Read using `read_file`)
- `src/ui/FilterPanel.cpp` (Read using `run_in_bash_session`)
- `AGENTS.md` Section 3.5 & Section 3.7 (Read using `read_file`)

### 2. Header Method Signatures Excerpts
From `src/ui/FilterPanel.h`:
```cpp
class FilterPanel : public QFrame {
    Q_OBJECT
public:
    explicit FilterPanel(QWidget* parent = nullptr);
    ~FilterPanel() override = default;
    QSize minimumSizeHint() const override { return QSize(230, 100); }
    void populateStats(const QuarkMeta::ScanStats& stats);
    void clearStats();
    FilterState currentFilter() const;
    void syncUIFromFilterState();
};
```

From `<QScrollArea>` (Qt6 Framework):
```cpp
class QScrollArea : public QAbstractScrollArea {
    // verticalScrollBar() returns QScrollBar* pointer, but QScrollBar is forward declared only in <QScrollArea>
    QScrollBar *verticalScrollBar() const;
};
```

### 3. SSOT Search Log
- Searched `verticalScrollBar` in `src/ui/FilterPanel.cpp`:
  - Line 452: `int savedScrollY = (m_scrollArea && m_scrollArea->verticalScrollBar()) ? m_scrollArea->verticalScrollBar()->value() : 0;`
  - Line 650: `m_scrollArea->verticalScrollBar()->setValue(savedScrollY);`
- Result: Invoking `value()` and `setValue()` on `QScrollBar*` requires full `<QScrollBar>` header definition.

### 4. Memories.md Key Rules Alignment
- **AGENTS.md Section 3.5-3**: Header Chain Closure rule mandates that when a pointer method from Qt headers is invoked, the defining header (`<QScrollBar>`) must be explicitly included in the header/source to prevent C2027 incomplete type compiler errors.

---

## 1. Overview
### 1.1 Problem Statement & Root Cause Attribution
During MSVC compilation of `src/ui/FilterPanel.cpp`, MSVC emitted the following C2027 compiler errors:
- `FilterPanel.cpp(452): error C2027: use of undefined type 'QScrollBar'`
- `FilterPanel.cpp(650): error C2027: use of undefined type 'QScrollBar'`

**Root Cause Attribution**: `FilterPanel.h` includes `<QScrollArea>`. In Qt's `<QScrollArea>` header, `class QScrollBar;` is forward-declared. Calling `m_scrollArea->verticalScrollBar()->value()` and `setValue(savedScrollY)` in `FilterPanel.cpp` requires complete definition of methods in `QScrollBar`. Missing `#include <QScrollBar>` in `FilterPanel.h` causes MSVC C2027 incomplete type compilation failure.

### 1.2 Solution
Include `#include <QScrollBar>` directly in `src/ui/FilterPanel.h` right after `#include <QScrollArea>`.

---

## 2. Modified Files List
- `src/ui/FilterPanel.h`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/FilterPanel.h`

```cpp
<<<<<<< SEARCH
#include <QCheckBox>
#include <QScrollArea>
#include <QPushButton>
=======
#include <QCheckBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QPushButton>
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Recompile `src/ui/FilterPanel.cpp` under MSVC.
2. Confirm that C2027 compiler errors on lines 452 and 650 are completely resolved.
3. Launch QuarkMeta and verify FilterPanel scroll position saving/restoring during group toggles.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Header Chain Closure**: Adding `#include <QScrollBar>` in `FilterPanel.h` satisfies Header Chain Closure without modifying existing business logic.

---

## 6. Header API Signature Verification

| Calling Class | Method Called | Header Declaration Location | Verified Signature |
|---|---|---|---|
| `FilterPanel` | `m_scrollArea->verticalScrollBar()` | `<QScrollArea>` | `QScrollBar* verticalScrollBar() const;` |
| `FilterPanel` | `verticalScrollBar()->value()` | `<QScrollBar>` | `int value() const;` |
| `FilterPanel` | `verticalScrollBar()->setValue(int)` | `<QScrollBar>` | `void setValue(int);` |

---

## 7. Header Inclusion Chain & Type Completeness Check
- `src/ui/FilterPanel.h` adds `#include <QScrollBar>`, providing complete definition for `QScrollBar` to all source files including `FilterPanel.h`.

---

## 8. Compliance Self-Check Table

| Item | Source Clause | Status | Location in Plan |
|---|---|---|---|
| Filename mapping 1:1 with core class/module | 3.1-2 | Compliant | `FilterPanel-1.md` |
| Isolated in physical plan directory | 3.1-1 | Compliant | `QuarkMeta Architecture/Implementation Plan/` |
| All SEARCH blocks exist in codebase | 3.2 | Compliant | Section 3 |
| CMake alignment for Q_OBJECT classes | 3.3 | Not Applicable | No new classes added |
| Chapters 1~9 complete | 3.4 | Compliant | Sections 1~9 |
| External member calls matched with headers | 3.5-1/2 | Compliant | Section 6 |
| Header inclusion chain verified | 3.5-3 | Compliant | Section 7 |
| Reused SSOT entry points | 2.4 | Compliant | Section 5 |
| No inline setStyleSheet | 4.1 | Compliant | Section 1 |
| No platform hacks | 4.2 | Compliant | Section 1 |
| Aggregated broadcast for batch operations | 4.3 | Not Applicable | Header inclusion fix |
| Zero value alteration | 4.4 | Compliant | Section 9 |
| Preserved user interaction design | 1.2 / Memories.md | Compliant | Section 1 |
| No files modified outside scope | 5.3 | Compliant | Section 2 |

---

## 9. Zero-Value-Alteration Check
- **Not Applicable**: Header inclusion fix only. No UI numerical parameters modified.
