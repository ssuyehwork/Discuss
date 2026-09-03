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