#include "SmartFolderRuleRow.h"
#include "../UiHelper.h"
#include <QPainter>
#include <QPixmap>

namespace QuarkMeta {

SmartFolderRuleRow::SmartFolderRuleRow(QWidget* parent) : QWidget(parent) {
    initUi();
}

SmartFolderRuleRow::SmartFolderRuleRow(const SmartFolderRule& rule, QWidget* parent) : QWidget(parent) {
    initUi();
    setRule(rule);
}

void SmartFolderRuleRow::initUi() {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 2, 0, 2);
    layout->setSpacing(8);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem("评分", static_cast<int>(SmartRuleType::Rating));
    m_typeCombo->addItem("颜色", static_cast<int>(SmartRuleType::Color));
    m_typeCombo->addItem("标签", static_cast<int>(SmartRuleType::Tag));
    m_typeCombo->addItem("扩展名", static_cast<int>(SmartRuleType::Extension));
    m_typeCombo->addItem("关键字", static_cast<int>(SmartRuleType::Keyword));

    m_opCombo = new QComboBox(this);

    m_valueEdit = new QLineEdit(this);
    m_valueCombo = new QComboBox(this);
    m_valueCombo->hide();

    m_tagContainer = new TagPillContainer(this, TagTriggerSource::SmartFolderDialog);
    m_tagContainer->hide();

    connect(m_tagContainer, &TagPillContainer::containerHeightChanged, this, [this]() {
        updateGeometry();
        emit rowHeightChanged();
    });

    m_btnDelete = new QPushButton(this);
    m_btnDelete->setFixedSize(24, 24);
    m_btnDelete->setIcon(UiHelper::getIcon("close", QColor("#CCCCCC"), 14));
    m_btnDelete->setObjectName("SmartFolderRowDeleteBtn");

    layout->addWidget(m_typeCombo);
    layout->addWidget(m_opCombo);
    layout->addWidget(m_valueEdit, 1);
    layout->addWidget(m_valueCombo, 1);
    layout->addWidget(m_tagContainer, 1);
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
    m_valueEdit->hide();
    m_valueCombo->hide();
    m_tagContainer->hide();

    switch (type) {
    case SmartRuleType::Rating:
        m_opCombo->addItem("大于等于", static_cast<int>(SmartRuleOperator::GreaterOrEqual));
        m_opCombo->addItem("等于", static_cast<int>(SmartRuleOperator::Equals));
        m_opCombo->addItem("小于等于", static_cast<int>(SmartRuleOperator::LessOrEqual));

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
        m_valueEdit->show();
        break;

    case SmartRuleType::Tag:
        m_opCombo->addItem("包含", static_cast<int>(SmartRuleOperator::Contains));
        m_opCombo->addItem("不包含", static_cast<int>(SmartRuleOperator::NotContains));
        m_tagContainer->show();
        break;

    case SmartRuleType::Extension:
        m_opCombo->addItem("等于", static_cast<int>(SmartRuleOperator::Equals));
        m_valueEdit->setPlaceholderText("输入文件扩展名 (如 png, psd)");
        m_valueEdit->show();
        break;

    case SmartRuleType::Keyword:
        m_opCombo->addItem("包含", static_cast<int>(SmartRuleOperator::Contains));
        m_opCombo->addItem("不包含", static_cast<int>(SmartRuleOperator::NotContains));
        m_valueEdit->setPlaceholderText("输入关键字");
        m_valueEdit->show();
        break;
    }

    updateGeometry();
    emit rowHeightChanged();
}

SmartFolderRule SmartFolderRuleRow::getRule() const {
    SmartFolderRule rule;
    rule.type = static_cast<SmartRuleType>(m_typeCombo->currentData().toInt());
    rule.op = static_cast<SmartRuleOperator>(m_opCombo->currentData().toInt());

    if (rule.type == SmartRuleType::Tag) {
        rule.tags = m_tagContainer->tags();
    } else if (m_valueCombo->isVisible()) {
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

    if (rule.type == SmartRuleType::Tag) {
        m_tagContainer->setTags(rule.tags);
    } else if (m_valueCombo->isVisible()) {
        int valIdx = m_valueCombo->findData(rule.value);
        if (valIdx >= 0) m_valueCombo->setCurrentIndex(valIdx);
    } else {
        m_valueEdit->setText(rule.value);
    }
    updateGeometry();
    emit rowHeightChanged();
}

} // namespace QuarkMeta
