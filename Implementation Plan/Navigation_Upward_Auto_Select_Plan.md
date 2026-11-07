# Navigation Upward Auto-Select Implementation Plan

## 概述与根因分析
- 根因：`NavigationService` 只在 `goUp()` 时简单记录离开路径，而 `goBack()`、面包屑导航、双击空白返回等均未统一计算返回上一层时应选中的子项路径；`PanelMediator` 与 `ContentPanel` 恢复选区时未针对折叠的文件夹分区展开，未在异步加载/布局完成后滚动至刚好可见。
- 设计：
  1. 在 `NavigationService` 中统一通过 `computeAscendingTarget(from, to)` 计算返回时应选中的直接子项；
  2. `takePendingTargetChildPath(targetUrl)` 绑定目标路径，用完即清，防串扰；
  3. 在 `ContentPanel` 封装提供 `isFolderSectionCollapsed()` 与 `expandFolderSection()` 高内聚接口，遵守黑盒隔离与契约锁；
  4. `ContentViewCoordinator::restoreSelections` 时，若目标为文件夹且分区被折叠，通过 `ContentPanel` 自动展开文件夹分区；
  5. 选中使用现有样式，`scrollTo` 采用 `EnsureVisible`。

## 修改文件清单
1. `QuarkMeta_Source/src/core/NavigationService.h`
2. `QuarkMeta_Source/src/core/NavigationService.cpp`
3. `QuarkMeta_Source/src/ui/PanelMediator.cpp`
4. `QuarkMeta_Source/src/ui/ContentPanel.h`
5. `QuarkMeta_Source/src/ui/ContentPanel.cpp`
6. `QuarkMeta_Source/src/ui/controllers/ContentViewCoordinator.cpp`

## 精准替换块

### 1. `QuarkMeta_Source/src/core/NavigationService.h`
<<<<<<< SEARCH
    QString takePendingGoUpChildPath();

    // 状态查询接口
=======
    QString takePendingTargetChildPath(const QString& targetUrl);
    static QString computeAscendingTarget(const QString& fromPath, const QString& toPath);

    // 状态查询接口
>>>>>>> REPLACE

<<<<<<< SEARCH
    QString m_pendingGoUpChildPath;
    static constexpr int kMaxHistoryDepth = 100;
=======
    QString m_pendingTargetChildPath;
    QString m_pendingTargetExpectedUrl;
    static constexpr int kMaxHistoryDepth = 100;
>>>>>>> REPLACE

### 2. `QuarkMeta_Source/src/core/NavigationService.cpp`
<<<<<<< SEARCH
void NavigationService::navigateTo(const QString& rawUrl, bool recordHistory, bool recursive) {
    if (rawUrl.isEmpty()) return;

    QString normalized = normalizeUrl(rawUrl);

    // 路径跳转幂等性守卫 (No-op Protection)：
    // 若目标路径与当前路径一致（大小写不敏感对比）且非递归状态变更，直接返回，阻断无意义的视图重载与筛选面板重置
    if (QString::compare(normalized, m_currentUrl, Qt::CaseInsensitive) == 0 && !recursive) {
        return;
    }
=======
QString NavigationService::computeAscendingTarget(const QString& fromPath, const QString& toPath) {
    if (fromPath.isEmpty() || toPath.isEmpty()) return QString();
    if (fromPath.contains("://") || toPath.contains("://")) return QString();

    QString fromNorm = QDir::fromNativeSeparators(QDir::cleanPath(fromPath));
    QString toNorm = QDir::fromNativeSeparators(QDir::cleanPath(toPath));

    if (toNorm.endsWith(':')) toNorm += "/";
    if (fromNorm.endsWith(':')) fromNorm += "/";

    QString fromLower = fromNorm.toLower();
    QString toLower = toNorm.toLower();
    if (toLower.endsWith('/')) toLower.chop(1);
    if (fromLower.endsWith('/')) fromLower.chop(1);

    if (fromLower == toLower) return QString();

    QString prefix = toLower + "/";
    if (!fromLower.startsWith(prefix)) return QString();

    QString rel = fromNorm.mid(prefix.length());
    int slashIdx = rel.indexOf('/');
    QString directChildName = (slashIdx == -1) ? rel : rel.left(slashIdx);
    if (directChildName.isEmpty()) return QString();

    QDir targetDir(toNorm);
    return QDir::toNativeSeparators(targetDir.filePath(directChildName));
}

QString NavigationService::takePendingTargetChildPath(const QString& targetUrl) {
    QString normTarget = normalizeUrl(targetUrl);
    if (QString::compare(normTarget, m_pendingTargetExpectedUrl, Qt::CaseInsensitive) == 0) {
        QString res = m_pendingTargetChildPath;
        m_pendingTargetChildPath.clear();
        m_pendingTargetExpectedUrl.clear();
        return res;
    }
    m_pendingTargetChildPath.clear();
    m_pendingTargetExpectedUrl.clear();
    return QString();
}

void NavigationService::navigateTo(const QString& rawUrl, bool recordHistory, bool recursive) {
    if (rawUrl.isEmpty()) return;

    QString normalized = normalizeUrl(rawUrl);

    // 计算返回上一级时需选中的子项
    QString ascTarget = computeAscendingTarget(m_currentUrl, normalized);
    if (!ascTarget.isEmpty()) {
        m_pendingTargetChildPath = ascTarget;
        m_pendingTargetExpectedUrl = normalized;
    }

    // 路径跳转幂等性守卫 (No-op Protection)：
    // 若目标路径与当前路径一致（大小写不敏感对比）且非递归状态变更，直接返回，阻断无意义的视图重载与筛选面板重置
    if (QString::compare(normalized, m_currentUrl, Qt::CaseInsensitive) == 0 && !recursive) {
        return;
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
void NavigationService::goBack() {
    if (canGoBack()) {
        m_currentIndex--;
        m_currentUrl = m_history[m_currentIndex];
        emit currentUrlChanged(m_currentUrl, currentDisplayPath(), false);
        emitNavState();
    }
}

void NavigationService::goForward() {
    if (canGoForward()) {
        m_currentIndex++;
        m_currentUrl = m_history[m_currentIndex];
        emit currentUrlChanged(m_currentUrl, currentDisplayPath(), false);
        emitNavState();
    }
}

QString NavigationService::takePendingGoUpChildPath() {
    QString path = m_pendingGoUpChildPath;
    m_pendingGoUpChildPath.clear();
    return path;
}

void NavigationService::goUp() {
    if (!canGoUp()) return;

    QString departingPath = m_currentUrl;
    QDir dir(m_currentUrl);
    if (dir.isRoot()) {
        m_pendingGoUpChildPath.clear();
        navigateTo("computer://");
        return;
    }

    if (dir.cdUp()) {
        m_pendingGoUpChildPath = departingPath;
        navigateTo(dir.absolutePath());
    } else {
        m_pendingGoUpChildPath.clear();
        navigateTo("computer://");
    }
}
=======
void NavigationService::goBack() {
    if (canGoBack()) {
        QString prevUrl = m_history[m_currentIndex - 1];
        QString ascTarget = computeAscendingTarget(m_currentUrl, prevUrl);
        if (!ascTarget.isEmpty()) {
            m_pendingTargetChildPath = ascTarget;
            m_pendingTargetExpectedUrl = normalizeUrl(prevUrl);
        }
        m_currentIndex--;
        m_currentUrl = m_history[m_currentIndex];
        emit currentUrlChanged(m_currentUrl, currentDisplayPath(), false);
        emitNavState();
    }
}

void NavigationService::goForward() {
    if (canGoForward()) {
        m_currentIndex++;
        m_currentUrl = m_history[m_currentIndex];
        emit currentUrlChanged(m_currentUrl, currentDisplayPath(), false);
        emitNavState();
    }
}

void NavigationService::goUp() {
    if (!canGoUp()) return;

    QDir dir(m_currentUrl);
    if (dir.isRoot()) {
        navigateTo("computer://");
        return;
    }

    if (dir.cdUp()) {
        navigateTo(dir.absolutePath());
    } else {
        navigateTo("computer://");
    }
}
>>>>>>> REPLACE

### 3. `QuarkMeta_Source/src/ui/PanelMediator.cpp`
<<<<<<< SEARCH
        if (targetPanel) {
            QString goUpChild = NavigationService::instance().takePendingGoUpChildPath();
            if (!goUpChild.isEmpty() && url != "computer://" && url != "trash://") {
                targetPanel->setPendingSelectPath(goUpChild, false);
            }
=======
        if (targetPanel) {
            QString targetChild = NavigationService::instance().takePendingTargetChildPath(url);
            if (!targetChild.isEmpty() && url != "computer://" && url != "trash://") {
                targetPanel->setPendingSelectPath(targetChild, false);
            }
>>>>>>> REPLACE

### 4. `QuarkMeta_Source/src/ui/ContentPanel.h`
<<<<<<< SEARCH
    void setViewMode(ViewMode mode);
    void toggleFolderSectionCollapse();
    void selectAndScrollToPath(const QString& path);
=======
    void setViewMode(ViewMode mode);
    void toggleFolderSectionCollapse();
    bool isFolderSectionCollapsed() const;
    void expandFolderSection();
    void selectAndScrollToPath(const QString& path);
>>>>>>> REPLACE

### 5. `QuarkMeta_Source/src/ui/ContentPanel.cpp`
<<<<<<< SEARCH
void ContentPanel::toggleFolderSectionCollapse() {
    if (m_currentViewMode == ColumnView && m_columnView) {
        m_columnView->toggleFolderSectionCollapse();
    } else if (m_sectionModel) {
        m_sectionModel->setFolderCollapsed(!m_sectionModel->isFolderCollapsed());
    }
}
=======
void ContentPanel::toggleFolderSectionCollapse() {
    if (m_currentViewMode == ColumnView && m_columnView) {
        m_columnView->toggleFolderSectionCollapse();
    } else if (m_sectionModel) {
        m_sectionModel->setFolderCollapsed(!m_sectionModel->isFolderCollapsed());
    }
}

bool ContentPanel::isFolderSectionCollapsed() const {
    if (m_currentViewMode == ColumnView) {
        return false;
    }
    return m_sectionModel && m_sectionModel->isFolderCollapsed();
}

void ContentPanel::expandFolderSection() {
    if (m_sectionModel && m_sectionModel->isFolderCollapsed()) {
        m_sectionModel->setFolderCollapsed(false);
    }
}
>>>>>>> REPLACE

### 6. `QuarkMeta_Source/src/ui/controllers/ContentViewCoordinator.cpp`
<<<<<<< SEARCH
        int total = viewModeModel->rowCount();
        for (int r = 0; r < total; ++r) {
            QModelIndex idx = viewModeModel->index(r, 0);
            if (idx.data(SectionHeaderRole).toBool()) continue;
            QString p = idx.data(PathRole).toString();
            if (matchesAny(p)) {
                sel.select(idx, idx);
                lastIdx = idx;
            }
        }
=======
        // 自动展开已折叠的文件夹分区，确保目标项可见
        if (m_panel && m_panel->isFolderSectionCollapsed()) {
            for (const QString& sp : selectedPaths) {
                if (QFileInfo(sp).isDir()) {
                    m_panel->expandFolderSection();
                    break;
                }
            }
        }

        int total = viewModeModel->rowCount();
        for (int r = 0; r < total; ++r) {
            QModelIndex idx = viewModeModel->index(r, 0);
            if (idx.data(SectionHeaderRole).toBool()) continue;
            QString p = idx.data(PathRole).toString();
            if (matchesAny(p)) {
                sel.select(idx, idx);
                lastIdx = idx;
            }
        }
>>>>>>> REPLACE
