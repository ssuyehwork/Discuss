#include "MetaPanel.h"
#include "UiHelper.h"
#include "StyleLibrary.h"
#include "ToolTipOverlay.h"
#include "../util/ShellHelper.h"
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QApplication>

namespace QuarkMeta {

MetaPanel::MetaPanel(QWidget* parent) : QFrame(parent) {
    setObjectName("MetadataContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);
    setStyleSheet("MetaPanel { background-color: #1E1E1E; color: #EEEEEE; }");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    initUi();
}

void MetaPanel::initUi() {
    QWidget* header = new QWidget(this);
    header->setObjectName("ContainerHeader");
    header->setFixedHeight(32);
    header->setStyleSheet("QWidget#ContainerHeader { background-color: #252526; border-bottom: 1px solid #333333; }");
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(15, 0, 5, 0);

    QLabel* iconLabel = new QLabel(header);
    iconLabel->setPixmap(UiHelper::getIcon("info", QColor("#378ADD"), 18).pixmap(18, 18));
    headerLayout->addWidget(iconLabel);

    QLabel* titleLabel = new QLabel("元数据", header);
    titleLabel->setStyleSheet("color: #378ADD; font-size: 13px; font-weight: bold; background: transparent; border: none;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    m_mainLayout->addWidget(header);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");

    m_container = new QWidget(m_scrollArea);
    m_containerLayout = new QVBoxLayout(m_container);
    m_containerLayout->setContentsMargins(10, 10, 10, 10);
    m_containerLayout->setSpacing(10);

    // 1. 预览与色板区
    m_previewWidget = new MetaPreviewWidget(m_container);
    connect(m_previewWidget, &MetaPreviewWidget::searchByColor, this, &MetaPanel::searchByColor);
    m_containerLayout->addWidget(m_previewWidget);

    // 2. 文件名区
    m_nameEdit = new ElasticEdit(m_container);
    m_nameEdit->setStyleSheet("QTextEdit { background: #252526; border: 1px solid #3C3C3C; border-radius: 4px; color: #FFFFFF; font-size: 14px; font-weight: bold; padding: 4px 6px; }");
    m_containerLayout->addWidget(m_nameEdit);

    connect(m_nameEdit, &ElasticEdit::returnPressed, this, [this]() {
        if (m_selectedPaths.size() == 1) {
            QString oldPath = m_selectedPaths.first();
            QFileInfo fi(oldPath);
            QString newName = m_nameEdit->toPlainText().trimmed();
            if (!newName.isEmpty() && newName != fi.fileName()) {
                QString newPath = fi.dir().filePath(newName);
                emit renameRequested(oldPath, newPath);
            }
        }
    });

    // 3. 评级色标区
    m_ratingColorWidget = new MetaRatingColorWidget(m_container);
    m_containerLayout->addWidget(m_ratingColorWidget);

    connect(m_ratingColorWidget, &MetaRatingColorWidget::ratingChanged, this, [this](int r) {
        emit metadataChanged(r, L"__NO_CHANGE__");
    });
    connect(m_ratingColorWidget, &MetaRatingColorWidget::colorChanged, this, [this](const std::wstring& c) {
        emit metadataChanged(-1, c);
    });

    // 4. 标签管理区
    m_tagSection = new MetaTagSection(m_container);
    connect(m_tagSection, &MetaTagSection::tagAddRequested, this, &MetaPanel::tagAddRequested);
    connect(m_tagSection, &MetaTagSection::tagRemoveRequested, this, &MetaPanel::tagRemoveRequested);
    connect(m_tagSection, &MetaTagSection::tagsChanged, this, &MetaPanel::tagsChanged);
    m_containerLayout->addWidget(createCollapsibleSection("标签", m_tagSection));

    // 5. 备注区
    m_noteEdit = new ElasticEdit(m_container);
    m_noteEdit->setPlaceholderText("添加备注说明...");
    m_noteEdit->setStyleSheet("QTextEdit { background: #252526; border: 1px solid #3C3C3C; border-radius: 4px; color: #CCCCCC; font-size: 12px; padding: 4px 6px; }");
    m_containerLayout->addWidget(createCollapsibleSection("备注", m_noteEdit));

    connect(m_noteEdit, &ElasticEdit::returnPressed, this, [this]() {
        if (!m_isInternalUpdating) {
            emit noteEdited(m_selectedPaths, m_noteEdit->toPlainText().trimmed());
        }
    });

    // 6. 链接区
    m_linkBox = new QWidget(m_container);
    m_linkBox->setStyleSheet("background: #252526; border: 1px solid #3C3C3C; border-radius: 4px;");
    QHBoxLayout* linkLayout = new QHBoxLayout(m_linkBox);
    linkLayout->setContentsMargins(0, 0, 0, 0);
    linkLayout->setSpacing(0);

    m_linkEdit = new QLineEdit(m_linkBox);
    m_linkEdit->setPlaceholderText("绑定 URL 链接...");
    m_linkEdit->setStyleSheet("QLineEdit { background: transparent; border: none; color: #CCCCCC; font-size: 11px; padding-left: 6px; }");

    m_btnOpenLink = new QPushButton(m_linkBox);
    m_btnOpenLink->setFixedSize(26, 26);
    m_btnOpenLink->setIcon(UiHelper::getIcon("link", QColor("#378ADD"), 14));
    m_btnOpenLink->setCursor(Qt::PointingHandCursor);
    m_btnOpenLink->setStyleSheet("QPushButton { background: transparent; border: none; border-left: 1px solid #3C3C3C; } QPushButton:hover { background: #333333; }");

    linkLayout->addWidget(m_linkEdit, 1);
    linkLayout->addWidget(m_btnOpenLink);

    m_containerLayout->addWidget(createCollapsibleSection("关联链接", m_linkBox));

    connect(m_linkEdit, &QLineEdit::editingFinished, this, [this]() {
        if (!m_isInternalUpdating) {
            emit linkEdited(m_selectedPaths, m_linkEdit->text().trimmed());
        }
    });
    connect(m_btnOpenLink, &QPushButton::clicked, this, [this]() {
        QString url = m_linkEdit->text().trimmed();
        if (!url.isEmpty()) {
            QDesktopServices::openUrl(QUrl(url));
        }
    });

    // 7. 物理属性区
    m_infoSection = new MetaInfoSection(m_container);
    m_containerLayout->addWidget(createCollapsibleSection("文件信息", m_infoSection));

    m_containerLayout->addStretch();
    m_scrollArea->setWidget(m_container);
    m_mainLayout->addWidget(m_scrollArea);

    updateControlsState(false);
}

QWidget* MetaPanel::createCollapsibleSection(const QString& title, QWidget* contentWidget, bool defaultExpanded) {
    QWidget* wrapper = new QWidget(m_container);
    QVBoxLayout* wl = new QVBoxLayout(wrapper);
    wl->setContentsMargins(0, 0, 0, 0);
    wl->setSpacing(4);

    QPushButton* hdr = new QPushButton(title, wrapper);
    hdr->setCheckable(true);
    hdr->setChecked(defaultExpanded);
    hdr->setIcon(UiHelper::getIcon(defaultExpanded ? "chevron_down" : "chevron_right", QColor("#888888"), 12));
    hdr->setStyleSheet("QPushButton { background: transparent; border: none; color: #888888; font-size: 11px; font-weight: bold; text-align: left; } QPushButton:hover { color: #FFFFFF; }");

    wl->addWidget(hdr);
    wl->addWidget(contentWidget);

    connect(hdr, &QPushButton::toggled, this, [hdr, contentWidget](bool checked) {
        contentWidget->setVisible(checked);
        hdr->setIcon(UiHelper::getIcon(checked ? "chevron_down" : "chevron_right", QColor("#888888"), 12));
    });

    return wrapper;
}

void MetaPanel::updateControlsState(bool hasSelection) {
    m_nameEdit->setEnabled(hasSelection && m_selectedPaths.size() == 1);
    m_noteEdit->setEnabled(hasSelection);
    m_linkEdit->setEnabled(hasSelection);
    m_btnOpenLink->setEnabled(hasSelection);
    m_ratingColorWidget->setEnabledState(hasSelection);
}

void MetaPanel::setSelectedPaths(const QStringList& paths) {
    m_selectedPaths = paths;
    m_tagSection->setSelectedPaths(paths);
    updateControlsState(!paths.isEmpty());
}

void MetaPanel::updateInfo(const QString& name, const QString& type, const QString& size,
                           const QString& ctime, const QString& mtime, const QString& atime,
                           const QString& path, bool encrypted, int width, int height) {
    m_isInternalUpdating = true;
    m_nameEdit->setText(name);
    m_infoSection->updateInfo(name, type, size, ctime, mtime, atime, path, encrypted, width, height);
    m_isInternalUpdating = false;
}

void MetaPanel::setImagePreview(const QPixmap& pixmap) {
    m_previewWidget->setImagePreview(pixmap);
}

void MetaPanel::setPalettes(const QVector<QPair<QColor, float>>& palette) {
    m_previewWidget->setPalettes(palette);
}

void MetaPanel::setTags(const QStringList& tags) {
    m_tagSection->setTags(tags, m_selectedPaths);
}

void MetaPanel::setNote(const QString& note) {
    m_isInternalUpdating = true;
    m_noteEdit->setText(note);
    m_isInternalUpdating = false;
}

void MetaPanel::setNote(const std::wstring& note) {
    setNote(QString::fromStdWString(note));
}

void MetaPanel::setURL(const QString& url) {
    m_isInternalUpdating = true;
    m_linkEdit->setText(url);
    m_linkEdit->setCursorPosition(0);
    m_isInternalUpdating = false;
}

void MetaPanel::setURL(const std::wstring& url) {
    setURL(QString::fromStdWString(url));
}

void MetaPanel::setRating(int rating) {
    m_ratingColorWidget->setRating(rating);
}

void MetaPanel::setColor(const std::wstring& color) {
    m_ratingColorWidget->setColor(color);
}

bool MetaPanel::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::ToolTip) {
        QString text = watched->property("tooltipText").toString();
        if (!text.isEmpty()) {
            ToolTipOverlay::instance()->showText(QCursor::pos(), text, 0);
            return true;
        }
    }
    return QFrame::eventFilter(watched, event);
}

void MetaPanel::resizeEvent(QResizeEvent* event) {
    QFrame::resizeEvent(event);
}

void MetaPanel::showEvent(QShowEvent* event) {
    QFrame::showEvent(event);
}

} // namespace QuarkMeta
