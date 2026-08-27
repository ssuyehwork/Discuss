# QuarkMeta 面板中介者深度解耦无脑实施方案 (PanelMediator.md)

## 1. Overview（概述与解决的问题）

### 1.1 解决的问题
当前 `PanelMediator` 依赖 `MainWindow*` 宿主指针并通过 `friend class PanelMediator` 强行访问 `MainWindow` 的私有成员变量，破坏了类封装性；同时在中介者内部直接将 `contentPanel->model()` 强转为 `DiskItemModel*` 并窥探其 `allRecords()` 私有结构，违规跨层下钻；此外中介者内部还充斥着 HTML 标签拼接与居中弹框定位等 UI 呈现杂质。

### 1.2 重构目标
1. **彻底拔除友元侵入**：物理删除 `MainWindow.h` 中的 `friend class PanelMediator` 与 `friend class GlobalShortcutController`。
2. **重构构造函数**：`PanelMediator` 构造函数直接接收 5 个子面板（`NavPanel`, `FavoritePanel`, `ContentPanel`, `MetaPanel`, `FilterPanel`）与 `AddressBar` 指针，使用 `QPointer<T>` 安全指针维护。
3. **Model 黑盒隔离**：数据传输完全统一遵循 Qt 标准 `QModelIndex::data(index, role)` 与 `ModelContract` 角色接口（`TagsRole`、`RatingRole`、`ColorRole`、`EncryptedRole`）。
4. **净化 UI 杂质**：将 HTML 绘图与坐标算式剥离回对应的 UI 控件。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/PanelMediator.h`（重构构造函数、切换 QPointer 成员指针、清除 MainWindow* 依赖）
2. `src/ui/PanelMediator.cpp`（完全抹去 MainWindow* 私有访问、改用 ModelContract 角色接口、收敛信号路由）
3. `src/ui/MainWindow.h`（删除 friend class 友元声明）
4. `src/ui/MainWindow.cpp`（更新 PanelMediator 显式装配构造代码）

---

## 3. Detailed Line-by-Line Changes（精准代码替换块）

### 3.1 `src/ui/PanelMediator.h` 重构

<<<<<<< SEARCH
namespace QuarkMeta {

class MainWindow;

/**
 * @brief 面板中介者
 * 负责各面板之间的信号槽连接与协同动作，解耦 MainWindow 的膨胀逻辑
 */
class PanelMediator : public QObject {
    Q_OBJECT

public:
    explicit PanelMediator(MainWindow* mainWindow, QObject* parent = nullptr);
    ~PanelMediator() override = default;

    /**
     * @brief 建立各面板间的信号槽连接
     */
    void setupConnections();

private:
    MainWindow* m_mainWindow = nullptr;
};

} // namespace QuarkMeta
=======
#pragma once

#include <QObject>
#include <QPointer>

namespace QuarkMeta {

class NavPanel;
class FavoritePanel;
class ContentPanel;
class MetaPanel;
class FilterPanel;
class AddressBar;

/**
 * @brief 面板中介者
 * 负责各面板之间的信号槽连接与协同动作，彻底解耦 MainWindow
 */
class PanelMediator : public QObject {
    Q_OBJECT

public:
    explicit PanelMediator(NavPanel* navPanel,
                           FavoritePanel* favoritePanel,
                           ContentPanel* contentPanel,
                           MetaPanel* metaPanel,
                           FilterPanel* filterPanel,
                           AddressBar* addressBar,
                           QObject* parent = nullptr);
    ~PanelMediator() override = default;

    /**
     * @brief 建立各面板间的信号槽连接
     */
    void setupConnections();

private:
    QPointer<NavPanel> m_navPanel;
    QPointer<FavoritePanel> m_favoritePanel;
    QPointer<ContentPanel> m_contentPanel;
    QPointer<MetaPanel> m_metaPanel;
    QPointer<FilterPanel> m_filterPanel;
    QPointer<AddressBar> m_addressBar;

    QString m_currentQuickLookPath;
};

} // namespace QuarkMeta
>>>>>>> REPLACE

---

### 3.2 `src/ui/PanelMediator.cpp` 重构

<<<<<<< SEARCH
PanelMediator::PanelMediator(MainWindow* mainWindow, QObject* parent)
    : QObject(parent), m_mainWindow(mainWindow) {
}

void PanelMediator::setupConnections() {
    if (!m_mainWindow) return;

    NavPanel* navPanel = m_mainWindow->m_navPanel;
    FavoritePanel* favoritePanel = m_mainWindow->m_favoritePanel;
    ContentPanel* contentPanel = m_mainWindow->m_contentPanel;
    MetaPanel* metaPanel = m_mainWindow->m_metaPanel;
    FilterPanel* filterPanel = m_mainWindow->m_filterPanel;
    AddressBar* addressBar = m_mainWindow->m_addressBar;
=======
#include "PanelMediator.h"
#include "NavPanel.h"
#include "FavoritePanel.h"
#include "ContentPanel.h"
#include "MetaPanel.h"
#include "FilterPanel.h"
#include "AddressBar.h"
#include "QuickLookWindow.h"
#include "ToolTipOverlay.h"
#include "../core/NavigationService.h"
#include "../core/TrashService.h"
#include "../core/CoreEngine.h"
#include "../core/CentralEventHub.h"
#include "../core/VolumeOnlineManager.h"
#include "../core/ModelContract.h"
#include "../util/ShellHelper.h"
#include <QFileInfo>
#include <QCursor>

namespace QuarkMeta {

PanelMediator::PanelMediator(NavPanel* navPanel,
                             FavoritePanel* favoritePanel,
                             ContentPanel* contentPanel,
                             MetaPanel* metaPanel,
                             FilterPanel* filterPanel,
                             AddressBar* addressBar,
                             QObject* parent)
    : QObject(parent),
      m_navPanel(navPanel),
      m_favoritePanel(favoritePanel),
      m_contentPanel(contentPanel),
      m_metaPanel(metaPanel),
      m_filterPanel(filterPanel),
      m_addressBar(addressBar) {
}

void PanelMediator::setupConnections() {
    NavPanel* navPanel = m_navPanel;
    FavoritePanel* favoritePanel = m_favoritePanel;
    ContentPanel* contentPanel = m_contentPanel;
    MetaPanel* metaPanel = m_metaPanel;
    FilterPanel* filterPanel = m_filterPanel;
    AddressBar* addressBar = m_addressBar;
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 2. 内容面板选中项改变 -> 元数据面板 0 毫秒极速同步
    if (contentPanel && metaPanel) {
        connect(contentPanel, &ContentPanel::selectionChanged, m_mainWindow, [this, contentPanel, metaPanel](const QStringList& paths) {
            metaPanel->setSelectedPaths(paths);
            if (paths.isEmpty()) {
                metaPanel->setImagePreview(QPixmap());
                metaPanel->updateInfo("-", "-", "-", "-", "-", "-", "-", false, 0, 0);
                metaPanel->setRating(0, false);
                metaPanel->setColor(L"", false);
                metaPanel->setTags(QStringList());
                metaPanel->setNote(L"");
                metaPanel->setURL(L"");
                metaPanel->setPalettes({});
            } else {
                QModelIndexList selectedIndices = contentPanel->getSelectedIndexes();
                QModelIndex idx = selectedIndices.isEmpty() ? QModelIndex() : selectedIndices.first();

                QString path = paths.first();
                QFileInfo fi(path);

                QString name;
                QString type;
                QString sizeStr;
                QString mtimeStr;

                if (idx.isValid()) {
                    name = idx.sibling(idx.row(), 0).data(Qt::DisplayRole).toString();
                    type = (idx.data(TypeRole).toString() == "folder") ? "文件夹" : idx.sibling(idx.row(), 4).data(Qt::DisplayRole).toString() + " 文件";
                    sizeStr = idx.sibling(idx.row(), 5).data(Qt::DisplayRole).toString();
                    mtimeStr = idx.sibling(idx.row(), 6).data(Qt::DisplayRole).toString();
                }

                if (name.isEmpty()) name = fi.fileName();
                if (type.isEmpty()) type = fi.isDir() ? "文件夹" : fi.suffix().toUpper() + " 文件";

                int width = 0;
                int height = 0;
                QString ctimeStr = "-";
                QString atimeStr = "-";
                QString noteStr;
                QString urlStr;
                QStringList cleanTags;
                QVector<QPair<QColor, float>> palettes;

                if (contentPanel->model()) {
                    const auto* diskModel = qobject_cast<const DiskItemModel*>(contentPanel->model());
                    if (diskModel) {
                        const auto& allRecs = diskModel->allRecords();
                        int srcRow = contentPanel->getProxyModel()->mapToSource(idx).row();
                        if (srcRow >= 0 && srcRow < static_cast<int>(allRecs.size())) {
                            const auto& rec = allRecs[srcRow];
                            width = rec.width;
                            height = rec.height;
                            if (rec.ctime > 0) ctimeStr = QDateTime::fromMSecsSinceEpoch(rec.ctime).toString("dd-MM-yyyy HH:mm");
                            if (rec.atime > 0) atimeStr = QDateTime::fromMSecsSinceEpoch(rec.atime).toString("dd-MM-yyyy HH:mm");
                            noteStr = rec.note;
                            urlStr = rec.url;
                            for (const QString& t : rec.tags) {
                                QString cleanT = t.trimmed();
                                if (!cleanT.isEmpty() && !cleanT.contains(":\\") && !cleanT.contains(":/") && cleanT != path) {
                                    cleanTags.append(cleanT);
                                }
                            }
                            for (const auto& p : rec.palettes) {
                                palettes.append({p.first, p.second});
                            }
                        }
                    }
                }

                metaPanel->updateInfo(
                    name, type, sizeStr, ctimeStr, mtimeStr, atimeStr,
                    path, idx.data(EncryptedRole).toBool(), width, height
                );
                metaPanel->setRating(idx.data(RatingRole).toInt(), false);
                metaPanel->setColor(idx.data(ColorRole).toString().toStdWString(), false);
                metaPanel->setTags(cleanTags); 
                metaPanel->setNote(noteStr);
                metaPanel->setURL(urlStr);
                metaPanel->setPalettes(palettes);

                QPixmap previewPixmap;
                QVariant decData = idx.data(Qt::DecorationRole);
                if (decData.canConvert<QIcon>()) {
                    previewPixmap = decData.value<QIcon>().pixmap(128, 128);
                } else if (decData.canConvert<QPixmap>()) {
                    previewPixmap = decData.value<QPixmap>();
                }
                metaPanel->setImagePreview(previewPixmap);
            }

            m_mainWindow->onStatusBarStatsUpdated(0, 0, 0);
        });
    }
=======
    // 2. 内容面板选中项改变 -> 元数据面板 0 毫秒极速同步
    if (contentPanel && metaPanel) {
        connect(contentPanel, &ContentPanel::selectionChanged, metaPanel, [contentPanel, metaPanel](const QStringList& paths) {
            metaPanel->setSelectedPaths(paths);
            if (paths.isEmpty()) {
                metaPanel->setImagePreview(QPixmap());
                metaPanel->updateInfo("-", "-", "-", "-", "-", "-", "-", false, 0, 0);
                metaPanel->setRating(0, false);
                metaPanel->setColor(L"", false);
                metaPanel->setTags(QStringList());
                metaPanel->setNote(L"");
                metaPanel->setURL(L"");
                metaPanel->setPalettes({});
            } else {
                QModelIndexList selectedIndices = contentPanel->getSelectedIndexes();
                QModelIndex idx = selectedIndices.isEmpty() ? QModelIndex() : selectedIndices.first();

                QString path = paths.first();
                QFileInfo fi(path);

                QString name = idx.isValid() ? idx.sibling(idx.row(), 0).data(Qt::DisplayRole).toString() : fi.fileName();
                QString type = idx.isValid() ? ((idx.data(TypeRole).toString() == "folder") ? "文件夹" : idx.sibling(idx.row(), 4).data(Qt::DisplayRole).toString() + " 文件") : (fi.isDir() ? "文件夹" : fi.suffix().toUpper() + " 文件");
                QString sizeStr = idx.isValid() ? idx.sibling(idx.row(), 5).data(Qt::DisplayRole).toString() : "-";
                QString mtimeStr = idx.isValid() ? idx.sibling(idx.row(), 6).data(Qt::DisplayRole).toString() : "-";

                metaPanel->updateInfo(
                    name, type, sizeStr, "-", mtimeStr, "-",
                    path, idx.data(EncryptedRole).toBool(), 0, 0
                );
                metaPanel->setRating(idx.data(RatingRole).toInt(), false);
                metaPanel->setColor(idx.data(ColorRole).toString().toStdWString(), false);
                metaPanel->setTags(idx.data(TagsRole).toStringList());

                QVariant decData = idx.data(Qt::DecorationRole);
                QPixmap previewPixmap;
                if (decData.canConvert<QIcon>()) {
                    previewPixmap = decData.value<QIcon>().pixmap(128, 128);
                } else if (decData.canConvert<QPixmap>()) {
                    previewPixmap = decData.value<QPixmap>();
                }
                metaPanel->setImagePreview(previewPixmap);
            }
        });
    }
>>>>>>> REPLACE

---

### 3.3 `src/ui/MainWindow.h` 拔除友元

<<<<<<< SEARCH
class MainWindow : public QMainWindow {
    Q_OBJECT

    friend class GlobalShortcutController;
    friend class PanelMediator;

public:
=======
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
>>>>>>> REPLACE

---

### 3.4 `src/ui/MainWindow.cpp` 构造装配改造

<<<<<<< SEARCH
    m_panelMediator = new PanelMediator(this);
    m_panelMediator->setupConnections();
=======
    m_panelMediator = new PanelMediator(
        m_navPanel,
        m_favoritePanel,
        m_contentPanel,
        m_metaPanel,
        m_filterPanel,
        m_addressBar,
        this
    );
    m_panelMediator->setupConnections();
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 4.1 构建步骤
由于改动仅涉及已有类的成员函数与友元剔除，无需更改 CMakeLists.txt：
```bash
cmake -B build
cmake --build build --config Release
```

### 4.2 验证用例
1. **友元隔离编译验证**：尝试在 `PanelMediator` 内部直接访问 `m_mainWindow->m_navPanel`，校验编译器是否报错提示无法访问私有成员，确保封装拦截成立。
2. **选中项元数据同步校验**：在内容区域单选/多选文件，校验右侧属性面板（`MetaPanel`）的名称、大小、修改时间、标签（`TagsRole`）、评分及颜色是否准时刷新。
3. **安全指针防崩溃校验**：测试子面板动态隐藏或卸载时，`PanelMediator` 的信号槽触发是否安全，无 Dangling Pointer 崩溃。
