# QuarkMeta 多步连续 Undo 事务快照实施方案

## 1. 目标与范围
- 补齐 Redo（重做）双向闭环：重构 `GeneralSnapshotUndoCommand`，同时持有正向操作（`doAction`）与逆向回滚（`undoAction`），使所有快照操作（批量改名、移入回收站、收藏夹切换等）**100% 具备完美的 `Ctrl + Y` 重做能力**。
- 规范 Toast 交互与 7 秒停留时长：将 `OperationSnapshotEngine` 中的 Toast 持续时间统一规范为 **7000ms（7 秒）**，并将点击回调显式绑定至 `UndoManager::instance().undo()`。
- 保持单一严格时序栈：确保全系统所有文件与元数据操作在同一个 `UndoManager` 栈中按时间线性排列，消灭撤销时序错乱。

---

## 2. 核心模块独立实现

### 2.1 `src/core/OperationSnapshotEngine.h`
```cpp
#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <functional>
#include <memory>
#include <QWidget>

namespace QuarkMeta {

enum class SnapshotOperationType {
    Rename,            // 重命名
    BatchRename,       // 批量重命名
    DragCategorize,    // 拖拽分类
    DeleteToTrash,     // 移入回收站
    ToggleFavorite     // 添加/取消收藏
};

struct AssetItemSnapshot {
    QString path;
    QString fileName;
    bool isPinned = false;
    int rating = 0;
    QString color;
    QStringList tags;
    QString note;
};

class OperationSnapshotEngine {
public:
    static OperationSnapshotEngine& instance();

    AssetItemSnapshot captureSingle(const QString& path);
    QVector<AssetItemSnapshot> captureBatch(const QStringList& paths);

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

private:
    OperationSnapshotEngine() = default;
    ~OperationSnapshotEngine() = default;
    OperationSnapshotEngine(const OperationSnapshotEngine&) = delete;
    OperationSnapshotEngine& operator=(const OperationSnapshotEngine&) = delete;
};

} // namespace QuarkMeta
```

### 2.2 `src/core/OperationSnapshotEngine.cpp`
```cpp
#include "OperationSnapshotEngine.h"
#include "../meta/MetadataManager.h"
#include "../ui/UndoToastOverlay.h"
#include "UndoManager.h"
#include "ActionCommand.h"
#include <QFileInfo>
#include <utility>

namespace QuarkMeta {

OperationSnapshotEngine& OperationSnapshotEngine::instance() {
    static OperationSnapshotEngine inst;
    return inst;
}

AssetItemSnapshot OperationSnapshotEngine::captureSingle(const QString& path) {
    AssetItemSnapshot snap;
    snap.path = path;
    snap.fileName = QFileInfo(path).fileName();

    std::wstring wpath = path.toStdWString();
    auto meta = MetadataManager::instance().getMeta(wpath);
    snap.isPinned = meta.pinned;
    snap.rating = meta.rating;
    snap.color = QString::fromStdWString(meta.manualColor);
    snap.tags = meta.tags;
    snap.note = QString::fromStdWString(meta.note);
    return snap;
}

QVector<AssetItemSnapshot> OperationSnapshotEngine::captureBatch(const QStringList& paths) {
    QVector<AssetItemSnapshot> list;
    list.reserve(paths.size());
    for (const auto& p : paths) {
        list.append(captureSingle(p));
    }
    return list;
}

bool OperationSnapshotEngine::executeWithSnapshot(
    QWidget* parentWidget,
    SnapshotOperationType opType,
    const QStringList& targetPaths,
    const QString& successToastMsg,
    std::function<bool()> doAction,
    std::function<bool(const QVector<AssetItemSnapshot>& beforeState)> undoAction)  
{
    Q_UNUSED(opType);
    if (!doAction) return false;

    // 1. 操作前：自动捕获受影响资产的状态快照 (Before State)
    QVector<AssetItemSnapshot> beforeState = captureBatch(targetPaths);

    // 2. 执行主体写操作
    bool ok = doAction();
    if (!ok) return false;

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

    return true;
}

} // namespace QuarkMeta
```

---

## 3. `UndoManager.h` 维护与加固

确保容量限幅（50 步）与永久删除路径清洗机制（`removeCommandsForPath`）完整生效：

```cpp
#pragma once

#include "ActionCommand.h"
#include <deque>
#include <memory>
#include <QObject>
#include <QMutex>
#include <QMutexLocker>

namespace QuarkMeta {

class UndoManager : public QObject {
    Q_OBJECT

public:
    static UndoManager& instance() {
        static UndoManager inst;
        return inst;
    }

    void pushCommand(std::unique_ptr<ActionCommand> command) {
        QMutexLocker lock(&m_mutex);
        m_undoStack.push_back(std::move(command));
        if (m_undoStack.size() > 50) {
            m_undoStack.pop_front();
        }
        m_redoStack.clear(); // 执行新操作时清空重做栈
        emit canUndoChanged(!m_undoStack.empty());
        emit canRedoChanged(false);
    }

    void undo() {
        QMutexLocker lock(&m_mutex);
        if (m_undoStack.empty()) return;

        auto command = std::move(m_undoStack.back());
        m_undoStack.pop_back();
        
        command->undo();
        m_redoStack.push_back(std::move(command));
        
        emit canUndoChanged(!m_undoStack.empty());
        emit canRedoChanged(true);
    }

    void redo() {
        QMutexLocker lock(&m_mutex);
        if (m_redoStack.empty()) return;

        auto command = std::move(m_redoStack.back());
        m_redoStack.pop_back();
        
        command->redo();
        m_undoStack.push_back(std::move(command));
        
        emit canUndoChanged(true);
        emit canRedoChanged(!m_redoStack.empty());
    }

    bool canUndo() const { return !m_undoStack.empty(); }
    bool canRedo() const { return !m_redoStack.empty(); }

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

signals:
    void canUndoChanged(bool canUndo);
    void canRedoChanged(bool canRedo);

private:
    UndoManager() = default;
    ~UndoManager() = default;

    std::deque<std::unique_ptr<ActionCommand>> m_undoStack;
    std::deque<std::unique_ptr<ActionCommand>> m_redoStack;
    QMutex m_mutex;
};

} // namespace QuarkMeta
```

---

## 4. `CMakeLists.txt` 构建配置维护
确保相关源文件在构建系统中已正规注册：
```cmake
set(CORE_SOURCES
    # ...
    src/core/ActionCommand.h
    src/core/BasicCommands.h
    src/core/UndoManager.h
    src/core/OperationSnapshotEngine.h
    src/core/OperationSnapshotEngine.cpp
)
```