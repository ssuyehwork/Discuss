# 全工程第二轮循环物理符号全量交叉比对记录 (Interim Record-4.md)

## 一、 第二轮深度排查维度
本轮排查针对全工程 203 个源码文件展开了**Command 模式命令类实例化覆盖率**以及**全系统 Signal / Slot 悬空发射图谱**的二次交叉深度扫描。

---

## 二、 重点排查事实清单

### 1. 悬空/未接入的 Command 命令类 (`src/core/BasicCommands.h`)
在 `BasicCommands.h` 中定义的底层 Command 模式命令类，在全工程任何 UI 控件或 `CoreEngine` 的 `.cpp` 实现中均**未被实例化或调用**：
- **`MoveCommand`**: 仅在 `BasicCommands.h` 与 `DiskIoService.h` 中有声明，全工程 `.cpp` 中 0 次实例化。
- **`MetadataCommand`**: 仅在 `BasicCommands.h` 中有声明，全工程 `.cpp` 中 0 次实例化。
- **`SecureDeleteCommand`**: 仅在 `BasicCommands.h` 中有声明，全工程 `.cpp` 中 0 次实例化。
- **`EncryptCommand`**: 仅在 `BasicCommands.h` 中有声明，全工程 `.cpp` 中 0 次实例化。

### 2. 悬空信号 (Unemitted / Unconnected Signals)
在头文件中声明了 `signal`，但在全系统 `.cpp` 实现中 **0 次调用 `emit` 发射** 且 **0 次 `connect` 连接** 的死信号：
- `src/meta/MetadataManager.h`: `signal pendingSyncChanged()` —— 0 次 emit, 0 次 connect。
- `src/meta/MetadataManager.h`: `signal triggerUiSignalTimer()` —— 0 次 emit, 0 次 connect。
- `src/ui/DropTreeView.h`: `signal notesDropped(...)` —— 0 次 emit, 0 次 connect。
- `src/ui/FilterPanel.h`: `signal clearAllFilters()` —— 0 次 emit, 0 次 connect。
- `src/ui/MetaPanel.h`: `signal tagsChanged(...)` —— 0 次 emit, 0 次 connect。

---

## 三、 审计结论
第二轮循环深排证实：`src/core/BasicCommands.h` 中的大部分 Command 命令类（`MoveCommand`、`MetadataCommand`、`SecureDeleteCommand`、`EncryptCommand`）属于重构未完成留下的悬空类定义；`MetadataManager` 与 `FilterPanel` 等类中存在多条从不 `emit` 也从不 `connect` 的死信号。
