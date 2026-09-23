#pragma once

#include <QObject>
#include <QFrame>
#include <QList>
#include <QPoint>
#include <QSplitter>

namespace QuarkMeta {

class ContentPanel;

class ContentPaneSplitManager : public QObject {
    Q_OBJECT
public:
    explicit ContentPaneSplitManager(ContentPanel* panel);
    ~ContentPaneSplitManager() override = default;

    bool isSplitMode() const;
    bool isSecondaryPane() const { return m_isSecondaryPane; }
    void setIsSecondaryPane(bool secondary) { m_isSecondaryPane = secondary; }

    ContentPanel* secondaryContentPanel() const;
    QList<ContentPanel*> panes() const { return m_panes; }
    int paneCount() const { return 1 + m_panes.size(); }
    ContentPanel* rootPane() const;

    void splitPane(Qt::Orientation orientation, const QString& secondaryPath = QString());
    void closePane(ContentPanel* pane);
    void closeSecondaryPane();
    void redistributePaneSizes();
    void setActivePane(bool active);
    void updateDragOverlay(const QPoint& pos);
    void hideDragOverlay();

private:
    ContentPanel* m_panel = nullptr;
    QSplitter* m_paneSplitter = nullptr;
    QFrame* m_primaryPaneContainer = nullptr;
    QList<QWidget*> m_paneContainers;
    QList<ContentPanel*> m_panes;
    ContentPanel* m_rootPane = nullptr;
    ContentPanel* m_activePaneForSplit = nullptr;
    QWidget* m_dragOverlayWidget = nullptr;
    Qt::Orientation m_splitOrientation = Qt::Horizontal;
    bool m_isSplit = false;
    bool m_isSecondaryPane = false;
};

} // namespace QuarkMeta
