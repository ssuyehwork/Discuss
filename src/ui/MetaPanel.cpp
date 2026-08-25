#include "MetaPanel.h"
#include "SvgIcons.h"
#include "ToolTipOverlay.h"
#include "UiHelper.h"
#include "../meta/MetadataManager.h"
#include "../meta/QuarkMetaJson.h"
#include "../util/ShellHelper.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QLabel>
#include <QRegularExpressionValidator>
#include <QDir>
#include <QFile>
#include <QApplication>
#include <QScreen>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QCursor>

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

void MetaPanel::initUi() {
    // 头部面板标识
    QWidget* header = new QWidget(this); 
    header->setObjectName("ContainerHeader"); 
    header->setFixedHeight(32);
    header->setStyleSheet("QWidget#ContainerHeader { background-color: #252526; border-bottom: 1px solid #333; }");
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
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setWidgetResizable(true); 
    m_scrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");
    
    m_container = new QWidget(m_scrollArea); 
    m_containerLayout = new QVBoxLayout(m_container); 
    m_containerLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    m_containerLayout->setContentsMargins(8, 8, 8, 8); 
    m_containerLayout->setSpacing(8);

    // ==========================================
    // 模块 1: 顶部预览与色板区 (m_topPreviewBox)
    // ==========================================
    m_topPreviewBox = new QWidget(m_container);
    m_topPreviewBox->setObjectName("TopPreviewBox");
    m_topPreviewBox->setStyleSheet("QWidget#TopPreviewBox { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; }");
    QVBoxLayout* previewLayout = new QVBoxLayout(m_topPreviewBox);
    previewLayout->setContentsMargins(6, 6, 6, 6);
    previewLayout->setSpacing(6);

    m_lblImagePreview = new QLabel(m_topPreviewBox);
    m_lblImagePreview->setAlignment(Qt::AlignCenter);
    m_lblImagePreview->setMinimumHeight(60);
    m_lblImagePreview->setStyleSheet("background: #1E1E1E; border-radius: 2px;");
    m_lblImagePreview->hide(); // 默认无缩略图时隐藏
    previewLayout->addWidget(m_lblImagePreview);

    m_paletteFlowLayout = new FlowLayout(nullptr, 4, 4, 4);
    m_paletteFlowLayout->setContentsMargins(0, 0, 0, 0);
    QWidget* paletteContainer = new QWidget(m_topPreviewBox);
    paletteContainer->setLayout(m_paletteFlowLayout);
    previewLayout->addWidget(paletteContainer);

    m_topPreviewBox->hide(); // 默认无内容隐藏，不占用任何界面空间
    m_containerLayout->addWidget(m_topPreviewBox);

    // ==========================================
    // 模块 2: 文件名大字高亮 (`m_nameEdit`)
    // ==========================================
    m_nameEdit = new ElasticEdit(m_container);
    m_nameEdit->setPlaceholderText("文件名...");
    m_nameEdit->setStyleSheet(
        "QTextEdit { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; padding: 4px 8px; "
        "font-size: 13px; font-weight: bold; color: #FFFFFF; }"
    );
    m_nameEdit->installEventFilter(this);
    m_containerLayout->addWidget(m_nameEdit);

    // ==========================================
    // 模块 3: 星级评级 + 颜色色标条 (独立整洁容器 + 100% SVG 图标)
    // ==========================================
    m_ratingColorBox = new QWidget(m_container);
    QVBoxLayout* ratingColorLayout = new QVBoxLayout(m_ratingColorBox);
    ratingColorLayout->setContentsMargins(0, 2, 0, 2);
    ratingColorLayout->setSpacing(8);

    // 评级行（无评级 SVG + 5 星 SVG）
    QWidget* ratingRow = new QWidget(m_ratingColorBox);
    ratingRow->setStyleSheet("QWidget { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; }");
    QHBoxLayout* starLayout = new QHBoxLayout(ratingRow);
    starLayout->setContentsMargins(8, 4, 8, 4);
    starLayout->setSpacing(4);

    QPushButton* btnClearStar = new QPushButton(ratingRow);
    btnClearStar->setFixedSize(20, 20);
    btnClearStar->setCursor(Qt::PointingHandCursor);
    btnClearStar->setIcon(UiHelper::getIcon("prohibit", QColor("#888888"), 14));
    btnClearStar->setIconSize(QSize(14, 14));
    btnClearStar->setProperty("tooltipText", "清除评级");
    btnClearStar->installEventFilter(this);
    btnClearStar->setStyleSheet("QPushButton { border: none; background: transparent; } QPushButton:hover { background: #333; border-radius: 3px; }");
    connect(btnClearStar, &QPushButton::clicked, this, [this]() {
        setRating(0);
        emit metadataChanged(m_currentRating, m_currentColor);
    });
    starLayout->addWidget(btnClearStar);

    for (int i = 1; i <= 5; ++i) {
        QPushButton* btnStar = new QPushButton(ratingRow);
        btnStar->setFixedSize(20, 20);
        btnStar->setCursor(Qt::PointingHandCursor);
        btnStar->setIcon(UiHelper::getIcon("star", QColor("#555555"), 16));
        btnStar->setIconSize(QSize(16, 16));
        btnStar->setStyleSheet("QPushButton { border: none; background: transparent; }");
        
        connect(btnStar, &QPushButton::clicked, this, [this, i]() {
            int newRating = (m_currentRating == i) ? 0 : i;
            setRating(newRating);
            emit metadataChanged(m_currentRating, m_currentColor);
        });
        m_starBtns.append(btnStar);
        starLayout->addWidget(btnStar);
    }
    starLayout->addStretch();
    ratingColorLayout->addWidget(ratingRow);

    // 颜色标记行（无色标 SVG + 8 基础色）
    QWidget* colorRow = new QWidget(m_ratingColorBox);
    colorRow->setStyleSheet("QWidget { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; }");
    QHBoxLayout* colorLayout = new QHBoxLayout(colorRow);
    colorLayout->setContentsMargins(8, 4, 8, 4);
    colorLayout->setSpacing(4);

    QPushButton* btnNoColor = new QPushButton(colorRow);
    btnNoColor->setFixedSize(18, 18);
    btnNoColor->setCursor(Qt::PointingHandCursor);
    btnNoColor->setIcon(UiHelper::getIcon("no_color", QColor("#888888"), 14));
    btnNoColor->setIconSize(QSize(14, 14));
    btnNoColor->setProperty("tooltipText", "无色标");
    btnNoColor->installEventFilter(this);
    btnNoColor->setStyleSheet("QPushButton { border: none; background: transparent; } QPushButton:hover { background: #333; border-radius: 9px; }");
    connect(btnNoColor, &QPushButton::clicked, this, [this]() {
        setColor(L"");
        emit metadataChanged(m_currentRating, m_currentColor);
    });
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
        btnColor->installEventFilter(this);
        btnColor->setStyleSheet(QString(
            "QPushButton { background-color: %1; border: 1px solid transparent; border-radius: 8px; }"
            "QPushButton:hover { border-color: #FFFFFF; }"
        ).arg(pair.second));

        std::wstring colorName = pair.first.toStdWString();
        connect(btnColor, &QPushButton::clicked, this, [this, colorName]() {
            setColor(colorName);
            emit metadataChanged(m_currentRating, m_currentColor);
        });
        m_colorBtns.append(btnColor);
        colorLayout->addWidget(btnColor);
    }
    colorLayout->addStretch();
    ratingColorLayout->addWidget(colorRow);
    m_containerLayout->addWidget(m_ratingColorBox);

    // ==========================================
    // 模块 4: 标签管理区 (`m_tagBox`) - 可折叠
    // ==========================================
    m_tagBox = new QWidget(m_container);
    QVBoxLayout* tagL = new QVBoxLayout(m_tagBox);
    tagL->setContentsMargins(0, 0, 0, 0);
    tagL->setSpacing(6);

    m_btnAddTagBig = new QPushButton(UiHelper::getIcon("add", QColor("#AAAAAA"), 14), "添加标签", m_tagBox);
    m_btnAddTagBig->setFixedHeight(28);
    m_btnAddTagBig->setCursor(Qt::PointingHandCursor);
    m_btnAddTagBig->setStyleSheet(
        "QPushButton { background-color: #252526; border: 1px dashed #3c3c3c; border-radius: 4px; padding: 0 10px; color: #AAAAAA; font-size: 12px; text-align: center; }"
        "QPushButton:hover { background-color: #2a2d2e; border-color: #378ADD; color: #FFFFFF; }"
        "QPushButton:pressed { background-color: #333333; }"
    );
    connect(m_btnAddTagBig, &QPushButton::clicked, this, [this]() {
        openTagSelectorOverlay(m_btnAddTagBig);
    });
    tagL->addWidget(m_btnAddTagBig);

    m_tagContainer = new QWidget(m_tagBox);
    m_tagFlowLayout = new FlowLayout(m_tagContainer, 0, 4, 4);
    tagL->addWidget(m_tagContainer);

    m_btnAddTagSmall = new QPushButton(m_tagContainer);
    m_btnAddTagSmall->setFixedSize(22, 22);
    m_btnAddTagSmall->setCursor(Qt::PointingHandCursor);
    m_btnAddTagSmall->setIcon(UiHelper::getIcon("add", QColor("#CCCCCC"), 12));
    m_btnAddTagSmall->setIconSize(QSize(12, 12));
    m_btnAddTagSmall->setProperty("tooltipText", "添加标签");
    m_btnAddTagSmall->installEventFilter(this);
    m_btnAddTagSmall->setStyleSheet(
        "QPushButton { background-color: #2D2D30; border: 1px dashed #555555; border-radius: 4px; padding: 0; }"
        "QPushButton:hover { background-color: #378ADD; border-color: #378ADD; }"
    );
    connect(m_btnAddTagSmall, &QPushButton::clicked, this, [this]() {
        openTagSelectorOverlay(m_btnAddTagSmall);
    });
    m_btnAddTagSmall->hide();

    QWidget* collapsibleTagSection = createCollapsibleSection("标签管理", m_tagBox, true);
    m_containerLayout->addWidget(collapsibleTagSection);

    // ==========================================
    // 模块 5: 备注说明区 (`m_noteEdit`) - 可折叠
    // ==========================================
    m_noteEdit = new ElasticEdit(m_container);
    m_noteEdit->setPlaceholderText("添加备注说明...");
    m_noteEdit->setStyleSheet("QTextEdit { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; padding: 4px 8px; font-size: 12px; color: #AAAAAA; }");
    m_noteEdit->installEventFilter(this);

    QWidget* collapsibleNoteSection = createCollapsibleSection("备注说明", m_noteEdit, true);
    m_containerLayout->addWidget(collapsibleNoteSection);

    // ==========================================
    // 模块 6: 关联网址区 (`m_linkSection`) - 可折叠
    // ==========================================
    m_linkBox = new QWidget(m_container);
    QHBoxLayout* linkL = new QHBoxLayout(m_linkBox);
    linkL->setContentsMargins(0, 0, 0, 0);
    linkL->setSpacing(4);

    m_linkEdit = new ElasticEdit(m_linkBox);
    m_linkEdit->setPlaceholderText("添加关联网址...");
    m_linkEdit->setStyleSheet("QTextEdit { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; padding: 4px 8px; font-size: 12px; color: #378ADD; }");
    m_linkEdit->installEventFilter(this);
    linkL->addWidget(m_linkEdit, 1);

    m_btnOpenLink = new QPushButton(m_linkBox);
    m_btnOpenLink->setFixedSize(24, 24);
    m_btnOpenLink->setCursor(Qt::PointingHandCursor);
    m_btnOpenLink->setProperty("tooltipText", "在默认浏览器中打开");
    m_btnOpenLink->installEventFilter(this);
    m_btnOpenLink->setIcon(UiHelper::getIcon("link", QColor("#378ADD"), 16));
    m_btnOpenLink->setIconSize(QSize(16, 16));
    m_btnOpenLink->setStyleSheet("QPushButton { border: 1px solid #3c3c3c; background: #252526; border-radius: 4px; } QPushButton:hover { background: #333; border-color: #378ADD; }");
    m_btnOpenLink->hide(); // 默认非空时显示
    connect(m_btnOpenLink, &QPushButton::clicked, this, [this]() {
        QString urlStr = m_linkEdit->toPlainText().trimmed();
        if (!urlStr.isEmpty()) {
            if (!urlStr.startsWith("http://") && !urlStr.startsWith("https://")) {
                urlStr = "https://" + urlStr;
            }
            QDesktopServices::openUrl(QUrl(urlStr));
        }
    });
    linkL->addWidget(m_btnOpenLink);

    QWidget* collapsibleLinkSection = createCollapsibleSection("关联网址", m_linkBox, true);
    m_containerLayout->addWidget(collapsibleLinkSection);

    // ==========================================
    // 模块 7: 基础物理属性区 (`m_infoSectionWidget`) - 可折叠
    // ==========================================
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

    QWidget* collapsibleInfoSection = createCollapsibleSection("基础属性", m_infoSectionWidget, true);
    m_containerLayout->addWidget(collapsibleInfoSection);

    // ==========================================
    // 模块 8: 物理路径区 (`m_pathSection`) - 可折叠
    // ==========================================
    QWidget* pathBox = new QWidget(m_container);
    QVBoxLayout* pathL = new QVBoxLayout(pathBox);
    pathL->setContentsMargins(0, 0, 0, 0);
    pathL->setSpacing(6);

    m_pathEdit = new ElasticEdit(pathBox);
    m_pathEdit->setReadOnly(true);
    m_pathEdit->setStyleSheet("QTextEdit { background: #1E1E1E; border: 1px solid #2A2A2A; border-radius: 4px; padding: 4px 8px; font-size: 11px; color: #CCCCCC; }");
    pathL->addWidget(m_pathEdit);

    QHBoxLayout* pathBtnL = new QHBoxLayout();
    pathBtnL->setContentsMargins(0, 0, 0, 0);
    pathBtnL->setSpacing(6);

    m_btnCopyPath = new QPushButton("复制路径", pathBox);
    m_btnCopyPath->setFixedHeight(24);
    m_btnCopyPath->setCursor(Qt::PointingHandCursor);
    m_btnCopyPath->setStyleSheet("QPushButton { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; color: #CCCCCC; font-size: 11px; } QPushButton:hover { background: #333333; color: #FFFFFF; }");
    connect(m_btnCopyPath, &QPushButton::clicked, this, [this]() {
        QString p = m_pathEdit->toPlainText().trimmed();
        if (!p.isEmpty()) {
            QApplication::clipboard()->setText(QDir::toNativeSeparators(p));
            ToolTipOverlay::instance()->showText(m_btnCopyPath->mapToGlobal(QPoint(0, -28)), "已复制路径");
        }
    });
    pathBtnL->addWidget(m_btnCopyPath);

    m_btnOpenLocation = new QPushButton("打开位置", pathBox);
    m_btnOpenLocation->setFixedHeight(24);
    m_btnOpenLocation->setCursor(Qt::PointingHandCursor);
    m_btnOpenLocation->setStyleSheet("QPushButton { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; color: #CCCCCC; font-size: 11px; } QPushButton:hover { background: #378ADD; border-color: #378ADD; color: #FFFFFF; }");
    connect(m_btnOpenLocation, &QPushButton::clicked, this, [this]() {
        QString p = m_pathEdit->toPlainText().trimmed();
        if (!p.isEmpty()) {
            ShellHelper::openInExplorer(p);
        }
    });
    pathBtnL->addWidget(m_btnOpenLocation);

    pathL->addLayout(pathBtnL);

    QWidget* collapsiblePathSection = createCollapsibleSection("物理路径", pathBox, true);
    m_containerLayout->addWidget(collapsiblePathSection);

    m_containerLayout->addStretch(1);
    m_scrollArea->setWidget(m_container);
    m_mainLayout->addWidget(m_scrollArea);

    updateControlsState(false);
}

QWidget* MetaPanel::createCollapsibleSection(const QString& title, QWidget* contentWidget, bool defaultExpanded) {
    QWidget* sectionWidget = new QWidget(m_container);
    QVBoxLayout* sectionLayout = new QVBoxLayout(sectionWidget);
    sectionLayout->setContentsMargins(0, 0, 0, 0);
    sectionLayout->setSpacing(4);

    QPushButton* btnHeader = new QPushButton(sectionWidget);
    btnHeader->setFixedHeight(24);
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

void MetaPanel::openTagSelectorOverlay(QWidget* targetAnchor) {
    if (m_tagSelectorOverlay) {
        m_tagSelectorOverlay->close();
        return;
    }

    QStringList currentTags;
    for (int i = 0; i < m_tagFlowLayout->count(); ++i) {
        TagPill* pill = qobject_cast<TagPill*>(m_tagFlowLayout->itemAt(i)->widget());
        if (pill) {
            QString tagStr = pill->property("tagText").toString();
            if (!tagStr.isEmpty()) currentTags.append(tagStr);
        }
    }

    QWidget* topWidget = this->topLevelWidget();
    m_tagSelectorOverlay = new TagSelectorOverlay(currentTags, topWidget);

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

    connect(m_tagSelectorOverlay, &TagSelectorOverlay::selectionChanged, this, [this](const QStringList& selectedTags) {
        setTags(selectedTags);
    });

    connect(m_tagSelectorOverlay, &TagSelectorOverlay::overlayClosed, this, [this]() {
        if (m_selectedPaths.isEmpty()) return;

        QStringList finalTags;
        for (int i = 0; i < m_tagFlowLayout->count(); ++i) {
            TagPill* pill = qobject_cast<TagPill*>(m_tagFlowLayout->itemAt(i)->widget());
            if (pill) {
                QString tagStr = pill->property("tagText").toString();
                if (!tagStr.isEmpty()) finalTags.append(tagStr);
            }
        }

        for (const QString& path : m_selectedPaths) {
            std::wstring wpath = path.toStdWString();
            MetadataManager::instance().setTags(wpath, finalTags, true);
        }

        emit tagsChanged(m_selectedPaths, finalTags);
    });
}

void MetaPanel::setSelectedPaths(const QStringList& paths) {
    m_selectedPaths = paths;
    bool hasSelection = !m_selectedPaths.isEmpty();
    updateControlsState(hasSelection);

    if (!hasSelection) {
        m_isInternalUpdating = true;
        if (m_nameEdit) m_nameEdit->clear();
        if (m_noteEdit) m_noteEdit->clear();
        if (m_linkEdit) m_linkEdit->clear();
        if (m_pathEdit) m_pathEdit->clear();
        if (lblType) lblType->setText("-");
        if (lblSize) lblSize->setText("-");
        if (lblDimensions) lblDimensions->setText("-");
        if (lblCtime) lblCtime->setText("-");
        if (lblMtime) lblMtime->setText("-");
        if (lblAtime) lblAtime->setText("-");
        if (lblEncrypted) lblEncrypted->setText("-");
        setRating(0);
        setColor(L"");
        setTags({});
        setPalettes({});
        m_isInternalUpdating = false;
    } else if (m_selectedPaths.size() == 1) {
        QString p = m_selectedPaths.first();
        QFileInfo fi(p);
        QuarkMetaJson json(fi.absolutePath().toStdWString());
        if (json.load()) {
            auto it = json.items().find(fi.fileName().toStdWString());
            if (it != json.items().end()) {
                QStringList loadedTags;
                for (const auto& t : it->second.tags) loadedTags << QString::fromStdWString(t);
                setTags(loadedTags);
                setRating(it->second.rating);
                setColor(it->second.color);
                setNote(QString::fromStdWString(it->second.note));
                setURL(QString::fromStdWString(it->second.url));
            }
        }
    }
}

void MetaPanel::updateControlsState(bool hasSelection) {
    if (m_nameEdit) m_nameEdit->setEnabled(hasSelection);
    if (m_noteEdit) m_noteEdit->setEnabled(hasSelection);
    if (m_linkEdit) m_linkEdit->setEnabled(hasSelection);
    if (m_btnAddTagBig) m_btnAddTagBig->setEnabled(hasSelection);
    if (m_btnAddTagSmall) m_btnAddTagSmall->setEnabled(hasSelection);
    if (m_ratingColorBox) m_ratingColorBox->setEnabled(hasSelection);
    if (m_btnCopyPath) m_btnCopyPath->setEnabled(hasSelection);
    if (m_btnOpenLocation) m_btnOpenLocation->setEnabled(hasSelection);

    QString editStyle = hasSelection
        ? "QTextEdit { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; padding: 4px 8px; font-size: 12px; color: #EEEEEE; }"
        : "QTextEdit { background: #1E1E1E; border: 1px solid #2A2A2A; border-radius: 4px; padding: 4px 8px; font-size: 12px; color: #555555; }";

    if (m_nameEdit) {
        m_nameEdit->setStyleSheet(hasSelection
            ? "QTextEdit { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; padding: 4px 8px; font-size: 13px; font-weight: bold; color: #FFFFFF; }"
            : "QTextEdit { background: #1E1E1E; border: 1px solid #2A2A2A; border-radius: 4px; padding: 4px 8px; font-size: 13px; font-weight: bold; color: #555555; }");
    }
    if (m_noteEdit) m_noteEdit->setStyleSheet(editStyle);
    if (m_linkEdit) m_linkEdit->setStyleSheet(editStyle);
}

void MetaPanel::addInfoRow(QVBoxLayout* layout, const QString& label, QLabel*& valueLabel) {
    QWidget* row = new QWidget(m_container); 
    QHBoxLayout* rl = new QHBoxLayout(row); 
    rl->setContentsMargins(0, 1, 0, 1); 
    rl->setSpacing(6); 
    
    QLabel* kl = new QLabel(label, row); 
    kl->setFixedWidth(70);
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

QFrame* MetaPanel::createSeparator() {
    QFrame* l = new QFrame(this); 
    l->setFrameShape(QFrame::HLine); 
    l->setFixedHeight(1); 
    l->setStyleSheet("background-color: #333333; border: none; margin: 2px 0;"); 
    return l; 
}

void MetaPanel::onTagDeleted(const QString& text) { 
    if (m_selectedPaths.isEmpty()) return; 
 
    for (int i = 0; i < m_tagFlowLayout->count(); ++i) { 
        QLayoutItem* item = m_tagFlowLayout->itemAt(i); 
        TagPill* pill = qobject_cast<TagPill*>(item->widget()); 
        if (pill && pill->property("tagText").toString() == text) { 
            m_tagFlowLayout->takeAt(i); 
            pill->deleteLater(); 
            delete item; 
            break; 
        } 
    } 
 
    adjustFlowHeights(); 
    if (m_container) m_container->adjustSize(); 

    QStringList remainingTags;
    for (int i = 0; i < m_tagFlowLayout->count(); ++i) {
        TagPill* pill = qobject_cast<TagPill*>(m_tagFlowLayout->itemAt(i)->widget());
        if (pill) {
            remainingTags.append(pill->property("tagText").toString());
        }
    }

    for (const QString& path : m_selectedPaths) {
        MetadataManager::instance().setTags(path.toStdWString(), remainingTags, true);
    }
 
    emit tagRemoveRequested(m_selectedPaths, text); 
    emit tagsChanged(m_selectedPaths, remainingTags);
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
            if (m_linkEdit) m_linkEdit->adjustHeight();
            if (m_btnAddTagBig) m_btnAddTagBig->setFixedWidth(maxW);
            if (m_pathEdit) {
                m_pathEdit->setFixedWidth(maxW);
                m_pathEdit->adjustHeight();
            }
            
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
        bool hasPreview = (m_lblImagePreview && m_lblImagePreview->isVisible());
        bool hasPalette = (m_paletteFlowLayout->count() > 0);
        if (hasPreview || hasPalette) {
            m_topPreviewBox->show();
            m_topPreviewBox->setFixedHeight(qMax(32, contentH + (hasPreview ? 70 : 0)));
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
    m_nameEdit->setPlainText(info.completeBaseName());
    m_nameEdit->adjustHeight();
    m_nameEdit->setProperty("oldPath", p);
    m_nameEdit->setProperty("suffix", info.suffix());
    
    lblType->setText(t); 
    lblSize->setText(s); 
    lblCtime->setText(ct); 
    lblMtime->setText(mt); 
    lblAtime->setText(at); 
    
    m_pathEdit->setPlainText(p);
    m_pathEdit->adjustHeight();

    lblEncrypted->setText(e ? "已加密" : "未加密");
    
    if (width > 0 && height > 0) {
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
    while (QLayoutItem* item = m_tagFlowLayout->takeAt(0)) {
        TagPill* pill = qobject_cast<TagPill*>(item->widget());
        if (pill) {
            pill->hide();
            m_tagPool.append(pill);
        }
        delete item;
    }

    for (const QString& tag : tags) {
        TagPill* pill = nullptr;
        if (!m_tagPool.isEmpty()) {
            pill = m_tagPool.takeFirst();
            pill->setData(tag);
        } else {
            pill = new TagPill(tag, m_tagContainer);
            connect(pill, &TagPill::deleteRequested, this, &MetaPanel::onTagDeleted);
        }
        pill->setProperty("tagText", tag);
        pill->setStyleSheet("QFrame { background-color: #2D2D30; border: 1px solid #3E3E42; border-radius: 4px; }");
        pill->show();
        m_tagFlowLayout->addWidget(pill);
    }

    if (m_btnAddTagSmall) {
        m_tagFlowLayout->addWidget(m_btnAddTagSmall);
    }

    adjustFlowHeights();
    if (m_container) m_container->adjustSize();
    m_adjustTimer->start();
}

void MetaPanel::setRating(int rating) {
    m_currentRating = rating;
    for (int i = 0; i < m_starBtns.size(); ++i) {
        bool active = (i < rating);
        m_starBtns[i]->setIcon(UiHelper::getIcon(
            active ? "star_filled" : "star",
            active ? QColor("#FF551C") : QColor("#555555"),
            16
        ));
    }
}

void MetaPanel::setColor(const std::wstring& color) {
    m_currentColor = color;
    QString colorStr = QString::fromStdWString(color);
    static const QMap<QString, QString> s_colorHexMap = {
        {"红色", "#E24B4A"}, {"橙色", "#EF9F27"}, {"黄色", "#FECF0E"}, {"绿色", "#639922"},
        {"青色", "#1D9E75"}, {"蓝色", "#378ADD"}, {"紫色", "#7F77DD"}, {"灰色", "#5F5E5A"}
    };

    QString normInputHex = UiHelper::normalizeColorHex(colorStr);

    for (QPushButton* btn : m_colorBtns) {
        QString btnToolTip = btn->property("tooltipText").toString();
        QString hexColor = s_colorHexMap.value(btnToolTip, "#5F5E5A");
        QString normBtnHex = UiHelper::normalizeColorHex(hexColor);
        
        bool active = (!normInputHex.isEmpty() && normInputHex == normBtnHex) || (btnToolTip == colorStr);
        btn->setStyleSheet(QString(
            "QPushButton { background-color: %1; border: %2; border-radius: 8px; }"
            "QPushButton:hover { border-color: #FFFFFF; }"
        ).arg(hexColor).arg(active ? "2px solid #FFFFFF" : "1px solid transparent"));
    }
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
    m_linkEdit->setPlainText(url); 
    m_linkEdit->adjustHeight();
    if (m_btnOpenLink) m_btnOpenLink->setVisible(!url.trimmed().isEmpty());
    if (m_container) m_container->adjustSize();
    m_isInternalUpdating = false;
}

void MetaPanel::setURL(const std::wstring& url) {
    setURL(QString::fromStdWString(url));
}

void MetaPanel::setPalettes(const QVector<QPair<QColor, float>>& palette) {
    if (!m_paletteFlowLayout) return;

    while (QLayoutItem* item = m_paletteFlowLayout->takeAt(0)) {
        ColorPill* pill = qobject_cast<ColorPill*>(item->widget());
        if (pill) {
            pill->hide();
            m_colorPool.append(pill);
        }
        delete item;
    }

    for (const auto& entry : palette) {
        ColorPill* pill = nullptr;
        if (!m_colorPool.isEmpty()) {
            pill = m_colorPool.takeFirst();
            pill->setData(entry.first, entry.second);
        } else {
            pill = new ColorPill(entry.first, entry.second, m_topPreviewBox);
            pill->setStyleSheet("background: transparent; border: none;");
            connect(pill, &ColorPill::colorSelected, [this](const QColor& c){ emit searchByColor(c); });
            connect(pill, &ColorPill::requestSetAsPrimary, this, &MetaPanel::setAsPrimaryColor);
        }
        pill->show();
        m_paletteFlowLayout->addWidget(pill);
    }

    m_paletteFlowLayout->invalidate();
    if (m_topPreviewBox) m_topPreviewBox->update();
    adjustFlowHeights();
    m_adjustTimer->start();
}

bool MetaPanel::eventFilter(QObject* watched, QEvent* event) {
    // 拦截与接管自定义 ToolTipOverlay 显示，禁止使用 Windows/Qt 系统默认样式
    if (event->type() == QEvent::ToolTip) {
        QString text = watched->property("tooltipText").toString();
        if (!text.isEmpty()) {
            ToolTipOverlay::instance()->showText(QCursor::pos(), text, 0);
            return true; // 拦截系统默认 ToolTip
        }
    } else if (event->type() == QEvent::Leave || event->type() == QEvent::MouseButtonPress) {
        ToolTipOverlay::hideTip();
    }

    if (m_isInternalUpdating) return QFrame::eventFilter(watched, event);

    if (event->type() == QEvent::FocusIn) {
        if (watched == m_noteEdit || watched == m_linkEdit || watched == m_nameEdit) {
            m_isUserEditing = true;
        }
    } else if (event->type() == QEvent::FocusOut) {
        if (watched == m_noteEdit || watched == m_linkEdit || watched == m_nameEdit) {
            m_isUserEditing = false;
        }
    }

    if (watched == m_noteEdit && event->type() == QEvent::FocusOut) {
        if (!m_selectedPaths.isEmpty()) {
            QString newNote = m_noteEdit->toPlainText();
            emit noteEdited(m_selectedPaths, newNote);
        }
    } else if (watched == m_linkEdit) {
        if (event->type() == QEvent::FocusOut) {
            if (!m_selectedPaths.isEmpty()) {
                QString newUrl = m_linkEdit->toPlainText();
                emit linkEdited(m_selectedPaths, newUrl);
            }
        }
        if (m_btnOpenLink) {
            m_btnOpenLink->setVisible(!m_linkEdit->toPlainText().trimmed().isEmpty());
        }
    } else if (watched == m_nameEdit && event->type() == QEvent::FocusOut) {
        QString oldPath = m_nameEdit->property("oldPath").toString();
        QString newName = m_nameEdit->toPlainText().trimmed();
        
        static const QRegularExpression illegalRegex("[\\\\/:*?\"<>|]");
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

void MetaPanel::setAsPrimaryColor(const QColor& color) {
    QString currentPath = m_pathEdit->toPlainText().trimmed();
    if (!currentPath.isEmpty()) {
        emit primaryColorChanged(currentPath, color);
    }
}

} // namespace QuarkMeta
