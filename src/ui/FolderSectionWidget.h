#pragma once

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>

namespace QuarkMeta {

/**
 * @brief 文件夹折叠/伸展标题栏 (子文件夹 (N) ▾)
 */
class FolderSectionHeaderBar : public QFrame {
    Q_OBJECT
public:
    explicit FolderSectionHeaderBar(QWidget* parent = nullptr);
    ~FolderSectionHeaderBar() override = default;

    void setCount(int count);
    int count() const { return m_count; }

    bool isCollapsed() const { return m_collapsed; }
    void setCollapsed(bool collapsed);

signals:
    void collapseToggled(bool collapsed);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    void updateUi();

    int m_count = 0;
    bool m_collapsed = false;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_arrowLabel = nullptr;
};

/**
 * @brief 内容文件区分界标题栏 (内容 (M))
 */
class FileSectionHeaderBar : public QFrame {
    Q_OBJECT
public:
    explicit FileSectionHeaderBar(QWidget* parent = nullptr);
    ~FileSectionHeaderBar() override = default;

    void setCount(int count);
    int count() const { return m_count; }

private:
    int m_count = 0;
    QLabel* m_titleLabel = nullptr;
};

} // namespace QuarkMeta
