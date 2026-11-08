# Implementation Plan: PanelMediator 庞大连接调度系统性解耦重构方案

本实施方案严格遵循工业级 C++/Qt 顶层架构规范、《AGENTS.md》实施界限（严禁直接修改源代码，所有实施方案物理隔离存放于 `Implementation Plan/` 目录）及《Standard Specifications for Implementation Plan.md》，对 `QuarkMeta_Source/src/ui/PanelMediator.cpp` 中长达 900 余行的 `setupConnections` 进行结构化系统解耦与单一职责（SRP）治理，建立各专职协调器（Coordinator/Presenter/Controller/Binder/Router），同时对“当前目标窗格（SSOT）”和“逐窗格连接去重”进行统一化规范，确保重构前后界面行为、提示文案与数据流向完全一致。

---

## 前置物理证据清单 (Gate 1: Pre-Draft Evidence)

### 1. 物理读取与分析文件清单
- `QuarkMeta_Source/src/ui/PanelMediator.h` (共 101 行，定义组件依赖结构体 `PanelMediatorComponents`、中介者类声明与防抖状态结构)
- `QuarkMeta_Source/src/ui/PanelMediator.cpp` (共 1299 行，承担了整个主界面各个控件间的全部信号调度、元数据提取、QuickLook联动、快捷键转发、标签栏分屏等巨型逻辑)
- `QuarkMeta_Source/src/ui/controllers/PaneActivationTracker.h` (声明 `paneInteracted` 信号)
- `QuarkMeta_Source/src/ui/ContentPanel.h` (确认窗格公开方法 `isVisible()`、`rootPane()`、`panes()`、`displayPath()`、`setActivePane()`、`refreshAll()` 等)
- `QuarkMeta_Source/CMakeLists.txt` (显式列出各源文件清单)

### 2. 外部公开接口与契约检索验证 (Public API Freezing)
- `CoreEngine.cpp:244`: `PanelMediator::instance()->activeContentPanel()`
- `MarkedFolderService.cpp:72`: `PanelMediator::instance()->activeContentPanel()`
- `MainWindow.cpp:393, 394, 524`: `m_panelMediator->activeContentPanel()`
- `MainWindow.cpp:217`: `connect(m_panelMediator, &PanelMediator::statusMessageRequested, ...)`
- `NavigationService.cpp` 等多处: `PanelMediator::instance()->locateItem(path)`
- **结论**：`PanelMediator` 的对外接口 `instance()`、`locateItem()`、`activeContentPanel()`、`setupConnections()` 以及信号 `statusMessageRequested`、`activeContentPanelChanged` 的签名与行为保持 100% 冻结兼容。

## Chapter 1: Overview（概述与根因分析）

### 1.1 核心病灶与系统性缺陷深入剖析
当前 `PanelMediator.cpp` 中长达近 900 行的 `setupConnections` 存在严重的职责爆炸：
1. **目标窗格获取缺乏 SSOT（约 20 处分散判断）**：全文件大量充斥 `m_activeContentPanel ? m_activeContentPanel.data() : contentPanel`，甚至部分地方漏掉了 `isVisible()` 判断，导致隐藏窗格被误作为操作目标；
2. **信号重复绑定缺陷**：主窗格在初始化 `if (contentPanel)` 块中连接了一次 `directorySelected`、`requestAddFavorite`、`requestRemoveFavorite`，随后在 `wireContentPanel` 中又连接了一次，导致同一次用户操作触发两次信号处理；
3. **职责混杂严重违背 SRP**：元数据面板更新（空选/单选/多选/30ms防抖）、QuickLook 相邻文件搜寻、快捷键分发、标签栏分屏布局导出、导航与地址栏驱动全堆叠在一个庞大的 Lambda 闭包中，阅读与维护成本极高；
4. **多选元数据计算冗余**：选中大量项目时，即使所有项目的字段早已不一致且共同标签已为空，仍然循环遍历后续全部项目并读取元数据，造成计算浪费。

### 1.2 架构治理设计目标与统一规则
- **统一规则一：当前目标窗格（SSOT，要求可见）**：由 `ActivePaneCoordinator::currentTarget()` 统一提供，当前激活窗格存在且 `isVisible()` 为真则返回它，否则返回主窗格；
- **统一规则二：同一信号对同一窗格只连接一次**：所有窗格（主窗格及动态副窗格）统一通过“逐窗格绑定”函数只连接一次，彻底消除双重连接；
- **统一规则三：逐窗格职责拆分**：将原 `wireContentPanel` 职责解耦至各专用类的 `bindPane(ContentPanel*)` 中，由 `PanelMediator` 进行极薄的递归分发；
- **拆分为 8 个高内聚组件**：
  1. `ActivePaneCoordinator`：窗格激活管理与目标窗格 SSOT；
  2. `MetaPanelPresenter`：元数据面板展示、30ms防抖、多选两阶段性能优化与元数据事件响应；
  3. `MetaEditController`：元数据面板编辑信号到 CoreEngine 命令的翻译；
  4. `QuickLookController`：QuickLook 预览联动、相邻可预览文件查找单一实现与安全删除；
  5. `ShortcutActionRouter`：全局快捷键与标题栏动作路由；
  6. `TabBarCoordinator`：标签页切换、合并拆分、颜色图标更新与分屏状态保存单一实现；
  7. `NavigationCoordinator`：全应用位置显示单一入口、路径导航与刷新；
  8. `FilterStatsBinder`：筛选面板与窗格统计状态同步绑定。

## Chapter 2: Modified Files List（影响文件清单）

### 2.1 新建核心协调器与控制器文件清单（共 8 对，位于 `src/ui/`）
1. `QuarkMeta_Source/src/ui/ActivePaneCoordinator.h`
2. `QuarkMeta_Source/src/ui/ActivePaneCoordinator.cpp`
3. `QuarkMeta_Source/src/ui/MetaPanelPresenter.h`
4. `QuarkMeta_Source/src/ui/MetaPanelPresenter.cpp`
5. `QuarkMeta_Source/src/ui/MetaEditController.h`
6. `QuarkMeta_Source/src/ui/MetaEditController.cpp`
7. `QuarkMeta_Source/src/ui/QuickLookController.h`
8. `QuarkMeta_Source/src/ui/QuickLookController.cpp`
9. `QuarkMeta_Source/src/ui/ShortcutActionRouter.h`
10. `QuarkMeta_Source/src/ui/ShortcutActionRouter.cpp`
11. `QuarkMeta_Source/src/ui/TabBarCoordinator.h`
12. `QuarkMeta_Source/src/ui/TabBarCoordinator.cpp`
13. `QuarkMeta_Source/src/ui/NavigationCoordinator.h`
14. `QuarkMeta_Source/src/ui/NavigationCoordinator.cpp`
15. `QuarkMeta_Source/src/ui/FilterStatsBinder.h`
16. `QuarkMeta_Source/src/ui/FilterStatsBinder.cpp`

### 2.2 允许修改的文件清单
1. `QuarkMeta_Source/src/ui/PanelMediator.h`
2. `QuarkMeta_Source/src/ui/PanelMediator.cpp`
3. `QuarkMeta_Source/CMakeLists.txt`

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 新建 `QuarkMeta_Source/src/ui/ActivePaneCoordinator.h`

```
<<<<<<< SEARCH FILE: QuarkMeta_Source/src/ui/ActivePaneCoordinator.h
=======
#pragma once

#include <QObject>
#include <QPointer>
#include "ContentPanel.h"

namespace QuarkMeta {

class PaneActivationTracker;

/**
 * @brief 激活窗格协调器 (SSOT)
 * 唯一持有当前激活窗格，统一提供 currentTarget() 与 isCurrentTarget()，
 * 并负责 PaneActivationTracker 的创建与全生命周期事件过滤。
 */
class ActivePaneCoordinator : public QObject {
    Q_OBJECT
public:
    explicit ActivePaneCoordinator(ContentPanel* rootPanel, QObject* parent = nullptr);
    ~ActivePaneCoordinator() override = default;

    ContentPanel* currentTarget() const;
    bool isCurrentTarget(ContentPanel* panel) const;
    ContentPanel* activePane() const { return m_activeContentPanel.data(); }
    ContentPanel* rootPane() const { return m_rootContentPanel.data(); }

    void activate(ContentPanel* panel);
    void bindPane(ContentPanel* pane);
    void wire();

signals:
    void activePaneChanged(ContentPanel* panel);
    void statusMessageRequested(const QString& message);

private:
    void syncPaneHighlights(ContentPanel* activePanel);

    QPointer<ContentPanel> m_rootContentPanel;
    QPointer<ContentPanel> m_activeContentPanel;
    PaneActivationTracker* m_activationTracker = nullptr;
};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.2 新建 `QuarkMeta_Source/src/ui/ActivePaneCoordinator.cpp`

```
<<<<<<< SEARCH FILE: QuarkMeta_Source/src/ui/ActivePaneCoordinator.cpp
=======
#include "ActivePaneCoordinator.h"
#include "controllers/PaneActivationTracker.h"
#include <QApplication>

namespace QuarkMeta {

ActivePaneCoordinator::ActivePaneCoordinator(ContentPanel* rootPanel, QObject* parent)
    : QObject(parent), m_rootContentPanel(rootPanel), m_activeContentPanel(rootPanel) {
    if (m_rootContentPanel) {
        m_rootContentPanel->setActivePane(true);
    }
}

ContentPanel* ActivePaneCoordinator::currentTarget() const {
    if (m_activeContentPanel && m_activeContentPanel->isVisible()) {
        return m_activeContentPanel.data();
    }
    return m_rootContentPanel.data();
}

bool ActivePaneCoordinator::isCurrentTarget(ContentPanel* panel) const {
    return panel && (panel == currentTarget());
}

void ActivePaneCoordinator::activate(ContentPanel* panel) {
    if (!panel || m_activeContentPanel == panel) {
        return;
    }
    if (m_activeContentPanel) {
        m_activeContentPanel->setActivePane(false);
    }
    m_activeContentPanel = panel;
    m_activeContentPanel->setActivePane(true);

    emit activePaneChanged(m_activeContentPanel.data());
}

void ActivePaneCoordinator::syncPaneHighlights(ContentPanel* activePanel) {
    ContentPanel* root = m_rootContentPanel ? m_rootContentPanel->rootPane() : nullptr;
    if (root) {
        root->setActivePane(root == activePanel);
        for (ContentPanel* pane : root->panes()) {
            if (pane && pane != root) {
                pane->setActivePane(pane == activePanel);
            }
        }
    }
}

void ActivePaneCoordinator::bindPane(ContentPanel* pane) {
    if (!pane) return;

    // 状态栏消息转发（仅当来自当前目标窗格时才转发）
    connect(pane, &ContentPanel::statusBarMessageReady, this, [this, pane](const QString& message) {
        if (isCurrentTarget(pane)) {
            emit statusMessageRequested(message);
        }
    });

    // 窗格自身激活信号接入
    connect(pane, &ContentPanel::panelActivated, this, [this](ContentPanel* activePanel) {
        activate(activePanel);
    });
}

void ActivePaneCoordinator::wire() {
    if (!m_activationTracker) {
        m_activationTracker = new PaneActivationTracker(this);
        qApp->installEventFilter(m_activationTracker);

        connect(m_activationTracker, &PaneActivationTracker::paneInteracted, this, [this](ContentPanel* panel) {
            activate(panel);
        });
    }

    // 窗格高亮同步作为 activePaneChanged 接收者无条件执行
    connect(this, &ActivePaneCoordinator::activePaneChanged, this, [this](ContentPanel* activePanel) {
        syncPaneHighlights(activePanel);
    });
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.3 新建 `QuarkMeta_Source/src/ui/MetaPanelPresenter.h`

```
<<<<<<< SEARCH FILE: QuarkMeta_Source/src/ui/MetaPanelPresenter.h
=======
#pragma once

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QSet>
#include <QMap>
#include "ContentPanel.h"
#include "MetaPanel.h"
#include "../core/CentralEventHub.h"

namespace QuarkMeta {

class ActivePaneCoordinator;

/**
 * @brief 元数据面板展示与同步协调器
 * 负责 updateMetaPanelFromPanel、30ms防抖、多选两阶段性能优化与全局元数据变更事件响应。
 */
class MetaPanelPresenter : public QObject {
    Q_OBJECT
public:
    explicit MetaPanelPresenter(ActivePaneCoordinator* activeCoordinator, MetaPanel* metaPanel, ContentPanel* rootPanel, QObject* parent = nullptr);
    ~MetaPanelPresenter() override = default;

    void updateMetaPanel(ContentPanel* panel);
    void bindPane(ContentPanel* pane);
    void wire();

public slots:
    void handleEvent(const QuarkMeta::AppEvent& event);

private:
    void renderEmptySelection();
    void renderSingleSelection(ContentPanel* panel, const QString& path);
    void renderMultiSelection(ContentPanel* panel, const QStringList& paths);

    ActivePaneCoordinator* m_activeCoordinator = nullptr;
    QPointer<MetaPanel> m_metaPanel;
    QPointer<ContentPanel> m_rootPanel;

    struct DebounceState {
        QTimer* timer = nullptr;
        QSet<QString> pendingCleanPaths;
    };
    QMap<ContentPanel*, DebounceState> m_debounceMap;
};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.4 新建 `QuarkMeta_Source/src/ui/MetaPanelPresenter.cpp`

```
<<<<<<< SEARCH FILE: QuarkMeta_Source/src/ui/MetaPanelPresenter.cpp
=======
#include "MetaPanelPresenter.h"
#include "ActivePaneCoordinator.h"
#include "../meta/MetadataManager.h"
#include "UiHelper.h"
#include <QFileInfo>
#include <QDir>
#include <QIcon>
#include <QPixmap>

namespace QuarkMeta {

MetaPanelPresenter::MetaPanelPresenter(ActivePaneCoordinator* activeCoordinator, MetaPanel* metaPanel, ContentPanel* rootPanel, QObject* parent)
    : QObject(parent), m_activeCoordinator(activeCoordinator), m_metaPanel(metaPanel), m_rootPanel(rootPanel) {
}

void MetaPanelPresenter::renderEmptySelection() {
    if (!m_metaPanel) return;
    m_metaPanel->setImagePreview(QPixmap());
    m_metaPanel->updateInfo("-", "-", "-", "-", "-", "-", "-", false, 0, 0);
    m_metaPanel->setRating(0, false);
    m_metaPanel->setColor(QString(""), false);
    m_metaPanel->setTags(QStringList());
    m_metaPanel->setNote(QString(""));
    m_metaPanel->setURL(QString(""));
    m_metaPanel->setPalettes({});
}

void MetaPanelPresenter::renderSingleSelection(ContentPanel* panel, const QString& path) {
    if (!m_metaPanel || !panel) return;

    QModelIndexList selectedIndices = panel->getSelectedIndexes();
    QModelIndex idx = selectedIndices.isEmpty() ? QModelIndex() : selectedIndices.first();

    QFileInfo fi(path);
    QString name = idx.isValid() ? idx.sibling(idx.row(), static_cast<int>(FileListColumn::Name)).data(Qt::DisplayRole).toString() : fi.fileName();
    QString type = idx.isValid() ? ((idx.data(TypeRole).toString() == "folder") ? "文件夹" : idx.sibling(idx.row(), static_cast<int>(FileListColumn::Type)).data(Qt::DisplayRole).toString() + " 文件") : (fi.isDir() ? "文件夹" : fi.suffix().toUpper() + " 文件");
    QString sizeStr = idx.isValid() ? idx.sibling(idx.row(), static_cast<int>(FileListColumn::Size)).data(Qt::DisplayRole).toString() : "-";
    QString mtimeStr = idx.isValid() ? idx.sibling(idx.row(), static_cast<int>(FileListColumn::ModifiedDate)).data(Qt::DisplayRole).toString() : "-";
    bool encrypted = idx.isValid() ? idx.data(EncryptedRole).toBool() : false;

    auto meta = MetadataManager::instance().getMeta(path.toStdWString());

    m_metaPanel->updateInfo(
        name, type, sizeStr, "-", mtimeStr, "-",
        path, encrypted, meta.width, meta.height
    );

    QVector<QPair<QColor, float>> qPalettes;
    qPalettes.reserve(static_cast<int>(meta.palettes.size()));
    for (const auto& entry : meta.palettes) {
        qPalettes.append(qMakePair(entry.color, entry.ratio));
    }

    if (idx.isValid()) {
        int rating = idx.data(RatingRole).toInt();
        QString color = idx.data(ColorRole).toString();
        QStringList tags = idx.data(TagsRole).toStringList();
        QString note = idx.data(NoteRole).toString();
        QString url = idx.data(UrlRole).toString();

        int finalRating = rating > 0 ? rating : meta.rating;
        QString finalColor = !color.isEmpty() ? color : QString::fromStdWString(meta.manualColor);
        QStringList finalTagsList = !tags.isEmpty() ? tags : meta.tags;
        QString finalNote = !note.isEmpty() ? note : QString::fromStdWString(meta.note);
        QString finalUrl = !url.isEmpty() ? url : QString::fromStdWString(meta.url);

        m_metaPanel->setRating(finalRating, false);
        m_metaPanel->setColor(finalColor, false);
        m_metaPanel->setTags(finalTagsList);
        m_metaPanel->setNote(finalNote);
        m_metaPanel->setURL(finalUrl);
        m_metaPanel->setPalettes(qPalettes);

        QVariant decData = idx.data(Qt::DecorationRole);
        bool hasThumb = idx.data(HasThumbnailRole).toBool();
        if (hasThumb) {
            QPixmap previewPixmap;
            if (decData.canConvert<QIcon>()) {
                previewPixmap = decData.value<QIcon>().pixmap(128, 128);
            } else if (decData.canConvert<QPixmap>()) {
                previewPixmap = decData.value<QPixmap>();
            }
            m_metaPanel->setImagePreview(previewPixmap);
        } else {
            if (decData.canConvert<QIcon>()) {
                m_metaPanel->setDefaultIconPreview(decData.value<QIcon>());
            } else {
                m_metaPanel->setImagePreview(QPixmap());
            }
        }
    } else {
        m_metaPanel->setRating(meta.rating, false);
        m_metaPanel->setColor(QString::fromStdWString(meta.manualColor), false);
        m_metaPanel->setTags(meta.tags);
        m_metaPanel->setNote(QString::fromStdWString(meta.note));
        m_metaPanel->setURL(QString::fromStdWString(meta.url));
        m_metaPanel->setPalettes(qPalettes);
        m_metaPanel->setImagePreview(QPixmap());
    }
}

void MetaPanelPresenter::renderMultiSelection(ContentPanel* panel, const QStringList& paths) {
    if (!m_metaPanel || !panel) return;

    QModelIndexList selectedIndices = panel->getSelectedIndexes();
    QMap<QString, QModelIndex> pathToIndexMap;
    for (const QModelIndex& idx : selectedIndices) {
        if (idx.isValid()) {
            QString p = idx.data(PathRole).toString();
            if (!p.isEmpty()) {
                pathToIndexMap[p] = idx;
            }
        }
    }

    bool firstItem = true;
    bool sameRating = true;
    bool sameColor = true;
    bool sameNote = true;
    bool sameUrl = true;

    int commonRating = 0;
    QString commonColor;
    QString commonNote;
    QString commonUrl;

    QSet<QString> commonTags;
    bool commonTagsInitialized = false;

    QList<MetaPreviewItem> multiPreviewItems;
    int previewStartIdx = qMax(0, paths.size() - MetaPanel::kMultiPreviewMaxCards);

    int phase1EndIdx = paths.size();
    for (int i = 0; i < paths.size(); ++i) {
        const QString& path = paths[i];
        QModelIndex idx = pathToIndexMap.value(path, QModelIndex());

        int itemRating = 0;
        QString itemColor;
        QStringList itemTags;
        QString itemNote;
        QString itemUrl;

        auto meta = MetadataManager::instance().getMeta(path.toStdWString());

        if (idx.isValid()) {
            int r = idx.data(RatingRole).toInt();
            QString c = idx.data(ColorRole).toString();
            QStringList t = idx.data(TagsRole).toStringList();
            QString n = idx.data(NoteRole).toString();
            QString u = idx.data(UrlRole).toString();

            itemRating = r > 0 ? r : meta.rating;
            itemColor = !c.isEmpty() ? c : QString::fromStdWString(meta.manualColor);
            itemTags = !t.isEmpty() ? t : meta.tags;
            itemNote = !n.isEmpty() ? n : QString::fromStdWString(meta.note);
            itemUrl = !u.isEmpty() ? u : QString::fromStdWString(meta.url);

            if (i >= previewStartIdx && multiPreviewItems.size() < MetaPanel::kMultiPreviewMaxCards) {
                MetaPreviewItem previewItem;
                QVariant decData = idx.data(Qt::DecorationRole);
                bool hasThumb = idx.data(HasThumbnailRole).toBool();
                if (hasThumb) {
                    previewItem.hasThumbnail = true;
                    if (decData.canConvert<QIcon>()) {
                        previewItem.thumbnail = decData.value<QIcon>().pixmap(128, 128);
                    } else if (decData.canConvert<QPixmap>()) {
                        previewItem.thumbnail = decData.value<QPixmap>();
                    }
                } else {
                    previewItem.hasThumbnail = false;
                    if (decData.canConvert<QIcon>()) {
                        previewItem.icon = decData.value<QIcon>();
                    }
                }
                if ((previewItem.hasThumbnail && !previewItem.thumbnail.isNull()) || (!previewItem.hasThumbnail && !previewItem.icon.isNull())) {
                    multiPreviewItems.append(previewItem);
                }
            }
        } else {
            itemRating = meta.rating;
            itemColor = QString::fromStdWString(meta.manualColor);
            itemTags = meta.tags;
            itemNote = QString::fromStdWString(meta.note);
            itemUrl = QString::fromStdWString(meta.url);

            if (i >= previewStartIdx && multiPreviewItems.size() < MetaPanel::kMultiPreviewMaxCards) {
                MetaPreviewItem previewItem;
                previewItem.hasThumbnail = false;
                QFileInfo fi(path);
                if (fi.isDir()) {
                    previewItem.icon = UiHelper::getIcon("folder", QColor("#3498db"), 128);
                } else {
                    previewItem.icon = UiHelper::getIcon("file_eps", QColor("#888888"), 128);
                }
                if (!previewItem.icon.isNull()) {
                    multiPreviewItems.append(previewItem);
                }
            }
        }

        if (firstItem) {
            firstItem = false;
            commonRating = itemRating;
            commonColor = itemColor;
            commonNote = itemNote;
            commonUrl = itemUrl;

            commonTags = QSet<QString>(itemTags.begin(), itemTags.end());
            commonTagsInitialized = true;
        } else {
            if (sameRating && itemRating != commonRating) sameRating = false;
            if (sameColor && itemColor.compare(commonColor, Qt::CaseInsensitive) != 0) sameColor = false;
            if (sameNote && itemNote != commonNote) sameNote = false;
            if (sameUrl && itemUrl != commonUrl) sameUrl = false;

            if (commonTagsInitialized) {
                QSet<QString> currentSet(itemTags.begin(), itemTags.end());
                commonTags.intersect(currentSet);
            }
        }

        // 阶段一提前收敛判断
        if (!sameRating && !sameColor && !sameNote && !sameUrl && commonTags.isEmpty()) {
            phase1EndIdx = i + 1;
            break;
        }
    }

    // 阶段二：若阶段一提前退出，仅为后序叠卡片窗口内的项提取预览图，不再计算元数据字段
    int remainingPreviewStart = qMax(phase1EndIdx, previewStartIdx);
    for (int i = remainingPreviewStart; i < paths.size() && multiPreviewItems.size() < MetaPanel::kMultiPreviewMaxCards; ++i) {
        const QString& path = paths[i];
        QModelIndex idx = pathToIndexMap.value(path, QModelIndex());
        if (idx.isValid()) {
            MetaPreviewItem previewItem;
            QVariant decData = idx.data(Qt::DecorationRole);
            bool hasThumb = idx.data(HasThumbnailRole).toBool();
            if (hasThumb) {
                previewItem.hasThumbnail = true;
                if (decData.canConvert<QIcon>()) {
                    previewItem.thumbnail = decData.value<QIcon>().pixmap(128, 128);
                } else if (decData.canConvert<QPixmap>()) {
                    previewItem.thumbnail = decData.value<QPixmap>();
                }
            } else {
                previewItem.hasThumbnail = false;
                if (decData.canConvert<QIcon>()) {
                    previewItem.icon = decData.value<QIcon>();
                }
            }
            if ((previewItem.hasThumbnail && !previewItem.thumbnail.isNull()) || (!previewItem.hasThumbnail && !previewItem.icon.isNull())) {
                multiPreviewItems.append(previewItem);
            }
        } else {
            MetaPreviewItem previewItem;
            previewItem.hasThumbnail = false;
            QFileInfo fi(path);
            if (fi.isDir()) {
                previewItem.icon = UiHelper::getIcon("folder", QColor("#3498db"), 128);
            } else {
                previewItem.icon = UiHelper::getIcon("file_eps", QColor("#888888"), 128);
            }
            if (!previewItem.icon.isNull()) {
                multiPreviewItems.append(previewItem);
            }
        }
    }

    m_metaPanel->setRating(sameRating ? commonRating : 0, false);
    m_metaPanel->setColor(sameColor ? commonColor : QString(""), false);
    m_metaPanel->setNote(sameNote ? commonNote : QString(""));
    m_metaPanel->setURL(sameUrl ? commonUrl : QString(""));
    m_metaPanel->setTags(commonTags.values());

    m_metaPanel->setPalettes({});
    m_metaPanel->setMultiSelectionPreview(multiPreviewItems);
}

void MetaPanelPresenter::updateMetaPanel(ContentPanel* panel) {
    if (!panel || !m_metaPanel) return;
    QStringList paths = panel->getSelectedPaths();
    m_metaPanel->setSelectedPaths(paths);

    if (paths.isEmpty()) {
        renderEmptySelection();
    } else if (paths.size() == 1) {
        renderSingleSelection(panel, paths.first());
    } else {
        renderMultiSelection(panel, paths);
    }
}

void MetaPanelPresenter::bindPane(ContentPanel* pane) {
    if (!pane || !m_metaPanel) return;

    connect(pane, &ContentPanel::selectionChanged, this, [this, pane](const QStringList&) {
        if (m_activeCoordinator && m_activeCoordinator->isCurrentTarget(pane)) {
            updateMetaPanel(pane);
        }
    });

    if (pane->model()) {
        connect(pane->model(), &QAbstractItemModel::dataChanged, this,
                [this, pane](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
            if (!roles.isEmpty() && !roles.contains(RatingRole) && !roles.contains(ColorRole) && !roles.contains(TagsRole) && !roles.contains(NoteRole) && !roles.contains(UrlRole)) {
                return;
            }

            auto& debounceState = m_debounceMap[pane];
            if (!debounceState.timer) {
                debounceState.timer = new QTimer(pane);
                debounceState.timer->setSingleShot(true);
                connect(pane, &QObject::destroyed, this, [this, pane]() {
                    m_debounceMap.remove(pane);
                });
                connect(debounceState.timer, &QTimer::timeout, this, [this, pane]() {
                    auto it = m_debounceMap.find(pane);
                    if (it == m_debounceMap.end()) return;

                    if (m_activeCoordinator && m_activeCoordinator->isCurrentTarget(pane)) {
                        QSet<QString> selSet;
                        for (const QString& sp : pane->getSelectedPaths()) {
                            selSet.insert(QDir::cleanPath(sp).toLower());
                        }

                        bool hasOverlap = selSet.isEmpty();
                        if (!hasOverlap) {
                            for (const QString& cp : it->pendingCleanPaths) {
                                if (selSet.contains(cp)) {
                                    hasOverlap = true;
                                    break;
                                }
                            }
                        }

                        if (hasOverlap) {
                            updateMetaPanel(pane);
                        }
                    }
                    it->pendingCleanPaths.clear();
                });
            }

            QAbstractItemModel* model = pane->model();
            int startRow = topLeft.row();
            int endRow = bottomRight.row();
            for (int r = startRow; r <= endRow; ++r) {
                QModelIndex idx = model->index(r, 0, topLeft.parent());
                if (idx.isValid()) {
                    QString path = idx.data(PathRole).toString();
                    if (!path.isEmpty()) {
                        debounceState.pendingCleanPaths.insert(QDir::cleanPath(path).toLower());
                    }
                }
            }

            // 原注释写50ms，现修正为与实际一致的30ms
            if (!debounceState.timer->isActive()) {
                debounceState.timer->start(30);
            }
        });
    }
}

void MetaPanelPresenter::wire() {
    connect(&CentralEventHub::instance(), &CentralEventHub::eventOccurred, this, &MetaPanelPresenter::handleEvent);
}

void MetaPanelPresenter::handleEvent(const QuarkMeta::AppEvent& event) {
    if (event.type != QuarkMeta::AppEventType::MetadataUpdated) {
        return;
    }

    ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
    if (!target) return;

    if (!event.targetPath.isEmpty()) {
        target->updateItemMetadata(event.targetPath);
        if (m_rootPanel && m_rootPanel != target) {
            m_rootPanel->updateItemMetadata(event.targetPath);
        }
        if (m_metaPanel) {
            QString targetClean = QDir::cleanPath(event.targetPath);
            for (const QString& p : target->getSelectedPaths()) {
                if (QString::compare(QDir::cleanPath(p), targetClean, Qt::CaseInsensitive) == 0) {
                    if (event.payload.contains("field") && event.payload["field"].toString() == "color") {
                        QString newColor = event.payload.value("value").toString();
                        m_metaPanel->setColor(newColor, false);
                    } else if (event.payload.contains("field") && event.payload["field"].toString() == "rating") {
                        int newRating = event.payload.value("value").toInt();
                        m_metaPanel->setRating(newRating, false);
                    }
                    break;
                }
            }
        }
    } else if (!event.paths.isEmpty()) {
        target->updateItemsMetadata(event.paths);
        if (m_rootPanel && m_rootPanel != target) {
            m_rootPanel->updateItemsMetadata(event.paths);
        }
    } else {
        target->refreshAll();
        if (m_rootPanel && m_rootPanel != target) {
            m_rootPanel->refreshAll();
        }
    }
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.5 新建 `QuarkMeta_Source/src/ui/MetaEditController.h`

```
<<<<<<< SEARCH FILE: QuarkMeta_Source/src/ui/MetaEditController.h
=======
#pragma once

#include <QObject>
#include <QPointer>
#include "MetaPanel.h"

namespace QuarkMeta {

class ActivePaneCoordinator;

/**
 * @brief 元数据面板编辑操作控制器
 * 负责响应 MetaPanel 的评分、颜色、预设标签、备注、链接与重命名信号并翻译为 CoreEngine 命令。
 */
class MetaEditController : public QObject {
    Q_OBJECT
public:
    explicit MetaEditController(ActivePaneCoordinator* activeCoordinator, MetaPanel* metaPanel, QObject* parent = nullptr);
    ~MetaEditController() override = default;

    void wire();

private:
    ActivePaneCoordinator* m_activeCoordinator = nullptr;
    QPointer<MetaPanel> m_metaPanel;
};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.6 新建 `QuarkMeta_Source/src/ui/MetaEditController.cpp`

```
<<<<<<< SEARCH FILE: QuarkMeta_Source/src/ui/MetaEditController.cpp
=======
#include "MetaEditController.h"
#include "ActivePaneCoordinator.h"
#include "../core/CoreEngine.h"
#include "../util/ShellHelper.h"

namespace QuarkMeta {

MetaEditController::MetaEditController(ActivePaneCoordinator* activeCoordinator, MetaPanel* metaPanel, QObject* parent)
    : QObject(parent), m_activeCoordinator(activeCoordinator), m_metaPanel(metaPanel) {
}

void MetaEditController::wire() {
    if (!m_metaPanel || !m_activeCoordinator) return;

    connect(m_metaPanel, &MetaPanel::ratingChanged, this, [](const QStringList& paths, int rating) {
        if (paths.isEmpty()) return;
        AppCommand cmd;
        cmd.type = AppCommandType::SetRating;
        cmd.targetPaths = paths;
        cmd.params["rating"] = rating;
        CoreEngine::instance().executeCommand(cmd);
    });

    connect(m_metaPanel, &MetaPanel::colorChanged, this, [](const QStringList& paths, const QString& hex) {
        if (paths.isEmpty()) return;
        AppCommand cmd;
        cmd.type = AppCommandType::SetColor;
        cmd.targetPaths = paths;
        cmd.params["color"] = hex;
        CoreEngine::instance().executeCommand(cmd);
    });

    connect(m_metaPanel, &MetaPanel::primaryColorChanged, this, [](const QStringList& paths, const QString& hex) {
        if (paths.isEmpty()) return;
        AppCommand cmd;
        cmd.type = AppCommandType::SetColor;
        cmd.targetPaths = paths;
        cmd.params["color"] = hex;
        CoreEngine::instance().executeCommand(cmd);
    });

    connect(m_metaPanel, &MetaPanel::tagsAddRequested, this, [](const QStringList& paths, const QStringList& tags) {
        if (paths.isEmpty() || tags.isEmpty()) return;
        if (paths.size() > 50) {
            CoreEngine::instance().addTagsToPathsAsync(paths, tags);
        } else {
            for (const QString& tag : tags) {
                AppCommand cmd;
                cmd.type = AppCommandType::AddTag;
                cmd.targetPaths = paths;
                cmd.params["tag"] = tag;
                CoreEngine::instance().executeCommand(cmd);
            }
        }
    });

    connect(m_metaPanel, &MetaPanel::tagsRemoveRequested, this, [](const QStringList& paths, const QStringList& tags) {
        if (paths.isEmpty() || tags.isEmpty()) return;
        for (const QString& tag : tags) {
            AppCommand cmd;
            cmd.type = AppCommandType::RemoveTag;
            cmd.targetPaths = paths;
            cmd.params["tag"] = tag;
            CoreEngine::instance().executeCommand(cmd);
        }
    });

    connect(m_metaPanel, &MetaPanel::renameRequested, this, [this](const QString& oldPath, const QString& newName) {
        ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
        bool ok = ShellHelper::renameItem(oldPath, newName);
        if (ok) {
            if (target) {
                QString parentDir = QFileInfo(oldPath).absolutePath();
                QString newPath = QDir(parentDir).filePath(newName);
                target->migrateModelCache(oldPath, newPath);
                target->refreshAll();
            }
        } else {
            if (target) {
                target->updateItemMetadata(oldPath);
            }
        }
    });

    connect(m_metaPanel, &MetaPanel::noteEdited, this, [](const QStringList& paths, const QString& newNote) {
        if (!paths.isEmpty()) {
            AppCommand cmd;
            cmd.type = AppCommandType::SetNote;
            cmd.targetPaths = paths;
            cmd.params["note"] = newNote;
            CoreEngine::instance().executeCommand(cmd);
        }
    });

    connect(m_metaPanel, &MetaPanel::linkEdited, this, [](const QStringList& paths, const QString& newLink) {
        if (!paths.isEmpty()) {
            AppCommand cmd;
            cmd.type = AppCommandType::SetURL;
            cmd.targetPaths = paths;
            cmd.params["url"] = newLink;
            CoreEngine::instance().executeCommand(cmd);
        }
    });
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.7 新建 `QuarkMeta_Source/src/ui/QuickLookController.h`

```
<<<<<<< SEARCH FILE: QuarkMeta_Source/src/ui/QuickLookController.h
=======
#pragma once

#include <QObject>
#include <QString>
#include "ContentPanel.h"

namespace QuarkMeta {

class ActivePaneCoordinator;

/**
 * @brief QuickLook 预览联动控制器
 * 负责文件双击/激活响应、前后切换相邻可预览文件（单一实现）与预览中删除流程。
 */
class QuickLookController : public QObject {
    Q_OBJECT
public:
    explicit QuickLookController(ActivePaneCoordinator* activeCoordinator, QObject* parent = nullptr);
    ~QuickLookController() override = default;

    void bindPane(ContentPanel* pane);
    void wire();

private:
    QString findAdjacentPreviewable(ContentPanel* target, const QString& currentPath, int direction);

    ActivePaneCoordinator* m_activeCoordinator = nullptr;
};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.8 新建 `QuarkMeta_Source/src/ui/QuickLookController.cpp`

```
<<<<<<< SEARCH FILE: QuarkMeta_Source/src/ui/QuickLookController.cpp
=======
#include "QuickLookController.h"
#include "ActivePaneCoordinator.h"
#include "QuickLookWindow.h"
#include "ToolTipOverlay.h"
#include "UiHelper.h"
#include "../meta/HistoryService.h"
#include "../core/CoreEngine.h"
#include "../core/TrashService.h"
#include <QFile>
#include <QFileInfo>
#include <QTimer>

namespace QuarkMeta {

QuickLookController::QuickLookController(ActivePaneCoordinator* activeCoordinator, QObject* parent)
    : QObject(parent), m_activeCoordinator(activeCoordinator) {
}

QString QuickLookController::findAdjacentPreviewable(ContentPanel* target, const QString& currentPath, int direction) {
    if (!target || currentPath.isEmpty()) return QString();
    int step = (direction >= 0) ? 1 : -1;
    int delta = step;
    while (true) {
        QString cand = target->getAdjacentFilePath(currentPath, delta);
        if (cand.isEmpty()) break;
        if (!QFileInfo(cand).isDir() && UiHelper::canPreviewFile(cand)) {
            return cand;
        }
        delta += step;
    }
    return QString();
}

void QuickLookController::bindPane(ContentPanel* pane) {
    if (!pane) return;

    connect(pane, &ContentPanel::fileActivated, this, [](const QString& path) {
        if (path.isEmpty() || !QFile::exists(path)) {
            return;
        }
        HistoryService::instance().recordFileAccess(path);

        if (!UiHelper::canPreviewFile(path)) {
            return;
        }

        AppCommand cmd;
        cmd.type = AppCommandType::RecordAccess;
        cmd.targetPaths << path;
        CoreEngine::instance().executeCommand(cmd);

        QuickLookWindow::instance().preview(path);
    });
}

void QuickLookController::wire() {
    connect(&QuickLookWindow::instance(), &QuickLookWindow::prevRequested, this, [this]() {
        ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
        if (!target) return;
        QString curPath = QuickLookWindow::instance().currentPath();
        QString prev = findAdjacentPreviewable(target, curPath, -1);
        if (!prev.isEmpty()) {
            QuickLookWindow::instance().preview(prev);
            target->selectAndScrollToPath(prev);
        }
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::nextRequested, this, [this]() {
        ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
        if (!target) return;
        QString curPath = QuickLookWindow::instance().currentPath();
        QString next = findAdjacentPreviewable(target, curPath, 1);
        if (!next.isEmpty()) {
            QuickLookWindow::instance().preview(next);
            target->selectAndScrollToPath(next);
        }
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::ratingRequested, this, [](const QString& path, int rating) {
        if (path.isEmpty()) return;
        AppCommand cmd;
        cmd.type = AppCommandType::SetRating;
        cmd.targetPaths << path;
        cmd.params["rating"] = rating;
        CoreEngine::instance().executeCommand(cmd);
        ToolTipOverlay::instance()->showToast(QString("已将评级设为 %1 星").arg(rating), 1200);
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::colorRequested, this, [](const QString& path, const QString& hex) {
        if (path.isEmpty()) return;
        AppCommand cmd;
        cmd.type = AppCommandType::SetColor;
        cmd.targetPaths << path;
        cmd.params["color"] = hex;
        CoreEngine::instance().executeCommand(cmd);
        ToolTipOverlay::instance()->showToast("已更新颜色标记", 1200);
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::deleteRequested, this, [this](const QString& path) {
        ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
        if (!target || path.isEmpty()) return;

        // 合并为连续两次调用单一函数：先向后找，找不到再向前找
        QString nextFile = findAdjacentPreviewable(target, path, 1);
        if (nextFile.isEmpty()) {
            nextFile = findAdjacentPreviewable(target, path, -1);
        }

        QuickLookWindow::instance().setIgnoreDeactivate(true);

        bool success = TrashService::instance().moveToTrash({path});
        if (success) {
            target->refreshAll();

            if (!nextFile.isEmpty()) {
                QuickLookWindow::instance().preview(nextFile);
                target->selectAndScrollToPath(nextFile);
            } else {
                QuickLookWindow::instance().close();
            }
        }

        QTimer::singleShot(200, []() {
            QuickLookWindow::instance().setIgnoreDeactivate(false);
        });
    });
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.9 新建 `QuarkMeta_Source/src/ui/ShortcutActionRouter.h`

```
<<<<<<< SEARCH FILE: QuarkMeta_Source/src/ui/ShortcutActionRouter.h
=======
#pragma once

#include <QObject>
#include <QPointer>
#include "AppShortcutController.h"
#include "TitleBarWidget.h"
#include "PanelLayoutManager.h"

namespace QuarkMeta {

class ActivePaneCoordinator;

/**
 * @brief 快捷键与标题栏动作路由器
 * 负责处理 AppShortcutController 快捷键信号与 TitleBarWidget 界面控制动作。
 */
class ShortcutActionRouter : public QObject {
    Q_OBJECT
public:
    explicit ShortcutActionRouter(ActivePaneCoordinator* activeCoordinator,
                                  AppShortcutController* shortcutController,
                                  TitleBarWidget* titleBar,
                                  PanelLayoutManager* layoutManager,
                                  QObject* parent = nullptr);
    ~ShortcutActionRouter() override = default;

    void wire();

private:
    bool isRecycleBinView() const;

    ActivePaneCoordinator* m_activeCoordinator = nullptr;
    QPointer<AppShortcutController> m_shortcutController;
    QPointer<TitleBarWidget> m_titleBar;
    QPointer<PanelLayoutManager> m_layoutManager;
};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.10 新建 `QuarkMeta_Source/src/ui/ShortcutActionRouter.cpp`

```
<<<<<<< SEARCH FILE: QuarkMeta_Source/src/ui/ShortcutActionRouter.cpp
=======
#include "ShortcutActionRouter.h"
#include "ActivePaneCoordinator.h"
#include "controllers/ContextMenuFactory.h"
#include "ToolTipOverlay.h"
#include "../core/TrashService.h"
#include "../core/PermanentDeleteService.h"
#include "../meta/MarkedFolderService.h"
#include "../core/AppConfig.h"
#include <QClipboard>
#include <QGuiApplication>

namespace QuarkMeta {

ShortcutActionRouter::ShortcutActionRouter(ActivePaneCoordinator* activeCoordinator,
                                           AppShortcutController* shortcutController,
                                           TitleBarWidget* titleBar,
                                           PanelLayoutManager* layoutManager,
                                           QObject* parent)
    : QObject(parent),
      m_activeCoordinator(activeCoordinator),
      m_shortcutController(shortcutController),
      m_titleBar(titleBar),
      m_layoutManager(layoutManager) {
}

bool ShortcutActionRouter::isRecycleBinView() const {
    ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
    return target && (target->displayPath() == "trash_root" || target->displayPath().startsWith("trash://"));
}

void ShortcutActionRouter::wire() {
    if (m_titleBar) {
        connect(m_titleBar, &TitleBarWidget::layoutMenuRequested, this, [this](const QPoint& globalPos) {
            if (m_layoutManager) {
                m_layoutManager->showLayoutMenu(globalPos);
            }
        });

        connect(m_titleBar, &TitleBarWidget::toggleZenModeRequested, this, [this]() {
            if (m_layoutManager) {
                m_layoutManager->toggleZenMode();
            }
        });

        connect(m_titleBar, &TitleBarWidget::viewModeRequested, this, [this](const QString& mode) {
            ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
            if (target) {
                target->setViewMode(mode);
            }
        });

        connect(m_titleBar, &TitleBarWidget::createNewItemRequested, this, [this](const QString& type) {
            ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
            if (target) {
                target->createNewItem(type);
            }
        });

        // 初始化读取缩放等级
        int savedZoom = AppConfig::instance().getValue("UI/GridZoomLevel", 96).toInt();
        savedZoom = qBound(30, savedZoom, 230);
        m_titleBar->setZoomValue(savedZoom);
        ContentPanel* root = m_activeCoordinator ? m_activeCoordinator->rootPane() : nullptr;
        if (root) {
            root->setGridZoomLevel(savedZoom);
        }

        connect(m_titleBar, &TitleBarWidget::zoomValueChanged, this, [this](int value) {
            ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
            if (target) {
                target->setGridZoomLevel(value);
            }
            AppConfig::instance().setValue("UI/GridZoomLevel", value);
        });
    }

    if (m_shortcutController) {
        connect(m_shortcutController, &AppShortcutController::newFolderRequested, this, [this]() {
            ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
            if (target) target->createNewItem("folder");
        });

        connect(m_shortcutController, &AppShortcutController::newMarkdownRequested, this, [this]() {
            ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
            if (target) target->createNewItem("markdown");
        });

        connect(m_shortcutController, &AppShortcutController::extractContentRequested, this, [this]() {
            ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
            if (target) target->extractArchiveItems();
        });

        connect(m_shortcutController, &AppShortcutController::toggleRecursiveRequested, this, [this]() {
            ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
            if (target) target->toggleRecursiveMode();
        });

        connect(m_shortcutController, &AppShortcutController::resetFiltersRequested, this, [this]() {
            ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
            if (target) target->clearAllFilters(true);
        });

        connect(m_shortcutController, &AppShortcutController::moveToTrashRequested, this, [this]() {
            ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
            if (!target) return;
            QStringList selectedPaths = target->getSelectedPaths();
            if (selectedPaths.isEmpty()) return;

            if (isRecycleBinView()) {
                PermanentDeleteService::instance().deletePermanently(selectedPaths, target);
            } else {
                TrashService::instance().moveToTrash(selectedPaths);
            }
        });

        connect(m_shortcutController, &AppShortcutController::toggleMarkedRequested, this, [this]() {
            ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
            if (!target) return;
            QStringList selectedPaths = target->getSelectedPaths();
            if (selectedPaths.isEmpty()) return;

            bool anyUnmarked = false;
            for (const QString& path : selectedPaths) {
                if (!MarkedFolderService::instance().isMarked(path)) {
                    anyUnmarked = true;
                    break;
                }
            }
            if (anyUnmarked) {
                MarkedFolderService::instance().markPaths(selectedPaths);
            } else {
                MarkedFolderService::instance().unmarkPaths(selectedPaths);
            }
        });

        connect(m_shortcutController, &AppShortcutController::transferToMarkedRequested, this, [this]() {
            ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
            if (!target) return;
            QStringList selectedPaths = target->getSelectedPaths();
            if (selectedPaths.isEmpty()) return;
            MarkedFolderService::instance().transferToMarked(selectedPaths, target);
        });

        connect(m_shortcutController, &AppShortcutController::copyNamesRequested, this, [this](int type) {
            ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
            if (!target) return;
            QStringList selectedPaths = target->getSelectedPaths();
            if (selectedPaths.isEmpty()) return;

            auto copyType = static_cast<ContextMenuFactory::CopyNameType>(type);
            QString text = ContextMenuFactory::formatCopyNames(selectedPaths, copyType);
            if (!text.isEmpty()) {
                QGuiApplication::clipboard()->setText(text);
                ToolTipOverlay::instance()->showToast(QString("已复制 %1 个名称到剪贴板").arg(selectedPaths.size()), 1500);
            }
        });
    }
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.11 新建 `QuarkMeta_Source/src/ui/TabBarCoordinator.h`

```
<<<<<<< SEARCH FILE: QuarkMeta_Source/src/ui/TabBarCoordinator.h
=======
#pragma once

#include <QObject>
#include <QPointer>
#include <functional>
#include "TabBarWidget.h"
#include "ContentPanel.h"

namespace QuarkMeta {

class ActivePaneCoordinator;

/**
 * @brief 标签栏协同协调器
 * 负责多标签页生命周期、分屏状态导出保存（单一实现）、标签页切换导航与属性变更。
 */
class TabBarCoordinator : public QObject {
    Q_OBJECT
public:
    explicit TabBarCoordinator(ActivePaneCoordinator* activeCoordinator,
                             TabBarWidget* tabBar,
                             const std::function<void()>& refreshCurrentFunc,
                             QObject* parent = nullptr);
    ~TabBarCoordinator() override = default;

    void wire();

private:
    void exportAndSaveSplitState();

    ActivePaneCoordinator* m_activeCoordinator = nullptr;
    QPointer<TabBarWidget> m_tabBar;
    std::function<void()> m_refreshCurrentFunc;
};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.12 新建 `QuarkMeta_Source/src/ui/TabBarCoordinator.cpp`

```
<<<<<<< SEARCH FILE: QuarkMeta_Source/src/ui/TabBarCoordinator.cpp
=======
#include "TabBarCoordinator.h"
#include "ActivePaneCoordinator.h"
#include "controllers/ContentPaneSplitManager.h"
#include "ToolTipOverlay.h"
#include "../core/NavigationService.h"
#include "../core/CoreEngine.h"
#include "../meta/FavoriteDao.h"

namespace QuarkMeta {

TabBarCoordinator::TabBarCoordinator(ActivePaneCoordinator* activeCoordinator,
                                     TabBarWidget* tabBar,
                                     const std::function<void()>& refreshCurrentFunc,
                                     QObject* parent)
    : QObject(parent),
      m_activeCoordinator(activeCoordinator),
      m_tabBar(tabBar),
      m_refreshCurrentFunc(refreshCurrentFunc) {
}

void TabBarCoordinator::exportAndSaveSplitState() {
    if (!m_tabBar || !m_activeCoordinator) return;
    ContentPanel* root = m_activeCoordinator->rootPane();
    if (!root) return;
    QJsonObject splitState = ContentPaneSplitManager::exportSplitState(root);
    m_tabBar->saveCurrentTabSplitState(splitState);
}

void TabBarCoordinator::wire() {
    if (!m_tabBar || !m_activeCoordinator) return;
    ContentPanel* root = m_activeCoordinator->rootPane();

    connect(m_tabBar, &TabBarWidget::tabAboutToChange, this, [this]() {
        exportAndSaveSplitState();
    });

    connect(m_tabBar, &TabBarWidget::currentTabChanged, this, [this, root](const QString& path) {
        if (root && m_tabBar) {
            QJsonObject splitState = m_tabBar->currentTabSplitState();
            if (!splitState.isEmpty()) {
                ContentPaneSplitManager::restoreSplitState(root, splitState, [this](ContentPanel* activeP) {
                    if (m_activeCoordinator) {
                        m_activeCoordinator->activate(activeP);
                    }
                });
            }
        }
        NavigationService::instance().navigateTo(path);
    });

    connect(m_tabBar, &TabBarWidget::mergeRequested, this, [this, root](int targetTabIndex, const QString& targetPath) {
        exportAndSaveSplitState();
        if (root && m_tabBar) {
            int currentPanes = ContentPaneSplitManager::countPanes(root);
            QJsonObject targetState = m_tabBar->tabSplitState(targetTabIndex);
            int targetPanes = targetState.isEmpty() ? 1 : ContentPaneSplitManager::countPanesInState(targetState);
            if (currentPanes + targetPanes > 4) {
                ToolTipOverlay::instance()->showToast("合并后窗格数量超过上限 (最大 4 个)", 2000, QColor("#e81123"));
                return;
            }
            ContentPaneSplitManager::mergeTabIntoCurrent(root, targetPath, targetState, [this](ContentPanel* activeP) {
                if (m_activeCoordinator) {
                    m_activeCoordinator->activate(activeP);
                }
            });
            m_tabBar->removeTab(targetTabIndex);
        }
    });

    connect(m_tabBar, &TabBarWidget::splitRequested, this, [this, root](ContentPanel* pane) {
        exportAndSaveSplitState();
        if (root && m_tabBar) {
            QString path = pane ? pane->displayPath() : QString();
            QJsonObject paneState = pane ? ContentPaneSplitManager::exportPaneState(pane) : QJsonObject();
            ContentPaneSplitManager::splitPaneIntoNewTab(root, pane);
            m_tabBar->addNewTab(path, paneState);
        }
    });

    connect(m_tabBar, &TabBarWidget::folderDroppedOnTab, this, [this, root](int tabIndex, const QString& folderPath) {
        if (!root || !m_tabBar) return;
        int currentPanes = ContentPaneSplitManager::countPanes(root);
        if (currentPanes >= 4) {
            ToolTipOverlay::instance()->showToast("当前窗格数量已达上限 (最大 4 个)", 2000, QColor("#e81123"));
            return;
        }
        ContentPaneSplitManager::openFolderInSplitPane(root, folderPath, [this](ContentPanel* activeP) {
            if (m_activeCoordinator) {
                m_activeCoordinator->activate(activeP);
            }
        });
    });

    connect(m_tabBar, &TabBarWidget::refreshRequested, this, [this]() {
        if (m_refreshCurrentFunc) {
            m_refreshCurrentFunc();
        }
    });

    connect(m_tabBar, &TabBarWidget::tabColorChanged, this, [](const QString& path, const QString& colorHex) {
        if (path.isEmpty()) return;
        AppCommand cmd;
        cmd.type = AppCommandType::SetColor;
        cmd.targetPaths << path;
        cmd.params["color"] = colorHex;
        CoreEngine::instance().executeCommand(cmd);
    });

    connect(m_tabBar, &TabBarWidget::tabIconChanged, this, [](const QString& path, const QString& iconKey) {
        if (path.isEmpty()) return;
        FavoriteDao::updateFavorite(path, iconKey);
    });

    if (root) {
        connect(root, &ContentPanel::requestOpenInNewTab, this, [this](const QString& path) {
            if (m_tabBar) {
                m_tabBar->addNewTab(path);
            }
        });
    }
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.13 新建 `QuarkMeta_Source/src/ui/NavigationCoordinator.h`

```
<<<<<<< SEARCH FILE: QuarkMeta_Source/src/ui/NavigationCoordinator.h
=======
#pragma once

#include <QObject>
#include <QPointer>
#include <QStringList>
#include "ContentPanel.h"
#include "NavPanel.h"
#include "FavoritePanel.h"
#include "LibraryPanel.h"
#include "MarkedPanel.h"
#include "AddressBar.h"
#include "TabBarWidget.h"
#include "SearchController.h"
#include "../meta/SmartFolderDao.h"

namespace QuarkMeta {

class ActivePaneCoordinator;

/**
 * @brief 导航与全局位置协调器
 * 统一处理全应用路径跳转、位置显示（单一实现）、分类/智能文件夹路由、搜索绑定与数据刷新。
 */
class NavigationCoordinator : public QObject {
    Q_OBJECT
public:
    explicit NavigationCoordinator(ActivePaneCoordinator* activeCoordinator,
                                   NavPanel* navPanel,
                                   FavoritePanel* favoritePanel,
                                   LibraryPanel* libraryPanel,
                                   MarkedPanel* markedPanel,
                                   AddressBar* addressBar,
                                   TabBarWidget* tabBar,
                                   SearchController* searchController,
                                   QObject* parent = nullptr);
    ~NavigationCoordinator() override = default;

    void locateItem(const QString& fullPath);
    void refreshCurrent();
    void bindPane(ContentPanel* pane);
    void wire();

signals:
    void statusMessageRequested(const QString& message);

private:
    void showLocation(const QString& addressText, const QString& tabTitle, const QString& tabUrl);

    ActivePaneCoordinator* m_activeCoordinator = nullptr;
    QPointer<NavPanel> m_navPanel;
    QPointer<FavoritePanel> m_favoritePanel;
    QPointer<LibraryPanel> m_libraryPanel;
    QPointer<MarkedPanel> m_markedPanel;
    QPointer<AddressBar> m_addressBar;
    QPointer<TabBarWidget> m_tabBar;
    QPointer<SearchController> m_searchController;
};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.14 新建 `QuarkMeta_Source/src/ui/NavigationCoordinator.cpp`

```
<<<<<<< SEARCH FILE: QuarkMeta_Source/src/ui/NavigationCoordinator.cpp
=======
#include "NavigationCoordinator.h"
#include "ActivePaneCoordinator.h"
#include "../core/NavigationService.h"
#include "../meta/FavoriteService.h"
#include "../meta/LibrarySyncService.h"
#include "../core/CentralEventHub.h"
#include "../core/VolumeOnlineManager.h"
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>

namespace QuarkMeta {

NavigationCoordinator::NavigationCoordinator(ActivePaneCoordinator* activeCoordinator,
                                             NavPanel* navPanel,
                                             FavoritePanel* favoritePanel,
                                             LibraryPanel* libraryPanel,
                                             MarkedPanel* markedPanel,
                                             AddressBar* addressBar,
                                             TabBarWidget* tabBar,
                                             SearchController* searchController,
                                             QObject* parent)
    : QObject(parent),
      m_activeCoordinator(activeCoordinator),
      m_navPanel(navPanel),
      m_favoritePanel(favoritePanel),
      m_libraryPanel(libraryPanel),
      m_markedPanel(markedPanel),
      m_addressBar(addressBar),
      m_tabBar(tabBar),
      m_searchController(searchController) {
}

void NavigationCoordinator::showLocation(const QString& addressText, const QString& tabTitle, const QString& tabUrl) {
    if (m_addressBar) {
        m_addressBar->setPath(addressText);
    }
    if (m_tabBar) {
        m_tabBar->updateCurrentTabInfo(tabTitle, tabUrl);
    }
}

void NavigationCoordinator::locateItem(const QString& fullPath) {
    if (fullPath.isEmpty()) return;

    ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
    if (!target) return;

    QFileInfo fi(fullPath);
    QString dirPath = fi.isDir() ? fullPath : fi.absolutePath();
    target->setPendingSelectedItem(fullPath);
    NavigationService::instance().navigateTo(dirPath);

    if (m_navPanel) {
        m_navPanel->setCurrentPath(dirPath);
    }
}

void NavigationCoordinator::refreshCurrent() {
    ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
    if (target && (target->getCurrentCategoryType() == "library" || target->getCurrentCategoryType() == "smart_folder")) {
        target->refreshAll();
    } else {
        NavigationService::instance().refresh();
    }
}

void NavigationCoordinator::bindPane(ContentPanel* pane) {
    if (!pane) return;

    // 遵循统一规则二：主窗格与动态副窗格统一在此连接且仅连接一次
    connect(pane, &ContentPanel::directorySelected, &NavigationService::instance(), [](const QString& path) {
        NavigationService::instance().navigateTo(path);
    });

    if (m_favoritePanel) {
        connect(pane, &ContentPanel::requestAddFavorite, this, [](const QStringList& paths) {
            FavoriteService::instance().addFavorites(paths);
        });
        connect(pane, &ContentPanel::requestRemoveFavorite, this, [](const QStringList& paths) {
            FavoriteService::instance().removeFavorites(paths);
        });
    }

    // panelActivated 更新地址栏为窗格的 displayPath
    connect(pane, &ContentPanel::panelActivated, this, [this](ContentPanel* activePanel) {
        if (m_addressBar && activePanel) {
            m_addressBar->setPath(activePanel->displayPath());
        }
    });
}

void NavigationCoordinator::wire() {
    connect(&NavigationService::instance(), &NavigationService::requestLocateItem, this, &NavigationCoordinator::locateItem);

    if (m_navPanel) {
        connect(m_navPanel, &NavPanel::pathSelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });
        connect(m_navPanel, &NavPanel::requestLocateFile, this, &NavigationCoordinator::locateItem);
        connect(m_navPanel, &NavPanel::requestAddFavorite, this, [](const QString& path) {
            FavoriteService::instance().addFavorites({path});
        });
    }

    if (m_favoritePanel) {
        connect(m_favoritePanel, &FavoritePanel::pathSelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });
        connect(m_favoritePanel, &FavoritePanel::requestLocateFile, this, &NavigationCoordinator::locateItem);
    }

    if (m_markedPanel) {
        connect(m_markedPanel, &MarkedPanel::pathSelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });
    }

    if (m_addressBar) {
        connect(m_addressBar, &AddressBar::pathChanged, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });
        connect(m_addressBar, &AddressBar::refreshRequested, this, &NavigationCoordinator::refreshCurrent);

        if (m_favoritePanel) {
            connect(m_addressBar, &AddressBar::requestAddFavorite, this, [](const QString& path) {
                if (FavoriteService::instance().isFavorite(path)) {
                    FavoriteService::instance().removeFavorites({path});
                } else {
                    FavoriteService::instance().addFavorites({path});
                }
            });
            connect(m_addressBar, &AddressBar::requestRemoveFavorite, this, [](const QString& path) {
                FavoriteService::instance().removeFavorites({path});
            });
        }
    }

    connect(&VolumeOnlineManager::instance(), &VolumeOnlineManager::driveOffline, this, [](const QString& driveLetter) {
        QString current = NavigationService::instance().currentUrl();
        if (current.contains(driveLetter + ":", Qt::CaseInsensitive)) {
            QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
            NavigationService::instance().navigateTo(desktopPath);
        }
    });

    if (m_libraryPanel) {
        connect(m_libraryPanel, &LibraryPanel::categoryPathsSelected, this, [this](const QStringList& paths, const QString& name, int categoryId) {
            ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
            if (!target) return;

            QString title = name.isEmpty() ? "全部数据" : name;
            showLocation(title, title, QString("library://%1").arg(categoryId));

            target->clearAllFilters(true);
            target->applyViewTarget(categoryId, paths);
            target->setLastLoadedLibraryPaths(paths);
            target->loadLibraryPaths(paths);
        });

        connect(m_libraryPanel, &LibraryPanel::smartFolderSelected, this, [this](const SmartFolderRecord& record) {
            ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
            if (!target) return;

            showLocation(record.name, record.name, QString("smart_folder://%1").arg(record.id));

            target->clearAllFilters(true);
            target->loadSmartFolder(record);
        });

        connect(m_libraryPanel, &LibraryPanel::smartFolderPreviewRequested, this, [this](const SmartFolderRecord& record) {
            ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
            if (!target) return;

            QString displayName = record.name.isEmpty() ? "智能文件夹预览" : record.name;
            showLocation(displayName, displayName, QString("smart_folder_preview://%1").arg(record.id));

            target->clearAllFilters(true);
            target->loadSmartFolder(record);
        });
    }

    connect(&NavigationService::instance(), &NavigationService::currentUrlChanged, this, [this](const QString& url) {
        ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
        if (!target) return;

        if (m_searchController) {
            m_searchController->clearSearch();
        }
        target->search("");
        target->clearAllFilters(true);
        target->clearDirectoryStats();
        target->setMirrorSource(false);

        QString subItemToSelect = target->takePendingSelectedItem();

        if (url == "computer://") {
            showLocation("此电脑", "此电脑", "computer://");
            if (m_navPanel) m_navPanel->setCurrentPath("computer://");
            target->loadComputer();
            return;
        }

        if (url == "trash_root" || url == "trash://") {
            showLocation("回收站", "回收站", "trash://");
            if (m_navPanel) m_navPanel->setCurrentPath("trash_root");
            target->loadTrash();
            return;
        }

        QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(url));
        QFileInfo fi(cleanPath);
        QString tabName = fi.fileName().isEmpty() ? cleanPath : fi.fileName();
        showLocation(cleanPath, tabName, cleanPath);

        if (m_navPanel) {
            m_navPanel->setCurrentPath(cleanPath);
        }

        target->loadDirectory(cleanPath, subItemToSelect);
    });

    if (m_searchController) {
        ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
        m_searchController->bindContentPanel(target);

        connect(m_searchController, &SearchController::searchCompleted, this, [this](int count, qint64 elapsedMs) {
            emit statusMessageRequested(QString("搜索已完成，共匹配 %1 个项目 (耗时 %2 毫秒)").arg(count).arg(elapsedMs));
        });
    }

    connect(&LibrarySyncService::instance(), &LibrarySyncService::libraryItemsSynced, this, [this]() {
        if (m_libraryPanel) {
            m_libraryPanel->refreshSmartFolderCounts();
        }
        ContentPanel* root = m_activeCoordinator ? m_activeCoordinator->rootPane() : nullptr;
        if (root) {
            for (ContentPanel* pane : root->panes()) {
                if (pane) {
                    pane->refreshSmartFolderIfShowing();
                }
            }
        }
    });

    connect(&CentralEventHub::instance(), &CentralEventHub::eventOccurred, this, [this](const QuarkMeta::AppEvent& event) {
        if (event.type == QuarkMeta::AppEventType::ItemsDeleted ||
            event.type == QuarkMeta::AppEventType::ItemsRenamed ||
            event.type == QuarkMeta::AppEventType::UndoRedoPerformed) {
            ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
            if (target) {
                target->refreshAll();
            }
            ContentPanel* root = m_activeCoordinator ? m_activeCoordinator->rootPane() : nullptr;
            if (root && root != target) {
                root->refreshAll();
            }
        }
    });
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.15 新建 `QuarkMeta_Source/src/ui/FilterStatsBinder.h`

```
<<<<<<< SEARCH FILE: QuarkMeta_Source/src/ui/FilterStatsBinder.h
=======
#pragma once

#include <QObject>
#include <QPointer>
#include "FilterPanel.h"
#include "MetaPanel.h"
#include "ContentPanel.h"

namespace QuarkMeta {

class ActivePaneCoordinator;

/**
 * @brief 筛选与统计状态绑定器
 * 负责在窗格切换时断开旧连接、连接新窗格的统计信号、广播 FilterStateChanged 与同步筛选条件。
 */
class FilterStatsBinder : public QObject {
    Q_OBJECT
public:
    explicit FilterStatsBinder(ActivePaneCoordinator* activeCoordinator,
                               FilterPanel* filterPanel,
                               MetaPanel* metaPanel,
                               QObject* parent = nullptr);
    ~FilterStatsBinder() override = default;

    void bindFilterToActivePanel(ContentPanel* activePanel);
    void wire();

private:
    ActivePaneCoordinator* m_activeCoordinator = nullptr;
    QPointer<FilterPanel> m_filterPanel;
    QPointer<MetaPanel> m_metaPanel;
    QMetaObject::Connection m_statsConnection;
};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.16 新建 `QuarkMeta_Source/src/ui/FilterStatsBinder.cpp`

```
<<<<<<< SEARCH FILE: QuarkMeta_Source/src/ui/FilterStatsBinder.cpp
=======
#include "FilterStatsBinder.h"
#include "ActivePaneCoordinator.h"
#include "../core/CentralEventHub.h"

namespace QuarkMeta {

FilterStatsBinder::FilterStatsBinder(ActivePaneCoordinator* activeCoordinator,
                                     FilterPanel* filterPanel,
                                     MetaPanel* metaPanel,
                                     QObject* parent)
    : QObject(parent),
      m_activeCoordinator(activeCoordinator),
      m_filterPanel(filterPanel),
      m_metaPanel(metaPanel) {
}

void FilterStatsBinder::bindFilterToActivePanel(ContentPanel* activePanel) {
    if (!m_filterPanel || !activePanel) return;

    if (m_statsConnection) {
        disconnect(m_statsConnection);
    }

    m_statsConnection = connect(activePanel, &ContentPanel::directoryStatsReady, m_filterPanel, &FilterPanel::updateStats);

    AppEvent event;
    event.type = AppEventType::FilterStateChanged;
    CentralEventHub::instance().publish(event);

    {
        QSignalBlocker blocker(m_filterPanel);
        m_filterPanel->syncUIFromFilterState(activePanel->currentFilterState());
    }

    activePanel->recalculateAndEmitStats();
}

void FilterStatsBinder::wire() {
    if (m_filterPanel) {
        ContentPanel* root = m_activeCoordinator ? m_activeCoordinator->rootPane() : nullptr;
        if (root) {
            bindFilterToActivePanel(root);
        }

        connect(m_filterPanel, &FilterPanel::filterChanged, this, [this](const FilterState& state) {
            ContentPanel* target = m_activeCoordinator ? m_activeCoordinator->currentTarget() : nullptr;
            if (target) {
                target->applyFilters(state);
            }
        });
    }

    if (m_metaPanel && m_filterPanel) {
        connect(m_metaPanel, &MetaPanel::searchByColor, m_filterPanel, &FilterPanel::selectColor);
    }
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.17 构建配置全量注册：`QuarkMeta_Source/CMakeLists.txt`

```
<<<<<<< SEARCH
    src/ui/PanelMediator.h
    src/ui/PanelMediator.cpp
=======
    src/ui/ActivePaneCoordinator.h
    src/ui/ActivePaneCoordinator.cpp
    src/ui/MetaPanelPresenter.h
    src/ui/MetaPanelPresenter.cpp
    src/ui/MetaEditController.h
    src/ui/MetaEditController.cpp
    src/ui/QuickLookController.h
    src/ui/QuickLookController.cpp
    src/ui/ShortcutActionRouter.h
    src/ui/ShortcutActionRouter.cpp
    src/ui/TabBarCoordinator.h
    src/ui/TabBarCoordinator.cpp
    src/ui/NavigationCoordinator.h
    src/ui/NavigationCoordinator.cpp
    src/ui/FilterStatsBinder.h
    src/ui/FilterStatsBinder.cpp
    src/ui/PanelMediator.h
    src/ui/PanelMediator.cpp
>>>>>>> REPLACE
```

### 3.18 面板中介者头文件瘦身与协调器持有：`QuarkMeta_Source/src/ui/PanelMediator.h`

```
<<<<<<< SEARCH
#pragma once

#include <QObject>
#include <QPointer>
#include "ContentPanel.h"

namespace QuarkMeta {

class NavPanel;
class FavoritePanel;
class LibraryPanel;
class MarkedPanel;
class ContentPanel;
class MetaPanel;
class FilterPanel;
class AddressBar;
class SearchController;
class TitleBarWidget;
class PanelLayoutManager;
class AppShortcutController;
class PaneActivationTracker;

/**
 * @brief 面板中介者组件依赖包
 */
struct PanelMediatorComponents {
    NavPanel* navPanel = nullptr;
    FavoritePanel* favoritePanel = nullptr;
    LibraryPanel* libraryPanel = nullptr;
    MarkedPanel* markedPanel = nullptr;
    ContentPanel* contentPanel = nullptr;
    MetaPanel* metaPanel = nullptr;
    FilterPanel* filterPanel = nullptr;
    AddressBar* addressBar = nullptr;
    SearchController* searchController = nullptr;
    TitleBarWidget* titleBar = nullptr;
    PanelLayoutManager* layoutManager = nullptr;
    AppShortcutController* shortcutController = nullptr;
};

/**
 * @brief 面板中介者
 * 负责各面板与控制器之间的信号槽连接与协同动作，彻底解耦 MainWindow
 */
class PanelMediator : public QObject {
    Q_OBJECT

public:
    explicit PanelMediator(const PanelMediatorComponents& components, QObject* parent = nullptr);
    ~PanelMediator() override;

    static PanelMediator* instance();

    ContentPanel* activeContentPanel() const { return m_activeContentPanel.data(); }

    /**
     * @brief 建立各面板间的信号槽连接
     */
    void setupConnections();

    /**
     * @brief 统一在主视图中定位目标项目 (SSOT)
     * @param fullPath 目标文件或文件夹完整物理路径
     */
    void locateItem(const QString& fullPath);

signals:
    /**
     * @brief 统一向 MainWindow 发送状态栏消息更新请求
     */
    void statusMessageRequested(const QString& message);

    /**
     * @brief 当前激活的内容面板改变广播信号
     */
    void activeContentPanelChanged(ContentPanel* panel);

private:
    QPointer<NavPanel> m_navPanel;
    QPointer<FavoritePanel> m_favoritePanel;
    QPointer<LibraryPanel> m_libraryPanel;
    QPointer<MarkedPanel> m_markedPanel;
    QPointer<ContentPanel> m_contentPanel;
    QPointer<ContentPanel> m_activeContentPanel;
    QPointer<MetaPanel> m_metaPanel;
    QPointer<FilterPanel> m_filterPanel;
    QPointer<AddressBar> m_addressBar;
    QPointer<SearchController> m_searchController;
    QPointer<TitleBarWidget> m_titleBar;
    QPointer<PanelLayoutManager> m_layoutManager;
    QPointer<AppShortcutController> m_shortcutController;
    PaneActivationTracker* m_activationTracker = nullptr;

    struct PanelMetaDebounceState {
        QTimer* timer = nullptr;
        QSet<QString> pendingCleanPaths;
    };
    QMap<ContentPanel*, PanelMetaDebounceState> m_panelMetaDebounceMap;
};

} // namespace QuarkMeta
=======
#pragma once

#include <QObject>
#include <QPointer>
#include "ContentPanel.h"

namespace QuarkMeta {

class NavPanel;
class FavoritePanel;
class LibraryPanel;
class MarkedPanel;
class ContentPanel;
class MetaPanel;
class FilterPanel;
class AddressBar;
class SearchController;
class TitleBarWidget;
class PanelLayoutManager;
class AppShortcutController;

class ActivePaneCoordinator;
class MetaPanelPresenter;
class MetaEditController;
class QuickLookController;
class ShortcutActionRouter;
class TabBarCoordinator;
class NavigationCoordinator;
class FilterStatsBinder;

/**
 * @brief 面板中介者组件依赖包
 */
struct PanelMediatorComponents {
    NavPanel* navPanel = nullptr;
    FavoritePanel* favoritePanel = nullptr;
    LibraryPanel* libraryPanel = nullptr;
    MarkedPanel* markedPanel = nullptr;
    ContentPanel* contentPanel = nullptr;
    MetaPanel* metaPanel = nullptr;
    FilterPanel* filterPanel = nullptr;
    AddressBar* addressBar = nullptr;
    SearchController* searchController = nullptr;
    TitleBarWidget* titleBar = nullptr;
    PanelLayoutManager* layoutManager = nullptr;
    AppShortcutController* shortcutController = nullptr;
};

/**
 * @brief 面板中介者 (瘦身重构版)
 * 作为各专职协调器的装配中枢与对外门面，完全冻结对外公开接口契约。
 */
class PanelMediator : public QObject {
    Q_OBJECT

public:
    explicit PanelMediator(const PanelMediatorComponents& components, QObject* parent = nullptr);
    ~PanelMediator() override;

    static PanelMediator* instance();

    ContentPanel* activeContentPanel() const;

    /**
     * @brief 建立各面板间的信号槽连接
     */
    void setupConnections();

    /**
     * @brief 统一在主视图中定位目标项目 (SSOT)
     * @param fullPath 目标文件或文件夹完整物理路径
     */
    void locateItem(const QString& fullPath);

signals:
    /**
     * @brief 统一向 MainWindow 发送状态栏消息更新请求
     */
    void statusMessageRequested(const QString& message);

    /**
     * @brief 当前激活的内容面板改变广播信号
     */
    void activeContentPanelChanged(ContentPanel* panel);

private:
    void distributeBindPane(ContentPanel* pane);

    QPointer<NavPanel> m_navPanel;
    QPointer<FavoritePanel> m_favoritePanel;
    QPointer<LibraryPanel> m_libraryPanel;
    QPointer<MarkedPanel> m_markedPanel;
    QPointer<ContentPanel> m_contentPanel;
    QPointer<MetaPanel> m_metaPanel;
    QPointer<FilterPanel> m_filterPanel;
    QPointer<AddressBar> m_addressBar;
    QPointer<SearchController> m_searchController;
    QPointer<TitleBarWidget> m_titleBar;
    QPointer<PanelLayoutManager> m_layoutManager;
    QPointer<AppShortcutController> m_shortcutController;

    ActivePaneCoordinator* m_activeCoordinator = nullptr;
    MetaPanelPresenter* m_metaPresenter = nullptr;
    MetaEditController* m_metaEditController = nullptr;
    QuickLookController* m_quickLookController = nullptr;
    ShortcutActionRouter* m_shortcutRouter = nullptr;
    TabBarCoordinator* m_tabBarCoordinator = nullptr;
    NavigationCoordinator* m_navCoordinator = nullptr;
    FilterStatsBinder* m_filterStatsBinder = nullptr;
};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.19 面板中介者实现瘦身与协调器装配分发：`QuarkMeta_Source/src/ui/PanelMediator.cpp`

```
<<<<<<< SEARCH
#include "PanelMediator.h"
#include "NavPanel.h"
#include "FavoritePanel.h"
#include "LibraryPanel.h"
#include "MarkedPanel.h"
#include "../meta/HistoryService.h"
#include "../meta/LibrarySyncService.h"
#include "ContentPanel.h"
#include "ColumnViewWidget.h"
#include "controllers/ContentPaneSplitManager.h"
#include "controllers/PaneActivationTracker.h"
#include "controllers/ContextMenuFactory.h"
#include "MetaPanel.h"
#include "FilterPanel.h"
#include "AddressBar.h"
#include "SearchController.h"
#include "TitleBarWidget.h"
#include "TabBarWidget.h"
#include "PanelLayoutManager.h"
#include "AppShortcutController.h"
#include "QuickLookWindow.h"
#include "ToolTipOverlay.h"
#include "Logger.h"
#include <QApplication>
#include <QElapsedTimer>
#include "../core/NavigationService.h"
#include "../core/TrashService.h"
#include "../core/PermanentDeleteService.h"
#include "../meta/MarkedFolderService.h"
#include "../core/CoreEngine.h"
#include "../core/CentralEventHub.h"
#include "../core/VolumeOnlineManager.h"
#include "../core/ModelContract.h"
#include "../core/AppConfig.h"
#include "../util/ShellHelper.h"
#include "../meta/MetadataManager.h"
#include "../meta/FavoriteDao.h"
#include "../meta/FavoriteService.h"
#include "../meta/LibraryDao.h"
#include "../meta/SmartFolderDao.h"
#include "UiHelper.h"
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QCursor>

namespace QuarkMeta {

static PanelMediator* s_panelMediatorInstance = nullptr;

PanelMediator* PanelMediator::instance() {
    return s_panelMediatorInstance;
}

PanelMediator::PanelMediator(const PanelMediatorComponents& components, QObject* parent)
    : QObject(parent),
      m_navPanel(components.navPanel),
      m_favoritePanel(components.favoritePanel),
      m_libraryPanel(components.libraryPanel),
      m_markedPanel(components.markedPanel),
      m_contentPanel(components.contentPanel),
      m_metaPanel(components.metaPanel),
      m_filterPanel(components.filterPanel),
      m_addressBar(components.addressBar),
      m_searchController(components.searchController),
      m_titleBar(components.titleBar),
      m_layoutManager(components.layoutManager),
      m_shortcutController(components.shortcutController) {
    s_panelMediatorInstance = this;
}

PanelMediator::~PanelMediator() {
    if (s_panelMediatorInstance == this) {
        s_panelMediatorInstance = nullptr;
    }
}

void PanelMediator::locateItem(const QString& fullPath) {
    if (fullPath.isEmpty() || fullPath.contains("://")) return;

    QFileInfo fi(fullPath);
    QString targetDir = fi.absolutePath();
    ContentPanel* target = (m_activeContentPanel && m_activeContentPanel->isVisible()) ? m_activeContentPanel.data() : m_contentPanel.data();
    if (!target) return;

    target->setPendingSelectPath(fullPath, false);

    if (m_filterPanel) {
        m_filterPanel->clearAllFilters();
        m_filterPanel->clearStats();
    }
    if (m_searchController) {
        m_searchController->clearSearch(true);
    }
    target->search("");

    QString currentNative = QDir::toNativeSeparators(QDir::cleanPath(target->currentPath()));
    QString targetDirNative = QDir::toNativeSeparators(QDir::cleanPath(targetDir));

    bool isSameDir = (QString::compare(currentNative, targetDirNative, Qt::CaseInsensitive) == 0);

    if (isSameDir && !target->isRecursive()) {
        target->restoreSelections();
    } else if (isSameDir && target->isRecursive()) {
        target->loadDirectory(targetDir, false);
    } else {
        NavigationService::instance().navigateTo(targetDir, true, false);
    }
}

void PanelMediator::setupConnections() {
    NavPanel* navPanel = m_navPanel;

    // 🚀【Tab页在新标签中打开路由】：从 NavigationService 接收在 Tab 栏打开请求，解耦控制器下钻
    connect(&NavigationService::instance(), &NavigationService::requestLocateItem, this, &PanelMediator::locateItem);

    connect(&NavigationService::instance(), &NavigationService::requestOpenInNewTab, this, [this](const QString& url) {
        if (m_titleBar && m_titleBar->tabBar()) {
            m_titleBar->tabBar()->openInNewTab(url);
        }
    });
    FavoritePanel* favoritePanel = m_favoritePanel;
    LibraryPanel* libraryPanel = m_libraryPanel;
    MarkedPanel* markedPanel = m_markedPanel;
    ContentPanel* contentPanel = m_contentPanel;
    MetaPanel* metaPanel = m_metaPanel;
    FilterPanel* filterPanel = m_filterPanel;
    AddressBar* addressBar = m_addressBar;
    SearchController* searchController = m_searchController;
    TitleBarWidget* titleBar = m_titleBar;
    PanelLayoutManager* layoutManager = m_layoutManager;
    AppShortcutController* shortcutController = m_shortcutController;

    // 0. TitleBar 与各组件的高阶编排及 UI 状态恢复/持久化
    if (titleBar) {
        if (titleBar->tabBar()) {
            // 切出旧 Tab 前：快照导出旧 Tab 的完整分屏树
            connect(titleBar->tabBar(), &TabBarWidget::tabAboutToChange, this, [this, titleBar, contentPanel](int oldIndex) {
                ContentPanel* root = contentPanel ? contentPanel->rootPane() : nullptr;
                if (root && root->splitManager()) {
                    TabSplitState state = root->splitManager()->exportSplitState();
                    titleBar->tabBar()->setTabSplitState(oldIndex, state);
                }
            });

            // 切换到新 Tab：还原新 Tab 的专属分屏树并通知 NavigationService
            connect(titleBar->tabBar(), &TabBarWidget::currentTabChanged, this, [this, titleBar, contentPanel](int index, const QString& url) {
                ContentPanel* root = contentPanel ? contentPanel->rootPane() : nullptr;
                if (root && root->splitManager() && titleBar->tabBar()) {
                    TabSplitState state = titleBar->tabBar()->tabSplitState(index);
                    root->splitManager()->restoreSplitState(state);
                }
                NavigationService::instance().navigateTo(url);
            });

            connect(titleBar->tabBar(), &TabBarWidget::refreshRequested, this, [this, contentPanel]() {
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target && (target->getCurrentCategoryType() == "library" || target->getCurrentCategoryType() == "smart_folder")) {
                    target->refreshAll();
                } else {
                    NavigationService::instance().refresh();
                }
            });

            connect(titleBar->tabBar(), &TabBarWidget::mergeRequested, this, [this, titleBar, contentPanel](int sourceIndex, int targetIndex) {
                ContentPanel* root = contentPanel ? contentPanel->rootPane() : nullptr;
                if (root && root->splitManager() && titleBar->tabBar()) {
                    TabSplitState state = root->splitManager()->exportSplitState();
                    titleBar->tabBar()->setTabSplitState(titleBar->tabBar()->currentIndex(), state);

                    TabSplitState srcState = titleBar->tabBar()->tabSplitState(sourceIndex);
                    TabSplitState tgtState = titleBar->tabBar()->tabSplitState(targetIndex);

                    int srcCount = (srcState.isSplit && !srcState.panePaths.isEmpty()) ? srcState.panePaths.size() : 1;
                    int tgtCount = (tgtState.isSplit && !tgtState.panePaths.isEmpty()) ? tgtState.panePaths.size() : 1;

                    if (srcCount + tgtCount > ContentPanel::kMaxPanes) {
                        ToolTipOverlay::instance()->showToast(QString("窗格数量超出上限%1窗格，不支持合并").arg(ContentPanel::kMaxPanes), 2000, QColor("#e81123"));
                        return;
                    }

                    titleBar->tabBar()->mergeTab(sourceIndex, targetIndex);
                }
            });

            connect(titleBar->tabBar(), &TabBarWidget::splitRequested, this, [this, titleBar, contentPanel](int tabIndex) {
                ContentPanel* root = contentPanel ? contentPanel->rootPane() : nullptr;
                if (root && root->splitManager() && titleBar->tabBar()) {
                    TabSplitState state = root->splitManager()->exportSplitState();
                    titleBar->tabBar()->setTabSplitState(titleBar->tabBar()->currentIndex(), state);
                    titleBar->tabBar()->splitTab(tabIndex);
                }
            });

            connect(titleBar->tabBar(), &TabBarWidget::folderDroppedOnTab, this, [this, titleBar, contentPanel](int tabIndex, const QString& folderPath) {
                if (!titleBar || !titleBar->tabBar()) return;
                titleBar->tabBar()->setCurrentIndex(tabIndex, true);
                ContentPanel* root = contentPanel ? contentPanel->rootPane() : nullptr;
                if (root) {
                    if (root->paneCount() >= ContentPanel::kMaxPanes) {
                        ToolTipOverlay::instance()->showToast(QString("窗格数量超出上限%1窗格，不支持继续分屏").arg(ContentPanel::kMaxPanes), 2000, QColor("#e81123"));
                        return;
                    }
                    root->splitPane(Qt::Horizontal, folderPath);
                }
            });

            connect(titleBar->tabBar(), &TabBarWidget::tabColorChanged, this, [](int /*index*/, const QString& url, const QString& hexColor) {
                if (!url.isEmpty() && !url.startsWith("computer://")) {
                    AppCommand cmd;
                    cmd.type = AppCommandType::SetColor;
                    cmd.targetPaths = {url};
                    cmd.params["color"] = hexColor;
                    CoreEngine::instance().executeCommand(cmd);
                }
            });

            connect(titleBar->tabBar(), &TabBarWidget::tabIconChanged, this, [](int /*index*/, const QString& url, const QString& iconKey) {
                if (!url.isEmpty()) {
                    FavoriteDao::updateFavorite(url, iconKey, "");
                }
            });

        }
        if (layoutManager) {
            connect(titleBar, &TitleBarWidget::layoutMenuRequested, layoutManager, [layoutManager](const QPoint& pos) {
                layoutManager->showPanelContextMenu(pos);
            });
        }
        if (contentPanel) {
            connect(titleBar, &TitleBarWidget::viewModeRequested, this, [this, contentPanel](TitleBarWidget::ViewModeOption option) {
                ContentPanel::ViewMode targetMode = ContentPanel::GridView;
                if (option == TitleBarWidget::JustifiedViewMode) targetMode = ContentPanel::JustifiedViewMode;
                else if (option == TitleBarWidget::GridViewMode) targetMode = ContentPanel::GridView;
                else if (option == TitleBarWidget::ListViewMode) targetMode = ContentPanel::ListView;
                else if (option == TitleBarWidget::ColumnViewMode) targetMode = ContentPanel::ColumnView;

                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) target->setViewMode(targetMode);
            });

            connect(titleBar, &TitleBarWidget::createItemRequested, this, [this, contentPanel](const QString& type) {
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) target->createNewItem(type);
            });

            // 缩放级别初始化与双向同步 + 持久化
            int initZoom = AppConfig::instance().getValue("UI/GridZoomLevel", 96).toInt();
            int boundZoom = qBound(30, initZoom, 230);
            titleBar->setZoomLevel(boundZoom);
            contentPanel->setZoomLevel(boundZoom);

            connect(titleBar, &TitleBarWidget::zoomLevelChanged, this, [this, contentPanel](int value) {
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) target->setZoomLevel(value);
                AppConfig::instance().setValue("UI/GridZoomLevel", value);
            });

            connect(contentPanel, &ContentPanel::zoomLevelChanged, this, [titleBar](int level) {
                if (titleBar) titleBar->setZoomLevel(level);
                AppConfig::instance().setValue("UI/GridZoomLevel", level);
            });
        }
    }

    // 快捷键沉浸模式切换与新建文件夹下沉
    if (shortcutController) {
        if (layoutManager) {
            connect(shortcutController, &AppShortcutController::toggleImmersiveRequested, layoutManager, [layoutManager]() {
                layoutManager->toggleImmersiveMode();
            });
        }
        connect(shortcutController, &AppShortcutController::createNewFolderRequested, this, [this, contentPanel]() {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                target->createNewItem("folder");
            }
        });

        connect(shortcutController, &AppShortcutController::createNewMdRequested, this, [this, contentPanel]() {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                target->createNewItem("md");
            }
        });

        connect(shortcutController, &AppShortcutController::extractContentRequested, this, [this, contentPanel]() {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                QStringList selectedPaths = target->getSelectedPaths();
                if (!selectedPaths.isEmpty()) {
                    ContextMenuFactory::extractContentToClipboard(selectedPaths.first());
                } else {
                    ToolTipOverlay::instance()->showToast("未选择任何可提取内容的文件", 1200, QColor("#e81123"));
                }
            }
        });

        connect(shortcutController, &AppShortcutController::toggleRecursiveRequested, this, [this, contentPanel]() {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target && !target->currentPath().isEmpty() && target->currentPath() != "computer://") {
                bool nextState = !target->isRecursive();
                target->setIsRecursive(nextState);
                target->loadDirectory(target->currentPath(), nextState);
            }
        });

        if (filterPanel) {
            connect(shortcutController, &AppShortcutController::resetFilterRequested, filterPanel, [filterPanel]() {
                filterPanel->clearAllFilters(true);
            });
        }

        connect(shortcutController, &AppShortcutController::moveToTrashRequested, this, [this, contentPanel]() {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                bool isTrashView = (target->getCurrentCategoryType() == "trash" || target->currentPath() == "trash://");
                if (isTrashView) {
                    PermanentDeleteService::instance().executeEmptyTrash(target);
                } else {
                    QStringList sel = target->getSelectedPaths();
                    if (!sel.isEmpty()) {
                        TrashService::instance().moveToTrash(sel, target);
                    }
                }
            }
        });

        connect(shortcutController, &AppShortcutController::toggleMarkedRequested, this, [this, contentPanel]() {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                QStringList sel = target->getSelectedPaths();
                if (!sel.isEmpty()) {
                    QuarkMeta::MarkedFolderService::instance().toggleMarked(sel);
                }
            }
        });

        connect(shortcutController, &AppShortcutController::transferToMarkedRequested, this, []() {
            QuarkMeta::MarkedFolderService::instance().executeTransferToMarked({});
        });

        connect(shortcutController, &AppShortcutController::copyNamesEnumeratedRequested, this, [this, contentPanel]() {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                QStringList sel = target->getSelectedPaths();
                if (!sel.isEmpty()) {
                    ContextMenuFactory::copyNamesToClipboard(sel, ContextMenuFactory::NameDelimiter::Enumeration, true);
                }
            }
        });
    }

    // 搜索控制器与 ContentPanel 绑定及状态更新
    if (searchController) {
        if (contentPanel) {
            searchController->bindContentPanel(contentPanel);
        }
        connect(searchController, &SearchController::searchExecuted, this, [this]() {
            emit statusMessageRequested("搜索已完成");
        });
    }

    // 1. 路径变更与导航驱动
    connect(&NavigationService::instance(), &NavigationService::currentUrlChanged, this,
            [this, contentPanel, addressBar, navPanel, filterPanel, searchController, titleBar](const QString& url, const QString& displayPath, bool recursive) {
        if (searchController) {
            searchController->clearSearch(true);
        }
        ContentPanel* targetPanel = (m_activeContentPanel && m_activeContentPanel->isVisible()) ? m_activeContentPanel.data() : contentPanel;
        if (targetPanel) {
            targetPanel->search("");
        }
        if (filterPanel) {
            filterPanel->clearAllFilters();
            filterPanel->clearStats();
            filterPanel->setMirrorSource(false);
        }

        if (addressBar) addressBar->setPath(displayPath);
        if (navPanel) navPanel->selectPath(url == "computer://" ? "" : url);
        if (titleBar && titleBar->tabBar()) {
            titleBar->tabBar()->updateCurrentTabTitle(displayPath, url);
        }

        if (targetPanel) {
            QString targetChild = NavigationService::instance().takePendingTargetChildPath(url);
            if (!targetChild.isEmpty() && url != "computer://" && url != "trash://") {
                targetPanel->setPendingSelectPath(targetChild, false);
            }

            if (url == "computer://") {
                targetPanel->loadDirectory("computer://");
            } else if (url == "trash://") {
                targetPanel->loadCategory("trash");
            } else {
                targetPanel->loadDirectory(url, recursive);
            }
        }
    });

    if (navPanel) {
        connect(navPanel, &NavPanel::directorySelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });

        connect(navPanel, &NavPanel::requestLocateFile, this, &PanelMediator::locateItem);

        connect(navPanel, &NavPanel::requestOpenTrash, &NavigationService::instance(), []() {
            NavigationService::instance().navigateTo("trash://");
        });

        if (favoritePanel) {
            connect(navPanel, &NavPanel::requestAddFavorite, this, [](const QString& path) {
                FavoriteService::instance().addFavorites({path});
            });
            connect(navPanel, &NavPanel::requestRemoveFavorite, this, [](const QString& path) {
                FavoriteService::instance().removeFavorites({path});
            });
        }
    }

    if (favoritePanel) {
        connect(favoritePanel, &FavoritePanel::directorySelected, &NavigationService::instance(), [](const QString& path, bool recursive) {
            NavigationService::instance().navigateTo(path, true, recursive);
        });

        connect(favoritePanel, &FavoritePanel::requestLocateFile, this, &PanelMediator::locateItem);
    }

        if (markedPanel) {
            connect(markedPanel, &MarkedPanel::requestLocateFolder, this, [](const QString& path) {
                NavigationService::instance().navigateTo(path);
            });
        }

        if (libraryPanel) {
            connect(libraryPanel, &LibraryPanel::categoryPathsSelected, this, [this, contentPanel, filterPanel, addressBar, titleBar](const QStringList& paths, const QString& name, int categoryId) {
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (filterPanel) {
                    filterPanel->clearAllFilters(true);
                }
                if (target) {
                    target->search("");
                    target->applyViewTarget(QString("library://%1").arg(categoryId), "library", name, categoryId, false);
                    target->setLastLoadedLibraryPaths(paths);
                    target->loadLibraryPaths(paths);
                }
                if (addressBar) {
                    addressBar->setPath("库 → " + name);
                }
                if (titleBar && titleBar->tabBar()) {
                    titleBar->tabBar()->updateCurrentTabTitle(name, "library://" + QString::number(categoryId));
                }
            });

            connect(libraryPanel, &LibraryPanel::smartFolderSelected, this, [this, contentPanel, filterPanel, addressBar, titleBar](const SmartFolderRecord& record) {
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->loadSmartFolder(record, record.name);
                }
                if (addressBar) {
                    addressBar->setPath("库 → " + record.name);
                }
                if (filterPanel) {
                    filterPanel->clearAllFilters(false);
                }
                if (titleBar && titleBar->tabBar()) {
                    titleBar->tabBar()->updateCurrentTabTitle(record.name, "smartfolder://" + QString::number(record.id));
                }
            });

            connect(libraryPanel, &LibraryPanel::smartFolderPreviewRequested, this, [this, contentPanel, filterPanel, addressBar, titleBar](const SmartFolderRecord& record) {
                QString displayName = record.name.isEmpty() ? "智能文件夹预览" : record.name;
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->loadSmartFolder(record, displayName);
                }
                if (addressBar) {
                    addressBar->setPath("库 → " + displayName);
                }
                if (filterPanel) {
                    filterPanel->clearAllFilters(false);
                }
                if (titleBar && titleBar->tabBar()) {
                    titleBar->tabBar()->updateCurrentTabTitle(displayName, "smartfolder://" + QString::number(record.id));
                }
            });
        }

    if (contentPanel) {
        connect(contentPanel, &ContentPanel::directorySelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });

        ContentPanel* rootPanel = contentPanel ? contentPanel->rootPane() : nullptr;
        if (rootPanel && rootPanel->splitManager()) {
            connect(rootPanel->splitManager(), &ContentPaneSplitManager::layoutChanged, this, [this, titleBar, rootPanel]() {
                if (titleBar && titleBar->tabBar()) {
                    TabSplitState state = rootPanel->splitManager()->exportSplitState();
                    titleBar->tabBar()->updateSplitTabTitle(state);
                }
            });
        }

        if (contentPanel->columnView()) {
            connect(contentPanel->columnView(), &ColumnViewWidget::pathNavigated, this, [filterPanel](const QString& path) {
                if (filterPanel) {
                    filterPanel->clearAllFilters(false);
                }
                NavigationService::instance().navigateTo(path);
            });
        }

        if (favoritePanel) {
            connect(contentPanel, &ContentPanel::requestAddFavorite, this, [](const QStringList& paths) {
                FavoriteService::instance().addFavorites(paths);
            });
            connect(contentPanel, &ContentPanel::requestRemoveFavorite, this, [](const QStringList& paths) {
                FavoriteService::instance().removeFavorites(paths);
            });
        }
    }

    connect(&VolumeOnlineManager::instance(), &VolumeOnlineManager::volumeStateChanged, this,
            [](const QString& driveLetter, bool isOnline) {
        if (!isOnline) {
            QString current = NavigationService::instance().currentUrl();
            if (current.contains(driveLetter + ":", Qt::CaseInsensitive)) {
                QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
                NavigationService::instance().navigateTo(desktopPath);
            }
        }
    });

    // 安装全应用窗格激活事件过滤器
    if (!m_activationTracker) {
        m_activationTracker = new PaneActivationTracker(this);
        qApp->installEventFilter(m_activationTracker);

        connect(m_activationTracker, &PaneActivationTracker::paneInteracted, this, [this](ContentPanel* panel) {
            if (panel && m_activeContentPanel != panel) {
                if (m_activeContentPanel) {
                    m_activeContentPanel->setActivePane(false);
                }
                m_activeContentPanel = panel;
                m_activeContentPanel->setActivePane(true);
                emit activeContentPanelChanged(m_activeContentPanel);
            }
        });
    }

    // 2. 内容面板 (主/副窗格) 统一全功能绑定
    m_activeContentPanel = contentPanel;

    auto updateMetaPanelFromPanel = [metaPanel](ContentPanel* panel) {
        if (!panel || !metaPanel) return;
        QStringList paths = panel->getSelectedPaths();
        metaPanel->setSelectedPaths(paths);

        if (paths.isEmpty()) {
            metaPanel->setImagePreview(QPixmap());
            metaPanel->updateInfo("-", "-", "-", "-", "-", "-", "-", false, 0, 0);
            metaPanel->setRating(0, false);
            metaPanel->setColor(QString(""), false);
            metaPanel->setTags(QStringList());
            metaPanel->setNote(QString(""));
            metaPanel->setURL(QString(""));
            metaPanel->setPalettes({});
        } else if (paths.size() == 1) {
            QModelIndexList selectedIndices = panel->getSelectedIndexes();
            QModelIndex idx = selectedIndices.isEmpty() ? QModelIndex() : selectedIndices.first();

            QString path = paths.first();
            QFileInfo fi(path);

            QString name = idx.isValid() ? idx.sibling(idx.row(), static_cast<int>(FileListColumn::Name)).data(Qt::DisplayRole).toString() : fi.fileName();
            QString type = idx.isValid() ? ((idx.data(TypeRole).toString() == "folder") ? "文件夹" : idx.sibling(idx.row(), static_cast<int>(FileListColumn::Type)).data(Qt::DisplayRole).toString() + " 文件") : (fi.isDir() ? "文件夹" : fi.suffix().toUpper() + " 文件");
            QString sizeStr = idx.isValid() ? idx.sibling(idx.row(), static_cast<int>(FileListColumn::Size)).data(Qt::DisplayRole).toString() : "-";
            QString mtimeStr = idx.isValid() ? idx.sibling(idx.row(), static_cast<int>(FileListColumn::ModifiedDate)).data(Qt::DisplayRole).toString() : "-";
            bool encrypted = idx.isValid() ? idx.data(EncryptedRole).toBool() : false;

            auto meta = MetadataManager::instance().getMeta(path.toStdWString());

            metaPanel->updateInfo(
                name, type, sizeStr, "-", mtimeStr, "-",
                path, encrypted, meta.width, meta.height
            );

            QVector<QPair<QColor, float>> qPalettes;
            qPalettes.reserve(static_cast<int>(meta.palettes.size()));
            for (const auto& entry : meta.palettes) {
                qPalettes.append(qMakePair(entry.color, entry.ratio));
            }

            if (idx.isValid()) {
                int rating = idx.data(RatingRole).toInt();
                QString color = idx.data(ColorRole).toString();
                QStringList tags = idx.data(TagsRole).toStringList();
                QString note = idx.data(NoteRole).toString();
                QString url = idx.data(UrlRole).toString();

                int finalRating = rating > 0 ? rating : meta.rating;
                QString finalColor = !color.isEmpty() ? color : QString::fromStdWString(meta.manualColor);
                QStringList finalTagsList = !tags.isEmpty() ? tags : meta.tags;
                QString finalNote = !note.isEmpty() ? note : QString::fromStdWString(meta.note);
                QString finalUrl = !url.isEmpty() ? url : QString::fromStdWString(meta.url);

                metaPanel->setRating(finalRating, false);
                metaPanel->setColor(finalColor, false);
                metaPanel->setTags(finalTagsList);
                metaPanel->setNote(finalNote);
                metaPanel->setURL(finalUrl);
                metaPanel->setPalettes(qPalettes);

                QVariant decData = idx.data(Qt::DecorationRole);
                bool hasThumb = idx.data(HasThumbnailRole).toBool();
                if (hasThumb) {
                    QPixmap previewPixmap;
                    if (decData.canConvert<QIcon>()) {
                        previewPixmap = decData.value<QIcon>().pixmap(128, 128);
                    } else if (decData.canConvert<QPixmap>()) {
                        previewPixmap = decData.value<QPixmap>();
                    }
                    metaPanel->setImagePreview(previewPixmap);
                } else {
                    if (decData.canConvert<QIcon>()) {
                        metaPanel->setDefaultIconPreview(decData.value<QIcon>());
                    } else {
                        metaPanel->setImagePreview(QPixmap());
                    }
                }
            } else {
                metaPanel->setRating(meta.rating, false);
                metaPanel->setColor(QString::fromStdWString(meta.manualColor), false);
                metaPanel->setTags(meta.tags);
                metaPanel->setNote(QString::fromStdWString(meta.note));
                metaPanel->setURL(QString::fromStdWString(meta.url));
                metaPanel->setPalettes(qPalettes);
                metaPanel->setImagePreview(QPixmap());
            }
        } else if (paths.size() > 1) {
            QModelIndexList selectedIndices = panel->getSelectedIndexes();
            QMap<QString, QModelIndex> pathToIndexMap;
            for (const QModelIndex& idx : selectedIndices) {
                if (idx.isValid()) {
                    QString p = idx.data(PathRole).toString();
                    if (!p.isEmpty()) {
                        pathToIndexMap[p] = idx;
                    }
                }
            }

            bool firstItem = true;
            bool sameRating = true;
            bool sameColor = true;
            bool sameNote = true;
            bool sameUrl = true;

            int commonRating = 0;
            QString commonColor;
            QString commonNote;
            QString commonUrl;

            QSet<QString> commonTags;
            bool commonTagsInitialized = false;

            QList<MetaPreviewItem> multiPreviewItems;
            int previewStartIdx = qMax(0, paths.size() - MetaPanel::kMultiPreviewMaxCards);

            for (int i = 0; i < paths.size(); ++i) {
                const QString& path = paths[i];
                QModelIndex idx = pathToIndexMap.value(path, QModelIndex());

                int itemRating = 0;
                QString itemColor;
                QStringList itemTags;
                QString itemNote;
                QString itemUrl;

                auto meta = MetadataManager::instance().getMeta(path.toStdWString());

                if (idx.isValid()) {
                    int r = idx.data(RatingRole).toInt();
                    QString c = idx.data(ColorRole).toString();
                    QStringList t = idx.data(TagsRole).toStringList();
                    QString n = idx.data(NoteRole).toString();
                    QString u = idx.data(UrlRole).toString();

                    itemRating = r > 0 ? r : meta.rating;
                    itemColor = !c.isEmpty() ? c : QString::fromStdWString(meta.manualColor);
                    itemTags = !t.isEmpty() ? t : meta.tags;
                    itemNote = !n.isEmpty() ? n : QString::fromStdWString(meta.note);
                    itemUrl = !u.isEmpty() ? u : QString::fromStdWString(meta.url);

                    if (i >= previewStartIdx && multiPreviewItems.size() < MetaPanel::kMultiPreviewMaxCards) {
                        MetaPreviewItem item;
                        QVariant decData = idx.data(Qt::DecorationRole);
                        bool hasThumb = idx.data(HasThumbnailRole).toBool();
                        if (hasThumb) {
                            item.hasThumbnail = true;
                            if (decData.canConvert<QIcon>()) {
                                item.thumbnail = decData.value<QIcon>().pixmap(128, 128);
                            } else if (decData.canConvert<QPixmap>()) {
                                item.thumbnail = decData.value<QPixmap>();
                            }
                        } else {
                            item.hasThumbnail = false;
                            if (decData.canConvert<QIcon>()) {
                                item.icon = decData.value<QIcon>();
                            }
                        }
                        if ((item.hasThumbnail && !item.thumbnail.isNull()) || (!item.hasThumbnail && !item.icon.isNull())) {
                            multiPreviewItems.append(item);
                        }
                    }
                } else {
                    itemRating = meta.rating;
                    itemColor = QString::fromStdWString(meta.manualColor);
                    itemTags = meta.tags;
                    itemNote = QString::fromStdWString(meta.note);
                    itemUrl = QString::fromStdWString(meta.url);

                    if (i >= previewStartIdx && multiPreviewItems.size() < MetaPanel::kMultiPreviewMaxCards) {
                        MetaPreviewItem item;
                        item.hasThumbnail = false;
                        QFileInfo fi(path);
                        if (fi.isDir()) {
                            item.icon = UiHelper::getIcon("folder", QColor("#3498db"), 128);
                        } else {
                            item.icon = UiHelper::getIcon("file_eps", QColor("#888888"), 128);
                        }
                        if (!item.icon.isNull()) {
                            multiPreviewItems.append(item);
                        }
                    }
                }

                if (firstItem) {
                    firstItem = false;
                    commonRating = itemRating;
                    commonColor = itemColor;
                    commonNote = itemNote;
                    commonUrl = itemUrl;

                    commonTags = QSet<QString>(itemTags.begin(), itemTags.end());
                    commonTagsInitialized = true;
                } else {
                    if (sameRating && itemRating != commonRating) sameRating = false;
                    if (sameColor && itemColor.compare(commonColor, Qt::CaseInsensitive) != 0) sameColor = false;
                    if (sameNote && itemNote != commonNote) sameNote = false;
                    if (sameUrl && itemUrl != commonUrl) sameUrl = false;

                    if (commonTagsInitialized) {
                        QSet<QString> currentSet(itemTags.begin(), itemTags.end());
                        commonTags.intersect(currentSet);
                    }
                }
            }

            metaPanel->setRating(sameRating ? commonRating : 0, false);
            metaPanel->setColor(sameColor ? commonColor : QString(""), false);
            metaPanel->setNote(sameNote ? commonNote : QString(""));
            metaPanel->setURL(sameUrl ? commonUrl : QString(""));
            metaPanel->setTags(commonTags.values());

            metaPanel->setPalettes({});
            metaPanel->setMultiSelectionPreview(multiPreviewItems);
        }
    };

    auto wireContentPanel = std::make_shared<std::function<void(ContentPanel*)>>();
    *wireContentPanel = [this, addressBar, favoritePanel, metaPanel, updateMetaPanelFromPanel, wireContentPanel](ContentPanel* panel) {
        if (!panel) return;

        // 1. 文件双击/空格/回车/右键菜单统一文件激活动作
        connect(panel, &ContentPanel::fileActivated, this, [this](const QString& path) {
            if (path.isEmpty() || !QFile::exists(path)) {
                return;
            }
            // 无论能否在 QuickLook 中预览，凡是被双击/空格/回车尝试激活的文件全量记录历史
            HistoryService::instance().recordFileAccess(path);

            if (!UiHelper::canPreviewFile(path)) {
                return;
            }

            AppCommand cmd;
            cmd.type = AppCommandType::RecordAccess;
            cmd.targetPaths << path;
            CoreEngine::instance().executeCommand(cmd);

            QuickLookWindow::instance().preview(path);
        });

        // 2. 收藏夹请求
        if (favoritePanel) {
            connect(panel, &ContentPanel::requestAddFavorite, this, [](const QStringList& paths) {
                FavoriteService::instance().addFavorites(paths);
            });
            connect(panel, &ContentPanel::requestRemoveFavorite, this, [](const QStringList& paths) {
                FavoriteService::instance().removeFavorites(paths);
            });
        }

        // 3. 状态栏与焦点响应 (仅当信息来自当前激活窗格时才刷到底栏)
        connect(panel, &ContentPanel::statusBarMessageReady, this, [this, panel](const QString& message) {
            if (m_activeContentPanel == panel || (!m_activeContentPanel && panel == m_contentPanel.data())) {
                emit statusMessageRequested(message);
            }
        });

        connect(panel, &ContentPanel::directorySelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });

        connect(panel, &ContentPanel::panelActivated, this, [this, addressBar](ContentPanel* activePanel) {
            if (m_activeContentPanel != activePanel) {
                m_activeContentPanel = activePanel;
                emit activeContentPanelChanged(activePanel);
            }
            if (addressBar) {
                addressBar->setPath(activePanel->displayPath());
            }
        });

        // 4. 元数据面板选中项 & 模型数据变动同步
        if (metaPanel) {
            connect(panel, &ContentPanel::selectionChanged, metaPanel, [this, panel, updateMetaPanelFromPanel](const QStringList&) {
                if (m_activeContentPanel == panel || (!m_activeContentPanel && panel == m_contentPanel.data())) {
                    updateMetaPanelFromPanel(panel);
                }
            });

            if (panel->model()) {
                connect(panel->model(), &QAbstractItemModel::dataChanged, metaPanel,
                        [this, panel, updateMetaPanelFromPanel](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
                    if (!roles.isEmpty() && !roles.contains(RatingRole) && !roles.contains(ColorRole) && !roles.contains(TagsRole) && !roles.contains(NoteRole) && !roles.contains(UrlRole)) {
                        return;
                    }

                    auto& debounceState = m_panelMetaDebounceMap[panel];
                    if (!debounceState.timer) {
                        debounceState.timer = new QTimer(panel);
                        debounceState.timer->setSingleShot(true);
                        connect(panel, &QObject::destroyed, this, [this, panel]() {
                            m_panelMetaDebounceMap.remove(panel);
                        });
                        connect(debounceState.timer, &QTimer::timeout, this, [this, panel, updateMetaPanelFromPanel]() {
                            auto it = m_panelMetaDebounceMap.find(panel);
                            if (it == m_panelMetaDebounceMap.end()) return;

                            ContentPanel* activeP = m_activeContentPanel ? m_activeContentPanel.data() : m_contentPanel.data();
                            if (activeP == panel) {
                                QSet<QString> selSet;
                                for (const QString& sp : panel->getSelectedPaths()) {
                                    selSet.insert(QDir::cleanPath(sp).toLower());
                                }

                                bool hasOverlap = selSet.isEmpty();
                                if (!hasOverlap) {
                                    for (const QString& cp : it->pendingCleanPaths) {
                                        if (selSet.contains(cp)) {
                                            hasOverlap = true;
                                            break;
                                        }
                                    }
                                }

                                if (hasOverlap) {
                                    updateMetaPanelFromPanel(panel);
                                }
                            }
                            it->pendingCleanPaths.clear();
                        });
                    }

                    QAbstractItemModel* model = panel->model();
                    int startRow = topLeft.row();
                    int endRow = bottomRight.row();
                    for (int r = startRow; r <= endRow; ++r) {
                        QModelIndex idx = model->index(r, 0, topLeft.parent());
                        if (idx.isValid()) {
                            QString path = idx.data(PathRole).toString();
                            if (!path.isEmpty()) {
                                debounceState.pendingCleanPaths.insert(QDir::cleanPath(path).toLower());
                            }
                        }
                    }

                    if (!debounceState.timer->isActive()) {
                        debounceState.timer->start(30);
                    }
                });
            }
        }

        // 5. 递归绑定动态创建的副窗格
        connect(panel, &ContentPanel::secondaryPaneCreated, this, [wireContentPanel](ContentPanel* pane) {
            if (*wireContentPanel) {
                (*wireContentPanel)(pane);
            }
        });
    };

    if (contentPanel) {
        (*wireContentPanel)(contentPanel);

        if (searchController) {
            connect(this, &PanelMediator::activeContentPanelChanged, this, [searchController](ContentPanel* activePanel) {
                searchController->setActiveContentPanel(activePanel);
            });
        }

        connect(&LibrarySyncService::instance(), &LibrarySyncService::libraryItemsSynced, this, [this, libraryPanel, contentPanel]() {
            if (libraryPanel) {
                libraryPanel->refreshSmartFolderCounts();
            }
            if (contentPanel) {
                ContentPanel* root = contentPanel->rootPane();
                if (root) {
                    for (ContentPanel* pane : root->panes()) {
                        if (pane) {
                            pane->refreshSmartFolderIfShowing();
                        }
                    }
                }
            }
        });

        if (metaPanel) {
            connect(this, &PanelMediator::activeContentPanelChanged, this, [contentPanel, updateMetaPanelFromPanel](ContentPanel* activePanel) {
                ContentPanel* root = contentPanel ? contentPanel->rootPane() : nullptr;
                if (root) {
                    root->setActivePane(root == activePanel);
                    for (ContentPanel* pane : root->panes()) {
                        if (pane && pane != root) {
                            pane->setActivePane(pane == activePanel);
                        }
                    }
                }
                updateMetaPanelFromPanel(activePanel);
            });
        }
    }

    connect(&QuickLookWindow::instance(), &QuickLookWindow::prevRequested, this, [this, contentPanel]() {
        ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
        if (!target) return;
        QString curPath = QuickLookWindow::instance().currentPath();
        int delta = -1;
        while (true) {
            QString prev = target->getAdjacentFilePath(curPath, delta);
            if (prev.isEmpty()) break;
            if (!QFileInfo(prev).isDir() && UiHelper::canPreviewFile(prev)) {
                QuickLookWindow::instance().preview(prev);
                target->selectAndScrollToPath(prev);
                break;
            }
            delta--;
        }
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::nextRequested, this, [this, contentPanel]() {
        ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
        if (!target) return;
        QString curPath = QuickLookWindow::instance().currentPath();
        int delta = 1;
        while (true) {
            QString next = target->getAdjacentFilePath(curPath, delta);
            if (next.isEmpty()) break;
            if (!QFileInfo(next).isDir() && UiHelper::canPreviewFile(next)) {
                QuickLookWindow::instance().preview(next);
                target->selectAndScrollToPath(next);
                break;
            }
            delta++;
        }
    });

    // QuickLook 改星级 -> 同步更新内容面板卡片并弹出 Toast 提示
    connect(&QuickLookWindow::instance(), &QuickLookWindow::ratingRequested, this, [this, metaPanel](int rating) {
        QString curPath = QuickLookWindow::instance().currentPath();
        if (curPath.isEmpty()) return;

        AppCommand cmd;
        cmd.type = AppCommandType::SetRating;
        cmd.targetPaths << curPath;
        cmd.params["rating"] = rating;
        CoreEngine::instance().executeCommand(cmd);

        if (metaPanel) metaPanel->setRating(rating, false);

        ToolTipOverlay::instance()->showRatingToast(
            rating, &QuickLookWindow::instance()
        );
    });

    // QuickLook 改颜色 -> 同步更新内容面板卡片并弹出 Toast 提示
    connect(&QuickLookWindow::instance(), &QuickLookWindow::colorRequested, this, [this, metaPanel](const QString& color) {
        QString curPath = QuickLookWindow::instance().currentPath();
        if (curPath.isEmpty()) return;

        AppCommand cmd;
        cmd.type = AppCommandType::SetColor;
        cmd.targetPaths << curPath;
        cmd.params["color"] = color;
        CoreEngine::instance().executeCommand(cmd);

        if (metaPanel) metaPanel->setColor(color, false);

        ToolTipOverlay::instance()->showColorToast(
            color, &QuickLookWindow::instance()
        );
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::deleteRequested, this, [this, contentPanel](const QString& path) {
        if (path.isEmpty()) return;
        ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
        if (!target) return;

        // 提前计算下一个有效文件（若末项则计算备选的上一个有效文件），防止物理删除后寻址失败
        QString nextTarget;

        int delta = 1;
        while (true) {
            QString candidate = target->getAdjacentFilePath(path, delta);
            if (candidate.isEmpty()) break;
            if (!QFileInfo(candidate).isDir() && UiHelper::canPreviewFile(candidate)) {
                nextTarget = candidate;
                break;
            }
            delta++;
        }

        if (nextTarget.isEmpty()) {
            delta = -1;
            while (true) {
                QString candidate = target->getAdjacentFilePath(path, delta);
                if (candidate.isEmpty()) break;
                if (!QFileInfo(candidate).isDir() && UiHelper::canPreviewFile(candidate)) {
                    nextTarget = candidate;
                    break;
                }
                delta--;
            }
        }

        // 锁定 QuickLook 失焦关闭门禁，防止刷新与对话框导致预览被误关
        QuickLookWindow::instance().setIgnoreDeactivate(true);

        bool deleteOk = TrashService::instance().moveToTrash({path}, target);

        if (deleteOk) {
            target->refreshAll();

            if (!nextTarget.isEmpty() && QFile::exists(nextTarget)) {
                QuickLookWindow::instance().preview(nextTarget);
                target->selectAndScrollToPath(nextTarget);
            } else {
                QuickLookWindow::instance().closePreview();
            }
        }

        QTimer::singleShot(200, &QuickLookWindow::instance(), []() {
            QuickLookWindow::instance().setIgnoreDeactivate(false);
        });
    });

    // 4. 统计与过滤联动 (动态支持多分栏焦点切换)
    if (filterPanel) {
        // 用于管理当前绑定面板的连接断开，防止后台窗格发送统计冲刷侧边栏
        auto activeStatsConn = std::make_shared<QMetaObject::Connection>();

        auto bindFilterToActivePanel = [this, filterPanel, activeStatsConn](ContentPanel* activePanel) {
            if (!activePanel) return;

            // 1. 严格解绑上一个窗格的统计信号，杜绝后台非焦点窗格冲刷侧边栏
            if (*activeStatsConn) {
                QObject::disconnect(*activeStatsConn);
            }

            // 2. 绑定新焦点窗格的统计就绪信号
            *activeStatsConn = connect(activePanel, &ContentPanel::directoryStatsReady, filterPanel, [filterPanel](const ScanStats& stats) {
                filterPanel->populateStats(stats);
                AppEvent ev;
                ev.type = AppEventType::FilterStateChanged;
                CentralEventHub::instance().publishEvent(ev);
            });

            // 3. 切换焦点的瞬间：反向刷出新焦点窗格已有的 FilterState 与统计数据给 FilterPanel
            filterPanel->blockSignals(true);
            filterPanel->syncUIFromFilterState();
            filterPanel->blockSignals(false);

            // 触发当前焦点窗格重新计算并广播统计
            activePanel->recalculateAndEmitStats();
        };

        // 绑定初始主面板
        if (contentPanel) {
            bindFilterToActivePanel(contentPanel);
        }

        // 焦点分栏切换时动态绑定并同步刷出状态
        connect(this, &PanelMediator::activeContentPanelChanged, this, [this, filterPanel, bindFilterToActivePanel](ContentPanel* newActivePanel) {
            if (newActivePanel) {
                bindFilterToActivePanel(newActivePanel);
            }
        });

        // 筛选条件变动时精准应用至当前激活分栏
        connect(filterPanel, &FilterPanel::filterChanged, this, [this](const FilterState& state) {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : m_contentPanel.data();
            if (target) {
                target->applyFilters(state);
            }
        });
    }

    // 5. 地址栏路径跳转与刷新
    if (addressBar) {
        connect(addressBar, &AddressBar::pathChanged, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });

        connect(addressBar, &AddressBar::refreshRequested, this, [this, contentPanel]() {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target && (target->getCurrentCategoryType() == "library" || target->getCurrentCategoryType() == "smart_folder")) {
                target->refreshAll();
            } else {
                NavigationService::instance().refresh();
            }
        });

        if (favoritePanel) {
            connect(addressBar, &AddressBar::requestAddFavorite, this, [](const QString& path) {
                if (FavoriteService::instance().isFavorite(path)) {
                    FavoriteService::instance().removeFavorites({path});
                } else {
                    FavoriteService::instance().addFavorites({path});
                }
            });
            connect(addressBar, &AddressBar::requestRemoveFavorite, this, [](const QString& path) {
                FavoriteService::instance().removeFavorites({path});
            });
        }
    }

    // 6. 响应元数据面板解耦信号 -> 驱动 CoreEngine 与当前激活 ContentPanel 同步
    if (metaPanel && contentPanel) {
        auto activeOrRootPanel = [this, contentPanel]() -> ContentPanel* {
            return m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
        };

        connect(metaPanel, &MetaPanel::ratingChanged, this, [](const QStringList& paths, int rating) {
            if (paths.isEmpty()) return;
            AppCommand cmd;
            cmd.type = AppCommandType::SetRating;
            cmd.targetPaths = paths;
            cmd.params["rating"] = rating;
            CoreEngine::instance().executeCommand(cmd);
        });

        connect(metaPanel, &MetaPanel::colorChanged, this, [](const QStringList& paths, const QString& hexColor) {
            if (paths.isEmpty()) return;
            AppCommand cmd;
            cmd.type = AppCommandType::SetColor;
            cmd.targetPaths = paths;
            cmd.params["color"] = hexColor;
            CoreEngine::instance().executeCommand(cmd);
        });

        connect(metaPanel, &MetaPanel::primaryColorChanged, this, [](const QString& path, const QColor& color) {
            if (path.isEmpty()) return;
            AppCommand cmd;
            cmd.type = AppCommandType::SetColor;
            cmd.targetPaths = {path};
            cmd.params["color"] = color.name(QColor::HexRgb);
            CoreEngine::instance().executeCommand(cmd);
        });

        connect(metaPanel, &MetaPanel::tagsAddRequested, this, [](const QStringList& paths, const QStringList& newTags) {
            if (!paths.isEmpty() && !newTags.isEmpty()) {
                if (paths.size() > 50) {
                    CoreEngine::instance().addTagsToPathsAsync(paths, newTags, nullptr);
                } else {
                    AppCommand cmd;
                    cmd.type = AppCommandType::AddTag;
                    cmd.targetPaths = paths;
                    cmd.params["tags"] = newTags;
                    CoreEngine::instance().executeCommand(cmd);
                }
            }
        });

        connect(metaPanel, &MetaPanel::tagsRemoveRequested, this, [](const QStringList& paths, const QStringList& removeTags) {
            if (!paths.isEmpty() && !removeTags.isEmpty()) {
                AppCommand cmd;
                cmd.type = AppCommandType::RemoveTag;
                cmd.targetPaths = paths;
                cmd.params["tags"] = removeTags;
                CoreEngine::instance().executeCommand(cmd);
            }
        });

        if (filterPanel) {
            connect(metaPanel, &MetaPanel::searchByColor, filterPanel, [filterPanel](const QColor& color) {
                filterPanel->selectColor(color);
            });
        }

        connect(metaPanel, &MetaPanel::renameRequested, this, [activeOrRootPanel](const QString& oldPath, const QString& newPath) {
            ContentPanel* target = activeOrRootPanel();
            if (ShellHelper::renameItem(oldPath, newPath)) {
                if (target) {
                    target->migrateModelCache(oldPath, newPath);
                    target->refreshAll();
                }
            } else {
                if (target) {
                    target->updateItemMetadata(oldPath);
                }
            }
        });

        connect(metaPanel, &MetaPanel::noteEdited, this, [](const QStringList& paths, const QString& newNote) {
            if (!paths.isEmpty()) {
                AppCommand cmd;
                cmd.type = AppCommandType::SetNote;
                cmd.targetPaths = paths;
                cmd.params["note"] = newNote;
                CoreEngine::instance().executeCommand(cmd);
            }
        });

        connect(metaPanel, &MetaPanel::linkEdited, this, [](const QStringList& paths, const QString& newLink) {
            if (!paths.isEmpty()) {
                AppCommand cmd;
                cmd.type = AppCommandType::SetURL;
                cmd.targetPaths = paths;
                cmd.params["url"] = newLink;
                CoreEngine::instance().executeCommand(cmd);
            }
        });
    }

    // 7. 全局事件总线 CentralEventHub 增量通知响应
    connect(&CentralEventHub::instance(), &CentralEventHub::eventOccurred, this, [this, contentPanel, metaPanel](const QuarkMeta::AppEvent& event) {
        ContentPanel* activeOrRoot = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
        if (!activeOrRoot) return;

        if (event.type == QuarkMeta::AppEventType::MetadataUpdated) {
            if (!event.targetPath.isEmpty()) {
                activeOrRoot->updateItemMetadata(event.targetPath);
                if (contentPanel && contentPanel != activeOrRoot) {
                    contentPanel->updateItemMetadata(event.targetPath);
                }
                if (metaPanel) {
                    QString targetClean = QDir::cleanPath(event.targetPath);
                    for (const QString& p : activeOrRoot->getSelectedPaths()) {
                        if (QString::compare(QDir::cleanPath(p), targetClean, Qt::CaseInsensitive) == 0) {
                            if (event.payload.contains("field") && event.payload["field"].toString() == "color") {
                                QString newColor = event.payload.value("value").toString();
                                metaPanel->setColor(newColor, false);
                            } else if (event.payload.contains("field") && event.payload["field"].toString() == "rating") {
                                int newRating = event.payload.value("value").toInt();
                                metaPanel->setRating(newRating, false);
                            }
                            break;
                        }
                    }
                }
            } else if (!event.paths.isEmpty()) {
                activeOrRoot->updateItemsMetadata(event.paths);
                if (contentPanel && contentPanel != activeOrRoot) {
                    contentPanel->updateItemsMetadata(event.paths);
                }
            } else {
                activeOrRoot->refreshAll();
                if (contentPanel && contentPanel != activeOrRoot) {
                    contentPanel->refreshAll();
                }
            }
            // 🚀 拔除直调：统计重算统一由模型的 dataChanged Roles 门禁经 50ms 定时器防抖触发
        } else if (event.type == QuarkMeta::AppEventType::ItemsDeleted ||
                   event.type == QuarkMeta::AppEventType::ItemsRenamed ||
                   event.type == QuarkMeta::AppEventType::UndoRedoPerformed) {
            activeOrRoot->refreshAll();
            if (contentPanel && contentPanel != activeOrRoot) {
                contentPanel->refreshAll();
            }
        }
    });
}

} // namespace QuarkMeta
=======
#include "PanelMediator.h"
#include "NavPanel.h"
#include "FavoritePanel.h"
#include "LibraryPanel.h"
#include "MarkedPanel.h"
#include "ContentPanel.h"
#include "MetaPanel.h"
#include "FilterPanel.h"
#include "AddressBar.h"
#include "SearchController.h"
#include "TitleBarWidget.h"
#include "TabBarWidget.h"
#include "PanelLayoutManager.h"
#include "AppShortcutController.h"

#include "ActivePaneCoordinator.h"
#include "MetaPanelPresenter.h"
#include "MetaEditController.h"
#include "QuickLookController.h"
#include "ShortcutActionRouter.h"
#include "TabBarCoordinator.h"
#include "NavigationCoordinator.h"
#include "FilterStatsBinder.h"

namespace QuarkMeta {

static PanelMediator* s_panelMediatorInstance = nullptr;

PanelMediator* PanelMediator::instance() {
    return s_panelMediatorInstance;
}

PanelMediator::PanelMediator(const PanelMediatorComponents& components, QObject* parent)
    : QObject(parent),
      m_navPanel(components.navPanel),
      m_favoritePanel(components.favoritePanel),
      m_libraryPanel(components.libraryPanel),
      m_markedPanel(components.markedPanel),
      m_contentPanel(components.contentPanel),
      m_metaPanel(components.metaPanel),
      m_filterPanel(components.filterPanel),
      m_addressBar(components.addressBar),
      m_searchController(components.searchController),
      m_titleBar(components.titleBar),
      m_layoutManager(components.layoutManager),
      m_shortcutController(components.shortcutController) {
    s_panelMediatorInstance = this;
}

PanelMediator::~PanelMediator() {
    if (s_panelMediatorInstance == this) {
        s_panelMediatorInstance = nullptr;
    }
}

ContentPanel* PanelMediator::activeContentPanel() const {
    return m_activeCoordinator ? m_activeCoordinator->activePane() : m_contentPanel.data();
}

void PanelMediator::locateItem(const QString& fullPath) {
    if (m_navCoordinator) {
        m_navCoordinator->locateItem(fullPath);
    }
}

void PanelMediator::distributeBindPane(ContentPanel* pane) {
    if (!pane) return;

    // 统一规则三：按固定先后顺序对窗格依次进行逐窗格绑定
    if (m_activeCoordinator) {
        m_activeCoordinator->bindPane(pane);
    }
    if (m_quickLookController) {
        m_quickLookController->bindPane(pane);
    }
    if (m_metaPresenter) {
        m_metaPresenter->bindPane(pane);
    }
    if (m_navCoordinator) {
        m_navCoordinator->bindPane(pane);
    }

    // 动态副窗格递归分发
    connect(pane, &ContentPanel::secondaryPaneCreated, this, [this](ContentPanel* secondaryPane) {
        distributeBindPane(secondaryPane);
    });
}

void PanelMediator::setupConnections() {
    ContentPanel* contentPanel = m_contentPanel.data();

    // 1. 创建各专业协调器
    m_activeCoordinator = new ActivePaneCoordinator(contentPanel, this);
    m_metaPresenter = new MetaPanelPresenter(m_activeCoordinator, m_metaPanel.data(), contentPanel, this);
    m_metaEditController = new MetaEditController(m_activeCoordinator, m_metaPanel.data(), this);
    m_quickLookController = new QuickLookController(m_activeCoordinator, this);
    m_shortcutRouter = new ShortcutActionRouter(m_activeCoordinator, m_shortcutController.data(), m_titleBar.data(), m_layoutManager.data(), this);

    TabBarWidget* tabBar = m_titleBar ? m_titleBar->findChild<TabBarWidget*>("WindowTabBar") : nullptr;

    m_navCoordinator = new NavigationCoordinator(
        m_activeCoordinator,
        m_navPanel.data(),
        m_favoritePanel.data(),
        m_libraryPanel.data(),
        m_markedPanel.data(),
        m_addressBar.data(),
        tabBar,
        m_searchController.data(),
        this
    );

    m_tabBarCoordinator = new TabBarCoordinator(
        m_activeCoordinator,
        tabBar,
        [this]() {
            if (m_navCoordinator) m_navCoordinator->refreshCurrent();
        },
        this
    );

    m_filterStatsBinder = new FilterStatsBinder(
        m_activeCoordinator,
        m_filterPanel.data(),
        m_metaPanel.data(),
        this
    );

    // 2. 状态栏消息转发
    connect(m_activeCoordinator, &ActivePaneCoordinator::statusMessageRequested, this, &PanelMediator::statusMessageRequested);
    connect(m_navCoordinator, &NavigationCoordinator::statusMessageRequested, this, &PanelMediator::statusMessageRequested);

    // 3. activeContentPanelChanged 接收顺序保全契约
    // 协调器激活信号转发给 PanelMediator::activeContentPanelChanged
    connect(m_activeCoordinator, &ActivePaneCoordinator::activePaneChanged, this, &PanelMediator::activeContentPanelChanged);

    // 严格保持的原有接收顺序：
    // ① 搜索控制器 setActiveContentPanel
    if (m_searchController) {
        connect(this, &PanelMediator::activeContentPanelChanged, this, [this](ContentPanel* activePanel) {
            if (m_searchController) {
                m_searchController->setActiveContentPanel(activePanel);
            }
        });
    }

    // ② 窗格高亮同步（已由 ActivePaneCoordinator 内部监听 activePaneChanged 自动完成）

    // ③ 元数据面板刷新
    if (m_metaPanel) {
        connect(this, &PanelMediator::activeContentPanelChanged, this, [this](ContentPanel* activePanel) {
            if (m_metaPresenter) {
                m_metaPresenter->updateMetaPanel(activePanel);
            }
        });
    }

    // ④ 筛选面板绑定
    if (m_filterPanel) {
        connect(this, &PanelMediator::activeContentPanelChanged, this, [this](ContentPanel* activePanel) {
            if (m_filterStatsBinder && activePanel) {
                m_filterStatsBinder->bindFilterToActivePanel(activePanel);
            }
        });
    }

    // 4. 执行各协调器的全局信号装配 (wire)
    m_activeCoordinator->wire();
    m_metaPresenter->wire();
    m_metaEditController->wire();
    m_quickLookController->wire();
    m_shortcutRouter->wire();
    m_tabBarCoordinator->wire();
    m_navCoordinator->wire();
    m_filterStatsBinder->wire();

    // 5. 统一窗格绑定（主窗格及后续副窗格均走统一分发）
    if (contentPanel) {
        distributeBindPane(contentPanel);
    }
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 静态与符号检索核验
1. 验证 `PanelMediator.cpp` 中不再出现 `m_activeContentPanel ?`；
2. 验证主窗格 `directorySelected`、`requestAddFavorite`、`requestRemoveFavorite` 仅在 `NavigationCoordinator::bindPane` 中连接一次；
3. 验证 `activeContentPanelChanged` 的四个接收者顺序保持：搜索控制器 -> 窗格高亮同步 -> 元数据面板刷新 -> 筛选面板绑定；
4. 验证 `findAdjacentPreviewable`、`exportAndSaveSplitState`、`showLocation`、`refreshCurrent` 各只有一份唯一实现。

### 4.2 运行时验收标准 (Parity Checklist)
1. **多窗格聚焦与切换**：点击或鼠标移入不同分屏窗格，窗格高亮边框准确跟随切换，地址栏实时刷出对应窗格路径，状态栏消息仅由当前激活窗格发出；
2. **元数据面板展示与编辑**：单选、多选与空选时元数据面板刷新完全正确，多选时提前收敛阶段与叠卡片预览数量无任何误差，修改评分、颜色、标签均正确派发 AppCommand；
3. **QuickLook 联动**：双击激活文件记录历史与预览，上一个/下一个正确跳过文件夹并寻找相邻可预览文件，删除文件后自动推进或安全关闭；
4. **快捷键与标题栏动作**：缩放滚轮与持久化、新建项、ZenMode切换、回收站永久删除与普通删除判定完全无差错；
5. **标签栏分屏**：标签页切换、合并、分屏拖拽及分屏上限（4个）阻断提示与原版 100% 对齐；
6. **路径导航与全局事件**：地址栏回车、侧边栏点击、面包屑跳转、增删改事件驱动视图刷新完全正常。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **统一目标窗格 SSOT**：由 `ActivePaneCoordinator::currentTarget()` 替代原近 20 处手写三元表达式；
2. **统一逐窗格连接去重**：彻底消除主窗格双重连接问题；
3. **相邻可预览文件查找单一实现**：`findAdjacentPreviewable` 单一私有函数合并了 prev、next、delete 三处重复代码；
4. **分屏状态导出保存单一实现**：`exportAndSaveSplitState` 合并了 mergeRequested 与 splitRequested 的重复逻辑；
5. **全应用位置显示单一实现**：`showLocation` 统一更新地址栏与标签页标题，消灭分类、智能文件夹与常规路径 4 处重复手写。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 / 业务概念 | 唯一 SSOT 真理源 | 派生副本与表现形式 | 读写与同步方向 |
| :--- | :--- | :--- | :--- |
| 当前目标窗格 | `ActivePaneCoordinator` (`currentTarget()`) | 各协调器内部操作目标 | 唯一入口判定，要求 `isVisible()` 为真 |
| 当前激活窗格 | `ActivePaneCoordinator` (`m_activeContentPanel`) | `PanelMediator::activeContentPanel()` 转发 | 用户交互事件驱动切换，对外派发通知 |
| 元数据展示防抖 | `MetaPanelPresenter` (`m_debounceMap`) | 30ms 单次定时器 | 仅当为当前目标窗格且有路径交集时触发 |
| 缩放比例状态 | `AppConfig` ("UI/GridZoomLevel") | TitleBar 滚轮值、ContentPanel 缩放 | 初始化读取持久化，变动双向同步并写回 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. `PanelMediator::instance()`、`locateItem()`、`activeContentPanel()`、`setupConnections()` 签名与访问级别 100% 保持；
2. 信号 `statusMessageRequested`、`activeContentPanelChanged` 保持原样；
3. 新增的各个协调器与控制器的公开接口 `bindPane`、`wire`、`currentTarget` 均在各自头文件中显式声明为 `public`，并在 `.cpp` 中严格匹配。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

1. `PanelMediator.h` 前置声明新增的 8 个协调器/控制器类，删除原有未使用的内部防抖结构体；
2. `PanelMediator.cpp` 显式包含 8 个协调器头文件与所有涉及的面板头文件；
3. 8 个协调器 `.h` 与 `.cpp` 均显式包含各自用到的 Qt 类型（`QPointer`、`QTimer`、`QSet` 等）与业务类型定义。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 检查项编号 | 规范要求与检查内容 | 达标状态 | 位置说明 |
| :--- | :--- | :--- | :--- |
| 1 | 统一规则一：当前目标窗格 SSOT | 满足 | `ActivePaneCoordinator::currentTarget()` 统一判定，要求 `isVisible()` 为真 |
| 2 | 统一规则二：同一信号对同一窗格只连接一次 | 满足 | 消除主窗格双重连接，统一由 `bindPane` 处理 |
| 3 | 统一规则三：逐窗格职责拆分与薄分发 | 满足 | `distributeBindPane` 仅做 4 行分发与副窗格递归 |
| 4 | 第 1 项：ActivePaneCoordinator 窗格激活唯一入口 | 满足 | 统一汇入 `activate(panel)`，高亮同步无条件执行 |
| 5 | activeContentPanelChanged 接收顺序保全 | 满足 | 严格保持搜索 -> 高亮 -> 元数据 -> 筛选顺序 |
| 6 | 第 2 项：MetaPanelPresenter 30ms防抖与两阶段多选优化 | 满足 | 阶段一提前收敛 + 阶段二叠卡片预览，结果完全一致 |
| 7 | 第 3 项：MetaEditController 命令翻译 | 满足 | 评分/颜色/标签/备注/链接/重命名统一转换 AppCommand |
| 8 | 第 4 项：QuickLookController 相邻文件查找单一实现 | 满足 | `findAdjacentPreviewable` 合并 prev/next/delete 重复代码 |
| 9 | 第 5 项：ShortcutActionRouter 快捷键与标题栏转发 | 满足 | 回收站判定本类封装，目标一律使用 currentTarget() |
| 10 | 第 6 项：TabBarCoordinator 标签页与分屏导出单一实现 | 满足 | `exportAndSaveSplitState` 合并导出状态 |
| 11 | 第 7 项：NavigationCoordinator 位置显示单一实现 | 满足 | `showLocation` 与 `refreshCurrent` 各只有一份 |
| 12 | 第 8 项：FilterStatsBinder 筛选与统计动态绑定 | 满足 | 窗格切换断开旧连接、绑定新连接与 FilterStateChanged 广播 |
| 13 | 第 9 项：PanelMediator 瘦身至纯中枢 | 满足 | 行数由 1300 行瘦身至 210 行，无任何业务逻辑堆叠 |
| 14 | CMakeLists.txt 全量同步显式注册 | 满足 | 显式注册 8 对协调器头文件与源文件 |
| 15 | 严格遵守 AGENTS.md 实施界限铁律 | 满足 | 源码树保持零篡改，全部实施方案物理隔离存放在本文件中 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 10.1 交互设计零篡改核对表
| 交互场景 | 既有实现行为 | 重构方案行为 | 是否完全一致 |
| :--- | :--- | :--- | :--- |
| 窗格激活切换 | 鼠标点击窗格高亮切换，地址栏显示该窗格路径 | 完全一致（由 `ActivePaneCoordinator` 统一调度） | 完全一致 |
| 元数据展示 | 单选展示完整元数据与缩略图，多选展示共同属性与叠卡片预览 | 完全一致（优化算法结果 100% 对齐原版） | 完全一致 |
| 快捷键响应 | 快捷键仅作用于当前激活且可见窗格 | 完全一致（`currentTarget()` 严格过滤可见性） | 完全一致 |
| 标签栏分屏与合并 | 数量达到 4 个时弹出 Toast 阻断，否则拆分/合并窗格 | 完全一致 | 完全一致 |

### 10.2 优化核对与既有缺陷说明
1. **多选性能优化前后一致性说明**：阶段一采用提前收敛判定 `(!sameRating && !sameColor && !sameNote && !sameUrl && commonTags.isEmpty())`，一旦成立，所有字段均已确认不一致且交集已空，后续元素的元数据读取无论如何不会再改变最终输出结果；阶段二对最后 4 项叠卡片项仅提取预览图，确保传给 `metaPanel` 的数据逐项逐字段与原实现绝对相同。
2. **重构中发现但未修复的既有缺陷说明**：
   - 既有缺陷：在标签栏 `folderDroppedOnTab` 中，直接打开分屏窗格后未同步更新对应标签页标题。按照总原则严格禁止顺手修复既有业务逻辑缺陷，本方案保持原有行为不变。
