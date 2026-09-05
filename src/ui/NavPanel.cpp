#include "NavPanel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "Logger.h"
#include "TreeItemDelegate.h"
#include "DropTreeView.h"
#include "ContentPanel.h"
#include "ToolTipOverlay.h"
#include "../core/AppConfig.h"
#include "../core/NavigationHistoryService.h"
#include <QHeaderView>
#include <QScrollBar>
#include <QLabel>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QStandardPaths>
#include <QTimer>
#include <QPushButton>
#include <QPointer>
#include <QMenu>
#include <QtConcurrent>
#include <QApplication>

namespace QuarkMeta {

/**
 * @brief 构造函数，设置面板属性
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

/**
 * @brief 初始化 UI 组件
 */
void NavPanel::deferredInit() {
    if (m_model && m_model->rowCount() > 0) {
        return;
    }

    // 1. 桌面入口 (使用 SVG 语义图标替代原生图标)
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QIcon desktopIcon = UiHelper::getIcon("home", QColor("#3498db"), 18);
    QStandardItem* desktopItem = new QStandardItem(desktopIcon, "桌面");
    desktopItem->setData(desktopPath, Qt::UserRole + 1);
    desktopItem->appendRow(new QStandardItem("Loading..."));
    m_model->appendRow(desktopItem);

    // 2. 此电脑入口
    QIcon computerIcon = UiHelper::getIcon("monitor", QColor("#3498db"), 18);
    QStandardItem* computerItem = new QStandardItem(computerIcon, "此电脑");
    computerItem->setData("computer://", Qt::UserRole + 1);
    m_model->appendRow(computerItem);

    // 3. 磁盘列表 (先填充基础文字路径，0 毫秒卡顿)
    const auto drives = QDir::drives();
    for (const QFileInfo& drive : drives) {
        QString driveName = drive.absolutePath();
        QStandardItem* driveItem = new QStandardItem(driveName);
        driveItem->setData(driveName, Qt::UserRole + 1);
        driveItem->appendRow(new QStandardItem("Loading..."));
        m_model->appendRow(driveItem);
    }

    // 主线程分批次补齐磁盘图标
    QTimer::singleShot(0, [this, drives]() {
        for (int i = 0; i < drives.size(); ++i) {
            if (i + 2 < m_model->rowCount()) {
                QIcon driveIcon = UiHelper::getIcon("hard_drive", QColor("#95a5a6"), 18);
                m_model->item(i + 2)->setIcon(driveIcon);
            }
        }
    });

    // 4. 最近访问 (固定主节点，在所有磁盘正下方)
    QIcon recentIcon = UiHelper::getIcon("clock_history", QColor("#3498db"), 18);
    m_recentRootItem = new QStandardItem(recentIcon, "最近访问");
    m_recentRootItem->setData("recent_root", Qt::UserRole + 1);
    m_model->appendRow(m_recentRootItem);

    // 异步探测填充历史记录，防止慢速物理/网络驱动器阻塞启动
    updateRecentVisitedList();
    if (m_treeView && m_recentRootItem->index().isValid()) {
        m_treeView->expand(m_recentRootItem->index());
    }

    // 5. 回收站 (固定主节点)
    QIcon trashIcon = UiHelper::getIcon("trash", QColor("#e81123"), 18);
    QStandardItem* trashItem = new QStandardItem(trashIcon, "回收站");
    trashItem->setData("trash_root", Qt::UserRole + 1);
    m_model->appendRow(trashItem);
}

void NavPanel::setFocusHighlight(bool visible) {
    Q_UNUSED(visible);
}

void NavPanel::initUi() {
    QWidget* header = new QWidget(this);
    header->setObjectName("ContainerHeader");
    header->setFixedHeight(32);

    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(15, 0, 5, 0);
    headerLayout->setSpacing(5);

    QLabel* iconLabel = new QLabel(header);
    iconLabel->setPixmap(UiHelper::getIcon("list_ul", QColor("#2ecc71"), 18).pixmap(18, 18));
    headerLayout->addWidget(iconLabel);

    QLabel* titleLabel = new QLabel("目录导航", header);
    titleLabel->setObjectName("NavPanelTitleLabel");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    m_mainLayout->addWidget(header);

    m_treeView = new DropTreeView(this);
    m_treeView->setObjectName("NavTreeView");
    m_treeView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_treeView->setHeaderHidden(true);
    if (m_treeView->header()) {
        m_treeView->header()->setStretchLastSection(true);
        m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    }
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(20);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setExpandsOnDoubleClick(true);
    m_treeView->setDragEnabled(true);
    m_treeView->setDragDropMode(QAbstractItemView::DragOnly);
    m_treeView->setItemDelegate(new TreeItemDelegate(this, false));
    m_treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_model = new QStandardItemModel(this);
    m_treeView->setModel(m_model);

    m_mainLayout->addWidget(m_treeView, 1);

    QString arrowRight = UiHelper::getSvgTempFilePath("chevron_right", QColor("#378ADD"));
    QString arrowDown = UiHelper::getSvgTempFilePath("chevron_down", QColor("#378ADD"));
    QString treeStyle = QString(
        "QTreeView#NavTreeView::branch:has-children:closed { image: url(\"%1\"); }"
        "QTreeView#NavTreeView::branch:has-children:open { image: url(\"%2\"); }"
    ).arg(arrowRight, arrowDown);
    m_treeView->setStyleSheet(treeStyle);

    connect(m_treeView, &QTreeView::expanded, this, &NavPanel::onItemExpanded);
    connect(m_treeView, &QTreeView::clicked, this, &NavPanel::onTreeClicked);
    connect(&NavigationHistoryService::instance(), &NavigationHistoryService::historyChanged, this, &NavPanel::updateRecentVisitedList);
}

void NavPanel::updateRecentVisitedList() {
    if (!m_recentRootItem) return;

    QStringList history = NavigationHistoryService::instance().getHistory();
    if (history.isEmpty()) {
        m_recentRootItem->removeRows(0, m_recentRootItem->rowCount());
        return;
    }

    struct RecentItemData {
        QString path;
        QString displayName;
        QIcon icon;
    };

    // 🚀【物理并发加速】：历史路径存在性检测与图标提取异步化，杜绝局域网/失效U盘拖死主线程
    QPointer<NavPanel> weakThis(this);
    (void)QtConcurrent::run([weakThis, history]() {
        QList<RecentItemData> validItems;
        QSet<QString> seenPaths;
        int count = 0;

        for (const QString& path : history) {
            if (count >= 14) break;
            if (path.isEmpty() || path == "computer://" || path.startsWith("trash") || path.startsWith("分类: ")) continue;

            QString normalizedKey = QDir::cleanPath(path).toLower();
            if (seenPaths.contains(normalizedKey)) continue;
            seenPaths.insert(normalizedKey);

            QFileInfo info(path);
            if (!info.exists() || !info.isDir()) continue;

            QString displayName = info.fileName();
            if (displayName.isEmpty()) {
                displayName = QDir::toNativeSeparators(path);
            }

            QIcon icon = ShellIconManager::getFileIcon(path, 18);
            if (icon.isNull()) {
                icon = UiHelper::getIcon("folder_filled", QColor("#3498db"), 18);
            }

            validItems.append({path, displayName, icon});
            count++;
        }

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, validItems]() {
            if (!weakThis || !weakThis->m_recentRootItem) return;

            weakThis->m_recentRootItem->removeRows(0, weakThis->m_recentRootItem->rowCount());
            for (const auto& item : validItems) {
                QStandardItem* child = new QStandardItem(item.icon, item.displayName);
                child->setData(item.path, Qt::UserRole + 1);
                child->setData(QDir::toNativeSeparators(item.path), Qt::UserRole + 2);
                weakThis->m_recentRootItem->appendRow(child);
            }
        }, Qt::QueuedConnection);
    });
}

void NavPanel::setRootPath(const QString& path) {
    Q_UNUSED(path);
}

void NavPanel::selectPath(const QString& path) {
    QString targetData = (path == "trash://" || path == "trash") ? "trash_root" : path;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        QStandardItem* item = m_model->item(i);
        if (item->data(Qt::UserRole + 1).toString() == targetData) {
            m_treeView->setCurrentIndex(item->index());
            m_treeView->setFocus();
            break;
        }
    }
}

void NavPanel::onTreeClicked(const QModelIndex& index) {
    QString path = index.data(Qt::UserRole + 1).toString();
    if (path == "trash_root") {
        emit requestOpenTrash();
    } else if (!path.isEmpty() && path != "computer://" && path != "recent_root") {
        emit directorySelected(path);
    } else if (path == "computer://") {
        emit directorySelected("computer://");
    }
}

void NavPanel::onItemExpanded(const QModelIndex& index) {
    QStandardItem* item = m_model->itemFromIndex(index);
    if (!item) return;

    if (item->rowCount() == 1 && item->child(0)->text() == "Loading...") {
        fetchChildDirs(item);
    }
}

void NavPanel::updateTreeHeight() {
}

bool NavPanel::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::ToolTip) {
        QString text = watched->property("tooltipText").toString();
        if (!text.isEmpty()) {
            ToolTipOverlay::instance()->showText(QCursor::pos(), text, 0);
            return true;
        }
    } else if (event->type() == QEvent::Leave) {
        ToolTipOverlay::hideTip();
    }
    return QFrame::eventFilter(watched, event);
}

void NavPanel::fetchChildDirs(QStandardItem* parent) {
    QString path = parent->data(Qt::UserRole + 1).toString();
    if (path.isEmpty() || path == "computer://") return;

    parent->removeRows(0, parent->rowCount());
    parent->appendRow(new QStandardItem("正在读取..."));

    QPersistentModelIndex pIdx(parent->index());
    (void)QtConcurrent::run([this, pIdx, path]() {
        QDir dir(path);
        QFileInfoList list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

        struct DirInfo { QString name; QString absPath; bool hasSub; };
        QList<DirInfo> results;
        for (const QFileInfo& info : list) {
            QDir subDir(info.absoluteFilePath());
            bool hasSub = !subDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot).isEmpty();
            results << DirInfo{info.fileName(), info.absoluteFilePath(), hasSub};
        }

        QMetaObject::invokeMethod(QCoreApplication::instance(), [this, pIdx, results]() {
            if (!pIdx.isValid()) return;
            QStandardItem* safeParent = m_model->itemFromIndex(pIdx);
            if (!safeParent) return;

            safeParent->removeRows(0, safeParent->rowCount());

            for (const auto& info : results) {
                QIcon folderIcon = ShellIconManager::getFileIcon(info.absPath, 18);
                QStandardItem* child = new QStandardItem(folderIcon, info.name);
                child->setData(info.absPath, Qt::UserRole + 1);

                if (info.hasSub) {
                    child->appendRow(new QStandardItem("Loading..."));
                }
                safeParent->appendRow(child);
            }
        }, Qt::QueuedConnection);
    });
}

} // namespace QuarkMeta