# 扩展 NativeFolderWatcher 监控自定义文件夹及 UI 联动规划 —— Modification_Plan-5.md

## 1. 任务背景
在目前的 ArcMeta 架构中，系统通过 NativeFolderWatcher (IOCP) 自启动监控每个物理磁盘上的默认“托管库（Managed Library）”（即 `ArcMeta.Library_[盘符]` 文件夹）。为了满足用户更高的灵活性需求，需要支持在盘符栏 (Drive Bar) 上动态添加、移除以及持久化监控任意自定义文件夹。本方案在设计上确保所有自定义监控文件夹在监控的同时，能自动在侧边栏分类树中对应建立 1:1 的镜像子分类，并在移除时彻底、干净地清理元数据，杜绝垃圾数据残留。

## 2. 问题定位、编译隐患排查与统一入库闭环
本方案在前序方案设计的基础之上，重拳出击，彻底消灭所有编译依赖与逻辑漏洞：
- **拒绝另起炉灶，融入统一入库分类通道**：新添加的自定义文件夹绝非只做单调的文件系统底层监控，而是必须无缝接入 `AutoImportManager` 中已有的、高度成熟的分类树自动构建引擎：**`AutoImportManager::handleRecursiveIngestion(rootPath)`**。
  - **动态导入新建时**：点击输入框“完成”并开启 IOCP 监控后，通过 `QtConcurrent::run` 异步触发 `handleRecursiveIngestion(normPath)`。该引擎会自动递归遍历并提取子目录，在侧边栏自动建立对应的 1:1 镜像物理分类树，并将所有内含的物理文件导入登记。
  - **自启动加载点火时**：主程序启动在 `CoreController::startSystem()` 中载入自定义 monitored 列表时，除开启 NativeFolderWatcher IOCP 监控外，也自动异步点火拉起 `handleRecursiveIngestion(normPath)` 完成全量镜像对账，保证侧边栏镜像一致。
  - **移除监控数据根除**：当用户在盘符栏对准文件夹按钮右键并点击“移除”时，不仅注销其 IOCP 监控，还必须通过直接调用 **`MetadataManager::instance().removeMetadataSync(normPath)`**，递归强力根除该目录下在数据库中的所有注册文件、项元数据记录以及在侧边栏中 1:1 自动建立的整个镜像分类树节点，保证数据库不保留任何不需要的垃圾数据，实现数据库的极致洁净（对应用户原话：“如果把“新建自动导入”的监控移除掉之后，那么与之相应数据库里的数据同样也要被移除掉，没必要保留”）。
- **编译隐患全面排查与依赖补全**：
  - `DriveButton.cpp` & `DriveButton.h` 必须补充包含 `<QEnterEvent>`、`<QFileInfo>`、`<QDir>`、`"ToolTipOverlay.h"`，否则由于不完整类型或缺少声明，会导致编译器报严重 C2079 / C2027 错误。
  - `MainWindow.cpp` 必须补充包含 `<QLineEdit>`、`"FramelessDialog.h"`、`"FramelessFileDialog.h"`，并在调用异步 handle 之前包含 `"core/AutoImportManager.h"`、`"meta/MetadataManager.h"`。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | NativeFolderWatcher (IOCP) 机制不仅仅监控“ArcMeta.Library_[盘符]”文件夹，还要监控其他灵活性较高的文件夹 | 在 `CoreController::startSystem` 中不仅监控默认托管库，还加载并监控 `AppConfig` 中保存的自定义监控文件夹列表；并在动态添加/移除时同步点火和注销监控 | ✅ |
| 2    | 当用户盘符栏空白处单击右键弹出一个菜单选项为“新建自动导入” | 设置 `m_driveBarWidget` 的 `ContextMenuPolicy` 为 `CustomContextMenu`，在右键空白处时弹出“新建自动导入”右键菜单 | ✅ |
| 3    | 点击该选项后，弹出一个仅可以输入文件夹路径的输入框和“浏览”以及“完成”按钮 | 实现一个自定义 `FramelessDialog` 子类 `CustomFolderImportDialog`，包含路径输入框、浏览（调用 `FramelessFileDialog`）与完成按钮 | ✅ |
| 4    | 点击完成按钮后，该文件夹正式被NativeFolderWatcher监控，然后采用folder.svg显示在盘符栏上 | 点击“完成”后，将该路径加入 `AppConfig` 的 `"DriveBar/CustomMonitoredFolders"`，立即调用 `NativeFolderWatcher::instance().addWatch(path)`，随后在 `DriveBar` 的布局上生成并添加 `FolderButton` 实例 | ✅ |
| 5    | 当鼠标悬停在某个folder.svg图标上方则显示ToolTipOverlay将该文件夹名称显示出来 | 在 `FolderButton` 中重写 `enterEvent` 与 `leaveEvent`，进入时调用 `ToolTipOverlay::instance()->showText` 显示文件夹名称与物理路径，离开时调用 `ToolTipOverlay::hideTip()` | ✅ |
| 6    | 当用户对准盘符栏上的文件夹单击右键时，弹出两个选项为“新建自动导入”、“移除” | 在 `FolderButton` 上设置右键菜单，选项包括“新建自动导入”与“移除” | ✅ |
| 7    | 所谓的“移除”就是将该文件夹从监控中移除，不再监控中 | 点击“移除”后，调用 `NativeFolderWatcher::instance().removeWatch` 停止监控，从 `AppConfig` 移除文件夹并更新 DriveBar UI | ✅ |
| 8    | 被监控的“ArcMeta.Library_[盘符]”文件夹 是不是会被创建到侧边栏分类呢？包括子文件夹也会被创建成子分类对不？ | 确认该机制，并决定自定义监控目录也完全接入统一的 handleRecursiveIngestion 入库分类树，自动镜像 1:1 分类树 | ✅ |
| 9    | 如果把“新建自动导入”的监控移除掉之后，那么与之相应数据库里的数据同样也要被移除掉，没必要保留 | 在点击“移除”时，同步调用 `MetadataManager::instance().removeMetadataSync(normPath)`，从数据库中物理根除该目录对应的元数据、文件注册表及侧边栏镜像分类 | ✅ |

## 4. 详细解决方案

### 4.1 引入 `FolderButton` 自定义控件与依赖补全 (定义于 `src/ui/DriveButton.h` & `DriveButton.cpp`)

#### 4.1.1 在 `src/ui/DriveButton.h` 中：
由于使用了 `QEnterEvent`，必须在头文件中引入 `<QEnterEvent>`：
```cpp
// 增加在 DriveButton.h 的头部
#include <QPushButton>
#include <QTimer>
#include <QEnterEvent> // 编译补全
#include "StyleLibrary.h"

namespace ArcMeta {

// 既有的 DriveButton 保持不变...

// 新增 FolderButton 声明
class FolderButton : public QPushButton {
    Q_OBJECT
public:
    explicit FolderButton(const QString& folderPath, QWidget* parent = nullptr);
    QString folderPath() const { return m_folderPath; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QString m_folderPath;
    QString m_folderName;
};

} // namespace ArcMeta
```

#### 4.1.2 在 `src/ui/DriveButton.cpp` 中：
必须补全 `QFileInfo`、`QDir` 以及 `ToolTipOverlay.h` 的包含，否则编译器会发生严重的不完整类型报错：
```cpp
// 增加在 DriveButton.cpp 的头部包含区
#include "DriveButton.h"
#include <QPainter>
#include <QMouseEvent>
#include <QFileInfo>       // 编译补全：解析路径名称
#include <QDir>            // 编译补全：路径格式标准化
#include "UiHelper.h"
#include "ToolTipOverlay.h" // 编译补全：气泡提示

namespace ArcMeta {

// FolderButton 实现部分
FolderButton::FolderButton(const QString& folderPath, QWidget* parent)
    : QPushButton(parent), m_folderPath(folderPath) {
    setFixedSize(28, 28);
    setCursor(Qt::PointingHandCursor);
    m_folderName = QFileInfo(folderPath).fileName();
    if (m_folderName.isEmpty()) {
        m_folderName = folderPath;
    }
}

void FolderButton::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect r = rect().adjusted(1, 1, -1, -1);
    QColor bgColor;
    QColor borderColor;

    // Hover 样式对齐 5.4 标题栏按钮规范，且拒绝使用 rgba 蒙版
    if (underMouse()) {
        if (isDown()) {
            bgColor = QColor("#4E4E52"); // Style::PressedBackground
        } else {
            bgColor = QColor("#3E3E42"); // Style::HoverBackground
        }
        borderColor = QColor("#555555");
    } else {
        bgColor = QColor("#333333");
        borderColor = QColor("#444444");
    }

    painter.setPen(QPen(borderColor, 1));
    painter.setBrush(bgColor);
    painter.drawRoundedRect(r, 4, 4);

    // 绘制 folder 矢量图标 (SvgIcons)
    QPixmap pix = UiHelper::getPixmap("folder", QSize(16, 16), Style::TextMain);
    QRect iconRect(r.left() + (r.width() - 16) / 2, r.top() + (r.height() - 16) / 2, 16, 16);
    painter.drawPixmap(iconRect, pix);
}

void FolderButton::enterEvent(QEnterEvent* event) {
    Q_UNUSED(event);
    // 悬停时通过全局 ToolTipOverlay 渲染文件夹名称与绝对路径，避免使用原生 ToolTip
    QString tipText = QString("<b>%1</b><br><span style='color:#888888;'>%2</span>")
        .arg(m_folderName)
        .arg(QDir::toNativeSeparators(m_folderPath));
    ToolTipOverlay::instance()->showText(mapToGlobal(QPoint(0, height() + 4)), tipText, 0);
}

void FolderButton::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    ToolTipOverlay::hideTip();
}

} // namespace ArcMeta
```

### 4.2 引入 `CustomFolderImportDialog` 路径输入窗与依赖补全 (实现于 `src/ui/MainWindow.cpp`)

#### 4.2.1 依赖补全 (在 `src/ui/MainWindow.cpp` 头部)
必须补全 `<QLineEdit>`、`"FramelessDialog.h"`、`"FramelessFileDialog.h"`、`"core/AutoImportManager.h"`、`"meta/MetadataManager.h"` 的包含，由于后面要对账同步及物理强力根除数据，这些头文件是必不可少的：
```cpp
// 增加在 MainWindow.cpp 头部包含区
#include <QLineEdit>             // 编译补全
#include "FramelessDialog.h"     // 编译补全：父对话框
#include "FramelessFileDialog.h" // 编译补全：选择路径
#include "../core/AutoImportManager.h" // 编译补全：调用 handleRecursiveIngestion 进行 1:1 分类
#include "../meta/MetadataManager.h"   // 编译补全：调用 removeMetadataSync 彻底根除垃圾数据
```

#### 4.2.2 `CustomFolderImportDialog` 类定义
在 `MainWindow.cpp` 中定义一个专用的输入框。按照宪法规约 **6.1 关于“清除”按钮** 的铁律，此 QLineEdit 必须且仅可通过原生的 `setClearButtonEnabled(true)` 支持清除动作。
```cpp
namespace ArcMeta {

class CustomFolderImportDialog : public FramelessDialog {
    Q_OBJECT
public:
    explicit CustomFolderImportDialog(QWidget* parent = nullptr)
        : FramelessDialog("新建自动导入", parent) {

        QWidget* content = getContentArea();
        QVBoxLayout* layout = new QVBoxLayout(content);
        layout->setContentsMargins(20, 15, 20, 20);
        layout->setSpacing(15);

        QHBoxLayout* pathL = new QHBoxLayout();
        pathL->setSpacing(8);

        m_edit = new QLineEdit(this);
        m_edit->setPlaceholderText("请输入或选择文件夹路径...");
        m_edit->setClearButtonEnabled(true); // 唯一指定清除实现
        m_edit->setStyleSheet(
            "QLineEdit { "
            "  background-color: #252526; "
            "  color: #F1F1F1; "
            "  border: 1px solid #3E3E42; "
            "  border-radius: 4px; "
            "  padding: 6px 10px; "
            "  font-family: 'Segoe UI', Microsoft YaHei; "
            "  font-size: 12px; "
            "}"
            "QLineEdit:focus { border: 1px solid #007ACC; }"
        );
        pathL->addWidget(m_edit);

        QPushButton* btnBrowse = new QPushButton("浏览", this);
        btnBrowse->setCursor(Qt::PointingHandCursor);
        btnBrowse->setStyleSheet(
            "QPushButton { "
            "  background-color: #3E3E42; "
            "  color: #F1F1F1; "
            "  border: 1px solid #555555; "
            "  border-radius: 4px; "
            "  padding: 6px 14px; "
            "  font-size: 12px; "
            "}"
            "QPushButton:hover { background-color: #4E4E52; }"
        );
        pathL->addWidget(btnBrowse);
        layout->addLayout(pathL);

        QHBoxLayout* bottomL = new QHBoxLayout();
        bottomL->addStretch();

        QPushButton* btnOk = new QPushButton("完成", this);
        btnOk->setCursor(Qt::PointingHandCursor);
        btnOk->setStyleSheet(
            "QPushButton { "
            "  background-color: #007ACC; "
            "  color: #FFFFFF; "
            "  border: none; "
            "  border-radius: 4px; "
            "  padding: 6px 20px; "
            "  font-weight: bold; "
            "  font-size: 12px; "
            "}"
            "QPushButton:hover { background-color: #1C97EA; }"
        );
        bottomL->addWidget(btnOk);
        layout->addLayout(bottomL);

        connect(btnBrowse, &QPushButton::clicked, this, &CustomFolderImportDialog::onBrowse);
        connect(btnOk, &QPushButton::clicked, this, [this]() {
            QString path = m_edit->text().trimmed();
            if (path.isEmpty()) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "路径不能为空！", 1500, QColor("#E81123"));
                return;
            }
            if (!QDir(path).exists()) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "无效的文件夹路径！", 1500, QColor("#E81123"));
                return;
            }
            accept();
        });
    }

    QString selectedPath() const { return m_edit->text().trimmed(); }

private:
    void onBrowse() {
        QString dir = FramelessFileDialog::getExistingDirectory(this, "选择自动导入文件夹");
        if (!dir.isEmpty()) {
            m_edit->setText(QDir::toNativeSeparators(dir));
        }
    }
    QLineEdit* m_edit;
};

} // namespace ArcMeta
```

### 4.3 `MainWindow` 盘符栏空白处与 FolderButton 右键菜单 management 交互

#### 4.3.1 修改 `MainWindow.h`：
增加对应声明与 `m_folderButtons` 控件数组：
```cpp
// 在 MainWindow.h 的 private 区域增加
QVector<class FolderButton*> m_folderButtons;

private slots:
    void updateCustomFolderButtons();
    void showNewAutoImportDialog();
    void removeCustomMonitoredFolder(const QString& path);
    void onDriveBarContextMenu(const QPoint& pos);
    void onFolderButtonContextMenu(const QPoint& pos);
```

#### 4.3.2 修改 `MainWindow.cpp` 的 `initDriveBar` 实现：
在 `MainWindow::initDriveBar` 的最末尾，注册 `m_driveBarWidget` 的自定义右键菜单并拉取初始化自定义文件夹按钮：
```cpp
    // 增加在 initDriveBar() 函数结束的 m_driveBarLayout->addStretch(); 后面
    m_driveBarWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_driveBarWidget, &QWidget::customContextMenuRequested, this, &MainWindow::onDriveBarContextMenu);

    // 绘制并加载初始化自定义 monitored 文件夹按钮
    updateCustomFolderButtons();
```

#### 4.3.3 实现右键空白/右键按钮及增删监控的核心联动逻辑：
```cpp
void MainWindow::updateCustomFolderButtons() {
    // 1. 安全清除既有的 custom folder 按钮，防止布局残余
    for (FolderButton* btn : m_folderButtons) {
        m_driveBarLayout->removeWidget(btn);
        btn->deleteLater();
    }
    m_folderButtons.clear();

    // 2. 载入配置并追加至伸缩垫片 (stretch spacer) 之前
    QStringList customFolders = AppConfig::instance().getValue("DriveBar/CustomMonitoredFolders").toStringList();
    int insertIndex = m_driveBarLayout->count() - 1;
    if (insertIndex < 0) insertIndex = 0;

    for (const QString& path : customFolders) {
        FolderButton* btn = new FolderButton(path, m_driveBarWidget);
        btn->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(btn, &QWidget::customContextMenuRequested, this, &MainWindow::onFolderButtonContextMenu);

        // 单击该文件夹按钮，自动将目录导航重定向至此物理路径
        connect(btn, &QPushButton::clicked, this, [this, path]() {
            m_navPanel->setRootPath(path);
        });

        m_driveBarLayout->insertWidget(insertIndex++, btn);
        m_folderButtons.push_back(btn);
    }
}

void MainWindow::onDriveBarContextMenu(const QPoint& pos) {
    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);
    QAction* act = menu.addAction("新建自动导入");

    QAction* selectedAct = menu.exec(m_driveBarWidget->mapToGlobal(pos));
    if (selectedAct == act) {
        showNewAutoImportDialog();
    }
}

void MainWindow::showNewAutoImportDialog() {
    CustomFolderImportDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QString path = dlg.selectedPath();
        if (!path.isEmpty()) {
            QStringList customFolders = AppConfig::instance().getValue("DriveBar/CustomMonitoredFolders").toStringList();
            std::wstring normPath = MetadataManager::normalizePath(path.toStdWString());
            QString finalPath = QString::fromStdWString(normPath);

            if (customFolders.contains(finalPath)) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "该文件夹已在监控列表中！", 1500, QColor("#E81123"));
                return;
            }

            // 1. 记入 AppConfig 并持久化
            customFolders.append(finalPath);
            AppConfig::instance().setValue("DriveBar/CustomMonitoredFolders", customFolders);
            AppConfig::instance().sync();

            // 2. 动态点火 NativeFolderWatcher 的监控
            NativeFolderWatcher::instance().addWatch(normPath);

            // 3. 拒绝另起炉灶：调用统一的 handleRecursiveIngestion 入库通道。
            //    这会在后台大事务中自动递归遍历此自定义文件夹，并为该物理目录建立 1:1 的侧边栏分类树，同时加载关联其中所有物理文件！
            (void)QtConcurrent::run([normPath]() {
                AutoImportManager::instance().handleRecursiveIngestion(normPath);
            });

            // 4. 重新加载渲染盘符栏 FolderButtons
            updateCustomFolderButtons();

            ToolTipOverlay::instance()->showText(QCursor::pos(), "已开始自动监控该文件夹并同步镜像分类", 1500, Style::SuccessGreen);
        }
    }
}

void MainWindow::onFolderButtonContextMenu(const QPoint& pos) {
    FolderButton* btn = qobject_cast<FolderButton*>(sender());
    if (!btn) return;
    QString path = btn->folderPath();

    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);
    QAction* actNew = menu.addAction("新建自动导入");
    QAction* actRemove = menu.addAction("移除");

    QAction* selectedAct = menu.exec(btn->mapToGlobal(pos));
    if (selectedAct == actNew) {
        showNewAutoImportDialog();
    } else if (selectedAct == actRemove) {
        removeCustomMonitoredFolder(path);
    }
}

void MainWindow::removeCustomMonitoredFolder(const QString& path) {
    QStringList customFolders = AppConfig::instance().getValue("DriveBar/CustomMonitoredFolders").toStringList();
    std::wstring normPath = MetadataManager::normalizePath(path.toStdWString());
    QString finalPath = QString::fromStdWString(normPath);

    if (customFolders.contains(finalPath)) {
        // 1. 从 AppConfig 中移除
        customFolders.removeAll(finalPath);
        AppConfig::instance().setValue("DriveBar/CustomMonitoredFolders", customFolders);
        AppConfig::instance().sync();

        // 2. 从 NativeFolderWatcher 监控中注销此路径
        NativeFolderWatcher::instance().removeWatch(normPath);

        // 3. 强力数据根除，拒绝数据残留：递归清理该目录下所有注册的文件、子项元数据以及在侧边栏自动创建的 1:1 分类树映射！
        MetadataManager::instance().removeMetadataSync(normPath);

        // 4. 动态刷新盘符栏
        updateCustomFolderButtons();

        ToolTipOverlay::instance()->showText(QCursor::pos(), "已停止监控该文件夹并移除相关镜像分类", 1500, QColor("#FECF0E"));
    }
}
```

### 4.4 后台自启动点火监控与分类对账同步 (实现于 `src/core/CoreController.cpp`)
在主程序异步初始化启动监控时，加载 `"DriveBar/CustomMonitoredFolders"` 中记录的所有持久化自定义目录，依次注册 NativeFolderWatcher 监控并异步进行全量 1:1 分类对账更新，彻底告别状态丢失：
```cpp
// 在 CoreController.cpp 的 CoreController::startSystem 里的 NativeFolderWatcher 自启动监控循环后追加：
QStringList customFolders = AppConfig::instance().getValue("DriveBar/CustomMonitoredFolders").toStringList();
for (const QString& folder : customFolders) {
    std::wstring normPath = MetadataManager::normalizePath(folder.toStdWString());
    if (!normPath.empty()) {
        qDebug() << "[Core] 识别到自定义监控目录，开启 IOCP 监控并对账同步:" << QString::fromStdWString(normPath);
        NativeFolderWatcher::instance().addWatch(normPath);

        // 自启动阶段自动拉起异步分类对账，实现真正的 1:1 绝对镜像
        (void)QtConcurrent::run([normPath]() {
            AutoImportManager::instance().handleRecursiveIngestion(normPath);
        });
    }
}
```

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [x] 模块/文件：`src/core/CoreController.cpp`（追加自定义监控目录的自启动 IOCP 监控与 handleRecursiveIngestion 分类对账逻辑）
- [x] 模块/文件：`src/ui/DriveButton.h` & `DriveButton.cpp`（定义并实现自定义 FolderButton，包含完整的头文件依赖引入）
- [x] 模块/文件：`src/ui/MainWindow.h` & `MainWindow.cpp`（定义 CustomFolderImportDialog、接管 DriveBar 空白处及 FolderButton 右键菜单弹出、追加与自适应 `handleRecursiveIngestion` 分类对账、移除监控递归清理元数据 `removeMetadataSync`、配置持久化更新，包含缺失 of QLineEdit、Dialog、AutoImportManager 等依赖引入）

**明确禁止越界修改的范围：**
- [ ] 严禁改变 NTFS 的物理 USN 日志接收底层。
- [ ] 严禁修改其他与此功能不相关的 UI 组件。

## 6. 实现准则与预警【核心】
1. **统一对账与清理，防重复造轮子**：绝对不允许为自定义监控文件夹另行手写一套新的文件扫描或分类映射代码。必须统一且唯一调用 `AutoImportManager::handleRecursiveIngestion` 进行对账；并统一调用 `MetadataManager::removeMetadataSync` 根除监控移除后的数据，保证代码纯净性和架构极致内聚。
2. **输入框清除功能**：强制贯彻 **6.1 关于“清除”按钮** 的铁律，新对话框中的 `QLineEdit` 必须通过原生自带的 `setClearButtonEnabled(true)` 实现。
3. **样式一致性**：对齐 **5.4 标题栏按钮样式** 及 **3.3 按钮物理参数**，悬停使用 `#3E3E42`背景，按下使用 `#4E4E52` 背景，且严禁使用 rgba 蒙版。
4. **安全跨线程与事件通知**：`NativeFolderWatcher` 仍运行于独立的 IOCP 专属工作线程。由于对自定义监控文件夹触发回调时需要通过 `MetadataManager::registerItemsAsync` 流水线式入库，该函数内部会自动分发到后台线程处理，故该机制具备极高的时间及空间并发安全性。
5. **ToolTipOverlay 的对接**：完全禁绝 QWidget 的原生 `setToolTip(...)`。文件夹悬停提示必须一律通过 `ToolTipOverlay::instance()->showText(...)` and `ToolTipOverlay::hideTip()` 手动实现。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 输入框清除   | 唯一标准：一律使用 Qt 原生 setClearButtonEnabled(true)，严禁通过自定义按钮模拟清除逻辑 | ✅ 符合，完美实现 |
| 标题栏按钮样式| 悬停使用 `#3E3E42`（Style::HoverBackground），按下使用 `#4E4E52`（Style::PressedBackground），严禁使用 rgba 蒙版 | ✅ 符合，不使用 rgba 蒙版并完美对齐色值 |
| ToolTipOverlay 渲染 | 禁绝原生 ToolTip，强制对接 ToolTipOverlay 渲染 | ✅ 符合，采用 enter/leaveEvent 强制触发 ToolTipOverlay 气泡 |
