# Implementation Plan - BatchRenameLayoutAndPresetRefactor

Base Commit Hash: `943037f85524b65875859191f2d3c1dba8aae1c1`
Prerequisite Plans: 无

---

### 已读物理证据清单 (Gate 1 Evidence)
1. **物理读取文件列表**：
   - `src/ui/BatchRenameDialog.h`
   - `src/ui/BatchRenameDialog.cpp`
   - `src/ui/FramelessDialogBase.h`
   - `src/ui/FramelessDialog.h`
   - `src/ui/FramelessDialog.cpp`
   - `src/ui/PresetManager.h`
   - `src/ui/PresetManager.cpp`
   - `src/ui/dialogs/FramelessInputDialog.h`
   - `resources/style.qss`
2. **头文件原样签名**：
   - `src/ui/BatchRenameDialog.h:17`: `class BatchRenameDialog : public FramelessDialog`
   - `src/ui/FramelessDialogBase.h:21`: `void setVisibleButtons(int flags);`
   - `src/ui/dialogs/FramelessInputDialog.h:10`: `explicit FramelessInputDialog(const QString& title, const QString& label, const QString& initial = "", QWidget* parent = nullptr);`
3. **既有 API 检索日志**：
   - `grep -rn "BatchRenameDialog" src/`
   - `grep -rn "FramelessInputDialog" src/ui/`

---

## Chapter 1: Overview
本实施方案旨在全面修复与升级 QuarkMeta 中“批量重命名对话框”（`BatchRenameDialog`）的响应式 UI 布局与预设管理交互体系，彻底消除窗口最大化时的死板硬编码局限与按钮逻辑冗余：

1. **标题栏右上角控制按钮重构（标注③）**：
   - 在 `BatchRenameDialog` 中隐藏置顶（Pin）、最小化（Min）、最大化（Max）按钮，仅保留关闭（Close）按钮；
   - 将“导入...”与“导出...”两个次要功能按钮由左侧配置区平移放置到标题栏右上角（紧贴关闭 `×` 按钮左侧，呈 `[ 导入... ] [ 导出... ] [ × ]` 排列）。
2. **预设控制区“储存/删除”及多预设持久化（标注④）**：
   - 将预设组中的操作按钮修改为 **“储存”** （`m_btnSavePreset`）与 **“删除”** （`m_btnDeletePreset`）；
   - 点击 **“储存”**：唤起 `FramelessInputDialog` 输入框提示用户输入预设名称，序列化当前规则列表并持久化保存至 `AppConfig`（`BatchRenamePresetsMap`）中，自动追加并选中新预设；
   - 点击 **“删除”**：从 `AppConfig` 与下拉框中移除当前选中的自定义预设，并平滑切换回“默认设置”；
   - 切换预设下拉框：自动加载并展现对应预设的规则行列表。
3. **最大化与拉伸全屏响应式自适应布局**：
   - 引入 `QSplitter` 替代固定横向布局，彻底解除 `leftPanel->setMaximumWidth(500)` 与 `m_table->setFixedWidth(700)` 的硬编码限制；
   - 设定左侧配置面板自适应范围（最小 380px，最大 480px，推荐初始 420px），右侧预览表格 `m_table` 设置 `QSizePolicy::Expanding` 与 `setStretchFactor(1, 1)`，配合 `QHeaderView::Stretch`；
   - 设置对话框最小尺寸 `900 x 500`。窗口在最大化或大分辨率下，右侧预览表格自动拉伸填充所有剩余宽度，两列路径全屏铺满显示，杜绝任何空白黑框。

---

## Chapter 2: Modified Files List
- `src/ui/BatchRenameDialog.h`
- `src/ui/BatchRenameDialog.cpp`

---

## Chapter 3: Detailed Line-by-Line Changes

### 1. `src/ui/BatchRenameDialog.h`
<<<<<<< SEARCH
private slots:
    void onAddRow(RuleRow* targetRow = nullptr);
    void updatePreview();
    void onExecute();
    void onBrowseTarget();
    void onImportPreset();
    void onExportPreset();
    void onDeleteCurrentPreset();
    void scheduleAutoSave();
    void doAutoSave();

private:
    void initContent();
    void applyTheme();
    void initTableItems();

    std::vector<std::wstring> m_originalPaths;
    QList<RuleRow*> m_ruleRows;
    QString m_firstNewName;

    QTableWidget* m_table = nullptr;
    QComboBox* m_presetCombo = nullptr;
    QPushButton* m_btnDeletePreset = nullptr;
    QPushButton* m_btnImportPreset = nullptr;
    QPushButton* m_btnExportPreset = nullptr;
=======
private slots:
    void onAddRow(RuleRow* targetRow = nullptr);
    void updatePreview();
    void onExecute();
    void onBrowseTarget();
    void onImportPreset();
    void onExportPreset();
    void onSavePreset();
    void onDeleteCurrentPreset();
    void onPresetComboChanged(int index);
    void scheduleAutoSave();
    void doAutoSave();

private:
    void initContent();
    void applyTheme();
    void initTableItems();
    void loadPresetsFromConfig();
    void applyRulesJson(const QString& jsonStr);

    std::vector<std::wstring> m_originalPaths;
    QList<RuleRow*> m_ruleRows;
    QString m_firstNewName;

    QTableWidget* m_table = nullptr;
    QComboBox* m_presetCombo = nullptr;
    QPushButton* m_btnSavePreset = nullptr;
    QPushButton* m_btnDeletePreset = nullptr;
    QPushButton* m_btnImportPreset = nullptr;
    QPushButton* m_btnExportPreset = nullptr;
>>>>>>> REPLACE

---

### 2. `src/ui/BatchRenameDialog.cpp`
<<<<<<< SEARCH
#include "FramelessFileDialog.h"
#include "FramelessDialog.h"
#include "ToolTipOverlay.h"
=======
#include "FramelessFileDialog.h"
#include "FramelessDialog.h"
#include "dialogs/FramelessInputDialog.h"
#include "ToolTipOverlay.h"
#include <QSplitter>
>>>>>>> REPLACE

<<<<<<< SEARCH
BatchRenameDialog::BatchRenameDialog(const std::vector<std::wstring>& originalPaths, QWidget* parent)
    : FramelessDialog("批量重命名 - QuarkMeta", parent), m_originalPaths(originalPaths) {
    resize(1220, 650); // 调整窗口尺寸为横向 1220×650 像素（左侧最大 500px，右侧 700px）
    initContent();
    applyTheme();

    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setSingleShot(true);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &BatchRenameDialog::doAutoSave);

    initTableItems(); // 仅在初始化时加载 1 次左侧原始文件名和图标！

    m_isInitializing = true;
    QString lastRules = AppConfig::instance().getValue("LastBatchRenameRules").toString();
    if (!lastRules.isEmpty()) {
        auto rules = PresetManager::deserializeRules(lastRules);
        for (const auto& rule : rules) {
            onAddRow();
            m_ruleRows.last()->setRule(rule);
        }
    }

    if (m_ruleRows.isEmpty()) {
        onAddRow();
    }
    m_isInitializing = false;

    updatePreview(); // 全局仅在此时执行 1 次预览刷新！
}
=======
BatchRenameDialog::BatchRenameDialog(const std::vector<std::wstring>& originalPaths, QWidget* parent)
    : FramelessDialog("批量重命名 - QuarkMeta", parent), m_originalPaths(originalPaths) {
    setMinimumSize(900, 500);
    resize(1220, 650);
    initContent();
    applyTheme();

    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setSingleShot(true);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &BatchRenameDialog::doAutoSave);

    initTableItems();

    m_isInitializing = true;
    loadPresetsFromConfig();

    QString lastRules = AppConfig::instance().getValue("LastBatchRenameRules").toString();
    if (!lastRules.isEmpty()) {
        applyRulesJson(lastRules);
    }

    if (m_ruleRows.isEmpty()) {
        onAddRow();
    }
    m_isInitializing = false;

    updatePreview();
}
>>>>>>> REPLACE

<<<<<<< SEARCH
void BatchRenameDialog::initContent() {
    QHBoxLayout* rootL = new QHBoxLayout(m_contentArea);
    rootL->setContentsMargins(20, 20, 20, 20);
    rootL->setSpacing(20);

    // ================= 左侧：配置区 (最大 500px) =================
    QWidget* leftPanel = new QWidget(this);
    leftPanel->setMaximumWidth(500);
    QVBoxLayout* configL = new QVBoxLayout(leftPanel);
    configL->setContentsMargins(0, 0, 0, 0);
    configL->setSpacing(15);

    // 1. 预设区
    QGroupBox* presetGroup = new QGroupBox("预设", leftPanel);
    QHBoxLayout* presetL = new QHBoxLayout(presetGroup);
    presetL->setContentsMargins(10, 5, 10, 5);
    presetL->setSpacing(5);

    m_presetCombo = new QComboBox(presetGroup);
    m_presetCombo->addItem("默认设置");
    m_presetCombo->setFixedHeight(25);
    presetL->addWidget(m_presetCombo, 1);

    // 删除预设按钮 "×"
    m_btnDeletePreset = new QPushButton("×", presetGroup);
    m_btnDeletePreset->setFixedSize(20, 20);
    m_btnDeletePreset->setCursor(Qt::PointingHandCursor);
    m_btnDeletePreset->setObjectName("BatchRenameDeletePresetBtn");
    presetL->addWidget(m_btnDeletePreset);

    // 导入/导出按钮
    m_btnImportPreset = new QPushButton("导入...", presetGroup);
    m_btnExportPreset = new QPushButton("导出...", presetGroup);
    m_btnImportPreset->setObjectName("BatchRenameSecBtn");
    m_btnExportPreset->setObjectName("BatchRenameSecBtn");
    m_btnImportPreset->setFixedHeight(25);
    m_btnExportPreset->setFixedHeight(25);
    m_btnImportPreset->setFixedWidth(80);
    m_btnExportPreset->setFixedWidth(80);

    presetL->addWidget(m_btnImportPreset);
    presetL->addWidget(m_btnExportPreset);
    configL->addWidget(presetGroup);

    // 2. 目标文件夹
    QGroupBox* targetGroup = new QGroupBox("目标文件夹", leftPanel);
    QVBoxLayout* targetL = new QVBoxLayout(targetGroup);
    m_rbRename = new QRadioButton("在同一文件夹中重命名", targetGroup);
    m_rbMove = new QRadioButton("移动到其他文件夹", targetGroup);
    m_rbCopy = new QRadioButton("复制到其他文件夹", targetGroup);
    m_rbRename->setChecked(true);
    targetL->addWidget(m_rbRename);
    targetL->addWidget(m_rbMove);
    targetL->addWidget(m_rbCopy);

    QHBoxLayout* pathL = new QHBoxLayout();
    m_targetPathEdit = new QLineEdit(targetGroup);
    m_targetPathEdit->setPlaceholderText("选择目标文件夹...");
    m_targetPathEdit->setFixedHeight(25);
    m_targetPathEdit->setEnabled(false);
    m_btnBrowse = new QPushButton("浏览...", targetGroup);
    m_btnBrowse->setObjectName("BatchRenameSecBtn");
    m_btnBrowse->setFixedSize(80, 25);
    m_btnBrowse->setEnabled(false);
    pathL->addWidget(m_targetPathEdit);
    pathL->addWidget(m_btnBrowse);
    targetL->addLayout(pathL);

    configL->addWidget(targetGroup);

    // 3. 新文件名 (规则构造器)
    QGroupBox* rulesGroup = new QGroupBox("新文件名", leftPanel);
    rulesGroup->setObjectName("BatchRenameRulesGroup");

    QVBoxLayout* rulesGroupL = new QVBoxLayout(rulesGroup);
    rulesGroupL->setContentsMargins(4, 2, 4, 4);
    rulesGroupL->setSpacing(0);

    QScrollArea* scroll = new QScrollArea(rulesGroup);
    scroll->setWidgetResizable(true);
    scroll->setAlignment(Qt::AlignTop);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setObjectName("BatchRenameScroll");

    m_rulesContainer = new QWidget(scroll);
    m_rulesContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    m_rulesLayout = new QVBoxLayout(m_rulesContainer);
    m_rulesLayout->setContentsMargins(0, 0, 0, 0);
    m_rulesLayout->setSpacing(2);

    scroll->setWidget(m_rulesContainer);
    rulesGroupL->addWidget(scroll);
    configL->addWidget(rulesGroup, 1);

    // 4. 左侧底部主执行按钮 (应用标准 Primary Blue #378ADD 高亮样式)
    QHBoxLayout* execBtnLayout = new QHBoxLayout();
    m_btnExecute = new QPushButton("重命名", leftPanel);
    m_btnExecute->setFixedSize(120, 36);
    m_btnExecute->setCursor(Qt::PointingHandCursor);
    m_btnExecute->setObjectName("BatchRenameExecuteBtn");
    execBtnLayout->addStretch();
    execBtnLayout->addWidget(m_btnExecute);
    execBtnLayout->addStretch();

    configL->addLayout(execBtnLayout);

    rootL->addWidget(leftPanel, 0);

    // ================= 右侧：实时预览面板 (700px 宽度) =================
    m_table = new QTableWidget(this);
    m_table->setFixedWidth(700);
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({"当前文件名", "重命名后名称"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(28); // 行高限制不超过 30 像素 (设定为 28px)
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true); // 开启斑马纹
    m_table->setShowGrid(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setIconSize(QSize(20, 20)); // 最左侧微型缩略图/图标尺寸
    m_table->setFocusPolicy(Qt::NoFocus); // 彻底消除选中单元格四周的虚线焦点框

    rootL->addWidget(m_table, 0);

    // Connections
    connect(m_rbRename, &QRadioButton::toggled, [this](bool checked){
        m_targetPathEdit->setEnabled(!checked);
        m_btnBrowse->setEnabled(!checked);
    });
    connect(m_btnBrowse, &QPushButton::clicked, this, &BatchRenameDialog::onBrowseTarget);
    connect(m_btnExecute, &QPushButton::clicked, this, &BatchRenameDialog::onExecute);
    connect(m_btnImportPreset, &QPushButton::clicked, this, &BatchRenameDialog::onImportPreset);
    connect(m_btnExportPreset, &QPushButton::clicked, this, &BatchRenameDialog::onExportPreset);
    connect(m_btnDeletePreset, &QPushButton::clicked, this, &BatchRenameDialog::onDeleteCurrentPreset);
}
=======
void BatchRenameDialog::initContent() {
    // 隐藏 Pin, Min, Max，仅保留 Close 按钮
    setVisibleButtons(Close);

    // 导入/导出按钮嵌入标题栏 (标注③)
    m_btnImportPreset = new QPushButton("导入...", this);
    m_btnExportPreset = new QPushButton("导出...", this);
    m_btnImportPreset->setObjectName("BatchRenameSecBtn");
    m_btnExportPreset->setObjectName("BatchRenameSecBtn");
    m_btnImportPreset->setFixedSize(56, 22);
    m_btnExportPreset->setFixedSize(56, 22);
    m_btnImportPreset->setCursor(Qt::PointingHandCursor);
    m_btnExportPreset->setCursor(Qt::PointingHandCursor);

    if (m_titleLayout && m_closeBtn) {
        int closeIdx = m_titleLayout->indexOf(m_closeBtn);
        if (closeIdx >= 0) {
            m_titleLayout->insertWidget(closeIdx, m_btnImportPreset);
            m_titleLayout->insertWidget(closeIdx + 1, m_btnExportPreset);
        } else {
            m_titleLayout->addWidget(m_btnImportPreset);
            m_titleLayout->addWidget(m_btnExportPreset);
        }
    }

    QHBoxLayout* rootL = new QHBoxLayout(m_contentArea);
    rootL->setContentsMargins(15, 15, 15, 15);
    rootL->setSpacing(0);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, m_contentArea);
    splitter->setChildrenCollapsible(false);
    splitter->setObjectName("BatchRenameSplitter");

    // ================= 左侧：配置区 =================
    QWidget* leftPanel = new QWidget(splitter);
    leftPanel->setMinimumWidth(380);
    leftPanel->setMaximumWidth(480);
    QVBoxLayout* configL = new QVBoxLayout(leftPanel);
    configL->setContentsMargins(0, 0, 10, 0);
    configL->setSpacing(15);

    // 1. 预设区 (标注④: 替换为 储存 / 删除 两个按钮)
    QGroupBox* presetGroup = new QGroupBox("预设", leftPanel);
    QHBoxLayout* presetL = new QHBoxLayout(presetGroup);
    presetL->setContentsMargins(10, 5, 10, 5);
    presetL->setSpacing(5);

    m_presetCombo = new QComboBox(presetGroup);
    m_presetCombo->addItem("默认设置");
    m_presetCombo->setFixedHeight(25);
    presetL->addWidget(m_presetCombo, 1);

    m_btnSavePreset = new QPushButton("储存", presetGroup);
    m_btnSavePreset->setObjectName("BatchRenameSecBtn");
    m_btnSavePreset->setFixedHeight(25);
    m_btnSavePreset->setFixedWidth(56);
    m_btnSavePreset->setCursor(Qt::PointingHandCursor);

    m_btnDeletePreset = new QPushButton("删除", presetGroup);
    m_btnDeletePreset->setObjectName("BatchRenameSecBtn");
    m_btnDeletePreset->setFixedHeight(25);
    m_btnDeletePreset->setFixedWidth(56);
    m_btnDeletePreset->setCursor(Qt::PointingHandCursor);
    m_btnDeletePreset->setEnabled(false);

    presetL->addWidget(m_btnSavePreset);
    presetL->addWidget(m_btnDeletePreset);
    configL->addWidget(presetGroup);

    // 2. 目标文件夹
    QGroupBox* targetGroup = new QGroupBox("目标文件夹", leftPanel);
    QVBoxLayout* targetL = new QVBoxLayout(targetGroup);
    m_rbRename = new QRadioButton("在同一文件夹中重命名", targetGroup);
    m_rbMove = new QRadioButton("移动到其他文件夹", targetGroup);
    m_rbCopy = new QRadioButton("复制到其他文件夹", targetGroup);
    m_rbRename->setChecked(true);
    targetL->addWidget(m_rbRename);
    targetL->addWidget(m_rbMove);
    targetL->addWidget(m_rbCopy);

    QHBoxLayout* pathL = new QHBoxLayout();
    m_targetPathEdit = new QLineEdit(targetGroup);
    m_targetPathEdit->setPlaceholderText("选择目标文件夹...");
    m_targetPathEdit->setFixedHeight(25);
    m_targetPathEdit->setEnabled(false);
    m_btnBrowse = new QPushButton("浏览...", targetGroup);
    m_btnBrowse->setObjectName("BatchRenameSecBtn");
    m_btnBrowse->setFixedSize(80, 25);
    m_btnBrowse->setEnabled(false);
    pathL->addWidget(m_targetPathEdit);
    pathL->addWidget(m_btnBrowse);
    targetL->addLayout(pathL);

    configL->addWidget(targetGroup);

    // 3. 新文件名 (规则构造器)
    QGroupBox* rulesGroup = new QGroupBox("新文件名", leftPanel);
    rulesGroup->setObjectName("BatchRenameRulesGroup");

    QVBoxLayout* rulesGroupL = new QVBoxLayout(rulesGroup);
    rulesGroupL->setContentsMargins(4, 2, 4, 4);
    rulesGroupL->setSpacing(0);

    QScrollArea* scroll = new QScrollArea(rulesGroup);
    scroll->setWidgetResizable(true);
    scroll->setAlignment(Qt::AlignTop);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setObjectName("BatchRenameScroll");

    m_rulesContainer = new QWidget(scroll);
    m_rulesContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    m_rulesLayout = new QVBoxLayout(m_rulesContainer);
    m_rulesLayout->setContentsMargins(0, 0, 0, 0);
    m_rulesLayout->setSpacing(2);

    scroll->setWidget(m_rulesContainer);
    rulesGroupL->addWidget(scroll);
    configL->addWidget(rulesGroup, 1);

    // 4. 左侧底部主执行按钮
    QHBoxLayout* execBtnLayout = new QHBoxLayout();
    m_btnExecute = new QPushButton("重命名", leftPanel);
    m_btnExecute->setFixedSize(120, 36);
    m_btnExecute->setCursor(Qt::PointingHandCursor);
    m_btnExecute->setObjectName("BatchRenameExecuteBtn");
    execBtnLayout->addStretch();
    execBtnLayout->addWidget(m_btnExecute);
    execBtnLayout->addStretch();

    configL->addLayout(execBtnLayout);

    // ================= 右侧：实时预览面板 (自适应弹性充满) =================
    m_table = new QTableWidget(splitter);
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({"当前文件名", "重命名后名称"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(28);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setIconSize(QSize(20, 20));
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    splitter->addWidget(leftPanel);
    splitter->addWidget(m_table);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({420, 780});

    rootL->addWidget(splitter);

    // Connections
    connect(m_rbRename, &QRadioButton::toggled, [this](bool checked){
        m_targetPathEdit->setEnabled(!checked);
        m_btnBrowse->setEnabled(!checked);
    });
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BatchRenameDialog::onPresetComboChanged);
    connect(m_btnBrowse, &QPushButton::clicked, this, &BatchRenameDialog::onBrowseTarget);
    connect(m_btnExecute, &QPushButton::clicked, this, &BatchRenameDialog::onExecute);
    connect(m_btnImportPreset, &QPushButton::clicked, this, &BatchRenameDialog::onImportPreset);
    connect(m_btnExportPreset, &QPushButton::clicked, this, &BatchRenameDialog::onExportPreset);
    connect(m_btnSavePreset, &QPushButton::clicked, this, &BatchRenameDialog::onSavePreset);
    connect(m_btnDeletePreset, &QPushButton::clicked, this, &BatchRenameDialog::onDeleteCurrentPreset);
}
>>>>>>> REPLACE

<<<<<<< SEARCH
void BatchRenameDialog::onDeleteCurrentPreset() {
    if (m_presetCombo->count() > 0) {
        int index = m_presetCombo->currentIndex();
        m_presetCombo->removeItem(index);
    }
}
=======
void BatchRenameDialog::loadPresetsFromConfig() {
    QSignalBlocker blocker(m_presetCombo);
    m_presetCombo->clear();
    m_presetCombo->addItem("默认设置");

    QVariantMap presetsMap = AppConfig::instance().getValue("BatchRenamePresetsMap").toMap();
    for (auto it = presetsMap.constBegin(); it != presetsMap.constEnd(); ++it) {
        if (it.key() != "默认设置") {
            m_presetCombo->addItem(it.key());
        }
    }
}

void BatchRenameDialog::applyRulesJson(const QString& jsonStr) {
    while (!m_ruleRows.isEmpty()) {
        auto* row = m_ruleRows.takeAt(0);
        row->deleteLater();
    }

    auto rules = PresetManager::deserializeRules(jsonStr);
    for (const auto& rule : rules) {
        onAddRow();
        m_ruleRows.last()->setRule(rule);
    }
    if (m_ruleRows.isEmpty()) {
        onAddRow();
    }
    updatePreview();
}

void BatchRenameDialog::onPresetComboChanged(int index) {
    if (m_isInitializing) return;

    QString name = m_presetCombo->currentText();
    if (index <= 0 || name == "默认设置") {
        m_btnDeletePreset->setEnabled(false);
        QString lastRules = AppConfig::instance().getValue("LastBatchRenameRules").toString();
        if (!lastRules.isEmpty()) {
            applyRulesJson(lastRules);
        }
        return;
    }

    m_btnDeletePreset->setEnabled(true);
    QVariantMap presetsMap = AppConfig::instance().getValue("BatchRenamePresetsMap").toMap();
    if (presetsMap.contains(name)) {
        QString rulesJson = presetsMap.value(name).toString();
        applyRulesJson(rulesJson);
    }
}

void BatchRenameDialog::onSavePreset() {
    std::vector<RenameRule> rules;
    for (auto* row : m_ruleRows) {
        if (row) rules.push_back(row->getRule());
    }
    if (rules.empty()) return;

    QString defaultName = (m_presetCombo->currentIndex() > 0) ? m_presetCombo->currentText() : "";
    FramelessInputDialog dlg("保存重命名预设", "请输入预设名称:", defaultName, this);
    if (dlg.exec() == QDialog::Accepted) {
        QString name = dlg.text().trimmed();
        if (name.isEmpty() || name == "默认设置") return;

        QString compactJson = PresetManager::serializeRules(rules);
        QVariantMap map = AppConfig::instance().getValue("BatchRenamePresetsMap").toMap();
        map.insert(name, compactJson);
        AppConfig::instance().setValue("BatchRenamePresetsMap", map);

        loadPresetsFromConfig();
        int idx = m_presetCombo->findText(name);
        if (idx >= 0) {
            m_presetCombo->setCurrentIndex(idx);
        }
    }
}

void BatchRenameDialog::onDeleteCurrentPreset() {
    int index = m_presetCombo->currentIndex();
    QString name = m_presetCombo->currentText();
    if (index <= 0 || name == "默认设置") return;

    QVariantMap map = AppConfig::instance().getValue("BatchRenamePresetsMap").toMap();
    map.remove(name);
    AppConfig::instance().setValue("BatchRenamePresetsMap", map);

    loadPresetsFromConfig();
    m_presetCombo->setCurrentIndex(0);
}
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps
1. **构建步骤**：
   在 sandbox 根目录运行 `cmake --build build` 编译项目。
2. **运行时与 UI 效果核验**：
   - **标题栏（标注③）**：打开“批量重命名”窗口，观察右上角，确认为 `[ 导入... ] [ 导出... ] [ × ]`，无置顶、最小化、最大化冗余按钮；
   - **预设控制（标注④）**：预设下拉框右侧为 `[储存]` 与 `[删除]` 按钮。点击“储存”弹出对话框命名预设，保存后可在下拉框切换选择并持久化恢复；
   - **窗口最大化自适应**：点击全屏或拖拽放大大窗口，观察右侧 `m_table` 预览表格自动伸展填充空间，无任何留白或黑框，两列完整铺满；
   - **核心重命名功能**：添加/移除规则行，检查预览同步计算与真实批量重命名逻辑运行无误。

---

## Chapter 5: Official API Reuse & Anti-Redundancy
- **FramelessInputDialog**：直接复用项目中成熟的无边框输入框 `FramelessInputDialog` 接受用户预设名称输入，无需重复开发输入 UI 控件；
- **PresetManager & AppConfig**：复用 `PresetManager::serializeRules` / `deserializeRules` 及 `AppConfig::instance().getValue / setValue` 实现规则序列化与多预设持久化；
- **FramelessDialog::setVisibleButtons**：复用基类 `setVisibleButtons(Close)` 进行按钮可见性管控。

---

## Chapter 6: Header API Signature Verification
- `src/ui/BatchRenameDialog.h` 新增非公开私有槽函数 `onSavePreset()`、`onPresetComboChanged(int)` 与私有辅助方法 `loadPresetsFromConfig()`、`applyRulesJson()`。
- 保持外部 `public` 接口 `getFirstNewName()` 完全未修改，满足公开 API 签名冻结规范。

---

## Chapter 7: Header Inclusion Chain & Type Completeness Check
- 在 `src/ui/BatchRenameDialog.cpp` 中新增 `#include "dialogs/FramelessInputDialog.h"` 和 `#include <QSplitter>`，所有类型说明与完整定义均有清晰的头文件包含支持。

---

## Chapter 8: Data SSOT Declaration Table
| 数据项 | 唯一 SSOT 源 | 派生/缓存副本 | 同步方向 |
| :--- | :--- | :--- | :--- |
| 自定义重命名预设字典 | `AppConfig` ("BatchRenamePresetsMap") | `m_presetCombo` 项与 `m_ruleRows` UI 展示 | `AppConfig` <-> `m_presetCombo` <-> `m_ruleRows` |
| 最后一次规则 JSON | `AppConfig` ("LastBatchRenameRules") | `m_ruleRows` UI 展示 | 定时器自动存入 `AppConfig` / 默认选择时恢复 |

---

## Chapter 9: Compliance & Quality Self-Check Table

| 序号 | 检查项目 | 结果 | 位置/说明 |
| :--- | :--- | :--- | :--- |
| 1 | 是否只修改实施方案（禁止直接动源码） | 满足 | 仅创建 `Implementation Plan/BatchRenameLayoutAndPresetRefactor.md` |
| 2 | SEARCH 块是否物理匹配且唯一 | 满足 | 精确匹配 `BatchRenameDialog.h` 和 `BatchRenameDialog.cpp` |
| 3 | 是否破坏冻结的 .h 公开 API 签名 | 满足 | 外部 API 零改动 |
| 4 | 是否包含 CMake 文件注册变更 | 不涉及 | 未创建新的 C++ 文件 |
| 5 | 是否遵循单向依赖流 | 满足 | 仅 UI 层内部控件组合与 Core AppConfig 数据读写 |
| 6 | 是否消除最大化布局失真与按钮冗余 | 满足 | 引入 `QSplitter` + 标题栏按钮置换 + 预设储存删除 |
| 7 | 是否包含完整 10 大章节 | 满足 | 包含 Chapter 1 至 Chapter 10 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table
- **批量重命名引擎逻辑**：`BatchRenameService::computePreview` 与 `executeAsync` 批量执行流程完全保持 100% 相同。
- **外观与模态行为**：`BatchRenameDialog` 模态阻塞与原始文件名图标解析逻辑保持 100% 相同。
