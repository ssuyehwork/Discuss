#include "FolderSectionWidget.h"
#include <QMouseEvent>
#include <QStyle>

namespace QuarkMeta {

FolderSectionHeaderBar::FolderSectionHeaderBar(QWidget* parent)
    : QFrame(parent)
{
    setObjectName("FolderSectionHeaderBar");
    setFixedHeight(28);
    setCursor(Qt::PointingHandCursor);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(6);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("FolderSectionTitleLabel");
    m_titleLabel->setStyleSheet("color: #CCCCCC; font-size: 12px; font-weight: bold;");

    m_arrowLabel = new QLabel("▾", this);
    m_arrowLabel->setObjectName("FolderSectionArrowLabel");
    m_arrowLabel->setStyleSheet("color: #888888; font-size: 11px;");

    layout->addWidget(m_titleLabel);
    layout->addWidget(m_arrowLabel);
    layout->addStretch();

    updateUi();
}

void FolderSectionHeaderBar::setCount(int count) {
    m_count = count;
    setVisible(count > 0);
    updateUi();
}

void FolderSectionHeaderBar::setCollapsed(bool collapsed) {
    if (m_collapsed == collapsed) return;
    m_collapsed = collapsed;
    updateUi();
    emit collapseToggled(m_collapsed);
}

void FolderSectionHeaderBar::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        setCollapsed(!m_collapsed);
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void FolderSectionHeaderBar::updateUi() {
    if (m_titleLabel) {
        m_titleLabel->setText(QString("子文件夹 (%1)").arg(m_count));
    }
    if (m_arrowLabel) {
        m_arrowLabel->setText(m_collapsed ? "▸" : "▾");
    }
}

// -------------------------------------------------------------

FileSectionHeaderBar::FileSectionHeaderBar(QWidget* parent)
    : QFrame(parent)
{
    setObjectName("FileSectionHeaderBar");
    setFixedHeight(28);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(6);

    m_titleLabel = new QLabel("内容 (0)", this);
    m_titleLabel->setObjectName("FileSectionTitleLabel");
    m_titleLabel->setStyleSheet("color: #888888; font-size: 12px; font-weight: bold;");

    layout->addWidget(m_titleLabel);
    layout->addStretch();
}

void FileSectionHeaderBar::setCount(int count) {
    m_count = count;
    if (m_titleLabel) {
        m_titleLabel->setText(QString("内容 (%1)").arg(count));
    }
}

} // namespace QuarkMeta
