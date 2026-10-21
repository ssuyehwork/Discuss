#include "SmartFolderDialog.h"
#include "../UiHelper.h"

namespace QuarkMeta {

SmartFolderDialog::SmartFolderDialog(QWidget* parent) : FramelessDialog("智能文件夹配置", parent) {
    m_record.name = "新智能文件夹";
    setupUi();
}

SmartFolderDialog::SmartFolderDialog(const SmartFolderRecord& record, QWidget* parent) : FramelessDialog("智能文件夹配置", parent), m_record(record) {
    setupUi();
}

void SmartFolderDialog::setupUi() {
    resize(380, 160);

    QWidget* bodyWidget = getContentArea();
    QVBoxLayout* bodyLayout = new QVBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(20, 15, 20, 20);
    bodyLayout->setSpacing(15);

    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->setSpacing(12);

    QLabel* nameLabel = new QLabel("名称:", bodyWidget);
    nameLabel->setObjectName("SmartFolderLabel");

    m_nameEdit = new QLineEdit(bodyWidget);
    m_nameEdit->setText(m_record.name);

    topLayout->addWidget(nameLabel);
    topLayout->addWidget(m_nameEdit, 1);

    bodyLayout->addLayout(topLayout);

    bodyLayout->addStretch(1);

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

    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_btnSave, &QPushButton::clicked, this, &SmartFolderDialog::onSaveAndAccept);
}

void SmartFolderDialog::onSaveAndAccept() {
    m_record.name = m_nameEdit->text().trimmed();
    if (m_record.name.isEmpty()) {
        m_record.name = "新智能文件夹";
    }
    accept();
}

SmartFolderRecord SmartFolderDialog::getRecord() const {
    return m_record;
}

} // namespace QuarkMeta
