#pragma once

#include <QFrame>
#include <QLineEdit>
#include <QShowEvent>
#include <QListWidget>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QMap>
#include <QStringList>
#include "components/FlowLayout.h"
#include "../core/TagLexiconService.h"
#include "FramelessWindowHelper.h"

namespace QuarkMeta {

class TagSelectorOverlay : public QFrame {
    Q_OBJECT
public:
    TagSelectorOverlay(const QStringList& initialSelected, QWidget* parent = nullptr);
    TagSelectorOverlay(const QStringList& initialSelected, const QStringList& candidatePaths, QWidget* parent = nullptr);
    ~TagSelectorOverlay() override;

signals:
    void selectionChanged(const QStringList& selectedTags);
    void tagsCommitted(const QStringList& finalTags);
    void overlayClosed();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void changeEvent(QEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;

private:
    void initUi();
    void loadRecentAndRecommendedTags(const QStringList& candidatePaths);
    void filterAndPopulate();
    void toggleTagSelection(const QString& tag);
    void recordRecentTag(const QString& tag);
    void closeOverlay();

    QStringList m_selectedTags;
    QStringList m_initialTags;
    QStringList m_recentTags;
    QStringList m_recommendedTags;

    QLineEdit* m_searchEdit = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_bodyContainer = nullptr;
    QVBoxLayout* m_bodyLayout = nullptr;

    QWidget* m_recSectionWidget = nullptr;
    FlowLayout* m_recFlowLayout = nullptr;
    QPushButton* m_recHeaderBtn = nullptr;
    bool m_recExpanded = true;

    QWidget* m_recentSectionWidget = nullptr;
    FlowLayout* m_recentFlowLayout = nullptr;
    QPushButton* m_recentHeaderBtn = nullptr;
    bool m_recentExpanded = true;

    bool m_isClosing = false;
    bool m_hasCommitted = false;
    bool m_isDragging = false;
    QPoint m_dragPos;

    FramelessWindowHelper* m_framelessHelper = nullptr;
};

} // namespace QuarkMeta