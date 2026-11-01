#pragma once

#include "FramelessDialog.h"
#include "../../meta/SmartFolderDao.h"
#include "../TagSelectorOverlay.h"
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QEvent>

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

private slots:
    void onTypeChanged(int index);

private:
    void setupUi();
    void updateOperatorAndValueControls();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    QComboBox* m_typeCombo{nullptr};
    QComboBox* m_opCombo{nullptr};
    QLineEdit* m_valueEdit{nullptr};
    QComboBox* m_valueCombo{nullptr};
    QPushButton* m_btnDelete{nullptr};
    QAction* m_tagAction{nullptr};
    QPointer<TagSelectorOverlay> m_tagOverlay;

    void openTagSelector();
};

class SmartFolderDialog : public FramelessDialog {
    Q_OBJECT
public:
    explicit SmartFolderDialog(QWidget* parent = nullptr);
    explicit SmartFolderDialog(const SmartFolderRecord& record, QWidget* parent = nullptr);
    ~SmartFolderDialog() override = default;

    SmartFolderRecord getRecord() const;

private slots:
    void onAddRule();
    void onRemoveRule(SmartFolderRuleRow* row);
    void onSaveAndAccept();

private:
    void setupUi();

    SmartFolderRecord m_record;

    QLineEdit* m_nameEdit{nullptr};
    QComboBox* m_matchModeCombo{nullptr};
    QVBoxLayout* m_rulesLayout{nullptr};
    QList<SmartFolderRuleRow*> m_ruleRows;

    QPushButton* m_btnAddRule{nullptr};
    QPushButton* m_btnCancel{nullptr};
    QPushButton* m_btnSave{nullptr};
};

} // namespace QuarkMeta
