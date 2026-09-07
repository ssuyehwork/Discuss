#pragma once

#include <QObject>
#include <QEvent>

namespace QuarkMeta {

class ContentPanel;

/**
 * @brief 内容面板事件与热键独立拦截器
 * 负责缩放滚轮、星级/颜色快捷键、空格 QuickLook 准入过滤、委托 Hitbox 命中检测
 */
class ContentKeyHandler : public QObject {
    Q_OBJECT
public:
    explicit ContentKeyHandler(ContentPanel* panel, QObject* parent = nullptr);
    ~ContentKeyHandler() override = default;

    /**
     * @brief 核心事件路由总入口
     */
    bool handleEvent(QObject* obj, QEvent* event);

private:
    bool handleWheel(QObject* obj, QEvent* event);
    bool handleMousePress(QObject* obj, QEvent* event);
    bool handleKeyPress(QObject* obj, QEvent* event);
    bool handleTooltipHover(QObject* obj, QEvent* event);

    ContentPanel* m_panel = nullptr;
};

} // namespace QuarkMeta
