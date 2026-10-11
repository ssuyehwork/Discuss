#include "LibraryPanel.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "PresetTagsDialog.h"
#include "ColorPicker.h"
#include "ShellIconManager.h"
#include "ViewDragDropHelper.h"
#include "../meta/LibraryDao.h"
#include "../meta/LibraryService.h"
#include "../meta/QuarkMetaJsonStore.h"
#include "../core/CoreEngine.h"
#include "../core/ModelContract.h"
#include "../core/AppConfig.h"
#include "controllers/ContextMenuFactory.h"
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QWidgetAction>
#include <QGridLayout>
#include <QHeaderView>
#include <QFileInfo>
#include <QDir>
#include <QCursor>
#include <QPainter>
#include <QLineEdit>
#include <QTimer>
#include <functional>

namespace QuarkMeta {

QSize LibraryItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QSize sz = QStyledItemDelegate::sizeHint(option, index);
    sz.setHeight(28);
    return sz;
}

QRect LibraryItemDelegate::getAddButtonRect(const QRect& optionRect) {
    int btnWidth = 20;
    int btnHeight = 20;
    int rightMargin = 8;
    return QRect(optionRect.right() - rightMargin - btnWidth,
                 optionRect.top() + (optionRect.height() - btnHeight) / 2,
                 btnWidth, btnHeight);
}

void LibraryItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    int nodeId = index.data(Qt::UserRole + 1).toInt();

    // 1. 渲染自定义分类折叠分组 Header (nodeId == 0)
    if (nodeId == 0) {
        if (opt.state & QStyle::State_Selected) {
            painter->fillRect(opt.rect, QColor("#37373D"));
        } else if (opt.state & QStyle::State_MouseOver) {
            painter->fillRect(opt.rect, QColor("#2A2D2E"));
        } else {
            painter->fillRect(opt.rect, Qt::transparent);
        }

        const QTreeView* treeView = qobject_cast<const QTreeView*>(option.widget);
        bool isExpanded = treeView ? treeView->isExpanded(index) : true;
        int customCount = index.data(Qt::UserRole + 9).toInt();

        // 绘制折叠/展开箭头 (scroll-008: 展开向下, scroll-010: 折叠向右)
        QString arrowKey = isExpanded ? "scroll-008" : "scroll-010";
        QIcon arrowIcon = UiHelper::getIcon(arrowKey, QColor("#888888"), 14);
        QRect arrowRect(opt.rect.left() + 6, opt.rect.top() + (opt.rect.height() - 14) / 2, 14, 14);
        arrowIcon.paint(painter, arrowRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);

        // 绘制 Header 文字 "文件夹(N)"
        QString sectionText = QString("文件夹(%1)").arg(customCount);
        QRect textRect(arrowRect.right() + 6, opt.rect.top(), opt.rect.width() - 50, opt.rect.height());

        QFont hdrFont = opt.font;
        hdrFont.setPointSize(11);
        hdrFont.setBold(true);
        painter->setFont(hdrFont);
        painter->setPen((opt.state & QStyle::State_MouseOver) ? QColor("#FFFFFF") : QColor("#888888"));
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, sectionText);

        // 右侧加号按钮 (+) 图标 ("add") - 共享 getAddButtonRect 统一计算几何矩形
        QRect addRect = getAddButtonRect(opt.rect);
        QIcon addIcon = UiHelper::getIcon("add", (opt.state & QStyle::State_MouseOver) ? QColor("#FFFFFF") : QColor("#888888"), 16);
        addIcon.paint(painter, addRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);

        painter->restore();
        return;
    }

    // 2. 渲染系统分类与普通自定义分类节点 (nodeId != 0)
    bool isDropTarget = index.data(IsDropTargetRole).toBool() ||
                       ViewDragDropHelper::isDropTarget(option.widget, index);

    if (isDropTarget) {
        QColor dropBg("#3498db");
        dropBg.setAlphaF(0.35f);
        painter->fillRect(opt.rect, dropBg);
    } else if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, QColor("#37373D"));
    } else if (opt.state & QStyle::State_MouseOver) {
        painter->fillRect(opt.rect, QColor("#2A2D2E"));
    } else {
        painter->fillRect(opt.rect, Qt::transparent);
    }

    int leftMargin = 10;
    int iconSize = 18;
    int spacing = 8;
    int rightMargin = 10;

    int count = index.data(Qt::UserRole + 9).toInt();
    QString countStr = (count >= 0) ? QString::number(count) : "";
    int countWidth = countStr.isEmpty() ? 0 : (opt.fontMetrics.horizontalAdvance(countStr) + 12);

    QRect iconRect(opt.rect.left() + leftMargin, opt.rect.top() + (opt.rect.height() - iconSize) / 2, iconSize, iconSize);
    QRect textRect(iconRect.right() + spacing, opt.rect.top(), opt.rect.width() - leftMargin - iconSize - spacing - countWidth - rightMargin, opt.rect.height());

    QVariant decoData = index.data(Qt::DecorationRole);
    if (decoData.canConvert<QIcon>()) {
        QIcon icon = decoData.value<QIcon>();
        if (!icon.isNull()) {
            icon.paint(painter, iconRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
        }
    }

    QString text = index.data(Qt::DisplayRole).toString();
    painter->setFont(opt.font);

    QString elidedText = opt.fontMetrics.elidedText(text, Qt::ElideRight, textRect.width() - 4);

    // 精确高亮渲染：仅高亮匹配的关键字子串 (#3498db)，非匹配子串维持正常颜色 (#EEEEEE / #FFFFFF)
    if (!m_filterKeyword.isEmpty() && elidedText.contains(m_filterKeyword, Qt::CaseInsensitive)) {
        QColor defaultPenColor = (opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : QColor("#EEEEEE");
        QColor highlightPenColor("#3498db");

        int curX = textRect.left();
        int fontHeight = opt.fontMetrics.height();
        int drawY = textRect.top() + (textRect.height() - fontHeight) / 2 + opt.fontMetrics.ascent();

        int pos = 0;
        int len = elidedText.length();
        int kwLen = m_filterKeyword.length();

        while (pos < len) {
            int matchIdx = elidedText.indexOf(m_filterKeyword, pos, Qt::CaseInsensitive);
            if (matchIdx == -1) {
                // 绘制末尾剩余的非匹配子串
                QString part = elidedText.mid(pos);
                painter->setPen(defaultPenColor);
                painter->drawText(curX, drawY, part);
                break;
            }

            if (matchIdx > pos) {
                // 绘制匹配位置之前的非匹配子串
                QString beforePart = elidedText.mid(pos, matchIdx - pos);
                painter->setPen(defaultPenColor);
                painter->drawText(curX, drawY, beforePart);
                curX += opt.fontMetrics.horizontalAdvance(beforePart);
            }

            // 绘制精确匹配的关键字子串
            QString matchPart = elidedText.mid(matchIdx, kwLen);
            painter->setPen(highlightPenColor);
            painter->drawText(curX, drawY, matchPart);
            curX += opt.fontMetrics.horizontalAdvance(matchPart);

            pos = matchIdx + kwLen;
        }
    } else {
        painter->setPen((opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : QColor("#EEEEEE"));
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);
    }

    // 绘制右对齐数字计数
    if (!countStr.isEmpty()) {
        QRect countRect(opt.rect.right() - countWidth - rightMargin, opt.rect.top(), countWidth, opt.rect.height());
        QFont countFont = opt.font;
        countFont.setBold(true);
        painter->setFont(countFont);
        painter->setPen((opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : QColor("#999999"));
        painter->drawText(countRect, Qt::AlignRight | Qt::AlignVCenter, countStr);
    }

    painter->restore();
}

void LibraryItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Q_UNUSED(index);
    if (!editor) return;

    QRect textRect = option.rect;
    textRect.setLeft(option.rect.left() + 33);
    textRect.setRight(option.rect.right() - 3);

    editor->setGeometry(textRect);
}

LibraryPanel::LibraryPanel(QWidget* parent) : QFrame(parent) {
    setObjectName("LibraryContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    initUi();
    loadLibrary();
}

void LibraryPanel::initUi() {
    m_treeView = new DropTreeView(this);
    m_treeView->setObjectName("LibraryTreeView");
    m_treeView->setHeaderHidden(true);
    if (m_treeView->header()) {
        m_treeView->header()->setStretchLastSection(true);
        m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    }
    m_treeView->setIndentation(16);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setDragEnabled(true);
    m_treeView->setAcceptDrops(true);
    m_treeView->setDropIndicatorShown(true);
    m_treeView->setDefaultDropAction(Qt::MoveAction);
    m_treeView->setDragDropMode(QAbstractItemView::DragDrop);
    m_treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_model = new QStandardItemModel(this);
    m_treeView->setModel(m_model);
    m_delegate = new LibraryItemDelegate(this);
    m_treeView->setItemDelegate(m_delegate);

    m_mainLayout->addWidget(m_treeView, 1);

    // 底部搜索过滤框部件创建与配置
    QWidget* filterContainer = new QWidget(this);
    filterContainer->setObjectName("LibraryFilterContainer");
    QVBoxLayout* filterLayout = new QVBoxLayout(filterContainer);
    filterLayout->setContentsMargins(8, 8, 8, 8);
    filterLayout->setSpacing(0);

    m_filterEdit = new QLineEdit(filterContainer);
    m_filterEdit->setObjectName("LibraryFilterEdit");
    m_filterEdit->setPlaceholderText("搜索文件夹...");
    m_filterEdit->setClearButtonEnabled(true);
    m_filterEdit->setFixedHeight(28);
    m_filterEdit->addAction(UiHelper::getIcon("seach-7", QColor("#888888"), 14), QLineEdit::LeadingPosition);
    filterLayout->addWidget(m_filterEdit);

    m_mainLayout->addWidget(filterContainer, 0);

    connect(m_filterEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        QString kw = text.trimmed();
        if (m_delegate) {
            m_delegate->setFilterKeyword(kw);
        }
        filterTreeNodes(kw);
        if (m_treeView && m_treeView->viewport()) {
            m_treeView->viewport()->update();
        }
    });

    connect(m_treeView, &QTreeView::clicked, this, &LibraryPanel::onCategoryClicked);
    connect(m_treeView, &QWidget::customContextMenuRequested, this, &LibraryPanel::onCategoryContextMenu);
    connect(m_treeView, &DropTreeView::pathsDropped, this, &LibraryPanel::onPathsDroppedToCategory);

    connect(&LibraryService::instance(), &LibraryService::libraryChanged, this, [this]() {
        QTimer::singleShot(0, this, [this]() {
            loadLibrary();
        });
    });

    connect(m_model, &QStandardItemModel::itemChanged, this, [this](QStandardItem* item) {
        if (!item || m_isLoading) return;
        int nodeId = item->data(Qt::UserRole + 1).toInt();
        if (nodeId > 0) {
            QString name = item->text();
            QString iconKey = item->data(Qt::UserRole + 2).toString();
            QString colorHex = item->data(Qt::UserRole + 3).toString();
            LibraryDao::updateCategoryNode(nodeId, name, iconKey, colorHex);
            QTimer::singleShot(0, this, [this]() {
                loadLibrary();
            });
        }
    });
}

void LibraryPanel::onCategoryClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    int nodeId = index.data(Qt::UserRole + 1).toInt();
    if (nodeId == 0) {
        // "文件夹(N)" Section 节点点击处理
        QPoint clickPos = m_treeView->viewport()->mapFromGlobal(QCursor::pos());
        QRect visualRect = m_treeView->visualRect(index);
        QRect plusRect = LibraryItemDelegate::getAddButtonRect(visualRect);

        if (plusRect.contains(clickPos)) {
            // 触发新建前：若有搜索关键字，自动清空搜索框以保证新建的分类项可见
            if (m_filterEdit && !m_filterEdit->text().isEmpty()) {
                m_filterEdit->clear();
            }

            // 触发新建前：确保分组展开，并更新 AppConfig 持久化值
            if (!m_treeView->isExpanded(index)) {
                m_treeView->setExpanded(index, true);
            }
            AppConfig::instance().setBool("Library/CustomSectionCollapsed", false);

            // 点击右侧 '+' 图标，新建根级自定义分类
            createAndEditCategory(0);
            return;
        }

        // 点击 Header 其它区域，切换展开/折叠状态并持久化记录
        bool isExpanded = m_treeView->isExpanded(index);
        m_treeView->setExpanded(index, !isExpanded);
        AppConfig::instance().setBool("Library/CustomSectionCollapsed", isExpanded);
        return;
    }

    QStringList paths = LibraryService::instance().getCategoryPaths(nodeId);
    emit categoryPathsSelected(paths);
}

void LibraryPanel::filterTreeNodes(const QString& keyword) {
    if (!m_model || !m_treeView) return;

    std::function<bool(QStandardItem*)> filterRecursive = [&](QStandardItem* item) -> bool {
        if (!item) return false;
        int nodeId = item->data(Qt::UserRole + 1).toInt();

        bool matchesSelf = keyword.isEmpty() || item->text().contains(keyword, Qt::CaseInsensitive);
        bool anyChildMatches = false;

        for (int i = 0; i < item->rowCount(); ++i) {
            QStandardItem* child = item->child(i);
            if (filterRecursive(child)) {
                anyChildMatches = true;
                m_treeView->setRowHidden(i, item->index(), false);
            } else {
                m_treeView->setRowHidden(i, item->index(), true);
            }
        }

        if (nodeId == 0) {
            return anyChildMatches;
        }

        bool visible = matchesSelf || anyChildMatches;
        if (!keyword.isEmpty() && visible && item->hasChildren()) {
            m_treeView->expand(item->index());
        }
        return visible;
    };

    for (int i = 0; i < m_model->rowCount(); ++i) {
        QStandardItem* topItem = m_model->item(i);
        if (!topItem) continue;
        int nodeId = topItem->data(Qt::UserRole + 1).toInt();
        if (nodeId < 0) {
            bool visible = keyword.isEmpty() || topItem->text().contains(keyword, Qt::CaseInsensitive);
            m_treeView->setRowHidden(i, QModelIndex(), !visible);
        } else if (nodeId == 0) {
            bool visible = filterRecursive(topItem);
            m_treeView->setRowHidden(i, QModelIndex(), !visible);
            if (!keyword.isEmpty() && visible) {
                m_treeView->expand(topItem->index());
            }
        }
    }
}

void LibraryPanel::onCategoryContextMenu(const QPoint& pos) {
    QModelIndex index = m_treeView->indexAt(pos);

    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    if (!index.isValid()) {
        QAction* newCatAct = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建库分类");
        connect(newCatAct, &QAction::triggered, this, [this]() {
            createAndEditCategory(0);
        });
        menu.exec(m_treeView->viewport()->mapToGlobal(pos));
        return;
    }

    int nodeId = index.data(Qt::UserRole + 1).toInt();
    if (nodeId < 0) {
        // 系统分类禁止弹出修改菜单
        return;
    }

    QString curIconKey = index.data(Qt::UserRole + 2).toString();
    QString curColorHex = index.data(Qt::UserRole + 3).toString();

    if (curIconKey.isEmpty()) curIconKey = "folder_filled";
    if (curColorHex.isEmpty()) curColorHex = "#888888";

    QAction* newSubCatAct = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建子分类");
    connect(newSubCatAct, &QAction::triggered, this, [this, nodeId]() {
        createAndEditCategory(nodeId);
    });

    QAction* presetTagAct = menu.addAction(UiHelper::getIcon("tag_filled", QColor("#9B59B6")), "设置预设标签");
    connect(presetTagAct, &QAction::triggered, this, [this, nodeId]() {
        PresetTagsDialog dlg(nodeId, this, true);
        dlg.exec();
    });

    menu.addSeparator();

    QPersistentModelIndex persistentIdx(index);
    ContextMenuFactory::buildIconPickerMenu(&menu, curIconKey, curColorHex,
        [this, persistentIdx](const QString& iconKey) {
            if (!persistentIdx.isValid()) return;
            QStandardItem* item = m_model ? m_model->itemFromIndex(persistentIdx) : nullptr;
            if (!item) return;

            QString colorHex = item->data(Qt::UserRole + 3).toString();
            if (colorHex.isEmpty()) colorHex = "#888888";

            QIcon newIcon = UiHelper::getIcon(iconKey, QColor(colorHex), 18);
            item->setIcon(newIcon);
            item->setData(iconKey, Qt::UserRole + 2);

            int nodeId = item->data(Qt::UserRole + 1).toInt();
            LibraryDao::updateCategoryNode(nodeId, item->text(), iconKey, colorHex);
        },
        [this, persistentIdx](const QString& hexColor) {
            if (!persistentIdx.isValid()) return;
            QStandardItem* item = m_model ? m_model->itemFromIndex(persistentIdx) : nullptr;
            if (!item) return;

            QString finalColor = hexColor.isEmpty() ? "#888888" : hexColor.toUpper();
            QString iconKey = item->data(Qt::UserRole + 2).toString();
            if (iconKey.isEmpty()) iconKey = "folder_filled";

            QIcon newIcon = UiHelper::getIcon(iconKey, QColor(finalColor), 18);
            item->setIcon(newIcon);
            item->setData(finalColor, Qt::UserRole + 3);

            int nodeId = item->data(Qt::UserRole + 1).toInt();
            LibraryDao::updateCategoryNode(nodeId, item->text(), iconKey, finalColor);
        }
    );

    QAction* renameAct = menu.addAction(UiHelper::getIcon("edit", QColor("#EEEEEE")), "重命名");
    connect(renameAct, &QAction::triggered, this, [this, index]() {
        if (m_treeView) m_treeView->edit(index);
    });

    QAction* removeAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE")), "删除分类");
    connect(removeAct, &QAction::triggered, this, [this, nodeId]() {
        LibraryService::instance().removeCategory(nodeId);
    });

    menu.exec(m_treeView->viewport()->mapToGlobal(pos));
}

void LibraryPanel::onPathsDroppedToCategory(const QStringList& paths, const QModelIndex& target) {
    if (paths.isEmpty()) return;

    // 拖拽到空白处时默认为“未分类” (-2)，拖到具体分类上则为对应的分类 ID
    int nodeId = -2;
    if (target.isValid()) {
        nodeId = target.data(Qt::UserRole + 1).toInt();
    }

    if (nodeId > 0 || nodeId == -2) {
        LibraryService::instance().addPathsToCategory(nodeId, paths);

        // 🚀【索引建库】：无损读取每个项目的 .QuarkMeta.json 并写入中心 library_item_index
        for (const QString& path : paths) {
            QFileInfo fi(path);
            if (fi.isDir()) {
                QDir dir(path);
                QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
                for (const auto& entry : entries) {
                    ItemMeta meta;
                    std::wstring wPath = entry.absoluteFilePath().toStdWString();
                    if (QuarkMetaJsonStore::instance().readItemMeta(wPath, meta)) {
                        LibraryService::instance().indexItem(nodeId, entry.absoluteFilePath(), meta);
                    }
                }
            } else if (fi.isFile()) {
                ItemMeta meta;
                std::wstring wPath = path.toStdWString();
                if (QuarkMetaJsonStore::instance().readItemMeta(wPath, meta)) {
                    LibraryService::instance().indexItem(nodeId, path, meta);
                }
            }
        }

        // 自动将当前分类及其所有祖先分类的预设标签递归合并加至入库文件
        auto categories = LibraryDao::getAllCategories();
        QMap<int, LibraryCategoryRecord> catMap;
        for (const auto& cat : categories) {
            catMap.insert(cat.id, cat);
        }

        QStringList presetTags;
        int currentId = nodeId;
        while (currentId > 0 && catMap.contains(currentId)) {
            const auto& curCat = catMap.value(currentId);
            for (const QString& tag : curCat.presetTags) {
                if (!presetTags.contains(tag)) {
                    presetTags.append(tag);
                }
            }
            currentId = curCat.parentId;
        }

        if (!presetTags.isEmpty()) {
            for (const QString& tag : presetTags) {
                AppCommand cmd;
                cmd.type = AppCommandType::AddTag;
                cmd.targetPaths = paths;
                cmd.params["tag"] = tag;
                CoreEngine::instance().executeCommand(cmd);
            }
            ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已将 %1 个项目关联至当前分类，并自动应用了 %2 个预设标签").arg(paths.size()).arg(presetTags.size()), 2000, QColor("#2ecc71"));
        } else {
            ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已关联 %1 个路径到当前分类").arg(paths.size()), 1500, QColor("#2ecc71"));
        }
    }
}

void LibraryPanel::loadLibrary() {
    if (!m_model) return;
    m_isLoading = true;
    m_model->clear();

    LibraryDao::initTable();

    // 1. 注入 2 个固定系统分类 (全部数据, 未分类 - 未标签已物理移除)
    auto addSystemItem = [this](const QString& name, const QString& iconKey, const QString& colorHex, int sysId) {
        int count = LibraryDao::getCategoryPaths(sysId).size();
        QIcon icon = UiHelper::getIcon(iconKey, QColor(colorHex), 18);
        QStandardItem* item = new QStandardItem(icon, name);
        item->setData("system", TypeRole);
        item->setData(sysId, IdRole);
        item->setData(sysId, Qt::UserRole + 1);
        item->setData(iconKey, Qt::UserRole + 2);
        item->setData(colorHex, Qt::UserRole + 3);
        item->setData(count, Qt::UserRole + 9);
        item->setEditable(false);
        m_model->appendRow(item);
    };

    addSystemItem("全部数据", "all_data", "#3498db", -1);
    addSystemItem("未分类", "uncategorized", "#95a5a6", -2);

    // 2. 构建 "文件夹(N)" 折叠分组 Header 节点
    auto list = LibraryDao::getAllCategories();
    int customCount = list.size();

    QStandardItem* sectionItem = new QStandardItem();
    sectionItem->setData("section", TypeRole);
    sectionItem->setData(0, IdRole);
    sectionItem->setData(0, Qt::UserRole + 1);
    sectionItem->setData(customCount, Qt::UserRole + 9);
    sectionItem->setEditable(false);
    m_model->appendRow(sectionItem);

    // 3. 加载用户自定义分类并挂载在 sectionItem 及其子节点下
    QMap<int, QStandardItem*> itemMap;
    for (const auto& rec : list) {
        int count = LibraryDao::getCategoryPaths(rec.id).size();
        QIcon icon = UiHelper::getIcon(rec.iconKey, QColor(rec.colorHex), 18);
        QStandardItem* item = new QStandardItem(icon, rec.name);
        item->setData("category", TypeRole);
        item->setData(rec.id, IdRole);
        item->setData(rec.id, Qt::UserRole + 1);
        item->setData(rec.iconKey, Qt::UserRole + 2);
        item->setData(rec.colorHex, Qt::UserRole + 3);
        item->setData(count, Qt::UserRole + 9);

        itemMap.insert(rec.id, item);
    }

    for (const auto& rec : list) {
        if (!itemMap.contains(rec.id)) continue;
        QStandardItem* item = itemMap.value(rec.id);

        if (rec.parentId > 0 && itemMap.contains(rec.parentId)) {
            itemMap.value(rec.parentId)->appendRow(item);
        } else {
            sectionItem->appendRow(item);
        }
    }

    // 恢复 "文件夹(N)" 节点折叠/展开持久化状态
    bool sectionCollapsed = AppConfig::instance().getBool("Library/CustomSectionCollapsed", false);
    if (m_treeView) {
        m_treeView->setExpanded(sectionItem->index(), !sectionCollapsed);
    }

    m_isLoading = false;

    // 若底栏存在搜索关键字，重新应用过滤
    if (m_filterEdit && !m_filterEdit->text().trimmed().isEmpty()) {
        filterTreeNodes(m_filterEdit->text().trimmed());
    }

    if (m_pendingEditNodeId > 0 && m_treeView) {
        int targetNodeId = m_pendingEditNodeId;
        QTimer::singleShot(0, this, [this, targetNodeId]() {
            if (m_pendingEditNodeId != targetNodeId) return;
            m_pendingEditNodeId = 0;

            QStandardItem* targetItem = findItemByNodeId(m_model->invisibleRootItem(), targetNodeId);
            if (targetItem && m_treeView) {
                QModelIndex parentIdx = targetItem->parent() ? targetItem->parent()->index() : QModelIndex();
                if (parentIdx.isValid()) {
                    m_treeView->expand(parentIdx);
                }
                m_treeView->setCurrentIndex(targetItem->index());
                m_treeView->edit(targetItem->index());
            }
        });
    }
}

QStandardItem* LibraryPanel::findItemByNodeId(QStandardItem* parent, int nodeId) {
    if (!parent) return nullptr;
    for (int i = 0; i < parent->rowCount(); ++i) {
        QStandardItem* child = parent->child(i);
        if (!child) continue;
        if (child->data(Qt::UserRole + 1).toInt() == nodeId) {
            return child;
        }
        QStandardItem* found = findItemByNodeId(child, nodeId);
        if (found) return found;
    }
    return nullptr;
}

void LibraryPanel::createAndEditCategory(int parentId) {
    int newId = LibraryService::instance().createCategory("新建分类", parentId);
    if (newId > 0) {
        m_pendingEditNodeId = newId;
        loadLibrary();
    }
}

} // namespace QuarkMeta
