# 全局职责单一模块化架构重构规划 —— Modification_Plan-56.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景

针对全款应用普遍存在的职责过载（如 UI 上帝类 `ContentPanel` 直接控制磁盘扫描、右键动作事件、以及 Delegate 重命名时通过 parent 强转寻找主面板等严重耦合债务），本方案提出从全局视角出发进行架构升格。

基于与用户达成的共识，方案完全摈弃了临时拼凑、缝缝补补的补丁思路，引入 **“方案 A - Qt 模型数据流向上微循环（Model Data Signal Flow）”** 以及 **“命令总线（Command Bus Pattern）”**，对整款应用的逻辑层次进行工业级的重构与单一职责模块化规划。

由于当前处于只读分析师模式，本方案仅产出完整的全局设计重构图纸，不做任何代码文件的物理改动。

---

## 2. 问题定位

全款应用中急需解决的几大严重危害系统稳定性与可维护性的架构设计缺陷如下：

### 2.1 Delegate 向上多级寻找 Parent 强转崩溃点
- **代码文件**：`src/ui/ThumbnailDelegate.cpp`（或 `GridItemDelegate`）
- **根因解析**：重命名完成写入数据模型时，Delegate 居然通过 `parentWidget()` 不断向上递归强转获取 `ContentPanel`，以此直接操作 `setPendingSelectName`。这种设计破坏了“高内聚低耦合”标准，一旦 UI 树被局部包装，极易导致强转失败而触发 NullPointer 瞬间闪退（Crash）。
- **重构方向**：采用 **Qt 经典模型信号流微循环（方案 A）**。Delegate 仅通过 `setData(index, name, Qt::EditRole)` 修改模型，模型层完成物理重命名并由其自身对外发射信号，UI 容器通过信号驱动自身刷新定位。

### 2.2 右键菜单点击事件与业务直接硬编码耦合
- **代码文件**：`src/ui/ContentPanel.cpp`
- **根因解析**：`ContentPanel` 本应仅仅是内容的排版渲染容器，却在其 `onCustomContextMenuRequested` 方法内部直接写死了物理删除、QFile 物理复制、物理文件移动等一系列高风险 I/O 和数据库同步业务。这直接导致主线程堵塞，且无法做单独的无 GUI 算法覆盖测试。
- **重构方向**：引入 **命令总线（Command Bus）** 和 **命令模式（Command Pattern）**，将所有点击行为转换成自包含 `undo()` / `redo()` 的 Command 类（如 `RenameCommand`, `SecureDeleteCommand`, `EncryptCommand`），投递给中央调度总线执行，实现完美的单向数据流闭环。

---

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 规划整体应用的逻辑架构时要从全局去审视、架构，不该采用临时拼凑、缝缝补补的方式 | 本方案通过彻底颠覆旧有的纵向强引用设计，引入四层解耦（UI-Controller-Service-Data）和命令总线（Command Bus）来进行全局模块化规划。 | ✅ |
| 2    | 那就按照方案A来实现吧 | 本方案在 4.1 节详细规划了针对 Delegate 解耦的“方案 A”——模型数据流向上微循环的实现路径。 | ✅ |

---

## 4. 详细解决方案

本次全局重构方案不采用任何细节拼凑，从整体软件工程的分层架构出发，对四大核心层级进行模块重新编排设计：

```
表现层 (UI View: MainWindow / ContentPanel)
      │
      │ 🚀 (仅发射语义化信号，如 requestExecuteCommand)
      ▼
业务控制层 (AppController / Command Bus)
      │
      │ ⚙️ (分发并调度，投递后台执行)
      ▼
核心服务层 (Domain Service: Ingestion / Encryption / ShellHelper)
      │
      │ 💾 (纯 C++ 计算与算法执行，无 GUI 依赖)
      ▼
数据持久层 (MetadataManager SCCH / SQLite DB)
```

### 4.1 【方案 A 落地】Delegate 重命名解耦与模型数据流向上微循环

为了实现 Delegate 的纯净化设计，本方案要求彻底删除 Delegate 内部的所有 parentWidget() 向上攀爬强转逻辑（对应用户原话：“那就按照方案A来实现吧”）：

```cpp
// 1. 净化后的委托层 (ThumbnailDelegate.cpp)
// 仅负责将编辑器里的新文字无差别地塞给模型，不干涉任何高层逻辑
void ThumbnailDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
    if (!lineEdit) return;

    QString newName = lineEdit->text().trimmed();
    if (newName.isEmpty()) return;

    // 🚀【方案 A 核心】：仅调用标准的 setData，没有任何 parent 向上引用的非标代码！
    model->setData(index, newName, Qt::EditRole);
}

// 2. 增强后的虚拟模型层 (FerrexVirtualDbModel.cpp)
bool FerrexVirtualDbModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (role == Qt::EditRole) {
        QString oldPath = index.data(PathRole).toString();
        QString newName = value.toString();

        // 驱动核心 Service 层执行重命名和对账
        QString newPath = ShellHelper::renameFile(oldPath, newName);
        if (!newPath.isEmpty()) {
            // 同步修改内存元数据缓存
            MetadataManager::instance().renameItem(oldPath.toStdWString(), newPath.toStdWString());

            // 🚀【方案 A 核心】：操作成功后，模型自身发射具有语义的高阶信号，向上传播！
            emit recordRenamed(oldPath, newPath, newName);
            return true;
        }
    }
    return false;
}

// 3. 高层 UI 容器被动消费与滚动重新对齐 (ContentPanel.cpp)
// 表现层初始化时，仅监听模型的 recordRenamed 信号
connect(m_model, &FerrexVirtualDbModel::recordRenamed, this, [this](const QString& oldPath, const QString& newPath, const QString& newName) {
    // 被动响应并优雅滚动定位，完全不需要 Delegate 主动向上寻址！
    this->setPendingSelectName(newName, false);
    this->selectAndScrollToPath(newPath);
});
```

### 4.2 【命令总线落地】右键上下文菜单事件解耦设计

移除 `ContentPanel::onCustomContextMenuRequested` 内部的所有物理执行代码，完全通过命令总线异步调度。

1. **定义命令基类 (ActionCommand)**：
```cpp
class ActionCommand {
public:
    virtual ~ActionCommand() = default;
    virtual bool redo() = 0; // 执行命令
    virtual bool undo() = 0; // 撤销命令
};
```

2. **实现具体的删除命令 (SecureDeleteCommand)**：
```cpp
class SecureDeleteCommand : public ActionCommand {
public:
    explicit SecureDeleteCommand(const QStringList& paths) : m_targetPaths(paths) {}

    bool redo() override {
        for (const auto& path : m_targetPaths) {
            // 在后台线程真正执行物理删除和元数据移除对账
            QFile::remove(path);
            MetadataManager::instance().removeMetadataSync(path.toStdWString());
        }
        return true;
    }

    bool undo() override {
        // 若有回收站机制，在此执行还原对账操作
        return false;
    }
private:
    QStringList m_targetPaths;
};
```

3. **UI 表现层高度收敛，只负责组装和派发**：
```cpp
void ContentPanel::onCustomContextMenuRequested(const QPoint& pos) {
    QModelIndexList selected = getSelectedIndexes();
    if (selected.isEmpty()) return;

    QMenu menu(this);
    QAction* actDelete = menu.addAction(UiHelper::getIcon("delete"), "安全物理删除");

    QAction* selectedAct = menu.exec(QCursor::pos());
    if (selectedAct == actDelete) {
        QStringList paths;
        for (const auto& idx : selected) paths << idx.data(PathRole).toString();

        // 🚀【命令总线核心】：构建删除命令并无差别塞给中央调度总线
        UndoManager::instance().push(new SecureDeleteCommand(paths));
    }
}
```

---

## 5. 修改边界声明【范围】

本方案严格定义了后续实际重构代码时（执行者角色下）的绝对物理界限：

**本次方案涉及范围：**
- [ ] 仅进行全局架构模块化重构规划的设计建档。无任何实体代码改动。

**明确禁止越界修改的范围：**
- [ ] 严禁修改任何 `.cpp`, `.h`, `.cmake` 等程序文件。
- [ ] 严禁运行任何编译、构建、或实质性的代码变更测试。

---

## 6. 实现准则与预警【核心】

1. **编译依赖安全**：在后续执行重构时，由于大量逻辑从 `ContentPanel` 迁移到 `FerrexVirtualDbModel`，必须小心循环包含问题。必须采用 **前置声明（Forward Declaration）** 并在 `.cpp` 中包含具体的实体头文件，严防 “incomplete type” 编译错误。
2. **多线程安全性**：任何由命令总线派发的后台异步操作（如加密、删除），在操作底层数据库时，必须安全获取 `DatabaseManager::getDriveMutex`，防止发生多线程写锁竞争死锁。
3. **开箱即用与信号防风暴**：任何大规模重命名或删除后，应当由模型发出总数变化，一次性更新 UI，绝不可在循环内频繁发射 metaChanged 导致 UI 频繁刷白闪烁。

---

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 输入框清除 | 每个可编辑输入框配置 setClearButtonEnabled(true) | ✅ 本方案在后续任何 Dialog 输入框重建中将严格执行 |
| 异步防闪烁 | 异步数据扫描前禁止先行调用 m_model->clear() 避免闪烁 | ✅ 规划的 `DirectoryScanner` 抛出数据时将遵循原子替换，不破坏该逻辑 |
| 窗口置顶 | 激活置顶强制使用 Win32 原生 SetWindowPos 配合 SWP_NOSENDCHANGING | ✅ 规划中完美保留此 Win32 稳定性能保障 |

---

## 8. 待确认事项

- **待确认 1**：未来启动这一全局大重构时，是否需要将 `UndoManager`（撤销栈）的 UI 面板（可展示历史操作队列）作为配套功能一同规划呈现出来？
- **待确认 2**：在模型信号向上流微循环中，若重命名由于磁盘写保护而中途报错，是否需配置通用的 `DialogErrorBridge` 信号投递链路？
