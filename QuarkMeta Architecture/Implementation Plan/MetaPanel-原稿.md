# QuarkMeta 五大子面板与导航视图体系纯化实施方案

## 1. 目标与范围
- 彻底消灭 `MetaPanel` 双重写盘：从 `setRating`、`setColor` 与 `onTagDeleted` 中彻底移除私自直调 `MetadataManager` 的违规代码，纯化为受控 View，所有持久化由领域服务单次原子执行。
- 彻底消灭 `AddressBar` 顶层窗口搜刮反模式：移除在 `QApplication::topLevelWidgets()` 中递归查找 `FavoritePanel` 的代码，改为标准信号路由。
- 纠偏 `NavPanel` 命名与直连导航：统一 `setObjectName("SidebarContainer")`，树形节点点击直连 `NavigationService::instance().navigateTo`。
- 强化 `FavoritePanel` 路径归一化：所有收藏路径严格经过 `QDir::cleanPath` 强归一化比对，彻底杜绝重复项。

---

## 2. 核心模块独立实现与改造

### 2.1 `src/ui/MetaPanel.cpp` 消除双重写盘
```cpp
#include "MetaPanel.h"
#include "UiHelper.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QCursor>
#include <QRegularExpression>

namespace QuarkMeta {

// ... [构造函数、createCollapsibleSection、initUi 等保持不变] ...

void MetaPanel::setRating(int rating, bool fromUser) {
    m_currentRating = rating;
    for (int i = 0; i < m_starBtns.size(); ++i) {
        bool active = (i < rating);
        m_starBtns[i]->setIcon(UiHelper::getIcon(
            active ? "star_filled" : "star",
            active ? QColor("#FF551C") : QColor("#555555"),
            18
        ));
        m_starBtns[i]->setIconSize(QSize(18, 18));
    }

    // 🚀【彻底消灭双重写盘】：只发射事件让领域服务单次执行，严禁 View 内部私自写盘！
    if (fromUser && !m_selectedPaths.isEmpty()) {
        emit metadataChanged(rating, m_currentColor);
    }
}

void MetaPanel::setColor(const std::wstring& color, bool fromUser) {
    m_currentColor = color;
    QString colorStr = QString::fromStdWString(color);

    for (QPushButton* btn : m_colorBtns) {
        QString hex = btn->property("hexColor").toString();
        QString tip = btn->property("tooltipText").toString();
        bool active = (!colorStr.isEmpty() && (colorStr == hex || colorStr == tip));

        btn->setStyleSheet(QString(
            "QPushButton { background-color: %1; border: %2; border-radius: 8px; }"
            "QPushButton:hover { border-color: #FFFFFF; }"
        ).arg(hex).arg(active ? "2px solid #FFFFFF" : "1px solid transparent"));
    }

    // 🚀【彻底消灭双重写盘】：只发射事件让领域服务单次执行，严禁 View 内部私自写盘！
    if (fromUser && !m_selectedPaths.isEmpty()) {
        emit metadataChanged(m_currentRating, color);
    }
}

void MetaPanel::onTagDeleted(const QString& text) { 
    if (m_selectedPaths.isEmpty()) return; 
 
    for (int i = 0; i < m_tagFlowLayout->count(); ++i) { 
        QLayoutItem* item = m_tagFlowLayout->itemAt(i); 
        TagPill* pill = qobject_cast<TagPill*>(item->widget()); 
        if (pill && pill->property("tagText").toString() == text) { 
            m_tagFlowLayout->takeAt(i); 
            pill->deleteLater(); 
            delete item; 
            break; 
        } 
    } 

    QStringList remainingTags;
    for (int i = 0; i < m_tagFlowLayout->count(); ++i) {
        TagPill* pill = qobject_cast<TagPill*>(m_tagFlowLayout->itemAt(i)->widget());
        if (pill) {
            remainingTags.append(pill->property("tagText").toString());
        }
    }

    if (remainingTags.isEmpty()) {
        m_btnAddTagBig->show();
        m_btnAddTagSmall->hide();
    }

    // 🚀 仅通过信号通知领域中枢统一处理持久化
    emit tagRemoveRequested(m_selectedPaths, text); 
    emit tagsChanged(m_selectedPaths, remainingTags);

    adjustFlowHeights(); 
    if (m_container) m_container->adjustSize(); 
}

// ... [其余实现保持不变] ...

} // namespace QuarkMeta
```

---

### 2.2 `src/ui/AddressBar.h`
```cpp
#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QStackedWidget>
#include <QPushButton>
#include "BreadcrumbBar.h"
#include "AddressHistoryPanel.h"

namespace QuarkMeta {

class AddressBar : public QWidget {
    Q_OBJECT

public:
    explicit AddressBar(QWidget* parent = nullptr);
    ~AddressBar() override = default;

    void setPath(const QString& path);
    QString currentPath() const { return m_currentPath; }

signals:
    void pathChanged(const QString& path);
    void refreshRequested();
    void favoriteToggleRequested(const QString& fullPath, const QPoint& globalPos);

private slots:
    void onBreadcrumbBlankClicked();
    void onPathEditFinished();
    void onBreadcrumbClicked(const QString& path);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    QWidget*        m_addressContainer = nullptr;
    QStackedWidget* m_pathStack = nullptr;
    BreadcrumbBar*  m_breadcrumbBar = nullptr;
    QLineEdit*      m_pathEdit = nullptr;
    QPushButton*    m_btnRefresh = nullptr;
    AddressHistoryPanel* m_historyPanel = nullptr;
    QString         m_currentPath;
};

} // namespace QuarkMeta
```

### 2.3 `src/ui/AddressBar.cpp` 消除顶层窗口搜刮
```cpp
#include "AddressBar.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "StyleLibrary.h"
#include "../core/NavigationService.h"
#include "../core/NavigationHistoryService.h"
#include <QHBoxLayout>
#include <QDir>
#include <QPushButton>
#include <QTimer>
#include <QApplication>

namespace QuarkMeta {

AddressBar::AddressBar(QWidget* parent) : QWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_addressContainer = new QWidget(this);
    m_addressContainer->setObjectName("AddressContainer");
    m_addressContainer->setFixedHeight(32);
    m_addressContainer->setStyleSheet(
        "QWidget#AddressContainer { background: #1E1E1E; border: 1px solid #333333; border-radius: 6px; }"
        "QWidget#AddressContainer[focused='true'] { border: 1px solid #3498db; }"
    );
    QHBoxLayout* containerLayout = new QHBoxLayout(m_addressContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    m_pathStack = new QStackedWidget(m_addressContainer);
    m_pathStack->setFixedHeight(30);
    m_pathStack->setStyleSheet("QStackedWidget { background: transparent; border: none; }");

    m_breadcrumbBar = new BreadcrumbBar(m_pathStack);
    m_pathStack->addWidget(m_breadcrumbBar);

    m_pathEdit = new QLineEdit(m_pathStack);
    m_pathEdit->setPlaceholderText("输入路径...");
    m_pathEdit->setFixedHeight(30); 
    m_pathEdit->setClearButtonEnabled(true);
    m_pathEdit->setStyleSheet("QLineEdit { background: transparent; border: none; color: #EEEEEE; padding-left: 8px; }");
    m_pathStack->addWidget(m_pathEdit);

    m_btnRefresh = new QPushButton(m_addressContainer);
    m_btnRefresh->setFixedSize(30, 30);
    m_btnRefresh->setIcon(UiHelper::getIcon("sync", QColor("#CCCCCC"), 16));
    m_btnRefresh->setProperty("tooltipText", "刷新 (F5)");
    m_btnRefresh->setCursor(Qt::ArrowCursor);
    m_btnRefresh->setStyleSheet(
        "QPushButton { background: transparent; border: none; border-left: 1px solid #333333; border-top-right-radius: 6px; border-bottom-right-radius: 6px; }"
    );
    m_btnRefresh->setAttribute(Qt::WA_Hover);
    m_btnRefresh->installEventFilter(this);

    containerLayout->addWidget(m_pathStack, 1);
    containerLayout->addWidget(m_btnRefresh);

    layout->addWidget(m_addressContainer);

    connect(m_btnRefresh, &QPushButton::clicked, &NavigationService::instance(), &NavigationService::refresh);
    connect(m_breadcrumbBar, &BreadcrumbBar::blankAreaClicked, this, &AddressBar::onBreadcrumbBlankClicked);
    connect(m_pathEdit, &QLineEdit::editingFinished, this, &AddressBar::onPathEditFinished);
    
    // 🚀【直连 NavigationService】：输入路径回车直接派发给领域服务
    connect(m_pathEdit, &QLineEdit::returnPressed, this, [this]() {
        QString input = m_pathEdit->text().trimmed();
        m_pathEdit->deselect();
        m_pathEdit->clearFocus();

        if (!input.isEmpty()) {
            NavigationService::instance().navigateTo(input);
        } else {
            m_pathStack->setCurrentWidget(m_breadcrumbBar);
        }
    });

    connect(m_breadcrumbBar, &BreadcrumbBar::pathClicked, this, &AddressBar::onBreadcrumbClicked);

    // 🚀【彻底消灭顶层窗口搜刮】：右键收藏直接向外发射信号，交由 Mediator 统一处理
    connect(m_breadcrumbBar, &BreadcrumbBar::favoriteToggleRequested, this, &AddressBar::favoriteToggleRequested);

    m_pathStack->installEventFilter(this);
    m_breadcrumbBar->installEventFilter(this);
    m_pathEdit->installEventFilter(this);

    m_historyPanel = new AddressHistoryPanel(this);
    connect(m_historyPanel, &AddressHistoryPanel::historyItemClicked, this, [this](const QString& path) {
        NavigationService::instance().navigateTo(path);
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
    NavigationService::instance().navigateTo(path);
}

bool AddressBar::eventFilter(QObject* obj, QEvent* event) {
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
```

---

### 2.4 `src/ui/NavPanel.cpp` 纠偏命名与直连导航
```cpp
#include "NavPanel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "TreeItemDelegate.h"
#include "DropTreeView.h"
#include "../core/NavigationService.h"
#include <QHeaderView>
#include <QLabel>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTimer>
#include <QPushButton>
#include <QtConcurrent>
#include <QCoreApplication>

namespace QuarkMeta {

NavPanel::NavPanel(QWidget* parent)
    : QFrame(parent) {
    // 🚀【命名纠偏】：统一命名为 SidebarContainer，消除 QSS 样式打架
    setObjectName("SidebarContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);
    setStyleSheet("background-color: #1E1E1E; color: #EEEEEE;");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    setContextMenuPolicy(Qt::CustomContextMenu);
    initUi();
}

// ... [deferredInit 保持不变] ...

void NavPanel::onTreeClicked(const QModelIndex& index) {
    QString path = index.data(Qt::UserRole + 1).toString();
    if (!path.isEmpty()) {
        // 🚀【直连 NavigationService】
        NavigationService::instance().navigateTo(path);
    }
}

// ... [其余方法保持不变] ...

} // namespace QuarkMeta
```

---

### 2.5 `src/ui/FavoritePanel.cpp` 强归一化去重与直连导航
```cpp
#include "FavoritePanel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "../core/NavigationService.h"
#include "../core/AppConfig.h"
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QFileInfo>
#include <QDir>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

namespace QuarkMeta {

// ... [FavoriteItemDelegate 保持不变] ...

FavoritePanel::FavoritePanel(QWidget* parent)
    : QFrame(parent) {
    setObjectName("FavoriteContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);
    setStyleSheet("background-color: #1E1E1E; color: #EEEEEE;");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    initUi();
    loadFavorites();
}

void FavoritePanel::onFavoriteClicked(const QModelIndex& index) {
    QString path = index.data(Qt::UserRole + 1).toString();
    if (path.isEmpty()) return;

    QFileInfo fi(path);
    if (fi.isDir()) {
        NavigationService::instance().navigateTo(path);
    } else {
        emit requestLocateFile(path);
    }
}

bool FavoritePanel::containsPath(const QString& path) const {
    if (!m_favoriteModel || path.isEmpty()) return false;
    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));

    for (int i = 0; i < m_favoriteModel->rowCount(); ++i) {
        QString existingPath = QDir::toNativeSeparators(QDir::cleanPath(m_favoriteModel->item(i)->data(Qt::UserRole + 1).toString()));
        if (QString::compare(existingPath, cleanPath, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

void FavoritePanel::removeFavoriteItem(const QString& path) {
    if (!m_favoriteModel || path.isEmpty()) return;
    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));

    for (int i = 0; i < m_favoriteModel->rowCount(); ++i) {
        QString existingPath = QDir::toNativeSeparators(QDir::cleanPath(m_favoriteModel->item(i)->data(Qt::UserRole + 1).toString()));
        if (QString::compare(existingPath, cleanPath, Qt::CaseInsensitive) == 0) {
            m_favoriteModel->removeRow(i);
            saveFavorites();
            return;
        }
    }
}

void FavoritePanel::addFavoriteItem(const QString& path) {
    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
    if (cleanPath.isEmpty() || containsPath(cleanPath)) return; // 强防重校验

    QFileInfo fi(cleanPath);
    if (!fi.exists()) return;

    QIcon icon = ShellIconManager::getFileIcon(cleanPath, 18);
    QStandardItem* item = new QStandardItem(icon, fi.fileName().isEmpty() ? cleanPath : fi.fileName());
    item->setData(cleanPath, Qt::UserRole + 1);

    m_favoriteModel->appendRow(item);
}

// ... [其余方法保持不变] ...

} // namespace QuarkMeta
```

---

### 2.6 `src/ui/PanelMediator.cpp` 面包屑收藏路由绑定

在 `PanelMediator::setupConnections()` 中无缝桥接地址栏与收藏夹：

```cpp
// PanelMediator.cpp 中追加：
if (addressBar && favoritePanel) {
    connect(addressBar, &AddressBar::favoriteToggleRequested, favoritePanel, 
            [favoritePanel](const QString& fullPath, const QPoint& globalPos) {
        QMenu menu;
        UiHelper::applyMenuStyle(&menu);

        bool isFav = favoritePanel->containsPath(fullPath);
        QAction* actFav = menu.addAction(isFav ? "取消收藏" : "添加至收藏夹");

        QAction* selected = menu.exec(globalPos);
        if (selected == actFav) {
            if (isFav) {
                favoritePanel->removeFavoriteItem(fullPath);
                ToolTipOverlay::instance()->showText(QCursor::pos(), "已从收藏夹移除", 1500, QColor("#e81123"));
            } else {
                favoritePanel->addFavoriteItem(fullPath);
                favoritePanel->saveFavorites();
                ToolTipOverlay::instance()->showText(QCursor::pos(), "已成功添加至收藏夹", 1500, Style::SuccessGreen);
            }
        }
    });
}
```