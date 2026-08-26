# Implementation Plan - MetaPanel I/O Isolation & PanelMediator O(1) Sync Optimization (`performance-1.md`)

## 1. Overview
This implementation plan fixes severe UI thread freezing and disk I/O storms during selection changes (Ctrl+A / Box Select):
1. **MetaPanel Disk I/O Storm Fix**: `MetaPanel::setRating` and `MetaPanel::setColor` were accidentally triggering real `.QuarkMeta.json` disk writes during passive selection updates. Adding a `fromUser = false` guard ensures disk write and signal emission only occur when the user explicitly clicks UI rating/color buttons.
2. **PanelMediator O(1) Sync Fix**: `PanelMediator::onContentSelectionChanged` previously executed an $O(N)$ linear row-by-row string search loop over 2,600+ proxy model rows in the main UI thread. Replaced with direct $O(1)$ index lookup via `contentPanel->getSelectedIndexes()`.

---

## 2. Modified Files List
- `src/ui/MetaPanel.h`
- `src/ui/MetaPanel.cpp`
- `src/ui/PanelMediator.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/MetaPanel.h`
```
<<<<<<< SEARCH
    void setRating(int rating);
    void setColor(const std::wstring& color);
=======
    void setRating(int rating, bool fromUser = false);
    void setColor(const std::wstring& color, bool fromUser = false);
>>>>>>> REPLACE
```

### 3.2 `src/ui/MetaPanel.cpp`
```
<<<<<<< SEARCH
void MetaPanel::setRating(int rating) {
    m_currentRating = rating;
    for (int i = 0; i < m_starBtns.size(); ++i) {
        bool active = (i < rating);
        m_starBtns[i]->setIcon(UiHelper::getIcon(
            active ? "star_filled" : "star",
            active ? QColor("#FF551C") : QColor("#555555"),
            18
        ));
        m_starBtns[i]->setIconSize(QSize(18, 18));
    }

    if (!m_isInternalUpdating && !m_selectedPaths.isEmpty()) {
        for (const QString& p : m_selectedPaths) {
            MetadataManager::instance().setRating(p.toStdWString(), rating, true);
        }
        emit metadataChanged(rating, m_currentColor);
    }
}
=======
void MetaPanel::setRating(int rating, bool fromUser) {
    m_currentRating = rating;
    for (int i = 0; i < m_starBtns.size(); ++i) {
        bool active = (i < rating);
        m_starBtns[i]->setIcon(UiHelper::getIcon(
            active ? "star_filled" : "star",
            active ? QColor("#FF551C") : QColor("#555555"),
            18
        ));
        m_starBtns[i]->setIconSize(QSize(18, 18));
    }

    if (fromUser && !m_selectedPaths.isEmpty()) {
        for (const QString& p : m_selectedPaths) {
            MetadataManager::instance().setRating(p.toStdWString(), rating, true);
        }
        emit metadataChanged(rating, m_currentColor);
    }
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void MetaPanel::setColor(const std::wstring& color) {
    m_currentColor = color;
    QString colorStr = QString::fromStdWString(color);

    for (QPushButton* btn : m_colorBtns) {
        QString hex = btn->property("hexColor").toString();
        QString tip = btn->property("tooltipText").toString();
        bool active = (!colorStr.isEmpty() && (colorStr == hex || colorStr == tip));

        btn->setStyleSheet(QString(
            "QPushButton { background-color: %1; border: %2; border-radius: 8px; }"
            "QPushButton:hover { border-color: #FFFFFF; }"
        ).arg(hex).arg(active ? "2px solid #FFFFFF" : "1px solid transparent"));
    }

    if (!m_isInternalUpdating && !m_selectedPaths.isEmpty()) {
        for (const QString& p : m_selectedPaths) {
            MetadataManager::instance().setColor(p.toStdWString(), color, true);
        }
        emit metadataChanged(m_currentRating, color);
    }
}
=======
void MetaPanel::setColor(const std::wstring& color, bool fromUser) {
    m_currentColor = color;
    QString colorStr = QString::fromStdWString(color);

    for (QPushButton* btn : m_colorBtns) {
        QString hex = btn->property("hexColor").toString();
        QString tip = btn->property("tooltipText").toString();
        bool active = (!colorStr.isEmpty() && (colorStr == hex || colorStr == tip));

        btn->setStyleSheet(QString(
            "QPushButton { background-color: %1; border: %2; border-radius: 8px; }"
            "QPushButton:hover { border-color: #FFFFFF; }"
        ).arg(hex).arg(active ? "2px solid #FFFFFF" : "1px solid transparent"));
    }

    if (fromUser && !m_selectedPaths.isEmpty()) {
        for (const QString& p : m_selectedPaths) {
            MetadataManager::instance().setColor(p.toStdWString(), color, true);
        }
        emit metadataChanged(m_currentRating, color);
    }
}
>>>>>>> REPLACE
```

### 3.3 `src/ui/PanelMediator.cpp`
```
<<<<<<< SEARCH
            if (paths.isEmpty()) {
                metaPanel->setImagePreview(QPixmap());
                metaPanel->updateInfo("-", "-", "-", "-", "-", "-", "-", false, 0, 0);
                metaPanel->setRating(0);
                metaPanel->setColor(L"");
                metaPanel->setTags(QStringList());
                metaPanel->setNote(L"");
                metaPanel->setURL(L"");
                metaPanel->setPalettes({});
            } else {
                QModelIndex idx;
                if (contentPanel->model()) {
                    for (int i = 0; i < contentPanel->getProxyModel()->rowCount(); ++i) {
                        QModelIndex proxyIdx = contentPanel->getProxyModel()->index(i, 0);
                        if (proxyIdx.data(PathRole).toString() == paths.first()) {
                            idx = proxyIdx;
                            break;
                        }
                    }
                }
=======
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
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
                metaPanel->setRating(idx.data(RatingRole).toInt());
                metaPanel->setColor(idx.data(ColorRole).toString().toStdWString());
=======
                metaPanel->setRating(idx.data(RatingRole).toInt(), false);
                metaPanel->setColor(idx.data(ColorRole).toString().toStdWString(), false);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Build application and open a directory containing 2,500+ items.
2. Perform Ctrl+A (Select All) or multi-item drag selection.
3. Verify selection completes in 0ms with zero disk writes to `.QuarkMeta.json`.
4. Click rating stars or color buttons in `MetaPanel` to verify user-initiated rating/color changes persist correctly to disk.
