#pragma once

#include <QObject>
#include <QFrame>
#include <QList>
#include <QPoint>
#include <QSplitter>
#include <QTimer>

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
    /**
     * @brief 获取管理的全部副窗格列表（扁平一维列表）
     * @note 严禁调用方对列表中的各副窗格再次递归调用 panes()。
     */
    QList<ContentPanel*> panes() const { return m_panes; }
    int paneCount() const { return 1 + m_panes.size(); }
    ContentPanel* rootPane() const;

    struct SplitEvaluationResult {
        bool isValid = false;
        Qt::Orientation orientation = Qt::Horizontal;
        bool insertBefore = false;
        QRect highlightRect;
    };

    static SplitEvaluationResult evaluateSplitDrop(const QPoint& pos, const QSize& refSize);

    void splitPane(Qt::Orientation orientation, const QString& secondaryPath = QString(), bool insertBefore = false);
    void closePane(ContentPanel* pane);
    void closeSecondaryPane();
    void redistributePaneSizes();
    void setActivePane(bool active);
    void refreshActiveIndicators();

    struct TabSplitState exportSplitState() const;
    void restoreSplitState(const struct TabSplitState& state);
    void updateDragOverlay(const QPoint& pos);
    void hideDragOverlay();
    void updateContainerMinimumWidth();
    void notifyLayoutChanged();

signals:
    void activePaneChanged(ContentPanel* panel);
    void layoutChanged();

private:
    QTimer* m_layoutDebounceTimer = nullptr;
    bool m_blockLayoutSignals = false;
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
