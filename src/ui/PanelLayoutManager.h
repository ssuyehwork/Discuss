#pragma once

#include <QObject>
#include <QSplitter>
#include <QMainWindow>
#include <QMenu>
#include <QPoint>
#include <QPointer>

namespace QuarkMeta {

class NavPanel;
class FavoritePanel;
class ContentPanel;
class MetaPanel;
class FilterPanel;

class PanelLayoutManager : public QObject {
    Q_OBJECT

public:
    explicit PanelLayoutManager(QMainWindow* mainWindow,
                                QSplitter* mainSplitter,
                                NavPanel* navPanel,
                                FavoritePanel* favoritePanel,
                                ContentPanel* contentPanel,
                                MetaPanel* metaPanel,
                                FilterPanel* filterPanel,
                                QObject* parent = nullptr);
    ~PanelLayoutManager() override = default;

    void initLayout();
    void resetSplitterLayout();
    void setPanelVisible(const QString& panelId, bool visible);
    bool isPanelVisible(const QString& panelId) const;
    void populatePanelMenu(QMenu* menu);
    void showPanelContextMenu(const QPoint& globalPos);
    void updateDynamicMinimumSize();
    void saveLayoutState();
    void setTagManagerMode(bool isMode) { m_isTagManagerMode = isMode; }

signals:
    void layoutResetCompleted();
    void panelVisibilityChanged(const QString& panelId, bool visible);

private:
    void loadPanelVisibility();

    QPointer<QMainWindow> m_mainWindow;
    QPointer<QSplitter> m_mainSplitter;

    QPointer<NavPanel> m_navPanel;
    QPointer<FavoritePanel> m_favoritePanel;
    QPointer<ContentPanel> m_contentPanel;
    QPointer<MetaPanel> m_metaPanel;
    QPointer<FilterPanel> m_filterPanel;

    bool m_isTagManagerMode = false;
    static constexpr int kBasePanelWidth = 230;
    static constexpr int kContentBaseWidth = 550;
    static constexpr int kSplitterHandleWidth = 5;
};

} // namespace QuarkMeta
