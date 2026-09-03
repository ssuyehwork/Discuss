# AGENTS.md

> **CRITICAL DIRECTIVE FOR ALL AI AGENTS (Jules, Cursor, Copilot, etc.)**  
> **READ THIS BEFORE PROPOSING OR MODIFYING ANY CODE IN THIS REPOSITORY.**  
> You are operating within an established, production-grade C++/Qt desktop codebase.  
> **Blindly adding patches, duplicating existing utilities, inventing custom infrastructure, or violating layered architecture WILL RESULT IN IMMEDIATE REJECTION.**

---

## 1. Zero-Duplication Policy (Anti-Wheel-Reinventing)

Before creating ANY utility function, helper class, or custom low-level logic, you **MUST** verify if it already exists in the table below. **Always reuse existing modules.**

### 🚫 Banned Inventions & Required Existing Modules

| Category | 🚫 FORBIDDEN TO CREATE / DUPLICATE | ✅ MANDATORY REUSE MODULE |
| :--- | :--- | :--- |
| **Path & Navigation** | Manual string manipulation for paths (`lastIndexOf('/')`, string concatenation). Custom history stacks. | `core/NavigationService.h`<br>`core/NavigationHistoryService.h`<br>`std::filesystem` |
| **Frameless & Windows OS** | Hand-crafted `nativeEvent` (`WM_NCCALCSIZE`, `WM_GETMINMAXINFO`, custom monitor calculation). | `ui/FramelessWindowHelper.h`<br>`ui/FramelessDialog.h` |
| **Icons & Vector Graphics** | Loading raw raster PNGs directly or hand-written color replacements for icons. | `ui/SvgIconRenderer.h`<br>`ui/ShellIconManager.h`<br>`ui/UiHelper.h` |
| **Configuration & Storage** | Instantiating independent `QSettings`, hand-crafted JSON configs, manual registry read/write. | `core/AppConfig.h` |
| **OS / Shell Integration** | Calling raw Win32 COM, `IShellItem`, `SHGetFileInfo`, `ShellExecute` directly inside UI panels. | `util/ShellHelper.h` |
| **Hashing & Duplication** | Custom MD5/SHA256 loops, hand-rolled file fingerprinters. | `meta/DuplicateDetectorService.h`<br>`QCryptographicHash` |
| **Media Extraction** | Parsing metadata headers directly in business services. | `util/DiskMediaExtractor.h` |
| **Asynchronous Tasks** | Spawning raw `std::thread`, unmanaged `QThread`, custom worker dispatch loops. | `QtConcurrent`<br>`QThreadPool`<br>`core/UndoManager.h` |
| **UI Tooltips & Badges** | Creating custom hovering popup loops or ad-hoc Tooltip widgets. | `ui/ToolTipOverlay.h`<br>`ui/HoverEventFilter.h` |

---

## 2. Strict Architectural Boundaries

The codebase follows a strict multi-layer separation. You must strictly adhere to these layer boundaries:

```text
┌─────────────────────────────────────────────────────────────┐
│ UI Layer: ui/* (Panels, Widgets, Dialogs, Overlays)         │
│  - CAN ONLY: Handle rendering, user inputs, signals/slots   │
│  - STRICTLY FORBIDDEN: Raw Win32 API, direct disk I/O, COM  │
└──────────────────────────────┬──────────────────────────────┘
                               │ invokes
┌──────────────────────────────▼──────────────────────────────┐
│ Core & Orchestration Layer: core/*, meta/*                  │
│  - CAN ONLY: Manage app state, business flow, DB, services  │
│  - STRICTLY FORBIDDEN: #include <QWidget>, GUI dependencies │
└──────────────────────────────┬──────────────────────────────┘
                               │ invokes
┌──────────────────────────────▼──────────────────────────────┐
│ Platform & Utility Layer: util/*                            │
│  - Single source of truth for OS-level and hardware logic   │
└─────────────────────────────────────────────────────────────┘
```

### Absolute Rules:
1. **Never pollute UI with Native APIs**: If you need Windows-specific logic, it **MUST** reside in `util/ShellHelper` or `ui/FramelessWindowHelper`, never directly inside a `*Panel.cpp` or `*Dialog.cpp`.
2. **Never cross-wire UI panels directly**: Panels (`NavPanel`, `ContentPanel`, `MetaPanel`) must communicate strictly via `core/CentralEventHub.h`, `PanelMediator.h`, or Qt Signals/Slots. Do not pass raw pointers of one panel into another.
3. **No Hidden Local State**: Do not invent local static caches or private state variables to bypass the centralized state managers (`CoreController`, `NavigationService`).

---

## 3. Mandatory Workflow for Agents (The 3-Step Protocol)

When given a task or bug report, follow these three steps sequentially. **Do not skip Step 1.**

### Step 1: Capability Inspection (Read Before Write)
* Search the repository for relevant keywords using `grep` or file search before writing any code.
* Identify which services or helpers are already handling related tasks.
* Ask yourself: *"Can this be solved by calling an existing method in `NavigationService`, `AppConfig`, or `UiHelper`?"*

### Step 2: Minimalist Surgical Modification
* Prefer small, localized changes that leverage existing extension points.
* Do not introduce new third-party dependencies without explicit instruction.
* Do not rewrite architectural layouts just to fix a visual glitch. Use existing helpers (e.g., `PanelLayoutManager`).

### Step 3: Self-Audit Checklist Before Submitting
Before finalizing your plan or code submission, you must verify:
- [ ] Did I introduce any new utility class that overlaps with `util/*` or `core/*`? (If yes, delete it and reuse).
- [ ] Did I add raw `#include <windows.h>` into a UI component? (If yes, move it to `util/ShellHelper`).
- [ ] Did I bypass `AppConfig` and write hardcoded configurations? (If yes, refactor to use `AppConfig`).
- [ ] Is this patch addressing the root cause or just slapping an ad-hoc fix on top of another patch?

---

## 4. Rejection Criteria

Your Pull Request or generated patch **will be rejected immediately** if it contains:
1. Copy-pasted helper functions across different files.
2. Direct calls to platform-specific Win32 APIs outside of `util/` or `FramelessWindowHelper`.
3. Newly created `*Manager` or `*Helper` classes that duplicate functions of Qt standard libraries or existing services.
4. "Quick-fix" patches that break existing geometry or layout preservation logic (e.g., in `MainWindow.cpp` or `PanelLayoutManager.cpp`).

// ===================|===================

---

## 5. Mandatory Patch Accounting & Circuit Breaker (补丁计数与熔断机制)

To prevent endless layered hotfixes, **EVERY** bug fix, behavioral tweak, or edge-case patch made by AI Agents MUST be explicitly logged at the very top of the modified file.

### 5.1 File Header Patch Block Standard
Whenever you modify an existing file to fix a bug or adjust behavior, you **MUST** update or create the `[AI-PATCH-LEDGER]` comment block at the top of the file:

```cpp
/**
 * [AI-PATCH-LEDGER]
 * Patch-Count: 3
 * Max-Allowed-Patches: 5
 * -------------------------------------------------------------
 * Rev | Date (UTC)   | Agent / Author | Reason / Root Cause Fixed
 * -------------------------------------------------------------
 * #1  | 2026-05-10   | Jules          | Fix window geometry restoration under multi-monitor setup.
 * #2  | 2026-05-18   | Jules          | Prevent taskbar overlap on maximized state (nativeEvent WM_NCCALCSIZE).
 * #3  | 2026-06-01   | Jules          | Fix flicker when restoring from system tray.
 * -------------------------------------------------------------
 */
 
// ===================|===================
 
## 6. The Normalization Law (归一化铁律：排查不断层)

Before touching any code related to Paths, Window States, Selection, or Navigation, you MUST adhere to `SYSTEM_CONTRACTS.md`.

1. **Check SSOT (Single Source of Truth)**:
   - Identify the UNIQUE authority for the state you are modifying.
   - If a bug occurs because State A (e.g. in AddressBar) mismatches State B (e.g. in ContentPanel), **DO NOT sync them with a local hack**. Fix the binding to the central authority (`NavigationService`).

2. **No Data Adulteration (数据形态一致性)**:
   - Every file path passed through the system MUST be pre-normalized. If you receive an unnormalized path, trace it back to the system boundary (input point) and fix it at the root with `PathHelper::normalize()`. Never sanitize strings deep inside business logic.

3. **No Timing Patches (严禁时序补丁)**:
   - PRs containing arbitrary `QTimer::singleShot(50/100/200, ...)` to bypass race conditions or initialization order bugs will be **IMMEDIATELY REJECTED**. Refactor the signal chain or follow the lifecycle contract.
   
   // ===================|===================
   
   ---

## 7. Naming Conventions & Lexicon Contract (命名与目录归一化铁律)

Semantic ambiguity in file names causes Agent amnesia and duplicate code. You MUST strictly adhere to the project's naming lexicon and directory structure.

### 7.1 Mandatory Suffixes & Locations
- **Dialogs**: MUST reside in `src/ui/dialogs/` and end with `*Dialog.h/.cpp` (MUST inherit `FramelessDialog`).
- **Data Access**: MUST end with `*Repo` (e.g., `TrashRepo`, NOT `TrashRepository` or `TrashDao`).
- **Pure Helpers**: MUST reside in `src/util/` and end with `*Helper.h/.cpp` (MUST only contain stateless/static methods).
- **Core Algorithms**: Long-running or heavy stateless computation units end with `*Engine`.
- **Business Orchestrators**: Singleton stateful business providers end with `*Service`.

### 7.2 Domain Lexicon (No Synonyms Allowed!)
DO NOT invent synonymous names. Use ONLY the established project domain words:
- Use `Trash` (FORBIDDEN: `RecycleBin`, `Garbage`, `Discard`)
- Use `Duplicate` (FORBIDDEN: `Clone`, `SameFile`, `Identical`)
- Use `Thumbnail` (FORBIDDEN: `PreviewIcon`, `Snapshot`, `MiniImage`)
- Use `Extension` (FORBIDDEN: `Suffix`, `ExtName`, `Postfix`)

### 7.3 Rejection Warning
Any PR that introduces files with ambiguous names (e.g., `MyUtils.cpp`, `CommonManager.h`, `DataHandler.cpp`) or mismatches the directory topology will be **AUTOMATICALLY REJECTED**.

// ===================|===================

