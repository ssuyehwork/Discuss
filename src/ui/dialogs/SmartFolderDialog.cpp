#include "SmartFolderDialog.h"
#include "../UiHelper.h"
#include "AppConfig.h"
#include <QScrollArea>
#include <QPainter>
#include <QPixmap>
#include <QJsonDocument>
#include <QJsonArray>

namespace QuarkMeta {

// --- SmartFolderRuleRow Implementation ---

SmartFolderRuleRow::SmartFolderRuleRow(QWidget* parent) : QWidget(parent) {
    setupUi();
}

SmartFolderRuleRow::SmartFolderRuleRow(const SmartFolderRule& rule, QWidget* parent) : QWidget(parent) {
    setupUi();
    setRule(rule);
}

void SmartFolderRuleRow::setupUi() {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 2, 0, 2);
    layout->setSpacing(8);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem("星级", static_cast<int>(SmartRuleType::Rating));
    m_typeCombo->addItem("颜色", static_cast<int>(SmartRuleType::Color));
    m_typeCombo->addItem("标签", static_cast<int>(SmartRuleType::Tag));
    m_typeCombo->addItem("扩展名/类型", static_cast<int>(SmartRuleType::Extension));
    m_typeCombo->addItem("关键字", static_cast<int>(SmartRuleType::Keyword));
    m_typeCombo->setFixedWidth(110);

    m_opCombo = new QComboBox(this);
    m_opCombo->setFixedWidth(110);

    m_valueEdit = new QLineEdit(this);

    m_valueCombo = new QComboBox(this);
    m_valueCombo->hide();

    m_btnDelete = new QPushButton(this);
    m_btnDelete->setFixedSize(28, 28);
    m_btnDelete->setCursor(Qt::PointingHandCursor);
    m_btnDelete->setIcon(UiHelper::getIcon("close", QColor("#888888"), 14));

    layout->addWidget(m_typeCombo);
    layout->addWidget(m_opCombo);
    layout->addWidget(m_valueEdit, 1);
    layout->addWidget(m_valueCombo, 1);
    layout->addWidget(m_btnDelete);

    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SmartFolderRuleRow::onTypeChanged);
    connect(m_btnDelete, &QPushButton::clicked, this, [this]() {
        emit removeRequested(this);
    });

    onTypeChanged(0);
}

void SmartFolderRuleRow::onTypeChanged(int index) {
    Q_UNUSED(index);
    SmartRuleType type = static_cast<SmartRuleType>(m_typeCombo->currentData().toInt());

    m_opCombo->clear();
    m_valueEdit->show();
    m_valueCombo->hide();

    switch (type) {
    case SmartRuleType::Rating:
        m_opCombo->addItem("大于等于", static_cast<int>(SmartRuleOperator::GreaterOrEqual));
        m_opCombo->addItem("等于", static_cast<int>(SmartRuleOperator::Equals));
        m_opCombo->addItem("小于等于", static_cast<int>(SmartRuleOperator::LessOrEqual));

        m_valueEdit->hide();
        m_valueCombo->clear();
        {
            int starSize = 14;
            int spacing = 2;
            int maxW = 5 * starSize + 4 * spacing;
            for (int r = 1; r <= 5; ++r) {
                QPixmap pix(maxW, starSize);
                pix.fill(Qt::transparent);
                QPainter painter(&pix);
                QPixmap starPix = UiHelper::getIcon("star_filled", QColor("#CCCCCC"), starSize).pixmap(starSize, starSize);
                for (int i = 0; i < r; ++i) {
                    painter.drawPixmap(i * (starSize + spacing), 0, starPix);
                }
                painter.end();
                m_valueCombo->addItem(QIcon(pix), "", QString::number(r));
            }
        }
        m_valueCombo->setIconSize(QSize(14 * 5 + 2 * 4, 14));
        m_valueCombo->show();
        break;

    case SmartRuleType::Color:
        m_opCombo->addItem("等于", static_cast<int>(SmartRuleOperator::Equals));
        m_valueEdit->setPlaceholderText("选择或输入颜色 hex (如 #FF0000)");
        break;

    case SmartRuleType::Tag:
        m_opCombo->addItem("包含", static_cast<int>(SmartRuleOperator::Contains));
        m_opCombo->addItem("不包含", static_cast<int>(SmartRuleOperator::NotContains));
        m_valueEdit->setPlaceholderText("输入标签名称");
        break;

    case SmartRuleType::Extension:
        m_opCombo->addItem("等于", static_cast<int>(SmartRuleOperator::Equals));
        m_valueEdit->setPlaceholderText("输入文件扩展名 (如 png, psd)");
        break;

    case SmartRuleType::Keyword:
        m_opCombo->addItem("包含", static_cast<int>(SmartRuleOperator::Contains));
        m_opCombo->addItem("不包含", static_cast<int>(SmartRuleOperator::NotContains));
        m_valueEdit->setPlaceholderText("输入关键字");
        break;
    }
}

SmartFolderRule SmartFolderRuleRow::getRule() const {
    SmartFolderRule rule;
    rule.type = static_cast<SmartRuleType>(m_typeCombo->currentData().toInt());
    rule.op = static_cast<SmartRuleOperator>(m_opCombo->currentData().toInt());

    if (m_valueCombo->isVisible()) {
        rule.value = m_valueCombo->currentData().toString();
    } else {
        rule.value = m_valueEdit->text().trimmed();
    }
    return rule;
}

void SmartFolderRuleRow::setRule(const SmartFolderRule& rule) {
    int typeIdx = m_typeCombo->findData(static_cast<int>(rule.type));
    if (typeIdx >= 0) {
        m_typeCombo->setCurrentIndex(typeIdx);
    }

    int opIdx = m_opCombo->findData(static_cast<int>(rule.op));
    if (opIdx >= 0) {
        m_opCombo->setCurrentIndex(opIdx);
    }

    if (m_valueCombo->isVisible()) {
        int valIdx = m_valueCombo->findData(rule.value);
        if (valIdx >= 0) m_valueCombo->setCurrentIndex(valIdx);
    } else {
        m_valueEdit->setText(rule.value);
    }
}

// --- SmartFolderDialog Implementation ---

SmartFolderDialog::SmartFolderDialog(QWidget* parent) : FramelessDialog("智能文件夹规则配置", parent) {
    m_record.name = "新智能文件夹";
    m_record.matchMode = "ALL";
    setupUi();
}

SmartFolderDialog::SmartFolderDialog(const SmartFolderRecord& record, QWidget* parent) : FramelessDialog("智能文件夹规则配置", parent), m_record(record) {
    setupUi();
}

void SmartFolderDialog::setupUi() {
    resize(520, 420);

    QWidget* bodyWidget = getContentArea();
    QVBoxLayout* bodyLayout = new QVBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(20, 15, 20, 20);
    bodyLayout->setSpacing(12);

    // 智能文件夹名称与匹配模式配置
    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->setSpacing(12);

    QLabel* nameLabel = new QLabel("名称:", bodyWidget);
    nameLabel->setObjectName("SmartFolderLabel");

    m_nameEdit = new QLineEdit(bodyWidget);
    m_nameEdit->setText(m_record.name);

    QLabel* modeLabel = new QLabel("匹配:", bodyWidget);
    modeLabel->setObjectName("SmartFolderLabel");

    m_matchModeCombo = new QComboBox(bodyWidget);
    m_matchModeCombo->addItem("符合以下所有条件 (AND)", "ALL");
    m_matchModeCombo->addItem("符合以下任一条件 (OR)", "ANY");
    if (m_record.matchMode == "ANY") {
        m_matchModeCombo->setCurrentIndex(1);
    } else {
        m_matchModeCombo->setCurrentIndex(0);
    }

    topLayout->addWidget(nameLabel);
    topLayout->addWidget(m_nameEdit, 1);
    topLayout->addWidget(modeLabel);
    topLayout->addWidget(m_matchModeCombo);

    bodyLayout->addLayout(topLayout);

    // 分割线
    QFrame* line = new QFrame(bodyWidget);
    line->setFrameShape(QFrame::HLine);
    line->setObjectName("SmartFolderSeparatorLine");
    bodyLayout->addWidget(line);

    // 规则列表区域
    QScrollArea* scrollArea = new QScrollArea(bodyWidget);
    scrollArea->setObjectName("SmartFolderScrollArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* scrollContent = new QWidget(scrollArea);
    m_rulesLayout = new QVBoxLayout(scrollContent);
    m_rulesLayout->setContentsMargins(0, 0, 0, 0);
    m_rulesLayout->setSpacing(6);
    m_rulesLayout->addStretch(1);

    scrollArea->setWidget(scrollContent);
    bodyLayout->addWidget(scrollArea, 1);

    // 添加规则按钮
    m_btnAddRule = new QPushButton("+ 添加新规则", bodyWidget);
    m_btnAddRule->setFixedHeight(30);

    bodyLayout->addWidget(m_btnAddRule);

    // 底部控制按钮
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    btnLayout->addStretch(1);

    m_btnCancel = new QPushButton("取消", bodyWidget);
    m_btnCancel->setFixedSize(80, 32);

    m_btnSave = new QPushButton("保存", bodyWidget);
    m_btnSave->setFixedSize(80, 32);

    btnLayout->addWidget(m_btnCancel);
    btnLayout->addWidget(m_btnSave);

    bodyLayout->addLayout(btnLayout);

    connect(m_btnAddRule, &QPushButton::clicked, this, &SmartFolderDialog::onAddRule);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_btnSave, &QPushButton::clicked, this, &SmartFolderDialog::onSaveAndAccept);

    // 加载已有规则；若为新建且无指定规则，尝试从 AppConfig 恢复上一次保存的整套模板规则
    if (m_record.rules.isEmpty()) {
        QString lastMatchMode = AppConfig::instance().getValue("SmartFolder/LastMatchMode", "").toString();
        if (!lastMatchMode.isEmpty()) {
            m_record.matchMode = lastMatchMode;
            if (lastMatchMode == "ANY") {
                m_matchModeCombo->setCurrentIndex(1);
            } else {
                m_matchModeCombo->setCurrentIndex(0);
            }
        }

        QString lastRulesJsonStr = AppConfig::instance().getValue("SmartFolder/LastTemplateRules", "").toString();
        if (!lastRulesJsonStr.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(lastRulesJsonStr.toUtf8());
            if (doc.isArray()) {
                QJsonArray arr = doc.array();
                for (const auto& val : arr) {
                    if (val.isObject()) {
                        m_record.rules.append(SmartFolderRule::fromJson(val.toObject()));
                    }
                }
            }
        }
    }

    if (!m_record.rules.isEmpty()) {
        for (const auto& rule : m_record.rules) {
            SmartFolderRuleRow* row = new SmartFolderRuleRow(rule, scrollContent);
            connect(row, &SmartFolderRuleRow::removeRequested, this, &SmartFolderDialog::onRemoveRule);
            m_rulesLayout->insertWidget(m_rulesLayout->count() - 1, row);
            m_ruleRows.append(row);
        }
    } else {
        onAddRule();
    }
}

void SmartFolderDialog::onAddRule() {
    QWidget* scrollContent = m_rulesLayout->parentWidget();
    SmartFolderRuleRow* row = new SmartFolderRuleRow(scrollContent);
    connect(row, &SmartFolderRuleRow::removeRequested, this, &SmartFolderDialog::onRemoveRule);
    m_rulesLayout->insertWidget(m_rulesLayout->count() - 1, row);
    m_ruleRows.append(row);
}

void SmartFolderDialog::onRemoveRule(SmartFolderRuleRow* row) {
    if (!row) return;
    m_ruleRows.removeOne(row);
    m_rulesLayout->removeWidget(row);
    row->deleteLater();
}

void SmartFolderDialog::onSaveAndAccept() {
    m_record.name = m_nameEdit->text().trimmed();
    if (m_record.name.isEmpty()) {
        m_record.name = "新智能文件夹";
    }

    m_record.matchMode = m_matchModeCombo->currentData().toString();
    m_record.rules.clear();

    for (SmartFolderRuleRow* row : m_ruleRows) {
        if (row) {
            m_record.rules.append(row->getRule());
        }
    }

    // 将整套规则模板序列化持久化至 AppConfig
    AppConfig::instance().setValue("SmartFolder/LastMatchMode", m_record.matchMode);
    QJsonArray rulesArr;
    for (const auto& rule : m_record.rules) {
        rulesArr.append(rule.toJson());
    }
    QJsonDocument doc(rulesArr);
    AppConfig::instance().setValue("SmartFolder/LastTemplateRules", QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));

    accept();
}

SmartFolderRecord SmartFolderDialog::getRecord() const {
    return m_record;
}

} // namespace QuarkMeta
