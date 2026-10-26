#pragma once

#include <QFrame>
#include <QTabBar>
#include <QStackedWidget>
#include <QVBoxLayout>
#include "FavoritePanel.h"
#include "LibraryPanel.h"
#include "SearchHistoryPanel.h"
#include <QLineEdit>

namespace QuarkMeta {

class SidebarContainerWidget : public QFrame {
    Q_OBJECT
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void sidebarTabChanged(int index);

public:
    explicit SidebarContainerWidget(QWidget* parent = nullptr);
    ~SidebarContainerWidget() override = default;

    FavoritePanel* favoritePanel() const { return m_favoritePanel; }
    LibraryPanel* libraryPanel() const { return m_libraryPanel; }

    QSize minimumSizeHint() const override { return QSize(230, 100); }

private:
    void initUi();

    QVBoxLayout* m_mainLayout = nullptr;
    QTabBar* m_tabBar = nullptr;
    QStackedWidget* m_stackedWidget = nullptr;
    FavoritePanel* m_favoritePanel = nullptr;
    LibraryPanel* m_libraryPanel = nullptr;
    QLineEdit* m_filterEdit = nullptr;
    SearchHistoryPanel* m_historyPanel = nullptr;
};

} // namespace QuarkMeta
