#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>

namespace QuarkMeta {

class ContentPanel;
class PanelLayoutManager;
class HoverEventFilter;

/**
 * @brief 独立标题栏组件
 * 封装 LOGO、应用名称、缩放滑杆、排列视图菜单、新建菜单、盘符折叠按钮、布局重置、窗口控制按钮(置顶/最小化/最大化/关闭)
 */
class TitleBarWidget : public QWidget {
    Q_OBJECT

public:
    explicit TitleBarWidget(QWidget* parent = nullptr, HoverEventFilter* hoverFilter = nullptr);
    ~TitleBarWidget() override = default;

    void bindContentPanel(ContentPanel* contentPanel);
    void bindLayoutManager(PanelLayoutManager* layoutManager);

    QPushButton* btnPinTop() const { return m_btnPinTop; }
    QPushButton* btnMin() const { return m_btnMin; }
    QPushButton* btnMax() const { return m_btnMax; }
    QPushButton* btnClose() const { return m_btnClose; }
    QPushButton* btnToggleDriveBar() const { return m_btnToggleDriveBar; }
    QPushButton* btnLayout() const { return m_btnLayout; }
    QPushButton* btnCreate() const { return m_btnCreate; }
    QPushButton* btnViewMenu() const { return m_btnViewMenu; }
    QSlider* sizeSlider() const { return m_sizeSlider; }

    void updateMaxButtonIcon();

protected:
    void showEvent(QShowEvent* event) override;

signals:
    void driveBarToggleRequested(bool visible);
    void pinToggled(bool pinned);

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

    ContentPanel* m_contentPanel = nullptr;
    PanelLayoutManager* m_layoutManager = nullptr;
    bool m_isPinned = false;
};

} // namespace QuarkMeta
