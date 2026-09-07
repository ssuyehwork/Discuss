#include "MetaInfoSection.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "../util/ShellHelper.h"
#include <QHBoxLayout>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QCursor>

namespace QuarkMeta {

MetaInfoSection::MetaInfoSection(QWidget* parent) : QWidget(parent) {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(4);

    addInfoRow(m_mainLayout, "类型", lblType);
    addInfoRow(m_mainLayout, "大小", lblSize);
    addInfoRow(m_mainLayout, "尺寸", lblDimensions);
    addInfoRow(m_mainLayout, "创建时间", lblCtime);
    addInfoRow(m_mainLayout, "修改时间", lblMtime);
    addInfoRow(m_mainLayout, "访问时间", lblAtime);
    addInfoRow(m_mainLayout, "保护状态", lblEncrypted);

    // 路径栏
    QWidget* pathBox = new QWidget(this);
    QHBoxLayout* pathLayout = new QHBoxLayout(pathBox);
    pathLayout->setContentsMargins(0, 4, 0, 0);
    pathLayout->setSpacing(4);

    m_pathEdit = new QLineEdit(pathBox);
    m_pathEdit->setReadOnly(true);
    m_pathEdit->setStyleSheet("QLineEdit { background: #252526; border: 1px solid #3C3C3C; border-radius: 4px; color: #CCCCCC; padding: 2px 6px; font-size: 11px; }");

    m_btnCopyPath = new QPushButton(pathBox);
    m_btnCopyPath->setFixedSize(24, 24);
    m_btnCopyPath->setIcon(UiHelper::getIcon("copy", QColor("#EEEEEE"), 14));
    m_btnCopyPath->setCursor(Qt::PointingHandCursor);
    m_btnCopyPath->setStyleSheet("QPushButton { background: #2A2A2A; border: 1px solid #3C3C3C; border-radius: 4px; } QPushButton:hover { background: #333333; }");

    m_btnOpenLocation = new QPushButton(pathBox);
    m_btnOpenLocation->setFixedSize(24, 24);
    m_btnOpenLocation->setIcon(UiHelper::getIcon("folder", QColor("#EEEEEE"), 14));
    m_btnOpenLocation->setCursor(Qt::PointingHandCursor);
    m_btnOpenLocation->setStyleSheet("QPushButton { background: #2A2A2A; border: 1px solid #3C3C3C; border-radius: 4px; } QPushButton:hover { background: #333333; }");

    pathLayout->addWidget(m_pathEdit, 1);
    pathLayout->addWidget(m_btnCopyPath);
    pathLayout->addWidget(m_btnOpenLocation);

    m_mainLayout->addWidget(pathBox);

    connect(m_btnCopyPath, &QPushButton::clicked, this, [this]() {
        if (!m_pathEdit->text().isEmpty()) {
            QApplication::clipboard()->setText(QDir::toNativeSeparators(m_pathEdit->text()));
            ToolTipOverlay::instance()->showText(QCursor::pos(), "路径已复制到剪贴板", 1200, QColor("#2ecc71"));
        }
    });

    connect(m_btnOpenLocation, &QPushButton::clicked, this, [this]() {
        if (!m_pathEdit->text().isEmpty()) {
            ShellHelper::openInExplorer(m_pathEdit->text());
        }
    });
}

void MetaInfoSection::addInfoRow(QVBoxLayout* layout, const QString& label, QLabel*& valueLabel) {
    QWidget* row = new QWidget(this);
    QHBoxLayout* hl = new QHBoxLayout(row);
    hl->setContentsMargins(0, 0, 0, 0);
    hl->setSpacing(8);

    QLabel* lbl = new QLabel(label, row);
    lbl->setStyleSheet("color: #888888; font-size: 11px;");
    lbl->setFixedWidth(60);

    valueLabel = new QLabel("-", row);
    valueLabel->setStyleSheet("color: #CCCCCC; font-size: 11px;");

    hl->addWidget(lbl);
    hl->addWidget(valueLabel, 1);
    layout->addWidget(row);
}

void MetaInfoSection::updateInfo(const QString& name, const QString& type, const QString& size,
                                  const QString& ctime, const QString& mtime, const QString& atime,
                                  const QString& path, bool encrypted, int width, int height) {
    Q_UNUSED(name);
    lblType->setText(type);
    lblSize->setText(size);
    lblDimensions->setText((width > 0 && height > 0) ? QString("%1 x %2").arg(width).arg(height) : "-");
    lblCtime->setText(ctime);
    lblMtime->setText(mtime);
    lblAtime->setText(atime);
    lblEncrypted->setText(encrypted ? "已加密保护" : "无加密");
    m_pathEdit->setText(path);
    m_pathEdit->setCursorPosition(0);
}

} // namespace QuarkMeta
