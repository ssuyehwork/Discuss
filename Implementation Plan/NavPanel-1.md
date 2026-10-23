# NavPanel Trash Icon Remapping Implementation Plan

## 1. Overview
This implementation plan specifies the icon remapping for the Trash item in `NavPanel.cpp`, updating the icon from `trash` to `delete_forever.svg` (`"delete_forever"`).

## 2. Modified Files List
- `src/ui/NavPanel.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 Update Trash Item Icon in `NavPanel.cpp`

```
<<<<<<< SEARCH
    // 5. 回收站 (固定主节点)
    QIcon trashIcon = UiHelper::getIcon("trash", QColor("#e81123"), 18);
    QStandardItem* trashItem = new QStandardItem(trashIcon, "回收站");
=======
    // 5. 回收站 (固定主节点)
    QIcon trashIcon = UiHelper::getIcon("delete_forever", QColor("#e81123"), 18);
    QStandardItem* trashItem = new QStandardItem(trashIcon, "回收站");
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. **Compilation**: Compile using CMake / MSVC build environment.
2. **Visual Verification**:
   - Inspect the bottom of the NavPanel sidebar tree view.
   - Verify that the "回收站" root item displays the `delete_forever` SVG icon in `ErrorRed` (#e81123).
