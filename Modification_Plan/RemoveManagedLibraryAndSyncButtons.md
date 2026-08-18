# QuarkMeta 彻底拔除“创建资源库”与“元数据同步按钮”无脑实施方案 (RemoveManagedLibraryAndSyncButtons)

## 1. 方案背景与清理目标

在独立的 QuarkMeta 纯磁盘直连模式下：
1. **“创建资源库”（及其右键菜单）** 是原 ArcMeta 内存托管模式遗留的僵尸功能。纯磁盘直连应用不再需要用户创建 `QuarkMeta.Library_X` 托管文件夹，所有磁盘盘符及普通文件夹均可直接浏览与做标。
2. **“元数据同步”（标题栏旋转按钮 `m_btnSync`）** 也是内存异步数据库遗留的僵尸按钮。纯磁盘模式下，元数据写入均实时即时落地（写入磁盘离散 JSON 或 `global.db`），无需任何中间状态与同步进度展示。

本方案提供一份**按步骤、精准定位文件路径与代码行号**的无脑清理与拔除指南，彻底根除上述僵尸 UI 与对应逻辑。

---

## 2. 涉及清理的文件与精准代码行号

| 文件路径 | 代码行号 / 位置 | 清理内容 |
| :--- | :--- | :--- |
| `src/ui/MainWindow.h` | **Line 80, Line 81, Line 168** | 删除 `onDriveButtonContextMenu`、`rescanManagedLibrary` 槽函数声明与 `m_btnSync` 成员指针 |
| `src/ui/MainWindow.cpp` | **Line 1424 - Line 1445** | 删除 `m_btnSync` 创建与 `SyncStatusService` 信号监听 |
| `src/ui/MainWindow.cpp` | **Line 1514** | 删除 `layout->addWidget(m_btnSync, ...)` 布局添加 |
| `src/ui/MainWindow.cpp` | **Line 1860 - Line 1867** | 清除 DriveButton 的 `Active/Inactive` 状态判定与 `customContextMenuRequested` 右键绑定 |
| `src/ui/MainWindow.cpp` | **Line 1899 - Line 1955** | 彻底删除 `onDriveButtonClicked`、`onDriveButtonContextMenu` 及 `rescanManagedLibrary` 函数实现 |

---

## 3. 详细分步骤无脑清理指南

### 步骤一：清理 `src/ui/MainWindow.h` 头文件

打开 `src/ui/MainWindow.h`：

1. **删除槽函数声明（Line 80 - Line 81 附近）**：
   ```cpp
   // 删除以下 2 行：
   void onDriveButtonContextMenu(const QPoint& pos);
   void rescanManagedLibrary(const QString& libraryPath);
   ```

2. **删除成员变量（Line 168 附近）**：
   ```cpp
   // 删除以下 1 行：
   QPushButton* m_btnSync   = nullptr;
   ```

---

### 步骤二：清理 `src/ui/MainWindow.cpp` 中的标题栏同步按钮 `m_btnSync`

打开 `src/ui/MainWindow.cpp`：

1. **清理按钮初始化与监听（Line 1424 - Line 1445）**：
   删除 `setupCustomTitleBarButtons()` 中有关 `m_btnSync` 的整段创建、信号绑定与点击槽函数：
   ```cpp
   // 彻底删除以下整段代码：
   m_btnSync = createTitleBtn("sync");
   m_btnSync->setProperty("tooltipText", "元数据已同步至物理文件");
   m_btnSync->installEventFilter(m_hoverFilter);

   connect(&SyncStatusService::instance(), &SyncStatusService::statusUpdated, this, [this](bool syncing, int count) {
       if (syncing) {
           m_btnSync->setIcon(UiHelper::getIcon("sync", ErrorRed));
           m_btnSync->setProperty("tooltipText", QString("正在同步元数据 (%1 项待落盘)...").arg(count));
       } else {
           m_btnSync->setIcon(UiHelper::getIcon("sync", TextMain));
           m_btnSync->setProperty("tooltipText", "元数据已同步至物理文件");
       }
   });

   connect(m_btnSync, &QPushButton::clicked, this, [this]() {
       if (SyncStatusService::instance().isSyncing()) {
           ToolTipOverlay::instance()->showText(m_btnSync->mapToGlobal(QPoint(0,0)), "同步正在进行中...", 1500);
       } else {
           ToolTipOverlay::instance()->showText(m_btnSync->mapToGlobal(QPoint(0,0)), "元数据已全部落地", 1500);
       }
   });
   ```

2. **清理标题栏按钮布局（Line 1514 附近）**：
   从标题栏按钮容器布局中删除 `m_btnSync` 的添加：
   ```cpp
   // 删除以下 1 行：
   layout->addWidget(m_btnSync, 0, Qt::AlignVCenter);
   ```

---

### 步骤三：清理 `src/ui/MainWindow.cpp` 中的盘符栏“创建资源库”及右键菜单

打开 `src/ui/MainWindow.cpp`：

1. **清理 `initDriveBar()` 中的盘符初始化（Line 1860 - Line 1867）**：
   盘符按钮作为纯粹的导航按钮，不再需要加载资源库状态与绑定右键菜单：
   ```cpp
   // 修改前：
   connect(btn, &QPushButton::clicked, this, &MainWindow::onDriveButtonClicked);
   btn->setContextMenuPolicy(Qt::CustomContextMenu);
   connect(btn, &QWidget::customContextMenuRequested, this, &MainWindow::onDriveButtonContextMenu);

   QString managedPath = drive.absolutePath() + "QuarkMeta.Library_" + letter.left(1);
   if (QDir(managedPath).exists()) {
       btn->setState(DriveButton::Active);
   } else {
       btn->setState(DriveButton::Inactive);
   }

   // 替换为简化的直连导航绑定：
   connect(btn, &QPushButton::clicked, this, [this, letter]() {
       unifiedNavigateTo(letter + "/");
   });
   ```

2. **删除僵尸函数实现（Line 1899 - Line 1955）**：
   彻底删除以下 3 个函数的全文实现：
   - `void MainWindow::onDriveButtonClicked()`
   - `void MainWindow::onDriveButtonContextMenu(const QPoint& pos)`
   - `void MainWindow::rescanManagedLibrary(const QString& libraryPath)`

---

## 4. 拔除后的预期效果与验证方法

1. **标题栏整洁无冗余**：
   标题栏右侧工具栏不再显示无意义的旋转“同步”图标，按钮间距与视觉更加精简。
2. **盘符栏响应直接高效**：
   - 盘符栏按钮上的 `C:`、`D:`、`G:` 等图标恢复为干净的统一状态；
   - 单击盘符按钮即可瞬间直达该磁盘根目录；
   - 盘符按钮不再弹出“创建资源库”、“重新扫描该库”等无效僵尸右键菜单。
