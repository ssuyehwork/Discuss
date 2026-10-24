# DeadCodeAndWrapperCleanup.md - 死接口与冗余转接方法物理清理实施方案

Base Commit Hash: `601627e4dab252d6111e4ad3673d8c5737b1dfc1`
Prerequisite Plans: 无
Supersedes: 无

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 背景与根因分析
在 QuarkMeta 架构演进过程中，部分模块遗留了以下三类无功能价值的死接口与冗余包装转接层：
1. **`UiHelper::isGraphicsFile`**：纯内联转接调用 `ColorPaletteEngine::isGraphicsFile` 的历史遗留接口，违背了图形格式判断的单一定义真理源 (SSOT) 原则。
2. **`MetadataManager::activateItem` 与 `syncPhysicalMetadata`**：无额外逻辑的空壳包装器，且在整个项目（`src/`）中外部调用点数量为 0。
3. **`MetaPanel::setNote(const std::wstring&)` / `setURL(const std::wstring&)` / `setColor(const std::wstring&, bool)`**：UI 层针对 `std::wstring` 的死重载接口，UI 协调层（如 `PanelMediator`）已全面使用 `QString` 版本的接口，其外部调用点数量为 0。

### 1.2 重构目标
1. 统一图形文件格式校验 SSOT：将所有调用 `UiHelper::isGraphicsFile` 的位置直接重定向至 `ColorPaletteEngine::isGraphicsFile`，并物理删除 `UiHelper::isGraphicsFile` 定义。
2. 物理下线 `MetadataManager` 中的 0 调用空壳方法：`activateItem` 与 `syncPhysicalMetadata`。
3. 物理下线 `MetaPanel` 中 0 调用的 `std::wstring` 重载接口，使 UI 控件的公开 API 纯粹聚焦于 Qt 原生类型 `QString`。

---

## Chapter 2: Modified Files List（影响文件清单）

### 2.1 修改文件 (Modify File)
- `src/ui/UiHelper.h`
- `src/ui/FavoritePanel.cpp`
- `src/ui/QuickLookWindow.cpp`
- `src/ui/ThumbnailDelegate.cpp`
- `src/ui/models/DiskItemModel.cpp`
- `src/meta/MetadataManager.h`
- `src/meta/MetadataManager.cpp`
- `src/ui/MetaPanel.h`
- `src/ui/MetaPanel.cpp`

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 修改 `src/ui/UiHelper.h`（清理 `isGraphicsFile` 并重定向 `canPreviewFile`）

<<<<<<< SEARCH
    static inline bool isGraphicsFile(const QString& ext) {
        return ColorPaletteEngine::isGraphicsFile(ext);
    }


    static inline bool isStandardImage(const QString& ext) {
=======
    static inline bool isStandardImage(const QString& ext) {
>>>>>>> REPLACE

<<<<<<< SEARCH
        QString ext = fi.suffix().toLower();
        if (isGraphicsFile(ext) || isTextFile(ext) || ext == "pdf") {
            return true;
        }
=======
        QString ext = fi.suffix().toLower();
        if (ColorPaletteEngine::isGraphicsFile(ext) || isTextFile(ext) || ext == "pdf") {
            return true;
        }
>>>>>>> REPLACE

### 3.2 修改 `src/ui/FavoritePanel.cpp`（重定向至 `ColorPaletteEngine::isGraphicsFile`）

<<<<<<< SEARCH
            QString ext = QFileInfo(nativePath).suffix().toLower();
            if (UiHelper::isGraphicsFile(ext) || ext == "psd" || ext == "ai" || ext == "eps" || ext == "pdf" || ext == "svg") {
                pathsToExtract << nativePath;
            }
=======
            QString ext = QFileInfo(nativePath).suffix().toLower();
            if (ColorPaletteEngine::isGraphicsFile(ext) || ext == "psd" || ext == "ai" || ext == "eps" || ext == "pdf" || ext == "svg") {
                pathsToExtract << nativePath;
            }
>>>>>>> REPLACE

### 3.3 修改 `src/ui/QuickLookWindow.cpp`（重定向至 `ColorPaletteEngine::isGraphicsFile`）

<<<<<<< SEARCH
    QString ext = fi.suffix().toLower();

    if (UiHelper::isGraphicsFile(ext)) {
        renderImage(filePath);
    } else {
=======
    QString ext = fi.suffix().toLower();

    if (ColorPaletteEngine::isGraphicsFile(ext)) {
        renderImage(filePath);
    } else {
>>>>>>> REPLACE

### 3.4 修改 `src/ui/ThumbnailDelegate.cpp`（重定向至 `ColorPaletteEngine::isGraphicsFile`）

<<<<<<< SEARCH
        QString path = index.data(m_pathRole).toString();
        QString ext = QFileInfo(path).suffix().toLower();
        if (UiHelper::isGraphicsFile(ext) || ext == "svg") isWaitingThumb = true;
=======
        QString path = index.data(m_pathRole).toString();
        QString ext = QFileInfo(path).suffix().toLower();
        if (ColorPaletteEngine::isGraphicsFile(ext) || ext == "svg") isWaitingThumb = true;
>>>>>>> REPLACE

### 3.5 修改 `src/ui/models/DiskItemModel.cpp`（重定向至 `ColorPaletteEngine::isGraphicsFile`）

<<<<<<< SEARCH
        const auto& rec = m_allRecords[i];
        if (!rec.isDir && rec.width == 0 && UiHelper::isGraphicsFile(rec.suffix)) {
            if (!filterSet.isEmpty() && !filterSet.contains(QDir::cleanPath(rec.path).toLower())) {
=======
        const auto& rec = m_allRecords[i];
        if (!rec.isDir && rec.width == 0 && ColorPaletteEngine::isGraphicsFile(rec.suffix.toLower())) {
            if (!filterSet.isEmpty() && !filterSet.contains(QDir::cleanPath(rec.path).toLower())) {
>>>>>>> REPLACE

<<<<<<< SEARCH
        QString ext = rec.suffix.toLower();
        bool isGraphic = UiHelper::isGraphicsFile(ext);
        if (rec.isDir || !isGraphic) {
=======
        QString ext = rec.suffix.toLower();
        bool isGraphic = ColorPaletteEngine::isGraphicsFile(ext);
        if (rec.isDir || !isGraphic) {
>>>>>>> REPLACE

<<<<<<< SEARCH
        QString ext = record.suffix.toLower();
        bool isGraphic = UiHelper::isGraphicsFile(ext);

        if (isGraphic) return QIcon();
=======
        QString ext = record.suffix.toLower();
        bool isGraphic = ColorPaletteEngine::isGraphicsFile(ext);

        if (isGraphic) return QIcon();
>>>>>>> REPLACE

### 3.6 修改 `src/meta/MetadataManager.h`（移除空壳方法声明）

<<<<<<< SEARCH
    void syncPhysicalMetadata(const std::wstring& path, bool notify = true);

    static void activateItem(const std::wstring& path);
=======
>>>>>>> REPLACE

### 3.7 修改 `src/meta/MetadataManager.cpp`（移除空壳方法定义）

<<<<<<< SEARCH
void MetadataManager::syncPhysicalMetadata(const std::wstring& path, bool notify) { persistAsync(path, notify); }

void MetadataManager::activateItem(const std::wstring& path) {
    instance().registerItem(path);
}
=======
>>>>>>> REPLACE

### 3.8 修改 `src/ui/MetaPanel.h`（移除 `std::wstring` 死重载声明）

<<<<<<< SEARCH
    void setNote(const QString& note);
    void setNote(const std::wstring& note);
    void setURL(const QString& url);
    void setURL(const std::wstring& url);
    void setRating(int rating, bool fromUser = false);
    void setColor(const QString& hexColor, bool fromUser = false);
    void setColor(const std::wstring& color, bool fromUser = false);
=======
    void setNote(const QString& note);
    void setURL(const QString& url);
    void setRating(int rating, bool fromUser = false);
    void setColor(const QString& hexColor, bool fromUser = false);
>>>>>>> REPLACE

### 3.9 修改 `src/ui/MetaPanel.cpp`（移除 `std::wstring` 死重载定义）

<<<<<<< SEARCH
void MetaPanel::setColor(const std::wstring& color, bool fromUser) {
    setColor(QString::fromStdWString(color), fromUser);
}
=======
>>>>>>> REPLACE

<<<<<<< SEARCH
void MetaPanel::setNote(const std::wstring& note) {
    setNote(QString::fromStdWString(note));
}
=======
>>>>>>> REPLACE

<<<<<<< SEARCH
void MetaPanel::setURL(const std::wstring& url) {
    setURL(QString::fromStdWString(url));
}
=======
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译指令
在项目根目录运行以下构建命令：
```bash
cmake -B build -S .
cmake --build build --config Debug
```

### 4.2 运行时验收标准
1. **纯净编译**：全项目无缺失 `UiHelper::isGraphicsFile`、`activateItem`、`syncPhysicalMetadata` 或 `MetaPanel::setNote/setURL/setColor(std::wstring)` 的链接或编译错误。
2. **图形文件卡片与预览**：Grid/Adaptive View 下的图片文件依旧能正常标注与加载缩略图；QuickLook 窗口下双击/空格预览图片能够正常绘制图像。
3. **属性面板更新**：选择文件时，`MetaPanel` 中的颜色、备注与链接依然能正常刷新显示与编辑。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- 图形格式校验统一归口使用唯一的 SSOT 入口 `ColorPaletteEngine::isGraphicsFile`。
- 彻底删除了 0 调用的包装函数与冗余重载，消除了“两套接口/转接包装”的隐患。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| **图形后缀判断集合** | `ColorPaletteEngine::isGraphicsFile` | 无 | `ColorPaletteEngine::isGraphicsFile(ext)` 唯一判定 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

| 类与接口 | 访问级别 | 声明位置 | 定义位置 | 调用点状态 |
| :--- | :--- | :--- | :--- | :--- |
| `ColorPaletteEngine::isGraphicsFile(const QString& ext)` | `public static` | `src/util/ColorPaletteEngine.h` | `src/util/ColorPaletteEngine.cpp` | `DiskItemModel`, `ThumbnailDelegate`, `QuickLookWindow`, `FavoritePanel`, `UiHelper` |
| `MetaPanel::setColor(const QString&, bool)` | `public` | `src/ui/MetaPanel.h` | `src/ui/MetaPanel.cpp` | `PanelMediator` |
| `MetaPanel::setNote(const QString&)` | `public` | `src/ui/MetaPanel.h` | `src/ui/MetaPanel.cpp` | `PanelMediator` |
| `MetaPanel::setURL(const QString&)` | `public` | `src/ui/MetaPanel.h` | `src/ui/MetaPanel.cpp` | `PanelMediator` |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- 所有涉及调用 `ColorPaletteEngine::isGraphicsFile` 的源文件均通过预置包含或关联头文件闭合引入了 `"ColorPaletteEngine.h"` 或 `"UiHelper.h"`。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 结论 | 备注 |
| :--- | :--- | :--- | :--- |
| 1 | 契约锁（不随意破坏 public 签名） | 满足 | 物理清理 0 调用的冗余死重载 |
| 2 | 依赖锁（单向依赖 Shell->View->Controller） | 满足 | 不改变已有依赖层次 |
| 3 | 静态红线（无 Win32 抢焦点/全局 hook） | 满足 | 纯标准 C++/Qt 逻辑重构 |
| 4 | 黑盒隔离锁（对接口/DTO 负责） | 满足 | 仅收敛公开接口定义 |
| 5 | CMake 注册同步 | 满足 | 未新增/删除文件，无需 CMake 修改 |
| 6 | 物理死代码彻底删除 | 满足 | 彻底物理删除废弃重载与空壳方法 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 属性/行为 | 既有值/行为 | 重构后值/行为 | 一致性确认 |
| :--- | :--- | :--- | :--- |
| **图形后缀判断格式** | `ColorPaletteEngine::isGraphicsFile` 内部规则 | `ColorPaletteEngine::isGraphicsFile` 内部规则 | 100% 一致 |
| **MetaPanel 属性更新** | `QString` 接收与更新 UI | `QString` 接收与更新 UI | 100% 一致 |
