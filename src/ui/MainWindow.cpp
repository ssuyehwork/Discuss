#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "MainWindow.h"
#include "TitleBarWidget.h"
#include "NavBarWidget.h"
#include "DriveBarWidget.h"
#include "UiHelper.h"
#include "SearchHistoryPanel.h"
#include "AppShortcutController.h"
#include "PanelMediator.h"
#include "SearchController.h"
#include "NavPanel.h"
#include "FavoritePanel.h"
#include "ContentPanel.h"
#include "MetaPanel.h"
#include "FilterPanel.h"
#include "TrayController.h"
#include "HoverEventFilter.h"
#include "FramelessWindowHelper.h"
#include "PanelLayoutManager.h"
#include "AddressBar.h"
#include "ToolTipOverlay.h"
#include "TaskProgressToolBar.h"
#include "../core/AppConfig.h"
#include "../core/NavigationService.h"
#include "../core/CoreController.h"
#include "../core/ModelContract.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDir>
#include <QTimer>
#include <QCloseEvent>

namespace QuarkMeta {

constexpr int kLayoutEdgeMargin = 5;
constexpr int kStatusBarHorizontalMargin = 12;

MainWindow::~MainWindow() = default;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
    setMinimumSize(475, 400);
    setWindowTitle("QuarkMeta");

    m_hoverFilter = new HoverEventFilter(this);
    m_isPinned = AppConfig::instance().getValue("MainWindow/AlwaysOnTop", false).toBool();

    ToolTipOverlay::instance();

    initUi();

    m_framelessHelper = FramelessWindowHelper::apply(this, m_titleBarWidget);
    if (m_isPinned) {
        FramelessWindowHelper::setAlwaysOnTop(this, true);
    }

    m_trayController = new TrayController(this);
    m_trayController->show();

    QTimer::singleShot(200, []() {
        QString lastPath = AppConfig::instance().getValue("MainWindow/LastPath", "computer://").toString();
        bool isValid = lastPath.contains("://") || QDir(lastPath).exists();
        NavigationService::instance().navigateTo(isValid ? lastPath : "computer://");
    });
}

void MainWindow::initUi() {
    QByteArray savedGeom = AppConfig::instance().getValue("MainWindow/Geometry").toByteArray();
    if (!savedGeom.isEmpty()) {
        restoreGeometry(savedGeom);
    } else {
        resize(1180, 800);
    }

    QWidget* centralC = new QWidget(this);
    centralC->setObjectName("CentralWidget");
    QVBoxLayout* mainL = new QVBoxLayout(centralC);
    mainL->setContentsMargins(0, 0, 0, 0);
    mainL->setSpacing(0);

    // 1. 顶层子组件实例化 (TitleBar / NavBar / DriveBar)
    m_titleBarWidget = new TitleBarWidget(centralC, m_hoverFilter);
    m_navBarWidget   = new NavBarWidget(centralC, m_hoverFilter);
    m_driveBarWidget = new DriveBarWidget(centralC);

    m_addressBar       = m_navBarWidget->addressBar();
    m_searchController = m_navBarWidget->searchController();

    connect(m_titleBarWidget, &TitleBarWidget::driveBarToggleRequested, this, [this](bool visible) {
        if (m_driveBarWidget) m_driveBarWidget->setVisible(visible);
    });

    // 2. 主 Splitter 与 5 大 Panel 骨架挂载
    QWidget* bodyWrapper = new QWidget(centralC);
    bodyWrapper->setObjectName("BodyWrapper");
    m_bodyLayout = new QVBoxLayout(bodyWrapper);
    m_bodyLayout->setContentsMargins(kLayoutEdgeMargin, 0, kLayoutEdgeMargin, kLayoutEdgeMargin);
    m_bodyLayout->setSpacing(0);

    m_mainSplitter = new QSplitter(Qt::Horizontal, bodyWrapper);
    m_mainSplitter->setHandleWidth(5);
    m_mainSplitter->setChildrenCollapsible(false);

    m_navPanel      = new NavPanel(this);      m_navPanel->setObjectName("SidebarContainer");
    m_favoritePanel = new FavoritePanel(this); m_favoritePanel->setObjectName("FavoriteContainer");
    m_contentPanel  = new ContentPanel(this);  m_contentPanel->setObjectName("EditorContainer");
    m_metaPanel     = new MetaPanel(this);     m_metaPanel->setObjectName("MetadataContainer");
    m_filterPanel   = new FilterPanel(this);   m_filterPanel->setObjectName("FilterContainer");

    m_mainSplitter->addWidget(m_navPanel);
    m_mainSplitter->addWidget(m_favoritePanel);
    m_mainSplitter->addWidget(m_contentPanel);
    m_mainSplitter->addWidget(m_metaPanel);
    m_mainSplitter->addWidget(m_filterPanel);

    m_bodyLayout->addWidget(m_mainSplitter);

    m_titleBarWidget->bindContentPanel(m_contentPanel);

    // 3. 控制器停机坪挂载
    m_panelLayoutManager = new PanelLayoutManager(this, m_mainSplitter, m_navPanel, m_favoritePanel, m_contentPanel, m_metaPanel, m_filterPanel, this);
    m_panelLayoutManager->initLayout();
    m_titleBarWidget->bindLayoutManager(m_panelLayoutManager);

    m_panelMediator = new PanelMediator(m_navPanel, m_favoritePanel, m_contentPanel, m_metaPanel, m_filterPanel, m_addressBar, m_searchController, this);
    m_panelMediator->setupConnections();

    if (m_searchController) {
        m_searchController->bindContentPanel(m_contentPanel);
        connect(m_searchController, &SearchController::searchExecuted, this, &MainWindow::updateStatusBar);
    }

    m_shortcutController = new AppShortcutController(this, m_searchController, this);
    connect(m_shortcutController, &AppShortcutController::togglePinRequested, this, [this]() {
        if (m_titleBarWidget && m_titleBarWidget->btnPinTop()) {
            m_titleBarWidget->btnPinTop()->setChecked(!m_titleBarWidget->btnPinTop()->isChecked());
        }
    });
    connect(m_shortcutController, &AppShortcutController::toggleImmersiveRequested, this, [this]() {
        if (m_panelLayoutManager) m_panelLayoutManager->toggleImmersiveMode();
    });

    // 4. 底部状态栏
    m_statusBarWidget = new QWidget(centralC);
    m_statusBarWidget->setObjectName("StatusBar");
    m_statusBarWidget->setFixedHeight(32);
    QHBoxLayout* statusL = new QHBoxLayout(m_statusBarWidget);
    statusL->setContentsMargins(kStatusBarHorizontalMargin, 0, kStatusBarHorizontalMargin, 0);
    statusL->setSpacing(0);

    m_statusLeft = new QLabel("就绪中...", m_statusBarWidget);
    m_statusLeft->setObjectName("StatusBarLeft");
    statusL->addWidget(m_statusLeft);
    statusL->addStretch(1);

    connect(m_contentPanel, &ContentPanel::statusBarMessageReady, this, [this](const QString& msg) {
        if (m_statusLeft) m_statusLeft->setText(msg);
    });

    auto updateStatus = [this]() {
        m_statusLeft->setText(CoreController::instance().statusText());
        m_statusLeft->setProperty("indexing", CoreController::instance().isIndexing());
        m_statusLeft->style()->unpolish(m_statusLeft);
        m_statusLeft->style()->polish(m_statusLeft);
    };
    connect(&CoreController::instance(), &CoreController::statusTextChanged, this, updateStatus);
    connect(&CoreController::instance(), &CoreController::isIndexingChanged, this, updateStatus);
    updateStatus();

    m_taskProgressToolBar = new TaskProgressToolBar(centralC);
    m_taskProgressToolBar->hide();

    // 5. 将顶级部件拼装至主布局
    mainL->addWidget(m_titleBarWidget);
    mainL->addWidget(m_driveBarWidget);
    mainL->addWidget(m_navBarWidget);
    mainL->addWidget(bodyWrapper, 1);
    mainL->addWidget(m_statusBarWidget);
    mainL->addWidget(m_taskProgressToolBar);

    setCentralWidget(centralC);
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    if (!m_panelsInitialized) {
        m_panelsInitialized = true;
        QTimer::singleShot(0, [this]() {
            if (m_navPanel) m_navPanel->deferredInit();
            if (m_contentPanel) m_contentPanel->deferredInit();
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

void MainWindow::onStatusBarStatsUpdated(int fileCount, int folderCount, int totalCount) {
    Q_UNUSED(fileCount); Q_UNUSED(folderCount); Q_UNUSED(totalCount);
}

void MainWindow::updateStatusBar() {
    onStatusBarStatsUpdated(0, 0, 0);
}

void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized() && m_searchController && m_searchController->historyPanel()) {
            m_searchController->historyPanel()->hide();
        }
        if (m_titleBarWidget && m_titleBarWidget->btnMax()) {
            QString iconKey = isMaximized() ? "restore_line" : "maximize";
            m_titleBarWidget->btnMax()->setIcon(UiHelper::getIcon(iconKey, QColor("#EEEEEE")));
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

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
}

bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    if (m_framelessHelper && m_framelessHelper->handleNativeEvent(message, result)) {
        return true;
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}

} // namespace QuarkMeta
