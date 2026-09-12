#pragma once

#include <QWidget>
#include <QListView>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QList>
#include "models/DiskItemModel.h"
#include "models/FilterProxyModel.h"

namespace QuarkMeta {

class ContentPanel;

/**
 * @brief 单个列视图面板 (ColumnViewPane)
 */
class ColumnViewPane : public QWidget {
    Q_OBJECT
public:
    explicit ColumnViewPane(const QString& path, ContentPanel* contentPanel, QWidget* parent = nullptr);

    QString path() const { return m_path; }
    QListView* listView() const { return m_listView; }
    DiskItemModel* model() const { return m_model; }
    FilterProxyModel* proxyModel() const { return m_proxyModel; }

    void loadDirectory();
    void selectItemByPath(const QString& itemPath);

signals:
    void folderSelected(const QString& folderPath, ColumnViewPane* pane);
    void fileSelected(const QString& filePath, ColumnViewPane* pane);

private slots:
    void onClicked(const QModelIndex& index);
    void onDoubleClicked(const QModelIndex& index);

private:
    QString m_path;
    ContentPanel* m_contentPanel = nullptr;
    DiskItemModel* m_model = nullptr;
    FilterProxyModel* m_proxyModel = nullptr;
    QListView* m_listView = nullptr;
};

/**
 * @brief 分栏视图（列视图）主容器 (ColumnViewWidget)
 */
class ColumnViewWidget : public QWidget {
    Q_OBJECT
public:
    explicit ColumnViewWidget(ContentPanel* contentPanel, QWidget* parent = nullptr);

    void setRootPath(const QString& path);
    void appendColumn(const QString& path);
    void dismissSubColumns(ColumnViewPane* targetPane);

    ColumnViewPane* activePane() const;
    void refreshActiveColumn();
    void updateMetadataForPath(const QString& path);

    QList<ColumnViewPane*> panes() const { return m_panes; }

private slots:
    void onFolderSelected(const QString& folderPath, ColumnViewPane* pane);
    void onFileSelected(const QString& filePath, ColumnViewPane* pane);

private:
    ContentPanel* m_contentPanel = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_container = nullptr;
    QHBoxLayout* m_containerLayout = nullptr;
    QList<ColumnViewPane*> m_panes;
    QString m_rootPath;
};

} // namespace QuarkMeta
