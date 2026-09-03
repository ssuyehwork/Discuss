#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QToolBar>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QSystemTrayIcon>
#include <QSet>
#include <QSlider>
#include <QProgressBar>
#include <QDateTime>

#include "FramelessDialog.h"
#include "TagSelectorOverlay.h"
#include <QPointer>

namespace QuarkMeta {

class TrayController;
class HoverEventFilter;
class AddressBar;
class TaskProgressToolBar;
class SearchController; 
class NavPanel;
class FavoritePanel;
class ContentPanel;
class MetaPanel;
class FilterPanel;
class SearchHistoryPanel;
class AppShortcutController;
class PanelMediator;
class PanelLayoutManager;
class FramelessWindowHelper;

/**
 * @brief 主窗口类
 * 负责六栏布局的组装、QSplitter 管理及自定义标题栏按钮
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    PanelLayoutManager* layoutManager() const { return m_panelLayoutManager; }

public slots:
    void onStatusBarStatsUpdated(int fileCount, int folderCount, int totalCount);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void changeEvent(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    // 2026-04-11 按照用户要求：showEvent 是执行 ToolTipOverlay GPU 真实预热的唯一合法时机
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;

private slots:
    void onPinToggled(bool checked);

protected:
    void closeEvent(QCloseEvent* event) override;

private:

    QWidget* m_titleBarWidget = nullptr;
    QHBoxLayout* m_titleBarLayout = nullptr;
    QLabel* m_logoLabel = nullptr;
    QLabel* m_appNameLabel = nullptr;
    QWidget* m_navBarWidget = nullptr;
    QVBoxLayout* m_navBarMainLayout = nullptr;
    QWidget* m_navRow1Widget = nullptr;
    QHBoxLayout* m_navRow1Layout = nullptr;
    bool m_navBarIsTwoRowMode = false;
    QVBoxLayout* m_bodyLayout = nullptr; // 2026-05-08 按照用户要求：提升为成员变量以支持动态边距切换

    void updateNavBarResponsiveLayout();

    void initUi();
    void updateStatusBar();
    void initDriveBar();

    /**
     * @brief 统一导航调度向前兼容转调接口
     */
    void unifiedNavigateTo(const QString& url, bool record = true);

    void initToolbar();
    void setupSplitters();
    void setupCustomTitleBarButtons();
    void resetSplitterLayout();

    // 复合地址栏
    AddressBar* m_addressBar = nullptr;

    // 六个面板
    // 2026-04-11 按照用户要求：记录当前预览的文件路径，用于驱动方向键切图
    QString m_currentQuickLookPath;
    
    // UI Panels
    NavPanel* m_navPanel = nullptr;
    FavoritePanel* m_favoritePanel = nullptr;
    ContentPanel* m_contentPanel = nullptr;
    MetaPanel* m_metaPanel = nullptr;
    FilterPanel* m_filterPanel = nullptr;

    QSplitter* m_mainSplitter = nullptr;

    // 工具栏组件
    QToolBar* m_toolbar    = nullptr;
    QPushButton* m_btnBack    = nullptr;
    QPushButton* m_btnForward = nullptr;
    QPushButton* m_btnUp      = nullptr;

    SearchController* m_searchController = nullptr;
    
    // 排列方式视图按钮及中性缩放滑杆 (Modification_Plan-47)
    QPushButton* m_btnViewMenu = nullptr;
    QSlider* m_sizeSlider = nullptr;

    // 标题栏按钮组 (用于 frameless 时的模拟，此处作为标准按钮展示)
    QPushButton* m_btnToggleDriveBar = nullptr;
    QPushButton* m_btnLayout = nullptr;
    QPushButton* m_btnCreate = nullptr;
    QPushButton* m_btnPinTop = nullptr;
    QPushButton* m_btnMin = nullptr;
    QPushButton* m_btnMax = nullptr;
    QPushButton* m_btnClose = nullptr;

    // 盘符管理栏组件
    QWidget* m_driveBarWidget = nullptr;
    QHBoxLayout* m_driveBarLayout = nullptr;
    QPushButton* m_btnTagManager = nullptr;
    void onDriveBarContextMenu(const QPoint& pos);

    // 状态管理
    bool m_isPinned = false;
    QString m_currentDataSource; // "category" or "nav"
    bool m_panelsInitialized = false; // 2026-04-12 状态锁：确保面板仅初始化一次

    // 底部状态栏
    QLabel* m_statusLeft = nullptr;
    QWidget* m_statusBarWidget = nullptr;
    TaskProgressToolBar* m_taskProgressToolBar = nullptr;


    // 系统托盘控制器
    TrayController* m_trayController = nullptr;
    HoverEventFilter*     m_hoverFilter     = nullptr;
    QTimer* m_sidebarRefreshTimer = nullptr;

    // 模块化控制器与中介者
    AppShortcutController* m_shortcutController = nullptr;
    PanelMediator* m_panelMediator = nullptr;
    PanelLayoutManager*       m_panelLayoutManager = nullptr;
    FramelessWindowHelper*    m_framelessHelper = nullptr;

};

} // namespace QuarkMeta
