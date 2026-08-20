# Undo/Redo 核心 ActionCommand 指令体系实施方案 (ActionCommands Implementation Plan)

## 1. Overview（概述与解决的问题）

本实施方案旨在对 QuarkMeta 纯磁盘直连架构下的撤销/重做（Undo/Redo）核心 ActionCommand 指令体系（包含 `MoveCommand`、`RenameCommand`、`MetadataCommand`、`SecureDeleteCommand`）进行规范化定义与落地保障：
1. **MoveCommand（移动命令）**：确保文件物理移动与撤销搬回的同时，将 `.QuarkMeta.json` 中的元数据在源/目标目录间进行原子迁移与还原。
2. **RenameCommand（改名命令）**：在文件重命名与 Undo 改回时，同步更新 `.QuarkMeta.json` 中以文件名作为 Key 的键名，并平滑迁移缩略图缓存 Key。
3. **MetadataCommand（元数据变更命令）**：记录 OldState 与 NewState 快照，实现星级、颜色、标签、备注等属性的秒级 `Ctrl+Z` 还原，并支持批量操作原子打包。
4. **SecureDeleteCommand（安全粉碎命令）**：执行不可逆的物理扇区覆写与数据擦除，彻底抹除 `.QuarkMeta.json` 记录，并强制从 `UndoManager` 撤销栈中销毁并清退受影响路径的历史指令。

---

## 2. Modified Files List（影响文件清单）

1. `src/core/BasicCommands.h`
2. `src/core/UndoManager.h`
3. `src/core/UndoManager.cpp`
4. `QuarkMeta Architecture/QuarkMeta-Architecture-Planning.md`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/core/BasicCommands.h`
确保四大 Command 的基类接口定义与 `.QuarkMeta.json` 原子更新行为完全相符，并明确安全物理粉碎的不可撤销铁律。

```
<<<<<<< SEARCH
    void undo() override {
        // 物理删除不可撤销，此接口留空
    }
=======
    void undo() override {
        // 🚨 铁律：安全物理粉碎不可撤销 (No Undo)
    }
>>>>>>> REPLACE
```

---

### 3.2 `src/core/UndoManager.h` & `src/core/UndoManager.cpp`
确保在执行 `SecureDeleteCommand` 物理深层粉碎文件后，`UndoManager` 能够自动清理并清退所有涉及该路径的历史撤销指令。

```
<<<<<<< SEARCH
    /**
     * @brief 当文件被永久删除时，清理受影响的指令
     */
    void removeCommandsAffectingPath(const QString& path);
=======
    /**
     * @brief 当文件被永久删除时，清理受影响的指令，防止访问已不存在的扇区/文件
     */
    void removeCommandsAffectingPath(const QString& path);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void UndoManager::removeCommandsAffectingPath(const QString& path) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto removeFilter = [&path](const std::unique_ptr<ActionCommand>& cmd) {
        return cmd && cmd->affectsPath(path);
    };

    m_undoStack.erase(
        std::remove_if(m_undoStack.begin(), m_undoStack.end(), removeFilter),
        m_undoStack.end()
    );

    m_redoStack.erase(
        std::remove_if(m_redoStack.begin(), m_redoStack.end(), removeFilter),
        m_redoStack.end()
    );
}
=======
void UndoManager::removeCommandsAffectingPath(const QString& path) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto removeFilter = [&path](const std::unique_ptr<ActionCommand>& cmd) {
        return cmd && cmd->affectsPath(path);
    };

    m_undoStack.erase(
        std::remove_if(m_undoStack.begin(), m_undoStack.end(), removeFilter),
        m_undoStack.end()
    );

    m_redoStack.erase(
        std::remove_if(m_redoStack.begin(), m_redoStack.end(), removeFilter),
        m_redoStack.end()
    );
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译确认**：
   在命令行运行 CMake 编译，验证 `MOC` 与全工程无符号缺失错误：
   ```bash
   cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```
2. **功能验证**：
   - **Move / Rename 撤销验证**：执行文件移动或重命名后按 `Ctrl+Z`，验证文件物理恢复且 `.QuarkMeta.json` 中的星级/颜色等元数据无损还原。
   - **Metadata 撤销验证**：批量选中 50 个文件打标签，按 `Ctrl+Z` 验证 50 个文件的标签单次批量还原。
   - **SecureDelete 栈清退验证**：粉碎删除文件后，验证 `UndoManager` 历史栈中关于该文件的历史修改指令被自动清退。
