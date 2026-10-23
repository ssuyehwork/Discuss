# Implementation Plan - DiskMediaExtractor

## 1. Overview（概述与解决的问题）
本实施方案旨在统一 `QuickLookWindow` 与 `DiskMediaExtractor` 两个模块的缓存文件存储逻辑，解决此前存在的两个问题：
1. **哈希散列算法不统一**：`QuickLookWindow` 此前采用 MD5 哈希算法，现统一为物理规范路径（小写 + Native Separators）的 **SHA256** 哈希算法。
2. **缺乏文件夹分桶**：两个模块此前均将缓存图片堆积在单层扁平目录下，当缓存文件数量较多时影响文件系统性能。本次改动统一引入基于 SHA256 前 4 位字符的**二级文件夹分桶机制**（`<L1>/<L2>/`，例如 `a1/b2/`）。

---

## 2. Modified Files List（影响文件清单）
- `src/ui/QuickLookWindow.cpp`
- `src/util/DiskMediaExtractor.cpp`

---

## 3. Detailed Line-by-Line Changes（包含 CMakeLists.txt 在内的精准替换块）

### Change 1: `src/ui/QuickLookWindow.cpp`
```cpp
<<<<<<< SEARCH
    // 🚀【物理目录隔离铁律】：QuickLook 144 DPI 大图绝对不与缩略图共用文件夹
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/quicklook_previews";
    QDir().mkpath(cacheDir);

    QFileInfo srcInfo(filePath);
    QString hashKey = QString::fromLatin1(QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Md5).toHex());
    QString cachePath = QString("%1/%2.png").arg(cacheDir, hashKey);
=======
    // 🚀【物理目录隔离铁律】：QuickLook 144 DPI 大图绝对不与缩略图共用文件夹 (使用 SHA256 与两级分桶)
    QByteArray normalized = QDir::toNativeSeparators(filePath).toLower().toUtf8();
    QString hashStr = QString::fromUtf8(QCryptographicHash::hash(normalized, QCryptographicHash::Sha256).toHex());
    QString dirL1 = hashStr.left(2);
    QString dirL2 = hashStr.mid(2, 2);

    QString baseCacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/quicklook_previews";
    QString cacheDir = QDir(baseCacheDir).filePath(QString("%1/%2").arg(dirL1, dirL2));
    QDir().mkpath(cacheDir);

    QFileInfo srcInfo(filePath);
    QString cachePath = QDir(cacheDir).filePath(QString("%1.png").arg(hashStr));
>>>>>>> REPLACE
```

### Change 2: `src/util/DiskMediaExtractor.cpp`
```cpp
<<<<<<< SEARCH
QString DiskMediaExtractor::getDiskThumbCachePath(const QString& filePath) {
    QByteArray normalized = QDir::toNativeSeparators(filePath).toLower().toUtf8();
    QString hashStr = QString::fromUtf8(QCryptographicHash::hash(normalized, QCryptographicHash::Sha256).toHex());
    QString cacheDir = QDir::temp().filePath("QuarkMeta_Thumbnails");
    return QDir(cacheDir).filePath(QString("%1_230.png").arg(hashStr.left(32)));
}
=======
QString DiskMediaExtractor::getDiskThumbCachePath(const QString& filePath) {
    QByteArray normalized = QDir::toNativeSeparators(filePath).toLower().toUtf8();
    QString hashStr = QString::fromUtf8(QCryptographicHash::hash(normalized, QCryptographicHash::Sha256).toHex());
    QString dirL1 = hashStr.left(2);
    QString dirL2 = hashStr.mid(2, 2);

    QString baseCacheDir = QDir::temp().filePath("QuarkMeta_Thumbnails");
    QString cacheDir = QDir(baseCacheDir).filePath(QString("%1/%2").arg(dirL1, dirL2));
    return QDir(cacheDir).filePath(QString("%1_230.png").arg(hashStr.left(32)));
}
>>>>>>> REPLACE
```

> 注：本次修改仅调整既有 C++ 函数体内部实现，不涉及新增 `.h`/`.cpp` 文件，CMakeLists.txt 无需增删条目。

---

## 4. Build & Verification Steps（编译命令与验证方法）
1. **静态代码核验**：
   确认 `QuickLookWindow.cpp` 与 `DiskMediaExtractor.cpp` 中引用的 `QCryptographicHash::Sha256`、`QDir::filePath` 与 `QDir::mkpath` 逻辑完整。
2. **运行/测试验证**：
   执行编译后运行应用，触发 QuickLook 空格预览与常规图像缩略图提取，检查对应临时/缓存文件夹（`CacheLocation/quicklook_previews/xx/yy/` 与 `Temp/QuarkMeta_Thumbnails/xx/yy/`）是否自动建立了 2 级 Hex 桶目录并正确写入 `.png` 缓存。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）
- **官方 API 通道复用**：
  - `DiskMediaExtractor::getDiskThumbCachePath` 保持作为缩略图缓存绝对路径计算的唯一入口（SSOT）。
  - `QuickLookWindow::loadOrExtractQuickLookEps` 继续作为 144 DPI 大图预览独立管道，复用统一的 SHA256 + 2 级分桶模式。
- **防另起炉灶自查**：
  - 未引入任何重复或额外的局部路径拼接逻辑，完全在既有 API 函数体内完成归一化重构。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用的成员函数 / 枚举 | 所在的头文件 | 精准物理签名 | 核查状态 |
| :--- | :--- | :--- | :--- |
| `QCryptographicHash::hash` | `<QCryptographicHash>` | `static QByteArray hash(const QByteArray &key, Algorithm method)` |  物理核实通过 |
| `QCryptographicHash::Sha256` | `<QCryptographicHash>` | `enum Algorithm { ..., Sha256, ... }` |  物理核实通过 |
| `QDir::toNativeSeparators` | `<QDir>` | `static QString toNativeSeparators(const QString &pathName)` |  物理核实通过 |
| `QDir::filePath` | `<QDir>` | `QString filePath(const QString &fileName) const` |  物理核实通过 |
| `QDir::mkpath` | `<QDir>` | `bool mkpath(const QString &dirPath) const` |  物理核实通过 |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

- **`src/ui/QuickLookWindow.cpp`**:
  - 已包含 `#include <QCryptographicHash>`
  - 已包含 `#include <QStandardPaths>`
  - 已包含 `#include <QDir>`
  - 已包含 `#include <QFileInfo>`
  - 类型完整性：`QDir`, `QCryptographicHash`, `QFileInfo` 在 cpp 中具备完整类型头文件引入。
- **`src/util/DiskMediaExtractor.cpp`**:
  - 已包含 `#include <QCryptographicHash>`
  - 已包含 `#include <QDir>`
  - 已包含 `#include <QFileInfo>`
  - 类型完整性：`QDir`, `QCryptographicHash`, `QFileInfo` 在 cpp 中具备完整类型头文件引入。
