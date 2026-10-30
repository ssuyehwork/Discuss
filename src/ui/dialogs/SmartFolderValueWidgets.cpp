#include "SmartFolderValueWidgets.h"
#include "../UiHelper.h"
#include <QPainter>
#include <QPixmap>
#include <QDateTime>

namespace QuarkMeta {

// --- 1. EmptyValueWidget ---
EmptyValueWidget::EmptyValueWidget(QWidget* parent) : ISmartFolderValueWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
}

void EmptyValueWidget::saveToRule(SmartFolderRule& rule) const {
    rule.value.clear();
}

void EmptyValueWidget::loadFromRule(const SmartFolderRule& rule) {
    Q_UNUSED(rule);
}

// --- 2. TextValueWidget ---
TextValueWidget::TextValueWidget(QWidget* parent) : ISmartFolderValueWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_lineEdit = new QLineEdit(this);
    layout->addWidget(m_lineEdit);

    connect(m_lineEdit, &QLineEdit::textChanged, this, &TextValueWidget::valueChanged);
}

void TextValueWidget::setPlaceholderText(const QString& text) {
    if (m_lineEdit) m_lineEdit->setPlaceholderText(text);
}

void TextValueWidget::saveToRule(SmartFolderRule& rule) const {
    rule.value = m_lineEdit->text().trimmed();
}

void TextValueWidget::loadFromRule(const SmartFolderRule& rule) {
    m_lineEdit->setText(rule.value);
}

// --- 3. StarRatingValueWidget ---
StarRatingValueWidget::StarRatingValueWidget(QWidget* parent) : ISmartFolderValueWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_ratingCombo = new QComboBox(this);
    m_ratingCombo->addItem("无", "0");

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
        m_ratingCombo->addItem(QIcon(pix), "", QString::number(r));
    }
    m_ratingCombo->setIconSize(QSize(maxW, starSize));

    layout->addWidget(m_ratingCombo);

    connect(m_ratingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StarRatingValueWidget::valueChanged);
}

void StarRatingValueWidget::saveToRule(SmartFolderRule& rule) const {
    rule.value = m_ratingCombo->currentData().toString();
}

void StarRatingValueWidget::loadFromRule(const SmartFolderRule& rule) {
    int idx = m_ratingCombo->findData(rule.value);
    if (idx >= 0) {
        m_ratingCombo->setCurrentIndex(idx);
    } else {
        m_ratingCombo->setCurrentIndex(0);
    }
}

// --- 4. ColorValueWidget ---
ColorValueWidget::ColorValueWidget(QWidget* parent) : ISmartFolderValueWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_colorEdit = new QLineEdit(this);
    m_colorEdit->setPlaceholderText("选择或输入颜色 hex (如 #FF0000)");
    layout->addWidget(m_colorEdit);

    connect(m_colorEdit, &QLineEdit::textChanged, this, &ColorValueWidget::valueChanged);
}

void ColorValueWidget::saveToRule(SmartFolderRule& rule) const {
    rule.value = m_colorEdit->text().trimmed();
}

void ColorValueWidget::loadFromRule(const SmartFolderRule& rule) {
    m_colorEdit->setText(rule.value);
}

// --- 5. SingleDateValueWidget ---
SingleDateValueWidget::SingleDateValueWidget(QWidget* parent) : ISmartFolderValueWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_dateEdit = new QDateEdit(QDate::currentDate(), this);
    m_dateEdit->setDisplayFormat("yyyy-MM-dd");
    m_dateEdit->setCalendarPopup(true);
    layout->addWidget(m_dateEdit);

    connect(m_dateEdit, &QDateEdit::dateChanged, this, &SingleDateValueWidget::valueChanged);
}

void SingleDateValueWidget::saveToRule(SmartFolderRule& rule) const {
    rule.date1 = m_dateEdit->date().toString("yyyy-MM-dd");
    rule.value = rule.date1;
}

void SingleDateValueWidget::loadFromRule(const SmartFolderRule& rule) {
    QDate d = QDate::fromString(rule.date1, "yyyy-MM-dd");
    if (!d.isValid()) {
        d = QDate::fromString(rule.value.trimmed(), "yyyy-MM-dd");
    }
    if (!d.isValid()) {
        d = QDate::currentDate();
    }
    m_dateEdit->setDate(d);
}

// --- 6. DateRangeValueWidget ---
DateRangeValueWidget::DateRangeValueWidget(QWidget* parent) : ISmartFolderValueWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    m_startDateEdit = new QDateEdit(QDate::currentDate(), this);
    m_startDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_startDateEdit->setCalendarPopup(true);

    m_toLabel = new QLabel("至", this);

    m_endDateEdit = new QDateEdit(QDate::currentDate(), this);
    m_endDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_endDateEdit->setCalendarPopup(true);

    layout->addWidget(m_startDateEdit, 1);
    layout->addWidget(m_toLabel);
    layout->addWidget(m_endDateEdit, 1);

    connect(m_startDateEdit, &QDateEdit::dateChanged, this, &DateRangeValueWidget::valueChanged);
    connect(m_endDateEdit, &QDateEdit::dateChanged, this, &DateRangeValueWidget::valueChanged);
}

void DateRangeValueWidget::saveToRule(SmartFolderRule& rule) const {
    rule.date1 = m_startDateEdit->date().toString("yyyy-MM-dd");
    rule.date2 = m_endDateEdit->date().toString("yyyy-MM-dd");
    rule.value = rule.date1;
}

void DateRangeValueWidget::loadFromRule(const SmartFolderRule& rule) {
    QDate d1 = QDate::fromString(rule.date1, "yyyy-MM-dd");
    if (!d1.isValid()) d1 = QDate::currentDate();
    QDate d2 = QDate::fromString(rule.date2, "yyyy-MM-dd");
    if (!d2.isValid()) d2 = d1;

    m_startDateEdit->setDate(d1);
    m_endDateEdit->setDate(d2);
}

// --- 7. RecentDaysValueWidget ---
RecentDaysValueWidget::RecentDaysValueWidget(QWidget* parent) : ISmartFolderValueWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    m_daysSpin = new QSpinBox(this);
    m_daysSpin->setRange(1, 99999);
    m_daysSpin->setValue(7);

    m_unitLabel = new QLabel("天内", this);

    layout->addWidget(m_daysSpin, 1);
    layout->addWidget(m_unitLabel);

    connect(m_daysSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &RecentDaysValueWidget::valueChanged);
}

void RecentDaysValueWidget::saveToRule(SmartFolderRule& rule) const {
    rule.days = m_daysSpin->value();
    rule.value = QString::number(rule.days);
}

void RecentDaysValueWidget::loadFromRule(const SmartFolderRule& rule) {
    int d = rule.days;
    if (d <= 0) d = rule.value.toInt();
    if (d <= 0) d = 7;
    m_daysSpin->setValue(d);
}

// --- 8. NumericValueWidget ---
NumericValueWidget::NumericValueWidget(QWidget* parent) : ISmartFolderValueWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    m_num1Spin = new QSpinBox(this);
    m_num1Spin->setRange(0, 2000000000);

    m_toLabel = new QLabel("至", this);
    m_toLabel->hide();

    m_num2Spin = new QSpinBox(this);
    m_num2Spin->setRange(0, 2000000000);
    m_num2Spin->hide();

    m_unitCombo = new QComboBox(this);
    m_unitCombo->addItem("KB");
    m_unitCombo->addItem("MB");
    m_unitCombo->addItem("GB");
    m_unitCombo->hide();

    layout->addWidget(m_num1Spin, 1);
    layout->addWidget(m_toLabel);
    layout->addWidget(m_num2Spin, 1);
    layout->addWidget(m_unitCombo);

    connect(m_num1Spin, QOverload<int>::of(&QSpinBox::valueChanged), this, &NumericValueWidget::valueChanged);
    connect(m_num2Spin, QOverload<int>::of(&QSpinBox::valueChanged), this, &NumericValueWidget::valueChanged);
    connect(m_unitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NumericValueWidget::valueChanged);
}

void NumericValueWidget::setBetweenMode(bool isBetween) {
    m_toLabel->setVisible(isBetween);
    m_num2Spin->setVisible(isBetween);
}

void NumericValueWidget::setFileSizeMode(bool isFileSize) {
    m_unitCombo->setVisible(isFileSize);
}

void NumericValueWidget::saveToRule(SmartFolderRule& rule) const {
    rule.num1 = m_num1Spin->value();
    rule.num2 = m_num2Spin->value();
    rule.value = QString::number(rule.num1);
    if (m_unitCombo->isVisible()) {
        rule.unit = m_unitCombo->currentText();
    }
}

void NumericValueWidget::loadFromRule(const SmartFolderRule& rule) {
    qint64 n1 = rule.num1;
    if (n1 == 0 && !rule.value.isEmpty()) n1 = rule.value.toLongLong();
    m_num1Spin->setValue(static_cast<int>(n1));
    m_num2Spin->setValue(static_cast<int>(rule.num2));

    if (m_unitCombo->isVisible()) {
        int idx = m_unitCombo->findText(rule.unit, Qt::MatchFixedString);
        if (idx >= 0) m_unitCombo->setCurrentIndex(idx);
        else m_unitCombo->setCurrentIndex(0);
    }
}

// --- 9. TagPillValueWidget ---
TagPillValueWidget::TagPillValueWidget(QWidget* parent) : ISmartFolderValueWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_tagContainer = new TagPillContainer(this, TagTriggerSource::SmartFolderDialog);
    layout->addWidget(m_tagContainer);

    connect(m_tagContainer, &TagPillContainer::containerHeightChanged, this, [this]() {
        emit heightChanged();
        emit valueChanged();
    });
}

void TagPillValueWidget::saveToRule(SmartFolderRule& rule) const {
    rule.tags = m_tagContainer->tags();
}

void TagPillValueWidget::loadFromRule(const SmartFolderRule& rule) {
    m_tagContainer->setTags(rule.tags);
}

} // namespace QuarkMeta
