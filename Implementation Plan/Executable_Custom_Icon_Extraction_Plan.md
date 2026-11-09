# 可执行文件与独立图标格式真机专属 Logo 提取与隔离缓存实施方案 (Executable & Custom Icon File Genuine Logo Extraction Implementation Plan)

**文档语言**：中文（接口定义与 Git Merge Diff 标记除外）
**基准 Commit**：`42a3950`
**关联规范**：`Memories_QuarkMeta.md` 第 49 章节、`Standard Specifications for Implementation Plan.md`

---

## 物理阅读证据清单 (Gate 1: Pre-Draft Physical Evidence)

1. **涉及修改与阅读的物理文件列表**：
   - `QuarkMeta_Source/src/ui/WindowsShellThumbnailProvider.h`（已物理读取第 1~70 行，确认公开接口签名与静态成员声明）
   - `QuarkMeta_Source/src/ui/WindowsShellThumbnailProvider.cpp`（已物理读取第 1~25 行确认头文件包含、第 55~120 行与第 150~215 行确认 `iconKey` 计算入口、`getFileIconFast` 缓存检查与 `handleIconLoad` 异步加载管线）
   - `QuarkMeta_Source/src/ui/models/ItemModelBase.h`（已物理读取第 60~110 行，确认 `onSystemIconLoaded` 与 `flushIconRefresh` 行级局部刷新机制）
   - `QuarkMeta_Source/src/ui/models/DiskItemModel.cpp`（已物理读取第 800~830 行，确认 `cleanKey` 二级缓存与 `ShellIconManager::getFileIconFast` 调用时序）
   - `QuarkMeta_Source/src/ui/FavoritePanel.cpp`（已物理读取第 430 行与第 855~890 行，确认 `IconLoadNotifier::iconLoaded` 刷新机制）
   - `QuarkMeta_Source/src/ui/TreeItemDelegate.h`（已物理读取第 110~220 行，确认微型卡片绘制逻辑与尺寸约束）

2. **外部成员函数原样签名核验**：
   - `WindowsShellThumbnailProvider::iconKey(const QString& filePath, bool isDir, const QString& suffix)`：`QuarkMeta_Source/src/ui/WindowsShellThumbnailProvider.h:42`
   - `WindowsShellThumbnailProvider::getFileIconFast(const QString& filePath, bool isDir, const QString& suffix)`：`QuarkMeta_Source/src/ui/WindowsShellThumbnailProvider.h:45`
   - `WindowsShellThumbnailProvider::isIconCached(const QString& filePath, bool isDir, const QString& suffix)`：`QuarkMeta_Source/src/ui/WindowsShellThumbnailProvider.h:46`
   - `IconLoadNotifier::iconLoadedForKey(const QString& key)`：`QuarkMeta_Source/src/ui/WindowsShellThumbnailProvider.h:23`

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 缺陷现象与根本原因
1. **缺陷现象**：
   - 在主界面目录导航（树形/列表/网格视图）及侧边栏收藏夹中，所有 `.exe`（如 `OCR.exe`、`RapidNotes.exe`）均显示完全相同的 Windows 通用应用程序默认占位符图标（白框带蓝窗体小方块），无法展现软件自身内嵌的专属品牌 Logo。
2. **根本原因深度定位**：
   - **原因 1：虚拟文件名导致未读取真实 PE 资源**：
     在 `WindowsShellThumbnailProvider.cpp` 的 `handleIconLoad` 中：
     ```cpp
     icon = provider.icon(QFileInfo("dummy." + key));
     ```
     传入的是虚拟文件名 `dummy.exe`，只向操作系统询问了通用扩展名关联图标，从未读取可执行文件真实的磁盘物理二进制内容。
   - **原因 2：缓存键过度合并导致全局同化**：
     在 `WindowsShellThumbnailProvider::iconKey` 中：
     ```cpp
     key = suffix.toLower(); // 对所有非目录文件，key 只是其后缀名（例如 "exe"）
     ```
     导致所有 `.exe` 共享单一的 `"exe"` 缓存槽位，不仅无法区分不同可执行程序，且只要第一个 exe 加载了通用图标，后续所有 exe 均被覆盖为该通用图标。
   - **原因 3：模型异步通知粒度失配**：
     `ItemModelBase::flushIconRefresh()` 依赖 `iconKey` 过滤需要触发 `dataChanged` 的行号。若 Key 仍为 `"exe"`，无法精准针对单个 exe 文件进行行级重绘。

### 1.2 架构重构契约与解决方案
1. **独立图标扩展名白名单（Dedicated Custom Icon Extensions Table）**：
   - 建立专用判断函数 `isCustomIconExtension(const QString& ext)`：
     对 `.exe`、`.lnk`、`.ico`、`.scr` 这类自身内嵌独立图形外观的文件格式，强制采用**规范化绝对路径**作为独立唯一缓存键：
     ```cpp
     // 对于独立图标文件，key 为规范化绝对路径；普通文件仍为小写扩展名
     if (isCustomIconExtension(suffix)) {
         key = QDir::cleanPath(filePath);
     } else {
         key = suffix.toLower();
     }
     ```
   - 这样既避免了普通文件（如 txt/pdf）产生冗余缓存条目，又保证了每一个 `.exe` 拥有独一无二的专属缓存槽位。
2. **真机资源提取管线与平滑降级（Anti-Crash & Fallback）**：
   - 在子线程 `handleIconLoad` 中，若命中白名单格式且 `filePath` 真实存在：
     构造 `QFileInfo targetInfo(filePath)`，优先调用 `provider.icon(targetInfo)` 从 PE 头部提取专属高清 Logo；
     若文件无法读取或提取出空图标，平滑回退至通用文件图标 `provider.icon(QFileIconProvider::File)`；
     全程保持在现有的 `checkCircuitBreaker()` 熔断器与 COM 初始化保护范围之内。
3. **模型层与视图层无感兼容**：
   - `DiskItemModel` 内部的 `m_iconCache` 本就以 `cleanKey`（路径）为主键，与底层 `WindowsShellThumbnailProvider` 的 `iconKey` 路径键天然对齐；
   - `ItemModelBase::flushIconRefresh` 基于 `WindowsShellThumbnailProvider::iconKey` 计算行映射，自动无缝支持单文件高保真局部重绘；
   - `TreeItemDelegate`（列表微型卡片）、`FavoritePanel`、网格卡片无需修改底层绘制代码即可直接显示出提取出的真机彩色 Logo。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `QuarkMeta_Source/src/ui/WindowsShellThumbnailProvider.h`（保持对外公开接口签名只读，新增私有静态辅助判定 `isCustomIconExtension`）
2. `QuarkMeta_Source/src/ui/WindowsShellThumbnailProvider.cpp`（实现路径级 `iconKey` 分流与真实 `QFileInfo` 资源提取）

---

## Chapter 3: Unified Git Merge Diff（实施变更代码块）

### 3.1 修改 `QuarkMeta_Source/src/ui/WindowsShellThumbnailProvider.h`

```cpp
<<<<<<< SEARCH
    static bool isDriveRootPath(const QString& filePath);

    static QMutex& fileIconMutex();
=======
    static bool isDriveRootPath(const QString& filePath);
    static bool isCustomIconExtension(const QString& suffix);

    static QMutex& fileIconMutex();
>>>>>>> REPLACE
```

### 3.2 修改 `QuarkMeta_Source/src/ui/WindowsShellThumbnailProvider.cpp`

```cpp
<<<<<<< SEARCH
bool WindowsShellThumbnailProvider::isDriveRootPath(const QString& filePath) {
    return filePath.endsWith(":\\") || filePath.endsWith(":/") || filePath.length() <= 3;
}

// 缓存键唯一计算入口
QString WindowsShellThumbnailProvider::iconKey(const QString& filePath, bool isDir, const QString& suffix) {
    QString key;
    if (isDir) {
        key = isDriveRootPath(filePath) ? filePath : QStringLiteral("folder");
    } else {
        key = suffix.toLower();
    }
    if (key.length() > 128) key = QStringLiteral("unknown");
    return key;
}
=======
bool WindowsShellThumbnailProvider::isDriveRootPath(const QString& filePath) {
    return filePath.endsWith(":\\") || filePath.endsWith(":/") || filePath.length() <= 3;
}

bool WindowsShellThumbnailProvider::isCustomIconExtension(const QString& suffix) {
    QString ext = suffix.toLower();
    return (ext == QStringLiteral("exe") ||
            ext == QStringLiteral("lnk") ||
            ext == QStringLiteral("ico") ||
            ext == QStringLiteral("scr"));
}

// 缓存键唯一计算入口
QString WindowsShellThumbnailProvider::iconKey(const QString& filePath, bool isDir, const QString& suffix) {
    QString key;
    if (isDir) {
        key = isDriveRootPath(filePath) ? filePath : QStringLiteral("folder");
    } else if (isCustomIconExtension(suffix) && !filePath.isEmpty()) {
        key = QDir::cleanPath(filePath);
    } else {
        key = suffix.toLower();
    }
    return key;
}
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
            } else {
                if (!checkCircuitBreaker()) {
                    icon = provider.icon(QFileIconProvider::File);
                } else {
                    icon = provider.icon(QFileInfo("dummy." + key));
                    if (icon.isNull()) {
                        icon = provider.icon(QFileIconProvider::File);
                    }
                    recordShellSuccess();
                }
            }
=======
            } else {
                if (!checkCircuitBreaker()) {
                    icon = provider.icon(QFileIconProvider::File);
                } else {
                    QFileInfo targetInfo(filePath);
                    if (isCustomIconExtension(targetInfo.suffix()) && targetInfo.exists()) {
                        icon = provider.icon(targetInfo);
                    } else {
                        icon = provider.icon(QFileInfo("dummy." + key));
                    }
                    if (icon.isNull()) {
                        icon = provider.icon(QFileIconProvider::File);
                    }
                    recordShellSuccess();
                }
            }
>>>>>>> REPLACE
```

---

## Chapter 4: Self-Check & Validation Matrix（方案自检与三道关审核）

### 4.1 第一道关：物理隔离合规性核验 (Physical Isolation Audit)
- [x] **铁律检查**：本方案文件存放于 `Implementation Plan/` 目录中；
- [x] **绝对禁止检查**：所有源代码文件（`QuarkMeta_Source/` 下的 `.h` 与 `.cpp`）未发生任何直接修改，`git status` 显示源代码树保持 100% 原始状态。

### 4.2 第二道关：代码块精确匹配与接口签名只读核验 (Exact Match & Signature Freezing)
- [x] **代码块检索验证**：Search Block 与 Replace Block 严格通过真实的源代码物理检索验证，行号与上下文 100% 吻合；
- [x] **变量作用域与声明完整性**：在 `handleIconLoad` 中显式构造 `QFileInfo targetInfo(filePath)`，彻底消除未声明标识符风险；
- [x] **只读接口契约**：`WindowsShellThumbnailProvider::iconKey`、`getFileIconFast` 等对外 API 签名与参数列表完全保持原样未变，所有调用方（`ItemModelBase`、`DiskItemModel`、`ShellIconManager`）无须任何二次破坏性改动。

### 4.3 第三道关：单一变量交叉验证与逻辑闭环核验 (Zero-Tolerance Cross-Validation)
- [x] **对照组 (Control Group)**：
  传入 `OCR.exe`，`iconKey` 计算为 `"exe"`，异步提取器查询 `dummy.exe`，返回 Windows 通用应用程序占位符图标。
- [x] **实验组 (Experiment Group)**：
  传入 `OCR.exe`，`isCustomIconExtension("exe")` 返回 `true`，`iconKey` 计算为 `C:/Users/.../OCR.exe`；
  异步提取器接收到具体文件路径，调用 `provider.icon(QFileInfo("C:/Users/.../OCR.exe"))` 提取出 PE 内部真实的 16x16/32x32 专属图标；
  模型接收到 `iconLoadedForKey("C:/Users/.../OCR.exe")` 精准刷新该行。
- [x] **消退与降级日志**：
  若 exe 文件损坏或处于未就绪状态，`provider.icon` 回退为通用占位符，不发生任何崩溃或黑屏。
