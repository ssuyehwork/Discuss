#include "DriveBarWidget.h"
#include "UiHelper.h"
#include "TagManagerDialog.h"
#include "../core/NavigationService.h"

namespace QuarkMeta {

DriveBarWidget::DriveBarWidget(QWidget* parent)
    : QWidget(parent) {
    setObjectName("DriveBar");
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(42);
    initUi();
}

void DriveBarWidget::initUi() {
    m_driveBarLayout = new QHBoxLayout(this);
    m_driveBarLayout->setContentsMargins(15, 5, 15, 5);
    m_driveBarLayout->setSpacing(8);

    m_btnTagManager = new QPushButton(UiHelper::getIcon("tag", QColor("#1abc9c"), 18), " 标签管理", this);
    m_btnTagManager->setFixedHeight(28);
    m_btnTagManager->setCursor(Qt::PointingHandCursor);
    m_btnTagManager->setObjectName("BtnTagManager");

    connect(m_btnTagManager, &QPushButton::clicked, this, [this]() {
        TagManagerDialog::showDialog(this, NavigationService::instance().currentUrl(), false);
    });

    m_driveBarLayout->addWidget(m_btnTagManager);
    m_driveBarLayout->addStretch();
}

} // namespace QuarkMeta
