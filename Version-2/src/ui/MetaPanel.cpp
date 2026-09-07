#include "MetaPanel.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "components/FlowLayout.h"
#include "../util/ShellHelper.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QLabel>
#include <QDir>
#include <QFile>
#include <QApplication>
#include <QScreen>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QCursor>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QPainter>
#include <QPainterPath>

namespace QuarkMeta {

MetaPanel::MetaPanel(QWidget* parent) : QFrame(parent) {
    setObjectName("MetadataContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);
    setStyleSheet("color: #EEEEEE;");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    m_adjustTimer = new QTimer(this);
    m_adjustTimer->setSingleShot(true);
    m_adjustTimer->setInterval(50);
    connect(m_adjustTimer, &QTimer::timeout, this, &MetaPanel::adjustFlowHeights);

    setContextMenuPolicy(Qt::CustomContextMenu);
    initUi();
}

QWidget* MetaPanel::createCollapsibleSection(const QString& title, QWidget* contentWidget, bool defaultExpanded) {
    QWidget* sectionWidget = new QWidget(m_container);
    QVBoxLayout* sectionLayout = new QVBoxLayout(sectionWidget);
    sectionLayout->setContentsMargins(0, 0, 0, 0);
    sectionLayout->setSpacing(4);

    QPushButton* btnHeader = new QPushButton(sectionWidget);
    btnHeader->setFixedHeight(22);
    btnHeader->setCursor(Qt::PointingHandCursor);
    btnHeader->setStyleSheet(
        "QPushButton { border: none; background: transparent; color: #888888; font-size: 11px; font-weight: bold; text-align: left; padding: 0; }"
        "QPushButton:hover { color: #FFFFFF; }"
    );

    auto updateHeaderText = [btnHeader, title](bool expanded) {
        btnHeader->setIcon(UiHelper::getIcon(expanded ? "chevron_down" : "chevron_right", QColor("#888888"), 12));
        btnHeader->setIconSize(QSize(12, 12));
        btnHeader->setText(" " + title);
    };

    updateHeaderText(defaultExpanded);
    contentWidget->setVisible(defaultExpanded);

    connect(btnHeader, &QPushButton::clicked, this, [contentWidget, updateHeaderText, btnHeader, this]() {
        bool nowVisible = !contentWidget->isVisible();
        contentWidget->setVisible(nowVisible);
        updateHeaderText(nowVisible);
        adjustFlowHeights();
        if (m_container) m_container->adjustSize();
    });

    sectionLayout->addWidget(btnHeader);
    sectionLayout->addWidget(contentWidget);
    return sectionWidget;
}

void MetaPanel::initUi() {
    QWidget* header = new QWidget(this);
    header->setObjectName("ContainerHeader");
    header->setFixedHeight(32);
    header->setStyleSheet("QWidget#ContainerHeader { background-color: #252526; border-bottom: 1px solid #333333; }");
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 0, 8, 0);
    headerLayout->setSpacing(6);
    
    QLabel* iconLabel = new QLabel(header);
    iconLabel->setPixmap(UiHelper::getIcon("all_data", QColor("#4a90e2"), 16).pixmap(16, 16));
    headerLayout->addWidget(iconLabel);
    
    QLabel* titleLabel = new QLabel("元数据属性", header);
    titleLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #4a90e2; background: transparent; border: none;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    m_mainLayout->addWidget(header);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");
    
    m_container = new QWidget(m_scrollArea);
    m_containerLayout = new QVBoxLayout(m_container);
    m_containerLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    m_containerLayout->setContentsMargins(8, 8, 8, 8);
    m_containerLayout->setSpacing(8);

    // 1. 顶部预览与色板区
    m_topPreviewBox = new QWidget(m_container);
    m_topPreviewBox->setObjectName("TopPreviewBox");
    m_topPreviewBox->setStyleSheet("QWidget#TopPreviewBox { background: transparent; border: none; }");
    QVBoxLayout* previewLayout = new QVBoxLayout(m_topPreviewBox);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(6);

    m_lblImagePreview = new QLabel(m_topPreviewBox);
    m_lblImagePreview->setAlignment(Qt::AlignCenter);
    m_lblImagePreview->setObjectName("MetaImagePreview");
    m_lblImagePreview->setStyleSheet("background: transparent; border: none;");
    m_lblImagePreview->hide();
    previewLayout->addWidget(m_lblImagePreview, 0, Qt::AlignHCenter);

    m_paletteContainer = new QWidget(m_topPreviewBox);
    m_paletteFlowLayout = new FlowLayout(m_paletteContainer, 0, 4, 4);
    previewLayout->addWidget(m_paletteContainer);

    m_topPreviewBox->hide();
    m_containerLayout->addWidget(m_topPreviewBox);

    // 2. 文件名编辑区
    m_nameEdit = new ElasticEdit(m_container);
    m_nameEdit->setPlaceholderText("文件名...");
    m_nameEdit->setStyleSheet(
        "QTextEdit { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; padding: 4px 8px; "
        "font-size: 13px; font-weight: bold; color: #FFFFFF; }"
        "QTextEdit:focus { border-color: #378ADD; }"
        "QTextEdit:disabled { background: #1E1E1E; color: #777777; border-color: #2A2A2A; }"
    );
    m_nameEdit->installEventFilter(this);
    m_containerLayout->addWidget(m_nameEdit);

    // 3. 备注说明区
    m_noteEdit = new ElasticEdit(m_container);
    m_noteEdit->setPlaceholderText("添加备注说明...");
    m_noteEdit->setStyleSheet(
        "QTextEdit { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; padding: 4px 8px; font-size: 12px; color: #AAAAAA; }"
        "QTextEdit:focus { border-color: #378ADD; color: #FFFFFF; }"
        "QTextEdit:disabled { background: #1E1E1E; color: #555555; }"
    );
    m_noteEdit->installEventFilter(this);
    m_containerLayout->addWidget(createCollapsibleSection("备注说明", m_noteEdit, true));

    // 4. 关联网址区
    m_linkEdit = new QLineEdit(m_container);
    m_linkEdit->setPlaceholderText("添加关联网址...");
    m_linkEdit->setFixedHeight(28);
    m_linkEdit->setStyleSheet(
        "QLineEdit { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; padding-left: 8px; font-size: 12px; color: #378ADD; }"
        "QLineEdit:focus { border-color: #378ADD; }"
        "QLineEdit:disabled { background: #1E1E1E; color: #555555; }"
    );
    m_linkEdit->installEventFilter(this);

    m_actOpenLink = m_linkEdit->addAction(UiHelper::getIcon("link", QColor("#378ADD"), 14), QLineEdit::TrailingPosition);
    m_actOpenLink->setVisible(false);

    for (QToolButton* btn : m_linkEdit->findChildren<QToolButton*>()) {
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QToolButton { border: none; border-left: 1px solid #3c3c3c; background: transparent; padding-left: 4px; padding-right: 4px; }"
            "QToolButton:hover { background: #3E3E42; }"
        );
    }

    connect(m_actOpenLink, &QAction::triggered, this, [this]() {
        QString urlStr = m_linkEdit->text().trimmed();
        if (!urlStr.isEmpty()) {
            QUrl url = QUrl::fromUserInput(urlStr);
            if (url.isValid() && (url.scheme().toLower() == "http" || url.scheme().toLower() == "https")) {
                QDesktopServices::openUrl(url);
            } else {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "无效的 Web 网址格式", 1500, QColor("#e81123"));
            }
        }
    });

    connect(m_linkEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        bool hasText = !text.trimmed().isEmpty();
        if (m_actOpenLink) m_actOpenLink->setVisible(hasText);
    });

    m_containerLayout->addWidget(createCollapsibleSection("关联网址", m_linkEdit, true));

    // 5. 星级评级 + 颜色色标条 (严格采用 Hex 物理色值)
    m_ratingColorBox = new QWidget(m_container);
    QVBoxLayout* ratingColorLayout = new QVBoxLayout(m_ratingColorBox);
    ratingColorLayout->setContentsMargins(0, 2, 0, 2);
    ratingColorLayout->setSpacing(6);

    QWidget* ratingRow = new QWidget(m_ratingColorBox);
    ratingRow->setStyleSheet("QWidget { background: transparent; border: none; }");
    QHBoxLayout* starLayout = new QHBoxLayout(ratingRow);
    starLayout->setContentsMargins(0, 2, 0, 2);
    starLayout->setSpacing(2);

    // 清除评级按钮 ⊘：完全保持原样（22x22px，图标 16px）
    QPushButton* btnClearStar = new QPushButton(ratingRow);
    btnClearStar->setFixedSize(22, 22);
    btnClearStar->setCursor(Qt::PointingHandCursor);
    btnClearStar->setIcon(UiHelper::getIcon("no_color", QColor("#888888"), 16));
    btnClearStar->setIconSize(QSize(16, 16));
    btnClearStar->setProperty("tooltipText", "清除评级");
    btnClearStar->installEventFilter(this);
    btnClearStar->setStyleSheet("QPushButton { border: none; background: transparent; } QPushButton:hover { background: #333333; border-radius: 4px; }");
    connect(btnClearStar, &QPushButton::clicked, this, [this]() { setRating(0, true); });
    starLayout->addWidget(btnClearStar);

    // 🚀 五角星按钮：仅按您的要求减小 2 像素（外框 20x20px，图标 16px）
    for (int i = 1; i <= 5; ++i) {
        QPushButton* btnStar = new QPushButton(ratingRow);
        btnStar->setFixedSize(20, 20); // 👈 减小 2 像素
        btnStar->setCursor(Qt::PointingHandCursor);
        btnStar->setIcon(UiHelper::getIcon("star", QColor("#555555"), 16)); // 👈 减小 2 像素
        btnStar->setIconSize(QSize(16, 16)); // 👈 减小 2 像素
        btnStar->setStyleSheet("QPushButton { border: none; background: transparent; } QPushButton:hover { background: #333333; border-radius: 3px; }");
        connect(btnStar, &QPushButton::clicked, this, [this, i]() {
            int newRating = (m_currentRating == i) ? 0 : i;
            setRating(newRating, true);
        });
        m_starBtns.append(btnStar);
        starLayout->addWidget(btnStar);
    }
    starLayout->addStretch();
    ratingColorLayout->addWidget(ratingRow);

    QWidget* colorRow = new QWidget(m_ratingColorBox);
    colorRow->setStyleSheet("QWidget { background: transparent; border: none; }");
    QHBoxLayout* colorLayout = new QHBoxLayout(colorRow);
    colorLayout->setContentsMargins(0, 2, 0, 2);
    colorLayout->setSpacing(6);

    QPushButton* btnNoColor = new QPushButton(colorRow);
    btnNoColor->setFixedSize(22, 22);
    btnNoColor->setCursor(Qt::PointingHandCursor);
    btnNoColor->setIcon(UiHelper::getIcon("no_color", QColor("#888888"), 16));
    btnNoColor->setIconSize(QSize(16, 16));
    btnNoColor->setProperty("tooltipText", "无色标");
    btnNoColor->installEventFilter(this);
    btnNoColor->setStyleSheet("QPushButton { border: none; background: transparent; } QPushButton:hover { background: #333333; border-radius: 4px; }");
    connect(btnNoColor, &QPushButton::clicked, this, [this]() { setColor(QString(""), true); });
    colorLayout->addWidget(btnNoColor);

    static const QVector<QPair<QString, QString>> s_colorMap = {
        {"红色", "#E24B4A"}, {"橙色", "#EF9F27"}, {"黄色", "#FECF0E"}, {"绿色", "#639922"},
        {"青色", "#1D9E75"}, {"蓝色", "#378ADD"}, {"紫色", "#7F77DD"}, {"灰色", "#5F5E5A"}
    };

    for (const auto& pair : s_colorMap) {
        QPushButton* btnColor = new QPushButton(colorRow);
        btnColor->setFixedSize(16, 16);
        btnColor->setCursor(Qt::PointingHandCursor);
        btnColor->setProperty("tooltipText", pair.first);
        btnColor->setProperty("hexColor", pair.second);
        btnColor->installEventFilter(this);
        btnColor->setStyleSheet(QString(
            "QPushButton { background-color: %1; border: 1px solid transparent; border-radius: 8px; }"
            "QPushButton:hover { border-color: #FFFFFF; }"
        ).arg(pair.second));

        QString hex = pair.second;
        connect(btnColor, &QPushButton::clicked, this, [this, hex]() {
            if (m_currentColorHex.compare(hex, Qt::CaseInsensitive) == 0) {
                setColor(QString(""), true);
            } else {
                setColor(hex, true);
            }
        });
        m_colorBtns.append(btnColor);
        colorLayout->addWidget(btnColor);
    }
    colorLayout->addStretch();
    ratingColorLayout->addWidget(colorRow);
    m_containerLayout->addWidget(m_ratingColorBox);

    // 6. 标签管理区
    m_tagBox = new QWidget(m_container);
    QVBoxLayout* tagL = new QVBoxLayout(m_tagBox);
    tagL->setContentsMargins(0, 0, 0, 0);
    tagL->setSpacing(6);

    m_btnAddTagBig = new QPushButton(UiHelper::getIcon("add", QColor("#AAAAAA"), 14), " 添加标签", m_tagBox);
    m_btnAddTagBig->setFixedHeight(28);
    m_btnAddTagBig->setCursor(Qt::PointingHandCursor);
    m_btnAddTagBig->setStyleSheet(
        "QPushButton { background-color: #252526; border: 1px solid #3c3c3c; border-radius: 4px; padding: 0 10px; color: #AAAAAA; font-size: 12px; text-align: center; }"
        "QPushButton:hover { background-color: #2a2d2e; border-color: #378ADD; color: #FFFFFF; }"
        "QPushButton:disabled { background: #1E1E1E; color: #555555; }"
    );
    connect(m_btnAddTagBig, &QPushButton::clicked, this, [this]() { openTagSelectorOverlay(m_btnAddTagBig); });
    tagL->addWidget(m_btnAddTagBig);

    m_tagContainer = new QWidget(m_tagBox);
    m_tagFlowLayout = new FlowLayout(m_tagContainer, 0, 4, 4);

    m_btnAddTagSmall = new QPushButton(m_tagContainer);
    m_btnAddTagSmall->setFixedSize(22, 22);
    m_btnAddTagSmall->setCursor(Qt::PointingHandCursor);
    m_btnAddTagSmall->setIcon(UiHelper::getIcon("add", QColor("#CCCCCC"), 12));
    m_btnAddTagSmall->setIconSize(QSize(12, 12));
    m_btnAddTagSmall->setProperty("tooltipText", "添加标签");
    m_btnAddTagSmall->installEventFilter(this);
    m_btnAddTagSmall->setStyleSheet(
        "QPushButton { background-color: #2D2D30; border: 1px solid #555555; border-radius: 4px; padding: 0; }"
        "QPushButton:hover { background-color: #378ADD; border-color: #378ADD; }"
    );
    connect(m_btnAddTagSmall, &QPushButton::clicked, this, [this]() { openTagSelectorOverlay(m_btnAddTagSmall); });
    m_btnAddTagSmall->hide();

    tagL->addWidget(m_tagContainer);
    m_containerLayout->addWidget(createCollapsibleSection("标签管理", m_tagBox, true));

    // 7. 基础物理属性区
    m_infoSectionWidget = new QWidget(m_container);
    QVBoxLayout* infoL = new QVBoxLayout(m_infoSectionWidget);
    infoL->setContentsMargins(0, 0, 0, 0);
    infoL->setSpacing(4);

    addInfoRow(infoL, "文件类型", lblType);
    addInfoRow(infoL, "文件大小", lblSize);
    addInfoRow(infoL, "图片尺寸", lblDimensions);
    addInfoRow(infoL, "创建时间", lblCtime);
    addInfoRow(infoL, "修改时间", lblMtime);
    addInfoRow(infoL, "访问时间", lblAtime);
    addInfoRow(infoL, "加密状态", lblEncrypted);

    m_containerLayout->addWidget(createCollapsibleSection("基础属性", m_infoSectionWidget, true));

    // 8. 物理路径区
    QWidget* pathBox = new QWidget(m_container);
    QVBoxLayout* pathL = new QVBoxLayout(pathBox);
    pathL->setContentsMargins(0, 0, 0, 0);
    pathL->setSpacing(6);

    m_pathEdit = new QLineEdit(pathBox);
    m_pathEdit->setReadOnly(true);
    m_pathEdit->setStyleSheet("QLineEdit { background: #1E1E1E; border: 1px solid #2A2A2A; border-radius: 4px; padding: 4px 6px; font-size: 11px; color: #CCCCCC; }");
    pathL->addWidget(m_pathEdit);

    QHBoxLayout* pathBtnL = new QHBoxLayout();
    pathBtnL->setContentsMargins(0, 0, 0, 0);
    pathBtnL->setSpacing(6);

    m_btnCopyPath = new QPushButton("复制路径", pathBox);
    m_btnCopyPath->setFixedHeight(24);
    m_btnCopyPath->setCursor(Qt::PointingHandCursor);
    m_btnCopyPath->setStyleSheet("QPushButton { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; color: #CCCCCC; font-size: 11px; } QPushButton:hover { background: #333333; color: #FFFFFF; }");
    connect(m_btnCopyPath, &QPushButton::clicked, this, [this]() {
        QString p = m_pathEdit->text().trimmed();
        if (!p.isEmpty() && !p.startsWith("已选中")) {
            QApplication::clipboard()->setText(QDir::toNativeSeparators(p));
            ToolTipOverlay::instance()->showText(QCursor::pos(), "已复制路径", 1200, QColor("#2ecc71"));
        }
    });
    pathBtnL->addWidget(m_btnCopyPath);

    m_btnOpenLocation = new QPushButton("打开位置", pathBox);
    m_btnOpenLocation->setFixedHeight(24);
    m_btnOpenLocation->setCursor(Qt::PointingHandCursor);
    m_btnOpenLocation->setStyleSheet("QPushButton { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; color: #CCCCCC; font-size: 11px; } QPushButton:hover { background: #378ADD; border-color: #378ADD; color: #FFFFFF; }");
    connect(m_btnOpenLocation, &QPushButton::clicked, this, [this]() {
        QString p = m_pathEdit->text().trimmed();
        if (!p.isEmpty() && !p.startsWith("已选中")) {
            ShellHelper::openInExplorer(p);
        }
    });
    pathBtnL->addWidget(m_btnOpenLocation);

    pathL->addLayout(pathBtnL);
    m_containerLayout->addWidget(createCollapsibleSection("物理路径", pathBox, true));

    m_containerLayout->addStretch(1);
    m_scrollArea->setWidget(m_container);
    m_mainLayout->addWidget(m_scrollArea);

    updateControlsState(false, false, false);
}

void MetaPanel::openTagSelectorOverlay(QWidget* targetAnchor) {
    if (m_tagSelectorOverlay) {
        m_tagSelectorOverlay->close();
        return;
    }

    QWidget* topWidget = this->topLevelWidget();
    m_tagSelectorOverlay = new TagSelectorOverlay(m_currentTagsSet.values(), topWidget);

    QPoint globalPos = targetAnchor->mapToGlobal(QPoint(0, targetAnchor->height() + 4));
    QPoint parentPos = topWidget ? topWidget->mapFromGlobal(globalPos) : globalPos;

    QScreen* screen = QApplication::screenAt(globalPos);
    if (!screen) screen = QApplication::primaryScreen();
    if (screen) {
        int overlayH = m_tagSelectorOverlay->height();
        int screenBottom = screen->availableGeometry().bottom();
        if (globalPos.y() + overlayH > screenBottom) {
            parentPos.setY(parentPos.y() - overlayH - targetAnchor->height() - 8);
        }
    }

    m_tagSelectorOverlay->move(parentPos);
    m_tagSelectorOverlay->show();

    connect(m_tagSelectorOverlay, &TagSelectorOverlay::selectionChanged, this, [this](const QStringList& newSelectedTags) {
        QSet<QString> newSet(newSelectedTags.begin(), newSelectedTags.end());

        for (const QString& t : newSet) {
            if (!m_currentTagsSet.contains(t)) {
                emit tagAddRequested(m_selectedPaths, t);
            }
        }
        for (const QString& t : m_currentTagsSet) {
            if (!newSet.contains(t)) {
                emit tagRemoveRequested(m_selectedPaths, t);
            }
        }

        setTags(newSelectedTags);
    });
}

void MetaPanel::setImagePreview(const QPixmap& pixmap) {
    if (!m_lblImagePreview) return;
    if (pixmap.isNull()) {
        m_lblImagePreview->clear();
        m_lblImagePreview->hide();
        if (m_topPreviewBox) m_topPreviewBox->hide();
    } else {
        int maxW = m_container ? (m_container->width() - 16) : 214;
        maxW = qBound(120, maxW, 230);
        int maxH = 220;

        QPixmap scaled = pixmap.scaled(QSize(maxW, maxH), Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QImage roundedImg(scaled.size(), QImage::Format_ARGB32_Premultiplied);
        roundedImg.fill(Qt::transparent);
        {
            QPainter painter(&roundedImg);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            QPainterPath path;
            path.addRoundedRect(QRectF(0, 0, scaled.width(), scaled.height()), 4.0, 4.0);
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, scaled);
        }

        m_lblImagePreview->setPixmap(QPixmap::fromImage(roundedImg));
        m_lblImagePreview->setFixedSize(scaled.size());

        m_lblImagePreview->show();
        if (m_topPreviewBox) m_topPreviewBox->show();
    }
    adjustFlowHeights();
    if (m_container) m_container->adjustSize();
}

void MetaPanel::setSelectedPaths(const QStringList& paths) {
    m_selectedPaths = paths;
    m_editingPathsSnapshot = paths;
    bool hasSelection = !m_selectedPaths.isEmpty();
    bool isMulti = m_selectedPaths.size() > 1;

    m_isReadOnlyMode = false;
    if (hasSelection && (m_selectedPaths.first().endsWith(".amenc", Qt::CaseInsensitive) || m_selectedPaths.first().contains("trash://"))) {
        m_isReadOnlyMode = true;
    }

    updateControlsState(hasSelection, isMulti, m_isReadOnlyMode);

    if (!hasSelection) {
        m_isInternalUpdating = true;
        setImagePreview(QPixmap());
        if (m_nameEdit) { m_nameEdit->clear(); m_nameEdit->setPlaceholderText("文件名..."); }
        if (m_noteEdit) { m_noteEdit->clear(); m_noteEdit->setPlaceholderText("添加备注说明..."); }
        if (m_linkEdit) { m_linkEdit->clear(); m_linkEdit->setPlaceholderText("添加关联网址..."); }
        if (m_pathEdit) m_pathEdit->clear();
        if (lblType) lblType->setText("-");
        if (lblSize) lblSize->setText("-");
        if (lblDimensions) lblDimensions->setText("-");
        if (lblCtime) lblCtime->setText("-");
        if (lblMtime) lblMtime->setText("-");
        if (lblAtime) lblAtime->setText("-");
        if (lblEncrypted) lblEncrypted->setText("-");
        setRating(0, false);
        setColor(QString(""), false);
        setTags({});
        setPalettes({});
        m_isInternalUpdating = false;
    } else if (isMulti) {
        m_isInternalUpdating = true;
        setImagePreview(QPixmap());
        if (m_nameEdit) {
            m_nameEdit->setPlainText(QString("已选中 %1 个项目").arg(m_selectedPaths.size()));
        }
        if (m_noteEdit) {
            m_noteEdit->clear();
            m_noteEdit->setPlaceholderText("添加批量备注...");
            m_noteEdit->adjustHeight();
        }
        if (m_linkEdit) {
            m_linkEdit->clear();
            m_linkEdit->setPlaceholderText("添加批量关联网址...");
        }
        if (m_pathEdit) {
            m_pathEdit->setText(QString("已选中 %1 个项目").arg(m_selectedPaths.size()));
        }
        if (lblType) lblType->setText("混合项目");
        if (lblSize) lblSize->setText("-");
        if (lblDimensions) lblDimensions->setText("-");
        if (lblCtime) lblCtime->setText("-");
        if (lblMtime) lblMtime->setText("-");
        if (lblAtime) lblAtime->setText("-");
        if (lblEncrypted) lblEncrypted->setText("-");
        
        setRating(0, false);
        setColor(QString(""), false);
        setPalettes({});
        m_isInternalUpdating = false;
    }
}

void MetaPanel::updateControlsState(bool hasSelection, bool isMultiSelection, bool isReadOnly) {
    if (m_nameEdit) {
        m_nameEdit->setEnabled(hasSelection && !isMultiSelection && !isReadOnly);
    }
    if (m_noteEdit) {
        m_noteEdit->setEnabled(hasSelection && !isReadOnly);
    }
    if (m_linkEdit) {
        m_linkEdit->setEnabled(hasSelection && !isReadOnly);
    }
    if (m_btnAddTagBig) {
        m_btnAddTagBig->setEnabled(hasSelection && !isReadOnly);
    }
    if (m_btnAddTagSmall) {
        m_btnAddTagSmall->setEnabled(hasSelection && !isReadOnly);
    }
    if (m_ratingColorBox) {
        m_ratingColorBox->setEnabled(hasSelection && !isReadOnly);
    }
    if (m_btnCopyPath) {
        m_btnCopyPath->setEnabled(hasSelection && !isMultiSelection);
    }
    if (m_btnOpenLocation) {
        m_btnOpenLocation->setEnabled(hasSelection && !isMultiSelection);
    }
}

void MetaPanel::addInfoRow(QVBoxLayout* layout, const QString& label, QLabel*& valueLabel) {
    QWidget* row = new QWidget(m_container);
    QHBoxLayout* rl = new QHBoxLayout(row);
    rl->setContentsMargins(0, 1, 0, 1);
    rl->setSpacing(6);
    
    QLabel* kl = new QLabel(label, row);
    kl->setFixedWidth(65);
    kl->setStyleSheet("font-size: 11px; color: #888888;");
    rl->addWidget(kl, 0, Qt::AlignTop);

    valueLabel = new QLabel("-", row);
    valueLabel->setWordWrap(true);
    valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    valueLabel->setStyleSheet("font-size: 11px; color: #CCCCCC; line-height: 1.4;");
    valueLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    rl->addWidget(valueLabel, 1);
    
    layout->addWidget(row);
}

void MetaPanel::onTagDeleted(const QString& text) {
    if (m_selectedPaths.isEmpty() || m_isReadOnlyMode) return;

    emit tagRemoveRequested(m_selectedPaths, text);

    QStringList remainingTags = m_currentTagsSet.values();
    remainingTags.removeAll(text);
    setTags(remainingTags);
}

void MetaPanel::resizeEvent(QResizeEvent* event) {
    QFrame::resizeEvent(event);
    QTimer::singleShot(0, this, [this]() {
        if (!m_scrollArea || !m_container) return;
        int viewportW = m_scrollArea->viewport()->width();
        if (viewportW < 100) return;

        if (m_container->width() != viewportW) {
            m_container->setFixedWidth(viewportW);
        }
        
        int maxW = viewportW - 16;
        if (maxW > 50) {
            auto syncWidthAndHeight = [maxW](ElasticEdit* edit) {
                if (edit && edit->width() != maxW) {
                    edit->setFixedWidth(maxW);
                    edit->adjustHeight();
                }
            };

            syncWidthAndHeight(m_nameEdit);
            syncWidthAndHeight(m_noteEdit);
            if (m_btnAddTagBig) m_btnAddTagBig->setFixedWidth(maxW);
            
            if (m_topPreviewBox) m_topPreviewBox->setFixedWidth(maxW);
            if (m_ratingColorBox) m_ratingColorBox->setFixedWidth(maxW);
            if (m_tagBox) m_tagBox->setFixedWidth(maxW);
            if (m_tagContainer) m_tagContainer->setFixedWidth(maxW);
            
            adjustFlowHeights();
            m_container->adjustSize();
        }
    });
}

void MetaPanel::adjustFlowHeights() {
    if (m_topPreviewBox && m_paletteFlowLayout) {
        int contentH = m_paletteFlowLayout->heightForWidth(m_topPreviewBox->width());
        bool hasPreview = (m_lblImagePreview && !m_lblImagePreview->pixmap().isNull());
        bool hasPalette = (m_paletteFlowLayout->count() > 0);
        if (hasPreview || hasPalette) {
            m_topPreviewBox->show();
            int previewH = hasPreview ? m_lblImagePreview->pixmap().height() : 0;
            int totalSpacing = (hasPreview && hasPalette) ? 6 : 0;
            m_topPreviewBox->setFixedHeight(contentH + previewH + totalSpacing);
        } else {
            m_topPreviewBox->hide();
            m_topPreviewBox->setFixedHeight(0);
        }
        m_paletteFlowLayout->activate();
    }
    if (m_tagContainer && m_tagFlowLayout) {
        bool hasTags = (m_tagFlowLayout->count() > (m_btnAddTagSmall ? 1 : 0));
        if (hasTags) {
            if (m_btnAddTagBig) m_btnAddTagBig->hide();
            if (m_btnAddTagSmall) m_btnAddTagSmall->show();
            int contentH = m_tagFlowLayout->heightForWidth(m_tagContainer->width());
            contentH = qMax(26, contentH);
            m_tagContainer->show();
            m_tagContainer->setFixedHeight(contentH);
        } else {
            if (m_btnAddTagSmall) m_btnAddTagSmall->hide();
            if (m_btnAddTagBig) m_btnAddTagBig->show();
            m_tagContainer->setFixedHeight(0);
            m_tagContainer->hide();
        }
        m_tagFlowLayout->activate();
    }
}

void MetaPanel::showEvent(QShowEvent* event) {
    QFrame::showEvent(event);
    QResizeEvent e(size(), size());
    MetaPanel::resizeEvent(&e);
}

void MetaPanel::updateInfo(const QString& n, const QString& t, const QString& s, 
                            const QString& ct, const QString& mt, const QString& at, 
                            const QString& p, bool e, int width, int height) {
    if (m_isUserEditing) return;

    m_isInternalUpdating = true;
    
    QFileInfo info(n);
    if (m_selectedPaths.size() <= 1) {
        m_nameEdit->setPlainText(info.completeBaseName());
        m_nameEdit->adjustHeight();
        m_nameEdit->setProperty("oldPath", p);
        m_nameEdit->setProperty("suffix", info.suffix());
        
        lblType->setText(t);
        lblSize->setText(s);
        lblCtime->setText(ct);
        lblMtime->setText(mt);
        lblAtime->setText(at);
        
        m_pathEdit->setText(p);
        m_pathEdit->setCursorPosition(0);
    }

    lblEncrypted->setText(e ? "已加密" : "未加密");
    
    if (width > 0 && height > 0 && m_selectedPaths.size() <= 1) {
        lblDimensions->setText(QString("%1 x %2 像素").arg(width).arg(height));
        if (lblDimensions->parentWidget()) lblDimensions->parentWidget()->show();
    } else {
        lblDimensions->setText("-");
        if (lblDimensions->parentWidget()) lblDimensions->parentWidget()->hide();
    }

    if (m_container) m_container->adjustSize();
    m_isInternalUpdating = false;
}

void MetaPanel::setTags(const QStringList& tags) {
    m_currentTagsSet = QSet<QString>(tags.begin(), tags.end());

    while (QLayoutItem* item = m_tagFlowLayout->takeAt(0)) {
        if (item->widget() && item->widget() != m_btnAddTagSmall) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    if (tags.isEmpty()) {
        m_btnAddTagBig->show();
        m_btnAddTagSmall->hide();
    } else {
        m_btnAddTagBig->hide();

        for (const QString& tag : tags) {
            TagPill* pill = new TagPill(tag, m_tagContainer);
            pill->setProperty("tagText", tag);
            connect(pill, &TagPill::deleteRequested, this, &MetaPanel::onTagDeleted);
            pill->show();
            m_tagFlowLayout->addWidget(pill);
        }

        m_btnAddTagSmall->show();
        m_tagFlowLayout->addWidget(m_btnAddTagSmall);
    }

    adjustFlowHeights();
    if (m_container) m_container->adjustSize();
    m_adjustTimer->start();
}

void MetaPanel::setRating(int rating, bool fromUser) {
    m_currentRating = rating;
    for (int i = 0; i < m_starBtns.size(); ++i) {
        bool active = (i < rating);
        // 🚀 五角星图标尺寸严格设为 16px
        m_starBtns[i]->setIcon(UiHelper::getIcon(
            active ? "star_filled" : "star",
            active ? QColor("#FF551C") : QColor("#555555"),
            16
        ));
        m_starBtns[i]->setIconSize(QSize(16, 16));
    }

    if (fromUser && !m_selectedPaths.isEmpty() && !m_isReadOnlyMode) {
        emit ratingChanged(m_selectedPaths, rating);
    }
}

void MetaPanel::setColor(const QString& hexColor, bool fromUser) {
    m_currentColorHex = hexColor;

    for (QPushButton* btn : m_colorBtns) {
        QString hex = btn->property("hexColor").toString();
        bool active = (!hexColor.isEmpty() && hex.compare(hexColor, Qt::CaseInsensitive) == 0);

        btn->setStyleSheet(QString(
            "QPushButton { background-color: %1; border: %2; border-radius: 8px; }"
            "QPushButton:hover { border-color: #FFFFFF; }"
        ).arg(hex).arg(active ? "2px solid #FFFFFF" : "1px solid transparent"));
    }

    if (fromUser && !m_selectedPaths.isEmpty() && !m_isReadOnlyMode) {
        emit colorChanged(m_selectedPaths, hexColor);
    }
}

void MetaPanel::setColor(const std::wstring& color, bool fromUser) {
    setColor(QString::fromStdWString(color), fromUser);
}

void MetaPanel::setNote(const QString& note) {
    m_isInternalUpdating = true;
    m_noteEdit->setPlainText(note);
    m_noteEdit->adjustHeight();
    if (m_container) m_container->adjustSize();
    m_isInternalUpdating = false;
}

void MetaPanel::setNote(const std::wstring& note) {
    setNote(QString::fromStdWString(note));
}

void MetaPanel::setURL(const QString& url) {
    m_isInternalUpdating = true;
    m_linkEdit->setText(url);
    m_linkEdit->setCursorPosition(0);
    if (m_actOpenLink) {
        m_actOpenLink->setVisible(!url.trimmed().isEmpty());
    }
    if (m_container) m_container->adjustSize();
    m_isInternalUpdating = false;
}

void MetaPanel::setURL(const std::wstring& url) {
    setURL(QString::fromStdWString(url));
}

void MetaPanel::setPalettes(const QVector<QPair<QColor, float>>& palette) {
    if (!m_paletteFlowLayout) return;

    while (QLayoutItem* item = m_paletteFlowLayout->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    for (const auto& entry : palette) {
        ColorPill* pill = new ColorPill(entry.first, entry.second, m_paletteContainer);
        pill->setStyleSheet("background: transparent; border: none;");
        connect(pill, &ColorPill::colorSelected, this, [this](const QColor& c){ emit searchByColor(c); });
        connect(pill, &ColorPill::requestSetAsPrimary, this, &MetaPanel::setAsPrimaryColor);
        pill->show();
        m_paletteFlowLayout->addWidget(pill);
    }

    m_paletteFlowLayout->invalidate();
    if (m_topPreviewBox) m_topPreviewBox->update();
    adjustFlowHeights();
    m_adjustTimer->start();
}

void MetaPanel::setAsPrimaryColor(const QColor& color) {
    QString currentPath = m_pathEdit->text().trimmed();
    if (!currentPath.isEmpty() && !currentPath.startsWith("已选中")) {
        emit primaryColorChanged(currentPath, color);
    }
}

bool MetaPanel::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::ToolTip) {
        QString text = watched->property("tooltipText").toString();
        if (!text.isEmpty()) {
            ToolTipOverlay::instance()->showText(QCursor::pos(), text, 0);
            return true;
        }
    } else if (event->type() == QEvent::Leave || event->type() == QEvent::MouseButtonPress) {
        ToolTipOverlay::hideTip();
    }

    if (m_isInternalUpdating || m_isReadOnlyMode) return QFrame::eventFilter(watched, event);

    // 拦截文件名编辑的回车键，防止插入换行符并触发提交
    if (watched == m_nameEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            m_nameEdit->clearFocus();
            return true;
        }
    }

    if (watched == m_linkEdit) {
        if (event->type() == QEvent::FocusIn) {
            m_isUserEditing = true;
            m_editingPathsSnapshot = m_selectedPaths;
        } else if (event->type() == QEvent::FocusOut) {
            m_isUserEditing = false;
        }
    } else if (event->type() == QEvent::FocusIn) {
        if (watched == m_noteEdit || watched == m_nameEdit) {
            m_isUserEditing = true;
            m_editingPathsSnapshot = m_selectedPaths;
        }
    } else if (event->type() == QEvent::FocusOut) {
        if (watched == m_noteEdit || watched == m_nameEdit) {
            m_isUserEditing = false;
        }
    }

    if (watched == m_noteEdit && event->type() == QEvent::FocusOut) {
        if (!m_editingPathsSnapshot.isEmpty()) {
            emit noteEdited(m_editingPathsSnapshot, m_noteEdit->toPlainText());
        }
    } else if (watched == m_linkEdit && event->type() == QEvent::FocusOut) {
        if (!m_editingPathsSnapshot.isEmpty()) {
            emit linkEdited(m_editingPathsSnapshot, m_linkEdit->text().trimmed());
        }
    } else if (watched == m_nameEdit && event->type() == QEvent::FocusOut) {
        if (m_editingPathsSnapshot.size() > 1) return true;

        QString oldPath = m_nameEdit->property("oldPath").toString();
        QString newName = m_nameEdit->toPlainText().trimmed();
        
        static const QRegularExpression illegalRegex("[\\\\/:*?\"<>|\r\n]");
        newName.remove(illegalRegex);
        m_nameEdit->setPlainText(newName);

        QString suffix = m_nameEdit->property("suffix").toString();
        if (!oldPath.isEmpty() && !newName.isEmpty()) {
            QFileInfo oldInfo(oldPath);
            if (newName != oldInfo.completeBaseName()) {
                QString newPath = oldInfo.absolutePath() + "/" + newName + (suffix.isEmpty() ? "" : "." + suffix);
                newPath = QDir::toNativeSeparators(newPath);
                
                if (QFile::exists(newPath)) {
                    m_nameEdit->setPlainText(oldInfo.completeBaseName());
                    return true;
                }

                emit renameRequested(oldPath, newPath);
            }
        }
    }
    return QFrame::eventFilter(watched, event);
}

} // namespace QuarkMeta