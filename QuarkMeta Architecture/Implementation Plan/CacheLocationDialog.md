# Implementation Plan - Cache Storage Location Management Dialog & Plugin Bar Integration

## 1. Overview
This implementation plan adds a dedicated cache storage location management button and dialog to QuarkMeta:
1. Adds a new plugin button `m_btnSaveThumbnail` to the plugin bar (`DriveBarWidget`), using the `save-Thumbnail` vector icon from `SvgIcons.h`.
2. Creates a custom frameless dialog `CacheLocationDialog` (`src/ui/dialogs/CacheLocationDialog.h` / `.cpp`) allowing users to view the current cache path and disk usage, browse and select a custom cache folder (e.g. on drive D or E), and optionally migrate existing thumbnail and preview cache data to the new location.
3. Updates `DiskMediaExtractor.cpp` and `QuickLookWindow.cpp` to use the unified cache root location provided by `AppConfig::instance().getGlobalCacheDir()`.
4. Registers `CacheLocationDialog.h` and `CacheLocationDialog.cpp` in `CMakeLists.txt`.

---

## 2. Modified Files List
- `CMakeLists.txt`
- `src/core/AppConfig.h`
- `src/util/DiskMediaExtractor.cpp`
- `src/ui/QuickLookWindow.cpp`
- `src/ui/DriveBarWidget.h`
- `src/ui/DriveBarWidget.cpp`
- `src/ui/dialogs/CacheLocationDialog.h` (New File)
- `src/ui/dialogs/CacheLocationDialog.cpp` (New File)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `CMakeLists.txt`
Register `CacheLocationDialog.h` and `CacheLocationDialog.cpp`:

```cmake
<<<<<<< SEARCH
    src/ui/dialogs/TextExtensionDialog.h
    src/ui/dialogs/TextExtensionDialog.cpp
=======
    src/ui/dialogs/TextExtensionDialog.h
    src/ui/dialogs/TextExtensionDialog.cpp
    src/ui/dialogs/CacheLocationDialog.h
    src/ui/dialogs/CacheLocationDialog.cpp
>>>>>>> REPLACE
```

### 3.2 `src/core/AppConfig.h`
Add `#include <QStandardPaths>` and `getGlobalCacheDir()` method to `AppConfig`:

```cpp
<<<<<<< SEARCH
#include <QSettings>
#include <QVariant>
#include <QString>
#include <QMutex>
=======
#include <QSettings>
#include <QVariant>
#include <QString>
#include <QMutex>
#include <QStandardPaths>
#include <QDir>
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    void sync() {
        QMutexLocker lock(&m_mutex);
        m_settings.sync();
    }
=======
    void sync() {
        QMutexLocker lock(&m_mutex);
        m_settings.sync();
    }

    QString getGlobalCacheDir() const {
        QMutexLocker lock(&m_mutex);
        QString customPath = m_settings.value("Cache/CustomLocation").toString();
        if (!customPath.isEmpty() && QDir(customPath).exists()) {
            return customPath + "/cache";
        }
        return QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    }
>>>>>>> REPLACE
```

### 3.3 `src/util/DiskMediaExtractor.cpp`
Use `AppConfig::instance().getGlobalCacheDir()` for disk thumbnail cache path:

```cpp
<<<<<<< SEARCH
    QString cacheDir = QCoreApplication::applicationDirPath() + "/.QuarkMeta/disk_thumbs/" + volStr + "/" + bucket;
=======
    QString cacheDir = AppConfig::instance().getGlobalCacheDir() + "/disk_thumbs/" + volStr + "/" + bucket;
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    return QCoreApplication::applicationDirPath() + "/.QuarkMeta/disk_thumbs/fallback/" + bucket + "/" + fileKey;
=======
    return AppConfig::instance().getGlobalCacheDir() + "/disk_thumbs/fallback/" + bucket + "/" + fileKey;
>>>>>>> REPLACE
```

### 3.4 `src/ui/QuickLookWindow.cpp`
Use `AppConfig::instance().getGlobalCacheDir()` for QuickLook preview cache path:

```cpp
<<<<<<< SEARCH
    QString baseCacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/quicklook_previews_v2";
=======
    QString baseCacheDir = AppConfig::instance().getGlobalCacheDir() + "/quicklook_previews";
>>>>>>> REPLACE
```

### 3.5 `src/ui/DriveBarWidget.h`
Add `m_btnSaveThumbnail` button and getter:

```cpp
<<<<<<< SEARCH
    QPushButton* textExtensionButton() const { return m_btnTextExtension; }
    QPushButton* extensionManagerButton() const { return m_btnExtensionManager; }
=======
    QPushButton* textExtensionButton() const { return m_btnTextExtension; }
    QPushButton* saveThumbnailButton() const { return m_btnSaveThumbnail; }
    QPushButton* extensionManagerButton() const { return m_btnExtensionManager; }
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    QPushButton* m_btnTagManager = nullptr;
    QPushButton* m_btnTextExtension = nullptr;
=======
    QPushButton* m_btnTagManager = nullptr;
    QPushButton* m_btnTextExtension = nullptr;
    QPushButton* m_btnSaveThumbnail = nullptr;
>>>>>>> REPLACE
```

### 3.6 `src/ui/DriveBarWidget.cpp`
Instantiate `m_btnSaveThumbnail` with `save-Thumbnail` icon, connect to `CacheLocationDialog`, and add to layout/menu:

```cpp
<<<<<<< SEARCH
#include "dialogs/TextExtensionDialog.h"
=======
#include "dialogs/TextExtensionDialog.h"
#include "dialogs/CacheLocationDialog.h"
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    m_btnTextExtension = createIconButton("Ext.svg", QColor("#e67e22"), "文本扩展名设置");
    connect(m_btnTextExtension, &QPushButton::clicked, this, [this]() {
        TextExtensionDialog dlg(this);
        dlg.exec();
    });

    // 默认加至布局并依配置显隐
    m_driveBarLayout->addWidget(m_btnTagManager);
    m_driveBarLayout->addWidget(m_btnTextExtension);
=======
    m_btnTextExtension = createIconButton("Ext.svg", QColor("#e67e22"), "文本扩展名设置");
    connect(m_btnTextExtension, &QPushButton::clicked, this, [this]() {
        TextExtensionDialog dlg(this);
        dlg.exec();
    });

    m_btnSaveThumbnail = createIconButton("save-Thumbnail", QColor("#3498db"), "缩略图与缓存位置设置");
    connect(m_btnSaveThumbnail, &QPushButton::clicked, this, [this]() {
        CacheLocationDialog dlg(this);
        dlg.exec();
    });

    // 默认加至布局并依配置显隐
    m_driveBarLayout->addWidget(m_btnTagManager);
    m_driveBarLayout->addWidget(m_btnTextExtension);
    m_driveBarLayout->addWidget(m_btnSaveThumbnail);
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
void DriveBarWidget::refreshPinnedButtons() {
    QStringList pinnedList = AppConfig::instance().getValue("DriveBar/PinnedExtensions", QStringList{"tag", "text_ext"}).toStringList();

    if (m_btnTagManager) {
        m_btnTagManager->setVisible(pinnedList.contains("tag"));
    }
    if (m_btnTextExtension) {
        m_btnTextExtension->setVisible(pinnedList.contains("text_ext"));
    }
}
=======
void DriveBarWidget::refreshPinnedButtons() {
    QStringList pinnedList = AppConfig::instance().getValue("DriveBar/PinnedExtensions", QStringList{"tag", "text_ext", "save_thumbnail"}).toStringList();

    if (m_btnTagManager) {
        m_btnTagManager->setVisible(pinnedList.contains("tag"));
    }
    if (m_btnTextExtension) {
        m_btnTextExtension->setVisible(pinnedList.contains("text_ext"));
    }
    if (m_btnSaveThumbnail) {
        m_btnSaveThumbnail->setVisible(pinnedList.contains("save_thumbnail"));
    }
}
>>>>>>> REPLACE
```

### 3.7 New Header File `src/ui/dialogs/CacheLocationDialog.h`
Create new frameless dialog class for cache path management:

```cpp
#pragma once

#include "FramelessDialog.h"
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>

namespace QuarkMeta {

class CacheLocationDialog : public FramelessDialog {
    Q_OBJECT
public:
    explicit CacheLocationDialog(QWidget* parent = nullptr);
    ~CacheLocationDialog() override = default;

private slots:
    void onBrowsePath();
    void onRestoreDefault();
    void onSaveAndAccept();

private:
    void setupUi();
    void updatePathDisplay();
    qint64 calculateDirectorySize(const QString& path);

    QLineEdit* m_pathEdit{nullptr};
    QPushButton* m_btnBrowse{nullptr};
    QLabel* m_sizeLabel{nullptr};
    QCheckBox* m_chkMigrateData{nullptr};
    QPushButton* m_btnRestore{nullptr};
    QPushButton* m_btnCancel{nullptr};
    QPushButton* m_btnSave{nullptr};
};

} // namespace QuarkMeta
```

### 3.8 New Source File `src/ui/dialogs/CacheLocationDialog.cpp`
Implement cache path selection, directory size calculation, and data migration:

```cpp
#include "dialogs/CacheLocationDialog.h"
#include "UiHelper.h"
#include "../core/AppConfig.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QDirIterator>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

namespace QuarkMeta {

CacheLocationDialog::CacheLocationDialog(QWidget* parent)
    : FramelessDialog(parent) {
    setWindowTitle("缩略图与缓存位置设置");
    resize(520, 240);
    setupUi();
    updatePathDisplay();
}

void CacheLocationDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(contentArea());
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    auto* pathLayout = new QHBoxLayout();
    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setReadOnly(true);
    UiHelper::setupLineEditContextMenu(m_pathEdit);

    m_btnBrowse = new QPushButton("浏览...", this);
    connect(m_btnBrowse, &QPushButton::clicked, this, &CacheLocationDialog::onBrowsePath);

    pathLayout->addWidget(m_pathEdit);
    pathLayout->addWidget(m_btnBrowse);
    mainLayout->addLayout(pathLayout);

    m_sizeLabel = new QLabel("当前缓存占用: 计算中...", this);
    mainLayout->addWidget(m_sizeLabel);

    m_chkMigrateData = new QCheckBox("迁移现有缓存数据到新目录", this);
    m_chkMigrateData->setChecked(true);
    mainLayout->addWidget(m_chkMigrateData);

    mainLayout->addStretch();

    auto* btnLayout = new QHBoxLayout();
    m_btnRestore = new QPushButton("恢复默认", this);
    connect(m_btnRestore, &QPushButton::clicked, this, &CacheLocationDialog::onRestoreDefault);

    m_btnCancel = new QPushButton("取消", this);
    connect(m_btnCancel, &QPushButton::clicked, this, &CacheLocationDialog::reject);

    m_btnSave = new QPushButton("保存", this);
    connect(m_btnSave, &QPushButton::clicked, this, &CacheLocationDialog::onSaveAndAccept);

    btnLayout->addWidget(m_btnRestore);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnCancel);
    btnLayout->addWidget(m_btnSave);
    mainLayout->addLayout(btnLayout);
}

void CacheLocationDialog::updatePathDisplay() {
    QString currentCacheDir = AppConfig::instance().getGlobalCacheDir();
    m_pathEdit->setText(currentCacheDir);

    qint64 totalBytes = calculateDirectorySize(currentCacheDir);
    double mbSize = totalBytes / (1024.0 * 1024.0);
    m_sizeLabel->setText(QString("当前缓存占用: %1 MB").arg(mbSize, 0, 'f', 2));
}

qint64 CacheLocationDialog::calculateDirectorySize(const QString& path) {
    qint64 size = 0;
    QDirIterator it(path, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        size += it.fileInfo().size();
    }
    return size;
}

void CacheLocationDialog::onBrowsePath() {
    QString selectedDir = QFileDialog::getExistingDirectory(this, "选择缓存存储根目录", m_pathEdit->text());
    if (!selectedDir.isEmpty()) {
        m_pathEdit->setText(selectedDir);
    }
}

void CacheLocationDialog::onRestoreDefault() {
    AppConfig::instance().remove("Cache/CustomLocation");
    AppConfig::instance().sync();
    updatePathDisplay();
}

void CacheLocationDialog::onSaveAndAccept() {
    QString newDir = m_pathEdit->text().trimmed();
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

    if (newDir != defaultDir && !newDir.isEmpty()) {
        AppConfig::instance().setValue("Cache/CustomLocation", newDir);
    } else {
        AppConfig::instance().remove("Cache/CustomLocation");
    }
    AppConfig::instance().sync();
    accept();
}

} // namespace QuarkMeta
```

---

## 4. Build & Verification Steps
1. Recompile QuarkMeta target using CMake:
   `cmake --build build`
2. Launch QuarkMeta and observe the plugin bar (`DriveBarWidget`).
3. Confirm that the new `save-Thumbnail` icon button is visible on the plugin bar.
4. Click the `save-Thumbnail` button to open `CacheLocationDialog`.
5. Verify that:
   - Current cache path and disk usage size are displayed correctly.
   - Clicking "Browse..." opens a directory selection dialog.
   - Saving a custom directory updates `AppConfig` and routes all subsequent thumbnail and QuickLook preview caches to the specified path.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Single Cache SSOT**: All cache components (`DiskMediaExtractor`, `QuickLookWindow`, `CacheLocationDialog`) route through `AppConfig::instance().getGlobalCacheDir()`.
- **Icon Reuse**: Uses existing `"save-Thumbnail"` icon key from `SvgIcons.h`.

---

## 6. Header API Signature Verification
| Class / Function Signature | Declared Header File | Signature Match Verification |
| :--- | :--- | :--- |
| `QPushButton* saveThumbnailButton() const` | `src/ui/DriveBarWidget.h` | 100% Exact Match |
| `QString getGlobalCacheDir() const` | `src/core/AppConfig.h` | 100% Exact Match |
| `explicit CacheLocationDialog(QWidget* parent = nullptr)` | `src/ui/dialogs/CacheLocationDialog.h` | 100% Exact Match |

---

## 7. Header Inclusion Chain & Type Completeness Check
| Inclusion / Type Name | File | Purpose & Completeness Check |
| :--- | :--- | :--- |
| `#include "FramelessDialog.h"` | `src/ui/dialogs/CacheLocationDialog.h` | Provides `FramelessDialog` base class. |
| `#include "dialogs/CacheLocationDialog.h"` | `src/ui/DriveBarWidget.cpp` | Provides `CacheLocationDialog` definition. |
| `#include <QFileDialog>` | `src/ui/dialogs/CacheLocationDialog.cpp` | Provides `QFileDialog` for folder selection. |
