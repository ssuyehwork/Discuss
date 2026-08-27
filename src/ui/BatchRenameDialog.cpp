#include "BatchRenameDialog.h"
#include "RuleRow.h"
#include "UiHelper.h"
#include "DiskBatchRenameService.h"
#include "PresetManager.h"
#include "UndoToastOverlay.h"
#include "ShellIconManager.h"
#include "../util/DiskMediaExtractor.h"
#include "../meta/BatchRenameEngine.h"
#include "../meta/MetadataManager.h"
#include <QHeaderView>
#include "FramelessFileDialog.h"
#include "FramelessDialog.h"
#include "ToolTipOverlay.h"
#include <QFileInfo>
#include <QTimer>
#include <QDir>
#include <QLabel>
#include <QFile>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTableWidgetItem>
#include <QRadioButton>
#include <QScrollArea>
#include "../core/AppConfig.h"
#include "../core/UndoManager.h"
#include "../core/BasicCommands.h"
#include "../core/OperationSnapshotEngine.h"

namespace QuarkMeta {

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
    m_btnDeletePreset->setStyleSheet(
        "QPushButton { background: #3E3E42; color: white; border: none; border-radius: 4px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background: #4E4E52; }"
    );
    presetL->addWidget(m_btnDeletePreset);

    // 导入/导出按钮
    m_btnImportPreset = new QPushButton("导入...", presetGroup);
    m_btnExportPreset = new QPushButton("导出...", presetGroup);
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
    m_btnBrowse->setFixedSize(80, 25);
    m_btnBrowse->setEnabled(false);
    pathL->addWidget(m_targetPathEdit);
    pathL->addWidget(m_btnBrowse);
    targetL->addLayout(pathL);

    configL->addWidget(targetGroup);

    // 3. 新文件名 (规则构造器)
    QGroupBox* rulesGroup = new QGroupBox("新文件名", leftPanel);
    rulesGroup->setStyleSheet("QGroupBox { padding-top: 15px; margin-top: 5px; } QGroupBox::title { top: -2px; left: 8px; }");
    
    QVBoxLayout* rulesGroupL = new QVBoxLayout(rulesGroup);
    rulesGroupL->setContentsMargins(4, 2, 4, 4);
    rulesGroupL->setSpacing(0);
    
    QScrollArea* scroll = new QScrollArea(rulesGroup);
    scroll->setWidgetResizable(true);
    scroll->setAlignment(Qt::AlignTop);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: transparent; border: none; padding: 0px; margin: 0px;");
    
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
    m_btnExecute->setStyleSheet(
        "QPushButton {"
        "  background-color: #378ADD;"
        "  color: #FFFFFF;"
        "  border: none;"
        "  border-radius: 6px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #4A9BE8;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #2674C2;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #333333;"
        "  color: #777777;"
        "}"
    );
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

void BatchRenameDialog::applyTheme() {
    static const QString arrowPath = UiHelper::getSvgTempFilePath("dropdown_triangle", QColor("#AAAAAA"));

    setStyleSheet(QString(
        "QDialog { background-color: #1E1E1E; color: #BBB; }"
        "QGroupBox { border: 1px solid #333; border-radius: 4px; margin-top: 10px; font-weight: bold; font-size: 11px; color: #888; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 3px; }"
        "QLineEdit { background: #252526; border: 1px solid #444; border-radius: 4px; padding: 2px 5px; color: #EEE; }"
        "QRadioButton { color: #BBB; spacing: 5px; }"
        "QPushButton { background: #333; color: #EEE; border-radius: 4px; }"
        "QComboBox { background: #252526; border: 1px solid #444; border-radius: 4px; padding: 1px 4px; color: #EEE; }"
        "QComboBox::drop-down { border: none; width: 24px; }"
        "QComboBox::down-arrow { image: url(%1); width: 12px; height: 12px; }"
        "QComboBox QAbstractItemView { background-color: #2D2D2D; border: 1px solid #444; selection-background-color: #3E3E42; selection-color: white; color: #EEE; outline: 0; }"
        "QComboBox QAbstractItemView::item { height: 22px; padding: 2px; }" 
        "QTableWidget { background-color: #1E1E1E; alternate-background-color: #252526; color: #EEEEEE; border: 1px solid #333333; gridline-color: transparent; selection-background-color: rgba(52, 152, 219, 0.2); outline: none; }"
        "QTableWidget::item { outline: none; border: none; }"
        "QTableWidget::item:focus { outline: none; border: none; }"
        "QHeaderView::section { background-color: #2D2D2D; color: #888888; border: none; height: 30px; font-weight: bold; font-size: 11px; }"
    ).arg(arrowPath));
}

void BatchRenameDialog::onAddRow(RuleRow* targetRow) {
    RuleRow* row = new RuleRow(m_rulesContainer);

    int insertIndex = -1;
    if (targetRow) {
        insertIndex = m_ruleRows.indexOf(targetRow);
    }

    if (insertIndex >= 0 && insertIndex < m_ruleRows.size()) {
        m_rulesLayout->insertWidget(insertIndex + 1, row);
        m_ruleRows.insert(insertIndex + 1, row);
    } else {
        m_rulesLayout->addWidget(row);
        m_ruleRows.append(row);
    }
    
    connect(row, &RuleRow::changed, this, &BatchRenameDialog::updatePreview);
    connect(row, &RuleRow::changed, this, &BatchRenameDialog::scheduleAutoSave);
    connect(row, &RuleRow::addRequested, this, [this, row]() {
        onAddRow(row);
    });
    connect(row, &RuleRow::addRequested, this, &BatchRenameDialog::scheduleAutoSave);
    connect(row, &RuleRow::removeRequested, [this, row]() {
        if (m_ruleRows.size() > 1) {
            m_ruleRows.removeOne(row);
            row->deleteLater();
            updatePreview();
            scheduleAutoSave();
        }
    });

    updatePreview();
}

void BatchRenameDialog::initTableItems() {
    m_table->setRowCount(static_cast<int>(m_originalPaths.size()));
    for (int i = 0; i < static_cast<int>(m_originalPaths.size()); ++i) {
        QString oldPath = QString::fromStdWString(m_originalPaths[static_cast<size_t>(i)]);
        QFileInfo info(oldPath);

        // 恢复老版本高速机制：直接加载现成的缩略图缓存小图（微秒级），没有才退避为文件图标
        QIcon fileIcon;
        QString thumbPath = DiskMediaExtractor::getDiskThumbCachePath(oldPath);
        if (QFile::exists(thumbPath)) {
            QPixmap pix(thumbPath);
            if (!pix.isNull()) {
                fileIcon = QIcon(pix);
            }
        }
        if (fileIcon.isNull()) {
            fileIcon = ShellIconManager::getFileIconFast(oldPath, info.isDir(), info.suffix().toLower());
        }

        auto* itemOld = new QTableWidgetItem(fileIcon, info.fileName());
        itemOld->setForeground(QColor("#B0B0B0"));
        m_table->setItem(i, 0, itemOld);

        auto* itemNew = new QTableWidgetItem();
        itemNew->setForeground(QColor("#2ecc71"));
        m_table->setItem(i, 1, itemNew);
    }
}

void BatchRenameDialog::updatePreview() {
    if (m_isInitializing) return; // 初始化加载规则期间直接拦截，不跑无用计算

    std::vector<RenameRule> rules;
    for (auto* row : m_ruleRows) {
        if (row) rules.push_back(row->getRule());
    }

    auto newNames = BatchRenameEngine::instance().preview(m_originalPaths, rules);
    int total = static_cast<int>(newNames.size());

    // 仅更新第 1 列的新名称文本，耗时 < 0.1ms，打字极度丝滑！
    for (int i = 0; i < total; ++i) {
        QTableWidgetItem* itemNew = m_table->item(i, 1);
        if (itemNew) {
            itemNew->setText(QString::fromStdWString(newNames[static_cast<size_t>(i)]));
        }
    }
}

void BatchRenameDialog::onImportPreset() {
    QString path = FramelessFileDialog::getOpenFileName(this, "导入重命名预设", "", "JSON Files (*.json)");
    if (path.isEmpty()) return;

    auto rules = PresetManager::importFromFile(path);
    if (!rules.empty()) {
        // 清空现有规则
        while (m_ruleRows.size() > 0) {
            auto* row = m_ruleRows.takeAt(0);
            row->deleteLater();
        }

        for (const auto& rule : rules) {
            onAddRow();
            m_ruleRows.last()->setRule(rule);
        }
        scheduleAutoSave();
        updatePreview();
    }
}

void BatchRenameDialog::onExportPreset() {
    QString path = FramelessFileDialog::getSaveFileName(this, "导出重命名预设", "Preset.json", "JSON Files (*.json)");
    if (path.isEmpty()) return;

    std::vector<RenameRule> rules;
    for (auto* row : m_ruleRows) {
        rules.push_back(row->getRule());
    }

    if (PresetManager::exportToFile(path, rules)) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "预设导出成功", 1500, QColor("#2ecc71"));
    }
}

void BatchRenameDialog::onDeleteCurrentPreset() {
    if (m_presetCombo->count() > 0) {
        int index = m_presetCombo->currentIndex();
        m_presetCombo->removeItem(index);
        ToolTipOverlay::instance()->showText(QCursor::pos(), "预设已删除", 1000);
    }
}

void BatchRenameDialog::scheduleAutoSave() {
    if (m_autoSaveTimer) m_autoSaveTimer->start(500);
}

void BatchRenameDialog::doAutoSave() {
    std::vector<RenameRule> rules;
    for (auto* row : m_ruleRows) {
        rules.push_back(row->getRule());
    }
    QString compactJson = PresetManager::serializeRules(rules);
    AppConfig::instance().setValue("LastBatchRenameRules", compactJson);
}

void BatchRenameDialog::onBrowseTarget() {
    QString dir = FramelessFileDialog::getExistingDirectory(this, "选择目标文件夹");
    if (!dir.isEmpty()) {
        m_targetPathEdit->setText(dir);
    }
}

void BatchRenameDialog::onExecute() {
    std::vector<RenameRule> rules;
    for (auto* row : m_ruleRows) rules.push_back(row->getRule());
    
    auto newNames = BatchRenameEngine::instance().preview(m_originalPaths, rules);
    if (newNames.empty()) return;

    // 禁用执行按钮以防重复点击
    m_btnExecute->setEnabled(false);

    // 记录本次执行所操作的旧路径、旧物理目标及新物理目标
    bool isCapsule = false;
    DiskOperationMode mode = DiskOperationMode::Rename;
    if (m_rbMove->isChecked()) mode = DiskOperationMode::Move;
    else if (m_rbCopy->isChecked()) mode = DiskOperationMode::Copy;

    QString targetDir = m_targetPathEdit->text();
    if (!isCapsule && mode != DiskOperationMode::Rename && targetDir.isEmpty()) {
        FramelessMessageBox::warning(this, "错误", "请先选择目标文件夹");
        m_btnExecute->setEnabled(true);
        return;
    }

    std::vector<std::wstring> oldPathsSnap = m_originalPaths;
    std::vector<std::wstring> newPathsSnap;
    newPathsSnap.reserve(m_originalPaths.size());

    for (size_t i = 0; i < m_originalPaths.size(); ++i) {
        QString oldPath = QString::fromStdWString(m_originalPaths[i]);
        QFileInfo oldInfo(oldPath);
        QString destDir = (mode == DiskOperationMode::Rename || isCapsule) ? oldInfo.absolutePath() : targetDir;
        QString newPathStr = QDir(destDir).absoluteFilePath(QString::fromStdWString(newNames[i]));
        newPathsSnap.push_back(QDir::toNativeSeparators(newPathStr).toStdWString());
    }

    QPointer<BatchRenameDialog> safeThis(this);
    auto onCompletedCallback = [safeThis, isCapsule, mode, oldPathsSnap, newPathsSnap](int successCount) {
        if (!safeThis) return;
        // 确保回到 UI 主线程
        safeThis->m_btnExecute->setEnabled(true);

        if (successCount > 0) {
            // 只有在存在实际成功记录时，才依据真实成功数自增序列号
            for (auto* row : safeThis->m_ruleRows) {
                RenameRule rule = row->getRule();
                if (rule.type == RenameComponentType::Sequence) {
                    rule.start = rule.start + successCount * rule.step;
                    row->setRule(rule);
                }
            }
            safeThis->doAutoSave();

            // 成功物理移动或重命名或复制后，向 UndoManager 推送一次完整的原子 BatchRenameCommand
            UndoManager::instance().pushCommand(std::make_unique<BatchRenameCommand>(mode, oldPathsSnap, newPathsSnap));
        }

        std::vector<RenameRule> currentRules;
        for (auto* row : safeThis->m_ruleRows) currentRules.push_back(row->getRule());
        auto finalNames = BatchRenameEngine::instance().preview(safeThis->m_originalPaths, currentRules);
        if (!finalNames.empty()) {
            safeThis->m_firstNewName = QString::fromStdWString(finalNames.front());
        }

        QWidget* mainWindowPtr = nullptr;
        QWidget* parentW = safeThis->parentWidget();
        while (parentW) {
            if (parentW->inherits("QuarkMeta::MainWindow") || parentW->objectName() == "MainWindow") {
                mainWindowPtr = parentW;
                break;
            }
            parentW = parentW->parentWidget();
        }

        UndoToastOverlay::instance()->showToast(
            mainWindowPtr,
            QString("成功处理 %1 个项目").arg(successCount),
            [successCount]() {
                if (successCount > 0) {
                    UndoManager::instance().undo();
                }
            },
            5000
        );
        safeThis->accept();
    };

    QStringList targetPaths;
    for (const auto& wp : m_originalPaths) {
        targetPaths << QString::fromStdWString(wp);
    }

    OperationSnapshotEngine::instance().executeWithSnapshot(
        this->parentWidget(),
        SnapshotOperationType::BatchRename,
        targetPaths,
        QString("成功处理 %1 个项目").arg(m_originalPaths.size()),
        [this, mode, targetDir, onCompletedCallback, newNames]() {
            DiskBatchRenameService::execute(m_originalPaths, newNames, mode, targetDir, onCompletedCallback);
            return true;
        },
        nullptr
    );
}

} // namespace QuarkMeta
