# Implementation Plan - QuarkMetaJson-2.md

## Overview
本实施方案旨在彻底解决文件夹内无任何有效项目元数据且文件夹自身亦无自定义试图偏好时，磁盘依然残留 `.QuarkMeta.json` 隐藏配置文件的问题：

1. **零残留物理自愈机制 (Zero-Residual Auto Cleanup)**：在 `QuarkMetaJson::save()` 执行保存落盘前，检查当前配置是否全空。即：
   - `m_items` 经过过滤后无任何具备用户操作的有效条目（`m_items.empty()`）；
   - `m_folder` 处于默认状态（`m_folder.isDefault()`，即未设置自定义排序、星级、颜色标记、备注、加密及标签）。
2. **物理擦除动作**：满足上述全空条件时，直接物理删除磁盘上的 `.QuarkMeta.json` 文件（若存在），并抹除临时文件 `.tmp`，避免在空文件夹中留下无益的磁盘配置文件。

---

## Modified Files List
1. `src/meta/QuarkMetaJson.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/meta/QuarkMetaJson.cpp`
在 `QuarkMetaJson::save()` 函数中引入全空物理自愈清理逻辑：

```
<<<<<<< SEARCH
bool QuarkMetaJson::save() const {
    QJsonObject root;
    root.insert("version", "2");
    root.insert("folder", folderToEntry(m_folder));

    QJsonObject itemsObj;
    for (const auto& [name, meta] : m_items) {
        if (meta.hasUserOperations()) {
            itemsObj.insert(toQString(name), itemToEntry(meta));
        }
    }
    root.insert("items", itemsObj);

    QByteArray jsonData = QJsonDocument(root).toJson(QJsonDocument::Indented);
    QString tmpPath = toQString(m_filePath) + ".tmp";

    QFile tmpFile(tmpPath);
    if (!tmpFile.open(QIODevice::WriteOnly)) return false;
    tmpFile.write(jsonData);
    tmpFile.close();

    if (!MoveFileExW(tmpPath.toStdWString().c_str(), m_filePath.c_str(), MOVEFILE_REPLACE_EXISTING)) {
        QFile::remove(tmpPath);
        return false;
    }
    // 赋予 Windows 隐藏文件属性
    SetFileAttributesW(m_filePath.c_str(), FILE_ATTRIBUTE_HIDDEN);
    return true;
}
=======
bool QuarkMetaJson::save() const {
    QJsonObject itemsObj;
    for (const auto& [name, meta] : m_items) {
        if (meta.hasUserOperations()) {
            itemsObj.insert(toQString(name), itemToEntry(meta));
        }
    }

    // 物理零残留自愈逻辑：若子项目全空且文件夹属性为默认，直接物理清理磁盘文件
    if (itemsObj.isEmpty() && m_folder.isDefault()) {
        QString realPath = toQString(m_filePath);
        if (QFile::exists(realPath)) {
            QFile::remove(realPath);
        }
        return true;
    }

    QJsonObject root;
    root.insert("version", "2");
    root.insert("folder", folderToEntry(m_folder));
    root.insert("items", itemsObj);

    QByteArray jsonData = QJsonDocument(root).toJson(QJsonDocument::Indented);
    QString tmpPath = toQString(m_filePath) + ".tmp";

    QFile tmpFile(tmpPath);
    if (!tmpFile.open(QIODevice::WriteOnly)) return false;
    tmpFile.write(jsonData);
    tmpFile.close();

    if (!MoveFileExW(tmpPath.toStdWString().c_str(), m_filePath.c_str(), MOVEFILE_REPLACE_EXISTING)) {
        QFile::remove(tmpPath);
        return false;
    }
    // 赋予 Windows 隐藏文件属性
    SetFileAttributesW(m_filePath.c_str(), FILE_ATTRIBUTE_HIDDEN);
    return true;
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **编译确认**：
   运行 CMake 编译，确保无语法错误：
   ```bash
   cmake --build build
   ```

2. **功能复核**：
   - 步骤 1：新建文件夹 A，并在内部放入文件 B，打上 5 星；（产生 `.QuarkMeta.json`）
   - 步骤 2：将文件 B 移入回收站；
   - 步骤 3：查看文件夹 A 内部，验证磁盘上的 `.QuarkMeta.json` 隐藏文件被自动物理删除，实现了 0 字节残留。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- **属性检测复用**：复用 `FolderMeta::isDefault()` 与 `ItemMeta::hasUserOperations()` 标准检测函数；
- **防另起炉灶**：在统一落盘接口 `QuarkMetaJson::save()` 中收敛自动清理逻辑。

---

## Header API Signature Verification

| 调用的类 / 成员函数 | `.h` 文件源头 | 物理精确签名 |
| :--- | :--- | :--- |
| `FolderMeta::isDefault()` | `src/meta/MetadataDefs.h` | `bool isDefault() const;` |
| `ItemMeta::hasUserOperations()` | `src/meta/MetadataDefs.h` | `bool hasUserOperations() const;` |

---

## Header Inclusion Chain & Type Completeness Check

- `src/meta/QuarkMetaJson.cpp`:
  - 依赖 `MetadataDefs.h`（已通过 `QuarkMetaJson.h` 隐式与显式闭环包含）。
