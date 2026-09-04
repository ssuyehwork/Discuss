#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "FilterPanel.h"

namespace QuarkMeta {

/**
 * @brief 内容面板顶部辅助控制工具栏组件
 * 封装标题图标、状态指示、递归显示按钮与隐藏/文件/文件夹过滤器切换按钮
 */
class ContentHeaderWidget : public QWidget {
    Q_OBJECT

public:
    explicit ContentHeaderWidget(QWidget* parent = nullptr);
    ~ContentHeaderWidget() override = default;

    void setFilterState(const FilterState& state);
    void setRecursive(bool recursive);
    void setLayersEnabled(bool enabled, const QString& tooltip);

signals:
    void filterStateChanged(const FilterState& state);
    void recursiveToggled(bool recursive);

private:
    void initUi();

    QHBoxLayout* m_layout = nullptr;
    QLabel* m_iconLabel = nullptr;
    QLabel* m_titleLabel = nullptr;

    QPushButton* m_btnLayers = nullptr;
    QPushButton* m_btnToggleHidden = nullptr;
    QPushButton* m_btnToggleFolders = nullptr;
    QPushButton* m_btnToggleFiles = nullptr;

    FilterState m_filterState;
};

} // namespace QuarkMeta
