#pragma once

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QAction>
#include "../../meta/SmartFolderDao.h"
#include "../TagPillContainer.h"

namespace QuarkMeta {

class SmartFolderRuleRow : public QWidget {
    Q_OBJECT
public:
    explicit SmartFolderRuleRow(QWidget* parent = nullptr);
    explicit SmartFolderRuleRow(const SmartFolderRule& rule, QWidget* parent = nullptr);
    ~SmartFolderRuleRow() override = default;

    SmartFolderRule getRule() const;
    void setRule(const SmartFolderRule& rule);

signals:
    void removeRequested(SmartFolderRuleRow* row);
    void rowHeightChanged();

private slots:
    void onTypeChanged(int index);

private:
    void initUi();

    QComboBox* m_typeCombo = nullptr;
    QComboBox* m_opCombo = nullptr;
    QLineEdit* m_valueEdit = nullptr;
    QComboBox* m_valueCombo = nullptr;
    TagPillContainer* m_tagContainer = nullptr;
    QPushButton* m_btnDelete = nullptr;
};

} // namespace QuarkMeta
