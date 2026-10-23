# ContentPanel Re-Split Layout Restoration Plan

## Overview
Fixes the potential layout issue when `splitPane()` is called a second time after `closeSecondaryPane()` had returned `m_headerWidget` and `m_viewStack` to `m_mainLayout`.

---

## Modified Files List
- `src/ui/ContentPanel.cpp`

---

## Detailed Line-by-Line Changes

### File: `src/ui/ContentPanel.cpp`

```diff
<<<<<<< SEARCH
    } else {
        m_paneSplitter->setOrientation(m_splitOrientation);
        if (m_primaryPaneContainer) {
            m_primaryPaneContainer->show();
        }
        if (m_secondaryPaneContainer) {
            m_secondaryPaneContainer->show();
        }
        if (m_paneSplitter) {
            m_paneSplitter->show();
        }
    }
=======
    } else {
        m_paneSplitter->setOrientation(m_splitOrientation);
        if (m_primaryPaneContainer) {
            if (m_headerWidget && m_primaryPaneContainer->layout()) {
                m_mainLayout->removeWidget(m_headerWidget);
                m_primaryPaneContainer->layout()->addWidget(m_headerWidget);
            }
            if (m_viewStack && m_primaryPaneContainer->layout()) {
                m_mainLayout->removeWidget(m_viewStack);
                m_primaryPaneContainer->layout()->addWidget(m_viewStack);
            }
            m_primaryPaneContainer->show();
        }
        if (m_secondaryPaneContainer) {
            m_secondaryPaneContainer->show();
        }
        if (m_paneSplitter) {
            m_paneSplitter->show();
        }
    }
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. Recompile `ContentPanel.cpp`.
2. Verify re-splitting behavior after closing and re-opening split pane.

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- Correctly reparents `m_headerWidget` and `m_viewStack` into `m_primaryPaneContainer` during subsequent `splitPane()` invocations.
