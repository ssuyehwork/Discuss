#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "MainWindow.h"
#include "AppShortcutController.h"
#include "PanelMediator.h"
#include "SearchController.h" 
#include "SearchHistoryPanel.h" 
#include <QDateTime>
#include <algorithm>
#include "../meta/DiskNavigatorService.h"
#include "Logger.h"
#include "../core/UndoManager.h"
#include "../core/BasicCommands.h"
#include "TrayController.h"
#include "HoverEventFilter.h"
#include "FramelessWindowHelper.h"
#include "PanelLayoutManager.h"
#include "AddressBar.h"
#include "../core/CoreController.h"
#include "ColorPicker.h"
#include "NavPanel.h"
#include "FavoritePanel.h"
#include "ContentPanel.h"
#include "../core/CoreEngine.h"
#include "../core/CentralEventHub.h"
#include "MetaPanel.h"
#include "FilterPanel.h"
#include "../core/NavigationHistoryService.h"
#include "QuickLookWindow.h"
#include "ToolTipOverlay.h"
#include "../meta/DuplicateDetectorService.h"
#include "../util/DiskMediaExtractor.h"
#include "ShellIconManager.h"
#include "DuplicateConflictDialog.h"
#include "TaskProgressToolBar.h"
#include "../core/VolumeOnlineManager.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

#include "SearchHistoryPanel.h"
#include "SvgIcons.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSvgRenderer>
#include <QPainter>
#include <QIcon>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QCursor>
#include <QApplication>
#include "../core/AppConfig.h"
#include <QCloseEvent>
#include <QMenu>
#include <QAction>
#include <QWidgetAction>
#include <QGridLayout>
#include <QTimer>
#include "UiHelper.h"
#include "StyleLibrary.h"
#include "SvgIconRenderer.h"
#include "../core/SearchHistoryService.h"
#include "DriveButton.h"
#include "TagManagerDialog.h"
#include "../util/ShellHelper.h"
using namespace QuarkMeta::Style;
#include "../core/ModelContract.h"
#include <QFileInfo>
#include <QDir>
#include "../meta/MetadataManager.h"
#include "FramelessDialog.h"
#include "FramelessFileDialog.h"
#include "../core/NavigationService.h"
#include <QSlider>
#include <QStyle>
#include <QSignalBlocker>

#include <QtConcurrent>

namespace QuarkMeta {

constexpr int kLayoutEdgeMargin = 5;
constexpr int kStatusBarHorizontalMargin = 12;

MainWindow::~MainWindow() {
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_currentDataSource("nav") {
    m_panelsInitialized = false;

    ToolTipOverlay::instance();

    setMinimumSize(475, 400); 
    setWindowTitle("QuarkMeta");

    m_hoverFilter = new HoverEventFilter(this);

    m_isPinned = AppConfig::instance().getValue("MainWindow/AlwaysOnTop", false).toBool();

    initUi();

    FramelessWindowHelper::apply(this, m_titleBarWidget);
    if (m_isPinned) {
        FramelessWindowHelper::setAlwaysOnTop(this, true);
    }

    m_trayController = new TrayController(this);
    m_trayController->show();

    this->installEventFilter(this);

    QTimer::singleShot(200, []() {
        QString lastPath = AppConfig::instance().getValue("MainWindow/LastPath", "computer://").toString();
        bool isValid = lastPath.contains("://") || QDir(lastPath).exists();
        NavigationService::instance().navigateTo(isValid ? lastPath : "computer://");
    });
}

void MainWindow::initUi() {
    Q_ASSERT(m_hoverFilter && "事件过滤器必须在 initUi() 之前创建");

    // 【归一化修复】窗口自身尺寸必须先于分栏状态确定，否则splitter会基于错误的临时宽度做比例分配
    QByteArray savedGeom = AppConfig::instance().getValue("MainWindow/Geometry").toByteArray();
    if (!savedGeom.isEmpty()) {
        restoreGeometry(savedGeom);
    } else {
        resize(1180, 800);
    }

    initToolbar();
    setupSplitters();

    setupCustomTitleBarButtons();

    // 【归一化修复】splitter拉伸系数与尺寸恢复的唯一权威实现在 PanelLayoutManager::initLayout()，此处不再重复

    m_panelMediator = new PanelMediator(
        m_navPanel,
        m_favoritePanel,
        m_contentPanel,
        m_metaPanel,
        m_filterPanel,
        m_addressBar,
        m_searchController,
        this
    );
    m_panelMediator->setupConnections();

    if (m_searchController) {
        m_searchController->bindContentPanel(m_contentPanel);
        connect(m_searchController, &SearchController::searchExecuted, this, &MainWindow::updateStatusBar);
    }

    m_shortcutController = new AppShortcutController(this, m_searchController, this);
    connect(m_shortcutController, &AppShortcutController::togglePinRequested, this, [this]() {
        if (m_btnPinTop) {
            m_btnPinTop->setChecked(!m_btnPinTop->isChecked());
        }
    });
    connect(m_shortcutController, &AppShortcutController::toggleImmersiveRequested, this, [this]() {
        if (m_panelLayoutManager) {
            m_panelLayoutManager->toggleImmersiveMode();
        }
    });
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    if (!m_panelsInitialized) {
        m_panelsInitialized = true;
        QTimer::singleShot(0, [this]() {
            if (m_navPanel) {
                m_navPanel->deferredInit();
            }
            if (m_contentPanel) {
                m_contentPanel->deferredInit();
            }
        });
    }
    
    static bool s_warmedUp = false;
    if (!s_warmedUp) {
        s_warmedUp = true;
        QTimer::singleShot(0, []() {
            auto* tip = ToolTipOverlay::instance();
            tip->show();
            tip->hide();
        });
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    setAttribute(Qt::WA_Hover);
    QMainWindow::keyPressEvent(event);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::initToolbar() {
    auto createBtn = [this](const QString& iconKey, const QString& tip) -> QPushButton* {
        QPushButton* btn = new QPushButton(this);
        btn->setAttribute(Qt::WA_Hover);
        btn->setFixedSize(32, 28);
        
        QIcon icon = UiHelper::getIcon(iconKey, QColor("#EEEEEE"));
        btn->setIcon(icon);
        btn->setIconSize(QSize(18, 18));
        
        btn->setProperty("tooltipText", tip);
        btn->installEventFilter(this);

        btn->setObjectName("NavControlBtn");
        return btn;
    };

    m_btnBack = createBtn("nav_prev", "");
    m_btnBack->setProperty("tooltipText", "后退");
    m_btnBack->installEventFilter(m_hoverFilter);

    m_btnForward = createBtn("nav_next", "");
    m_btnForward->setProperty("tooltipText", "前进");
    m_btnForward->installEventFilter(m_hoverFilter);

    m_btnUp = createBtn("arrow_up", "");
    m_btnUp->setProperty("tooltipText", "上级");
    m_btnUp->installEventFilter(m_hoverFilter);

    connect(m_btnBack, &QPushButton::clicked, &NavigationService::instance(), &NavigationService::goBack);
    connect(m_btnForward, &QPushButton::clicked, &NavigationService::instance(), &NavigationService::goForward);
    connect(m_btnUp, &QPushButton::clicked, &NavigationService::instance(), &NavigationService::goUp);

    connect(&NavigationService::instance(), &NavigationService::navStateChanged, this,
            [this](bool canBack, bool canForward, bool canUp) {
        m_btnBack->setEnabled(canBack);
        m_btnForward->setEnabled(canForward);
        m_btnUp->setEnabled(canUp);
    });

    m_addressBar = new AddressBar(this);
    m_addressBar->setMinimumWidth(80);

    m_searchController = new SearchController(this);
}

void MainWindow::setupSplitters() {
    QWidget* centralC = new QWidget(this);
    centralC->setObjectName("CentralWidget");
    // CentralWidget style in style.qss 
    QVBoxLayout* mainL = new QVBoxLayout(centralC);
    mainL->setContentsMargins(0, 0, 0, 0); 
    mainL->setSpacing(0); 

    m_titleBarWidget = new QWidget(centralC);
    m_titleBarWidget->setObjectName("TitleBar");
    // TitleBar style in style.qss
    m_titleBarWidget->setFixedHeight(34);
    m_titleBarLayout = new QHBoxLayout(m_titleBarWidget);
    m_titleBarLayout->setContentsMargins(5, 0, kLayoutEdgeMargin, 0); 
    m_titleBarLayout->setSpacing(8);

    m_logoLabel = new QLabel(m_titleBarWidget);
    m_logoLabel->setFixedSize(18, 18);
    m_logoLabel->setPixmap(UiHelper::getIcon("ferrex", BrandOrange).pixmap(16, 16));
    m_logoLabel->setAlignment(Qt::AlignCenter);
    m_logoLabel->setObjectName("TitleLogoLabel");
    m_titleBarLayout->addWidget(m_logoLabel);

    m_appNameLabel = new QLabel("QuarkMeta", m_titleBarWidget);
    m_appNameLabel->setObjectName("AppNameLabel");
    m_titleBarLayout->addWidget(m_appNameLabel);
    m_titleBarLayout->addStretch();

    m_navBarWidget = new QWidget(centralC);
    m_navBarWidget->setObjectName("NavBar");
    m_navBarWidget->setFixedHeight(42); 

    m_navBarMainLayout = new QVBoxLayout(m_navBarWidget);
    m_navBarMainLayout->setContentsMargins(kLayoutEdgeMargin, 2, kLayoutEdgeMargin, 2);
    m_navBarMainLayout->setSpacing(2);

    m_navRow1Widget = new QWidget(m_navBarWidget);
    m_navRow1Layout = new QHBoxLayout(m_navRow1Widget);
    m_navRow1Layout->setContentsMargins(0, 0, 0, 0);
    m_navRow1Layout->setSpacing(5);
    m_navRow1Layout->setAlignment(Qt::AlignVCenter);

    m_navRow1Layout->addWidget(m_btnBack);
    m_navRow1Layout->addWidget(m_btnForward);
    m_navRow1Layout->addWidget(m_btnUp);
    m_navRow1Layout->addWidget(m_addressBar, 1);
    if (m_searchController && m_searchController->toolbarWidget()) {
        m_navRow1Layout->addWidget(m_searchController->toolbarWidget());
    }

    m_navBarMainLayout->addWidget(m_navRow1Widget);

    QWidget* bodyWrapper = new QWidget(centralC);
    bodyWrapper->setObjectName("BodyWrapper");
    m_bodyLayout = new QVBoxLayout(bodyWrapper);
    m_bodyLayout->setContentsMargins(kLayoutEdgeMargin, 0, kLayoutEdgeMargin, kLayoutEdgeMargin); 
    m_bodyLayout->setSpacing(0);

    // 5px 实体物理缝隙 (2px margin + 1px handle + 2px margin) + Dual-mode 深色样式
    m_mainSplitter = new QSplitter(Qt::Horizontal, bodyWrapper);
    m_mainSplitter->setHandleWidth(5); 
    m_mainSplitter->setChildrenCollapsible(false);
    // QSplitter style in style.qss

    m_navPanel = new NavPanel(this);
    m_navPanel->setObjectName("SidebarContainer");

    m_favoritePanel = new FavoritePanel(this);
    m_favoritePanel->setObjectName("FavoriteContainer");
    
    m_contentPanel = new ContentPanel(this);
    m_contentPanel->setObjectName("EditorContainer");
    
    m_metaPanel = new MetaPanel(this);
    m_metaPanel->setObjectName("MetadataContainer");
    
    m_filterPanel = new FilterPanel(this);
    m_filterPanel->setObjectName("FilterContainer");

    connect(m_contentPanel, &ContentPanel::dataSourceChanged, this, [this](const QString& source) {
        m_currentDataSource = source;
        if (m_navPanel) m_navPanel->setFocusHighlight(source == "nav");
    });

    m_mainSplitter->addWidget(m_navPanel);
    m_mainSplitter->addWidget(m_favoritePanel);
    m_mainSplitter->addWidget(m_contentPanel);
    m_mainSplitter->addWidget(m_metaPanel);
    m_mainSplitter->addWidget(m_filterPanel);

    m_panelLayoutManager = new PanelLayoutManager(
        this, m_mainSplitter,
        m_navPanel, m_favoritePanel, m_contentPanel, m_metaPanel, m_filterPanel,
        this
    );
    m_panelLayoutManager->initLayout();

    connect(m_contentPanel, &ContentPanel::statusBarStatsUpdated, this, &MainWindow::onStatusBarStatsUpdated);

    m_bodyLayout->addWidget(m_mainSplitter);

    m_statusBarWidget = new QWidget(centralC);
    m_statusBarWidget->setObjectName("StatusBar");
    m_statusBarWidget->setFixedHeight(28);
    QHBoxLayout* statusL = new QHBoxLayout(m_statusBarWidget);
    statusL->setContentsMargins(kStatusBarHorizontalMargin, 0, kStatusBarHorizontalMargin, 0);
    statusL->setSpacing(0);

    m_statusLeft = new QLabel("就绪中...", m_statusBarWidget);
    m_statusLeft->setObjectName("StatusBarLeft");

    statusL->addWidget(m_statusLeft);
    statusL->addStretch(1);

    auto updateStatus = [this]() {
        m_statusLeft->setText(CoreController::instance().statusText());
        if (CoreController::instance().isIndexing()) {
            m_statusLeft->setProperty("indexing", true);
            m_statusLeft->style()->unpolish(m_statusLeft);
            m_statusLeft->style()->polish(m_statusLeft);
        } else {
            m_statusLeft->setProperty("indexing", false);
            m_statusLeft->style()->unpolish(m_statusLeft);
            m_statusLeft->style()->polish(m_statusLeft);
        }
    };
    connect(&CoreController::instance(), &CoreController::statusTextChanged, this, updateStatus);
    connect(&CoreController::instance(), &CoreController::isIndexingChanged, this, updateStatus);
    updateStatus();

    initDriveBar();

    m_taskProgressToolBar = new TaskProgressToolBar(centralC);
    m_taskProgressToolBar->hide();

    mainL->addWidget(m_titleBarWidget);
    mainL->addWidget(m_driveBarWidget);
    mainL->addWidget(m_navBarWidget);
    mainL->addWidget(bodyWrapper, 1);
    mainL->addWidget(m_statusBarWidget);
    mainL->addWidget(m_taskProgressToolBar);

    setCentralWidget(centralC);
}

void MainWindow::setupCustomTitleBarButtons() {
    QWidget* titleBarBtns = new QWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(titleBarBtns);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    auto createTitleBtn = [this](const QString& iconKey) -> QPushButton* {
                QPushButton* btn = new QPushButton(this);
        btn->setAttribute(Qt::WA_Hover);
        btn->setFixedSize(24, 24);
        
        QIcon icon = UiHelper::getIcon(iconKey, QColor("#EEEEEE"));
        btn->setIcon(icon);
        btn->setIconSize(QSize(18, 18));
        
        btn->setObjectName("TitleControlBtn");
        return btn;
    };

    m_btnViewMenu = createTitleBtn("grid");  
    m_btnViewMenu->setProperty("tooltipText", "排列方式"); 
    m_btnViewMenu->installEventFilter(m_hoverFilter); 
 
    connect(m_btnViewMenu, &QPushButton::clicked, this, [this]() { 
        QMenu menu(this); 
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
            "QMenu::item:checked { color: #ff551c; }"
            "QMenu::item:checked:selected { color: #ff551c; }"
            "QMenu::indicator:checked { image: url(%1); width: 14px; height: 14px; left: 4px; }"
        ).arg(checkPath)); 
 
        connect(actAdaptive, &QAction::triggered, this, [this]() { 
            m_contentPanel->setViewMode(ContentPanel::JustifiedViewMode); 
        }); 
        connect(actGrid, &QAction::triggered, this, [this]() { 
            m_contentPanel->setViewMode(ContentPanel::GridView); 
        }); 
        connect(actList, &QAction::triggered, this, [this]() { 
            m_contentPanel->setViewMode(ContentPanel::ListView); 
        }); 
 
        menu.exec(m_btnViewMenu->mapToGlobal(QPoint(0, m_btnViewMenu->height()))); 
    }); 

    m_sizeSlider = new QSlider(Qt::Horizontal, m_titleBarWidget); 
    m_sizeSlider->setRange(30, 230);  
    m_sizeSlider->setFixedSize(110, 20); 
    m_sizeSlider->setCursor(Qt::PointingHandCursor); 
    m_sizeSlider->setObjectName("SizeSlider");
     
    connect(m_sizeSlider, &QSlider::valueChanged, this, [this](int value) { 
        m_contentPanel->setZoomLevel(value); 
    }); 

    connect(m_contentPanel, &ContentPanel::zoomLevelChanged, this, [this](int level) { 
        QSignalBlocker blocker(m_sizeSlider); 
        m_sizeSlider->setValue(level); 
    }); 
 
    int initZoom = AppConfig::instance().getValue("UI/GridZoomLevel", 96).toInt(); 
    m_sizeSlider->setValue(qBound(30, initZoom, 230)); 

    m_btnToggleDriveBar = createTitleBtn("chevrons_down");
    m_btnToggleDriveBar->setProperty("tooltipText", "展开/收起盘符管理栏");
    m_btnToggleDriveBar->installEventFilter(m_hoverFilter);
    m_btnToggleDriveBar->setCheckable(true);
    m_btnToggleDriveBar->setChecked(true);
    connect(m_btnToggleDriveBar, &QPushButton::toggled, this, [this](bool checked) {
        if (m_driveBarWidget) {
            m_driveBarWidget->setVisible(checked);
            m_btnToggleDriveBar->setIcon(UiHelper::getIcon(checked ? "chevrons_down" : "chevrons_up", QColor("#EEEEEE")));
        }
    });

    m_btnLayout = createTitleBtn("layout");
    m_btnLayout->setProperty("tooltipText", "布局管理与重置");
    m_btnLayout->installEventFilter(m_hoverFilter);
    connect(m_btnLayout, &QPushButton::clicked, this, [this]() {
        if (m_panelLayoutManager) {
            m_panelLayoutManager->showPanelContextMenu(m_btnLayout->mapToGlobal(QPoint(0, m_btnLayout->height())));
        }
    });

    m_btnCreate = createTitleBtn("add");
    m_btnCreate->setProperty("tooltipText", "新建...");
    QMenu* createMenu = new QMenu(m_btnCreate);
    UiHelper::applyMenuStyle(createMenu);
    
    QAction* actNewFolder = createMenu->addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "创建文件夹");
    QAction* actNewMd     = createMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建 Markdown");
    QAction* actNewTxt    = createMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建纯文本文件 (txt)");
    
    connect(m_btnCreate, &QPushButton::clicked, this, [this, createMenu]() {
        createMenu->popup(m_btnCreate->mapToGlobal(QPoint(0, m_btnCreate->height())));
    });

    auto handleCreate = [this](const QString& type) {
        m_contentPanel->createNewItem(type);
    };
    connect(actNewFolder, &QAction::triggered, this, [handleCreate](){ handleCreate("folder"); });
    connect(actNewMd,     &QAction::triggered, this, [handleCreate](){ handleCreate("md"); });
    connect(actNewTxt,    &QAction::triggered, this, [handleCreate](){ handleCreate("txt"); });

    m_btnPinTop = createTitleBtn(m_isPinned ? "pin_vertical" : "pin_tilted");
    m_btnPinTop->setProperty("tooltipText", "置顶窗口");
    m_btnPinTop->installEventFilter(m_hoverFilter);
    m_btnPinTop->setCheckable(true);
    m_btnPinTop->setChecked(m_isPinned);
    if (m_isPinned) {
        m_btnPinTop->setIcon(UiHelper::getIcon("pin_vertical", Style::ActiveOrange));
    }

    m_btnMin = createTitleBtn("minimize");
    m_btnMin->setProperty("tooltipText", "最小化");
    m_btnMin->installEventFilter(m_hoverFilter);

    m_btnMax = createTitleBtn(isMaximized() ? "restore_line" : "maximize");
    m_btnMax->setProperty("tooltipText", "最大化/还原");
    m_btnMax->installEventFilter(m_hoverFilter);

    m_btnClose = createTitleBtn("close");
    m_btnClose->setObjectName("TitleCloseBtn");
    m_btnClose->setProperty("tooltipText", "关闭项目");
    m_btnClose->installEventFilter(m_hoverFilter);

    m_btnCreate->installEventFilter(m_hoverFilter);

    layout->addWidget(m_btnToggleDriveBar, 0, Qt::AlignVCenter);
    layout->addWidget(m_btnLayout, 0, Qt::AlignVCenter);
    layout->addWidget(m_btnCreate, 0, Qt::AlignVCenter);
    layout->addWidget(m_btnPinTop, 0, Qt::AlignVCenter);
    layout->addWidget(m_btnMin, 0, Qt::AlignVCenter);
    layout->addWidget(m_btnMax, 0, Qt::AlignVCenter);
    layout->addWidget(m_btnClose, 0, Qt::AlignVCenter);

    connect(m_btnMin, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(m_btnMax, &QPushButton::clicked, this, [this]() {
        if (isMaximized()) showNormal();
        else showMaximized();
    });
    connect(m_btnClose, &QPushButton::clicked, this, &MainWindow::close);

    if (m_titleBarLayout) {
        m_titleBarLayout->addWidget(m_sizeSlider, 0, Qt::AlignVCenter);
        m_titleBarLayout->addWidget(m_btnViewMenu, 0, Qt::AlignVCenter);
        m_titleBarLayout->addWidget(titleBarBtns);
    }

    connect(m_btnPinTop, &QPushButton::toggled, this, &MainWindow::onPinToggled);
}

void MainWindow::unifiedNavigateTo(const QString& url, bool record) {
    NavigationService::instance().navigateTo(url, record);
}

void MainWindow::onStatusBarStatsUpdated(int fileCount, int folderCount, int totalCount) {
    if (!m_statusLeft || !m_contentPanel || !m_contentPanel->getProxyModel()) return;

    int visibleCount = m_contentPanel->getProxyModel()->rowCount();
    int fullCount = m_contentPanel->model() ? m_contentPanel->model()->rowCount() : visibleCount;
    int hiddenCount = fullCount - visibleCount;
    int selectedCount = m_contentPanel->getSelectedIndexes().size();

    QString statusText;
    if (hiddenCount > 0) {
        statusText = QString("%1个项目，%2个已隐藏，选中了%3个")
                     .arg(visibleCount).arg(hiddenCount).arg(selectedCount);
    } else {
        statusText = QString("%1个项目，选中了%2个")
                     .arg(visibleCount).arg(selectedCount);
    }

    m_statusLeft->setText(statusText);

    Q_UNUSED(fileCount);
    Q_UNUSED(folderCount);
    Q_UNUSED(totalCount);
}

void MainWindow::updateStatusBar() {
    onStatusBarStatsUpdated(0, 0, 0);
}

void MainWindow::onPinToggled(bool checked) {
    if (m_isPinned == checked) return;
    m_isPinned = checked;

    FramelessWindowHelper::setAlwaysOnTop(this, checked);

    if (m_isPinned) {
        m_btnPinTop->setIcon(UiHelper::getIcon("pin_vertical", Style::ActiveOrange));
    } else {
        m_btnPinTop->setIcon(UiHelper::getIcon("pin_tilted", TextMain));
    }

    AppConfig::instance().setValue("MainWindow/AlwaysOnTop", m_isPinned);
}

void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized() && m_searchController && m_searchController->historyPanel()) {
            m_searchController->historyPanel()->hide();
        }

        if (m_btnMax) {
            QString iconKey = isMaximized() ? "restore_line" : "maximize";
            m_btnMax->setIcon(UiHelper::getIcon(iconKey, QColor("#EEEEEE")));
        }

        if (m_bodyLayout) {
            m_bodyLayout->setContentsMargins(kLayoutEdgeMargin, 0, kLayoutEdgeMargin, kLayoutEdgeMargin);
        }
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    AppConfig::instance().setValue("MainWindow/LastPath", NavigationService::instance().currentUrl());
    AppConfig::instance().setValue("MainWindow/Geometry", saveGeometry());

    if (m_panelLayoutManager) {
        m_panelLayoutManager->saveLayoutState();
    }
    AppConfig::instance().sync();

    QMainWindow::closeEvent(event);
}

void MainWindow::initDriveBar() {
    m_driveBarWidget = new QWidget(this);
    m_driveBarWidget->setObjectName("DriveBar");
    m_driveBarWidget->setFixedHeight(42);
    // DriveBar style in style.qss
    m_driveBarWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_driveBarWidget, &QWidget::customContextMenuRequested, this, &MainWindow::onDriveBarContextMenu);

    m_driveBarLayout = new QHBoxLayout(m_driveBarWidget);
    m_driveBarLayout->setContentsMargins(15, 5, 15, 5);
    m_driveBarLayout->setSpacing(8);

    m_btnTagManager = new QPushButton(UiHelper::getIcon("tag", QColor("#1abc9c"), 18), " 标签管理", m_driveBarWidget);
    m_btnTagManager->setFixedHeight(28);
    m_btnTagManager->setCursor(Qt::PointingHandCursor);
    m_btnTagManager->setObjectName("BtnTagManager");

    connect(m_btnTagManager, &QPushButton::clicked, this, [this]() {
        TagManagerDialog::showDialog(this, NavigationService::instance().currentUrl(), false);
    });

    m_driveBarLayout->addWidget(m_btnTagManager);
    m_driveBarLayout->addStretch();
}

void MainWindow::onDriveBarContextMenu(const QPoint& pos) {
    Q_UNUSED(pos);
}

void MainWindow::updateNavBarResponsiveLayout() {
    if (!m_navBarWidget || !m_searchController || !m_searchController->toolbarWidget()) return;

    QWidget* searchW = m_searchController->toolbarWidget();
    QLineEdit* searchEdit = m_searchController->searchEdit();

    // 规则一：响应式折行
    // 窄屏判别阈值：当 m_navBarWidget 宽度不足以容纳 [前进/后退/上级]+[最小地址栏]+[搜索框] 时（约 650px）
    bool needTwoRow = (m_navBarWidget->width() < 650);

    if (needTwoRow && !m_navBarIsTwoRowMode) {
        m_navBarIsTwoRowMode = true;
        m_navRow1Layout->removeWidget(searchW);
        m_navBarMainLayout->addWidget(searchW);
        if (searchEdit) {
            searchEdit->setFixedWidth(QWIDGETSIZE_MAX);
            searchEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        }
        searchW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_navBarWidget->setFixedHeight(78);
    } else if (!needTwoRow && m_navBarIsTwoRowMode) {
        m_navBarIsTwoRowMode = false;
        m_navBarMainLayout->removeWidget(searchW);
        m_navRow1Layout->addWidget(searchW);
        if (searchEdit) {
            searchEdit->setFixedSize(230, 32);
        }
        searchW->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        m_navBarWidget->setFixedHeight(42);
    }
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    updateNavBarResponsiveLayout();
}

bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    Q_UNUSED(eventType);
#ifdef Q_OS_WIN
    MSG* msg = static_cast<MSG*>(message);

    if (msg->message == WM_NCCALCSIZE) {
        // 告诉Windows：客户区 = 整个窗口矩形（不留标题栏/边框空间，因为我们自己画标题栏）
        *result = 0;
        return true;
    }

    if (msg->message == WM_GETMINMAXINFO) {
        // 修正最大化时的尺寸，让它精确匹配"工作区"（即排除任务栏后的可用屏幕区域）
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(msg->lParam);
        HMONITOR monitor = MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONEAREST);
        if (monitor) {
            MONITORINFO monitorInfo = {};
            monitorInfo.cbSize = sizeof(MONITORINFO);
            GetMonitorInfo(monitor, &monitorInfo);

            RECT workArea = monitorInfo.rcWork;
            RECT monitorArea = monitorInfo.rcMonitor;

            mmi->ptMaxPosition.x = workArea.left - monitorArea.left;
            mmi->ptMaxPosition.y = workArea.top - monitorArea.top;
            mmi->ptMaxSize.x = workArea.right - workArea.left;
            mmi->ptMaxSize.y = workArea.bottom - workArea.top;
        }
        *result = 0;
        return true;
    }
#else
    Q_UNUSED(message);
    Q_UNUSED(result);
#endif
    return false;
}

} // namespace QuarkMeta