这是一份专为全团队及后续开发 AI 制定的**《QuarkMeta 防补丁式故障排查与根因修复指南（ROOT_CAUSE_REPAIR_GUIDE.md）》**。

本指南将常见 Bug 的“毒瘤补丁做法”与“正规根因解法”进行了**逐一明细对照**，并在修复前强制执行四步归因流程，从制度上彻底杜绝“自自然然又去打补丁”的下意识行为。

---

# 📖 QuarkMeta 防补丁式故障排查与根因修复指南
`ROOT_CAUSE_REPAIR_GUIDE.md`

---

## 🚫 常见故障与“严禁补丁 vs 根因正解”对照表

当你面对任何具体 Bug 时，**严禁使用左侧的补丁方案，必须严格执行右侧的根因正解**：

| 故障现象 | ❌ 严禁采用的毒瘤补丁（一律驳回） | ✅ 架构级根因正解（唯一标准） |
| :--- | :--- | :--- |
| **1. 托盘右键菜单点击无反应/秒退** | 严禁在 `onTrayActivated` 中调用 Win32 `SetForegroundWindow` 强夺焦点，严禁在菜单注入 `Qt::FramelessWindowHint`。 | **根因**：交由 `m_trayIcon->setContextMenu(m_trayMenu)` 原生托管，并确保 `WindowDeactivate` 不误判自身子控件。 |
| **2. 表头文字与内容列错位/截断** | 严禁在 QSS 中使用 `QHeaderView::section:first { padding-left: ... }` 硬推文字位置。 | **根因**：在 `HeaderView` 内部重写 `paintSection` 对齐 Delegate 缩略图 X 坐标，首列设为 `QHeaderView::Stretch`。 |
| **3. 托盘/弹窗菜单出现系统白底** | 严禁在具体的控制器（如 `TrayController.cpp`）内部硬编码写死一段 QSS 字符串。 | **根因**：将全局样式表通过 `ThemeManager::initialize(&app)` 注入到全局 `qApp` 上，全软件自动统一继承。 |
| **4. 属性修改后界面未变/被旧值覆盖** | 严禁在 Model（`DiskItemModel`）的 `updateRecordMetadata` 中直接用 `QFile` 打开硬盘上的 `.QuarkMeta.json`。 | **根因**：Model 只能从内存缓存（`MetadataManager::getMeta()`）0ms 读实时数据，磁盘写盘由底盘 50ms 自动异步固化。 |
| **5. 快捷键在打字时误触发全局功能** | 严禁在 `eventFilter` 中手写 `if (keyEvent->key() == Qt::Key_Z)` 拦截按键。 | **根因**：基于标准 `QShortcut` 构建，并显式指定作用域为 `Qt::WindowShortcut`，由 Qt 自动保障输入框优先权。 |
| **6. 多选打标导致文件私有标签丢失** | 严禁收集当前 UI 面板的全部 TagPill 字符串并全量覆写（`setTags`）回所有选中文件。 | **根因**：采用 **Delta 差集模式**（仅发射 `tagAddRequested(paths, newTag)` 与 `tagRemoveRequested(paths, delTag)`）。 |
| **7. 悬停窗口边缘无法拉伸/无箭头** | 严禁在主窗口重新写一套 `mouseMoveEvent` 算坐标。 | **根因**：`FramelessWindowHelper` 必须将事件过滤器安装在 `QCoreApplication::instance()` 全局总线上，穿透子控件遮蔽。 |
| **8. 删除文件在不同入口行为不一致** | 严禁在按键处写 `QFile::remove`，在右键处写 `executeWithSnapshot`，在预览处调 `ShellHelper`。 | **根因**：全系统所有入口统一且只能调用 `TrashService::instance().moveToTrash` 或 `PermanentDeleteService::instance().execute`。 |

---

## 🔍 根因排查四步归因法 (The 4-Step Root Cause Protocol)

每次动手修改代码前，必须先在脑中走完这 4 步自检：

```
                           【根因排查 4 步归因模型】

  第 1 步：【查数据流向】
  • 这个数据目前谁是唯一真理源 (SSOT)？
  • 读数据是不是从内存缓存读？写数据是不是单向流经领域服务？
  • 严禁 View 绕过 Service 直接去撬数据库或读写磁盘！
             │
             ▼
  第 2 步：【查事件上下文】
  • 事件是在哪个层级被触发的？
  • 是否存在子控件覆盖拦截、模态焦点冲突、或 WindowFlags 破坏了原生事件链？
  • 严禁使用 OS 平台级 API 进行暴力强制夺焦！
             │
             ▼
  第 3 步：【查职责边界】
  • 拟修改的代码到底属于五层架构中的哪一层？
  • 是否存在把算法写在 UI 控件里、把业务写在 Model 里的越权行为？
  • 任何跨模块交互严禁声明 friend class！
             │
             ▼
  第 4 步：【宁建服务，不加补丁】
  • 如果发现功能缺乏统一入口，宁可独立新建一个 Service（如 TrashService），
    也绝对不在当前的 eventFilter 或 switch-case 里多塞一行私有逻辑！
```

---

## 📋 代码提交前“防补丁”军规自查清单 (Pre-Merge Checklist)

在任何代码完成编译前，凡命中以下任意一条，**该代码直接判定为违规补丁，必须撤回重构**：

- [ ] **检查 1**：代码中是否出现了 `friend class`？（**绝对禁止**）
- [ ] **检查 2**：控制器或视图中是否硬编码了大段 CSS/QSS 字符串？（**绝对禁止，统一归入 `ThemeManager`**）
- [ ] **检查 3**：UI 控件中是否出现了 `QFile::remove`、`QDir::removeRecursively`、`CreateFileW` 等底层 I/O？（**绝对禁止**）
- [ ] **检查 4**：是否使用了 `QApplication::topLevelWidgets()` 或 `findChild` 在运行时跨窗口搜刮私有指针？（**绝对禁止**）
- [ ] **检查 5**：是否在处理大批量数据时，在 `for` 循环内部逐个发射全局事件？（**绝对禁止，必须 1 次聚合批量发射**）
- [ ] **检查 6**：是否有新功能只改了局部某一个入口，而漏掉了快捷键/菜单的其他同类入口？（**绝对禁止，必须封装为统一 Service 函数**）

---

### 💡 指南总结：
**“表象出问题，必是底层契约有漏洞；治标不治本，补丁终成烂代码。”**  
只要严格按照本指南的四步法排查，全工程将永远免疫“打补丁”的恶性循环！