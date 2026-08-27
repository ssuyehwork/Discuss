# 永久删除 —— PermanentDeleteService

以下为为您正式起草并标准化的**《QuarkMeta 核心架构决策备案：永久删除服务 (PermanentDeleteService) 与物理抹除规范》**。

本规范确立了永久删除（硬删除）的最高安全等级、不可逆生命周期以及全系统唯一的执行管道。

---

# 📑 QuarkMeta 架构决策备案 (ADR)
## 模块名称：永久删除服务 (PermanentDeleteService) 与物理抹除规范

---

### 一、 顶层架构设计哲学与定位

1. **业务本质：不可逆物理销毁（Irreversible Destruction）**：
   - “永久删除”属于系统中的**最高安全风险操作**，目标是使物理介质上的数据彻底消失并不可恢复；
   - 严禁将永久删除与“移入回收站（`TrashService`）”混用同一个数据流或同一个类；
   - 严禁在永久删除流程中录制任何可逆的撤销快照（Undo Snapshot）。
2. **状态彻底清场（State Annihilation）**：
   - 永久删除不仅要销毁物理磁盘扇区，还必须**同步、彻底地清理与该文件相关的一切衍生状态**（包括撤销历史栈、SQLite 数据库、本地 `.QuarkMeta.json`、缩略图缓存及宽高比缓存）。

---

### 二、 核心架构铁律（红线禁令）

1. **【单一执行入口铁律（Single Execution Entrypoint）】**：
   - **全系统所有触发“永久删除/彻底粉碎”的入口（键盘 `Shift + Del` 快捷键、常规右键菜单“永久删除”、回收站右键“彻底删除”、重复文件清理对话框等），最终必须且只能调用 `PermanentDeleteService::instance().execute(...)`。**
   - 严禁在 `eventFilter` 或任何 UI 控件中私自调用 `QFile::remove` 或 `QDir::removeRecursively` 进行同步硬删。
2. **【强制安全确认门（Safety Confirmation Gate）】**：
   - 永久删除必须统一弹出警示级别的模态二次确认框（明确提示：“*此操作不可恢复，数据将被物理覆写抹除*”）；若用户取消，系统必须在毫秒级内安全中断，严禁静默直接执行。
3. **【撤销栈强力清洗铁律（Undo History Purging）】**：
   - 执行物理抹除前/后，必须强制调用 `UndoManager::instance().removeCommandsForPath(path)`，彻底从内存撤销历史中抹除该路径，**严禁物理已死的文件在 Undo 栈中残留为“幽灵记录”**。
4. **【UI 主线程零阻塞与多线程隔离】**：
   - 物理覆写（Shredding）与大目录递归删除必须全权交由后台工作线程（Worker Thread）处理；必须使用 `QPointer` 哨兵对 UI 进度条与面板进行防悬空保护，严禁阻塞 UI 主线程事件循环。

---

### 三、 `PermanentDeleteService` 标准领域服务契约（接口定义）

```cpp
#pragma once

#include <QObject>
#include <QStringList>
#include <QWidget>
#include <vector>
#include <utility>

namespace QuarkMeta {

/**
 * @brief 永久删除与物理粉碎领域服务 (Domain Service)
 * 职责：纯业务领域调度，负责高危确认、进度条托管、物理安全覆写、撤销栈清洗与元数据全量销毁
 */
class PermanentDeleteService : public QObject {
    Q_OBJECT

public:
    static PermanentDeleteService& instance();

    /**
     * @brief 统一执行常规物理文件/目录的永久删除
     * @param paths 待物理销毁的目标路径列表
     * @param parentWidget 宿主窗口（用于定位模态确认框与进度条）
     * @param isSecureShred 是否启用多重覆写物理粉碎（默认 true）
     * @return 是否成功通过确认并派发至后台执行
     */
    bool execute(const QStringList& paths, QWidget* parentWidget = nullptr, bool isSecureShred = true);

    /**
     * @brief 统一执行回收站中未还原条目的彻底销毁
     * @param trashItems 包含 (trashId, trashPath) 的回收站条目列表
     * @param parentWidget 宿主窗口
     */
    bool executeTrashItems(const std::vector<std::pair<int, QString>>& trashItems, QWidget* parentWidget = nullptr);

signals:
    /**
     * @brief 永久删除批量任务执行完毕广播
     */
    void permanentDeleteFinished(int successCount, int totalCount);

private:
    explicit PermanentDeleteService(QObject* parent = nullptr) : QObject(parent) {}
    ~PermanentDeleteService() override = default;
    PermanentDeleteService(const PermanentDeleteService&) = delete;
    PermanentDeleteService& operator=(const PermanentDeleteService&) = delete;
};

} // namespace QuarkMeta
```

---

### 四、 标准七步不可逆原子流水线（`execute` 内部实现规范）

当调用 `PermanentDeleteService::instance().execute` 时，服务内部必须严格按照以下顺序执行：

```
[ Step 1: 高危模态确认门 ] ──► 统一弹出 FramelessMessageBox 警告框，用户取消则立即 return
            │
[ Step 2: 进度条安全托管 ] ──► 拉起 BatchProgressDialog，使用 QPointer 挂载哨兵防护
            │
[ Step 3: 并发内部加锁   ] ──► MetadataManager::instance().beginInternalOperation()
            │
[ Step 4: 撤销历史清洗   ] ──► UndoManager::instance().removeCommandsForPath(p) 拔除幽灵命令
            │
[ Step 5: 后台物理粉碎   ] ──► 调度线程池调用 SecureFileEraser::shredFile / 递归物理硬删
            │
[ Step 6: 数据库/缓存抹除] ──► 同步销毁 SQLite 元数据、清除磁盘 .QuarkMeta.json 与缩略图缓存
            │
[ Step 7: 全局事件广播   ] ──► 发布 AppEventType::ItemsDeleted，UI 触发 $O(1)$ 局部更新
```

---

### 五、 全工程 UI 层的统一调用规范（落地范例）

全系统所有 UI 类的调用代码**强行收敛为纯净的一行代码**，彻底消灭就地编写的百行多线程硬删逻辑：

```cpp
// 1. 【键盘 Shift + Delete 快捷键】 (ContentPanel.cpp)
if (keyEvent->key() == Qt::Key_Delete && (keyEvent->modifiers() & Qt::ShiftModifier)) {
    PermanentDeleteService::instance().execute(getSelectedPaths(), this);
    return true;
}

// 2. 【常规右键菜单“永久删除”】 (ContentPanel.cpp)
case ActionSecureDelete: {
    PermanentDeleteService::instance().execute(getSelectedPaths(), this);
    break;
}

// 3. 【回收站右键“彻底删除”（销毁已移入的项目）】 (ContentPanel.cpp)
case ActionTrashSecureDelete: {
    PermanentDeleteService::instance().executeTrashItems(getSelectedDiskTrashItems(), this);
    break;
}

// 4. 【重复文件冲突对话框一键粉碎】 (DuplicateConflictDialog.cpp)
void DuplicateConflictDialog::onShredButtonClicked() {
    PermanentDeleteService::instance().execute(getConflictDuplicatePaths(), this);
}
```

---

### 📌 备案总结：
至此，系统的**删除业务体系完成绝对物理双轨隔离**：
- **`TrashService`**：负责**可逆**的资产暂存、快照录制与原路还原；
- **`PermanentDeleteService`**：负责**不可逆**的高危拦截、物理粉碎与状态清场。

两者各自作为独立的领域能力存在，**UI 控件永久只保留 1 行标准调用**。