# Implementation Plan - PresetTagsDialog-3.md

## 1. Overview
This implementation plan enhances `PresetTagsDialog` so that when a user adds new preset tags to a Library category or Favorite folder and saves the settings, the newly added preset tags are automatically retroactively applied to all existing/historical items currently bound to that category or favorite.

As requested by the user:
- Compute newly added preset tags by comparing old vs new preset tag lists.
- Retrieve all associated paths currently belonging to the category or favorite folder.
- Execute `AppCommandType::AddTag` via `CoreEngine` to batch-apply the new tags to all existing items.
- Save the updated preset tags list to the database via `LibraryDao::updatePresetTags` or `FavoriteDao::updatePresetTags`.

In accordance with AGENTS.md rules:
- Source files are NOT modified directly in this plan stage.
- This plan is strictly isolated in `QuarkMeta Architecture/Implementation Plan/PresetTagsDialog-3.md`.

## 2. Modified Files List
- `src/ui/PresetTagsDialog.h`
- `src/ui/PresetTagsDialog.cpp`

## 3. Detailed Line-by-Line Changes

### 1. `src/ui/PresetTagsDialog.h`

```diff
<<<<<<< SEARCH
    int m_categoryId = 0;
    bool m_isLibrary = true;
    QString m_categoryName;
    QStringList m_presetTags;
=======
    int m_categoryId = 0;
    bool m_isLibrary = true;
    QString m_categoryName;
    QStringList m_presetTags;
    QStringList m_initialPresetTags;
>>>>>>> REPLACE
```

### 2. `src/ui/PresetTagsDialog.cpp`

```diff
<<<<<<< SEARCH
void PresetTagsDialog::loadTags() {
    if (m_isLibrary) {
        auto list = LibraryDao::getAllCategories();
        for (const auto& rec : list) {
            if (rec.id == m_categoryId) {
                m_categoryName = rec.name;
                m_presetTags = rec.presetTags;
                break;
            }
        }
    } else {
        auto list = FavoriteDao::getAllFavorites();
        for (const auto& rec : list) {
            if (rec.id == m_categoryId) {
                m_categoryName = rec.name;
                m_presetTags = rec.presetTags;
                break;
            }
        }
    }
    m_folderNameEdit->setText(m_categoryName);
}
=======
void PresetTagsDialog::loadTags() {
    if (m_isLibrary) {
        auto list = LibraryDao::getAllCategories();
        for (const auto& rec : list) {
            if (rec.id == m_categoryId) {
                m_categoryName = rec.name;
                m_presetTags = rec.presetTags;
                m_initialPresetTags = rec.presetTags;
                break;
            }
        }
    } else {
        auto list = FavoriteDao::getAllFavorites();
        for (const auto& rec : list) {
            if (rec.id == m_categoryId) {
                m_categoryName = rec.name;
                m_presetTags = rec.presetTags;
                m_initialPresetTags = rec.presetTags;
                break;
            }
        }
    }
    m_folderNameEdit->setText(m_categoryName);
}
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
#include "PresetTagsDialog.h"
#include "UiHelper.h"
#include "../meta/FavoriteDao.h"
#include "../meta/FavoriteService.h"
#include "../meta/LibraryDao.h"
#include "../meta/MetadataManager.h"
=======
#include "PresetTagsDialog.h"
#include "UiHelper.h"
#include "../meta/FavoriteDao.h"
#include "../meta/FavoriteService.h"
#include "../meta/LibraryDao.h"
#include "../meta/MetadataManager.h"
#include "../core/CoreEngine.h"
#include "ToolTipOverlay.h"
#include <QCursor>
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void PresetTagsDialog::onSaveClicked() {
    if (m_isLibrary) {
        LibraryDao::updatePresetTags(m_categoryId, m_presetTags);
    } else {
        FavoriteDao::updatePresetTags(m_categoryId, m_presetTags);
    }
    accept();
}
=======
void PresetTagsDialog::onSaveClicked() {
    QStringList newlyAddedTags;
    for (const QString& tag : m_presetTags) {
        if (!m_initialPresetTags.contains(tag)) {
            newlyAddedTags.append(tag);
        }
    }

    if (m_isLibrary) {
        LibraryDao::updatePresetTags(m_categoryId, m_presetTags);
        if (!newlyAddedTags.isEmpty()) {
            QStringList associatedPaths = LibraryDao::getCategoryPaths(m_categoryId);
            if (!associatedPaths.isEmpty()) {
                for (const QString& tag : newlyAddedTags) {
                    AppCommand cmd;
                    cmd.type = AppCommandType::AddTag;
                    cmd.targetPaths = associatedPaths;
                    cmd.params["tag"] = tag;
                    CoreEngine::instance().executeCommand(cmd);
                }
                ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已自动追溯将 %1 个新预设标签绑定至既有 %2 个项目").arg(newlyAddedTags.size()).arg(associatedPaths.size()), 2000, QColor("#2ecc71"));
            }
        }
    } else {
        FavoriteDao::updatePresetTags(m_categoryId, m_presetTags);
    }
    accept();
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build `QuarkMeta` target:
   ```bash
   cmake --build build --config Release
   ```
2. Run `QuarkMeta`, open the "库" tab in the left sidebar:
   - Right-click a category that already has bound files and select "设置预设标签".
   - Add a new preset tag and click "保存设置".
   - Verify that the newly added preset tag is immediately retroactively applied to all existing files in that category, and metadata tags update across the app.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: Reuses `CoreEngine::executeCommand(AppCommandType::AddTag)`, `LibraryDao::getCategoryPaths`, and `ToolTipOverlay`.
- **Zero Redundancy**: Directly binds new tags using existing metadata pipeline with zero duplicate logic.

## 6. Header API Signature Verification
- `PresetTagsDialog::m_initialPresetTags` in `src/ui/PresetTagsDialog.h`.
- `LibraryDao::getCategoryPaths(int id)` in `src/meta/LibraryDao.h`.

## 7. Header Inclusion Chain & Type Completeness Check
- Added `#include "../core/CoreEngine.h"` and `#include "ToolTipOverlay.h"` in `src/ui/PresetTagsDialog.cpp`.
- Type completeness checked for `CoreEngine`, `AppCommand`, and `ToolTipOverlay`.
