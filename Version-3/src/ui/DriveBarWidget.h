#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>

namespace QuarkMeta {

/**
 * @brief 盘符与标签管理栏独立组件
 */
class DriveBarWidget : public QWidget {
    Q_OBJECT

public:
    explicit DriveBarWidget(QWidget* parent = nullptr);
    ~DriveBarWidget() override = default;

    QPushButton* tagManagerButton() const { return m_btnTagManager; }
    QHBoxLayout* driveBarLayout() const { return m_driveBarLayout; }

private:
    void initUi();

    QHBoxLayout* m_driveBarLayout = nullptr;
    QPushButton* m_btnTagManager = nullptr;
};

} // namespace QuarkMeta
