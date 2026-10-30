#include "SmartFolderRuleRow.h"
#include "../UiHelper.h"

namespace QuarkMeta {

struct RuleOperatorItem {
    QString label;
    SmartRuleOperator op;
};

static QList<RuleOperatorItem> getOperatorsForType(SmartRuleType type, bool isNoRating = false) {
    QList<RuleOperatorItem> list;
    switch (type) {
    case SmartRuleType::Rating:
        if (isNoRating) {
            list.append({"等于", SmartRuleOperator::Equals});
        } else {
            list.append({"大于等于", SmartRuleOperator::GreaterOrEqual});
            list.append({"等于", SmartRuleOperator::Equals});
            list.append({"小于等于", SmartRuleOperator::LessOrEqual});
        }
        break;

    case SmartRuleType::Name:
    case SmartRuleType::FolderName:
        list.append({"包含", SmartRuleOperator::Contains});
        list.append({"不包含", SmartRuleOperator::NotContains});
        list.append({"等于", SmartRuleOperator::Equals});
        list.append({"开头是", SmartRuleOperator::StartsWith});
        list.append({"结尾是", SmartRuleOperator::EndsWith});
        break;

    case SmartRuleType::Folder:
        list.append({"是文件夹", SmartRuleOperator::IsFolder});
        list.append({"不是文件夹", SmartRuleOperator::IsNotFolder});
        break;

    case SmartRuleType::AddedDate:
    case SmartRuleType::ModifiedDate:
    case SmartRuleType::CreatedDate:
        list.append({"早于", SmartRuleOperator::Before});
        list.append({"晚于", SmartRuleOperator::After});
        list.append({"等于", SmartRuleOperator::Equals});
        list.append({"介于", SmartRuleOperator::Between});
        list.append({"最近 N 天", SmartRuleOperator::RecentDays});
        break;

    case SmartRuleType::Width:
    case SmartRuleType::Height:
    case SmartRuleType::FileSize:
        list.append({"大于等于", SmartRuleOperator::GreaterOrEqual});
        list.append({"等于", SmartRuleOperator::Equals});
        list.append({"小于等于", SmartRuleOperator::LessOrEqual});
        list.append({"介于", SmartRuleOperator::Between});
        break;

    case SmartRuleType::Link:
    case SmartRuleType::Note:
        list.append({"包含", SmartRuleOperator::Contains});
        list.append({"不包含", SmartRuleOperator::NotContains});
        list.append({"为空", SmartRuleOperator::IsEmpty});
        list.append({"不为空", SmartRuleOperator::IsNotEmpty});
        break;

    case SmartRuleType::Color:
    case SmartRuleType::Extension:
        list.append({"等于", SmartRuleOperator::Equals});
        list.append({"不包含", SmartRuleOperator::NotContains});
        break;

    case SmartRuleType::Tag:
        list.append({"包含", SmartRuleOperator::Contains});
        list.append({"不包含", SmartRuleOperator::NotContains});
        break;
    }
    return list;
}

static ISmartFolderValueWidget* createValueWidgetForTypeAndOp(SmartRuleType type, SmartRuleOperator op, QWidget* parent) {
    if (op == SmartRuleOperator::IsEmpty || op == SmartRuleOperator::IsNotEmpty || type == SmartRuleType::Folder) {
        return new EmptyValueWidget(parent);
    }

    switch (type) {
    case SmartRuleType::Rating:
        return new StarRatingValueWidget(parent);
    case SmartRuleType::Name: {
        auto* w = new TextValueWidget(parent);
        w->setPlaceholderText("输入匹配名称");
        return w;
    }
    case SmartRuleType::FolderName: {
        auto* w = new TextValueWidget(parent);
        w->setPlaceholderText("输入匹配文件夹名称");
        return w;
    }
    case SmartRuleType::Extension: {
        auto* w = new TextValueWidget(parent);
        w->setPlaceholderText("输入文件扩展名 (如 png, psd)");
        return w;
    }
    case SmartRuleType::Link: {
        auto* w = new TextValueWidget(parent);
        w->setPlaceholderText("输入链接关键字");
        return w;
    }
    case SmartRuleType::Note: {
        auto* w = new TextValueWidget(parent);
        w->setPlaceholderText("输入注释关键字");
        return w;
    }
    case SmartRuleType::Color:
        return new ColorValueWidget(parent);
    case SmartRuleType::Tag:
        return new TagPillValueWidget(parent);
    case SmartRuleType::AddedDate:
    case SmartRuleType::ModifiedDate:
    case SmartRuleType::CreatedDate:
        if (op == SmartRuleOperator::Between) {
            return new DateRangeValueWidget(parent);
        } else if (op == SmartRuleOperator::RecentDays) {
            return new RecentDaysValueWidget(parent);
        } else {
            return new SingleDateValueWidget(parent);
        }
    case SmartRuleType::Width:
    case SmartRuleType::Height: {
        auto* w = new NumericValueWidget(parent);
        w->setBetweenMode(op == SmartRuleOperator::Between);
        w->setFileSizeMode(false);
        return w;
    }
    case SmartRuleType::FileSize: {
        auto* w = new NumericValueWidget(parent);
        w->setBetweenMode(op == SmartRuleOperator::Between);
        w->setFileSizeMode(true);
        return w;
    }
    default:
        return new EmptyValueWidget(parent);
    }
}

SmartFolderRuleRow::SmartFolderRuleRow(QWidget* parent) : QWidget(parent) {
    initUi();
}

SmartFolderRuleRow::SmartFolderRuleRow(const SmartFolderRule& rule, QWidget* parent) : QWidget(parent) {
    initUi();
    setRule(rule);
}

void SmartFolderRuleRow::initUi() {
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 2, 0, 2);
    m_layout->setSpacing(8);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem("评分", static_cast<int>(SmartRuleType::Rating));
    m_typeCombo->addItem("名称", static_cast<int>(SmartRuleType::Name));
    m_typeCombo->addItem("扩展名", static_cast<int>(SmartRuleType::Extension));
    m_typeCombo->addItem("标签", static_cast<int>(SmartRuleType::Tag));
    m_typeCombo->addItem("文件夹", static_cast<int>(SmartRuleType::Folder));
    m_typeCombo->addItem("文件夹名", static_cast<int>(SmartRuleType::FolderName));
    m_typeCombo->addItem("颜色", static_cast<int>(SmartRuleType::Color));
    m_typeCombo->addItem("添加日期", static_cast<int>(SmartRuleType::AddedDate));
    m_typeCombo->addItem("修改日期", static_cast<int>(SmartRuleType::ModifiedDate));
    m_typeCombo->addItem("创建日期", static_cast<int>(SmartRuleType::CreatedDate));
    m_typeCombo->addItem("链接", static_cast<int>(SmartRuleType::Link));
    m_typeCombo->addItem("注释", static_cast<int>(SmartRuleType::Note));
    m_typeCombo->addItem("宽度", static_cast<int>(SmartRuleType::Width));
    m_typeCombo->addItem("高度", static_cast<int>(SmartRuleType::Height));
    m_typeCombo->addItem("文件大小", static_cast<int>(SmartRuleType::FileSize));

    m_opCombo = new QComboBox(this);

    m_btnDelete = new QPushButton(this);
    m_btnDelete->setFixedSize(24, 24);
    m_btnDelete->setIcon(UiHelper::getIcon("close", QColor("#CCCCCC"), 14));
    m_btnDelete->setObjectName("SmartFolderRowDeleteBtn");

    m_layout->addWidget(m_typeCombo);
    m_layout->addWidget(m_opCombo);

    // 先实例化一个 EmptyValueWidget 占位
    m_valueWidget = new EmptyValueWidget(this);
    m_layout->addWidget(m_valueWidget, 1);
    m_layout->addWidget(m_btnDelete);

    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SmartFolderRuleRow::onTypeChanged);
    connect(m_opCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SmartFolderRuleRow::onOpChanged);
    connect(m_btnDelete, &QPushButton::clicked, this, [this]() {
        emit removeRequested(this);
    });

    rebuildOperators();
    rebuildValueWidget();
}

void SmartFolderRuleRow::rebuildOperators(const SmartFolderRule& presetRule) {
    SmartRuleType type = static_cast<SmartRuleType>(m_typeCombo->currentData().toInt());
    bool isNoRating = false;

    if (type == SmartRuleType::Rating) {
        if (!presetRule.value.isEmpty()) {
            isNoRating = (presetRule.value == "0" || presetRule.value == "无");
        } else if (m_valueWidget) {
            SmartFolderRule r;
            m_valueWidget->saveToRule(r);
            isNoRating = (r.value == "0" || r.value == "无");
        }
    }

    QList<RuleOperatorItem> opItems = getOperatorsForType(type, isNoRating);

    int curOpVal = m_opCombo->currentData().isValid() ? m_opCombo->currentData().toInt() : -1;
    m_opCombo->blockSignals(true);
    m_opCombo->clear();
    int selectIdx = 0;
    for (int i = 0; i < opItems.size(); ++i) {
        m_opCombo->addItem(opItems[i].label, static_cast<int>(opItems[i].op));
        if (static_cast<int>(opItems[i].op) == curOpVal) {
            selectIdx = i;
        }
    }
    m_opCombo->setCurrentIndex(selectIdx);
    m_opCombo->blockSignals(false);
}

void SmartFolderRuleRow::rebuildValueWidget() {
    SmartRuleType type = static_cast<SmartRuleType>(m_typeCombo->currentData().toInt());
    SmartRuleOperator op = static_cast<SmartRuleOperator>(m_opCombo->currentData().toInt());

    // 记住旧规则
    SmartFolderRule oldRule;
    if (m_valueWidget) {
        m_valueWidget->saveToRule(oldRule);
        m_layout->removeWidget(m_valueWidget);
        m_valueWidget->deleteLater();
        m_valueWidget = nullptr;
    }

    m_valueWidget = createValueWidgetForTypeAndOp(type, op, this);
    // 放入删除按钮之前
    m_layout->insertWidget(m_layout->count() - 1, m_valueWidget, 1);

    connect(m_valueWidget, &ISmartFolderValueWidget::valueChanged, this, &SmartFolderRuleRow::onValueWidgetChanged);
    connect(m_valueWidget, &ISmartFolderValueWidget::heightChanged, this, [this]() {
        updateGeometry();
        emit rowHeightChanged();
    });

    if (!oldRule.value.isEmpty() || !oldRule.tags.isEmpty() || !oldRule.date1.isEmpty()) {
        m_valueWidget->loadFromRule(oldRule);
    }

    updateGeometry();
    emit rowHeightChanged();
}

void SmartFolderRuleRow::onTypeChanged(int index) {
    Q_UNUSED(index);
    rebuildOperators();
    rebuildValueWidget();
}

void SmartFolderRuleRow::onOpChanged(int index) {
    Q_UNUSED(index);
    rebuildValueWidget();
}

void SmartFolderRuleRow::onValueWidgetChanged() {
    SmartRuleType type = static_cast<SmartRuleType>(m_typeCombo->currentData().toInt());
    if (type == SmartRuleType::Rating) {
        SmartFolderRule r;
        m_valueWidget->saveToRule(r);
        bool isNoRating = (r.value == "0" || r.value == "无");
        int curOp = m_opCombo->currentData().toInt();

        rebuildOperators(r);

        if (isNoRating && curOp != static_cast<int>(SmartRuleOperator::Equals)) {
            rebuildValueWidget();
        }
    }
}

SmartFolderRule SmartFolderRuleRow::getRule() const {
    SmartFolderRule rule;
    rule.type = static_cast<SmartRuleType>(m_typeCombo->currentData().toInt());
    rule.op = static_cast<SmartRuleOperator>(m_opCombo->currentData().toInt());
    if (m_valueWidget) {
        m_valueWidget->saveToRule(rule);
    }
    return rule;
}

void SmartFolderRuleRow::setRule(const SmartFolderRule& rule) {
    int typeIdx = m_typeCombo->findData(static_cast<int>(rule.type));
    if (typeIdx >= 0) {
        m_typeCombo->blockSignals(true);
        m_typeCombo->setCurrentIndex(typeIdx);
        m_typeCombo->blockSignals(false);
    }

    rebuildOperators(rule);

    int opIdx = m_opCombo->findData(static_cast<int>(rule.op));
    if (opIdx >= 0) {
        m_opCombo->blockSignals(true);
        m_opCombo->setCurrentIndex(opIdx);
        m_opCombo->blockSignals(false);
    }

    rebuildValueWidget();

    if (m_valueWidget) {
        m_valueWidget->loadFromRule(rule);
    }
}

} // namespace QuarkMeta
