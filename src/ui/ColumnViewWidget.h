#pragma once

#include <QScrollArea>
#include <QHBoxLayout>
#include <QListView>
#include <QLabel>
#include <QPointer>
#include "models/DiskItemModel.h"
#include "models/FilterProxyModel.h"

namespace QuarkMeta {

class ColumnViewPane : public QWidget {
    Q_OBJECT
public:
    explicit ColumnViewPane(const QString& path, QWidget* parent = nullptr);
    ~ColumnViewPane() override = default;

    QString currentPath() const { return m_path; }
    void loadDirectory();

    void selectItemByPath(const QString& targetPath);
    void clearSelection();

signals:
    void folderSelected(const QString& folderPath, int paneIndex);
    void fileSelected(const QString& filePath, int paneIndex);

private:
    QString m_path;
    DiskItemModel* m_model = nullptr;
    FilterProxyModel* m_proxyModel = nullptr;
    QListView* m_listView = nullptr;
};

class ColumnViewWidget : public QScrollArea {
    Q_OBJECT
public:
    explicit ColumnViewWidget(QWidget* parent = nullptr);
    ~ColumnViewWidget() override = default;

    void setRootPath(const QString& path);
    void clearAllColumns();

signals:
    void pathNavigated(const QString& path);

private:
    void dismissSubColumns(int fromIndex);
    ColumnViewPane* appendColumn(const QString& path);
    void clearOtherSelections(int activePaneIdx);
    void updatePaneWidths();

    QWidget* m_container = nullptr;
    QHBoxLayout* m_layout = nullptr;
    QList<ColumnViewPane*> m_panes;
};

} // namespace QuarkMeta
