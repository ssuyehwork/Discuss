# ContentHeaderWidget Split View Toolbar Button Implementation Plan

## 1. Overview
Add a split-pane button (`columns.svg`) to the `ContentHeaderWidget` toolbar on both primary and secondary content panels. Clicking this button in single-pane mode creates a horizontal dual-pane split view, opening the secondary pane with the last visited directory from `NavigationHistoryService`. Clicking it when split mode is active closes the secondary pane.

## 2. Modified Files List
- `src/ui/ContentHeaderWidget.h`
- `src/ui/ContentHeaderWidget.cpp`
- `src/ui/ContentPanel.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ContentHeaderWidget.h`
Add `splitViewRequested()` signal and `m_btnSplitView` member button.

```
<<<<<<< SEARCH
signals:
    void filterStateChanged(const FilterState& state);
    void recursiveToggled(bool recursive);

private:
    void initUi();

    QHBoxLayout* m_layout = nullptr;
    QLabel* m_iconLabel = nullptr;
    QLabel* m_titleLabel = nullptr;

    QPushButton* m_btnLayers = nullptr;
    QPushButton* m_btnToggleHidden = nullptr;
=======
signals:
    void filterStateChanged(const FilterState& state);
    void recursiveToggled(bool recursive);
    void splitViewRequested();

private:
    void initUi();

    QHBoxLayout* m_layout = nullptr;
    QLabel* m_iconLabel = nullptr;
    QLabel* m_titleLabel = nullptr;

    QPushButton* m_btnSplitView = nullptr;
    QPushButton* m_btnLayers = nullptr;
    QPushButton* m_btnToggleHidden = nullptr;
>>>>>>> REPLACE
```

### 3.2 `src/ui/ContentHeaderWidget.cpp`
Create and position `m_btnSplitView` with icon `columns` before the filter buttons.

```
<<<<<<< SEARCH
    m_layout->addWidget(m_iconLabel);
    m_layout->addWidget(m_titleLabel);
    m_layout->addStretch();

    auto setupToggleBtn = [this](QPushButton*& btn, const QString& iconKey, const QColor& activeColor, bool defaultChecked, const QString& tooltip) {
=======
    m_layout->addWidget(m_iconLabel);
    m_layout->addWidget(m_titleLabel);
    m_layout->addStretch();

    m_btnSplitView = new QPushButton(this);
    m_btnSplitView->setFixedSize(24, 24);
    m_btnSplitView->setIcon(UiHelper::getIcon("columns", QColor("#888888"), 18));
    m_btnSplitView->setProperty("tooltipText", "双窗格分栏视图");
    m_btnSplitView->setObjectName("ViewModeToolBtn");
    m_btnSplitView->installEventFilter(this);

    connect(m_btnSplitView, &QPushButton::clicked, this, [this]() {
        emit splitViewRequested();
    });

    m_layout->addWidget(m_btnSplitView, 0, Qt::AlignVCenter);

    auto setupToggleBtn = [this](QPushButton*& btn, const QString& iconKey, const QColor& activeColor, bool defaultChecked, const QString& tooltip) {
>>>>>>> REPLACE
```

### 3.3 `src/ui/ContentPanel.cpp`
Connect `splitViewRequested` signal from `m_headerWidget` to split/close pane logic using history.

```
<<<<<<< SEARCH
#include "ContentHeaderWidget.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "../core/AppConfig.h"
=======
#include "ContentHeaderWidget.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "../core/AppConfig.h"
#include "../core/NavigationHistoryService.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_headerWidget = new ContentHeaderWidget(this);
    m_headerWidget->setFilterState(m_currentFilter);

    connect(m_headerWidget, &ContentHeaderWidget::filterStateChanged, this, [this](const FilterState& state) {
=======
    m_headerWidget = new ContentHeaderWidget(this);
    m_headerWidget->setFilterState(m_currentFilter);

    connect(m_headerWidget, &ContentHeaderWidget::splitViewRequested, this, [this]() {
        if (m_isSecondaryPane) {
            emit closePaneRequested();
            return;
        }
        if (isSplitMode()) {
            closeSecondaryPane();
        } else {
            QStringList history = NavigationHistoryService::instance().getHistory();
            QString lastPath;
            for (const QString& hPath : history) {
                if (!hPath.isEmpty() && QDir::cleanPath(hPath) != QDir::cleanPath(m_currentPath)) {
                    lastPath = hPath;
                    break;
                }
            }
            if (lastPath.isEmpty()) {
                lastPath = m_currentPath;
            }
            splitPane(Qt::Horizontal, lastPath);
        }
    });

    connect(m_headerWidget, &ContentHeaderWidget::filterStateChanged, this, [this](const FilterState& state) {
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build target `QuarkMeta` via CMake/Ninja.
2. Launch `QuarkMeta`, verify `columns` icon button appears on `ContentHeaderWidget`.
3. Click button: verify dual-pane split opens with last visited folder.
4. Click button again: verify secondary pane closes cleanly.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `NavigationHistoryService::instance().getHistory()` for last visited folder retrieval.
- Reused `ContentPanel::splitPane` and `ContentPanel::closeSecondaryPane` for dual-pane state manipulation.

## 6. Header API Signature Verification
- `NavigationHistoryService::instance().getHistory()` -> `QStringList getHistory() const` in `NavigationHistoryService.h`
- `ContentPanel::isSplitMode()` -> `bool isSplitMode() const` in `ContentPanel.h`
- `ContentPanel::splitPane(Qt::Orientation, const QString&)` -> `void splitPane(Qt::Orientation orientation, const QString& secondaryPath = QString())` in `ContentPanel.h`
- `ContentPanel::closeSecondaryPane()` -> `void closeSecondaryPane()` in `ContentPanel.h`
