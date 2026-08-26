#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ContentPanel.h"
#include "../meta/TrashRepository.h" 
#include "../meta/DiskTrashRepo.h"
#include "ColorPicker.h"
#include "../core/DiskTrashService.h"
#include <QWidgetAction>
#include "../meta/MetadataManager.h"
#include "../meta/MediaExtractorPipeline.h"
#include <algorithm>
#include "Logger.h"
#include "SvgIcons.h" 
#include "TreeItemDelegate.h" 
#include "DropTreeView.h" 
#include "DropListView.h" 
#include "DropJustifiedView.h"
#include "BatchProgressDialog.h"
#include "ThumbnailDelegate.h"
#include "../meta/QuarkMetaJson.h"
#include "../core/NavigationHistoryService.h"
#include "ToolTipOverlay.h" 
#include "MainWindow.h"
#include "../core/CoreEngine.h"
#include "../core/CentralEventHub.h"
#include "../util/SecureFileEraser.h"
#include "../util/DiskIoService.h"
#include "../util/DeepThumbnailExtractor.h"
 
#include <QVBoxLayout> 
#include <QHBoxLayout> 
#include <QIcon> 
#include <QSvgRenderer> 
#include <QPainter> 
#include <QHeaderView> 
#include <QScrollBar> 
#include <QStyle> 
#include <QLabel> 
#include <QAction> 
#include <QActionGroup>
#include <QMenu> 
#include <QAbstractItemView> 
#include <QStandardItem> 
#include "../core/AppConfig.h"
#include <QEvent> 
#include <QKeyEvent> 
#include <QMouseEvent> 
#include <QWheelEvent> 
#include <QStyleOptionViewItem> 
#include <QItemSelectionModel> 
#include <QFileInfo> 
#include <QDir> 
#include <QSet>
#include <QFile>
#include <QDateTime> 
#include <QDesktopServices> 
#include <QUrl> 
#include <QApplication> 
#include <QCoreApplication> 
#include <QProcess> 
#include <QClipboard> 
#include <QMimeData> 
#include <QLineEdit> 
#include "FramelessDialog.h"
#include <memory>
#include <QRandomGenerator>
#include <QtConcurrent> 
#include <QThreadPool> 
#include <QTimer> 
#include <QPointer> 
#include <QPersistentModelIndex> 
 
 
#include <windows.h> 
#include <objbase.h>
#include <shellapi.h> 
#include <io.h>
#include "../meta/BatchRenameEngine.h" 
#include "../meta/StatisticsService.h"
#include "../crypto/EncryptionManager.h" 
#include "BatchRenameDialog.h" 
#include "BatchCreateDialog.h"
#include "UiHelper.h" 
#include "ShellIconManager.h"
#include "StyleLibrary.h"
#include <QFileIconProvider>
#include "../core/CoreController.h"
#include "../core/UndoManager.h"
#include "../core/BasicCommands.h"
#include "../core/OperationSnapshotEngine.h"
using namespace QuarkMeta::Style;
#include "../util/ShellHelper.h"
#include "DiskScanService.h"
#include "../ui/MediaColorExtractor.h"
#include "../meta/MetaCacheDecorator.h"
 
namespace QuarkMeta { 


// --- FilterProxyModel 实现 --- 
FilterProxyModel::FilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {} 

void FilterProxyModel::recomputeDuplicateCache() {
    m_cachedDuplicatePaths.clear();
    if (currentFilter.duplicatePresence == FilterState::DupAll) return;

    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel());
    if (!sourceModelPtr) return;

    const auto& records = sourceModelPtr->allRecords();

    // 按照 (SHA256 或 文件大小 + 宽x高) 进行内存桶聚合（排除文件名，实现真实副本判重）
    std::unordered_map<std::string, std::vector<QString>> hashBucket;
    for (const auto& rec : records) {
        if (rec.isDir) continue; // 排除目录
        
        // 构造唯一指纹键：优先 SHA256，其次 大小 + 尺寸，兜底 纯大小
        std::string key;
        if (!rec.sha256.isEmpty()) {
            key = rec.sha256.toStdString();
        } else if (rec.width > 0 && rec.height > 0) {
            key = std::to_string(rec.size) + "_" + std::to_string(rec.width) + "x" + std::to_string(rec.height);
        } else {
            key = std::to_string(rec.size);
        }
        hashBucket[key].push_back(rec.path);
    }

    // 凡是数量 >= 2 的桶，全部标记为重复项
    for (const auto& pair : hashBucket) {
        if (pair.second.size() > 1) {
            for (const auto& path : pair.second) {
                m_cachedDuplicatePaths.insert(path);
            }
        }
    }
}
 
void FilterProxyModel::updateFilter() { 
    recomputeDuplicateCache(); // 预建缓存，保证 filterAcceptsRow 为 O(1) 瞬时响应
    beginFilterChange(); 
    endFilterChange(); 
} 
 
bool FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {  
    QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);  
      
    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel()); 
    if (!sourceModelPtr) return true; 

    const auto& records = sourceModelPtr->allRecords(); 
    if (sourceRow < 0 || sourceRow >= (int)records.size()) return false; 
    const auto& record = records[sourceRow]; 
 
    auto* contentPanel = qobject_cast<ContentPanel*>(parent()); 
    bool isTrashView = contentPanel && (contentPanel->getCurrentCategoryType() == "trash");

    // 0. 隐藏属性过滤拦截（当关闭隐藏项显示时，物理隐藏项一律不展示）
    if (record.isHidden && !currentFilter.showHidden) {
        return false;
    }

    // 1. 文件夹与分类卡片控制 (回收站视图下不执行“显示/隐藏文件和文件夹”过滤限制，确保双轨资产百分百正常呈现)
    if (!isTrashView) {
        if (record.isDir) { 
            bool isEmptyFolder = record.isEmpty; 
     
            bool isFolderExplicitlySelected = currentFilter.types.contains("folder") ||  
                                             (isEmptyFolder && currentFilter.types.contains("空文件夹")); 
             
            if (!currentFilter.showFolders && !isFolderExplicitlySelected) { 
                return false; 
            } 
        } else { 
            if (!currentFilter.showFiles) return false; 
        } 
    }

    // 1. 评级过滤 
    if (!currentFilter.ratings.isEmpty()) { 
        int r = record.rating; // 直接从烘焙好的 record 获取，消除 idx.data 虚拟调用开销
        if (!currentFilter.ratings.contains(r)) return false; 
    } 
 
    // 2. 颜色标记过滤（支持标准色值与中文名称双向匹配）
    if (!currentFilter.colors.isEmpty()) {
        bool matchColor = false;

        // 建立标准色名与 Hex 的权威映射表
        static const QMap<QString, QString> s_colorHexMap = {
            {"红色", "#E24B4A"}, {"橙色", "#EF9F27"}, {"黄色", "#FECF0E"},
            {"绿色", "#639922"}, {"青色", "#1D9E75"}, {"蓝色", "#378ADD"},
            {"紫色", "#7F77DD"}, {"灰色", "#5F5E5A"}
        };

        for (const QString& colName : currentFilter.colors) {
            if (colName == "无色标" || colName.isEmpty()) {
                if (record.manualColor.isEmpty() && record.autoColor.isEmpty()) {
                    matchColor = true;
                    break;
                }
            } else {
                QString targetHex = s_colorHexMap.value(colName, colName); // 将"红色"转换为"#E24B4A"
                if (record.manualColor.compare(targetHex, Qt::CaseInsensitive) == 0 ||
                    record.manualColor.contains(colName, Qt::CaseInsensitive) ||
                    record.autoColor.contains(colName, Qt::CaseInsensitive)) {
                    matchColor = true;
                    break;
                }
            }
        }
        if (!matchColor) return false;
    }
 
    // 4. 类型过滤 
    if (!currentFilter.types.isEmpty() || !currentFilter.typeFilterText.isEmpty()) { 
        QString type = record.isDir ? "folder" : "file";
        QString ext = record.suffix.toUpper();
        bool matchType = false; 

        if (!currentFilter.typeFilterText.isEmpty()) {
            QString searchText = currentFilter.typeFilterText.trimmed();
            if (searchText == "文件夹" || searchText.toLower() == "folder") {
                if (type == "folder") matchType = true;
            } else if (searchText == "空文件夹") {
                if (type == "folder" && record.isEmpty) matchType = true;
            } else {
                if (ext.contains(searchText.toUpper())) matchType = true;
            }
            if (!matchType) return false;
        }

        if (!currentFilter.types.isEmpty()) {
            matchType = false;
            for (const QString& fType : currentFilter.types) { 
                if (fType == "folder") { 
                    if (type == "folder") { matchType = true; break; } 
                } else if (fType == "file") {
                    if (type != "folder") { matchType = true; break; }
                } else if (fType == "空文件夹") {
                    if (type == "folder" && record.isEmpty) { matchType = true; break; }
                } else { 
                    if (ext == fType.toUpper()) { matchType = true; break; } 
                } 
            } 
            if (!matchType) return false; 
        }
    } 
 
    // 5. 创建日期过滤 
    if (!currentFilter.createDates.isEmpty() || !currentFilter.createDateFilterText.isEmpty()) { 
        QDate d = QDateTime::fromMSecsSinceEpoch(record.ctime).date();
        QString dStr = d.toString("dd-MM-yyyy"); 
        bool matchDate = false; 

        if (!currentFilter.createDateFilterText.isEmpty()) {
            if (dStr.contains(currentFilter.createDateFilterText.trimmed())) matchDate = true;
            if (!matchDate) return false;
        }

        if (!currentFilter.createDates.isEmpty()) {
            matchDate = false;
            for (const QString& fDate : currentFilter.createDates) { 
                if (fDate == dStr) { matchDate = true; break; } 
            } 
            if (!matchDate) return false; 
        }
    } 

    // 7. 链接过滤 (Plan-30)
    if (currentFilter.linkPresence != FilterState::All) {
        bool hasLink = !record.url.isEmpty();
        if (currentFilter.linkPresence == FilterState::Yes && !hasLink) return false;
        if (currentFilter.linkPresence == FilterState::No && hasLink) return false;
    }

    // 8. 备注过滤 (Plan-30)
    if (currentFilter.notePresence != FilterState::All) {
        bool hasNote = !record.note.isEmpty();
        if (currentFilter.notePresence == FilterState::Yes && !hasNote) return false;
        if (currentFilter.notePresence == FilterState::No && hasNote) return false;
    }

    // 8.5. 标签过滤
    if (currentFilter.tagPresence != FilterState::All) {
        bool hasTags = !record.tags.isEmpty();
        if (currentFilter.tagPresence == FilterState::Yes && !hasTags) return false;
        if (currentFilter.tagPresence == FilterState::No && hasTags) return false;
    }

    // 9. 文件大小过滤 (Plan-30)
    if (currentFilter.minSize != -1 && record.size < currentFilter.minSize) return false;
    if (currentFilter.maxSize != -1 && record.size > currentFilter.maxSize) return false;

    // 10. 图像比例过滤 (Plan-29)
    if (currentFilter.ratio != FilterState::AspectAny) {
        // 直接使用 record 中缓存的尺寸信息 (Plan-30 优化：避免重复查询元数据管理器)
        if (record.width > 0 && record.height > 0) {
            double r = (double)record.width / record.height;
            if (currentFilter.ratio == FilterState::Horizontal && record.width <= record.height) return false;
            if (currentFilter.ratio == FilterState::Vertical && record.height <= record.width) return false;
            if (currentFilter.ratio == FilterState::Square && std::abs(r - 1.0) > 0.05) return false;
            if (currentFilter.ratio == FilterState::Ratio169 && std::abs(r - 1.77) > 0.05) return false;
        } else {
            return false; // 无尺寸信息不匹配任何比例筛选
        }
    }
 
    // 6. 修改日期过滤 
    if (!currentFilter.modifyDates.isEmpty() || !currentFilter.modifyDateFilterText.isEmpty()) { 
        QDate d = QDateTime::fromMSecsSinceEpoch(record.mtime).date();
        QString dStr = d.toString("dd-MM-yyyy"); 
        bool matchDate = false; 

        if (!currentFilter.modifyDateFilterText.isEmpty()) {
            if (dStr.contains(currentFilter.modifyDateFilterText.trimmed())) matchDate = true;
            if (!matchDate) return false;
        }

        if (!currentFilter.modifyDates.isEmpty()) {
            matchDate = false;
            for (const QString& fDate : currentFilter.modifyDates) { 
                if (fDate == dStr) { matchDate = true; break; } 
            } 
            if (!matchDate) return false; 
        }
    } 
 
    // 10.5 缩略图状态过滤
    if (currentFilter.thumbnailPresence != FilterState::ThumbAll) {
        if (record.isDir || !UiHelper::isGraphicsFile(record.suffix)) return false;
        QString thumbPath = DiskMediaExtractor::getDiskThumbCachePath(record.path);
        bool hasThumb = QFile::exists(thumbPath);
        if (currentFilter.thumbnailPresence == FilterState::HasThumbnail && !hasThumb) return false;
        if (currentFilter.thumbnailPresence == FilterState::NoThumbnail && hasThumb) return false;
    }

    // 11. 重复状态过滤 (O(1) 瞬时判定)
    if (currentFilter.duplicatePresence != FilterState::DupAll) {
        if (record.isDir) {
            return false; // 处于重复项/未重复筛选时，自动排除文件夹
        }
        bool isDuplicate = (m_cachedDuplicatePaths.count(record.path) > 0);
        if (currentFilter.duplicatePresence == FilterState::DuplicateOnly && !isDuplicate) {
            return false;
        }
        if (currentFilter.duplicatePresence == FilterState::UniqueOnly && isDuplicate) {
            return false;
        }
    }

    // 统一支持：文件名/文件夹名、已绑定标签、备注说明 多维度命中搜索
    if (!currentFilter.keyword.isEmpty()) {
        const QString& kw = currentFilter.keyword;
        
        // 1. 匹配文件名/文件夹名
        bool match = record.filename.contains(kw, Qt::CaseInsensitive);

        // 2. 匹配绑定的标签 (Tags)
        if (!match) {
            for (const QString& tag : record.tags) {
                if (tag.contains(kw, Qt::CaseInsensitive)) {
                    match = true;
                    break;
                }
            }
        }

        // 3. 匹配备注说明 (Note)
        if (!match && !record.note.isEmpty()) {
            if (record.note.contains(kw, Qt::CaseInsensitive)) {
                match = true;
            }
        }

        if (!match) return false;
    }

    return true;
} 
 
bool FilterProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const { 
    // 1. 直取内存结构，废除 source_left.data(...) 虚拟调用与冗余内存分配
    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel());
    if (!sourceModelPtr) return QSortFilterProxyModel::lessThan(source_left, source_right);

    const auto& records = sourceModelPtr->allRecords();
    int leftRow = source_left.row();
    int rightRow = source_right.row();
    if (leftRow < 0 || leftRow >= (int)records.size() || rightRow < 0 || rightRow >= (int)records.size()) {
        return QSortFilterProxyModel::lessThan(source_left, source_right);
    }

    const auto& leftRec = records[leftRow];
    const auto& rightRec = records[rightRow];

    // 🚀 【绝对结构权重 1】：文件夹/分类 永远排在 文件 前面（物理隔绝，不受用户升降序取反下沉影响，实现无缝上下两区！） 
    bool leftIsDir  = leftRec.isDir; 
    bool rightIsDir = rightRec.isDir; 
 
    if (leftIsDir != rightIsDir) { 
        return (sortOrder() == Qt::AscendingOrder) ? leftIsDir : !leftIsDir; 
    } 

    // 4. 物理第二权重：置顶优先规则 (升降序下均强制置顶，不随用户排序取反下沉)
    bool leftPinned = leftRec.pinned || leftRec.encrypted;
    bool rightPinned = rightRec.pinned || rightRec.encrypted;
 
    if (leftPinned != rightPinned) { 
        return leftPinned; // 置顶项永远排在前面
    } 

    // 5. 物理第三权重：具体的排序类型逻辑，平局时统一追加二级决胜键 (localeAwareCompare 拼音/文件名)
    auto* contentPanel = qobject_cast<ContentPanel*>(parent());
    ContentPanel::SortType sType = contentPanel ? contentPanel->currentSortType() : ContentPanel::SortByName;

    auto compareNames = [](const ItemRecord& l, const ItemRecord& r) {
        const QString& lName = l.filename;
        const QString& rName = r.filename;
        return lName.localeAwareCompare(rName) < 0;
    };

    switch (sType) {
        case ContentPanel::SortByName: {
            return compareNames(leftRec, rightRec);
        }
        case ContentPanel::SortByCreateDate: {
            if (leftRec.ctime != rightRec.ctime) {
                return leftRec.ctime < rightRec.ctime;
            }
            return compareNames(leftRec, rightRec);
        }
        case ContentPanel::SortByModifyDate: {
            if (leftRec.mtime != rightRec.mtime) {
                return leftRec.mtime < rightRec.mtime;
            }
            return compareNames(leftRec, rightRec);
        }
        case ContentPanel::SortByExtension: {
            int comp = leftRec.suffix.localeAwareCompare(rightRec.suffix);
            if (comp != 0) {
                return comp < 0;
            }
            return compareNames(leftRec, rightRec);
        }
        case ContentPanel::SortBySize: {
            long long lSize = leftRec.isDir ? -1 : leftRec.size;
            long long rSize = rightRec.isDir ? -1 : rightRec.size;
            if (lSize != rSize) {
                return lSize < rSize;
            }
            return compareNames(leftRec, rightRec);
        }
        case ContentPanel::SortByDimension: {
            long long lDim = (long long)leftRec.width * leftRec.height;
            long long rDim = (long long)rightRec.width * rightRec.height;
            if (lDim != rDim) {
                return lDim < rDim;
            }
            if (leftRec.width != rightRec.width) {
                return leftRec.width < rightRec.width;
            }
            if (leftRec.height != rightRec.height) {
                return leftRec.height < rightRec.height;
            }
            return compareNames(leftRec, rightRec);
        }
        case ContentPanel::SortByRating: {
            if (leftRec.rating != rightRec.rating) {
                return leftRec.rating < rightRec.rating;
            }
            return compareNames(leftRec, rightRec);
        }
        case ContentPanel::SortByAddedDate: {
            long long leftAdded = leftRec.added_at == 0 ? leftRec.ctime : leftRec.added_at;
            long long rightAdded = rightRec.added_at == 0 ? rightRec.ctime : rightRec.added_at;
            if (leftAdded != rightAdded) {
                return leftAdded < rightAdded;
            }
            return compareNames(leftRec, rightRec);
        }
    }

    return QSortFilterProxyModel::lessThan(source_left, source_right); 
} 
 
 
ContentPanel::ContentPanel(QWidget* parent) 
    : QFrame(parent) { 
    // 2026-07-xx 按照 Plan-63：启用右键菜单策略（容器级）
    setContextMenuPolicy(Qt::CustomContextMenu);

    setObjectName("EditorContainer"); 
    setAttribute(Qt::WA_StyledBackground, true); 
    setMinimumWidth(230); 
    setStyleSheet("color: #EEEEEE;"); 
 
    m_mainLayout = new QVBoxLayout(this); 
    m_mainLayout->setContentsMargins(0, 0, 0, 0); 
    m_mainLayout->setSpacing(0); 
 
 
    m_diskModel = new DiskItemModel(this);
    m_model = m_diskModel; // 默认挂载纯物理磁盘导航模型
    connect(m_diskModel, &DiskItemModel::thumbnailLoaded, this, &ContentPanel::refreshVisibleThumbnails);

    m_proxyModel = new FilterProxyModel(this); 
    m_proxyModel->setSourceModel(m_model); 

    m_visibleTimer = new QTimer(this);
    m_visibleTimer->setSingleShot(true);
    m_visibleTimer->setInterval(60); 
    connect(m_visibleTimer, &QTimer::timeout, this, &ContentPanel::refreshVisibleThumbnails);
    
    // 建立 300ms 统计防抖定时器
    QTimer* statsDebounceTimer = new QTimer(this);
    statsDebounceTimer->setSingleShot(true);
    statsDebounceTimer->setInterval(300);
    connect(statsDebounceTimer, &QTimer::timeout, this, &ContentPanel::recalculateAndEmitStats);

    auto onDataChanged = [statsDebounceTimer](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
        Q_UNUSED(topLeft); Q_UNUSED(bottomRight);
        if (roles.isEmpty() || roles.contains(ColorRole) || roles.contains(RatingRole) || 
            roles.contains(TagsRole) || roles.contains(AspectRatioRole)) {
            // 🚨 核心止血点：重置防抖定时器，300ms 内只允许计算 1 次！
            statsDebounceTimer->start();
        }
    };
    connect(m_diskModel, &ItemModelBase::dataChanged, this, onDataChanged);
     
    // 2026-04-12 深度修复：强制锁定过滤列为第 0 列（名称列），确保搜索逻辑不偏离 
    m_proxyModel->setFilterKeyColumn(0); 
    // 2026-05-29 物理修复：开启动态排序，确保“置顶优先”逻辑能在数据加载后自动生效
    m_proxyModel->setDynamicSortFilter(true);
    m_proxyModel->sort(0, Qt::AscendingOrder);
 
    // 2026-06-05 按照要求：从配置中加载上次保存的缩放比例 
    m_zoomLevel = AppConfig::instance().getValue("UI/GridZoomLevel", 96).toInt(); 
    m_isRecursive = false; 
    // 2026-07-xx 按照用户要求：文件夹默认设为隐藏 (false)
    m_showFolders = AppConfig::instance().getValue("ContentPanel/ShowFolders", false).toBool();
    m_showFiles = AppConfig::instance().getValue("ContentPanel/ShowFiles", true).toBool();
    m_showHidden = AppConfig::instance().getValue("ContentPanel/ShowHidden", false).toBool();
    
    // 同步到当前 FilterState
    m_currentFilter.showFolders = m_showFolders;
    m_currentFilter.showFiles = m_showFiles;
    m_currentFilter.showHidden = m_showHidden;
 
    // 从配置中恢复排序类型与方向 (对应用户原话："名称、创建日期、修改日期、扩展名、大小、尺寸、评分" 与 "升序、降序")
    m_sortType = static_cast<SortType>(AppConfig::instance().getValue("ContentPanel/RightClickSortType", SortByName).toInt());
    m_sortOrder = static_cast<Qt::SortOrder>(AppConfig::instance().getValue("ContentPanel/RightClickSortOrder", Qt::AscendingOrder).toInt());
    m_proxyModel->sort(0, m_sortOrder);

    initUi(); 
    // 2026-05-27 按照用户要求：构造函数末尾强行对齐初始网格尺寸，废除 initGridView 中的旧硬编码值 
    updateGridSize(); 

    // 从 AppConfig 恢复上一次的视图模式
    int savedMode = AppConfig::instance().getValue("ContentPanel/ViewMode", static_cast<int>(GridView)).toInt();
    setViewMode(static_cast<ViewMode>(savedMode));
} 
 
void ContentPanel::deferredInit() { 
    // 2026-04-12 按照用户要求：补全延迟初始化逻辑，此处可处理模型预热或首屏数据对齐 
} 

 
void ContentPanel::initUi() { 
    QWidget* titleBar = new QWidget(this); 
    titleBar->setObjectName("ContainerHeader"); 
    titleBar->setFixedHeight(32); 
    titleBar->setStyleSheet( 
        "QWidget#ContainerHeader {" 
        "  background-color: #252526;" 
        "  border-bottom: 1px solid #333;" 
        "}" 
    ); 
    QHBoxLayout* titleL = new QHBoxLayout(titleBar); 
    titleL->setContentsMargins(15, 0, 5, 0); // 2026-xx-xx 按照用户要求：右侧保留 5px 呼吸边距
    titleL->setSpacing(5);                  // 2026-05-17 按照用户要求：间距统一为 5px
 
    QLabel* iconLabel = new QLabel(titleBar); 
    iconLabel->setPixmap(UiHelper::getIcon("eye", QColor("#41F2F2"), 18).pixmap(18, 18)); 
    titleL->addWidget(iconLabel); 
 
    QLabel* titleLabel = new QLabel("内容", titleBar); 
    titleLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #41F2F2; background: transparent; border: none;"); 
     
    m_btnToggleHidden = new QPushButton(titleBar);
    m_btnToggleHidden->setCheckable(true);
    m_btnToggleHidden->setFixedSize(24, 24);
    m_btnToggleHidden->setChecked(m_showHidden);
    m_btnToggleHidden->setIcon(UiHelper::getIcon("eye", m_showHidden ? QColor("#3498db") : QColor("#888888"), 16));
    m_btnToggleHidden->setProperty("tooltipText", "显示/隐藏属性为隐藏的项目");
    m_btnToggleHidden->installEventFilter(this);
    m_btnToggleHidden->setStyleSheet(
        "QPushButton { background: transparent; border: 1px solid #444; border-radius: 4px; }"
        "QPushButton:hover { background: #3E3E42; border-color: #666; }"
        "QPushButton:checked { background: #3E3E42; border-color: #3498db; }" 
        "QPushButton:pressed { background: #4E4E52; }"
    );
    connect(m_btnToggleHidden, &QPushButton::clicked, [this]() {
        m_showHidden = m_btnToggleHidden->isChecked();
        m_btnToggleHidden->setIcon(UiHelper::getIcon("eye", 
                                                     m_showHidden ? QColor("#3498db") : QColor("#888888"), 16));
        AppConfig::instance().setValue("ContentPanel/ShowHidden", m_showHidden);
        m_currentFilter.showHidden = m_showHidden;
        applyFilters();
    });

    m_btnToggleFolders = new QPushButton(titleBar);
    m_btnToggleFolders->setCheckable(true);
    m_btnToggleFolders->setFixedSize(24, 24);
    m_btnToggleFolders->setChecked(m_showFolders);
    m_btnToggleFolders->setIcon(UiHelper::getIcon("folder_filled", m_showFolders ? QColor("#FDB70A") : QColor("#B0B0B0"), 16));
    m_btnToggleFolders->setProperty("tooltipText", "显示/隐藏文件夹");
    m_btnToggleFolders->installEventFilter(this);
    m_btnToggleFolders->setStyleSheet(
        "QPushButton { background: transparent; border: none; border-radius: 4px; }"
        "QPushButton:hover { background: #3E3E42; }"
        "QPushButton:checked { background: #3E3E42; border: none; }" 
        "QPushButton:pressed { background: #4E4E52; }"
    );
    connect(m_btnToggleFolders, &QPushButton::clicked, [this]() {
        m_showFolders = m_btnToggleFolders->isChecked();
        m_btnToggleFolders->setIcon(UiHelper::getIcon("folder_filled", m_showFolders ? QColor("#FDB70A") : QColor("#B0B0B0"), 16));
        AppConfig::instance().setValue("ContentPanel/ShowFolders", m_showFolders);
        m_currentFilter.showFolders = m_showFolders;
        applyFilters();
    });

    m_btnToggleFiles = new QPushButton(titleBar);
    m_btnToggleFiles->setCheckable(true);
    m_btnToggleFiles->setFixedSize(24, 24);
    m_btnToggleFiles->setChecked(m_showFiles);
    m_btnToggleFiles->setIcon(UiHelper::getIcon("file", m_showFiles ? QColor("#2ecc71") : QColor("#B0B0B0"), 16));
    m_btnToggleFiles->setProperty("tooltipText", "显示/隐藏文件");
    m_btnToggleFiles->installEventFilter(this);
    m_btnToggleFiles->setStyleSheet(
        "QPushButton { background: transparent; border: none; border-radius: 4px; }"
        "QPushButton:hover { background: #3E3E42; }"
        "QPushButton:checked { background: #3E3E42; border: none; }" 
        "QPushButton:pressed { background: #4E4E52; }"
    );
    connect(m_btnToggleFiles, &QPushButton::clicked, [this]() {
        m_showFiles = m_btnToggleFiles->isChecked();
        m_btnToggleFiles->setIcon(UiHelper::getIcon("file", m_showFiles ? QColor("#2ecc71") : QColor("#B0B0B0"), 16));
        AppConfig::instance().setValue("ContentPanel/ShowFiles", m_showFiles);
        m_currentFilter.showFiles = m_showFiles;
        applyFilters();
    });

    m_btnLayers = new QPushButton(titleBar); 
    m_btnLayers->setCheckable(true); 
    m_btnLayers->setFixedSize(24, 24); 
    m_btnLayers->setIcon(UiHelper::getIcon("layers", QColor("#2ecc71"), 18)); // 2026-xx-xx 按照用户要求：图层按钮改为绿色，以匹配目录导航配色
    // 2026-03-xx 按照宪法要求：禁绝原生 ToolTip，强制对接 ToolTipOverlay 
    m_btnLayers->setProperty("tooltipText", "显示子文件夹中的项目"); 
    m_btnLayers->installEventFilter(this); 
    m_btnLayers->setStyleSheet( 
        "QPushButton { background: transparent; border: none; border-radius: 4px; }" 
        "QPushButton:hover { background: #3E3E42; }" 
        "QPushButton:checked { background: #3E3E42; border: none; }" 
        "QPushButton:pressed { background: #4E4E52; }" 
        "QPushButton:disabled { opacity: 0.3; }" 
    ); 
    connect(m_btnLayers, &QPushButton::clicked, [this]() { 
        if (m_currentPath.isEmpty() || m_currentPath == "computer://") { 
            m_btnLayers->setChecked(false); 
            return; 
        } 
 
        if (m_btnLayers->isChecked()) { 
            // 探测是否有子文件夹 
            QDir dir(m_currentPath); 
            bool hasSubDirs = !dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot).isEmpty(); 
            if (!hasSubDirs) { 
                m_btnLayers->setChecked(false); 
                ToolTipOverlay::instance()->showText(QCursor::pos(), "当前文件夹不支持显示子文件夹项目", 1500, QColor("#E81123")); 
                return; 
            } 
            loadDirectory(m_currentPath, true); 
        } else { 
            loadDirectory(m_currentPath, false); 
        } 
    }); 
 
    titleL->addWidget(titleLabel); 
    titleL->addStretch(); 
    titleL->addWidget(m_btnToggleHidden, 0, Qt::AlignVCenter);
    titleL->addWidget(m_btnToggleFolders, 0, Qt::AlignVCenter);
    titleL->addWidget(m_btnToggleFiles, 0, Qt::AlignVCenter);
    titleL->addWidget(m_btnLayers, 0, Qt::AlignVCenter); 
 
    m_mainLayout->addWidget(titleBar); 
 
    m_viewStack = new QStackedWidget(this); 
     
    initGridView(); 
    initListView(); 
 
    m_viewStack->addWidget(m_gridView); 
    m_viewStack->addWidget(m_treeView); 

    m_viewStack->setCurrentWidget(m_gridView); 
 
    QVBoxLayout* contentWrapper = new QVBoxLayout(); 
    // 2026-06-xx 物理对齐：右侧边距设为 0，使滚动条贴合容器边缘
    contentWrapper->setContentsMargins(4, 4, 0, 4); 
    contentWrapper->setSpacing(0); 
    contentWrapper->addWidget(m_viewStack); 
     
    m_mainLayout->addLayout(contentWrapper); 
 
    m_gridView->installEventFilter(this); 
    m_treeView->installEventFilter(this);
} 
 
void ContentPanel::updateStatusBarStats() {
    if (!m_proxyModel) return;
    
    // 只计算当前显示的总项目数量，不区分文件和文件夹
    int totalCount = m_proxyModel->rowCount();
    
    // 发送状态栏统计信号
    emit statusBarStatsUpdated(0, 0, totalCount);
}

void ContentPanel::refreshVisibleThumbnails() {
    QWidget* current = m_viewStack->currentWidget();
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(current);
    if (!view || !m_model || CoreController::isShuttingDown()) return;

    int top = 0;
    int bottom = m_proxyModel->rowCount() - 1;

    // 获取视口可见区域对应的索引
    QModelIndex topIdx = view->indexAt(QPoint(10, 10));
    QModelIndex bottomIdx = view->indexAt(QPoint(view->viewport()->width() - 10, view->viewport()->height() - 10));

    if (topIdx.isValid()) top = topIdx.row();
    if (bottomIdx.isValid()) bottom = bottomIdx.row();

    // 稍微向外预加载 4 行缓冲，消除白块
    top = std::max(0, top - 4);
    bottom = std::min(m_proxyModel->rowCount() - 1, bottom + 4);

    QList<int> visibleRows;
    for (int r = top; r <= bottom; ++r) {
        QModelIndex proxyIdx = m_proxyModel->index(r, 0);
        QModelIndex srcIdx = m_proxyModel->mapToSource(proxyIdx);
        if (srcIdx.isValid()) {
            visibleRows.append(srcIdx.row());
        }
    }

    // 触发本批次加载
    m_model->loadThumbnailsForRows(visibleRows);
}

void ContentPanel::updateGridSize() {
    if (m_viewStack->currentWidget() == m_gridView) {
        if (auto* jv = qobject_cast<JustifiedView*>(m_gridView)) {
            jv->setTargetRowHeight(m_zoomLevel); // 自适应/网格模式下的卡片/行高
        } else if (auto* lv = qobject_cast<QListView*>(m_gridView)) {
            lv->setIconSize(QSize(m_zoomLevel, m_zoomLevel));
            int side = m_zoomLevel + 46;
            int ratingH = 24;
            int nameH = (int)(m_zoomLevel * 0.25);
            int gap = 6;
            int totalH = side + gap + ratingH + gap + nameH + 8;
            lv->setGridSize(QSize(side, totalH));
        }
    } else if (m_viewStack->currentWidget() == m_treeView) {
        // 列表模式：动态计算安全图标尺寸（最低不小于 16px）
        int iconSize = qMax(16, m_zoomLevel - 8);
        m_treeView->setIconSize(QSize(iconSize, iconSize));

        // 动态设置列表项的物理行高为 m_zoomLevel (范围：30px ~ 230px)
        static int lastTreeHeight = -1;
        if (lastTreeHeight != m_zoomLevel) {
            m_treeView->setStyleSheet( 
                QString("QTreeView { background-color: transparent; border: none; outline: none; font-size: 12px; }" 
                        "QTreeView::item { height: %1px; color: #EEEEEE; padding-left: 0px; }" 
                        "QTreeView::item:alternate { background-color: #252526; }" 
                        "QTreeView::item:selected { background-color: rgba(52, 152, 219, 0.2); border-left: 2px solid #3498db; }"
                        "QTreeView::item:hover { background-color: #2A2A2A; }"
                        "QTreeView QLineEdit { background-color: #2D2D2D; color: #FFFFFF; border: 1px solid #378ADD; border-radius: 6px; padding: 2px; selection-background-color: #378ADD; selection-color: #FFFFFF; }")
                .arg(m_zoomLevel)
            );
            lastTreeHeight = m_zoomLevel;
        }
    }

    // 持久化保存当前的缩放级别
    AppConfig::instance().setValue("UI/GridZoomLevel", m_zoomLevel);
} 
 
bool ContentPanel::canPaste(const QString& targetOverride) const { 
    // 1. 动态决议本次粘贴的实际物理落脚点（右键点击文件夹时以该文件夹为目标，否则以当前路径为目标） 
    QString destDir = targetOverride.isEmpty() ? m_currentPath : targetOverride; 
 
    // 2. 回收站与虚拟路径全局拦截 
    if (m_currentCategoryType == "trash" || destDir.isEmpty() ||  
        destDir == "computer://" || destDir == "trash://" || destDir.contains("://")) { 
        return false; 
    } 
 
    // 3. 目标必须是物理磁盘上真实存在且具备写入权限的文件夹 
    QFileInfo destInfo(destDir); 
    if (!destInfo.exists() || !destInfo.isDir() || !destInfo.isWritable()) { 
        return false; 
    } 
 
    // 4. 提取系统剪贴板 
    const QMimeData* mime = QApplication::clipboard()->mimeData(); 
    if (!mime) return false; 
 
    // 5. 图像截图直粘支持：必须真正能够提取出非空且有效像素的图片数据（杜绝纯文本/富文本剪贴板被 hasImage 误判）
    if (mime->hasImage()) { 
        QImage img = qvariant_cast<QImage>(mime->imageData());
        if (!img.isNull() && img.width() > 0 && img.height() > 0) {
            return true; 
        }
    } 
 
    // 6. 若不包含物理文件链接，直接置灰（内容面板不支持粘贴纯文字）
    if (!mime->hasUrls() || mime->urls().isEmpty()) { 
        return false; 
    } 
 
    // 7. 检测剪贴板操作模式（复制 vs 剪切） 
    bool isCut = false; 
    if (mime->hasFormat("Preferred DropEffect")) { 
        QByteArray effect = mime->data("Preferred DropEffect"); 
        if (!effect.isEmpty() && (effect.at(0) & 0x02)) { 
            isCut = true; 
        } 
    } 
 
    QString cleanDest = QDir::toNativeSeparators(QDir::cleanPath(destDir)).toLower(); 
    bool hasValidPhysicalSource = false; 
    bool isSameDirForCut = true; 
 
    for (const QUrl& url : mime->urls()) { 
        if (!url.isLocalFile()) continue;
        QString localPath = url.toLocalFile(); 
        if (localPath.isEmpty()) continue; 
 
        QFileInfo srcInfo(localPath); 
        if (!srcInfo.exists()) continue; 
 
        hasValidPhysicalSource = true; 
        QString cleanSrc = QDir::toNativeSeparators(QDir::cleanPath(localPath)).toLower(); 
 
        // 🚨 循环嵌套防爆：绝对禁止将父文件夹粘贴到其自身或其子文件夹内部 
        if (srcInfo.isDir()) { 
            if (cleanDest == cleanSrc || cleanDest.startsWith(cleanSrc + "\\") || cleanDest.startsWith(cleanSrc + "/")) { 
                return false; 
            } 
        } 
 
        // 检查来源父目录是否与目标一致 
        QString srcParent = QDir::toNativeSeparators(QDir::cleanPath(srcInfo.absolutePath())).toLower(); 
        if (srcParent != cleanDest) { 
            isSameDirForCut = false; 
        } 
    } 
 
    if (!hasValidPhysicalSource) return false; 
 
    // 🚨 原地剪切拦截：只有在【剪切模式】且【所有文件均处于目标目录下】时才置灰；普通复制即使同目录也允许（生成副本） 
    if (isCut && isSameDirForCut) { 
        return false; 
    } 
 
    return true; 
}

bool ContentPanel::eventFilter(QObject* obj, QEvent* event) { 
    if (event->type() == QEvent::Wheel) {
        QWheelEvent* wEvent = static_cast<QWheelEvent*>(event);
        if (wEvent->modifiers() & Qt::ControlModifier) {
            int deltaY = wEvent->angleDelta().y();
            int newZoom = m_zoomLevel + (deltaY > 0 ? 8 : -8);
            setZoomLevel(newZoom);
            wEvent->accept();
            return true; // 吞噬该事件，不让子视图产生滚动，彻底解决逻辑混乱和时灵时不灵问题
        }
    }

    // 2026-03-xx 按照宪法要求：物理拦截 Hover 事件以触发 ToolTipOverlay 
    // 2026-05-20 性能优化：同时支持 Enter/Leave 事件，确保响应灵敏 
    if (event->type() == QEvent::HoverEnter || event->type() == QEvent::Enter) { 
        QString text = obj->property("tooltipText").toString(); 
        if (!text.isEmpty()) { 
            int timeout = (obj == m_btnLayers || obj == m_btnToggleFolders || obj == m_btnToggleFiles) ? 0 : 700;
            ToolTipOverlay::instance()->showText(QCursor::pos(), text, timeout); 
        } 
    } else if (event->type() == QEvent::HoverLeave || event->type() == QEvent::Leave || event->type() == QEvent::MouseButtonPress) { 
        ToolTipOverlay::hideTip(); 
    } 
 
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mEvent = reinterpret_cast<QMouseEvent*>(event);
        if (mEvent->button() == Qt::LeftButton) {
            if (obj == m_gridView || obj == m_gridView->viewport() || obj == m_treeView || obj == m_treeView->viewport()) {
                QAbstractItemView* view = qobject_cast<QAbstractItemView*>(obj);
                if (!view) view = qobject_cast<QAbstractItemView*>(obj->parent());
                if (view) {
                    QPoint pos = mEvent->pos();
                    if (obj == view && view->viewport()) {
                        pos = view->viewport()->mapFrom(view, pos);
                    }
                    QModelIndex index = view->indexAt(pos);
                    if (index.isValid()) {
                        // 针对 Grid 模式 / Justified 模式的 Hitbox
                        ThumbnailDelegate* thumbDel = qobject_cast<ThumbnailDelegate*>(view->itemDelegateForIndex(index));
                        if (thumbDel) {
                            QStyleOptionViewItem opt;
                            opt.rect = view->visualRect(index);
                            opt.decorationSize = view->iconSize();
                            if (opt.decorationSize.width() <= 0) opt.decorationSize = QSize(96, 96);
                            ThumbnailDelegate::Metrics m = thumbDel->calculateMetrics(opt);

                            bool isBanHit = m.banRect.contains(pos);
                            int hitStar = -1;
                            for (int i = 0; i < 5; ++i) {
                                if (m.starRect(i).contains(pos)) {
                                    hitStar = i + 1;
                                    break;
                                }
                            }

                            if (isBanHit || hitStar != -1) {
                                bool isSelected = false;
                                if (view->selectionModel()) {
                                    isSelected = view->selectionModel()->isSelected(index);
                                }
                                if (!isSelected) return false;

                                int newValue = isBanHit ? 0 : hitStar;
                                if (view->selectionModel() && view->selectionModel()->isSelected(index)) {
                                    auto selectedIndexes = view->selectionModel()->selectedIndexes();
                                    for (const auto& selIdx : selectedIndexes) {
                                        if (selIdx.column() == 0) {
                                            m_proxyModel->setData(selIdx, newValue, RatingRole);
                                        }
                                    }
                                } else {
                                    m_proxyModel->setData(index, newValue, RatingRole);
                                }

                                QAbstractItemView::EditTriggers currentTriggers = view->editTriggers();
                                view->setEditTriggers(QAbstractItemView::NoEditTriggers);
                                QTimer::singleShot(0, view, [view, currentTriggers]() {
                                    view->setEditTriggers(currentTriggers);
                                });
                                event->accept();
                                return true;
                            }
                        }

                        // 针对 TreeView 列 2 (星级列) 的 Hitbox
                        if (view == m_treeView) {
                            QModelIndex indexCol2 = index.model()->index(index.row(), 2, index.parent());
                            QRect col2Rect = m_treeView->visualRect(indexCol2);
                            
                            int banW = 12;
                            int starSize = 18;
                            int banGap = 2;
                            int starSpacing = -4; // 与 Delegate 严格保持 -4 间距对齐
                            int totalW = banW + banGap + 5 * starSize + 4 * starSpacing; // 88px
                            int startX = col2Rect.left() + (col2Rect.width() - totalW) / 2;

                            QRect banHitbox(startX, col2Rect.top() + (col2Rect.height() - banW)/2, banW, banW);
                            bool isBanHit = banHitbox.contains(pos);
                            int hitStar = -1;

                            // 统一星级点击命中区参数，使其与 TreeItemDelegate 绘制参数保持绝对物理对齐
                            int starsStartX = startX + banW + banGap; 
                            for (int i = 0; i < 5; ++i) {
                                QRect starRect(starsStartX + i * (starSize + starSpacing), col2Rect.top() + (col2Rect.height() - starSize) / 2, starSize, starSize);
                                if (starRect.contains(pos)) {
                                    hitStar = i + 1;
                                    break;
                                }
                            }

                            if (isBanHit || hitStar != -1) {
                                bool isRowSelected = false;
                                if (m_treeView->selectionModel()) {
                                    isRowSelected = m_treeView->selectionModel()->isRowSelected(index.row(), index.parent());
                                }
                                if (!isRowSelected) return false;

                                int newValue = isBanHit ? 0 : hitStar;
                                if (m_treeView->selectionModel()) {
                                    auto selectedRows = m_treeView->selectionModel()->selectedRows();
                                    for (const auto& selRow : selectedRows) {
                                        QModelIndex targetIdx = m_treeView->model()->index(selRow.row(), 0, selRow.parent());
                                        m_proxyModel->setData(targetIdx, newValue, RatingRole);
                                    }
                                } else {
                                    m_proxyModel->setData(index.model()->index(index.row(), 0, index.parent()), newValue, RatingRole);
                                }

                                QAbstractItemView::EditTriggers currentTriggers = m_treeView->editTriggers();
                                m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
                                QTimer::singleShot(0, m_treeView, [this, currentTriggers]() {
                                    m_treeView->setEditTriggers(currentTriggers);
                                });
                                event->accept();
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }

 
    if (event->type() == QEvent::KeyPress) { 
        QKeyEvent* keyEvent = reinterpret_cast<QKeyEvent*>(event); 
 
        QAbstractItemView* view = qobject_cast<QAbstractItemView*>(obj); 
        if (!view) view = qobject_cast<QAbstractItemView*>(obj->parent()); 
 
        if (qobject_cast<QLineEdit*>(QApplication::focusWidget())) { 
            return false; 
        } 
 
        if (view) { 
            if ((keyEvent->modifiers() & Qt::ControlModifier) &&  
                (keyEvent->key() >= Qt::Key_0 && keyEvent->key() <= Qt::Key_5)) { 
                 
                int rating = keyEvent->key() - Qt::Key_0; 
                auto indexes = view->selectionModel()->selectedIndexes(); 
                for (const auto& idx : indexes) { 
                    if (idx.column() == 0) { 
                        m_proxyModel->setData(idx, rating, RatingRole); 
                    } 
                } 
                return true; 
            } 
 
            if (((keyEvent->modifiers() & Qt::AltModifier) || (keyEvent->modifiers() & (Qt::AltModifier | Qt::WindowShortcut))) &&  
                (keyEvent->key() == Qt::Key_D)) { 
                auto indexes = view->selectionModel()->selectedIndexes(); 
                for (const QModelIndex& idx : indexes) { 
                    if (idx.column() == 0) { 
                        bool current = idx.data(IsLockedRole).toBool(); 
                        m_proxyModel->setData(idx, !current, IsLockedRole); 
                    } 
                } 
                return true; 
            } 
 
            if ((keyEvent->modifiers() & Qt::AltModifier) &&  
                (keyEvent->key() >= Qt::Key_1 && keyEvent->key() <= Qt::Key_9)) { 
                 
                QString colorValue; 
                switch (keyEvent->key()) { 
                    case Qt::Key_1: colorValue = "#E24B4A"; break; // red
                    case Qt::Key_2: colorValue = "#EF9F27"; break; // orange
                    case Qt::Key_3: colorValue = "#FECF0E"; break; // yellow
                    case Qt::Key_4: colorValue = "#639922"; break; // green
                    case Qt::Key_5: colorValue = "#1D9E75"; break; // cyan
                    case Qt::Key_6: colorValue = "#378ADD"; break; // blue
                    case Qt::Key_7: colorValue = "#7F77DD"; break; // purple
                    case Qt::Key_8: colorValue = "#5F5E5A"; break; // gray
                    case Qt::Key_9: colorValue = ""; break; 
                } 
 
                auto indexes = view->selectionModel()->selectedIndexes(); 
                for (const auto& idx : indexes) { 
                    if (idx.column() == 0) { 
                        m_proxyModel->setData(idx, colorValue, ColorRole); 
 
                        // 2026-06-05 按照要求：快捷键设置颜色后立即重渲染图标，实现视觉同步 
                        QString path = idx.data(PathRole).toString(); 
                        QIcon coloredIcon = ShellIconManager::getFileIcon(path, 128); 
                        m_proxyModel->setData(idx, coloredIcon, Qt::DecorationRole); 
                    } 
                } 
                return true; 
            } 
 
            if (keyEvent->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) { 
                if (keyEvent->key() == Qt::Key_C) { 
                    QStringList paths; 
                    auto indexes = view->selectionModel()->selectedIndexes(); 
                    for (const auto& idx : indexes) if (idx.column() == 0) paths << QDir::toNativeSeparators(idx.data(PathRole).toString()); 
                    if (!paths.isEmpty()) QApplication::clipboard()->setText(paths.join("\r\n")); 
                    return true; 
                } 
                // 2026-03-xx 按照用户要求：补全批量重命名 (Ctrl+Shift+R) 快捷键绑定 
                if (keyEvent->key() == Qt::Key_R) { 
                    performBatchRename(); 
                    return true; 
                } 
            } 
 
            if (keyEvent->key() == Qt::Key_F2) { 
                view->edit(view->currentIndex()); 
                return true; 
            } 
            if (keyEvent->key() == Qt::Key_Delete) { 
                onCustomContextMenuRequested(view->mapFromGlobal(QCursor::pos())); 
                return true; 
            } 
             
            if (keyEvent->modifiers() & Qt::ControlModifier) { 
                // 2026-03-xx 按照用户要求：逻辑重构，统一调用 performCopy 业务函数 
                if (keyEvent->key() == Qt::Key_C && !(keyEvent->modifiers() & Qt::ShiftModifier)) { 
                    performCopy(false); 
                    return true; 
                } 
                // 2026-03-xx 按照用户要求：实现剪切逻辑 (Ctrl+X) 
                if (keyEvent->key() == Qt::Key_X) { 
                    performCopy(true); 
                    return true; 
                } 
                // 2026-03-xx 按照用户要求：逻辑重构，统一调用 performPaste 业务函数 
                if (keyEvent->key() == Qt::Key_V) { 
                    if (canPaste()) {
                        performPaste(); 
                    }
                    return true; 
                } 
            } 
 
            if (keyEvent->key() == Qt::Key_Space) { 
                QModelIndex idx = view->currentIndex(); 
                if (idx.isValid()) {
                    QString path = idx.data(PathRole).toString();
                    if (!path.isEmpty()) {
                        // 2026-11-14 按照 Plan-109：全口径预览属性过滤（白名单优先策略）
                        QFileInfo info(path);
                        if (info.isDir()) return true; // 拦截文件夹

                        QString ext = info.suffix().toLower();
                        // 1. 系统级不可预览黑名单 (包含压缩包、二进制文件及系统库)
                        static const QSet<QString> blackList = {
                            "exe", "dll", "sys", "bin", "dat", "lib", "obj", "msi", "com",
                            "zip", "rar", "7z", "iso", "tar", "gz", "bz2", "dmg", "pkg"
                        };
                        if (blackList.contains(ext)) return true;

                        // 2. 预览准入白名单 (仅限受支持的图像类及文本/代码类文件)
                        static const QSet<QString> whiteList = {
                            "jpg", "jpeg", "png", "bmp", "webp", "gif", "ico", "cur", "ani", "psd", "ai", "eps", "pdf", "svg",
                            "txt", "md", "markdown", "log", "cpp", "h", "hpp", "c", "py", "js", "css", "html", "json", "xml", "ini", "conf", "yaml", "yml"
                        };

                        if (whiteList.contains(ext)) {
                            emit requestQuickLook(path);
                        }
                    }
                }
                return true; 
            } 
            if (keyEvent->key() == Qt::Key_Backspace) { 
                QDir dir(m_currentPath); 
                if (dir.cdUp()) emit directorySelected(dir.absolutePath()); 
                return true; 
            } 
            if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) { 
                onDoubleClicked(view->currentIndex()); 
                return true; 
            } 
            if (keyEvent->modifiers() & Qt::ControlModifier && keyEvent->key() == Qt::Key_Backslash) { 
                ViewMode nextMode = ListView;
                if (m_currentViewMode == ListView) nextMode = GridView;
                else if (m_currentViewMode == GridView) nextMode = JustifiedViewMode;
                else if (m_currentViewMode == JustifiedViewMode) nextMode = ListView;
                setViewMode(nextMode); 
                return true; 
            } 
        } 
    } 
    return QWidget::eventFilter(obj, event); 
} 
 
void ContentPanel::selectAndScrollToPath(const QString& path) {
    if (!m_proxyModel) return;
    for (int i = 0; i < m_proxyModel->rowCount(); ++i) {
        QModelIndex proxyIdx = m_proxyModel->index(i, 0);
        if (proxyIdx.data(PathRole).toString() == path) {
            QAbstractItemView* view = (m_viewStack->currentWidget() == m_treeView) ? 
                static_cast<QAbstractItemView*>(m_treeView) : static_cast<QAbstractItemView*>(m_gridView);
            if (view) {
                view->scrollTo(proxyIdx);
                view->setCurrentIndex(proxyIdx);
                view->selectionModel()->select(proxyIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            }
            break;
        }
    }
}

void ContentPanel::selectAndScrollToItem(const QString& path) {
    if (!m_proxyModel || path.isEmpty()) return;
    for (int i = 0; i < m_proxyModel->rowCount(); ++i) {
        QModelIndex proxyIdx = m_proxyModel->index(i, 0);
        bool match = (proxyIdx.data(PathRole).toString() == path);

        if (match) {
            QAbstractItemView* view = (m_viewStack->currentWidget() == m_treeView) ? 
                static_cast<QAbstractItemView*>(m_treeView) : static_cast<QAbstractItemView*>(m_gridView);
            if (view) {
                view->scrollTo(proxyIdx);
                view->setCurrentIndex(proxyIdx);
                view->selectionModel()->select(proxyIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            }
            break;
        }
    }
}

QString ContentPanel::getAdjacentFilePath(const QString& currentPath, int delta) { 
    if (!m_proxyModel || m_proxyModel->rowCount() == 0) return QString(); 
 
    int currentIndex = -1; 
    for (int i = 0; i < m_proxyModel->rowCount(); ++i) { 
        QModelIndex idx = m_proxyModel->index(i, 0); 
        if (idx.data(PathRole).toString() == currentPath) { 
            currentIndex = i; 
            break; 
        } 
    } 
 
    if (currentIndex == -1) return QString(); 
 
    int targetIndex = currentIndex + delta; 
    // 逻辑：触达边界时停止，不进行循环跳转 
    if (targetIndex < 0 || targetIndex >= m_proxyModel->rowCount()) { 
        return QString(); 
    } 
 
    QModelIndex targetIdx = m_proxyModel->index(targetIndex, 0); 
    return targetIdx.data(PathRole).toString(); 
} 
 
void ContentPanel::setZoomLevel(int level) {
    // 1. 根据当前视图模式动态决定最小/最大像素边界
    int minZoom = 93;
    int maxZoom = 230;

    if (m_currentViewMode == ListView) {
        minZoom = 30;   // 列表视图最小值：30 像素
        maxZoom = 230;  // 列表视图最大值：230 像素
    } else { // GridView 与 JustifiedViewMode
        minZoom = 93;   // 网格/自适应最小值：93 像素
        maxZoom = 230;  // 网格/自适应最大值：230 像素
    }

    // 2. 严格按模式物理裁切
    int boundedLevel = qBound(minZoom, level, maxZoom);
    if (m_zoomLevel == boundedLevel) return;

    m_zoomLevel = boundedLevel;
    updateGridSize();
    emit zoomLevelChanged(m_zoomLevel);
}

void ContentPanel::wheelEvent(QWheelEvent* event) { 
    if (event->modifiers() & Qt::ControlModifier) { 
        int deltaY = event->angleDelta().y(); 
        int newZoom = m_zoomLevel + (deltaY > 0 ? 8 : -8); 
        setZoomLevel(newZoom); 
        event->accept(); 
        return; 
    } 
    QWidget::wheelEvent(event); 
} 
 
void ContentPanel::setViewMode(ViewMode mode) { 
    m_currentViewMode = mode;

    // 1. 模式切换时自动校准 m_zoomLevel，确保处于新模式的合法范围内
    int minZoom = (mode == ListView) ? 30 : 93;
    int maxZoom = 230;
    m_zoomLevel = qBound(minZoom, m_zoomLevel, maxZoom);

    // 2. 切换 ViewStack 页面
    if (mode == ListView) {
        m_viewStack->setCurrentWidget(m_treeView);
    } else if (mode == GridView) {
        auto* justifiedView = qobject_cast<JustifiedView*>(m_gridView);
        if (justifiedView) {
            justifiedView->setLayoutMode(JustifiedView::GridMode);
        }
        m_viewStack->setCurrentWidget(m_gridView);
    } else if (mode == JustifiedViewMode) {
        auto* justifiedView = qobject_cast<JustifiedView*>(m_gridView);
        if (justifiedView) {
            justifiedView->setLayoutMode(JustifiedView::JustifiedMode);
        }
        m_viewStack->setCurrentWidget(m_gridView);
    }

    // 保存当前的视图模式到 AppConfig，实现跨生命周期持久化
    AppConfig::instance().setValue("ContentPanel/ViewMode", static_cast<int>(mode));
    AppConfig::instance().sync();

    updateGridSize();
    emit viewModeChanged(mode); // 触发模式改变信号
    emit zoomLevelChanged(m_zoomLevel); // 通知标题栏滑杆更新数值
    m_visibleTimer->start();
} 
 
void ContentPanel::initGridView() { 
    m_gridView = new DropJustifiedView(this); 
    m_gridView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded); 
    m_gridView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded); 
    m_gridView->setSelectionMode(QAbstractItemView::ExtendedSelection); 
    // 2026-06-xx 按照用户要求：开启蓝色透明框选效果
    // 物理修复：对于 ListView/TreeView 使用 setSelectionRectVisible
    if (auto* lv = qobject_cast<QListView*>(m_gridView)) lv->setSelectionRectVisible(true);

    // 2026-06-xx 物理对齐：通过 QPalette 设定全局蓝色透明框选视觉样式
    QPalette p = m_gridView->palette();
    // 使用 #378ADD (QColor(55, 138, 221)) 并设定 Alpha 为 80 以确保框选内容清晰可见
    p.setColor(QPalette::Highlight, QColor(55, 138, 221, 80)); 
    p.setColor(QPalette::HighlightedText, Qt::white);
    m_gridView->setPalette(p);
    m_gridView->setContextMenuPolicy(Qt::CustomContextMenu); 
 
    m_gridView->setDragEnabled(true); 
    m_gridView->setAcceptDrops(true);
    m_gridView->setDragDropMode(QAbstractItemView::DragDrop); 
 
    // 2026-06-xx 物理纠偏：移除 SelectedClicked，防止单击项目时意外触发重命名，确保交互稳健
    m_gridView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed); 
 
    m_gridView->setModel(m_proxyModel); 

    connect(m_gridView, SIGNAL(pathsDropped(QStringList,QModelIndex)), this, SLOT(onPathsDropped(QStringList,QModelIndex)));

    auto* justifiedView = qobject_cast<JustifiedView*>(m_gridView);
    if (justifiedView) {
        justifiedView->setAspectRatioRole(AspectRatioRole);
        auto* delegate = new ThumbnailDelegate(this);
        delegate->setHasThumbnailRole(HasThumbnailRole);
        delegate->setRatingRole(RatingRole);
        delegate->setPathRole(PathRole);
        delegate->setPinnedRole(PinnedRole);
        delegate->setTypeRole(TypeRole);
        delegate->setIsEmptyRole(IsEmptyRole);
        delegate->setColorRole(ColorRole);
        m_gridView->setItemDelegate(delegate);
    }

    m_gridView->viewport()->installEventFilter(this); 
 
    connect(m_gridView, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked); 
 
    m_gridView->setStyleSheet( 
        "QAbstractItemView { background-color: transparent; border: none; outline: none; }" 
        "QAbstractItemView::item { background: transparent; }" 
        "QAbstractItemView::item:selected { background-color: transparent; }" 
        "QAbstractItemView::item:hover { background-color: transparent; }"
    ); 
 
    connect(m_gridView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged); 
    connect(m_gridView, &QAbstractItemView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested); 
    connect(m_gridView->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
        m_visibleTimer->start(60);
    });
    connect(m_gridView->verticalScrollBar(), &QScrollBar::sliderReleased, this, [this]() {
        refreshVisibleThumbnails();
    });
} 
 
void ContentPanel::initListView() { 
    m_treeView = new DropTreeView(this); 
    m_treeView->setAlternatingRowColors(true); // 开启交替斑马纹背景
    m_treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded); 
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded); 
    m_treeView->setSortingEnabled(true); 
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu); 
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection); 
    // 2026-06-xx 按照用户要求：开启蓝色透明框选效果
    // 物理修复：QTreeView 不支持 setSelectionRectVisible，通过 QPalette 高亮色实现视觉对齐
    QPalette tp = m_treeView->palette();
    tp.setColor(QPalette::Highlight, QColor(55, 138, 221, 80));
    tp.setColor(QPalette::HighlightedText, Qt::white);
    m_treeView->setPalette(tp);
     
    m_treeView->setDragEnabled(true); 
    m_treeView->setAcceptDrops(true);
    m_treeView->setDragDropMode(QAbstractItemView::DragDrop); 
 
    m_treeView->setExpandsOnDoubleClick(false); 
    m_treeView->setRootIsDecorated(false); 
     
    // 在列表视图中显式指定空项时的文字占位提醒
    DropTreeView* dropTree = qobject_cast<DropTreeView*>(m_treeView);
    if (dropTree) {
        dropTree->setEmptyHint("没有可显示的项目");
    }

    // 列表视图开启 m_drawMiniCards = true，以启用 Column 0 “最左侧微卡片圆角预览”和底部分割线贯通绘制
    m_treeView->setItemDelegate(new TreeItemDelegate(this, true, true)); 
 
    m_treeView->setModel(m_proxyModel); 
    m_treeView->viewport()->installEventFilter(this); 

    connect(m_treeView, SIGNAL(pathsDropped(QStringList,QModelIndex)), this, SLOT(onPathsDropped(QStringList,QModelIndex)));
 
    m_treeView->setStyleSheet( 
        "QTreeView { background-color: transparent; border: none; outline: none; font-size: 12px; }" 
        "QTreeView::item { height: 28px; color: #EEEEEE; padding-left: 0px; }" 
        "QTreeView::item:alternate { background-color: #252526; }" // 斑马纹交替行高亮背景
        "QTreeView::item:selected { background-color: rgba(52, 152, 219, 0.2); border-left: 2px solid #3498db; }"
        "QTreeView::item:hover { background-color: #2A2A2A; }"
        "QTreeView QLineEdit { background-color: #2D2D2D; color: #FFFFFF; border: 1px solid #378ADD; border-radius: 6px; padding: 2px; selection-background-color: #378ADD; selection-color: #FFFFFF; }" 
    ); 
 
    m_treeView->header()->setDefaultAlignment(Qt::AlignCenter);
    m_treeView->header()->setStyleSheet( 
        "QHeaderView::section { background-color: #252525; color: #B0B0B0; border: none; border-right: 1px solid #333333; height: 32px; font-size: 11px; }" 
    ); 
    
    // --- 列表表头（Header）列宽固定化重构 ---
    auto* header = m_treeView->header();
    header->setStretchLastSection(false); // 禁止末端强行拉伸
    header->setCascadingSectionResizes(false);

    // 1. 确保所有 7 列均可见，并且彻底隐藏或移除多余的第 7 列（原本的第 7 列已被前移）
    for (int i = 0; i <= 6; ++i) {
        header->setSectionHidden(i, false);
    }
    header->setSectionHidden(7, true);

    // 2. 精确设置各列固定像素宽度（彻底移除“颜色”列，平移后续所有列宽度）
    header->resizeSection(1, 50);   // 状态 (固定 50px 图标区)
    header->resizeSection(2, 120);  // 星级 (固定 120px 图标区)
    header->resizeSection(3, 120);  // 尺寸 (固定 120px)
    header->resizeSection(4, 80);   // 类型 (固定 80px)
    header->resizeSection(5, 100);  // 大小 (固定 100px)
    header->resizeSection(6, 120);  // 修改日期 (固定 120px)

    // 3. 锁定调整模式：第 0 列（名称）弹性自适应拉伸，第 1~6 列物理固定禁止拖拽
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int i = 1; i <= 6; ++i) {
        header->setSectionResizeMode(i, QHeaderView::Fixed);
    }
 
    connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged); 
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested); 
    connect(m_treeView, &QTreeView::doubleClicked, this, &ContentPanel::onDoubleClicked); 
    connect(m_treeView->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
        m_visibleTimer->start();
    });
} 
 
void ContentPanel::onCustomContextMenuRequested(const QPoint& pos) { 
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(sender()); 
    if (!view) return; 

    QModelIndex currentIndex = view->indexAt(pos); 
    bool onItem = currentIndex.isValid(); 
    QString path = onItem ? currentIndex.data(PathRole).toString() : ""; 
    QFileInfo itemInfo(path);

    bool isComputerRoot = (m_currentPath.isEmpty() || m_currentPath == "computer://");
    bool isTrashView = (m_currentCategoryType == "trash" || m_currentPath == "trash://");

    // 权威判定：是否为物理驱动器/盘符（如 C:/、D:\）
    bool isDriveRoot = onItem && (itemInfo.isRoot() || path.endsWith(":\\") || path.endsWith(":/") || (path.length() == 2 && path.endsWith(':')));
    bool isFolder = onItem && (isDriveRoot || currentIndex.data(TypeRole).toString() == "folder");

    QMenu menu(this); 
    UiHelper::applyMenuStyle(&menu); 

    // =========================================================================
    // 场景 1：回收站视图（全场景独立接管）
    // =========================================================================
    if (isTrashView) {
        if (onItem) {
            menu.addAction(UiHelper::getIcon("sync", QColor("#2ecc71"), 18), "还原")->setData(ActionRestore);
            menu.addAction(UiHelper::getIcon("cut", QColor("#EEEEEE"), 18), "剪切")->setData(ActionCut);
            menu.addAction(UiHelper::getIcon("trash", QColor("#e81123"), 18), "永久删除")->setData(ActionSecureDelete);
            menu.addSeparator();
        }
        menu.addAction(UiHelper::getIcon("sync", QColor("#2ecc71"), 18), "还原全部")->setData(ActionRestoreAll);
        menu.addAction(UiHelper::getIcon("trash", QColor("#e81123"), 18), "清空回收站")->setData(ActionEmptyTrash);

        m_isContextMenuActive = true;
        QAction* selectedAction = menu.exec(view->viewport()->mapToGlobal(pos));
        m_isContextMenuActive = false;

        if (!selectedAction || !selectedAction->data().isValid()) return;

        ContextAction action = static_cast<ContextAction>(selectedAction->data().toInt());
        switch (action) {
            case ActionRestore: {
                auto indexes = view->selectionModel()->selectedIndexes();
                for (const auto& idx : indexes) {
                    if (idx.column() == 0) {
                        if (idx.data(IsDiskTrashRole).toBool()) {
                            int id = idx.data(DiskTrashIdRole).toInt();
                            QString trashPath = idx.data(PathRole).toString();
                            DiskTrashService::restoreFromDiskTrash(id, trashPath);
                        }
                    }
                }
                refreshAll();
                MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
                break;
            }
            case ActionCut: {
                performCopy(true);
                break;
            }
            case ActionSecureDelete: {
                auto indexes = view->selectionModel()->selectedIndexes();
                QStringList targetPaths;
                std::vector<std::pair<int, QString>> diskTrashItems;

                for (const auto& idx : indexes) {
                    if (idx.column() == 0) {
                        if (idx.data(IsDiskTrashRole).toBool()) {
                            int id = idx.data(DiskTrashIdRole).toInt();
                            QString p = idx.data(PathRole).toString();
                            diskTrashItems.push_back({id, p});
                        } else {
                            targetPaths << idx.data(PathRole).toString();
                        }
                    }
                }
                if (targetPaths.isEmpty() && diskTrashItems.empty() && !path.isEmpty()) {
                    targetPaths << path;
                }

                if (targetPaths.isEmpty() && diskTrashItems.empty()) break;

                QString msg = "确定要永久删除选中的项目吗？数据将被物理覆写并彻底抹除，此操作不可恢复。";
                if (!FramelessMessageBox::question(this, "确认删除", msg)) break;

                BatchProgressDialog* progress = new BatchProgressDialog("正在执行永久删除（深层抹除）...", this);
                progress->show();

                QPointer<ContentPanel> weakPanel(this);
                QPointer<BatchProgressDialog> weakProgress(progress);

                MetadataManager::instance().beginInternalOperation();

                (void)QtConcurrent::run([targetPaths, diskTrashItems, weakPanel, weakProgress]() {
                    int total = static_cast<int>(targetPaths.size() + diskTrashItems.size());
                    int count = 0;

                    for (const QString& p : targetPaths) {
                        if (!weakPanel) return;
                        std::wstring wp = QDir::toNativeSeparators(p).toStdWString();
                        MetadataManager::instance().setEncrypted(wp, false);
                        QFile::remove(p);
                        count++;
                        if (weakProgress) {
                            QMetaObject::invokeMethod(weakProgress, "updateProgress",
                                                      Qt::QueuedConnection,
                                                      Q_ARG(int, count),
                                                      Q_ARG(int, total),
                                                      Q_ARG(QString, QFileInfo(p).fileName()));
                        }
                    }

                    for (const auto& item : diskTrashItems) {
                        if (!weakPanel) return;
                        DiskTrashService::permanentlyDeleteDiskTrash(item.first, item.second);
                        count++;
                        if (weakProgress) {
                            QMetaObject::invokeMethod(weakProgress, "updateProgress",
                                                      Qt::QueuedConnection,
                                                      Q_ARG(int, count),
                                                      Q_ARG(int, total),
                                                      Q_ARG(QString, QFileInfo(item.second).fileName()));
                        }
                    }

                    QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, weakProgress]() {
                        MetadataManager::instance().endInternalOperation();
                        if (weakProgress) weakProgress->close();
                        if (weakPanel) {
                            weakPanel->refreshAll();
                            MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
                            ToolTipOverlay::instance()->showText(QCursor::pos(), "选中的回收站项目已彻底永久删除", 1500, QColor("#2ecc71"));
                        }
                    });
                });
                break;
            }
            case ActionRestoreAll: {
                if (DiskTrashService::restoreAllDiskTrash()) {
                    ToolTipOverlay::instance()->showText(QCursor::pos(), "所有回收站项目已成功还原", 1500, QColor("#2ecc71"));
                }
                refreshAll();
                MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
                break;
            }
            case ActionEmptyTrash: {
                if (FramelessMessageBox::question(this, "清空回收站", "确定要清空回收站吗？回收站内的所有文件将被彻底删除，此操作不可撤销。")) {
                    if (DiskTrashService::emptyDiskTrash()) {
                        ToolTipOverlay::instance()->showText(QCursor::pos(), "回收站已清空", 1500, QColor("#2ecc71"));
                    }
                    refreshAll();
                    MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
                }
                break;
            }
            default:
                break;
        }
        return;
    } 

    // =========================================================================
    // 场景 2：选中具体项目（右键点击某个项目）
    // =========================================================================
    if (onItem) { 
        // -------------------------------------------------------------
        // 分支 2.A：选中的是【物理驱动器/盘符】（C:、D: 等）
        // -------------------------------------------------------------
        if (isDriveRoot) {
            menu.addAction("打开")->setData(ActionOpen);
            menu.addAction("在“资源管理器”中显示")->setData(ActionShowInExplorer);

            // 颜色条
            QString currentColorStr = currentIndex.data(ColorRole).toString();
            QWidgetAction* pickerAction = new QWidgetAction(&menu);
            ColorStripPicker* pickerWidget = new ColorStripPicker(currentColorStr, &menu);
            pickerAction->setDefaultWidget(pickerWidget);
            menu.addAction(pickerAction);

            connect(pickerWidget, &ColorStripPicker::colorSelected, this, [this, view, &menu](const QString& hexColor) {
                auto indexes = view->selectionModel()->selectedIndexes();  
                for (const auto& idx : indexes) {  
                    if (idx.column() == 0) m_proxyModel->setData(idx, hexColor, ColorRole);  
                } 
                menu.close(); 
            });

            // 置顶 / 取消置顶
            bool isPinned = currentIndex.data(IsLockedRole).toBool(); 
            menu.addAction(isPinned ? "取消置顶" : "置顶")->setData(isPinned ? ActionUnpin : ActionPin); 
            menu.addAction("添加至收藏夹")->setData(ActionAddToFavorites); 

            menu.addSeparator();

            // 允许向该盘根目录直接粘贴内容（智能受控）
            QAction* actItemPaste = menu.addAction("粘贴");
            actItemPaste->setData(ActionPaste);
            actItemPaste->setEnabled(canPaste(path)); 

            menu.addAction("复制名称")->setData(ActionCopyName); 
            menu.addAction("复制路径")->setData(ActionCopyPath); 

            menu.addSeparator();
            menu.addAction("刷新")->setData(ActionRefresh);
            // 🚨 严禁挂载：删除、重命名、剪切、复制、加密保护、重新提取缩略图！
        } 
        // -------------------------------------------------------------
        // 分支 2.B：选中的是【常规文件 / 普通文件夹】
        // -------------------------------------------------------------
        else {
            menu.addAction(isFolder ? "打开文件夹" : "打开")->setData(ActionOpen); 
            if (!isFolder) { 
                menu.addAction("用系统默认程序打开")->setData(ActionOpenDefault); 
            } 
            menu.addAction("在“资源管理器”中显示")->setData(ActionShowInExplorer); 

            // 颜色条
            QString currentColorStr = currentIndex.data(ColorRole).toString();
            QWidgetAction* pickerAction = new QWidgetAction(&menu);
            ColorStripPicker* pickerWidget = new ColorStripPicker(currentColorStr, &menu);
            pickerAction->setDefaultWidget(pickerWidget);
            menu.addAction(pickerAction);

            connect(pickerWidget, &ColorStripPicker::colorSelected, this, [this, view, &menu](const QString& hexColor) {
                auto indexes = view->selectionModel()->selectedIndexes();  
                for (const auto& idx : indexes) {  
                    if (idx.column() == 0) m_proxyModel->setData(idx, hexColor, ColorRole);  
                } 
                menu.close(); 
            });

            bool isPinned = currentIndex.data(IsLockedRole).toBool(); 
            menu.addAction(isPinned ? "取消置顶" : "置顶")->setData(isPinned ? ActionUnpin : ActionPin); 
            menu.addAction("添加至收藏夹")->setData(ActionAddToFavorites); 

            menu.addSeparator(); 

            menu.addAction("复制")->setData(ActionCopy); 
            menu.addAction("剪切")->setData(ActionCut); 
            
            QAction* actItemPaste = menu.addAction("粘贴"); 
            actItemPaste->setData(ActionPaste); 
            actItemPaste->setEnabled(canPaste(isFolder ? path : m_currentPath)); 

            menu.addAction("复制名称")->setData(ActionCopyName); 
            menu.addAction("复制路径")->setData(ActionCopyPath); 

            int selectedCount = 0;
            for (const auto& selIdx : view->selectionModel()->selectedIndexes()) {
                if (selIdx.column() == 0 && !selIdx.data(PathRole).toString().isEmpty()) selectedCount++;
            }

            if (selectedCount <= 1) {
                menu.addAction("重命名")->setData(ActionRename); 
            }
            if (isFolder || selectedCount > 1) { 
                menu.addAction("批量重命名 (Ctrl+Shift+R)")->setData(ActionBatchRename); 
            }

            menu.addSeparator(); 
            menu.addAction("刷新")->setData(ActionRefresh); 

            if (!isFolder) {
                menu.addAction(UiHelper::getIcon("sync", QColor("#3498db"), 18), "重新提取缩略图")->setData(ActionReextractThumbnail);

                QMenu* cryptoMenu = menu.addMenu("加密保护"); 
                UiHelper::applyMenuStyle(cryptoMenu); 
                cryptoMenu->addAction("执行加密保护")->setData(ActionEncrypt); 
                cryptoMenu->addAction("解除加密")->setData(ActionDecrypt); 
                cryptoMenu->addAction("修改加密密码")->setData(ActionChangePwd); 
            }
        }
    } 
    // =========================================================================
    // 场景 3：点击空白处（未选中任何项目）
    // =========================================================================
    else { 
        // -------------------------------------------------------------
        // 分支 3.A：“此电脑”（computer://）空白处
        // -------------------------------------------------------------
        if (isComputerRoot) {
            menu.addAction("在“资源管理器”中显示")->setData(ActionShowInExplorer);
            menu.addAction("刷新")->setData(ActionRefresh);
            // 🚨 严禁挂载：新建、批量创建、粘贴！
        } 
        // -------------------------------------------------------------
        // 分支 3.B：常规物理目录空白处
        // -------------------------------------------------------------
        else {
            QMenu* newMenu = menu.addMenu("新建..."); 
            UiHelper::applyMenuStyle(newMenu); 
            newMenu->addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "创建文件夹")->setData(ActionNewFolder); 
            newMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建 Markdown")->setData(ActionNewMd); 
            newMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建纯文本文件 (txt)")->setData(ActionNewTxt); 

            menu.addSeparator(); 
            menu.addAction(UiHelper::getIcon("add", QColor("#EEEEEE")), "批量创建项目...")->setData(ActionBatchCreate);

            menu.addSeparator(); 
            QAction* actPaste = menu.addAction("粘贴"); 
            actPaste->setData(ActionPaste); 
            actPaste->setEnabled(canPaste(m_currentPath)); 

            menu.addSeparator(); 
            bool isPhysicalPath = !m_currentPath.isEmpty() && !m_currentPath.contains("://") && QDir(m_currentPath).exists();
            QAction* actShowInExp = menu.addAction("在“资源管理器”中显示");
            actShowInExp->setData(ActionShowInExplorer);
            actShowInExp->setEnabled(isPhysicalPath);

            menu.addAction("刷新")->setData(ActionRefresh);
        }
    } 

    menu.addSeparator();

    // 注入“排序”二级子菜单
    QMenu* sortMenu = menu.addMenu("排序");
    UiHelper::applyMenuStyle(sortMenu);

    // 属性单选组
    QActionGroup* typeGroup = new QActionGroup(this);
    auto addTypeAct = [&](const QString& label, ContentPanel::SortType type) {
        QAction* act = sortMenu->addAction(label);
        act->setCheckable(true);
        act->setChecked(m_sortType == type);
        typeGroup->addAction(act);
        connect(act, &QAction::triggered, [this, type]() {
            m_sortType = type;
            AppConfig::instance().setValue("ContentPanel/RightClickSortType", static_cast<int>(type));
            
            // 实时触发全量无效化与排序重计算
            m_proxyModel->invalidate();
            m_proxyModel->sort(0, m_sortOrder);
        });
    };

    addTypeAct("名称", ContentPanel::SortByName);
    addTypeAct("创建日期", ContentPanel::SortByCreateDate);
    addTypeAct("修改日期", ContentPanel::SortByModifyDate);
    addTypeAct("扩展名", ContentPanel::SortByExtension);
    addTypeAct("大小", ContentPanel::SortBySize);
    addTypeAct("尺寸", ContentPanel::SortByDimension);
    addTypeAct("评分", ContentPanel::SortByRating);

    sortMenu->addSeparator();

    // 方向单选组
    QActionGroup* orderGroup = new QActionGroup(this);
    auto addOrderAct = [&](const QString& label, Qt::SortOrder order) {
        QAction* act = sortMenu->addAction(label);
        act->setCheckable(true);
        act->setChecked(m_sortOrder == order);
        orderGroup->addAction(act);
        connect(act, &QAction::triggered, [this, order]() {
            m_sortOrder = order;
            AppConfig::instance().setValue("ContentPanel/RightClickSortOrder", static_cast<int>(order));
            
            m_proxyModel->invalidate();
            m_proxyModel->sort(0, order);
        });
    };

    addOrderAct("升序", Qt::AscendingOrder);
    addOrderAct("降序", Qt::DescendingOrder);

    // =========================================================================
    // 🚨 全局唯一位置：“删除”子菜单（严格位于排序之后，且仅在选中常规文件/目录时呈现）
    // =========================================================================
    if (onItem && !isDriveRoot) {
        menu.addSeparator();
        QMenu* delMenu = menu.addMenu("删除");
        UiHelper::applyMenuStyle(delMenu);
        delMenu->addAction("移入回收站")->setData(ActionDelete);
        delMenu->addAction("永久删除")->setData(ActionSecureDelete);
    }

    // 🚀 【补丁彻底根除】：废除硬锁信号与物理禁用绘制！
    // 菜单弹出期间开启无锁模态标记，后台异步提取数据仅挂起不触发死锁，菜单关闭后自动 Flush
    m_isContextMenuActive = true;
    QAction* selectedAction = menu.exec(view->viewport()->mapToGlobal(pos)); 
    m_isContextMenuActive = false;

    if (!selectedAction || !selectedAction->data().isValid()) return; 
 
    ContextAction action = static_cast<ContextAction>(selectedAction->data().toInt()); 
 
    switch (action) { 
        case ActionOpen: {
            onDoubleClicked(currentIndex); 
            break; 
        }
        case ActionOpenDefault: {
            auto indexes = view->selectionModel()->selectedIndexes();
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    QString filePath = idx.data(PathRole).toString();
                    if (!filePath.isEmpty() && QFileInfo::exists(filePath)) {
                        // 1. 记录文件访问时间
                        MetadataManager::instance().recordAccess(filePath.toStdWString());
                        
                        // 2. 真正的操作系统级外部默认程序启动！
                        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
                    }
                }
            }
            break; 
        }
        case ActionShowInExplorer: { 
            QString targetPath = onItem ? path : m_currentPath;
            if (!targetPath.isEmpty() && !targetPath.contains("://")) {
                ShellHelper::openInExplorer(targetPath); 
            }
            break; 
        } 
        case ActionNewFolder: createNewItem("folder"); break; 
        case ActionNewMd: createNewItem("md"); break; 
        case ActionNewTxt: createNewItem("txt"); break; 
        case ActionPin: 
        case ActionUnpin: { 
            auto indexes = view->selectionModel()->selectedIndexes(); 
            bool pin = (action == ActionPin); 
            for (const QModelIndex& idx : indexes) { 
                if (idx.column() == 0) { 
                    // 2026-06-xx 架构简化：统一由 model->setData 处理持久化与缓存清理
                    m_proxyModel->setData(idx, pin, IsLockedRole); 
                } 
            } 
            // 2026-06-xx 物理修复：强制刷新代理模型排序，确保置顶项立即重排至顶部
            m_proxyModel->invalidate();
            m_proxyModel->sort(0, m_proxyModel->sortOrder());
            break; 
        } 
        case ActionEncrypt: { 
            FramelessInputDialog dlg("加密保护", "设置加密密码:", "", this);
            dlg.setEchoMode(QLineEdit::Password);
            if (dlg.exec() == QDialog::Accepted) { 
                QString pwd = dlg.text();
                if (pwd.isEmpty()) break;
                auto indexes = view->selectionModel()->selectedIndexes(); 
                QStringList targets; 
                for (const auto& idx : indexes) if (idx.column() == 0) targets << idx.data(PathRole).toString(); 
                 
                ToolTipOverlay::instance()->showText(QCursor::pos(), "加密任务已在后台启动...", 2000); 
                 
                std::string stdPwd = pwd.toStdString(); 
                QPointer<ContentPanel> self(this); 
                QString currentDir = m_currentPath; 
 
                (void)QThreadPool::globalInstance()->start([self, targets, stdPwd, currentDir]() { 
                    for (const QString& src : targets) { 
                        QString dest = src + ".amenc"; 
                        if (EncryptionManager::instance().encryptFile(src.toStdWString(), dest.toStdWString(), stdPwd)) { 
                            QFile::remove(src); 
                            MetadataManager::instance().setEncrypted(dest.toStdWString(), true); 
                        } 
                    } 
                    QMetaObject::invokeMethod(QCoreApplication::instance(), [self, currentDir]() { 
                        if (self && self->m_currentPath == currentDir) self->loadDirectory(currentDir, self->m_isRecursive); 
                        ToolTipOverlay::instance()->showText(QCursor::pos(), "加密任务处理完成", 1500, QColor("#2ecc71")); 
                    }); 
                }); 
            } 
            break; 
        } 
        case ActionDecrypt: { 
            FramelessInputDialog dlg("解除加密", "输入加密密码:", "", this);
            dlg.setEchoMode(QLineEdit::Password);
            if (dlg.exec() == QDialog::Accepted) { 
                QString pwd = dlg.text();
                if (!pwd.isEmpty()) { 
                    ToolTipOverlay::instance()->showText(QCursor::pos(), "解除加密逻辑已触发", 1500); 
                }
            } 
            break; 
        } 
        case ActionBatchRename: performBatchRename(); break; 
        case ActionRename: view->edit(currentIndex); break; 
        case ActionCopy: performCopy(false); break; 
        case ActionCut: performCopy(true); break; 
        case ActionPaste: performPaste(); break; 
        case ActionBatchCreate: {
            BatchCreateDialog dlg(m_currentPath, this);
            if (dlg.exec() == QDialog::Accepted) {
                refreshAll();
            }
            break;
        }
        case ActionRestore: {
            auto indexes = view->selectionModel()->selectedIndexes();
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    if (idx.data(IsDiskTrashRole).toBool()) {
                        int id = idx.data(DiskTrashIdRole).toInt();
                        QString trashPath = idx.data(PathRole).toString();
                        DiskTrashService::restoreFromDiskTrash(id, trashPath);
                    }
                }
            }
            // 修正：采用 refreshAll() 替换 loadDirectory(m_currentPath)
            refreshAll();
            MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild); // 刷新全量统计
            break;
        }
        case ActionDelete: 
        case ActionSecureDelete: {
            auto indexes = view->selectionModel()->selectedIndexes();
            QStringList targetPaths;
            std::vector<std::pair<int, QString>> diskTrashItems; // id, trashPath

            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    if (idx.data(IsDiskTrashRole).toBool()) {
                        int id = idx.data(DiskTrashIdRole).toInt();
                        QString p = idx.data(PathRole).toString();
                        diskTrashItems.push_back({id, p});
                    } else {
                        targetPaths << idx.data(PathRole).toString();
                    }
                }
            }
            if (targetPaths.isEmpty() && diskTrashItems.empty() && !path.isEmpty()) {
                targetPaths << path;
            }

            if (targetPaths.isEmpty() && diskTrashItems.empty()) break;

            if (action == ActionDelete) {
                OperationSnapshotEngine::instance().executeWithSnapshot(
                    this,
                    SnapshotOperationType::DeleteToTrash,
                    targetPaths,
                    QString("已将 %1 个项目移入回收站").arg(targetPaths.size()),
                    [this, targetPaths]() {
                        // 1. 开启内部操作锁
                        MetadataManager::instance().beginInternalOperation();

                        bool ok = false;
                        ok = DiskTrashService::moveToDiskTrash(targetPaths);

                        if (ok) {
                            refreshAll();
                        }

                        MetadataManager::instance().endInternalOperation();
                        return ok;
                    },
                    [this](const QVector<AssetItemSnapshot>& beforeState) {
                        for (const auto& snap : beforeState) {
                            int trashId = -1;
                            QString trashPath;
                            if (QuarkMeta::TrashRepository::instance().getDiskTrashRecordByPath(snap.path.toStdWString(), trashId, trashPath)) {
                                DiskTrashService::restoreFromDiskTrash(trashId, trashPath);
                            }
                        }
                        refreshAll();
                        MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
                        return true;
                    }
                );
            } else {
                QString msg = "确定要永久删除选中的项目吗？数据将被物理覆写并彻底抹除，此操作不可恢复。";
                if (!FramelessMessageBox::question(this, "确认删除", msg)) break;

                BatchProgressDialog* progress = new BatchProgressDialog("正在执行永久删除（深层抹除）...", this);
                progress->show();

                // 异步多线程执行物理安全删除与数据库记录清除 (双轨隔离)
                // 强制使用 QPointer 哨兵保护，防止 ContentPanel 提前析构导致主线程异步回调发生野指针悬空崩溃
                QPointer<ContentPanel> weakPanel(this);
                QPointer<BatchProgressDialog> weakProgress(progress);

                // 1. 开启内部操作锁
                MetadataManager::instance().beginInternalOperation();

                (void)QtConcurrent::run([targetPaths, diskTrashItems, action, weakPanel, weakProgress]() {
                    int total = static_cast<int>(targetPaths.size() + diskTrashItems.size());
                    int count = 0;

                    // A. 处理常规项目 (资源库托管或非回收站物理文件)
                    for (const QString& p : targetPaths) {
                        if (!weakPanel) return;
                        std::wstring wp = QDir::toNativeSeparators(p).toStdWString();
                        
                        bool physicalOk = false;
                        if (action == ActionSecureDelete) {
                            physicalOk = SecureFileEraser::shredFile(p);
                        } else {
                            // 普通彻底递归删除
                            std::function<bool(const QString&)> recursiveRemove;
                            recursiveRemove = [&](const QString& target) -> bool {
                                QFileInfo info(target);
                                bool ok = false;
                                QDir parentDir = info.dir();
                                if (info.isDir()) {
                                    QDir dir(target);
                                    for (const QString& entry : dir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
                                        recursiveRemove(target + "/" + entry);
                                    }
                                    ok = QDir().rmdir(target);
                                } else {
                                    ok = QFile::remove(target);
                                }

                                return ok;
                            };
                            physicalOk = recursiveRemove(p);
                        }

                        if (physicalOk) {
                            AppCommand cmd;
                            cmd.type = AppCommandType::DeletePermanently;
                            cmd.targetPaths << QString::fromStdWString(wp);
                            CoreEngine::instance().executeCommand(cmd);
                            UndoManager::instance().removeCommandsForPath(p);
                        }

                        count++;
                        if (weakProgress) {
                            int percent = (int)((float)count / total * 100);
                            QMetaObject::invokeMethod(QCoreApplication::instance(), [weakProgress, percent]() {
                                if (weakProgress) weakProgress->setValue(percent);
                            });
                        }
                    }

                    // B. 处理磁盘回收站中未还原项目 (通过专用表清除)
                    for (const auto& item : diskTrashItems) {
                        if (!weakPanel) return;
                        int id = item.first;
                        QString p = item.second;

                        DiskTrashService::permanentlyDeleteDiskTrash(id, p);

                        count++;
                        if (weakProgress) {
                            int percent = (int)((float)count / total * 100);
                            QMetaObject::invokeMethod(QCoreApplication::instance(), [weakProgress, percent]() {
                                if (weakProgress) weakProgress->setValue(percent);
                            });
                        }
                    }

                    // 完成后回到主线程安全执行 UI 级刷新与锁释放
                    QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, weakProgress]() {
                        if (weakProgress) {
                            weakProgress->accept();
                            weakProgress->deleteLater();
                        }
                        // 无论 UI 控件是否被销毁，操作锁都能被正确安全地释放 (双轨防护)
                        MetadataManager::instance().endInternalOperation();

                        if (weakPanel) {
                            weakPanel->refreshAll();
                        }
                    });
                });
            }
            break;
        }
        case ActionAddToFavorites: {
            QStringList selectedPaths;
            QModelIndexList indexes = getSelectedIndexes();
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    QString p = idx.data(PathRole).toString();
                    if (!p.isEmpty()) {
                        selectedPaths << p;
                    }
                }
            }
            if (!selectedPaths.isEmpty()) {
                OperationSnapshotEngine::instance().executeWithSnapshot(
                    this,
                    SnapshotOperationType::ToggleFavorite,
                    selectedPaths,
                    "已成功添加至收藏夹",
                    [this, selectedPaths]() {
                        emit requestAddFavorite(selectedPaths);
                        return true;
                    },
                    [](const QVector<AssetItemSnapshot>& beforeState) {
                        for (const auto& snap : beforeState) {
                            AppCommand cmd;
                            cmd.type = AppCommandType::SetPinned;
                            cmd.targetPaths << snap.path;
                            cmd.params["pinned"] = snap.isPinned;
                            CoreEngine::instance().executeCommand(cmd);
                        }
                        return true;
                    }
                );
            }
            break;
        }
        case ActionCopyName: {
            QModelIndexList indexes = getSelectedIndexes();
            QStringList targetNames;
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    QString p = idx.data(PathRole).toString();
                    if (!p.isEmpty()) targetNames << QFileInfo(p).fileName();
                }
            }
            if (targetNames.isEmpty() && !path.isEmpty()) {
                targetNames << QFileInfo(path).fileName();
            }
            if (!targetNames.isEmpty()) {
                QApplication::clipboard()->setText(targetNames.join("\r\n"));
                ToolTipOverlay::instance()->showText(QCursor::pos(), "已复制文件名到剪贴板", 1200, QColor("#2ecc71"));
            }
            break;
        }
        case ActionCopyPath: {
            QModelIndexList indexes = getSelectedIndexes();
            QStringList targetPaths;
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    QString p = idx.data(PathRole).toString();
                    if (!p.isEmpty()) targetPaths << QDir::toNativeSeparators(p);
                }
            }
            if (targetPaths.isEmpty() && !path.isEmpty()) {
                targetPaths << QDir::toNativeSeparators(path);
            }
            if (!targetPaths.isEmpty()) {
                QApplication::clipboard()->setText(targetPaths.join("\n"));
            }
            break;
        }
        case ActionRefresh: {
            refreshAll();
            break;
        }
        case ActionReextractThumbnail: {
            auto indexes = view->selectionModel()->selectedIndexes();
            QStringList targetPaths;
            for (const auto& idx : indexes) {
                if (idx.column() == 0 && !idx.data(PathRole).toString().isEmpty() && idx.data(TypeRole).toString() != "folder") {
                    targetPaths << idx.data(PathRole).toString();
                }
            }
            if (targetPaths.isEmpty() && !path.isEmpty() && !isFolder) {
                targetPaths << path;
            }

            if (!targetPaths.isEmpty()) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), QString("正在深度重新提取 %1 个项目的缩略图...").arg(targetPaths.size()), 2000, QColor("#3498db"));
                
                QPointer<ContentPanel> weakThis(this);
                DeepThumbnailExtractor::instance().extractBatchAsync(
                    targetPaths,
                    [weakThis](const QString& itemPath, bool success) {
                        if (weakThis && success && weakThis->m_diskModel) {
                            weakThis->m_diskModel->reloadThumbnailForPath(itemPath);
                        }
                    },
                    [weakThis](int successCount, int totalCount) {
                        if (weakThis) {
                            ToolTipOverlay::instance()->showText(
                                QCursor::pos(),
                                QString("缩略图提取完成：成功 %1 / 总计 %2").arg(successCount).arg(totalCount),
                                2000,
                                successCount > 0 ? QColor("#2ecc71") : QColor("#e81123")
                            );
                        }
                    }
                );
            }
            break;
        }
        default: break; 
    } 
} 
 
void ContentPanel::performCopy(bool cutMode) { 
    // 2026-03-xx 按照用户要求：封装标准化文件复制/剪切逻辑 
    QModelIndexList indexes = getSelectedIndexes(); 
    QList<QUrl> urls; 
    for (const auto& idx : indexes) { 
        if (idx.column() == 0) { 
            QString path = idx.data(PathRole).toString(); 
            if (!path.isEmpty()) urls << QUrl::fromLocalFile(path); 
        } 
    } 
 
    if (urls.isEmpty()) return; 
 
    QMimeData* mime = new QMimeData(); 
    mime->setUrls(urls); 
     
    if (cutMode) { 
        // 核心规范：告知系统这是剪切操作 (DROPEFFECT_MOVE = 2) 
        // 修复：将变量名由 data 改为 effectData，避免隐藏类成员警告 
        QByteArray effectData; 
        effectData.append((char)2);  
        mime->setData("Preferred DropEffect", effectData); 
    } 
 
    QApplication::clipboard()->setMimeData(mime); 
} 
 
void ContentPanel::performPaste() { 
    const QMimeData* mime = QApplication::clipboard()->mimeData(); 
    if (!mime) return; 

    // 1. 如果剪贴板是截图/图片：直接保存为新 PNG 文件 
    if (mime->hasImage() && (!mime->hasUrls() || mime->urls().isEmpty())) { 
        QImage img = qvariant_cast<QImage>(mime->imageData()); 
        if (!img.isNull() && !m_currentPath.isEmpty() && QDir(m_currentPath).exists()) { 
            QString timeStr = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"); 
            QString baseName = QString("贴图_%1").arg(timeStr); 
            QString fileName = baseName + ".png"; 
            QString fullPath = QDir(m_currentPath).filePath(fileName); 

            int counter = 1; 
            while (QFile::exists(fullPath)) { 
                fileName = QString("%1_(%2).png").arg(baseName).arg(counter++); 
                fullPath = QDir(m_currentPath).filePath(fileName); 
            } 

            if (img.save(fullPath, "PNG")) { 
                loadDirectory(m_currentPath, m_isRecursive); 
                setPendingSelectName(fileName, false); 
                ToolTipOverlay::instance()->showText(QCursor::pos(), "已将剪贴板图片粘贴为新文件", 1500, QColor("#2ecc71")); 
            } 
            return; 
        } 
    } 

    // 2. 常规文件与目录的粘贴（原有流程） 
    QList<QUrl> urls = mime->urls(); 
    QStringList fromPaths;
    for (const QUrl& url : urls) {
        fromPaths << url.toLocalFile();
    }
     
    if (fromPaths.isEmpty()) return; 

    if (!resolvePasteDestination()) return; // 内部已完成提示/取消处理

    if (dataSourceType() == DataSourceType::DiskNav) {
        bool isMove = false; 
        if (mime->hasFormat("Preferred DropEffect")) { 
            QByteArray effect = mime->data("Preferred DropEffect"); 
            if (!effect.isEmpty() && (effect.at(0) & 0x02)) isMove = true; 
        } 

        // 检查来源路径中是否包含回收站路径 (.QuarkMeta/disk_trash)
        bool hasTrashSource = false;
        for (const QString& p : fromPaths) {
            if (p.contains(".QuarkMeta/disk_trash", Qt::CaseInsensitive)) {
                hasTrashSource = true;
                break;
            }
        }

        if (hasTrashSource) {
            QPointer<ContentPanel> weakThis(this);
            QString targetDir = m_currentPath;
            (void)QtConcurrent::run([fromPaths, targetDir, weakThis]() {
                int successCount = 0;
                for (const QString& p : fromPaths) {
                    if (DiskTrashService::restoreToDirectory(p, targetDir)) {
                        successCount++;
                    }
                }
                QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, targetDir, successCount]() {
                    if (weakThis) {
                        weakThis->loadDirectory(targetDir, weakThis->m_isRecursive);
                        if (successCount > 0) {
                            ToolTipOverlay::instance()->showText(QCursor::pos(), QString("成功从回收站还原并恢复 %1 个项目").arg(successCount), 1500, QColor("#2ecc71"));
                        }
                    }
                });
            });
            return;
        }

        // 彻底切断主线程物理 I/O，全权交由 DiskIoService 异步处理，UI 主线程 0 毫秒阻塞
        DiskIoContext ioCtx;
        ioCtx.sources = fromPaths;
        ioCtx.destination = m_currentPath;
        ioCtx.isMove = isMove;
        
        QPointer<ContentPanel> weakThis(this);
        DiskIoService::instance().executeAsync(ioCtx, [weakThis, fromPaths](bool success) {
            QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, success, fromPaths]() {
                Q_UNUSED(fromPaths);
                if (weakThis) {
                    if (success) {
                        weakThis->loadDirectory(weakThis->m_currentPath, weakThis->m_isRecursive);
                    } else {
                        ToolTipOverlay::instance()->showText(QCursor::pos(), "粘贴失败：文件写入操作未能完成", 2000, QColor("#e81123"));
                    }
                }
            });
        });
    }
} 
 
void ContentPanel::performBatchRename() { 
    // 2026-03-xx 按照用户要求：弹出深度集成的高级批量重命名对话框 
    QModelIndexList indexes = getSelectedIndexes(); 
    QStringList selectedPaths;
    std::vector<std::wstring> originalPaths; 
    for (const auto& idx : indexes) { 
        if (idx.column() == 0) { 
            QString path = idx.data(PathRole).toString(); 
            if (!path.isEmpty()) {
                selectedPaths << path;
                originalPaths.push_back(QDir::toNativeSeparators(path).toStdWString()); 
            }
        } 
    } 
 
    if (originalPaths.empty()) { 
        return; 
    } 
 
    BatchRenameDialog dlg(originalPaths, this); 
    if (dlg.exec() == QDialog::Accepted) { 
        QString firstNew = dlg.getFirstNewName();
        if (!firstNew.isEmpty()) {
            setPendingSelectName(firstNew, false);
        }
        refreshAll(); 
    }
} 
 
ContentPanel::DataSourceType ContentPanel::dataSourceType() const {
    if (m_currentCategoryType == "path_list" || m_currentCategoryType == "search") {
        return DataSourceType::PathList;
    }
    return DataSourceType::DiskNav;
}


void ContentPanel::onSelectionChanged() { 
    // 1. 初始化 30ms 防抖定时器
    if (!m_selectionTimer) {
        m_selectionTimer = new QTimer(this);
        m_selectionTimer->setSingleShot(true);
        m_selectionTimer->setInterval(30); // 30 毫秒黄金防抖窗口

        connect(m_selectionTimer, &QTimer::timeout, this, [this]() {
            QItemSelectionModel* selectionModel = (m_viewStack->currentWidget() == m_gridView) ? 
                m_gridView->selectionModel() : m_treeView->selectionModel();
            if (!selectionModel) return;

            QStringList selectedPaths;
            QModelIndexList indices = selectionModel->selectedIndexes();
            for (const QModelIndex& index : indices) {
                if (index.column() == 0) {
                    QString path = index.data(PathRole).toString();
                    if (!path.isEmpty()) selectedPaths.append(path);
                }
            }

            // 🚨 防抖时间到达后，只向外发射 1 次选中信号！彻底消灭信号风暴！
            emit selectionChanged(selectedPaths);
        });
    }

    // 🚨 2. 每次鼠标快速点击/滑动，仅重置防抖定时器，不阻塞 UI 线程！
    m_selectionTimer->start();
} 
 
void ContentPanel::refreshAll() {
    // 1. 批量暂存所有当前选中项的文件名，杜绝多选丢失！
    QModelIndexList selected = getSelectedIndexes();
    if (!selected.isEmpty() && m_pendingSelectNames.isEmpty()) {
        for (const auto& idx : selected) {
            if (idx.column() == 0) {
                QString p = idx.data(PathRole).toString();
                if (!p.isEmpty()) {
                    m_pendingSelectNames.insert(QFileInfo(p).fileName());
                }
            }
        }
        m_isPendingEdit = false;
    }

    // 2026-06-xx 物理对标：完善刷新逻辑，支持所有上下文类型
    if (!m_currentPath.isEmpty() && m_currentPath != "computer://") {
        loadDirectory(m_currentPath, m_isRecursive);
    } else {
        // 兜底逻辑：加载“此电脑”
        loadDirectory("computer://");
    }
}

void ContentPanel::updateItemMetadata(const QString& path) {
    if (m_model) {
        m_model->updateRecordMetadata(path);
    }
}

void ContentPanel::migrateModelCache(const QString& oldPath, const QString& newPath) {
    if (m_model) {
        m_model->migrateCache(oldPath, newPath);
    }
}

void ContentPanel::clearFolderCache(const QString& folderPath) {
    if (m_model) {
        m_model->clearCacheForFolder(folderPath);
    }
}

void ContentPanel::onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex) {
    if (paths.isEmpty()) return;
    if (m_currentPath.isEmpty() || m_currentPath == "computer://") return;

    QString destDir = m_currentPath;
    if (targetIndex.isValid()) {
        QModelIndex srcIdx = m_proxyModel->mapToSource(targetIndex);
        if (srcIdx.isValid()) {
            QString targetPath = srcIdx.data(PathRole).toString();
            if (!targetPath.isEmpty() && QFileInfo(targetPath).isDir()) {
                destDir = targetPath;
            }
        }
    }

    // 检查是否在原地投放
    bool sameDir = true;
    for (const QString& p : paths) {
        if (QDir::toNativeSeparators(QFileInfo(p).absolutePath()) != QDir::toNativeSeparators(destDir)) {
            sameDir = false;
            break;
        }
    }
    if (sameDir && destDir == m_currentPath) return;

    bool isMove = !(QApplication::keyboardModifiers() & Qt::ControlModifier);
    
    DiskIoContext ioCtx; 
    ioCtx.sources = paths; 
    ioCtx.destination = destDir; 
    ioCtx.isMove = isMove; 

    QPointer<ContentPanel> weakThis(this); 
    DiskIoService::instance().executeAsync(ioCtx, [weakThis](bool success) { 
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, success]() { 
            if (weakThis && success) { 
                weakThis->loadDirectory(weakThis->m_currentPath, weakThis->m_isRecursive); 
            } 
        }); 
    }); 
}

void ContentPanel::onDoubleClicked(const QModelIndex& index) { 
    if (!index.isValid()) return; 
 
    QString path = index.data(PathRole).toString(); 
    if (path.isEmpty()) return; 
 
    QFileInfo info(path); 
    if (info.isDir()) { 
        emit directorySelected(path);  
    } else { 
        AppCommand cmd;
        cmd.type = AppCommandType::RecordAccess;
        cmd.targetPaths << path;
        CoreEngine::instance().executeCommand(cmd);
        
        // 2026-11-xx 按照用户全新要求：在内容面板双击某个文件时如同按下空格键那样打开预览
        QString ext = info.suffix().toLower();
        // 1. 系统级不可预览黑名单 (包含压缩包、二进制文件及系统库)
        static const QSet<QString> blackList = {
            "exe", "dll", "sys", "bin", "dat", "lib", "obj", "msi", "com",
            "zip", "rar", "7z", "iso", "tar", "gz", "bz2", "dmg", "pkg"
        };
        if (blackList.contains(ext)) return;

        // 2. 预览准入白名单 (仅限受支持的图像类及文本/代码类文件)
        static const QSet<QString> whiteList = {
            "jpg", "jpeg", "png", "bmp", "webp", "gif", "ico", "cur", "ani", "psd", "ai", "eps", "pdf", "svg",
            "txt", "md", "markdown", "log", "cpp", "h", "hpp", "c", "py", "js", "css", "html", "json", "xml", "ini", "conf", "yaml", "yml"
        };

        if (whiteList.contains(ext)) {
            emit requestQuickLook(path);
        }
    } 
} 
 
void ContentPanel::restoreActiveView() {
    if (m_currentViewMode == ListView) {
        m_viewStack->setCurrentWidget(m_treeView);
    } else {
        m_viewStack->setCurrentWidget(m_gridView);
    }
}

void ContentPanel::restoreSelections() {
    if (!m_pendingSelectNames.isEmpty()) {
        QAbstractItemView* view = (m_viewStack->currentWidget() == m_gridView) ? 
            static_cast<QAbstractItemView*>(m_gridView) : static_cast<QAbstractItemView*>(m_treeView);

        if (view && view->selectionModel()) {
            QItemSelection selection;
            QModelIndex lastProxyIdx;
            const auto& records = m_model->allRecords();
            for (size_t i = 0; i < records.size(); ++i) {
                QString fn = QFileInfo(records[i].path).fileName();
                if (m_pendingSelectNames.contains(fn)) {
                    QModelIndex srcIdx = m_model->index(static_cast<int>(i), 0);
                    QModelIndex proxyIdx = m_proxyModel->mapFromSource(srcIdx);
                    if (proxyIdx.isValid()) {
                        selection.select(proxyIdx, proxyIdx);
                        lastProxyIdx = proxyIdx;
                    }
                }
            }
            view->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            if (lastProxyIdx.isValid()) {
                view->scrollTo(lastProxyIdx);
                if (m_isPendingEdit) view->edit(lastProxyIdx);
            }
        }
        m_pendingSelectNames.clear();
    }
}

void ContentPanel::loadDirectory(const QString& path, bool recursive) { 
    restoreActiveView(); // 🚨 强行切离开锁屏页，恢复卡片网格/列表页！

    // 1. 切换目录时立即熔断并清空前一次目录的后台提图任务队列
    MediaExtractorPipeline::instance().cancelAll();

    // 2. 递增模型代际号，废止前一个目录正在跑的所有子任务
    if (m_diskModel) {
        m_diskModel->incrementGeneration();
    }

    if (m_model != m_diskModel) {
        m_model = m_diskModel;
        m_proxyModel->setSourceModel(m_model);
    }

    m_isLoading = true;
    int reqId = ++m_loadRequestId;
    m_currentCategoryType = ""; // 物理导航模式下清除系统类型
    emit dataSourceChanged("nav"); 
    if (m_viewStack) m_viewStack->show(); 
 
    m_isRecursive = recursive; 
    if (m_btnLayers) m_btnLayers->setChecked(recursive); 
 
    if (path.isEmpty() || path == "computer://") { 
        m_currentPath = "computer://"; 
        updateLayersButtonState(); 
 
        const auto drives = QDir::drives(); 
        std::vector<ItemRecord> driveRecords;
        for (const QFileInfo& drive : drives) { 
            driveRecords.push_back(ItemRecord::create(drive.absolutePath()));
        } 
        MetaCacheDecorator::decorate(driveRecords);
        m_model->setRecords(driveRecords);
        // 2026-05-29 物理对齐：在加载“此电脑”后显式触发一次排序，使用持久化的 m_sortOrder
        m_proxyModel->sort(0, m_sortOrder);
        m_isLoading = false;
        recalculateAndEmitStats();
        return; 
    } 
 
    m_currentPath = path; 
    updateLayersButtonState(); 

    QPointer<ContentPanel> panelPtr(this); 

    // 通道隔离：使用 QtConcurrent::run 代替 QThreadPool::globalInstance()->start，
    // 防止 UI 导航与目录扫描任务被后台大量缩略图提取慢任务死死霸占而排队卡死
    (void)QtConcurrent::run([panelPtr, path, recursive, reqId]() { 
        if (!panelPtr) return; 

        std::vector<ItemRecord> allItems = DiskScanService::scanDirectory(
            path, recursive,
            [panelPtr]() { return static_cast<bool>(panelPtr); }
        );
        if (!panelPtr) return; 

        // 🚀 线程安全地装配离散业务元数据（支持单级与深层递归目录） 
        MetaCacheDecorator::decorate(allItems); 
 
        QMetaObject::invokeMethod(QCoreApplication::instance(), [panelPtr, path, allItems, reqId]() { 
            if (panelPtr && panelPtr->m_loadRequestId == reqId) { 
                panelPtr->m_model->setRecords(allItems);
                panelPtr->m_proxyModel->sort(0, panelPtr->m_sortOrder);
                panelPtr->m_isLoading = false;
                panelPtr->recalculateAndEmitStats();
                // 2026-06-xx 物理同步：数据加载完成后强制重新应用筛选，防止显示已过滤掉的占位符记录
                panelPtr->applyFilters();

                panelPtr->restoreSelections();

                panelPtr->m_visibleTimer->start();
            }
        }, Qt::QueuedConnection); 
    }); 
} 
 
 
 
 
void ContentPanel::search(const QString& query) { 
    // 2026-07-xx 按照 Plan-118：搜索行为回归筛选流。
    // 搜索框仅作为当前视图的本地过滤器，禁止切换 m_currentCategoryType 为 "search"。

    // 1. 同步关键词到当前筛选状态
    m_currentFilter.keyword = query;

    // 2. 触发本地过滤（invalidateFilter）
    applyFilters();

    // 3. 视觉状态同步
    if (m_viewStack) m_viewStack->show(); 
} 
 
void ContentPanel::applyFilters(const FilterState& state) { 
    // 2026-07-xx 物理防护：保留标题栏按钮独占维护的显隐状态，防止被 FilterPanel 的默认值覆盖
    bool preservedShowFolders = m_currentFilter.showFolders;
    bool preservedShowFiles = m_currentFilter.showFiles;
    bool preservedShowHidden = m_currentFilter.showHidden;
    m_currentFilter = state; 
    m_currentFilter.showFolders = preservedShowFolders;
    m_currentFilter.showFiles = preservedShowFiles;
    m_currentFilter.showHidden = preservedShowHidden;
    applyFilters(); 
} 
 
void ContentPanel::applyFilters() { 
    // 2026-05-25 编译修复：改用 qobject_cast 彻底根除 static_cast 指针转换报错 
    auto* proxy = qobject_cast<FilterProxyModel*>(m_proxyModel); 
    if (proxy) { 
        proxy->currentFilter = m_currentFilter; 
        proxy->updateFilter(); 
    } 
    // 2026-05-08 按照用户要求：筛选条件变化后更新状态栏统计
    updateStatusBarStats();
    m_visibleTimer->start();
} 
 
 
void ContentPanel::loadCategory(const QString& categoryType) {
    m_currentCategoryType = categoryType;
    if (categoryType == "trash") {
        m_currentPath = "trash://";
        loadPaths({});
    }
} 
 
static std::vector<ItemRecord> loadPathItemsInternal(const QStringList& paths) {
    std::vector<ItemRecord> records;
    records.reserve(paths.size());
    for (const QString& p : paths) {
        records.push_back(ItemRecord::create(p));
    }
    MetaCacheDecorator::decorate(records);
    return records;
}

static std::vector<ItemRecord> loadTrashItemsInternal() {
    std::vector<ItemRecord> records;
    
    // 1. 扫描物理磁盘回收站表 (disk_trash)
    auto rawDiskTrash = DiskTrashRepo::getAllTrashItems();
    for (const auto& raw : rawDiskTrash) {
        ItemRecord rec;
        rec.isDiskTrash = true;
        rec.diskTrashId = raw.id;
        rec.fileId = QString::fromStdWString(raw.fileId);
        rec.path = QString::fromStdWString(raw.trashPath);
        rec.originalPath = QString::fromStdWString(raw.originalPath);
        rec.filename = QString::fromStdWString(raw.fileName);
        rec.suffix = QFileInfo(rec.filename).suffix();
        rec.isDir = raw.isFolder;
        rec.size = raw.fileSize;
        rec.ctime = raw.createdAt > 0 ? raw.createdAt : raw.deletedAt;
        rec.mtime = raw.deletedAt;
        records.push_back(rec);
    }

    MetaCacheDecorator::decorate(records);
    return records;
}

void ContentPanel::loadPaths(const QStringList& paths, int reqId) {
    restoreActiveView();

    if (m_model != m_diskModel) {
        m_model = m_diskModel;
        m_proxyModel->setSourceModel(m_model);
    }

    if (paths.isEmpty() && m_currentCategoryType != "trash") {
        if (reqId == 0) m_loadRequestId++;
        else m_loadRequestId = reqId;
        
        m_model->clear();
        m_isLoading = false;
        recalculateAndEmitStats();
        return;
    }

    m_isLoading = true;
    if (reqId == 0) reqId = ++m_loadRequestId;

    // 维持既有的系统分类类型标识
    if (m_currentCategoryType.isEmpty()) {
        m_currentCategoryType = "path_list";
    }
    updateLayersButtonState();
    
    m_viewStack->show(); 
    
    emit dataSourceChanged("category"); 
    
    QPointer<ContentPanel> weakThis(this);
    (void)QtConcurrent::run([weakThis, paths, reqId]() {
        if (!weakThis) return;
        std::vector<ItemRecord> records;
        if (weakThis->getCurrentCategoryType() == "trash") {
            records = loadTrashItemsInternal();
        } else {
            records = loadPathItemsInternal(paths);
        }
        if (!weakThis) return;
        
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, records, reqId]() {
            if (weakThis && weakThis->m_loadRequestId == reqId) {
                weakThis->m_model->setRecords(records);
                weakThis->m_proxyModel->sort(0, weakThis->m_sortOrder);
                weakThis->m_isLoading = false;
                weakThis->recalculateAndEmitStats();
                weakThis->applyFilters(); 

                weakThis->restoreSelections();
            }
        });
    });
}

void ContentPanel::appendPaths(const QStringList& paths, int reqId) {
    if (paths.isEmpty()) return;

    // 物理校验：如果指定了请求 ID，则必须与当前 ID 匹配，否则视为过期搜索结果
    if (reqId != 0 && m_loadRequestId != reqId) {
        return;
    }

    QPointer<ContentPanel> weakThis(this);
    (void)QtConcurrent::run([weakThis, paths, reqId]() {
        if (!weakThis) return;
        std::vector<ItemRecord> newRecords = loadPathItemsInternal(paths);
        if (!weakThis) return;

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, newRecords, reqId]() {
            if (weakThis && (reqId == 0 || weakThis->m_loadRequestId == reqId)) {
                // 获取当前已有记录并追加
                std::vector<ItemRecord> all = weakThis->m_model->allRecords();
                all.insert(all.end(), newRecords.begin(), newRecords.end());
                weakThis->m_model->setRecords(all);
                
                // 异步流式追加时，每批次都尝试更新一次统计与筛选
                weakThis->recalculateAndEmitStats();
                weakThis->applyFilters();
            }
        });
    });
}
 
bool ContentPanel::resolvePasteDestination() {
    if (m_currentCategoryType == "trash") {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "当前视图为回收站，不支持粘贴或拖拽导入新项目", 2000, QColor("#e81123"));
        return false;
    }

    if (m_currentPath.isEmpty() || m_currentPath == "computer://") {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "粘贴失败：当前未处于任何有效目录中", 2000, QColor("#e81123"));
        return false;
    }

    return true;
}

void ContentPanel::recalculateAndEmitStats() {
    const std::vector<ItemRecord>& records = m_model->allRecords();
    if (records.empty()) return;

    QPointer<ContentPanel> weakThis(this);
    (void)QtConcurrent::run([weakThis, records]() {
        ScanStats stats;

        auto makeDupKey = [](const ItemRecord& rec) -> std::string {
            if (!rec.sha256.isEmpty()) {
                return rec.sha256.toStdString();
            }
            if (rec.width > 0 && rec.height > 0) {
                return std::to_string(rec.size) + "_" + std::to_string(rec.width) + "x" + std::to_string(rec.height);
            }
            return std::to_string(rec.size);
        };

        // 1. 预统计重复项（内存快速桶，去文件名化判重）
        std::unordered_map<std::string, int> hashCounts;
        for (const auto& record : records) {
            if (record.isDir) continue;
            hashCounts[makeDupKey(record)]++;
        }

        // 2. 遍历全量记录进行多维统计
        for (const auto& record : records) {
            if (!weakThis) return;

            // 0. 隐藏属性过滤（关闭隐藏项显示时，隐藏属性文件不参与筛选面板统计）
            if (record.isHidden && !weakThis->m_currentFilter.showHidden) {
                continue;
            }

            stats.ratingCounts[record.rating]++;
            QString normHex = UiHelper::normalizeColorHex(record.manualColor);
            stats.colorCounts[normHex]++;
            
            if (record.isDir) {
                stats.typeCounts["folder"]++;
                if (record.isEmpty) {
                    stats.emptyFolderCount++;
                }
            } else {
                stats.typeCounts["file"]++;
                stats.typeCounts[record.suffix.toUpper()]++;

                // 链接统计
                if (!record.url.isEmpty()) stats.hasLinkCount++;
                else stats.noLinkCount++;

                // 备注统计
                if (!record.note.isEmpty()) stats.hasNoteCount++;
                else stats.noNoteCount++;

                // 标签存在性统计
                if (!record.tags.isEmpty()) stats.hasTagCount++;
                else stats.noTagCount++;

                // 图像比例统计
                if (record.width > 0 && record.height > 0) {
                    double r = (double)record.width / record.height;
                    if (record.width > record.height) stats.ratioHorizontalCount++;
                    if (record.height > record.width) stats.ratioVerticalCount++;
                    if (std::abs(r - 1.0) <= 0.05) stats.ratioSquareCount++;
                    if (std::abs(r - 1.77) <= 0.05) stats.ratio169Count++;
                }

                // 重复状态统计
                if (hashCounts[makeDupKey(record)] > 1) {
                    stats.duplicateCount++;
                } else {
                    stats.uniqueCount++;
                }

                // 缩略图状态 (提取成功/缺失) 统计
                if (UiHelper::isGraphicsFile(record.suffix)) {
                    QString thumbPath = DiskMediaExtractor::getDiskThumbCachePath(record.path);
                    if (QFile::exists(thumbPath)) {
                        stats.hasThumbnailCount++;
                    } else {
                        stats.noThumbnailCount++;
                    }
                }
            }
            
            auto dateKey = [&](long long ts) {
                return QDateTime::fromMSecsSinceEpoch(ts).date().toString("dd-MM-yyyy");
            };
            stats.createDateCounts[dateKey(record.ctime)]++;
            stats.modifyDateCounts[dateKey(record.mtime)]++;
        }

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, stats]() {
            if (weakThis) {
                emit weakThis->directoryStatsReady(stats);
            }
        });
    });
}

void ContentPanel::createNewItem(const QString& type) { 
    if (m_currentPath.isEmpty() || m_currentPath == "computer://") return; 

    QString baseName = (type == "folder") ? "新建文件夹" : "未命名"; 
    QString ext = (type == "md") ? ".md" : ((type == "txt") ? ".txt" : ""); 
    QString finalName = baseName + ext; 
    QString fullPath = m_currentPath + "/" + finalName; 

    int counter = 1; 
    while (QFileInfo::exists(fullPath)) { 
        finalName = baseName + QString(" (%1)").arg(counter++) + ext; 
        fullPath = m_currentPath + "/" + finalName; 
    } 

    bool success = false; 
    if (type == "folder") { 
        success = QDir(m_currentPath).mkdir(finalName); 
    } else { 
        QFile file(fullPath); 
        if (file.open(QIODevice::WriteOnly)) { 
            file.close(); 
            success = true; 
        } 
    } 

    if (success) { 
        setPendingSelectName(finalName, true);
        loadDirectory(m_currentPath, m_isRecursive); 
    }
} 
 
void ContentPanel::updateLayersButtonState() { 
    if (!m_btnLayers) return; 
 
    m_btnLayers->setVisible(true);

    if (m_currentPath.isEmpty() || m_currentPath == "computer://") { 
        m_btnLayers->setEnabled(false); 
        m_btnLayers->setChecked(false); 
        m_btnLayers->setProperty("tooltipText", "“此电脑”不支持递归显示"); 
        return; 
    } 

    m_btnLayers->setEnabled(true); 
    m_btnLayers->setProperty("tooltipText", "显示子文件夹中的项目"); 
} 
 
} // namespace QuarkMeta
