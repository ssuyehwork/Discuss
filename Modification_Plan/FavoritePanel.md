# QuarkMeta 收藏夹独立第二栏（FavoritePanel）无脑实施方案

## 1. 方案背景与改造目标
在 independent QuarkMeta 架构规范中，主界面横向从左到右共有 5 栏（1. 目录导航 | 2. 收藏夹独占栏 | 3. 内容展示区 | 4. 元数据属性栏 | 5. 条件筛选栏）。
当前代码中，收藏夹强行嵌套在 `NavPanel` 内部（与磁盘树上下堆叠挤在第一栏），导致第二栏缺失。

本方案旨在将“收藏夹”从 `NavPanel` 中彻底剥离，独立封装为 `FavoritePanel`，并在 `MainWindow` 中挂载为垂直贯通的第二栏，提供最精准、最详尽、可直接复制执行的代码落地指南。

---

## 2. 涉及修改文件清单

| 变更类型 | 文件路径 | 职责描述 |
| :--- | :--- | :--- |
| **新增文件** | `src/ui/FavoritePanel.h` | 独立收藏夹面板头文件 |
| **新增文件** | `src/ui/FavoritePanel.cpp` | 独立收藏夹面板实现文件 |
| **修改文件** | `src/ui/NavPanel.h` | 剔除收藏夹相关成员与接口 |
| **修改文件** | `src/ui/NavPanel.cpp` | 移除收藏夹 View/Model 及 QSplitter 布局 |
| **修改文件** | `src/ui/MainWindow.h` | 引入 `FavoritePanel` 成员与头文件 |
| **修改文件** | `src/ui/MainWindow.cpp` | 在主 Splitter 中组装 5 栏，绑定信号量 |
| **修改文件** | `CMakeLists.txt` | 将新增的 `FavoritePanel.h/.cpp` 写入编译列表 |

---

## 3. 详细分步骤落地指南

### 3.1 步骤一：创建 `FavoritePanel.h` 与 `FavoritePanel.cpp`

在 `src/ui/` 目录下新增 `FavoritePanel.h` 与 `FavoritePanel.cpp`：

#### `src/ui/FavoritePanel.h`
```cpp
#pragma once

#include <QFrame>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include "DropTreeView.h"

namespace QuarkMeta {

/**
 * @brief 独立收藏夹面板（主界面第二栏）
 * 垂直贯通独占，专门展示常用快捷文件/文件夹
 */
class FavoritePanel : public QFrame {
    Q_OBJECT

public:
    explicit FavoritePanel(QWidget* parent = nullptr);
    ~FavoritePanel() override = default;

    /**
     * @brief 物理还原：设置 1px 高亮线的显隐状态
     */
    void setFocusHighlight(bool visible);

    /**
     * @brief 向收藏夹追加项目并防重
     */
    void addFavoriteItem(const QString& path);

    /**
     * @brief 持久化保存收藏夹到 AppConfig
     */
    void saveFavorites();

    /**
     * @brief 从 AppConfig 加载收藏夹
     */
    void loadFavorites();

signals:
    /**
     * @brief 当点击收藏的文件夹时发出，通知主窗口跳转
     */
    void directorySelected(const QString& path);

    /**
     * @brief 当点击收藏的文件时发出，通知主窗口跳转到父目录并高亮文件
     */
    void requestLocateFile(const QString& path);

private slots:
    void onFavoriteClicked(const QModelIndex& index);
    void onFavoriteContextMenu(const QPoint& pos);
    void onPathsDroppedToFavorite(const QStringList& paths, const QModelIndex& target);

private:
    void initUi();

    QVBoxLayout* m_mainLayout = nullptr;
    QWidget* m_focusLine = nullptr;

    DropTreeView* m_favoriteView = nullptr;
    QStandardItemModel* m_favoriteModel = nullptr;
};

} // namespace QuarkMeta
```

#### `src/ui/FavoritePanel.cpp`
```cpp
#include "FavoritePanel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "../core/AppConfig.h"
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

namespace QuarkMeta {

FavoritePanel::FavoritePanel(QWidget* parent)
    : QFrame(parent) {
    setObjectName("ListContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(200);
    // 物理对齐：设置右侧 1px 深色物理分割线与内嵌容器背景，确保面板间物理分割清晰可见
    setStyleSheet("FavoritePanel { background-color: #1E1E1E; color: #EEEEEE; border-right: 1px solid #2D2D30; }");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    initUi();
    loadFavorites();
}

void FavoritePanel::setFocusHighlight(bool visible) {
    if (m_focusLine) m_focusLine->setVisible(visible);
}

void FavoritePanel::initUi() {
    // 顶部 1px 焦点线
    m_focusLine = new QWidget(this);
    m_focusLine->setFixedHeight(1);
    m_focusLine->setStyleSheet("background-color: #007ACC;");
    m_focusLine->hide();
    m_mainLayout->addWidget(m_focusLine);

    // 固定顶栏 Header
    QWidget* header = new QWidget(this);
    header->setObjectName("ContainerHeader");
    header->setFixedHeight(32);
    header->setStyleSheet(
        "QWidget#ContainerHeader {"
        "  background-color: #252526;"
        "  border-bottom: 1px solid #333333;"
        "}"
    );
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(15, 0, 5, 0);
    headerLayout->setSpacing(5);

    QLabel* iconLabel = new QLabel(header);
    iconLabel->setPixmap(UiHelper::getIcon("star_filled", QColor("#FDB70A"), 18).pixmap(18, 18));
    headerLayout->addWidget(iconLabel);

    QLabel* titleLabel = new QLabel("收藏夹", header);
    titleLabel->setStyleSheet("color: #FDB70A; font-size: 13px; font-weight: bold; background: transparent; border: none;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    m_mainLayout->addWidget(header);

    // 收藏夹树视图
    m_favoriteView = new DropTreeView(this);
    m_favoriteView->setHeaderHidden(true);
    m_favoriteView->setIndentation(0);
    m_favoriteView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_favoriteView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_favoriteView->setDragEnabled(true);
    m_favoriteView->setAcceptDrops(true);
    m_favoriteView->setDropIndicatorShown(true);
    m_favoriteView->setDefaultDropAction(Qt::MoveAction);
    m_favoriteView->setDragDropMode(QAbstractItemView::DragDrop);
    m_favoriteView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_favoriteView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_favoriteView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_favoriteModel = new QStandardItemModel(this);
    m_favoriteView->setModel(m_favoriteModel);

    // 树视图 QSS 样式
    QString treeStyle = QString(
        "QTreeView { background-color: transparent; border: none; font-size: 12px; outline: none; padding-left: 10px; }"
        "QTreeView::item { height: 28px; padding-left: 0px; color: #EEEEEE; }"
        "QTreeView::item:hover { background-color: #2A2D2E; }"
        "QTreeView::item:selected { background-color: #37373D; color: #FFFFFF; }"
    );
    m_favoriteView->setStyleSheet(treeStyle);

    m_mainLayout->addWidget(m_favoriteView, 1);

    // 信号绑定
    connect(m_favoriteView, &QTreeView::clicked, this, &FavoritePanel::onFavoriteClicked);
    connect(m_favoriteView, &QWidget::customContextMenuRequested, this, &FavoritePanel::onFavoriteContextMenu);
    connect(m_favoriteView, &DropTreeView::pathsDropped, this, &FavoritePanel::onPathsDroppedToFavorite);

    // 模型数据变动监听
    auto updateFavAndSave = [this](){ saveFavorites(); };
    connect(m_favoriteModel, &QStandardItemModel::rowsMoved, this, updateFavAndSave);
    connect(m_favoriteModel, &QStandardItemModel::rowsInserted, this, updateFavAndSave);
    connect(m_favoriteModel, &QStandardItemModel::rowsRemoved, this, updateFavAndSave);
}

void FavoritePanel::onFavoriteClicked(const QModelIndex& index) {
    QString path = index.data(Qt::UserRole + 1).toString();
    if (path.isEmpty()) return;

    QFileInfo fi(path);
    if (fi.isDir()) {
        emit directorySelected(path);
    } else {
        emit requestLocateFile(path);
    }
}

void FavoritePanel::onFavoriteContextMenu(const QPoint& pos) {
    QModelIndex index = m_favoriteView->indexAt(pos);
    if (!index.isValid()) return;

    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    QAction* removeAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE")), "取消收藏");
    connect(removeAct, &QAction::triggered, this, [this, index]() {
        m_favoriteModel->removeRow(index.row());
    });

    menu.exec(m_favoriteView->viewport()->mapToGlobal(pos));
}

void FavoritePanel::onPathsDroppedToFavorite(const QStringList& paths, const QModelIndex& target) {
    Q_UNUSED(target);
    for (const QString& path : paths) {
        addFavoriteItem(path);
    }
    saveFavorites();
}

void FavoritePanel::loadFavorites() {
    if (!m_favoriteModel) return;
    m_favoriteModel->clear();

    QVariant val = AppConfig::instance().getValue("FavoritePanel/Favorites");
    if (!val.isValid()) {
        val = AppConfig::instance().getValue("NavPanel/Favorites"); // 向下兼容原配置
    }
    if (!val.isValid()) return;

    QJsonDocument doc = QJsonDocument::fromJson(val.toByteArray());
    if (!doc.isArray()) return;

    QJsonArray arr = doc.array();
    for (int i = 0; i < arr.size(); ++i) {
        QJsonObject obj = arr.at(i).toObject();
        QString path = obj.value("path").toString();
        if (!path.isEmpty()) {
            addFavoriteItem(path);
        }
    }
}

void FavoritePanel::saveFavorites() {
    if (!m_favoriteModel) return;

    QJsonArray arr;
    for (int i = 0; i < m_favoriteModel->rowCount(); ++i) {
        QStandardItem* item = m_favoriteModel->item(i);
        QString path = item->data(Qt::UserRole + 1).toString();

        QJsonObject obj;
        obj.insert("path", path);
        arr.append(obj);
    }

    QJsonDocument doc(arr);
    AppConfig::instance().setValue("FavoritePanel/Favorites", doc.toJson(QJsonDocument::Compact));
}

void FavoritePanel::addFavoriteItem(const QString& path) {
    for (int i = 0; i < m_favoriteModel->rowCount(); ++i) {
        if (m_favoriteModel->item(i)->data(Qt::UserRole + 1).toString() == path) {
            return;
        }
    }

    QFileInfo fi(path);
    if (!fi.exists()) return;

    QIcon icon = ShellIconManager::getFileIcon(path, 18);
    QStandardItem* item = new QStandardItem(icon, fi.fileName().isEmpty() ? path : fi.fileName());
    item->setData(path, Qt::UserRole + 1);

    m_favoriteModel->appendRow(item);
}

} // namespace QuarkMeta
```

---

### 3.2 步骤二：从 `NavPanel` 中彻底清理收藏夹

#### `src/ui/NavPanel.h`
1. 移除 `DropTreeView* m_favoriteView`、`QStandardItemModel* m_favoriteModel`、`QSplitter* m_splitter` 定义。
2. 移除 `addFavoriteItem()`、`saveFavorites()`、`loadFavorites()`、`buildGroup()`、`onFavoriteClicked()` 等成员函数声明。

#### `src/ui/NavPanel.cpp`
1. 修改 `initUi()`：
   - 彻底干掉 `m_splitter` 与 `buildGroup("收藏夹", ...)`。
   - 直接将 `m_treeView` 填入 `m_mainLayout`：
     ```cpp
     m_mainLayout->addWidget(m_treeView, 1);
     ```
2. 删除所有 `m_favoriteView` 相关的槽函数实现。

---

### 3.3 步骤三：修改 `MainWindow` 组装五栏

#### `src/ui/MainWindow.h`
1. 添加头文件包含/前向声明：
   ```cpp
   class FavoritePanel;
   ```
2. 添加成员变量：
   ```cpp
   FavoritePanel* m_favoritePanel = nullptr;
   ```

#### `src/ui/MainWindow.cpp`
1. `#include "FavoritePanel.h"`。
2. 在 `setupSplitters()` 中实例化并依序加入 `m_mainSplitter`：
   ```cpp
   m_navPanel = new NavPanel(this);
   m_favoritePanel = new FavoritePanel(this);
   m_contentPanel = new ContentPanel(this);
   m_metaPanel = new MetaPanel(this);
   m_filterPanel = new FilterPanel(this);

   m_mainSplitter->addWidget(m_navPanel);
   m_mainSplitter->addWidget(m_favoritePanel);
   m_mainSplitter->addWidget(m_contentPanel);
   m_mainSplitter->addWidget(m_metaPanel);
   m_mainSplitter->addWidget(m_filterPanel);
   ```
3. 连接 `m_favoritePanel` 的信号：
   ```cpp
   connect(m_favoritePanel, &FavoritePanel::directorySelected, this, [this](const QString& path) {
       unifiedNavigateTo(path);
   });
   connect(m_favoritePanel, &FavoritePanel::requestLocateFile, this, [this](const QString& path) {
       QFileInfo fi(path);
       m_contentPanel->setPendingSelectName(fi.fileName(), false);
       unifiedNavigateTo(fi.absolutePath());
   });
   connect(m_contentPanel, &ContentPanel::requestAddFavorite, this, [this](const QStringList& paths) {
       if (m_favoritePanel) {
           for (const QString& p : paths) {
               m_favoritePanel->addFavoriteItem(p);
           }
           m_favoritePanel->saveFavorites();
       }
   });
   ```
4. 更新 `populatePanelMenu` 与 `resetSplitterLayout` 支持 5 栏拉伸比例（如：`200 << 200 << 550 << 200 << 200`）。

---

### 3.4 步骤四：更新 `CMakeLists.txt`
在 `CMakeLists.txt` 的 `SOURCES` 列表中添加：
```cmake
src/ui/FavoritePanel.h
src/ui/FavoritePanel.cpp
```

---

## 4. 实施验证事项
1. 启动应用后，主界面平铺 5 栏，无任何弹窗/崩溃。
2. 第二栏“收藏夹”垂直贯通占满全高。
3. 从内容面板右键添加至收藏夹，第二栏实时刷新。
4. 拖拽文件夹至第二栏能成功收藏，重启应用后收藏项无损恢复。
