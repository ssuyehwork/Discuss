# OperationSnapshotEngine 多步连续 Undo/Redo 事务快照实施方案

## 1. Overview（概述与解决的问题）
本实施方案旨在补齐全系统快照事务的 Redo（重做）双向闭环能力，并规范操作反馈 Toast 提示交互：
1. **Redo（重做）闭环**：重构 `GeneralSnapshotUndoCommand`，同时保存正向操作（`doAction`）与逆向回滚（`undoAction`），使撤销栈支持 `Ctrl + Z`（撤销）与 `Ctrl + Y`（重做）双向对称切换。
2. ** Toast 规范**：将 `UndoToastOverlay` 持续停留时间统一固定为 **7000ms (7秒)**，并将撤销按钮显式绑定至 `UndoManager::instance().undo()`。
3. **单一时序栈与路径清洗**：所有文件与元数据操作统一经由 `UndoManager` 线程安全单例栈维护，且文件永久删除时自动清洗涉及该路径的 Command。

---

## 2. Modified Files List（影响文件清单）
- `src/core/OperationSnapshotEngine.h`
- `src/core/OperationSnapshotEngine.cpp`
- `src/core/UndoManager.h`
- `CMakeLists.txt`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/core/OperationSnapshotEngine.h`
<<<<<<< SEARCH
    // 执行带快照捕获与 UndoToastOverlay 弹窗提醒的操作
    // 对应用户原话：“快照结合UndoToastOverlay”
    bool executeWithSnapshot(
        QWidget* parentWidget,
        SnapshotOperationType opType,
        const QStringList& targetPaths,
        const QString& successToastMsg,
        std::function<bool()> doAction,
        std::function<bool(const QVector<AssetItemSnapshot>& beforeState)> undoAction
    );
=======
    /**
     * @brief 执行带快照捕获、标准 7 秒 Toast 提醒与完美 Redo 支持的事务操作
     */
    bool executeWithSnapshot(
        QWidget* parentWidget,
        SnapshotOperationType opType,
        const QStringList& targetPaths,
        const QString& successToastMsg,
        std::function<bool()> doAction,
        std::function<bool(const QVector<AssetItemSnapshot>& beforeState)> undoAction
    );
>>>>>>> REPLACE

### 3.2 `src/core/OperationSnapshotEngine.cpp`
<<<<<<< SEARCH
    // 3. 操作成功：如果外部传入了专用的 undoAction，
    // 在主线程中生成一个通用快照回滚 ActionCommand 并推送给 UndoManager，实现 100% 物理与虚拟并轨！
    if (undoAction) {
        class GeneralSnapshotUndoCommand : public ActionCommand {
        public:
            GeneralSnapshotUndoCommand(QVector<AssetItemSnapshot> before,
                                       std::function<bool(const QVector<AssetItemSnapshot>& beforeState)> undo)
                : m_before(before), m_undoFunc(undo) {}

            void execute() override {}
            void undo() override {
                if (m_undoFunc) {
                    m_undoFunc(m_before);
                }
            }
            void redo() override {}
            QString description() const override { return "快照撤销"; }
            bool affectsPath(const QString& path) const override {
                for (const auto& snap : m_before) {
                    if (snap.path == path) return true;
                }
                return false;
            }
        private:
            QVector<AssetItemSnapshot> m_before;
            std::function<bool(const QVector<AssetItemSnapshot>& beforeState)> m_undoFunc;
        };

        // 压入全局撤销栈，这样无论是按 Ctrl+Z 还是点击气泡，均能完美统一调用同一个 Command 恢复物理与逻辑状态
        UndoManager::instance().pushCommand(std::make_unique<GeneralSnapshotUndoCommand>(beforeState, undoAction));

        // 弹出反馈气泡，点击撤销会直接调用 UndoManager::instance().undo()
        UndoToastOverlay::instance()->showToast(
            parentWidget,
            successToastMsg,
            []() {
                // 回调闭包留空或传入 dummy 即可，因为在 4.1 节中 UndoToastOverlay 已经并轨至 UndoManager
            },
            5000
        );
    }
=======
    // 3. 操作成功：如果外部传入了专用的 undoAction，构建双向对称 Command
    if (undoAction) {
        class GeneralSnapshotUndoCommand : public ActionCommand {
        public:
            GeneralSnapshotUndoCommand(QVector<AssetItemSnapshot> before,
                                       std::function<bool()> doFunc,
                                       std::function<bool(const QVector<AssetItemSnapshot>& beforeState)> undoFunc)
                : m_before(std::move(before)),
                  m_doFunc(std::move(doFunc)),
                  m_undoFunc(std::move(undoFunc)) {}

            void execute() override {
                // 🚀【Redo 核心闭环】：重新执行正向操作
                if (m_doFunc) {
                    m_doFunc();
                }
            }

            void undo() override {
                // 🚀【Undo 核心闭环】：基于快照执行逆向回滚
                if (m_undoFunc) {
                    m_undoFunc(m_before);
                }
            }

            void redo() override {
                execute();
            }

            QString description() const override {
                return "快照事务撤销/重做";
            }

            bool affectsPath(const QString& path) const override {
                for (const auto& snap : m_before) {
                    if (snap.path == path) return true;
                }
                return false;
            }

        private:
            QVector<AssetItemSnapshot> m_before;
            std::function<bool()> m_doFunc;
            std::function<bool(const QVector<AssetItemSnapshot>& beforeState)> m_undoFunc;
        };

        // 压入全局撤销栈，实现 Ctrl+Z 与 Ctrl+Y 的 100% 严格时序双向可逆
        UndoManager::instance().pushCommand(
            std::make_unique<GeneralSnapshotUndoCommand>(beforeState, doAction, undoAction)
        );

        // 🚀【规范 7000ms 与点击显式绑定】：弹出 7 秒气泡，点击右侧“撤销”按钮直接调用 UndoManager::undo()
        UndoToastOverlay::instance()->showToast(
            parentWidget,
            successToastMsg,
            []() {
                UndoManager::instance().undo();
            },
            7000 // 👈 统一 7 秒停留时长红线
        );
    }
>>>>>>> REPLACE

### 3.3 `src/core/UndoManager.h`
<<<<<<< SEARCH
    /**
     * @brief 2026-06-xx 按照分析计划 #8：当文件被永久删除时，清理受影响的指令
     */
    void removeCommandsForPath(const QString& path) {
        QMutexLocker lock(&m_mutex);
        auto cleaner = [&](std::deque<std::unique_ptr<ActionCommand>>& stack) {
            for (auto it = stack.begin(); it != stack.end(); ) {
                // 这里需要一种方式判定 Command 是否涉及该路径
                // 暂时通过 description 或其他元数据判定（在 BasicCommands 中补全）
                // 简化处理：由于 std::unique_ptr 不能简单判定内容，我们在 ActionCommand 中增加接口
                if ((*it)->affectsPath(path)) {
                    it = stack.erase(it);
                } else {
                    ++it;
                }
            }
        };
        cleaner(m_undoStack);
        cleaner(m_redoStack);
        emit canUndoChanged(!m_undoStack.empty());
        emit canRedoChanged(!m_redoStack.empty());
    }
=======
    void removeCommandsForPath(const QString& path) {
        QMutexLocker lock(&m_mutex);
        auto cleaner = [&](std::deque<std::unique_ptr<ActionCommand>>& stack) {
            for (auto it = stack.begin(); it != stack.end(); ) {
                if ((*it)->affectsPath(path)) {
                    it = stack.erase(it);
                } else {
                    ++it;
                }
            }
        };
        cleaner(m_undoStack);
        cleaner(m_redoStack);
        emit canUndoChanged(!m_undoStack.empty());
        emit canRedoChanged(!m_redoStack.empty());
    }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）
1. 检查构建文件配置：确保 `CMakeLists.txt` 中包含 `src/core/OperationSnapshotEngine.h` 与 `src/core/OperationSnapshotEngine.cpp`。
2. 静态分析校验：确保 `GeneralSnapshotUndoCommand` 的 `execute()` 与 `redo()` 均正确转发调用 `m_doFunc()`。
3. 动态验证（在支持编译的环境下）：
   - 执行一次批量重命名或移入回收站操作。
   - 按 `Ctrl + Z` 或点击 Toast 上的“撤销”，确认资产状态完全回滚。
   - 按 `Ctrl + Y`（重做），确认资产再次应用正向修改，达成 100% 双向闭环。
