#include "src/ui/models/SectionProxyModel.h"
#include "src/core/ModelContract.h"
#include <QStandardItemModel>
#include <QElapsedTimer>
#include <QDebug>
#include <cassert>

using namespace QuarkMeta;

void runSectionProxyModelTests() {
    qDebug() << "=== Starting SectionProxyModel Self-Tests ===";

    QStandardItemModel sourceModel;
    for (int i = 0; i < 10; ++i) {
        auto* folderItem = new QStandardItem(QString("Folder %1").arg(i));
        folderItem->setData("folder", TypeRole);
        sourceModel.appendRow(folderItem);
    }
    for (int i = 0; i < 20; ++i) {
        auto* fileItem = new QStandardItem(QString("File %1").arg(i));
        fileItem->setData("file", TypeRole);
        sourceModel.appendRow(fileItem);
    }

    SectionProxyModel proxyModel;
    proxyModel.setSourceModel(&sourceModel);

    // Test 1: Initial mapping counts
    assert(proxyModel.folderCount() == 10);
    assert(proxyModel.fileCount() == 20);
    // Row 0: FolderHeader, Rows 1..10: FolderItems, Row 11: FileHeader, Rows 12..31: FileItems
    assert(proxyModel.rowCount() == 32);
    qDebug() << "[PASS] Test 1: Initial mapping counts";

    // Test 2: MapFromSource & MapToSource consistency
    QModelIndex srcFolder0 = sourceModel.index(0, 0);
    QModelIndex proxyFolder0 = proxyModel.mapFromSource(srcFolder0);
    assert(proxyFolder0.isValid() && proxyFolder0.row() == 1);
    assert(proxyModel.mapToSource(proxyFolder0) == srcFolder0);

    QModelIndex srcFile0 = sourceModel.index(10, 0);
    QModelIndex proxyFile0 = proxyModel.mapFromSource(srcFile0);
    assert(proxyFile0.isValid() && proxyFile0.row() == 12);
    assert(proxyModel.mapToSource(proxyFile0) == srcFile0);
    qDebug() << "[PASS] Test 2: MapFromSource & MapToSource consistency";

    // Test 3: Collapse Folders
    proxyModel.setFolderCollapsed(true);
    assert(proxyModel.isFolderCollapsed());
    // Row 0: FolderHeader, Row 1: FileHeader, Rows 2..21: FileItems
    assert(proxyModel.rowCount() == 22);
    assert(!proxyModel.mapFromSource(srcFolder0).isValid()); // Folder items unmapped when collapsed
    QModelIndex proxyFile0Collapsed = proxyModel.mapFromSource(srcFile0);
    assert(proxyFile0Collapsed.isValid() && proxyFile0Collapsed.row() == 2);
    qDebug() << "[PASS] Test 3: Collapse Folders";

    // Test 4: Expand Folders
    proxyModel.setFolderCollapsed(false);
    assert(!proxyModel.isFolderCollapsed());
    assert(proxyModel.rowCount() == 32);
    assert(proxyModel.mapFromSource(srcFolder0).row() == 1);
    qDebug() << "[PASS] Test 4: Expand Folders";

    // Test 5: Benchmark 5000 rows dataChanged handling (< 10ms target)
    QStandardItemModel bigModel;
    for (int i = 0; i < 5000; ++i) {
        auto* item = new QStandardItem(QString("Item %1").arg(i));
        item->setData(i < 500 ? "folder" : "file", TypeRole);
        bigModel.appendRow(item);
    }
    SectionProxyModel bigProxy;
    bigProxy.setSourceModel(&bigModel);

    QElapsedTimer timer;
    timer.start();
    emit bigModel.dataChanged(bigModel.index(0, 0), bigModel.index(4999, 0));
    qint64 elapsedMs = timer.elapsed();
    qDebug() << "[PASS] Test 5: 5000 items dataChanged processed in" << elapsedMs << "ms";
    assert(elapsedMs <= 10);

    qDebug() << "=== All SectionProxyModel Self-Tests Passed Successfully ===";
}

int main(int argc, char** argv) {
    runSectionProxyModelTests();
    return 0;
}
