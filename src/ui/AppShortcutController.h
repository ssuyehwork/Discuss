#pragma once

#include <QObject>
#include <QPointer>
#include <QWidget>
#include <QShortcut>

namespace QuarkMeta {

class SearchController;

/**
 * @brief 局内（应用内）快捷键控制器
 * 基于 Qt 标准 QShortcut 实现，严格限定为 Qt::WindowShortcut 作用域，绝不侵入操作系统全局
 */
class AppShortcutController : public QObject {
    Q_OBJECT

public:
    explicit AppShortcutController(QWidget* targetWindow,
                                  SearchController* searchController,
                                  QObject* parent = nullptr);
    ~AppShortcutController() override;

    static bool isEditingFocus();

signals:
    /**
     * @brief Alt+Q 局内快捷键触发置顶状态翻转
     */
    void togglePinRequested();

    /**
     * @brief Tab 局内快捷键（非编辑状态）触发沉浸单栏模式切换
     */
    void toggleImmersiveRequested();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void initShortcuts();

    QPointer<QWidget> m_window;
    QPointer<SearchController> m_searchController;
};

} // namespace QuarkMeta
