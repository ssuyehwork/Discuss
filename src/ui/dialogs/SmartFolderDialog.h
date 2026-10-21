#pragma once

#include "FramelessDialog.h"
#include "../../meta/SmartFolderDao.h"
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

namespace QuarkMeta {

class SmartFolderDialog : public FramelessDialog {
    Q_OBJECT
public:
    explicit SmartFolderDialog(QWidget* parent = nullptr);
    explicit SmartFolderDialog(const SmartFolderRecord& record, QWidget* parent = nullptr);
    ~SmartFolderDialog() override = default;

    SmartFolderRecord getRecord() const;

private slots:
    void onSaveAndAccept();

private:
    void setupUi();

    SmartFolderRecord m_record;

    QLineEdit* m_nameEdit{nullptr};
    QPushButton* m_btnCancel{nullptr};
    QPushButton* m_btnSave{nullptr};
};

} // namespace QuarkMeta
