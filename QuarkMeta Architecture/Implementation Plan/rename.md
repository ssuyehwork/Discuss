# Batch Rename Case Preservation and Thumbnail Display Fix Implementation Plan

## Overview
This implementation plan resolves two issues in the Batch Rename functionality:
1. **Case Preservation Failure**: Batch rename allowed users to input mixed or uppercase text (e.g. `DZKJ 电子科技`), and preview showed uppercase. However, upon executing, `MetadataManager::normalizePath` converted std::wstring paths to lowercase (`.toLower()`), ruining the casing in internal state and UI refresh signals.
2. **Missing Thumbnail Previews in Batch Rename Dialog**: `BatchRenameDialog::updatePreview()` called `ShellIconManager::getFileIcon(oldPath, 20)`, which only returns standard file type icons. It did not query generated media thumbnails from `DiskMediaExtractor::getCapsuleThumbnailReadOnly(oldPath)`.

## Modified Files List
- `src/meta/MetadataManager.cpp`
- `src/ui/BatchRenameDialog.cpp`

## Detailed Line-by-Line Changes

### 1. Fix `MetadataManager::normalizePath` to retain original path casing
In `src/meta/MetadataManager.cpp`, update `normalizePath` so that path cleaning and native separator conversion preserve casing instead of forcing `.toLower()`.

```
<<<<<<< SEARCH
std::wstring MetadataManager::normalizePath(const std::wstring& path) {
    if (path.empty()) return L"";
    // 2026-06-xx 物理对账优化：Windows 环境下路径不区分大小写，
    // 统一转换为全小写以确保内存缓存 (std::unordered_map) 的 Key 匹配一致性，彻底消除“幽灵项”。
    QString qp = QDir::toNativeSeparators(QDir::cleanPath(QString::fromStdWString(path))).toLower();
    if (qp.length() == 2 && qp.endsWith(':')) qp += '\\';
    return qp.toStdWString();
}
=======
std::wstring MetadataManager::normalizePath(const std::wstring& path) {
    if (path.empty()) return L"";
    // Clean path and ensure native separators while maintaining original letter casing
    QString qp = QDir::toNativeSeparators(QDir::cleanPath(QString::fromStdWString(path)));
    if (qp.length() == 2 && qp.endsWith(':')) qp += '\\';
    return qp.toStdWString();
}
>>>>>>> REPLACE
```

### 2. Update `BatchRenameDialog::updatePreview` to load thumbnail icons
In `src/ui/BatchRenameDialog.cpp`, attempt to retrieve capsule thumbnail using `DiskMediaExtractor::getCapsuleThumbnailReadOnly(oldPath)` before falling back to `ShellIconManager::getFileIcon(oldPath, 20)`.

```
<<<<<<< SEARCH
        // 1. 左侧原文件名（带微型缩略图/文件关联图标）
        QIcon fileIcon = ShellIconManager::getFileIcon(oldPath, 20);
        auto* itemOld = new QTableWidgetItem(fileIcon, info.fileName());
        itemOld->setForeground(QColor("#B0B0B0"));
        m_table->setItem(i, 0, itemOld);
=======
        // 1. 左侧原文件名（优先显示真实的缩略图，后备使用系统关联图标）
        QIcon fileIcon;
        QImage thumbImg = DiskMediaExtractor::getCapsuleThumbnailReadOnly(oldPath);
        if (!thumbImg.isNull()) {
            fileIcon = QIcon(QPixmap::fromImage(thumbImg));
        } else {
            fileIcon = ShellIconManager::getFileIcon(oldPath, 20);
        }
        auto* itemOld = new QTableWidgetItem(fileIcon, info.fileName());
        itemOld->setForeground(QColor("#B0B0B0"));
        m_table->setItem(i, 0, itemOld);
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Build the project with CMake:
   ```bash
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```
2. Verify Batch Rename Case Preservation:
   - Select files with uppercase letters or rename to uppercase text (e.g. `DZKJ 电子科技_006.eps`).
   - Execute batch rename and confirm that the resulting disk files and UI display keep uppercase `DZKJ`.
3. Verify Thumbnail Display:
   - Open Batch Rename dialog for items with generated thumbnails (e.g. EPS, images).
   - Check that the preview table item icons display actual image thumbnails instead of fallback generic file icons.
