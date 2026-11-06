#pragma once

#include <QDialog>
#include "../core/SmartFolderRecord.h"

namespace RapidNotes {

class SmartFolderDialog : public QDialog {
    Q_OBJECT
public:
    explicit SmartFolderDialog(QWidget* parent = nullptr) : QDialog(parent) {}
    explicit SmartFolderDialog(const SmartFolderRecord& record, QWidget* parent = nullptr) : QDialog(parent), m_record(record) {}
    ~SmartFolderDialog() override = default;

    SmartFolderRecord getRecord() const { return m_record; }

signals:
    void rulesPreviewRequested(const SmartFolderRecord& record);

private:
    SmartFolderRecord m_record;
};

} // namespace RapidNotes
