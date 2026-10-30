#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDateEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include "../../meta/SmartFolderDao.h"
#include "../TagPillContainer.h"

namespace QuarkMeta {

/**
 * @brief 智能文件夹值输入小控件统一接口
 */
class ISmartFolderValueWidget : public QWidget {
    Q_OBJECT
public:
    explicit ISmartFolderValueWidget(QWidget* parent = nullptr) : QWidget(parent) {}
    ~ISmartFolderValueWidget() override = default;

    virtual void saveToRule(SmartFolderRule& rule) const = 0;
    virtual void loadFromRule(const SmartFolderRule& rule) = 0;

signals:
    void valueChanged();
    void heightChanged();
};

/**
 * @brief 1. 无输入控件（适用于"文件夹"类型或"为空/不为空"运算符）
 */
class EmptyValueWidget : public ISmartFolderValueWidget {
    Q_OBJECT
public:
    explicit EmptyValueWidget(QWidget* parent = nullptr);
    ~EmptyValueWidget() override = default;

    void saveToRule(SmartFolderRule& rule) const override;
    void loadFromRule(const SmartFolderRule& rule) override;
};

/**
 * @brief 2. 文本输入控件（名称、文件夹名、扩展名、链接、注释等）
 */
class TextValueWidget : public ISmartFolderValueWidget {
    Q_OBJECT
public:
    explicit TextValueWidget(QWidget* parent = nullptr);
    ~TextValueWidget() override = default;

    void setPlaceholderText(const QString& text);
    void saveToRule(SmartFolderRule& rule) const override;
    void loadFromRule(const SmartFolderRule& rule) override;

private:
    QLineEdit* m_lineEdit = nullptr;
};

/**
 * @brief 3. 星级选择控件（评分）
 */
class StarRatingValueWidget : public ISmartFolderValueWidget {
    Q_OBJECT
public:
    explicit StarRatingValueWidget(QWidget* parent = nullptr);
    ~StarRatingValueWidget() override = default;

    void saveToRule(SmartFolderRule& rule) const override;
    void loadFromRule(const SmartFolderRule& rule) override;

private:
    QComboBox* m_ratingCombo = nullptr;
};

/**
 * @brief 4. 颜色输入控件（颜色）
 */
class ColorValueWidget : public ISmartFolderValueWidget {
    Q_OBJECT
public:
    explicit ColorValueWidget(QWidget* parent = nullptr);
    ~ColorValueWidget() override = default;

    void saveToRule(SmartFolderRule& rule) const override;
    void loadFromRule(const SmartFolderRule& rule) override;

private:
    QLineEdit* m_colorEdit = nullptr;
};

/**
 * @brief 5. 单日期选择控件（早于、晚于、等于某一天）
 */
class SingleDateValueWidget : public ISmartFolderValueWidget {
    Q_OBJECT
public:
    explicit SingleDateValueWidget(QWidget* parent = nullptr);
    ~SingleDateValueWidget() override = default;

    void saveToRule(SmartFolderRule& rule) const override;
    void loadFromRule(const SmartFolderRule& rule) override;

private:
    QDateEdit* m_dateEdit = nullptr;
};

/**
 * @brief 6. 日期区间选择控件（介于）
 */
class DateRangeValueWidget : public ISmartFolderValueWidget {
    Q_OBJECT
public:
    explicit DateRangeValueWidget(QWidget* parent = nullptr);
    ~DateRangeValueWidget() override = default;

    void saveToRule(SmartFolderRule& rule) const override;
    void loadFromRule(const SmartFolderRule& rule) override;

private:
    QDateEdit* m_startDateEdit = nullptr;
    QLabel* m_toLabel = nullptr;
    QDateEdit* m_endDateEdit = nullptr;
};

/**
 * @brief 7. 最近 N 天输入控件（最近 N 天）
 */
class RecentDaysValueWidget : public ISmartFolderValueWidget {
    Q_OBJECT
public:
    explicit RecentDaysValueWidget(QWidget* parent = nullptr);
    ~RecentDaysValueWidget() override = default;

    void saveToRule(SmartFolderRule& rule) const override;
    void loadFromRule(const SmartFolderRule& rule) override;

private:
    QSpinBox* m_daysSpin = nullptr;
    QLabel* m_unitLabel = nullptr;
};

/**
 * @brief 8. 数值/区间/单位输入控件（宽度、高度、文件大小）
 */
class NumericValueWidget : public ISmartFolderValueWidget {
    Q_OBJECT
public:
    explicit NumericValueWidget(QWidget* parent = nullptr);
    ~NumericValueWidget() override = default;

    void setBetweenMode(bool isBetween);
    void setFileSizeMode(bool isFileSize);

    void saveToRule(SmartFolderRule& rule) const override;
    void loadFromRule(const SmartFolderRule& rule) override;

private:
    QSpinBox* m_num1Spin = nullptr;
    QLabel* m_toLabel = nullptr;
    QSpinBox* m_num2Spin = nullptr;
    QComboBox* m_unitCombo = nullptr;
};

/**
 * @brief 9. 标签胶囊容器控件（标签）
 */
class TagPillValueWidget : public ISmartFolderValueWidget {
    Q_OBJECT
public:
    explicit TagPillValueWidget(QWidget* parent = nullptr);
    ~TagPillValueWidget() override = default;

    void saveToRule(SmartFolderRule& rule) const override;
    void loadFromRule(const SmartFolderRule& rule) override;

private:
    TagPillContainer* m_tagContainer = nullptr;
};

} // namespace QuarkMeta
