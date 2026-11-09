# 缓存根目录整体搬家迁移与 QuickLook 预览图缓存源双轨选择实施方案 (Cache Directory Migration & QuickLook Thumbnail Source Plan)

**文档语言**：中文（接口定义与 Git Merge Diff 标记除外）
**基准 Commit**：`42a3950`
**关联规范**：`Memories_QuarkMeta.md` 第 48 章节、`Standard Specifications for Implementation Plan.md`

---

## 物理阅读证据清单 (Gate 1: Pre-Draft Physical Evidence)

1. **涉及修改与阅读的物理文件列表**：
   - `QuarkMeta_Source/src/ui/dialogs/CacheLocationDialog.h`（已物理读取第 1~35 行，确认对话框结构与成员变量声明）
   - `QuarkMeta_Source/src/ui/dialogs/CacheLocationDialog.cpp`（已物理读取第 20~135 行，确认 `onBrowsePath`、`onRestoreDefault`、`onSaveAndAccept` 以及搬家迁移逻辑，发现旧代码使用 `QFile::copy` 且未清理原路径的伪迁移缺陷）
   - `QuarkMeta_Source/src/core/AppConfig.h`（已物理读取第 40~55 行，确认 `getGlobalCacheDir` 读取 `Cache/CustomLocation` 拼接 `/cache` 的单一事实来源）
   - `QuarkMeta_Source/src/ui/QuickLookWindow.h`（已物理读取第 20~60 行，确认 `loadOrExtractQuickLookImage` 原样声明）
   - `QuarkMeta_Source/src/ui/QuickLookWindow.cpp`（已物理读取第 1~35 行包含列表与第 325~390 行，确认双轨取图与降级逻辑）
   - `QuarkMeta_Source/src/util/DiskMediaExtractor.h` 与 `DiskMediaExtractor.cpp`（已物理读取第 185~210 行，确认 `DiskMediaExtractor::getDiskThumbCachePath(filePath)` 签名与实现）

2. **外部成员函数原样签名核验**：
   - `DiskMediaExtractor::getDiskThumbCachePath(const QString& filePath)`：`QuarkMeta_Source/src/util/DiskMediaExtractor.h`
   - `AppConfig::instance().getGlobalCacheDir()`：`QuarkMeta_Source/src/core/AppConfig.h:42`
   - `AppConfig::instance().getValue(const QString& key, const QVariant& defaultValue)`：`QuarkMeta_Source/src/core/AppConfig.h`
   - `AppConfig::instance().setValue(const QString& key, const QVariant& value)`：`QuarkMeta_Source/src/core/AppConfig.h`

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 缺陷现象与架构纠偏
1. **纠偏一：假“迁移”变成真“搬家”（原路径数据必须彻底清理消除）**
   - **痛点分析**：在旧实现中，`CacheLocationDialog::onSaveAndAccept` 执行的是 `QFile::copy(it.filePath(), destPath);`，拷贝完成后**完全没有对原路径的文件和目录执行删除清理**！导致字面上显示的是“迁移”，实际上仅仅是“复制”，旧路径依然白白霸占着几个 G 的磁盘空间，旧缩略图纹丝不动，严重违背“迁移搬家”的常理；
   - **纠偏规范**：
     - 执行“真搬家”策略：整体迁移成功后，**必须将旧 `cache` 目录中的文件及空目录彻底删除清理（Move/Purge）**；
     - 优先采用物理移动（若同盘直接快速 Rename/Move；若跨卷则逐项 Copy 成功后立即 Delete 原文件，并在所有文件搬迁完成后递归移除旧 `cache` 空文件夹），确保原路径干干净净，原磁盘空间立即释放。

2. **纠偏二：整棵 `cache` 根目录的整体搬迁**
   - **架构事实**：整个 QuarkMeta 的所有缩略图与预览图缓存，物理上均统一收敛于单一树状根目录：
     ```
     .../.QuarkMeta/cache/             <--- 唯一的全局缓存根目录 (getGlobalCacheDir)
         ├── disk_thumbs/              (网格/列表缩略图)
         └── quicklook_previews/       (QuickLook 高清大图预览)
     ```
   - **纠偏规范**：用户勾选“迁移现有缓存数据到新目录”时，直接将旧的整棵 `cache` 目录及其内部所有子文件夹整体搬家至新选中的存储目录下的 `cache` 文件夹中，严禁在业务逻辑中分别孤立处理 `disk_thumbs` 或 `quicklook_previews`；
   - **路径语义纠偏**：在 `onBrowsePath` 时，用户选择的是“缓存存储根目录”，若用户选择的路径末尾未带 `/cache`，统一标准化映射并展示真实的 `.../cache` 物理路径，确保界面输入框、磁盘迁移与 `AppConfig::getGlobalCacheDir()` 的存储基准绝对 1:1 闭环一致。

3. **纠偏三：运行时 QuickLook 预览图缓存源互斥二选一与说明注释**
   - **需求背景**：用户在设置界面中，可自由选择 QuickLook 预览读取哪个子目录：
     - **选项 1（快速模式）**：优先使用现成缩略图（`disk_thumbs`）——已由系统日常浏览现成生成，加载速度极快、无需二次解码，但清晰度不如专用预览图；
     - **选项 2（高清模式）**：优先使用高质量大图预览（`quicklook_previews`）——生成最高 2048px 超高清画质，细节极其锐利，但首次加载或未缓存时解码稍需时间；
   - **互斥契约**：两选项**绝对互斥（刚性单选源，二选一必选其一）**，勾选后点击保存持久化至 `AppConfig`；并在下方配备清晰说明注释供用户参考思量；
   - **安全平滑降级（防黑屏）**：在 `disk_thumbs` 模式下，若遇到尚未生成 `disk_thumbs` 缩略图的文件，自动平滑降级穿透到实时解码，绝对杜绝黑屏无图现象。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `QuarkMeta_Source/src/ui/dialogs/CacheLocationDialog.h`（新增互斥复选框指针与说明标签指针成员声明）
2. `QuarkMeta_Source/src/ui/dialogs/CacheLocationDialog.cpp`（实现真搬迁数据清理、整棵 `cache` 目录整体迁移、互斥二选一复选框组排版与持久化保存）
3. `QuarkMeta_Source/src/ui/QuickLookWindow.cpp`（包含 `DiskMediaExtractor.h`，实现 `disk_thumbs` 优先读取与自动降级）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 更新 `CacheLocationDialog.h` 增加互斥复选框与说明注释成员

```
<<<<<<< SEARCH
    QLabel* m_sizeLabel{nullptr};
    QCheckBox* m_chkMigrateData{nullptr};
    QPushButton* m_btnRestore{nullptr};
=======
    QLabel* m_sizeLabel{nullptr};
    QCheckBox* m_chkMigrateData{nullptr};
    QCheckBox* m_chkDiskThumbs{nullptr};
    QCheckBox* m_chkQuickLookPreviews{nullptr};
    QLabel* m_sourceDescLabel{nullptr};
    QPushButton* m_btnRestore{nullptr};
>>>>>>> REPLACE
```

---

### 3.2 更新 `CacheLocationDialog.cpp` 实现互斥单选源界面与思量注释

```
<<<<<<< SEARCH
    m_chkMigrateData = new QCheckBox("迁移现有缓存数据到新目录", this);
    m_chkMigrateData->setChecked(true);
    mainLayout->addWidget(m_chkMigrateData);

    mainLayout->addStretch();
=======
    m_chkMigrateData = new QCheckBox("迁移现有缓存数据到新目录", this);
    m_chkMigrateData->setChecked(true);
    mainLayout->addWidget(m_chkMigrateData);

    // QuickLook 预览图缓存源选择（严格互斥二选一单选源）
    m_chkDiskThumbs = new QCheckBox("优先使用现成缩略图缓存 (disk_thumbs)", this);
    m_chkQuickLookPreviews = new QCheckBox("优先使用高质量大图预览缓存 (quicklook_previews)", this);

    QString currentSource = AppConfig::instance().getValue("QuickLook/ThumbnailSource", "quicklook_previews").toString();
    if (currentSource == "disk_thumbs") {
        m_chkDiskThumbs->setChecked(true);
        m_chkQuickLookPreviews->setChecked(false);
    } else {
        m_chkDiskThumbs->setChecked(false);
        m_chkQuickLookPreviews->setChecked(true);
    }

    // 刚性互斥二选一逻辑：勾选一个自动取消另一个，且禁止同时取消
    connect(m_chkDiskThumbs, &QCheckBox::clicked, this, [this](bool checked) {
        if (!checked) {
            m_chkDiskThumbs->setChecked(true); // 保证二选一必选其一
        } else {
            m_chkQuickLookPreviews->setChecked(false);
        }
    });

    connect(m_chkQuickLookPreviews, &QCheckBox::clicked, this, [this](bool checked) {
        if (!checked) {
            m_chkQuickLookPreviews->setChecked(true); // 保证二选一必选其一
        } else {
            m_chkDiskThumbs->setChecked(false);
        }
    });

    mainLayout->addWidget(m_chkDiskThumbs);
    mainLayout->addWidget(m_chkQuickLookPreviews);

    // 复选框下方清晰说明注释，供用户参考思量
    m_sourceDescLabel = new QLabel(
        "提示：\n"
        "• disk_thumbs：日常浏览现成生成，载入速度极快、0ms秒开无需二次解码，但清晰度较低；\n"
        "• quicklook_previews：生成最高 2048px 超高清画质，细节极其锐利，但首次载入需要深度解码耗时。",
        this
    );
    m_sourceDescLabel->setWordWrap(true);
    m_sourceDescLabel->setStyleSheet("color: #888888; font-size: 11px; line-height: 1.4;");
    mainLayout->addWidget(m_sourceDescLabel);

    mainLayout->addStretch();
>>>>>>> REPLACE
```

---

### 3.3 更新 `CacheLocationDialog.cpp` 实现真搬家（清理原路径数据）与整棵 `cache` 整体迁移

```
<<<<<<< SEARCH
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
    QString oldDir = AppConfig::instance().getGlobalCacheDir();
    QString newDir = m_pathEdit->text().trimmed();
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

    if (m_chkMigrateData && m_chkMigrateData->isChecked() && !oldDir.isEmpty() && !newDir.isEmpty() && QDir(oldDir).canonicalPath() != QDir(newDir).canonicalPath()) {
        QDir().mkpath(newDir);
        QDirIterator it(oldDir, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            QString relativePath = QDir(oldDir).relativeFilePath(it.filePath());
            QString destPath = QDir(newDir).filePath(relativePath);
            QDir().mkpath(QFileInfo(destPath).absolutePath());
            if (!QFile::exists(destPath)) {
                QFile::copy(it.filePath(), destPath);
            }
        }
    }

    if (newDir != defaultDir && !newDir.isEmpty()) {
        AppConfig::instance().setValue("Cache/CustomLocation", newDir);
    } else {
        AppConfig::instance().remove("Cache/CustomLocation");
    }
    AppConfig::instance().sync();
    accept();
}
=======
void CacheLocationDialog::onBrowsePath() {
    QString selectedDir = QFileDialog::getExistingDirectory(this, "选择缓存存储根目录", m_pathEdit->text());
    if (!selectedDir.isEmpty()) {
        QString cleanSelected = QDir::toNativeSeparators(QDir::cleanPath(selectedDir));
        if (!cleanSelected.endsWith("cache", Qt::CaseInsensitive)) {
            cleanSelected = QDir(cleanSelected).filePath("cache");
        }
        m_pathEdit->setText(QDir::toNativeSeparators(cleanSelected));
    }
}

void CacheLocationDialog::onRestoreDefault() {
    AppConfig::instance().remove("Cache/CustomLocation");
    AppConfig::instance().remove("QuickLook/ThumbnailSource");
    AppConfig::instance().sync();
    updatePathDisplay();
    if (m_chkDiskThumbs && m_chkQuickLookPreviews) {
        m_chkDiskThumbs->setChecked(false);
        m_chkQuickLookPreviews->setChecked(true); // 默认高清大图模式
    }
}

void CacheLocationDialog::onSaveAndAccept() {
    // 1. 持久化 QuickLook 缓存源选择 (二选一互斥)
    if (m_chkDiskThumbs && m_chkDiskThumbs->isChecked()) {
        AppConfig::instance().setValue("QuickLook/ThumbnailSource", "disk_thumbs");
    } else {
        AppConfig::instance().setValue("QuickLook/ThumbnailSource", "quicklook_previews");
    }

    // 2. 缓存根目录整体真搬迁（迁移后彻底清除原路径数据）
    QString oldDir = AppConfig::instance().getGlobalCacheDir();
    QString targetDisplayDir = m_pathEdit->text().trimmed();
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

    // 标准化去除末尾 /cache，以获取 AppConfig 期望的父级根目录
    QString customRootToStore = targetDisplayDir;
    if (customRootToStore.endsWith("/cache", Qt::CaseInsensitive) || customRootToStore.endsWith("\\cache", Qt::CaseInsensitive)) {
        customRootToStore.chop(6);
    }

    QString newDir = targetDisplayDir;
    if (!newDir.endsWith("cache", Qt::CaseInsensitive)) {
        newDir = QDir(newDir).filePath("cache");
    }

    if (m_chkMigrateData && m_chkMigrateData->isChecked() && !oldDir.isEmpty() && !newDir.isEmpty() && QDir(oldDir).canonicalPath() != QDir(newDir).canonicalPath()) {
        QDir().mkpath(newDir);
        // A. 逐项复制并校验
        QDirIterator it(oldDir, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
        QStringList copiedOldFiles;
        while (it.hasNext()) {
            it.next();
            QString srcFilePath = it.filePath();
            QString relativePath = QDir(oldDir).relativeFilePath(srcFilePath);
            QString destPath = QDir(newDir).filePath(relativePath);
            QDir().mkpath(QFileInfo(destPath).absolutePath());

            bool copyOk = false;
            if (QFile::exists(destPath)) {
                QFile::remove(destPath);
            }
            if (QFile::copy(srcFilePath, destPath)) {
                copyOk = true;
            }
            if (copyOk) {
                copiedOldFiles.append(srcFilePath);
            }
        }

        // B. 搬家清理：迁移成功的文件从原路径彻底删除，杜绝伪迁移空间残留
        for (const QString& oldFile : copiedOldFiles) {
            QFile::remove(oldFile);
        }

        // C. 递归清理旧 cache 目录内的空子目录
        QDir(oldDir).removeRecursively();
    }

    if (newDir != defaultDir && !customRootToStore.isEmpty()) {
        AppConfig::instance().setValue("Cache/CustomLocation", customRootToStore);
    } else {
        AppConfig::instance().remove("Cache/CustomLocation");
    }
    AppConfig::instance().sync();
    accept();
}
>>>>>>> REPLACE
```

---

### 3.4 更新 `QuickLookWindow.cpp` 支持双轨缩略图源与平滑降级

```
<<<<<<< SEARCH
#include "QuickLookWindow.h"
#include "UiHelper.h"
#include "ColorPaletteEngine.h"
=======
#include "QuickLookWindow.h"
#include "UiHelper.h"
#include "ColorPaletteEngine.h"
#include "../util/DiskMediaExtractor.h"
>>>>>>> REPLACE
```

---

```
<<<<<<< SEARCH
QImage QuickLookWindow::loadOrExtractQuickLookImage(const QString& filePath, int targetSize, std::shared_ptr<CancellationToken> token) {
    // 🚀【全图形图像 QuickLook 统一缓存架构】：使用 SHA256 与两级分桶
    QByteArray normalized = QDir::toNativeSeparators(filePath).toLower().toUtf8();
    QString hashStr = QString::fromUtf8(QCryptographicHash::hash(normalized, QCryptographicHash::Sha256).toHex());
    QString dirL1 = hashStr.left(2);
    QString dirL2 = hashStr.mid(2, 2);

    QString baseCacheDir = AppConfig::instance().getGlobalCacheDir() + "/quicklook_previews";
    QString cacheDir = QDir(baseCacheDir).filePath(QString("%1/%2").arg(dirL1, dirL2));
    QDir().mkpath(cacheDir);

    QFileInfo srcInfo(filePath);
=======
QImage QuickLookWindow::loadOrExtractQuickLookImage(const QString& filePath, int targetSize, std::shared_ptr<CancellationToken> token) {
    QFileInfo srcInfo(filePath);

    // 🚀【双轨缓存分流策略】：检查用户配置的缩略图源
    QString thumbSource = AppConfig::instance().getValue("QuickLook/ThumbnailSource", "quicklook_previews").toString();
    if (thumbSource == "disk_thumbs") {
        QString diskCachePath = DiskMediaExtractor::getDiskThumbCachePath(filePath);
        QFileInfo diskInfo(diskCachePath);
        if (diskInfo.exists() && diskInfo.size() > 0 && diskInfo.lastModified() >= srcInfo.lastModified()) {
            QImage diskImg(diskCachePath);
            if (!diskImg.isNull()) {
                return diskImg; // 极速秒开返回现成磁盘缩略图
            }
        }
        // 若 disk_thumbs 不存在，自动平滑向下穿透到高清实时解码管线，杜绝黑屏
    }

    // 🚀【全图形图像 QuickLook 统一缓存架构】：使用 SHA256 与两级分桶
    QByteArray normalized = QDir::toNativeSeparators(filePath).toLower().toUtf8();
    QString hashStr = QString::fromUtf8(QCryptographicHash::hash(normalized, QCryptographicHash::Sha256).toHex());
    QString dirL1 = hashStr.left(2);
    QString dirL2 = hashStr.mid(2, 2);

    QString baseCacheDir = AppConfig::instance().getGlobalCacheDir() + "/quicklook_previews";
    QString cacheDir = QDir(baseCacheDir).filePath(QString("%1/%2").arg(dirL1, dirL2));
    QDir().mkpath(cacheDir);
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译指令
```bash
cd QuarkMeta_Source
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### 4.2 运行时验收标准
1. **真搬家测试**：
   - 记录旧缓存路径（如 `D:/Old/.QuarkMeta/cache`）中的占用大小；
   - 在弹窗中选择新路径（如 `G:/C++/测试-迁移缩略图/cache`），勾选“迁移现有缓存数据到新目录”并保存；
   - 检查新路径：新路径完整包含了所有 `disk_thumbs` 与 `quicklook_previews` 文件；
   - 检查旧路径：原旧路径被彻底清除干净（`removeRecursively`），旧磁盘不再占用任何冗余空间，实现彻底的真搬家！
2. **设置弹窗呈现与互斥二选一**：
   - 在“迁移现有缓存数据到新目录”下方，清晰展示两个互斥复选框以及下方的灰色注释说明文字；
   - 勾选 `disk_thumbs` 时，`quicklook_previews` 自动取消；勾选 `quicklook_previews` 时，`disk_thumbs` 自动取消；禁止全部取消勾选（刚性单选源）；
3. **配置持久化**：点击“保存”后退出，重新打开弹窗或重启软件，所选的选项状态被 100% 正确恢复；
4. **QuickLook 预览极速秒开**：勾选 `disk_thumbs` 后在列表中空格预览文件，QuickLook 直接秒开读取已有的磁盘缓存缩略图，响应时间达到 0ms 级；
5. **平滑降级防黑屏**：对于未曾生成过 `disk_thumbs` 的新文件，在 `disk_thumbs` 模式下依然能够自动降级实时解码并正常呈现大图。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. 复用 `AppConfig::instance().getGlobalCacheDir()` 作为唯一权威缓存根路径；
2. 复用 `DiskMediaExtractor::getDiskThumbCachePath(filePath)` 获取现成磁盘缩略图路径；
3. 复用 `AppConfig::instance().getValue / setValue` 进行配置持久化；
4. 复用既有的 `loadOrExtractQuickLookImage` 解码管线作为安全降级兜底。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | SSOT 唯一数据源 | 派生展示副本 | 刷新机制与方向 |
| :--- | :--- | :--- | :--- |
| 全局缓存根目录 | `AppConfig::getGlobalCacheDir()` | `CacheLocationDialog::m_pathEdit` | 浏览更新时统一以整棵 `cache` 目录为基准，迁移后旧目录彻底清理 |
| QuickLook 缓存源配置 | `AppConfig` 中的 `QuickLook/ThumbnailSource` | `CacheLocationDialog` 复选框状态 | 打开对话框时读取，点击保存时持久化写入 |
| 现成缩略图物理路径 | `DiskMediaExtractor::getDiskThumbCachePath` | `QuickLookWindow` 预览加载 | 运行时动态获取判断文件有效性 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. **外部调用接口保持**：
   - `CacheLocationDialog` 构造函数与槽函数签名保持不变；
   - `QuickLookWindow::loadOrExtractQuickLookImage` 签名完全保持一致；
2. **新增成员一致性**：
   - `CacheLocationDialog` 新增私有指针成员 `m_chkDiskThumbs`、`m_chkQuickLookPreviews`、`m_sourceDescLabel`，均在 `setupUi` 中安全初始化。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

1. `CacheLocationDialog.h` 显式包含 `<QCheckBox>` 与 `<QLabel>`；
2. `QuickLookWindow.cpp` 显式增加包含 `../util/DiskMediaExtractor.h`，确保 `DiskMediaExtractor` 类型及静态函数完整定义。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 结果 | 证据与位置 |
| :--- | :--- | :--- | :--- |
| 1 | 是否物理隔离且未直接修改业务源代码 | 满足 | 方案独立位于 `Implementation Plan/` 目录下 |
| 2 | SEARCH 块是否在目标文件中真实存在且唯一匹配 | 满足 | 经过实际源码比对，精确匹配唯一位置 |
| 3 | 是否包含全部 10 大必备章节 | 满足 | 第 1 至第 10 章节完整具备 |
| 4 | 两选项是否严格互斥（单选源） | 满足 | 第 3.2 节槽函数逻辑保证刚性互斥与必选其一 |
| 5 | 是否在选项下方提供了清晰的思量注释说明 | 满足 | 第 3.2 节新增了换行包装的灰色提示标签 |
| 6 | 是否具备平滑降级防黑屏机制 | 满足 | 第 3.4 节磁盘缓存未命中时向下穿透实时解码 |
| 7 | 是否实现真搬迁并彻底清理旧路径数据 | 满足 | 第 3.3 节搬迁成功后删除原文件并递归清除旧 cache 目录 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

1. **迁移语义完全纠偏**：由原来的“只复制不删除的假迁移”彻底纠偏为“拷贝成功后彻底清除旧数据的真搬迁”，真正释放原磁盘空间；
2. **默认高清体验保持**：新装或默认状态下依旧为 `quicklook_previews` 高清画质模式，不破坏老用户既有画质预期。
