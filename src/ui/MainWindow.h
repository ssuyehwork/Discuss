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
class TitleBarWidget;
class DriveBarWidget;
class NavBarWidget;
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
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
    void closeEvent(QCloseEvent* event) override;

private:

    TitleBarWidget* m_titleBarWidget = nullptr;
    NavBarWidget* m_navBarWidget = nullptr;
    DriveBarWidget* m_driveBarWidget = nullptr;
    QVBoxLayout* m_bodyLayout = nullptr;

    void initUi();
    void updateStatusBar();

    // 导航与搜索组件句柄
    AddressBar* m_addressBar = nullptr;
    SearchController* m_searchController = nullptr;

    // 5 大核心面板与 Splitter
    NavPanel* m_navPanel = nullptr;
    FavoritePanel* m_favoritePanel = nullptr;
    ContentPanel* m_contentPanel = nullptr;
    MetaPanel* m_metaPanel = nullptr;
    FilterPanel* m_filterPanel = nullptr;

    QSplitter* m_mainSplitter = nullptr;

    // 状态管理
    bool m_isPinned = false;
    bool m_panelsInitialized = false; // 2026-04-12 状态锁：确保面板仅初始化一次

    // 底部状态栏
    QLabel* m_statusLeft = nullptr;
    QWidget* m_statusBarWidget = nullptr;
    TaskProgressToolBar* m_taskProgressToolBar = nullptr;

    // 系统托盘控制器
    TrayController* m_trayController = nullptr;
    HoverEventFilter*     m_hoverFilter     = nullptr;

    // 模块化控制器与中介者
    AppShortcutController* m_shortcutController = nullptr;
    PanelMediator* m_panelMediator = nullptr;
    PanelLayoutManager*       m_panelLayoutManager = nullptr;
    FramelessWindowHelper*    m_framelessHelper = nullptr;

};

} // namespace QuarkMeta
