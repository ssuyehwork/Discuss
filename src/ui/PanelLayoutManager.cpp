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

    bool isImmersive = AppConfig::instance().getValue("MainWindow/IsImmersiveMode", false).toBool();
    if (isImmersive) {
        if (m_navPanel) m_navPanel->setVisible(false);
        if (m_favoritePanel) m_favoritePanel->setVisible(false);
        if (m_metaPanel) m_metaPanel->setVisible(false);
        if (m_filterPanel) m_filterPanel->setVisible(false);
        emit panelVisibilityChanged("nav", false);
        emit panelVisibilityChanged("favorite", false);
        emit panelVisibilityChanged("meta", false);
        emit panelVisibilityChanged("filter", false);
    }

    // 【归一化修复】splitter状态恢复必须延迟到下一轮事件循环，确保此时子控件已完成首次布局、窗口geometry已经是最终值
    QByteArray state = AppConfig::instance().getValue("MainWindow/SplitterState").toByteArray();
    QPointer<QSplitter> splitterPtr = m_mainSplitter;
    QPointer<PanelLayoutManager> selfPtr(this);
    QTimer::singleShot(0, [splitterPtr, state, selfPtr, isImmersive]() {
        if (!splitterPtr) return;
        if (!state.isEmpty() && !isImmersive) {
            splitterPtr->restoreState(state);
        } else if (!isImmersive) {
            QList<int> sizes;
            sizes << kBasePanelWidth << kBasePanelWidth << kContentBaseWidth << kBasePanelWidth << kBasePanelWidth;
            splitterPtr->setSizes(sizes);
        }
        // 【归一化修复】restoreState() 会把配置文件里存档的旧 handle 宽度一并带回来，
        // 覆盖掉刚才在 setupSplitters() 里设置好的 5px，此处强制重新纠正
        splitterPtr->setHandleWidth(kSplitterHandleWidth);
        if (selfPtr) {
            selfPtr->updateDynamicMinimumSize();
        }
    });
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
    saveLayoutState();
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

bool PanelLayoutManager::isImmersiveMode() const {
    return !isPanelVisible("nav") &&
           !isPanelVisible("favorite") &&
           !isPanelVisible("meta") &&
           !isPanelVisible("filter");
}

void PanelLayoutManager::savePreImmersiveState() {
    AppConfig::instance().setValue("MainWindow/PreImmersiveNavVisible", isPanelVisible("nav"));
    AppConfig::instance().setValue("MainWindow/PreImmersiveFavoriteVisible", isPanelVisible("favorite"));
    AppConfig::instance().setValue("MainWindow/PreImmersiveMetaVisible", isPanelVisible("meta"));
    AppConfig::instance().setValue("MainWindow/PreImmersiveFilterVisible", isPanelVisible("filter"));
    if (m_mainSplitter) {
        AppConfig::instance().setValue("MainWindow/PreImmersiveSplitterState", m_mainSplitter->saveState());
    }
    AppConfig::instance().sync();
}

void PanelLayoutManager::restorePreImmersiveState() {
    bool navVis = AppConfig::instance().getValue("MainWindow/PreImmersiveNavVisible", true).toBool();
    bool favVis = AppConfig::instance().getValue("MainWindow/PreImmersiveFavoriteVisible", true).toBool();
    bool metaVis = AppConfig::instance().getValue("MainWindow/PreImmersiveMetaVisible", true).toBool();
    bool filterVis = AppConfig::instance().getValue("MainWindow/PreImmersiveFilterVisible", true).toBool();
    QByteArray preSplitterState = AppConfig::instance().getValue("MainWindow/PreImmersiveSplitterState").toByteArray();

    if (!navVis && !favVis && !metaVis && !filterVis) {
        navVis = favVis = metaVis = filterVis = true;
    }

    if (m_navPanel) m_navPanel->setVisible(navVis);
    if (m_favoritePanel) m_favoritePanel->setVisible(favVis);
    if (m_metaPanel) m_metaPanel->setVisible(metaVis);
    if (m_filterPanel) m_filterPanel->setVisible(filterVis);

    if (m_mainSplitter && !preSplitterState.isEmpty()) {
        m_mainSplitter->restoreState(preSplitterState);
        m_mainSplitter->setHandleWidth(kSplitterHandleWidth);
    }

    updateDynamicMinimumSize();
    emit panelVisibilityChanged("nav", navVis);
    emit panelVisibilityChanged("favorite", favVis);
    emit panelVisibilityChanged("meta", metaVis);
    emit panelVisibilityChanged("filter", filterVis);
}

void PanelLayoutManager::toggleImmersiveMode() {
    if (isImmersiveMode()) {
        restorePreImmersiveState();
    } else {
        savePreImmersiveState();
        if (m_navPanel) m_navPanel->setVisible(false);
        if (m_favoritePanel) m_favoritePanel->setVisible(false);
        if (m_metaPanel) m_metaPanel->setVisible(false);
        if (m_filterPanel) m_filterPanel->setVisible(false);

        updateDynamicMinimumSize();
        emit panelVisibilityChanged("nav", false);
        emit panelVisibilityChanged("favorite", false);
        emit panelVisibilityChanged("meta", false);
        emit panelVisibilityChanged("filter", false);
    }

    saveLayoutState();

    ToolTipOverlay::instance()->showText(
        QCursor::pos(),
        isImmersiveMode() ? "已进入沉浸全屏模式" : "已恢复分栏布局",
        1200,
        QColor("#378ADD")
    );
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

    // 🚀【顶栏物理安全锁】：动态计算值与 475px 绝对下限取最大值
    int calculatedMinW = (visibleCount * kBasePanelWidth) + ((visibleCount - 1) * kSplitterHandleWidth) + 10;
    int finalMinW = std::max(kWindowAbsoluteMinWidth, calculatedMinW);

    m_mainWindow->setMinimumWidth(finalMinW);
}

void PanelLayoutManager::saveLayoutState() {
    AppConfig::instance().setValue("MainWindow/IsImmersiveMode", isImmersiveMode());
    AppConfig::instance().setValue("MainWindow/NavVisible", isPanelVisible("nav"));
    AppConfig::instance().setValue("MainWindow/FavoriteVisible", isPanelVisible("favorite"));
    AppConfig::instance().setValue("MainWindow/MetaVisible", isPanelVisible("meta"));
    AppConfig::instance().setValue("MainWindow/FilterVisible", isPanelVisible("filter"));
    if (m_mainSplitter && !isImmersiveMode()) {
        AppConfig::instance().setValue("MainWindow/SplitterState", m_mainSplitter->saveState());
    }
    AppConfig::instance().sync();
}

} // namespace QuarkMeta