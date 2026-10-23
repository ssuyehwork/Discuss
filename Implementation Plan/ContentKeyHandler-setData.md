# ContentKeyHandler.cpp 快捷键 setData 错配修复方案

**文件：** `src/ui/controllers/ContentKeyHandler.cpp`
**根因：** 4处 `m_panel->getActiveProxyModel()` 返回的是与 `idx` 不匹配的孤立 proxy（`m_proxyModel`），应改为直接取 `idx` 所属的 `view->model()`。

---

**1. Ctrl+0~5 评级**
```
SEARCH:
    if ((keyEvent->modifiers() & Qt::ControlModifier) && (keyEvent->key() >= Qt::Key_0 && keyEvent->key() <= Qt::Key_5)) {
        int rating = keyEvent->key() - Qt::Key_0;
        auto indexes = view->selectionModel()->selectedIndexes();
        for (const auto& idx : indexes) {
            if (idx.column() == 0) m_panel->getActiveProxyModel()->setData(idx, rating, RatingRole);
        }
        return true;
    }

REPLACE:
    if ((keyEvent->modifiers() & Qt::ControlModifier) && (keyEvent->key() >= Qt::Key_0 && keyEvent->key() <= Qt::Key_5)) {
        int rating = keyEvent->key() - Qt::Key_0;
        auto* model = qobject_cast<QSortFilterProxyModel*>(view->model());
        if (!model) return true;
        auto indexes = view->selectionModel()->selectedIndexes();
        for (const auto& idx : indexes) {
            if (idx.column() == 0) model->setData(idx, rating, RatingRole);
        }
        return true;
    }
```

**2. Alt+D 置顶/取消置顶**
```
SEARCH:
    if (((keyEvent->modifiers() & Qt::AltModifier) || (keyEvent->modifiers() & (Qt::AltModifier | Qt::WindowShortcut))) && (keyEvent->key() == Qt::Key_D)) {
        auto indexes = view->selectionModel()->selectedIndexes();
        for (const QModelIndex& idx : indexes) {
            if (idx.column() == 0) {
                bool current = idx.data(IsLockedRole).toBool();
                m_panel->getActiveProxyModel()->setData(idx, !current, IsLockedRole);
            }
        }
        return true;
    }

REPLACE:
    if (((keyEvent->modifiers() & Qt::AltModifier) || (keyEvent->modifiers() & (Qt::AltModifier | Qt::WindowShortcut))) && (keyEvent->key() == Qt::Key_D)) {
        auto* model = qobject_cast<QSortFilterProxyModel*>(view->model());
        if (!model) return true;
        auto indexes = view->selectionModel()->selectedIndexes();
        for (const QModelIndex& idx : indexes) {
            if (idx.column() == 0) {
                bool current = idx.data(IsLockedRole).toBool();
                model->setData(idx, !current, IsLockedRole);
            }
        }
        return true;
    }
```

**3. Alt+1~9 色标**
```
SEARCH:
        QString colorValue = colors[keyEvent->key() - Qt::Key_1];

        auto indexes = view->selectionModel()->selectedIndexes();
        for (const auto& idx : indexes) {
            if (idx.column() == 0) {
                m_panel->getActiveProxyModel()->setData(idx, colorValue, ColorRole);
                QString path = idx.data(PathRole).toString();
                QIcon coloredIcon = ShellIconManager::getFileIcon(path, 128);
                m_panel->getActiveProxyModel()->setData(idx, coloredIcon, Qt::DecorationRole);
            }
        }
        return true;

REPLACE:
        QString colorValue = colors[keyEvent->key() - Qt::Key_1];

        auto* model = qobject_cast<QSortFilterProxyModel*>(view->model());
        if (!model) return true;
        auto indexes = view->selectionModel()->selectedIndexes();
        for (const auto& idx : indexes) {
            if (idx.column() == 0) {
                model->setData(idx, colorValue, ColorRole);
                QString path = idx.data(PathRole).toString();
                QIcon coloredIcon = ShellIconManager::getFileIcon(path, 128);
                model->setData(idx, coloredIcon, Qt::DecorationRole);
            }
        }
        return true;
```

**4. Ctrl+Shift+V 粘贴标签**
```
SEARCH:
            auto indexes = view->selectionModel()->selectedIndexes();
            int count = 0;
            for (const auto& targetIdx : indexes) {
                if (targetIdx.column() == 0) {
                    m_panel->getActiveProxyModel()->setData(targetIdx, copiedTags, TagsRole);
                    count++;
                }
            }

REPLACE:
            auto* model = qobject_cast<QSortFilterProxyModel*>(view->model());
            auto indexes = view->selectionModel()->selectedIndexes();
            int count = 0;
            for (const auto& targetIdx : indexes) {
                if (model && targetIdx.column() == 0) {
                    model->setData(targetIdx, copiedTags, TagsRole);
                    count++;
                }
            }
```

---

**说明：** F4"重复上一次操作"里也有3处 `m_panel->getActiveProxyModel()->setData(targetIdx, ...)`（评级/色标/粘贴标签），是同一模式、同一病灶，一并列入本次修复范围，改法与上面完全一致（`m_panel->getActiveProxyModel()` → `qobject_cast<QSortFilterProxyModel*>(view->model())`），篇幅关系不重复贴代码，一并执行。