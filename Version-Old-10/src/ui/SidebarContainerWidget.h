#pragma once

#include <QFrame>
#include <QTabBar>
#include <QStackedWidget>
#include <QVBoxLayout>
#include "FavoritePanel.h"
#include "LibraryPanel.h"

namespace QuarkMeta {

class SidebarContainerWidget : public QFrame {
    Q_OBJECT

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
};

} // namespace QuarkMeta
