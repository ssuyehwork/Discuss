#pragma once

#include <QFrame>
#include <QLineEdit>
#include <QListWidget>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QMap>
#include <QStringList>
#include "components/FlowLayout.h"
#include "../meta/TagRepository.h"

namespace QuarkMeta {

class TagSelectorOverlay : public QFrame {
    Q_OBJECT
public:
    TagSelectorOverlay(const QStringList& initialSelected, QWidget* parent = nullptr);
    ~TagSelectorOverlay() override;

signals:
    void selectionChanged(const QStringList& selectedTags);
    void overlayClosed();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void changeEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void initUi();
    void loadTagsAndGroups();
    void filterTags();
    void populateGrid();
    void toggleTagSelection(const QString& tag);
    void updateSelectionHighlight();
    void handleGridNavigation(int key);
    void handleGroupNavigation(int key);

    void updateCursorShape(const QPoint& pos);
    int getResizeDirection(const QPoint& pos);
    bool isInteractiveChild(QWidget* child) const;

    QStringList m_selectedTags;
    QStringList m_displayedTags;
    QMap<QString, int> m_allTagCounts;
    QList<TagRepository::TagGroup> m_allGroups;

    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_btnToggleSidebar = nullptr;
    QListWidget* m_groupList = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_tagGridWidget = nullptr;
    FlowLayout* m_gridFlowLayout = nullptr;
    QList<QPushButton*> m_tagButtons;
    int m_currentTagIndex = -1;

    QPoint m_dragStartPos;
    QRect m_dragStartGeometry;
    bool m_isDragging = false;
    int m_resizeDir = 0;
    const int m_margin = 6;
    bool m_wasActivated = false;
};

} // namespace QuarkMeta