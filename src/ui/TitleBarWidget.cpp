#include "TitleBarWidget.h"
#include "UiHelper.h"
#include "ContentPanel.h"
#include "PanelLayoutManager.h"
#include "HoverEventFilter.h"
#include "SvgIconRenderer.h"
#include "StyleLibrary.h"
#include "../core/AppConfig.h"
#include "FramelessWindowHelper.h"

#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QSignalBlocker>

namespace QuarkMeta {

constexpr int kLayoutEdgeMargin = 5;

TitleBarWidget::TitleBarWidget(QWidget* parent, HoverEventFilter* hoverFilter)
    : QWidget(parent) {
    setObjectName("TitleBar");
    setFixedHeight(34);
    m_isPinned = AppConfig::instance().getValue("MainWindow/AlwaysOnTop", false).toBool();
    initUi(hoverFilter);
}

void TitleBarWidget::bindContentPanel(ContentPanel* contentPanel) {
    m_contentPanel = contentPanel;
    if (!m_contentPanel) return;

    connect(m_sizeSlider, &QSlider::valueChanged, this, [this](int value) {
        if (m_contentPanel) m_contentPanel->setZoomLevel(value);
    });

    connect(m_contentPanel, &ContentPanel::zoomLevelChanged, this, [this](int level) {
        QSignalBlocker blocker(m_sizeSlider);
        m_sizeSlider->setValue(level);
    });

    int initZoom = AppConfig::instance().getValue("UI/GridZoomLevel", 96).toInt();
    m_sizeSlider->setValue(qBound(30, initZoom, 230));
}

void TitleBarWidget::bindLayoutManager(PanelLayoutManager* layoutManager) {
    m_layoutManager = layoutManager;
}

void TitleBarWidget::initUi(HoverEventFilter* hoverFilter) {
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(5, 0, kLayoutEdgeMargin, 0);
    m_layout->setSpacing(8);

    m_logoLabel = new QLabel(this);
    m_logoLabel->setFixedSize(18, 18);
    m_logoLabel->setPixmap(UiHelper::getIcon("ferrex", Style::BrandOrange).pixmap(16, 16));
    m_logoLabel->setAlignment(Qt::AlignCenter);
    m_logoLabel->setObjectName("TitleLogoLabel");
    m_layout->addWidget(m_logoLabel);

    m_appNameLabel = new QLabel("QuarkMeta", this);
    m_appNameLabel->setObjectName("AppNameLabel");
    m_layout->addWidget(m_appNameLabel);
    m_layout->addStretch();

    auto createTitleBtn = [this, hoverFilter](const QString& iconKey, const QString& tip) -> QPushButton* {
        QPushButton* btn = new QPushButton(this);
        btn->setAttribute(Qt::WA_Hover);
        btn->setFixedSize(24, 24);
        btn->setIcon(UiHelper::getIcon(iconKey, QColor("#EEEEEE")));
        btn->setIconSize(QSize(18, 18));
        btn->setObjectName("TitleControlBtn");
        btn->setProperty("tooltipText", tip);
        if (hoverFilter) {
            btn->installEventFilter(hoverFilter);
        }
        return btn;
    };

    m_btnViewMenu = createTitleBtn("grid", "排列方式");
    setupViewMenu();

    m_sizeSlider = new QSlider(Qt::Horizontal, this);
    m_sizeSlider->setRange(30, 230);
    m_sizeSlider->setFixedSize(110, 20);
    m_sizeSlider->setCursor(Qt::PointingHandCursor);
    m_sizeSlider->setObjectName("SizeSlider");

    m_btnToggleDriveBar = createTitleBtn("chevrons_down", "展开/收起盘符管理栏");
    m_btnToggleDriveBar->setCheckable(true);
    m_btnToggleDriveBar->setChecked(true);
    connect(m_btnToggleDriveBar, &QPushButton::toggled, this, [this](bool checked) {
        m_btnToggleDriveBar->setIcon(UiHelper::getIcon(checked ? "chevrons_down" : "chevrons_up", QColor("#EEEEEE")));
        emit driveBarToggleRequested(checked);
    });

    m_btnLayout = createTitleBtn("layout", "布局管理与重置");
    connect(m_btnLayout, &QPushButton::clicked, this, [this]() {
        if (m_layoutManager) {
            m_layoutManager->showPanelContextMenu(m_btnLayout->mapToGlobal(QPoint(0, m_btnLayout->height())));
        }
    });

    m_btnCreate = createTitleBtn("add", "新建...");
    setupCreateMenu();

    m_btnPinTop = createTitleBtn(m_isPinned ? "pin_vertical" : "pin_tilted", "置顶窗口");
    m_btnPinTop->setCheckable(true);
    m_btnPinTop->setChecked(m_isPinned);
    if (m_isPinned) {
        m_btnPinTop->setIcon(UiHelper::getIcon("pin_vertical", Style::ActiveOrange));
    }

    connect(m_btnPinTop, &QPushButton::toggled, this, [this](bool checked) {
        m_isPinned = checked;
        QWidget* topWin = window();
        if (topWin) {
            FramelessWindowHelper::setAlwaysOnTop(topWin, checked);
        }
        m_btnPinTop->setIcon(UiHelper::getIcon(checked ? "pin_vertical" : "pin_tilted", checked ? Style::ActiveOrange : Style::TextMain));
        AppConfig::instance().setValue("MainWindow/AlwaysOnTop", checked);
        emit pinToggled(checked);
    });

    m_btnMin = createTitleBtn("minimize", "最小化");
    m_btnMax = createTitleBtn("maximize", "最大化/还原");
    m_btnClose = createTitleBtn("close", "关闭项目");
    m_btnClose->setObjectName("TitleCloseBtn");

    connect(m_btnMin, &QPushButton::clicked, this, [this]() {
        if (window()) window()->showMinimized();
    });
    connect(m_btnMax, &QPushButton::clicked, this, [this]() {
        if (window()) {
            if (window()->isMaximized()) window()->showNormal();
            else window()->showMaximized();
        }
    });
    connect(m_btnClose, &QPushButton::clicked, this, [this]() {
        if (window()) window()->close();
    });

    m_layout->addWidget(m_sizeSlider, 0, Qt::AlignVCenter);
    m_layout->addWidget(m_btnViewMenu, 0, Qt::AlignVCenter);
    m_layout->addWidget(m_btnToggleDriveBar, 0, Qt::AlignVCenter);
    m_layout->addWidget(m_btnLayout, 0, Qt::AlignVCenter);
    m_layout->addWidget(m_btnCreate, 0, Qt::AlignVCenter);
    m_layout->addWidget(m_btnPinTop, 0, Qt::AlignVCenter);
    m_layout->addWidget(m_btnMin, 0, Qt::AlignVCenter);
    m_layout->addWidget(m_btnMax, 0, Qt::AlignVCenter);
    m_layout->addWidget(m_btnClose, 0, Qt::AlignVCenter);
}

void TitleBarWidget::setupViewMenu() {
    connect(m_btnViewMenu, &QPushButton::clicked, this, [this]() {
        if (!m_contentPanel) return;

        QMenu menu(this);
        menu.setObjectName("TitleBarViewModeMenu");
        UiHelper::applyMenuStyle(&menu);

        QAction* actAdaptive = menu.addAction("自适应(A)");
        QAction* actGrid = menu.addAction("网格(G)");
        QAction* actList = menu.addAction("列表(L)");

        actAdaptive->setCheckable(true);
        actGrid->setCheckable(true);
        actList->setCheckable(true);

        ContentPanel::ViewMode mode = m_contentPanel->currentViewMode();
        actAdaptive->setChecked(mode == ContentPanel::JustifiedViewMode);
        actGrid->setChecked(mode == ContentPanel::GridView);
        actList->setChecked(mode == ContentPanel::ListView);

        QString checkPath = SvgIconRenderer::getSvgTempFilePath("check", QColor("#ff551c"));
        menu.setStyleSheet(menu.styleSheet() + QString(
            "QMenu#TitleBarViewModeMenu::indicator:checked { image: url(%1); }"
        ).arg(checkPath));

        connect(actAdaptive, &QAction::triggered, this, [this]() {
            if (m_contentPanel) m_contentPanel->setViewMode(ContentPanel::JustifiedViewMode);
        });
        connect(actGrid, &QAction::triggered, this, [this]() {
            if (m_contentPanel) m_contentPanel->setViewMode(ContentPanel::GridView);
        });
        connect(actList, &QAction::triggered, this, [this]() {
            if (m_contentPanel) m_contentPanel->setViewMode(ContentPanel::ListView);
        });

        menu.exec(m_btnViewMenu->mapToGlobal(QPoint(0, m_btnViewMenu->height())));
    });
}

void TitleBarWidget::setupCreateMenu() {
    QMenu* createMenu = new QMenu(m_btnCreate);
    UiHelper::applyMenuStyle(createMenu);

    QAction* actNewFolder = createMenu->addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "创建文件夹");
    QAction* actNewMd     = createMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建 Markdown");
    QAction* actNewTxt    = createMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建纯文本文件 (txt)");

    connect(m_btnCreate, &QPushButton::clicked, this, [this, createMenu]() {
        createMenu->popup(m_btnCreate->mapToGlobal(QPoint(0, m_btnCreate->height())));
    });

    auto handleCreate = [this](const QString& type) {
        if (m_contentPanel) m_contentPanel->createNewItem(type);
    };
    connect(actNewFolder, &QAction::triggered, this, [handleCreate](){ handleCreate("folder"); });
    connect(actNewMd,     &QAction::triggered, this, [handleCreate](){ handleCreate("md"); });
    connect(actNewTxt,    &QAction::triggered, this, [handleCreate](){ handleCreate("txt"); });
}

} // namespace QuarkMeta
