#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include "ContentPanel.h"

namespace QuarkMeta {

class HoverEventFilter;

/**
 * @brief 独立标题栏组件
 * 封装 LOGO、应用名称、缩放滑杆、排列视图菜单、新建菜单、盘符折叠按钮、布局重置、窗口控制按钮(置顶/最小化/最大化/关闭)
 * 纯 View 部件：不依赖 ContentPanel/PanelLayoutManager 指针，不直接读写 AppConfig。
 */
class TitleBarWidget : public QWidget {
    Q_OBJECT

public:
    explicit TitleBarWidget(QWidget* parent = nullptr, HoverEventFilter* hoverFilter = nullptr);
    ~TitleBarWidget() override = default;

    QPushButton* btnPinTop() const { return m_btnPinTop; }
    QPushButton* btnMin() const { return m_btnMin; }
    QPushButton* btnMax() const { return m_btnMax; }
    QPushButton* btnClose() const { return m_btnClose; }
    QPushButton* btnToggleDriveBar() const { return m_btnToggleDriveBar; }
    QPushButton* btnLayout() const { return m_btnLayout; }
    QPushButton* btnCreate() const { return m_btnCreate; }
    QPushButton* btnViewMenu() const { return m_btnViewMenu; }
    QSlider* sizeSlider() const { return m_sizeSlider; }

    bool isPinned() const;
    void setPinned(bool pinned);
    void setZoomLevel(int value);
    void setWindowMaximized(bool maximized);

signals:
    void driveBarToggleRequested(bool visible);
    void pinToggled(bool pinned);
    void zoomLevelChanged(int value);
    void viewModeRequested(ContentPanel::ViewMode mode);
    void createItemRequested(const QString& type);
    void layoutMenuRequested(const QPoint& globalPos);

private:
    void initUi(HoverEventFilter* hoverFilter);
    void setupViewMenu();
    void setupCreateMenu();

    QHBoxLayout* m_layout = nullptr;
    QLabel* m_logoLabel = nullptr;
    QLabel* m_appNameLabel = nullptr;

    QPushButton* m_btnViewMenu = nullptr;
    QSlider* m_sizeSlider = nullptr;

    QPushButton* m_btnToggleDriveBar = nullptr;
    QPushButton* m_btnLayout = nullptr;
    QPushButton* m_btnCreate = nullptr;
    QPushButton* m_btnPinTop = nullptr;
    QPushButton* m_btnMin = nullptr;
    QPushButton* m_btnMax = nullptr;
    QPushButton* m_btnClose = nullptr;

    ContentPanel::ViewMode m_currentViewMode = ContentPanel::GridView;
};

} // namespace QuarkMeta
