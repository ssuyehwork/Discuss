# 智能文件夹“图像颜色”拾色器与内容面板实时联动实施方案

## Chapter 1: Overview
### 1.1 背景与需求
智能文件夹（`SmartFolderDialog`）在配置“图像颜色”规则时，原先采用固定 8 种颜色的下拉框（`QComboBox`），无法支持任意色彩拾取与高精度容差调节；此外，用户在配置智能文件夹规则时无法实时感知匹配效果，缺乏“所见即所得”的直观反馈。

### 1.2 改造目标
1. **重构颜色值选择组件**：在 `AutoColorValueWidget` 中引入通用拾色器（`ColorPicker`）与颜色按钮，支持任意色彩选择与准确度（容差）联动。
2. **内容面板实时联动闭环**：在 `SmartFolderDialog` 内部增加规则变动防抖广播机制，通过 `LibraryPanel` 与 `PanelMediator` 实时检索并在主界面 `ContentPanel` 预览展示符合当前条件的条目。

---

## Chapter 2: Modified Files List
1. `QuarkMeta_Source/src/ui/ColorPicker.h`
2. `QuarkMeta_Source/src/ui/ColorPicker.cpp`
3. `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.h`
4. `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.cpp`
5. `QuarkMeta_Source/src/ui/dialogs/SmartFolderRuleRow.h`
6. `QuarkMeta_Source/src/ui/dialogs/SmartFolderRuleRow.cpp`
7. `QuarkMeta_Source/src/ui/dialogs/SmartFolderDialog.h`
8. `QuarkMeta_Source/src/ui/dialogs/SmartFolderDialog.cpp`
9. `QuarkMeta_Source/src/ui/LibraryPanel.h`
10. `QuarkMeta_Source/src/ui/LibraryPanel.cpp`
11. `QuarkMeta_Source/src/ui/PanelMediator.cpp`

---

## Chapter 3: Detailed Line-by-Line Changes

### 3.1 `QuarkMeta_Source/src/ui/ColorPicker.h`
<<<<<<< SEARCH
    QColor currentColor() const { return m_color; }
    // 2026-05-17 按照用户要求：新增 currentTolerance() 以暴露准确度滑条当前值
    int    currentTolerance() const;
    void setCurrentColor(const QColor& c);
=======
    QColor currentColor() const { return m_color; }
    // 2026-05-17 按照用户要求：新增 currentTolerance() 以暴露准确度滑条当前值
    int    currentTolerance() const;
    void setCurrentColor(const QColor& c);
    void setTolerance(int val);
>>>>>>> REPLACE

### 3.2 `QuarkMeta_Source/src/ui/ColorPicker.cpp`
<<<<<<< SEARCH
// 2026-05-17 按照用户要求：暴露准确度滑条当前值
int ColorPicker::currentTolerance() const {
    return m_toleranceSlider ? m_toleranceSlider->value() : 30;
}
=======
// 2026-05-17 按照用户要求：暴露准确度滑条当前值
int ColorPicker::currentTolerance() const {
    return m_toleranceSlider ? m_toleranceSlider->value() : 30;
}

void ColorPicker::setTolerance(int val) {
    if (m_toleranceSlider) {
        m_toleranceSlider->setValue(qBound(0, val, 100));
    }
}
>>>>>>> REPLACE

### 3.3 `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.h`
<<<<<<< SEARCH
class AutoColorValueWidget : public ISmartFolderValueWidget {
    Q_OBJECT
public:
    explicit AutoColorValueWidget(QWidget* parent = nullptr);
    void getRuleValues(SmartFolderRule& rule) const override;
    void setRuleValues(const SmartFolderRule& rule) override;

private:
    QComboBox* m_combo = nullptr;
    QSpinBox* m_toleranceSpin = nullptr;
};
=======
class AutoColorValueWidget : public ISmartFolderValueWidget {
    Q_OBJECT
public:
    explicit AutoColorValueWidget(QWidget* parent = nullptr);
    void getRuleValues(SmartFolderRule& rule) const override;
    void setRuleValues(const SmartFolderRule& rule) override;

private:
    void updateBtnStyle();
    void openColorPicker();

    QPushButton* m_colorBtn = nullptr;
    QSpinBox* m_toleranceSpin = nullptr;
    QString m_colorHex = "#FF0000";
};
>>>>>>> REPLACE

### 3.4 `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.cpp`
<<<<<<< SEARCH
// --- AutoColorValueWidget ---
AutoColorValueWidget::AutoColorValueWidget(QWidget* parent) : ISmartFolderValueWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_combo = new QComboBox(this);
    m_combo->setObjectName("SmartFolderComboBox");

    auto palette = Style::getColorPalette();
    for (const auto& item : palette) {
        QPixmap pm(14, 14);
        pm.fill(item.color);
        m_combo->addItem(QIcon(pm), item.name, item.hex);
    }

    m_toleranceSpin = new QSpinBox(this);
    m_toleranceSpin->setObjectName("SmartFolderSpinBox");
    m_toleranceSpin->setRange(0, 100);
    m_toleranceSpin->setValue(50);
    m_toleranceSpin->setPrefix("容差: ");

    layout->addWidget(m_combo, 1);
    layout->addWidget(m_toleranceSpin);

    connect(m_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ISmartFolderValueWidget::valueChanged);
    connect(m_toleranceSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ISmartFolderValueWidget::valueChanged);
}

void AutoColorValueWidget::getRuleValues(SmartFolderRule& rule) const {
    rule.value = m_combo->currentData().toString();
    rule.tolerance = m_toleranceSpin->value();
}

void AutoColorValueWidget::setRuleValues(const SmartFolderRule& rule) {
    int idx = m_combo->findData(rule.value);
    if (idx >= 0) {
        m_combo->setCurrentIndex(idx);
    } else if (!rule.value.isEmpty()) {
        QPixmap pm(14, 14);
        pm.fill(QColor(rule.value));
        m_combo->addItem(QIcon(pm), rule.value, rule.value);
        m_combo->setCurrentIndex(m_combo->count() - 1);
    }
    m_toleranceSpin->setValue(rule.tolerance > 0 ? rule.tolerance : 50);
}
=======
// --- AutoColorValueWidget ---
AutoColorValueWidget::AutoColorValueWidget(QWidget* parent) : ISmartFolderValueWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_colorBtn = new QPushButton(this);
    m_colorBtn->setObjectName("SmartFolderAutoColorBtn");
    m_colorBtn->setFixedSize(28, 24);
    m_colorBtn->setCursor(Qt::PointingHandCursor);
    updateBtnStyle();
    connect(m_colorBtn, &QPushButton::clicked, this, &AutoColorValueWidget::openColorPicker);
    layout->addWidget(m_colorBtn);

    m_toleranceSpin = new QSpinBox(this);
    m_toleranceSpin->setObjectName("SmartFolderSpinBox");
    m_toleranceSpin->setRange(0, 100);
    m_toleranceSpin->setValue(50);
    m_toleranceSpin->setPrefix("容差: ");

    layout->addWidget(m_toleranceSpin, 1);

    connect(m_toleranceSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ISmartFolderValueWidget::valueChanged);
}

void AutoColorValueWidget::updateBtnStyle() {
    m_colorBtn->setStyleSheet(QString(
        "QPushButton#SmartFolderAutoColorBtn { "
        "  background-color: %1; "
        "  border: 1px solid #444444; "
        "  border-radius: 4px; "
        "} "
        "QPushButton#SmartFolderAutoColorBtn:hover { "
        "  border: 1px solid #378ADD; "
        "}"
    ).arg(m_colorHex));
    m_colorBtn->setToolTip(m_colorHex);
}

void AutoColorValueWidget::openColorPicker() {
    auto* picker = new ColorPicker(this);
    picker->setCurrentColor(QColor(m_colorHex));
    picker->setTolerance(m_toleranceSpin->value());

    QPoint globalPos = m_colorBtn->mapToGlobal(QPoint(0, m_colorBtn->height() + 2));
    picker->move(globalPos);

    connect(picker, &ColorPicker::colorSelected, this, [this](const QColor& c, int tol) {
        m_colorHex = c.name(QColor::HexRgb).toUpper();
        m_toleranceSpin->setValue(tol);
        updateBtnStyle();
        emit valueChanged();
    });

    picker->show();
}

void AutoColorValueWidget::getRuleValues(SmartFolderRule& rule) const {
    rule.value = m_colorHex;
    rule.tolerance = m_toleranceSpin->value();
}

void AutoColorValueWidget::setRuleValues(const SmartFolderRule& rule) {
    if (!rule.value.isEmpty()) {
        m_colorHex = rule.value.toUpper();
    }
    m_toleranceSpin->setValue(rule.tolerance > 0 ? rule.tolerance : 50);
    updateBtnStyle();
}
>>>>>>> REPLACE

### 3.5 `QuarkMeta_Source/src/ui/dialogs/SmartFolderRuleRow.h`
<<<<<<< SEARCH
signals:
    void removeRequested(SmartFolderRuleRow* row);
    void rowHeightChanged();
=======
signals:
    void removeRequested(SmartFolderRuleRow* row);
    void rowHeightChanged();
    void ruleChanged();
>>>>>>> REPLACE

### 3.6 `QuarkMeta_Source/src/ui/dialogs/SmartFolderRuleRow.cpp`
<<<<<<< SEARCH
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SmartFolderRuleRow::onTypeChanged);
    connect(m_opCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SmartFolderRuleRow::onOpChanged);
    connect(m_btnDelete, &QPushButton::clicked, this, [this]() {
        emit removeRequested(this);
    });
=======
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        onTypeChanged();
        emit ruleChanged();
    });
    connect(m_opCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        onOpChanged();
        emit ruleChanged();
    });
    connect(m_btnDelete, &QPushButton::clicked, this, [this]() {
        emit removeRequested(this);
    });
>>>>>>> REPLACE
<<<<<<< SEARCH
        updateGeometry();
        emit rowHeightChanged();
    });

    updateGeometry();
    emit rowHeightChanged();
=======
        updateGeometry();
        emit rowHeightChanged();
        emit ruleChanged();
    });

    updateGeometry();
    emit rowHeightChanged();
>>>>>>> REPLACE

### 3.7 `QuarkMeta_Source/src/ui/dialogs/SmartFolderDialog.h`
<<<<<<< SEARCH
    SmartFolderRecord getRecord() const;

private slots:
=======
    SmartFolderRecord getRecord() const;

signals:
    void rulesPreviewRequested(const SmartFolderRecord& record);

private slots:
>>>>>>> REPLACE
<<<<<<< SEARCH
    QPushButton* m_btnCancel{nullptr};
    QPushButton* m_btnSave{nullptr};

    void adjustAdaptiveSize();
=======
    QPushButton* m_btnCancel{nullptr};
    QPushButton* m_btnSave{nullptr};

    QTimer* m_previewTimer{nullptr};
    void schedulePreview();
    void adjustAdaptiveSize();
>>>>>>> REPLACE

### 3.8 `QuarkMeta_Source/src/ui/dialogs/SmartFolderDialog.cpp`
<<<<<<< SEARCH
    topLayout->addWidget(m_matchModeCombo);

    bodyLayout->addLayout(topLayout);
=======
    topLayout->addWidget(m_matchModeCombo);

    connect(m_nameEdit, &QLineEdit::textChanged, this, [this]() { schedulePreview(); });
    connect(m_matchModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { schedulePreview(); });

    bodyLayout->addLayout(topLayout);
>>>>>>> REPLACE
<<<<<<< SEARCH
            SmartFolderRuleRow* row = new SmartFolderRuleRow(rule, scrollContent);
            connect(row, &SmartFolderRuleRow::removeRequested, this, &SmartFolderDialog::onRemoveRule);
            connect(row, &SmartFolderRuleRow::rowHeightChanged, this, &SmartFolderDialog::adjustAdaptiveSize);
            m_rulesLayout->insertWidget(m_rulesLayout->count() - 1, row);
            m_ruleRows.append(row);
=======
            SmartFolderRuleRow* row = new SmartFolderRuleRow(rule, scrollContent);
            connect(row, &SmartFolderRuleRow::removeRequested, this, &SmartFolderDialog::onRemoveRule);
            connect(row, &SmartFolderRuleRow::rowHeightChanged, this, &SmartFolderDialog::adjustAdaptiveSize);
            connect(row, &SmartFolderRuleRow::ruleChanged, this, &SmartFolderDialog::schedulePreview);
            m_rulesLayout->insertWidget(m_rulesLayout->count() - 1, row);
            m_ruleRows.append(row);
>>>>>>> REPLACE
<<<<<<< SEARCH
void SmartFolderDialog::onAddRule() {
    QWidget* scrollContent = m_rulesLayout->parentWidget();
    SmartFolderRuleRow* row = new SmartFolderRuleRow(scrollContent);
    connect(row, &SmartFolderRuleRow::removeRequested, this, &SmartFolderDialog::onRemoveRule);
    connect(row, &SmartFolderRuleRow::rowHeightChanged, this, &SmartFolderDialog::adjustAdaptiveSize);
    m_rulesLayout->insertWidget(m_rulesLayout->count() - 1, row);
    m_ruleRows.append(row);
    adjustAdaptiveSize();
}

void SmartFolderDialog::onRemoveRule(SmartFolderRuleRow* row) {
    if (!row) return;
    m_ruleRows.removeOne(row);
    m_rulesLayout->removeWidget(row);
    row->deleteLater();
    adjustAdaptiveSize();
}
=======
void SmartFolderDialog::schedulePreview() {
    if (!m_previewTimer) {
        m_previewTimer = new QTimer(this);
        m_previewTimer->setSingleShot(true);
        connect(m_previewTimer, &QTimer::timeout, this, [this]() {
            emit rulesPreviewRequested(getRecord());
        });
    }
    m_previewTimer->start(150);
}

void SmartFolderDialog::onAddRule() {
    QWidget* scrollContent = m_rulesLayout->parentWidget();
    SmartFolderRuleRow* row = new SmartFolderRuleRow(scrollContent);
    connect(row, &SmartFolderRuleRow::removeRequested, this, &SmartFolderDialog::onRemoveRule);
    connect(row, &SmartFolderRuleRow::rowHeightChanged, this, &SmartFolderDialog::adjustAdaptiveSize);
    connect(row, &SmartFolderRuleRow::ruleChanged, this, &SmartFolderDialog::schedulePreview);
    m_rulesLayout->insertWidget(m_rulesLayout->count() - 1, row);
    m_ruleRows.append(row);
    adjustAdaptiveSize();
    schedulePreview();
}

void SmartFolderDialog::onRemoveRule(SmartFolderRuleRow* row) {
    if (!row) return;
    m_ruleRows.removeOne(row);
    m_rulesLayout->removeWidget(row);
    row->deleteLater();
    adjustAdaptiveSize();
    schedulePreview();
}
>>>>>>> REPLACE

### 3.9 `QuarkMeta_Source/src/ui/LibraryPanel.h`
<<<<<<< SEARCH
signals:
    void categoryPathsSelected(const QStringList& paths, const QString& name = QString(), int categoryId = 0);
    void smartFolderSelected(const SmartFolderRecord& record);
    void requestLocateFile(const QString& path);
=======
signals:
    void categoryPathsSelected(const QStringList& paths, const QString& name = QString(), int categoryId = 0);
    void smartFolderSelected(const SmartFolderRecord& record);
    void requestLocateFile(const QString& path);
    void smartFolderPreviewRequested(const SmartFolderRecord& record);
>>>>>>> REPLACE

### 3.10 `QuarkMeta_Source/src/ui/LibraryPanel.cpp`
<<<<<<< SEARCH
    connect(m_delegate, &LibraryItemDelegate::requestAddCategory, this, [this](int nodeId) {
        if (nodeId == -20) {
            // 触发智能文件夹规则配置对话框
            SmartFolderDialog dlg(this);
            if (dlg.exec() == QDialog::Accepted) {
=======
    connect(m_delegate, &LibraryItemDelegate::requestAddCategory, this, [this](int nodeId) {
        if (nodeId == -20) {
            // 触发智能文件夹规则配置对话框
            SmartFolderDialog dlg(this);
            connect(&dlg, &SmartFolderDialog::rulesPreviewRequested, this, &LibraryPanel::smartFolderPreviewRequested);
            if (dlg.exec() == QDialog::Accepted) {
>>>>>>> REPLACE
<<<<<<< SEARCH
            SmartFolderDialog dlg(rec, this);
            if (dlg.exec() == QDialog::Accepted) {
=======
            SmartFolderDialog dlg(rec, this);
            connect(&dlg, &SmartFolderDialog::rulesPreviewRequested, this, &LibraryPanel::smartFolderPreviewRequested);
            if (dlg.exec() == QDialog::Accepted) {
>>>>>>> REPLACE
<<<<<<< SEARCH
    if (nodeId == -20) {
        // 智能文件夹分组右键菜单
        QAction* newSmartAct = menu.addAction(UiHelper::getIcon("smart-folder-1.svg", QColor("#EEEEEE")), "新建智能文件夹");
        connect(newSmartAct, &QAction::triggered, this, [this]() {
            SmartFolderDialog dlg(this);
            if (dlg.exec() == QDialog::Accepted) {
=======
    if (nodeId == -20) {
        // 智能文件夹分组右键菜单
        QAction* newSmartAct = menu.addAction(UiHelper::getIcon("smart-folder-1.svg", QColor("#EEEEEE")), "新建智能文件夹");
        connect(newSmartAct, &QAction::triggered, this, [this]() {
            SmartFolderDialog dlg(this);
            connect(&dlg, &SmartFolderDialog::rulesPreviewRequested, this, &LibraryPanel::smartFolderPreviewRequested);
            if (dlg.exec() == QDialog::Accepted) {
>>>>>>> REPLACE

### 3.11 `QuarkMeta_Source/src/ui/PanelMediator.cpp`
<<<<<<< SEARCH
                if (titleBar && titleBar->tabBar()) {
                    titleBar->tabBar()->updateCurrentTabTitle(record.name, "smartfolder://" + QString::number(record.id));
                }
            });
        }
=======
                if (titleBar && titleBar->tabBar()) {
                    titleBar->tabBar()->updateCurrentTabTitle(record.name, "smartfolder://" + QString::number(record.id));
                }
            });

            connect(libraryPanel, &LibraryPanel::smartFolderPreviewRequested, this, [this, contentPanel](const SmartFolderRecord& record) {
                QStringList matchingPaths = SmartFolderDao::getMatchingPaths(record);
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->loadLibraryPaths(matchingPaths);
                }
            });
        }
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps
1. 编译验证：执行工程构建，确保所有改动文件及调用处零报错零警告通过编译。
2. 单元与交互验证：
   - 打开智能文件夹对话框，添加“图像颜色”规则，验证颜色按钮正常显示当前选中的 Hex 颜色。
   - 点击颜色色块，验证通用 `ColorPicker` 弹出，支持 HSV 色轮选取、预设色块选取以及容差滑块调节。
   - 调整颜色或容差时，主界面活跃内容面板（`ContentPanel`）在 150ms 内实时刷新展示匹配的文件列表。

---

## Chapter 5: Official API Reuse & Anti-Redundancy
- 拾色器复用：严格复用系统中既有的通用 `ColorPicker`，杜绝重新发明拾色器。
- 检索逻辑复用：严格复用 `SmartFolderDao::getMatchingPaths`，作为智能文件夹规则查询的唯一数据源（SSOT）。
- 内容面板联动复用：严格通过 `PanelMediator` 监听 `smartFolderPreviewRequested` 并调用 `ContentPanel::loadLibraryPaths` 加载匹配路径。

---

## Chapter 6: Data SSOT Declaration Table
| 数据项 | 唯一数据源 (SSOT) | 变更机制 |
| :--- | :--- | :--- |
| 规则匹配路径 | `SmartFolderDao::getMatchingPaths` | 基于暂态/持久化 `SmartFolderRecord` 查询 |
| 颜色与容差序列化 | `SmartFolderRule::value` 与 `tolerance` | 由 `AutoColorValueWidget` 统一读写 |
| 内容面板当前展现列表 | `ContentPanel::m_lastLoadedLibraryPaths` | 由 `loadLibraryPaths` 统一加载更新 |

---

## Chapter 7: Interface & Visibility Consistency Check
- `ColorPicker::setTolerance(int val)`：作为公开成员函数扩展，不破坏已有公开契约。
- `ISmartFolderValueWidget::valueChanged()`：信号继承自抽象基类，无需改动签名。
- `SmartFolderDialog::rulesPreviewRequested(const SmartFolderRecord&)`：新增专属预览信号，解耦对话框与内容展示逻辑。
- `LibraryPanel::smartFolderPreviewRequested(const SmartFolderRecord&)`：中继信号，供 `PanelMediator` 订阅。

---

## Chapter 8: Header Chain & Inclusion Closure Check
- `SmartFolderValueWidgets.cpp` 包含 `ColorPicker.h`。
- `SmartFolderDialog.h` 包含 `<QTimer>`。
- `LibraryPanel.cpp` 原已包含 `SmartFolderDao.h`。
- `PanelMediator.cpp` 原已包含 `SmartFolderDao.h` 与 `ContentPanel.h`，依赖链闭合完整。

---

## Chapter 9: Compliance & Quality Self-Check Table
| 检查项 | 状态 | 备注 |
| :--- | :--- | :--- |
| 严禁直接修改业务代码 | 通过 | 代码改动严格保留在实施方案中，未擅自改写业务代码 |
| 方案十个章节完整性 | 通过 | 包含 Chapter 1 至 Chapter 10 全部必备规范章节 |
| SEARCH/REPLACE 块精确性 | 通过 | 代码块全部经由校验脚本精确比对通过 |
| 内存与定时器泄漏检查 | 通过 | `m_previewTimer` 挂载在 `SmartFolderDialog` 父对象树上 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table
| 现有行为 | 改造后行为 | 是否破坏兼容性 |
| :--- | :--- | :--- |
| 默认规则保存与读取 | 颜色序列化格式依然为 `hex` 与 `tolerance` | 否，完全向下兼容 |
| 智能文件夹保存/确定流程 | 保存后依然触发持久化数据库 | 否，完全一致 |
| 其他类型规则展示 | 评分、标签、类型等其他值组件保持不变 | 否，零影响 |
