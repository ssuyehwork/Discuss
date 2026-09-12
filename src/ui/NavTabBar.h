#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QList>

namespace QuarkMeta {

class NavTabBar : public QWidget {
    Q_OBJECT

public:
    explicit NavTabBar(QWidget* parent = nullptr);
    ~NavTabBar() override = default;

    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void rebuildTabs();

private:
    QWidget* createTabWidget(int index, const QString& title, const QString& url, bool isActive);

    QHBoxLayout* m_mainLayout = nullptr;
    QHBoxLayout* m_tabsLayout = nullptr;
    QPushButton* m_btnAddTab = nullptr;
};

} // namespace QuarkMeta
