# Implementation Plan - Standardize UndoToastOverlay Duration to 7 Seconds

## 1. Overview
This implementation plan standardizes the default display duration of `UndoToastOverlay` notifications across the application to 7 seconds (7000 ms). This provides users with sufficient time to read toast messages and execute undo actions.

## 2. Modified Files List
- `src/ui/UndoToastOverlay.h`

## 3. Detailed Line-by-Line Changes

```diff
<<<<<<< SEARCH
    void showToast(QWidget* parent, 
                   const QString& message, 
                   std::function<void()> undoCallback, 
                   int durationMs = 5000);
=======
    void showToast(QWidget* parent, 
                   const QString& message, 
                   std::function<void()> undoCallback, 
                   int durationMs = 7000);
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Rebuild application using CMake & Qt build pipeline.
2. Trigger any toast notification (e.g. batch rename or file action).
3. Verify that the Toast notification overlay remains visible for 7 seconds before auto-hiding.
