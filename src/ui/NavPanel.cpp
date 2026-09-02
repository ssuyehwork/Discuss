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

    // 2026-07-xx 按照 Plan-63：启用右键菜单
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

    // 1. 新增：桌面入口 (使用 SVG 语义图标替代原生图标)
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QIcon desktopIcon = UiHelper::getIcon("home", QColor("#3498db"), 18);
    QStandardItem* desktopItem = new QStandardItem(desktopIcon, "桌面");
    desktopItem->setData(desktopPath, Qt::UserRole + 1);
    // 增加虚拟子项以便显示展开箭头
    desktopItem->appendRow(new QStandardItem("Loading..."));
    m_model->appendRow(desktopItem);

    // 2. 新增：此电脑入口 (使用 SVG 语义图标替代原生图标)
    // 2026-03-xx 物理加速：先展示文字项，图标通过延时加载或在主线程空闲时补全，防止磁盘休眠导致启动假死
    QIcon computerIcon = UiHelper::getIcon("monitor", QColor("#3498db"), 18);
    QStandardItem* computerItem = new QStandardItem(computerIcon, "此电脑");
    computerItem->setData("computer://", Qt::UserRole + 1);
    m_model->appendRow(computerItem);

    // 3. 磁盘列表 (逻辑异步预备：先填充基础文字路径)
    const auto drives = QDir::drives();
    for (const QFileInfo& drive : drives) {
        QString driveName = drive.absolutePath();
        QStandardItem* driveItem = new QStandardItem(driveName);
        driveItem->setData(driveName, Qt::UserRole + 1);
        driveItem->appendRow(new QStandardItem("Loading..."));
        m_model->appendRow(driveItem);
    }

    // 2026-03-xx 线程安全修复：图标提取必须在主线程执行。
    // 为了平衡性能与安全，图标提取在主线程分批次（Idle 状态）补全。
    QTimer::singleShot(0, [this, drives]() {
        for (int i = 0; i < drives.size(); ++i) {
            if (i + 2 < m_model->rowCount()) {
                QIcon driveIcon = UiHelper::getIcon("hard_drive", QColor("#95a5a6"), 18);
                m_model->item(i + 2)->setIcon(driveIcon);
            }
        }
    });

    // 4. 新增：最近访问 (固定主节点，在所有磁盘正下方)
    QIcon recentIcon = UiHelper::getIcon("clock_history", QColor("#3498db"), 18);
    m_recentRootItem = new QStandardItem(recentIcon, "最近访问");
    m_recentRootItem->setData("recent_root", Qt::UserRole + 1);
    m_model->appendRow(m_recentRootItem);

    updateRecentVisitedList();

    // 5. 新增：回收站 (固定主节点，在“最近访问”正下方)
    QIcon trashIcon = UiHelper::getIcon("trash", QColor("#e81123"), 18);
    QStandardItem* trashItem = new QStandardItem(trashIcon, "回收站");
    trashItem->setData("trash_root", Qt::UserRole + 1);
    m_model->appendRow(trashItem);
}

void NavPanel::setFocusHighlight(bool visible) {
    Q_UNUSED(visible);
}

void NavPanel::initUi() {
    // 面板标题 (2026-xx-xx 按照 Plan-96：作为顶层固定标题)
    QWidget* header = new QWidget(this);
    header->setObjectName("ContainerHeader");
    header->setFixedHeight(32);
// ContainerHeader in style.qss
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

    // --- 磁盘树 ---
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
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setExpandsOnDoubleClick(true);
    m_treeView->setDragEnabled(true);
    m_treeView->setDragDropMode(QAbstractItemView::DragOnly);
    m_treeView->setItemDelegate(new TreeItemDelegate(this, false));
    // 物理恢复：允许内部滚动
    m_treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_model = new QStandardItemModel(this);
    m_treeView->setModel(m_model);

    m_mainLayout->addWidget(m_treeView, 1);


    // 信号连接
    connect(m_treeView, &QTreeView::expanded, this, &NavPanel::onItemExpanded);
    connect(m_treeView, &QTreeView::clicked, this, &NavPanel::onTreeClicked);
    connect(&NavigationHistoryService::instance(), &NavigationHistoryService::historyChanged, this, &NavPanel::updateRecentVisitedList);
}

void NavPanel::updateRecentVisitedList() {
    if (!m_recentRootItem) return;

    m_recentRootItem->removeRows(0, m_recentRootItem->rowCount());

    QStringList history = NavigationHistoryService::instance().getHistory();
    QSet<QString> seenPaths;
    int count = 0;

    for (const QString& path : history) {
        if (count >= 14) break;
        if (path.isEmpty() || path == "computer://" || path.startsWith("分类: ")) continue;

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

        QStandardItem* child = new QStandardItem(icon, displayName);
        child->setData(path, Qt::UserRole + 1);
        child->setData(QDir::toNativeSeparators(path), Qt::UserRole + 2);

        m_recentRootItem->appendRow(child);
        count++;
    }

    if (m_treeView && m_recentRootItem->index().isValid()) {
        m_treeView->expand(m_recentRootItem->index());
    }
}

/**
 * @brief 设置当前显示的根路径并自动展开
 */
void NavPanel::setRootPath(const QString& path) {
    Q_UNUSED(path);
    // 由于改为扁平化快捷入口列表，不再支持 setRootPath 的树深度同步
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

/**
 * @brief 当用户点击目录时，发出信号告知外部组件（如内容面板）
 */
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

    // 如果只有一个 Loading 子项，则触发真实加载
    if (item->rowCount() == 1 && item->child(0)->text() == "Loading...") {
        fetchChildDirs(item);
    }
}

void NavPanel::updateTreeHeight() {
    // 2026-xx-xx 按照 Plan-107：废弃手动高度计算，解锁 Splitter 自由拉伸
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

/**
 * @brief 异步获取子目录，解决展开文件夹时的界面假死 (2026-05-25 物理加速)
 */
void NavPanel::fetchChildDirs(QStandardItem* parent) {
    QString path = parent->data(Qt::UserRole + 1).toString();
    if (path.isEmpty() || path == "computer://") return;

    parent->removeRows(0, parent->rowCount());
    parent->appendRow(new QStandardItem("正在读取..."));

    // 2026-05-25 编译修复：QStandardItem 不继承自 QObject，严禁使用 QPointer。
    // 改用 QPersistentModelIndex 确保异步回调时索引的有效性。
    QPersistentModelIndex pIdx(parent->index());
    (void)QtConcurrent::run([this, pIdx, path]() {
        QDir dir(path);
        // 执行耗时的物理磁盘读取
        QFileInfoList list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

        struct DirInfo { QString name; QString absPath; bool hasSub; };
        QList<DirInfo> results;
        for (const QFileInfo& info : list) {
            QDir subDir(info.absoluteFilePath());
            bool hasSub = !subDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot).isEmpty();
            results << DirInfo{info.fileName(), info.absoluteFilePath(), hasSub};
        }

        // 投递回主线程进行 UI 更新
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
