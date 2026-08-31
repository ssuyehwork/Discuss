#include "PanelLayoutManager.h"
#include "NavPanel.h"
#include "FavoritePanel.h"
#include "ContentPanel.h"
#include "MetaPanel.h"
#include "FilterPanel.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "../core/AppConfig.h"
#include <QAction>
#include <QCursor>
#include <QTimer>
#include <QList>
#include <QStringList>

namespace QuarkMeta {

PanelLayoutManager::PanelLayoutManager(QMainWindow* mainWindow,
                                       QSplitter* mainSplitter,
                                       NavPanel* navPanel,
                                       FavoritePanel* favoritePanel,
                                       ContentPanel* contentPanel,
                                       MetaPanel* metaPanel,
                                       FilterPanel* filterPanel,
                                       QObject* parent)
    : QObject(parent),
      m_mainWindow(mainWindow),
      m_mainSplitter(mainSplitter),
      m_navPanel(navPanel),
      m_favoritePanel(favoritePanel),
      m_contentPanel(contentPanel),
      m_metaPanel(metaPanel),
      m_filterPanel(filterPanel) {
}

void PanelLayoutManager::initLayout() {
    if (!m_mainSplitter) return;

    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 0);
    m_mainSplitter->setStretchFactor(2, 1);
    m_mainSplitter->setStretchFactor(3, 0);
    m_mainSplitter->setStretchFactor(4, 0);

    QByteArray state = AppConfig::instance().getValue("MainWindow/SplitterState").toByteArray();
    if (!state.isEmpty()) {
        m_mainSplitter->restoreState(state);
    } else {
        QList<int> sizes;
        sizes << kBasePanelWidth << kBasePanelWidth << kContentBaseWidth << kBasePanelWidth << kBasePanelWidth;
        m_mainSplitter->setSizes(sizes);
    }

    updateDynamicMinimumSize();
}

void PanelLayoutManager::resetSplitterLayout() {
    if (!m_mainSplitter) return;

    if (m_navPanel) m_navPanel->show();
    if (m_favoritePanel) m_favoritePanel->show();
    if (m_contentPanel) m_contentPanel->show();
    if (m_metaPanel) m_metaPanel->show();
    if (m_filterPanel) m_filterPanel->show();

    QList<int> sizes;
    sizes << kBasePanelWidth << kBasePanelWidth << kContentBaseWidth << kBasePanelWidth << kBasePanelWidth;
    m_mainSplitter->setSizes(sizes);

    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 0);
    m_mainSplitter->setStretchFactor(2, 1);
    m_mainSplitter->setStretchFactor(3, 0);
    m_mainSplitter->setStretchFactor(4, 0);

    AppConfig::instance().remove("MainWindow/SplitterState");
    AppConfig::instance().remove("MainWindow/PanelVisibility");
    AppConfig::instance().sync();

    ToolTipOverlay::instance()->showText(QCursor::pos(), "分栏布局已重置为默认值", 1500, QColor("#2ecc71"));
    updateDynamicMinimumSize();
    emit layoutResetCompleted();
}

void PanelLayoutManager::setPanelVisible(const QString& panelId, bool visible) {
    if (panelId == "nav" && m_navPanel) m_navPanel->setVisible(visible);
    else if (panelId == "favorite" && m_favoritePanel) m_favoritePanel->setVisible(visible);
    else if (panelId == "content" && m_contentPanel) m_contentPanel->setVisible(true);
    else if (panelId == "meta" && m_metaPanel) m_metaPanel->setVisible(visible);
    else if (panelId == "filter" && m_filterPanel) m_filterPanel->setVisible(visible);

    updateDynamicMinimumSize();
    emit panelVisibilityChanged(panelId, visible);
}

bool PanelLayoutManager::isPanelVisible(const QString& panelId) const {
    if (panelId == "nav" && m_navPanel) return !m_navPanel->isHidden();
    if (panelId == "favorite" && m_favoritePanel) return !m_favoritePanel->isHidden();
    if (panelId == "content" && m_contentPanel) return !m_contentPanel->isHidden();
    if (panelId == "meta" && m_metaPanel) return !m_metaPanel->isHidden();
    if (panelId == "filter" && m_filterPanel) return !m_filterPanel->isHidden();
    return false;
}

void PanelLayoutManager::populatePanelMenu(QMenu* menu) {
    if (!menu) return;

    auto addToggleAction = [this, menu](const QString& text, const QString& panelId, QWidget* panel, bool canHide = true) {
        if (!panel) return;
        QAction* action = menu->addAction(text);
        action->setCheckable(true);
        action->setChecked(!panel->isHidden());
        action->setEnabled(canHide);

        connect(action, &QAction::toggled, this, [this, panelId](bool visible) {
            setPanelVisible(panelId, visible);
        });
    };

    addToggleAction("显示目录导航", "nav", m_navPanel);
    addToggleAction("显示收藏夹", "favorite", m_favoritePanel);
    addToggleAction("显示内容区", "content", m_contentPanel, false);
    addToggleAction("显示元数据栏", "meta", m_metaPanel);
    addToggleAction("显示筛选栏", "filter", m_filterPanel);

    menu->addSeparator();
    QAction* resetAct = menu->addAction("重置分栏");
    connect(resetAct, &QAction::triggered, this, &PanelLayoutManager::resetSplitterLayout);
}

void PanelLayoutManager::showPanelContextMenu(const QPoint& globalPos) {
    QMenu menu;
    UiHelper::applyMenuStyle(&menu);
    populatePanelMenu(&menu);
    menu.exec(globalPos);
}

void PanelLayoutManager::updateDynamicMinimumSize() {
    if (!m_mainWindow) return;

    int visibleCount = 0;
    if (m_navPanel && !m_navPanel->isHidden()) visibleCount++;
    if (m_favoritePanel && !m_favoritePanel->isHidden()) visibleCount++;
    if (m_contentPanel && !m_contentPanel->isHidden()) visibleCount++;
    if (m_metaPanel && !m_metaPanel->isHidden()) visibleCount++;
    if (m_filterPanel && !m_filterPanel->isHidden()) visibleCount++;

    if (visibleCount <= 0) visibleCount = 1;

    // 🚀【彻底干掉 465 像素】：按可见栏区数量精准计算（单面板时为 240px，五面板时为 1180px）
    int calculatedMinW = (visibleCount * kBasePanelWidth) + ((visibleCount - 1) * kSplitterHandleWidth) + 10;
    int finalMinW = calculatedMinW;

    m_mainWindow->setMinimumWidth(finalMinW);
}

void PanelLayoutManager::saveLayoutState() {
    if (m_mainSplitter) {
        AppConfig::instance().setValue("MainWindow/SplitterState", m_mainSplitter->saveState());
    }
    AppConfig::instance().sync();
}

} // namespace QuarkMeta