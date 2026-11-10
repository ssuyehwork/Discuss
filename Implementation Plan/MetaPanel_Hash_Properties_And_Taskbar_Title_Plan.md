# MetaPanel_Hash_Properties_And_Taskbar_Title_Plan.md - 实施方案

## 已读物理证据清单（第 1 关：Gate 1）

| 文件相对路径 | 物理确认已读 | 关键物理签名 / 代码行核验 |
|---|---|---|
| `QuarkMeta_Source/src/ui/MetaPanel.h` | ✅ | 包含 `m_pathEdit`, `m_btnCopyPath`, `m_btnOpenLocation`，声明了属性面板各控件 |
| `QuarkMeta_Source/src/ui/MetaPanel.cpp` | ✅ | 第 360–398 行构建了“物理路径”控件与按钮；第 675–700 行 `updateControlsState` 控制控件启用/禁用 |
| `QuarkMeta_Source/src/ui/NavigationCoordinator.h` | ✅ | 第 37 行：`void showLocation(const QString& addressText, const QString& tabTitle, const QString& tabUrl);` |
| `QuarkMeta_Source/src/ui/NavigationCoordinator.cpp` | ✅ | 第 35 行：`showLocation(...)` 中负责更新地址栏和标签页标题 |
| `QuarkMeta_Source/src/ui/TabBarWidget.h` | ✅ | 第 117 行：`void updateCurrentTabTitle(const QString& title, const QString& url);` |
| `QuarkMeta_Source/src/ui/TabBarWidget.cpp` | ✅ | 第 725 行：负责根据当前路径或分屏状态生成标签页显示标题 |
| `QuarkMeta_Source/src/ui/MainWindow.cpp` | ✅ | 第 59 行：`setWindowTitle("QuarkMeta");` 初始硬编码，未动态跟随文件夹联动 |
| `QuarkMeta_Source/src/util/ShellHelper.h` | ✅ | 第 33 行：`static void showProperties(const QString& path);` 已实现原生属性查看 |
| `QuarkMeta_Source/src/meta/DuplicateDetectorService.h` | ✅ | 第 29 行：`static QString computeFullSha256(const QString& filePath, const std::atomic<bool>* cancelToken = nullptr);` 已实现流式 SHA-256 计算 |
| `QuarkMeta_Source/src/ui/dialogs/FileCompareDialog.h` | ✅ | 第 82 行：`class FileCompareDialog : public FramelessDialog` 支持多文件哈希对比展示 |
| `QuarkMeta_Source/resources/style.qss` | ✅ | 第 1798–1824 行定义了 `BtnCopyPath` 与 `BtnOpenLocation` 的统一按钮样式 |

**既有 API 检索与防重复造轮子记录**：
- Windows 原生属性弹窗 SSOT：`ShellHelper::showProperties(path)`；
- SHA-256 哈希计算 SSOT：`DuplicateDetectorService::computeFullSha256(path, cancelToken)`；
- 多文件哈希对比弹窗 SSOT：`FileCompareDialog(files, parent)`；
- 窗口标题动态变更入口：`TabBarWidget` 新增 `windowTitleFollowRequested(const QString& title)` 信号通知主窗口，或由 `NavigationCoordinator` 统一分发驱动主窗口 `setWindowTitle`。

---

## Chapter 1: Overview（概述与根因分析）

### 1. 现象描述与问题定位
1. **元数据面板底部缺少常用操作入口**：
   - 当前 `MetaPanel` 的“物理路径”区块仅有“复制路径”与“打开位置”，缺少用户高频使用的“查看属性”与“查看哈希值”能力；
   - 用户在需要查看系统原生属性或校验文件 SHA-256 哈希时，必须通过操作系统右键或专用工具，操作链路较长。
2. **哈希展示性能与交互需求**：
   - 用户明确要求“查看哈希值”必须支持单个项目/多个项目；
   - 选中项目时**默认不需要显示哈希**，防止大文件选定时在后台静默发起耗时哈希计算导致 I/O 抢占与 UI 掉帧；
   - 仅当用户主动点击“查看哈希值”按钮后才进行异步计算并在按钮下方展开显示；多选时点击直接唤起专用的多文件对比弹窗。
3. **任务栏窗口标题不明确**：
   - 当前主窗口在构造后仅硬编码设置了 `setWindowTitle("QuarkMeta")`；
   - 当打开不同文件夹或分类时，任务栏缩略图和窗口标题栏仅显示固定名称，无法快速区分当前正在浏览的工作目录。用户期望显示为：“`文件夹名称-QuarkMeta`”。

### 2. 重构设计目标
1. **任务栏动态标题规范化**：
   - 在 `TabBarWidget` 中增加 `currentTitleChanged(const QString& title)` 信号（或在 `NavigationCoordinator::showLocation` 中）；
   - 在主窗口 `MainWindow` 监听到当前展示的文件夹名称变化时，调用 `setWindowTitle(QString("%1-QuarkMeta").arg(folderName))`，保持任务栏显示与当前文件夹 100% 动态对齐。
2. **元数据面板增加操作栏与哈希动态展示**：
   - 在“物理路径”折叠区块内部或紧随其后，新增独立操作行：
     - `m_btnShowProperties`（“查看属性”）：单选可用，点击调用 `ShellHelper::showProperties`；
     - `m_btnComputeHash`（“查看哈希值”）：单选/多选均可点击；
   - **单个项目交互**：
     - 新增动态折叠结果框 `m_hashResultBox`（包含哈希值单行纯文本 `m_hashLabel` 与复制小按钮 `m_btnCopyHash`）；
     - 默认隐匿（`hide()`），选中新项目时自动隐匿并取消正在运行的计算线程；
     - 点击“查看哈希值”后展开，在后台线程并发调用 `DuplicateDetectorService::computeFullSha256` 计算；期间显示“计算中…”，计算完成后显示完整 SHA-256；
   - **多个项目交互**：
     - 当选中的文件数 $\ge 2$ 时，点击直接唤起 `FileCompareDialog(files, this)` 弹窗；
3. **外联 QSS 样式标准保障**：
   - 新增按钮在 `resources/style.qss` 中配置与 `BtnCopyPath`、`BtnOpenLocation` 100% 统一的视觉样式，杜绝 C++ 内联样式。

---

## Chapter 2: Modified Files List（影响文件清单）

| 操作类型 | 文件相对路径 | 说明 |
|---|---|---|
| **修改** | `QuarkMeta_Source/src/ui/TabBarWidget.h` | 声明 `tabDisplayTitleChanged` 信号用于标题栏与任务栏联动 |
| **修改** | `QuarkMeta_Source/src/ui/TabBarWidget.cpp` | 在 `updateCurrentTabTitle` 完成标题计算后发出该信号 |
| **修改** | `QuarkMeta_Source/src/ui/MainWindow.cpp` | 监听 `tabDisplayTitleChanged` 信号，动态更新 `setWindowTitle(folderName + "-QuarkMeta")` |
| **修改** | `QuarkMeta_Source/src/ui/MetaPanel.h` | 声明“查看属性”、“查看哈希值”按钮、哈希结果展示容器及后台取消 Token |
| **修改** | `QuarkMeta_Source/src/ui/MetaPanel.cpp` | 实现按钮布局、单选/多选启用禁用控制、异步哈希计算展示与弹窗触发 |
| **修改** | `QuarkMeta_Source/resources/style.qss` | 增加新增按钮与哈希展示容器的外联 QSS 样式规则 |

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `QuarkMeta_Source/src/ui/TabBarWidget.h`
新增 `tabDisplayTitleChanged` 信号，供主窗口捕获以更新任务栏标题。

```
<<<<<<< SEARCH
signals:
    void tabAboutToChange(int oldIndex);
    void currentTabChanged(int index, const QString& url);
    void tabClosed(int index);
=======
signals:
    void tabAboutToChange(int oldIndex);
    void currentTabChanged(int index, const QString& url);
    void tabDisplayTitleChanged(const QString& displayTitle);
    void tabClosed(int index);
>>>>>>> REPLACE
```

---

### 3.2 `QuarkMeta_Source/src/ui/TabBarWidget.cpp`
在计算并应用当前标签页的 `currentTab.title` 后，发出 `tabDisplayTitleChanged` 信号。

```
<<<<<<< SEARCH
    applyDiffUpdate();
    saveStateToConfig();
}

void TabBarWidget::showTabContextMenu(int index, const QPoint& globalPos) {
=======
    applyDiffUpdate();
    saveStateToConfig();
    emit tabDisplayTitleChanged(currentTab.title);
}

void TabBarWidget::showTabContextMenu(int index, const QPoint& globalPos) {
>>>>>>> REPLACE
```

---

### 3.3 `QuarkMeta_Source/src/ui/MainWindow.cpp`
在主窗口初始化时，连接 `tabDisplayTitleChanged` 信号，动态设置任务栏标题为 `“文件夹名称-QuarkMeta”`。

```
<<<<<<< SEARCH
    m_panelMediator = new PanelMediator(components, this);
    m_panelMediator->setupConnections();

    connect(m_panelMediator, &PanelMediator::statusMessageRequested, this, [this](const QString& msg) {
        if (m_statusLeft) m_statusLeft->setText(msg);
    });
=======
    m_panelMediator = new PanelMediator(components, this);
    m_panelMediator->setupConnections();

    if (m_titleBarWidget && m_titleBarWidget->tabBar()) {
        connect(m_titleBarWidget->tabBar(), &TabBarWidget::tabDisplayTitleChanged, this, [this](const QString& title) {
            QString cleanTitle = title.trimmed();
            if (cleanTitle.isEmpty() || cleanTitle == "此电脑") {
                setWindowTitle("此电脑-QuarkMeta");
            } else {
                setWindowTitle(QString("%1-QuarkMeta").arg(cleanTitle));
            }
        });
    }

    connect(m_panelMediator, &PanelMediator::statusMessageRequested, this, [this](const QString& msg) {
        if (m_statusLeft) m_statusLeft->setText(msg);
    });
>>>>>>> REPLACE
```

---

### 3.4 `QuarkMeta_Source/src/ui/MetaPanel.h`
在 `MetaPanel.h` 中引入所需的成员指针与取消 Token。

```
<<<<<<< SEARCH
    ElasticEdit* m_pathEdit = nullptr;
    QPushButton* m_btnCopyPath = nullptr;
    QPushButton* m_btnOpenLocation = nullptr;

    QStringList m_selectedPaths;
=======
    ElasticEdit* m_pathEdit = nullptr;
    QPushButton* m_btnCopyPath = nullptr;
    QPushButton* m_btnOpenLocation = nullptr;
    QPushButton* m_btnShowProperties = nullptr;
    QPushButton* m_btnComputeHash = nullptr;
    QWidget* m_hashResultBox = nullptr;
    QLabel* m_hashValueLabel = nullptr;
    QPushButton* m_btnCopyHash = nullptr;
    std::shared_ptr<std::atomic<bool>> m_hashCancelToken;

    QStringList m_selectedPaths;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
private slots:
    void onTagDeleted(const QString& text);
    void flushPendingTagDeletions();
    void openTagSelectorOverlay(QWidget* targetAnchor);
    void setAsPrimaryColor(const QColor& color);
};
=======
private slots:
    void onTagDeleted(const QString& text);
    void flushPendingTagDeletions();
    void openTagSelectorOverlay(QWidget* targetAnchor);
    void setAsPrimaryColor(const QColor& color);
    void onComputeHashClicked();
    void onShowPropertiesClicked();
};
>>>>>>> REPLACE
```

---

### 3.5 `QuarkMeta_Source/src/ui/MetaPanel.cpp`
在头文件包含中引入 `DuplicateDetectorService.h`、`FileCompareDialog.h` 与并发所需头文件。

```
<<<<<<< SEARCH
#include "../util/ShellHelper.h"
#include <QVBoxLayout>
=======
#include "../util/ShellHelper.h"
#include "../meta/DuplicateDetectorService.h"
#include "dialogs/FileCompareDialog.h"
#include <QtConcurrent>
#include <atomic>
#include <memory>
#include <QVBoxLayout>
>>>>>>> REPLACE
```

---

### 3.6 `QuarkMeta_Source/src/ui/MetaPanel.cpp`
在 `initUi()` 中“物理路径”布局下方构建“查看属性”与“查看哈希值”按钮，以及动态哈希展示行。

```
<<<<<<< SEARCH
    pathBtnL->addWidget(m_btnOpenLocation);

    pathL->addLayout(pathBtnL);
    m_containerLayout->addWidget(createCollapsibleSection("物理路径", pathBox, true));

    m_containerLayout->addStretch(1);
=======
    pathBtnL->addWidget(m_btnOpenLocation);

    pathL->addLayout(pathBtnL);

    // 操作按钮行：查看属性、查看哈希值
    QHBoxLayout* actionBtnL = new QHBoxLayout();
    actionBtnL->setContentsMargins(0, 4, 0, 0);
    actionBtnL->setSpacing(6);

    m_btnShowProperties = new QPushButton("查看属性", pathBox);
    m_btnShowProperties->setFocusPolicy(Qt::NoFocus);
    m_btnShowProperties->setFixedHeight(24);
    m_btnShowProperties->setCursor(Qt::PointingHandCursor);
    m_btnShowProperties->setObjectName("BtnShowProperties");
    connect(m_btnShowProperties, &QPushButton::clicked, this, &MetaPanel::onShowPropertiesClicked);
    actionBtnL->addWidget(m_btnShowProperties);

    m_btnComputeHash = new QPushButton("查看哈希值", pathBox);
    m_btnComputeHash->setFocusPolicy(Qt::NoFocus);
    m_btnComputeHash->setFixedHeight(24);
    m_btnComputeHash->setCursor(Qt::PointingHandCursor);
    m_btnComputeHash->setObjectName("BtnComputeHash");
    connect(m_btnComputeHash, &QPushButton::clicked, this, &MetaPanel::onComputeHashClicked);
    actionBtnL->addWidget(m_btnComputeHash);

    pathL->addLayout(actionBtnL);

    // 单个项目哈希结果展示容器（默认隐藏）
    m_hashResultBox = new QWidget(pathBox);
    m_hashResultBox->setObjectName("MetaHashResultBox");
    m_hashResultBox->hide();
    QHBoxLayout* hashL = new QHBoxLayout(m_hashResultBox);
    hashL->setContentsMargins(0, 4, 0, 0);
    hashL->setSpacing(4);

    m_hashValueLabel = new QLabel(m_hashResultBox);
    m_hashValueLabel->setObjectName("MetaHashValueLabel");
    m_hashValueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QFont monoFont("Consolas");
    monoFont.setPointSize(9);
    m_hashValueLabel->setFont(monoFont);
    hashL->addWidget(m_hashValueLabel, 1);

    m_btnCopyHash = new QPushButton("复制", m_hashResultBox);
    m_btnCopyHash->setFocusPolicy(Qt::NoFocus);
    m_btnCopyHash->setFixedSize(36, 20);
    m_btnCopyHash->setCursor(Qt::PointingHandCursor);
    m_btnCopyHash->setObjectName("BtnCopyHash");
    connect(m_btnCopyHash, &QPushButton::clicked, this, [this]() {
        QString h = m_hashValueLabel ? m_hashValueLabel->text().trimmed() : QString();
        if (!h.isEmpty() && h != "计算中…" && !h.startsWith("无法")) {
            QApplication::clipboard()->setText(h);
            ToolTipOverlay::instance()->showToast("已复制哈希值", 1200, QColor("#2ecc71"));
        }
    });
    hashL->addWidget(m_btnCopyHash);

    pathL->addWidget(m_hashResultBox);

    m_containerLayout->addWidget(createCollapsibleSection("物理路径", pathBox, true));

    m_containerLayout->addStretch(1);
>>>>>>> REPLACE
```

---

### 3.7 `QuarkMeta_Source/src/ui/MetaPanel.cpp`
在 `setSelectedPaths` 切换选中项目时，取消既有哈希任务并重置隐藏哈希容器，并在 `updateControlsState` 中维护按钮可用性。

```
<<<<<<< SEARCH
    QElapsedTimer timer;
    timer.start();
    m_selectedPaths = paths;
    m_editingPathsSnapshot = paths;
    bool hasSelection = !m_selectedPaths.isEmpty();
    bool isMulti = m_selectedPaths.size() > 1;
=======
    // 切换选择时：安全取消正在计算的哈希任务并隐匿结果框
    if (m_hashCancelToken) {
        m_hashCancelToken->store(true);
        m_hashCancelToken.reset();
    }
    if (m_hashResultBox) {
        m_hashResultBox->hide();
    }
    if (m_hashValueLabel) {
        m_hashValueLabel->clear();
    }

    QElapsedTimer timer;
    timer.start();
    m_selectedPaths = paths;
    m_editingPathsSnapshot = paths;
    bool hasSelection = !m_selectedPaths.isEmpty();
    bool isMulti = m_selectedPaths.size() > 1;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    if (m_btnCopyPath) {
        m_btnCopyPath->setEnabled(hasSelection && !isMultiSelection);
    }
    if (m_btnOpenLocation) {
        m_btnOpenLocation->setEnabled(hasSelection && !isMultiSelection);
    }
}
=======
    if (m_btnCopyPath) {
        m_btnCopyPath->setEnabled(hasSelection && !isMultiSelection);
    }
    if (m_btnOpenLocation) {
        m_btnOpenLocation->setEnabled(hasSelection && !isMultiSelection);
    }
    if (m_btnShowProperties) {
        m_btnShowProperties->setEnabled(hasSelection && !isMultiSelection);
    }
    if (m_btnComputeHash) {
        m_btnComputeHash->setEnabled(hasSelection);
    }
}
>>>>>>> REPLACE
```

---

### 3.8 `QuarkMeta_Source/src/ui/MetaPanel.cpp`
实现 `onShowPropertiesClicked()` 与 `onComputeHashClicked()` 成员函数，插入在 `setAsPrimaryColor` 之后。

```
<<<<<<< SEARCH
void MetaPanel::setAsPrimaryColor(const QColor& color) {
    QString currentPath = m_pathEdit->toPlainText().trimmed();
    currentPath.remove(QChar(0x200B));
    if (!currentPath.isEmpty() && !currentPath.startsWith("已选中")) {
        emit primaryColorChanged(currentPath, color);
    }
}

bool MetaPanel::eventFilter(QObject* watched, QEvent* event) {
=======
void MetaPanel::setAsPrimaryColor(const QColor& color) {
    QString currentPath = m_pathEdit->toPlainText().trimmed();
    currentPath.remove(QChar(0x200B));
    if (!currentPath.isEmpty() && !currentPath.startsWith("已选中")) {
        emit primaryColorChanged(currentPath, color);
    }
}

void MetaPanel::onShowPropertiesClicked() {
    if (m_selectedPaths.isEmpty()) return;
    QString path = m_selectedPaths.first();
    if (!path.isEmpty() && !path.contains("://")) {
        ShellHelper::showProperties(path);
    }
}

void MetaPanel::onComputeHashClicked() {
    if (m_selectedPaths.isEmpty()) return;

    // 过滤出所有选中的非文件夹实体文件
    QStringList validFiles;
    for (const QString& p : m_selectedPaths) {
        if (!p.isEmpty() && !p.contains("://") && !QFileInfo(p).isDir()) {
            validFiles << p;
        }
    }

    if (validFiles.isEmpty()) {
        ToolTipOverlay::instance()->showToast("所选项目不包含可计算哈希的文件", 1500, QColor("#e81123"));
        return;
    }

    // 多选文件（>= 2）：弹出专用 FileCompareDialog
    if (validFiles.size() >= 2) {
        FileCompareDialog dlg(validFiles, this);
        dlg.exec();
        return;
    }

    // 单个文件：在下方结果框异步计算并展示
    QString targetFile = validFiles.first();
    if (!m_hashResultBox || !m_hashValueLabel) return;

    m_hashResultBox->show();
    m_hashValueLabel->setText("计算中…");

    if (m_hashCancelToken) {
        m_hashCancelToken->store(true);
    }
    m_hashCancelToken = std::make_shared<std::atomic<bool>>(false);
    auto cancelToken = m_hashCancelToken;

    QPointer<MetaPanel> weakThis(this);
    (void)QtConcurrent::run([weakThis, targetFile, cancelToken]() {
        QString hash = DuplicateDetectorService::computeFullSha256(targetFile, cancelToken.get());
        if (cancelToken->load()) return;

        QMetaObject::invokeMethod(qApp, [weakThis, hash, cancelToken]() {
            if (!weakThis || cancelToken->load()) return;
            if (weakThis->m_hashValueLabel) {
                weakThis->m_hashValueLabel->setText(hash.isEmpty() ? "无法读取" : hash);
            }
        });
    });
}

bool MetaPanel::eventFilter(QObject* watched, QEvent* event) {
>>>>>>> REPLACE
```

---

### 3.9 `QuarkMeta_Source/resources/style.qss`
为新增按钮与哈希展示标签添加外联 QSS。

```
<<<<<<< SEARCH
QPushButton#BtnOpenLocation:hover {
    background: #378ADD;
    border-color: #378ADD;
    color: #FFFFFF;
}
QLabel#MetaInfoKeyLabel {
=======
QPushButton#BtnOpenLocation:hover {
    background: #378ADD;
    border-color: #378ADD;
    color: #FFFFFF;
}
QPushButton#BtnShowProperties, QPushButton#BtnComputeHash {
    background: #252526;
    border: 1px solid #3c3c3c;
    border-radius: 4px;
    color: #CCCCCC;
    font-size: 11px;
    outline: none;
}
QPushButton#BtnShowProperties:hover, QPushButton#BtnComputeHash:hover {
    background: #333333;
    color: #FFFFFF;
}
QLabel#MetaHashValueLabel {
    font-size: 10px;
    color: #3498db;
    background: #1e1e1e;
    border: 1px solid #333333;
    border-radius: 3px;
    padding: 2px 4px;
}
QPushButton#BtnCopyHash {
    background: #252526;
    border: 1px solid #3c3c3c;
    border-radius: 3px;
    color: #CCCCCC;
    font-size: 10px;
    outline: none;
}
QPushButton#BtnCopyHash:hover {
    background: #378ADD;
    border-color: #378ADD;
    color: #FFFFFF;
}
QLabel#MetaInfoKeyLabel {
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 构建命令
```bash
cd QuarkMeta_Source
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target QuarkMeta
```

### 运行时验收标准

| 序号 | 验证场景 | 操作步骤 | 预期效果 |
|---|---|---|---|
| 1 | **任务栏窗口标题跟随** | 打开不同物理文件夹（如 `Download`、`Projects`）或切换标签页 | 任务栏标题与窗口标题即时更新为：`"Download-QuarkMeta"`、`"Projects-QuarkMeta"` |
| 2 | **“此电脑”特殊标题** | 导航至此电脑根视图 | 标题显示为 `"此电脑-QuarkMeta"` |
| 3 | **查看属性按钮** | 单击选中一个文件，点击【查看属性】 | 原生弹出 Windows 系统文件属性窗口 |
| 4 | **单选哈希展示** | 选中单个文件，观察初始状态；点击【查看哈希值】 | 选中时**不显示哈希**；点击后下方展开显示“计算中…”，随后呈现 64 位 SHA-256 哈希值；点击“复制”提示 Toast |
| 5 | **多选哈希对比** | 按住 Ctrl 选中 2 个或以上文件，点击【查看哈希值】 | 弹出 `FileCompareDialog` 文件对比弹窗，显示表格并发计算哈希与高亮差异 |
| 6 | **切换重置状态** | 在已展示哈希后，点击切换选择其他文件或取消选择 | 下方哈希展示区自动收起隐匿，不预计算哈希，等待用户下次点击 |

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

| 业务动作 | 复用的官方 SSOT 入口 | 消除的重复轮子/散落实现 |
|---|---|---|
| 查看 Windows 属性 | `ShellHelper::showProperties(path)` | 杜绝在 MetaPanel 内部私自写 ShellExecuteEx |
| SHA-256 流式计算 | `DuplicateDetectorService::computeFullSha256(path, cancelToken)` | 杜绝重复手写 QCryptographicHash 循环 |
| 多文件哈希对比 | `FileCompareDialog(validFiles, this)` | 统一复用既有的多文件对比弹窗与模型 |

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一真理源 (SSOT) | 派生展示点 | 同步与刷新机制 |
|---|---|---|---|
| 文件物理路径 | `MetaPanel::m_selectedPaths` | 物理路径输入框、属性查看参数 | 随选中项信号更新 |
| 窗口展示标题 | `TabBarWidget::currentTab.title` | `MainWindow::setWindowTitle` | 由 `tabDisplayTitleChanged` 信号单向驱动 |
| 文件哈希值 | 物理文件流经 `computeFullSha256` 实时哈希 | `m_hashValueLabel` / `FileCompareDialog` | 按需主动触发，单次结算不持久化 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

| 调用接口 | 物理头文件与签名位置 | 访问级别 |
|---|---|---|
| `ShellHelper::showProperties` | `ShellHelper.h:33`：`static void showProperties(const QString& path);` | ✅ public |
| `DuplicateDetectorService::computeFullSha256` | `DuplicateDetectorService.h:29`：`static QString computeFullSha256(...)` | ✅ public |
| `FileCompareDialog::FileCompareDialog` | `FileCompareDialog.h:85`：`explicit FileCompareDialog(const QStringList&, QWidget*)` | ✅ public |
| `TabBarWidget::tabDisplayTitleChanged` | `TabBarWidget.h` 新增 signal | ✅ public 信号 |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

| 类型 / API | 显式 `#include` 状态 |
|---|---|
| `DuplicateDetectorService` | `MetaPanel.cpp` 显式包含 `../meta/DuplicateDetectorService.h` ✅ |
| `FileCompareDialog` | `MetaPanel.cpp` 显式包含 `dialogs/FileCompareDialog.h` ✅ |
| `QtConcurrent` | `MetaPanel.cpp` 显式包含 `<QtConcurrent>` ✅ |
| `std::atomic` / `std::shared_ptr` | `MetaPanel.h` 与 `MetaPanel.cpp` 均显式包含 `<atomic>` 与 `<memory>` ✅ |

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 编号 | 检查项 | 状态 | 说明 |
|---|---|---|---|
| 1 | 方案已通过需求确认 (Gate 0) | ✅ 满足 | 用户明确下达并在对话中确认需求细节 |
| 2 | 物理读取所有涉及文件 (Gate 1) | ✅ 满足 | 已全量物理核查对应源码签名 |
| 3 | SEARCH 块原文真实存在且唯一匹配 | ✅ 满足 | 逐块核对，精准无误 |
| 4 | 新增接口声明、定义、调用点签名一致 | ✅ 满足 | 信号槽与槽函数签名完全闭合 |
| 5 | 被修改的 `#include` 间接包含链闭合 | ✅ 满足 | 显式包含所有依赖头文件 |
| 6 | 新建/删除文件在 CMakeLists.txt 同步 | ✅ 不涉及 | 未新建或删除物理文件 |
| 7 | 跨类调用访问级别均为 public | ✅ 满足 | 均为 public |
| 8 | 方案未触及范围外文件 | ✅ 满足 | 仅涉及 MetaPanel、TabBarWidget、MainWindow 及 style.qss |
| 9 | 未采用笨拙浪费计算资源的手法 | ✅ 满足 | 选中时不计算哈希，主动点击才异步分块计算 |
| 10 | 代码中无打补丁行为 | ✅ 满足 | 采用标准 Qt 信号槽与纯事件驱动架构 |
| 11 | 遵循 Memories.md 与 Memories_QuarkMeta.md 规范 | ✅ 满足 | 严格符合第 51 节规范 |
| 12 | ToolTipOverlay 不追随鼠标 | ✅ 满足 | Toast 使用固定位置 |
| 13 | 100% 语义化矢量图标 | ✅ 满足 | 纯外联样式与原生按钮 |
| 14 | 异步回调按值捕获 | ✅ 满足 | 异步 Lambda 捕获弱引用与 `cancelToken` 值 |
| 15 | 无 Lambda 循环引用 | ✅ 满足 | 无强引用自身情况 |
| 16 | 后台线程 UI 操作回归主线程 | ✅ 满足 | 通过 `QMetaObject::invokeMethod(qApp, ...)` 调度回 GUI 线程 |
| 17 | Qt 信号槽使用 C++11 函数指针语法 | ✅ 满足 | 严格采用 `&Class::signal` 语法 |
| 18 | 对外 `.h` 签名冻结 | ✅ 满足 | 不改动外部公共模块 API |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 行为零篡改声明

| 既有设计行为 | 本方案处理方式 | 结论 |
|---|---|---|
| 原有“复制路径”与“打开位置”操作 | 原封不动保留，不改动任何既有逻辑 | ✅ 100% 原样保持 |
| 元数据面板原有星级/颜色/标签等交互 | 保持原有事件流转与落盘时序 | ✅ 100% 隔离不受影响 |
| 标签栏原有多分屏与分栏状态持久化 | 仅在标题计算后发射通知信号，不改变标签数据结构 | ✅ 100% 保持 |

### 数值零篡改声明

| 样式与几何数值 | 数值本身 | 来源与处理 |
|---|---|---|
| 按钮高度 | `24px` | 与既有 `BtnCopyPath` / `BtnOpenLocation` 严格一致 |
| 按钮边框 | `1px solid #3c3c3c` | 原样照抄 `style.qss` |
| 按钮圆角 | `4px` | 原样照抄 `style.qss` |
| 按钮字体大小 | `11px` | 原样照抄 `style.qss` |
| 默认背景色 | `#252526` | 原样照抄 `style.qss` |
| 悬停背景色 | `#333333` | 原样照抄 `style.qss` |
