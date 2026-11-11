#include "NavPanel.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include <QStyleOptionTab>
#include <QPainter>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace RapidNotes {

/**
 * @brief 构造函数，设置面板属性与布局
 */
NavPanel::NavPanel(QWidget* parent)
    : QFrame(parent) {
    setObjectName("SidebarContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    setContextMenuPolicy(Qt::CustomContextMenu);
    initUi();
}

void NavPanel::deferredInit() {
    // 纯文本笔记架构：NavPanel 作为空壳运行，无需进行物理磁盘与目录树扫描
}

void NavPanel::setFocusHighlight(bool visible) {
    Q_UNUSED(visible);
}

void NavPanel::setRootPath(const QString& path) {
    Q_UNUSED(path);
}

void NavPanel::selectPath(const QString& path) {
    Q_UNUSED(path);
}

QWidget* NavPanel::createPlaceholderPage(const QString& iconName, const QString& title, const QString& subtitle) {
    QWidget* page = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 40, 20, 20);
    layout->setSpacing(10);
    layout->setAlignment(Qt::AlignCenter);

    QLabel* iconLabel = new QLabel(page);
    iconLabel->setPixmap(UiHelper::getIcon(iconName, "#555555", 36).pixmap(36, 36));
    iconLabel->setAlignment(Qt::AlignCenter);

    QLabel* titleLabel = new QLabel(title, page);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #777777; font-size: 13px; font-weight: bold;");

    QLabel* descLabel = new QLabel(subtitle, page);
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setStyleSheet("color: #555555; font-size: 11px;");

    layout->addStretch();
    layout->addWidget(iconLabel);
    layout->addWidget(titleLabel);
    layout->addWidget(descLabel);
    layout->addStretch();

    return page;
}

void NavPanel::initUi() {
    QWidget* header = new QWidget(this);
    header->setObjectName("ContainerHeader");
    header->setFixedHeight(32);

    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(5, 0, 5, 0);
    headerLayout->setSpacing(0);

    // 侧边栏专用选项卡：高保真绘制，硬锁 7 像素间距
    class SidebarTabBar : public QTabBar {
    public:
        using QTabBar::QTabBar;
    protected:
        QSize tabSizeHint(int index) const override {
            QSize s = QTabBar::tabSizeHint(index);
            if (!tabIcon(index).isNull() && !tabText(index).isEmpty()) {
                s.setWidth(s.width() - 5);
            }
            return s;
        }

        void paintEvent(QPaintEvent* event) override {
            Q_UNUSED(event);
            QPainter painter(this);
            QStyleOptionTab opt;
            for (int i = 0; i < count(); ++i) {
                initStyleOption(&opt, i);
                style()->drawControl(QStyle::CE_TabBarTabShape, &opt, &painter, this);

                QRect tr = opt.rect;
                QSize iconSize = opt.iconSize.isValid() ? opt.iconSize : QSize(16, 16);
                int iconWidth = opt.icon.isNull() ? 0 : iconSize.width();
                int textWidth = opt.fontMetrics.horizontalAdvance(opt.text);
                int spacing = (iconWidth > 0 && !opt.text.isEmpty()) ? 7 : 0;
                int totalContentWidth = iconWidth + spacing + textWidth;
                int startX = tr.left() + (tr.width() - totalContentWidth) / 2;

                if (iconWidth > 0) {
                    int iconY = tr.top() + (tr.height() - iconSize.height()) / 2;
                    QRect iconRect(startX, iconY, iconSize.width(), iconSize.height());
                    opt.icon.paint(&painter, iconRect, Qt::AlignCenter);
                }

                QRect textRect(startX + iconWidth + spacing, tr.top(), textWidth + 2, tr.height());
                painter.save();
                painter.setFont(font());
                QColor textColor = (opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : ((opt.state & QStyle::State_MouseOver) ? QColor("#CCCCCC") : QColor("#AAAAAA"));
                painter.setPen(textColor);
                painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, opt.text);
                painter.restore();
            }
        }
    };

    m_tabBar = new SidebarTabBar(header);
    m_tabBar->setObjectName("SidebarTabBar");
    m_tabBar->setDrawBase(false);
    m_tabBar->setIconSize(QSize(16, 16));
    m_tabBar->addTab(UiHelper::getIcon("list_ul", "#888888", 16), "目录导航");
    m_tabBar->addTab(UiHelper::getIcon("clock", "#888888", 16), "历史");
    m_tabBar->setCursor(Qt::PointingHandCursor);

    headerLayout->addWidget(m_tabBar);
    headerLayout->addStretch();
    m_mainLayout->addWidget(header);

    m_stackedWidget = new QStackedWidget(this);

    // Page 0: 目录导航空壳占位
    QWidget* navPage = createPlaceholderPage("list_ul", "目录导航 (TODO)", "纯文本笔记架构占位容器");

    // Page 1: 历史记录空壳占位
    QWidget* historyPage = createPlaceholderPage("clock", "历史记录 (TODO)", "纯文本笔记架构占位容器");

    m_stackedWidget->addWidget(navPage);
    m_stackedWidget->addWidget(historyPage);

    m_mainLayout->addWidget(m_stackedWidget, 1);

    connect(m_tabBar, &QTabBar::currentChanged, this, [this](int index) {
        m_stackedWidget->setCurrentIndex(index);
    });
}

bool NavPanel::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::ToolTip) {
        QString text = watched->property("tooltipText").toString();
        if (!text.isEmpty()) {
            return true;
        }
    } else if (event->type() == QEvent::Leave) {
        ToolTipOverlay::hideTip();
    }
    return QFrame::eventFilter(watched, event);
}

} // namespace RapidNotes
