#include "AddressBar.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "StyleLibrary.h"
#include "../core/NavigationHistoryService.h"
#include "../meta/FavoriteDao.h"
#include <QHBoxLayout>
#include <QDir>
#include <QPushButton>
#include <QTimer>
#include <QStyle>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QClipboard>

namespace QuarkMeta {

AddressBar::AddressBar(QWidget* parent) : QWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_addressContainer = new QWidget(this);
    m_addressContainer->setObjectName("AddressContainer");
    m_addressContainer->setFixedHeight(32);

    QHBoxLayout* containerLayout = new QHBoxLayout(m_addressContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    m_pathStack = new QStackedWidget(m_addressContainer);
    m_pathStack->setFixedHeight(30);
    m_pathStack->setObjectName("AddressPathStack");

    m_breadcrumbBar = new BreadcrumbBar(m_pathStack);
    m_pathStack->addWidget(m_breadcrumbBar);

    m_pathEdit = new QLineEdit(m_pathStack);
    m_pathEdit->setPlaceholderText("输入路径...");
    m_pathEdit->setFixedHeight(30); 
    m_pathEdit->setClearButtonEnabled(true);
    m_pathEdit->setObjectName("AddressPathEdit");
    m_pathStack->addWidget(m_pathEdit);

    m_btnRefresh = new QPushButton(m_addressContainer);
    m_btnRefresh->setFixedSize(30, 30);
    m_btnRefresh->setIcon(UiHelper::getIcon("sync", QColor("#CCCCCC"), 16));
    m_btnRefresh->setProperty("tooltipText", "刷新 (F5)");
    m_btnRefresh->setCursor(Qt::ArrowCursor);
    m_btnRefresh->setObjectName("BtnRefreshAddress");
    m_btnRefresh->setAttribute(Qt::WA_Hover);
    m_btnRefresh->installEventFilter(this);

    containerLayout->addWidget(m_pathStack, 1);
    containerLayout->addWidget(m_btnRefresh);

    layout->addWidget(m_addressContainer);

    connect(m_btnRefresh, &QPushButton::clicked, this, &AddressBar::refreshRequested);
    connect(m_breadcrumbBar, &BreadcrumbBar::blankAreaClicked, this, &AddressBar::onBreadcrumbBlankClicked);
    connect(m_pathEdit, &QLineEdit::editingFinished, this, &AddressBar::onPathEditFinished);
    connect(m_pathEdit, &QLineEdit::returnPressed, this, [this]() {
        QString input = m_pathEdit->text();
        m_pathEdit->deselect();
        m_pathEdit->clearFocus();

        if (QDir(input).exists() || input == "computer://" || input == tr("此电脑")) {
            emit pathChanged(input == tr("此电脑") ? "computer://" : input);
        } else {
            QString rollback = (m_currentPath == "computer://") ? tr("此电脑") : QDir::toNativeSeparators(m_currentPath);
            m_pathEdit->setText(rollback);
            m_pathStack->setCurrentWidget(m_breadcrumbBar);
        }
    });
    connect(m_breadcrumbBar, &BreadcrumbBar::pathClicked, this, &AddressBar::onBreadcrumbClicked);

    connect(m_breadcrumbBar, &BreadcrumbBar::favoriteToggleRequested, this, [this](const QString& fullPath, const QPoint& globalPos) {
        if (fullPath.isEmpty() || fullPath == "computer://") return;

        QString nativePath = QDir::toNativeSeparators(QDir::cleanPath(fullPath));

        QMenu menu(this);
        UiHelper::applyMenuStyle(&menu);

        QAction* actFavToggle = menu.addAction(UiHelper::getIcon("star_filled", QColor("#FDB70A")), "收藏 / 取消收藏");
        QAction* actCopyPath = menu.addAction(UiHelper::getIcon("copy", QColor("#EEEEEE")), "复制完整路径");

        QAction* selected = menu.exec(globalPos);
        if (selected == actFavToggle) {
            emit requestAddFavorite(nativePath);
        } else if (selected == actCopyPath) {
            QApplication::clipboard()->setText(nativePath);
            ToolTipOverlay::instance()->showText(QCursor::pos(), "已复制路径至剪贴板", 1500, Style::SuccessGreen);
        }
    });

    m_breadcrumbBar->setAttribute(Qt::WA_Hover);
    m_breadcrumbBar->installEventFilter(this);
    m_pathStack->installEventFilter(this);
    m_pathEdit->installEventFilter(this);

    m_historyPanel = new AddressHistoryPanel(this);
    connect(m_historyPanel, &AddressHistoryPanel::historyItemClicked, this, [this](const QString& path) {
        emit pathChanged(path);
        m_historyPanel->hide();
    });
}

void AddressBar::setPath(const QString& path) {
    m_currentPath = path;
    QString displayPath = (path == "computer://") ? tr("此电脑") : QDir::toNativeSeparators(path);
    m_pathEdit->setText(displayPath);
    m_breadcrumbBar->setPath(path);
    m_pathStack->setCurrentWidget(m_breadcrumbBar);
    NavigationHistoryService::instance().appendPath(path);
}

void AddressBar::onBreadcrumbBlankClicked() {
    QString displayPath = (m_currentPath == "computer://") ? tr("此电脑") : QDir::toNativeSeparators(m_currentPath);
    m_pathEdit->setText(displayPath);
    m_pathStack->setCurrentWidget(m_pathEdit);
    m_pathEdit->setFocus();
    QTimer::singleShot(50, m_pathEdit, &QLineEdit::selectAll);
}

void AddressBar::onPathEditFinished() {
    if (m_pathStack->currentWidget() == m_pathEdit) {
        m_pathStack->setCurrentWidget(m_breadcrumbBar);
    }
}

void AddressBar::onBreadcrumbClicked(const QString& path) {
    emit pathChanged(path);
}

bool AddressBar::eventFilter(QObject* obj, QEvent* event) {
    // 规则三：悬停气泡提示（仅在路径超长被截断/省略时，才使用 ToolTipOverlay 显示完整物理路径）
    if (obj == m_breadcrumbBar) {
        if (event->type() == QEvent::HoverEnter || event->type() == QEvent::Enter) {
            // 核心把关门禁：路径完整可见时绝对不弹，彻底杜绝无谓弹窗与 DWM 频繁合成假死
            if (m_breadcrumbBar && m_breadcrumbBar->isPathElided() && !m_currentPath.isEmpty()) {
                QString fullPath = (m_currentPath == "computer://") ? tr("此电脑") : QDir::toNativeSeparators(m_currentPath);
                ToolTipOverlay::instance()->showText(QCursor::pos(), fullPath, 0);
            }
        } else if (event->type() == QEvent::HoverLeave || event->type() == QEvent::Leave) {
            if (m_breadcrumbBar && m_breadcrumbBar->isPathElided()) {
                ToolTipOverlay::hideTip();
            }
        }
    }

    if (obj == m_btnRefresh) {
        if (event->type() == QEvent::HoverEnter || event->type() == QEvent::Enter) {
            m_btnRefresh->setIcon(UiHelper::getIcon("sync", Qt::white, 16));
            QString text = m_btnRefresh->property("tooltipText").toString();
            if (!text.isEmpty()) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), text, 0);
            }
        } else if (event->type() == QEvent::HoverLeave || event->type() == QEvent::Leave) {
            m_btnRefresh->setIcon(UiHelper::getIcon("sync", QColor("#CCCCCC"), 16));
            ToolTipOverlay::hideTip();
        }
    }

    if (obj == m_pathEdit) {
        if (event->type() == QEvent::FocusIn) {
            m_addressContainer->setProperty("focused", true);
            m_addressContainer->style()->unpolish(m_addressContainer);
            m_addressContainer->style()->polish(m_addressContainer);
        } else if (event->type() == QEvent::FocusOut) {
            m_addressContainer->setProperty("focused", false);
            m_addressContainer->style()->unpolish(m_addressContainer);
            m_addressContainer->style()->polish(m_addressContainer);
        }
    }

    if ((obj == m_pathStack || obj == m_breadcrumbBar || obj == m_pathEdit) && 
        event->type() == QEvent::MouseButtonDblClick) {
        
        QStringList history = NavigationHistoryService::instance().getHistory();
        if (!history.isEmpty()) {
            m_historyPanel->setHistory(history);
            m_historyPanel->showBelow(m_pathStack);
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

} // namespace QuarkMeta