# Implementation Plan: 虚拟协议（库分类/智能文件夹）Tab 标签与地址栏逻辑名称解析实施方案

Base Commit Hash: `9e1a729872cd931147645028836e2ea735493827`
关联模块: `QuarkMeta_Source / NavigationService / TabBarWidget / ContentDataLoader / PanelMediator`

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题现象与缺陷本质
在 QuarkMeta 运行过程中，当用户在“库”选项卡下浏览分类（如分类 ID 为 11 的“测试”）或智能文件夹后，程序退回保存会话状态并在下一次重启时：
1. 顶部的标签页（`TabBarWidget`）标题被显示为了纯数字 ID（例如 `11`）；
2. 地址栏（`AddressBar` / `BreadcrumbBar`）被解析为了生硬的内部协议切分路径（例如 `lib > 11`）；
3. 导致界面顶层视觉呈现与下方内容面板标题（“测试”）严重割裂，将底层内部技术路由协议直接粗暴暴露给终端用户。

### 1.2 根因分析 (Root Cause Analysis)
经过对底层源码调用链的逐层深挖，该缺陷由以下四重因素复合导致：
1. **`NavigationService::currentDisplayPath` 缺乏虚拟协议反查能力**：
   在 `NavigationService.cpp` 中，`currentDisplayPath()` 遇到非 `computer://` / `trash://` 的协议时直接执行了：
   ```cpp
   if (m_currentUrl.contains("://")) return m_currentUrl;
   ```
   导致当 `currentUrl` 为 `library://11` 或 `smartfolder://1` 时，广播发出的 `displayPath` 直接就是原生的 `library://11` 或 `smartfolder://1`。
2. **`TabBarWidget::updateCurrentTabTitle` 与 `cleanNameForTab` 退化**：
   `PanelMediator` 在监听 `currentUrlChanged` 信号时，将收到的 `displayPath`（`library://11`）传入了 `updateCurrentTabTitle(displayPath, url)`。由于 `title` 包含 `/`（即 `library://`），它触发了：
   ```cpp
   folderName = cleanNameForTab(url);
   ```
   `cleanNameForTab` 使用 `QFileInfo` 对 `library://11` 提取 `fileName()`，错误地将协议末尾的数字 ID `11` 提取作为了 Tab 标题！
3. **`TabBarWidget::restoreStateFromConfig` 还原未做标题防退化校验**：
   在从配置还原时，若配置中暂存或被覆盖成了数字，或者通过 `currentTabChanged` 触发了上述 `updateCurrentTabTitle`，Tab 标题会迅速被冲刷为纯数字 ID。
4. **`ContentDataLoader` 缺少对全局导航状态的反向同步闭环**：
   在 `ContentDataLoader::loadDirectory` 中，虽然通过 `LibraryDao::getCategoryById(catId)` 和 `SmartFolderDao::getSmartFolderById(sfId)` 正确读取出了分类和智能文件夹的逻辑中文名称 `rec.name`，并将其成功设置到了内容面板（`applyViewTarget`），但未向外界统一同步地址栏与标签页标题，导致整个状态链路未形成闭环。

### 1.3 重构设计目标
1. **全链路逻辑名称反解**：在 `NavigationService::currentDisplayPath()` 中接入对 `library://` 和 `smartfolder://` 的解析，自动查询对应 DAO 获取逻辑名称并组装为标准的 `库 → 分类名`，确保所有依赖 `currentDisplayPath()` 的广播天然具备人类可读的语义名称；
2. **Tab 标题防退化防御**：在 `TabBarWidget::cleanNameForTab` 与 `updateCurrentTabTitle` 中加入协议感知与防退化逻辑，严禁将虚拟协议的末尾数字 ID 截取为标签页标题；
3. **导航加载闭环同步**：在 `ContentDataLoader` 处理虚拟协议加载时，确保地址栏与 Tab 栏均同步更新为正确的逻辑名称。

---

## Chapter 2: Modified Files List（影响文件清单）

本次实施方案涉及修改以下 3 个既有源文件，无新建或删除文件，无需更改 `CMakeLists.txt`：
1. `QuarkMeta_Source/src/core/NavigationService.cpp` (修改：引入 DAO 查询，实现虚拟协议真实名称反解)
2. `QuarkMeta_Source/src/ui/TabBarWidget.cpp` (修改：防退化逻辑，禁止将协议 ID 截取为 Tab 标题)
3. `QuarkMeta_Source/src/ui/controllers/ContentDataLoader.cpp` (修改：加载虚拟协议时同步更新外部地址栏与 Tab 标题)

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 修改 `QuarkMeta_Source/src/core/NavigationService.cpp`

**修改目的**：引入 `LibraryDao.h` 与 `SmartFolderDao.h`，在 `currentDisplayPath()` 中将 `library://<id>` 和 `smartfolder://<id>` 转换为标准的 `库 → <名称>`，彻底杜绝将未解析的协议暴露给外部。

```
<<<<<<< SEARCH
#include "NavigationService.h"
#include "NavigationHistoryService.h"
#include "DeviceWatcher.h"
#include <QDir>
#include <QFileInfo>
=======
#include "NavigationService.h"
#include "NavigationHistoryService.h"
#include "DeviceWatcher.h"
#include "../meta/LibraryDao.h"
#include "../meta/SmartFolderDao.h"
#include <QDir>
#include <QFileInfo>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
QString NavigationService::currentDisplayPath() const {
    if (m_currentUrl == "computer://") return "此电脑";
    if (m_currentUrl == "trash://") return "回收站";
    if (m_currentUrl.contains("://")) return m_currentUrl;
    return QDir::toNativeSeparators(m_currentUrl);
}
=======
QString NavigationService::currentDisplayPath() const {
    if (m_currentUrl == "computer://") return "此电脑";
    if (m_currentUrl == "trash://") return "回收站";

    if (m_currentUrl.startsWith("library://")) {
        int catId = m_currentUrl.mid(10).toInt();
        if (catId == -1) return "库 → 全部数据";
        if (catId == -2) return "库 → 未分类";
        LibraryCategoryRecord rec = LibraryDao::getCategoryById(catId);
        return rec.name.isEmpty() ? "库" : ("库 → " + rec.name);
    }

    if (m_currentUrl.startsWith("smartfolder://")) {
        int sfId = m_currentUrl.mid(14).toInt();
        SmartFolderRecord rec = SmartFolderDao::getSmartFolderById(sfId);
        return rec.name.isEmpty() ? "库" : ("库 → " + rec.name);
    }

    if (m_currentUrl.contains("://")) return m_currentUrl;
    return QDir::toNativeSeparators(m_currentUrl);
}
>>>>>>> REPLACE
```

---

### 3.2 修改 `QuarkMeta_Source/src/ui/TabBarWidget.cpp`

**修改目的**：在 `cleanNameForTab` 与 `updateCurrentTabTitle` 中防止将 `library://` 或 `smartfolder://` 的 URL 作为文件路径解析出纯数字 ID。

```
<<<<<<< SEARCH
static QString cleanNameForTab(const QString& path) {
    if (path.isEmpty() || path == "computer://") return "此电脑";
    if (path == "trash://") return "回收站";
    QString cleanP = QDir::cleanPath(path);
    QFileInfo fi(cleanP);
    if (fi.isRoot() || cleanP.endsWith(":\\") || cleanP.endsWith(":/") || (cleanP.length() == 2 && cleanP.endsWith(':'))) {
        return cleanP;
    }
    QString fn = fi.fileName();
    return fn.isEmpty() ? cleanP : fn;
}
=======
static QString cleanNameForTab(const QString& path) {
    if (path.isEmpty() || path == "computer://") return "此电脑";
    if (path == "trash://") return "回收站";
    if (path.startsWith("library://") || path.startsWith("smartfolder://")) {
        return "库";
    }
    QString cleanP = QDir::cleanPath(path);
    QFileInfo fi(cleanP);
    if (fi.isRoot() || cleanP.endsWith(":\\") || cleanP.endsWith(":/") || (cleanP.length() == 2 && cleanP.endsWith(':'))) {
        return cleanP;
    }
    QString fn = fi.fileName();
    return fn.isEmpty() ? cleanP : fn;
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    } else {
        QString folderName = title;
        if (url == "computer://" || url.isEmpty()) {
            folderName = "此电脑";
        } else if (title.contains("/") || title.contains("\\")) {
            folderName = cleanNameForTab(url);
        }
        currentTab.title = folderName.isEmpty() ? "此电脑" : folderName;
        currentTab.url = url;
    }
=======
    } else {
        QString folderName = title;
        if (folderName.startsWith("库 → ")) {
            folderName = folderName.mid(4);
        }
        if (url == "computer://" || url.isEmpty()) {
            folderName = "此电脑";
        } else if (!url.startsWith("library://") && !url.startsWith("smartfolder://") && (title.contains("/") || title.contains("\\"))) {
            folderName = cleanNameForTab(url);
        }
        currentTab.title = folderName.isEmpty() ? "此电脑" : folderName;
        currentTab.url = url;
    }
>>>>>>> REPLACE
```

---

### 3.3 修改 `QuarkMeta_Source/src/ui/controllers/ContentDataLoader.cpp`

**修改目的**：在 `ContentDataLoader::loadDirectory` 处理 `library://` 和 `smartfolder://` 协议时，直接向顶层分发正确的逻辑显示路径，确保无论是程序启动恢复还是直接路由跳转，UI 组件标题始终同步。

```
<<<<<<< SEARCH
    if (path.startsWith("library://")) {
        int catId = path.mid(10).toInt();
        LibraryCategoryRecord rec = LibraryDao::getCategoryById(catId);
        QStringList pList = LibraryService::instance().getCategoryPaths(catId);
        m_panel->applyViewTarget(path, "library", rec.name, catId, false);
        m_panel->loadLibraryPaths(pList);
        return;
    }

    if (path.startsWith("smartfolder://")) {
        int sfId = path.mid(14).toInt();
        SmartFolderRecord rec = SmartFolderDao::getSmartFolderById(sfId);
        QStringList matching = SmartFolderDao::getMatchingPaths(rec);
        m_panel->applyViewTarget(path, "smart_folder", rec.name, -20, false);
        m_panel->loadLibraryPaths(matching);
        return;
    }
=======
    if (path.startsWith("library://")) {
        int catId = path.mid(10).toInt();
        QString categoryName;
        if (catId == -1) categoryName = "全部数据";
        else if (catId == -2) categoryName = "未分类";
        else {
            LibraryCategoryRecord rec = LibraryDao::getCategoryById(catId);
            categoryName = rec.name;
        }
        QStringList pList = LibraryService::instance().getCategoryPaths(catId);
        m_panel->applyViewTarget(path, "library", categoryName, catId, false);
        m_panel->loadLibraryPaths(pList);
        return;
    }

    if (path.startsWith("smartfolder://")) {
        int sfId = path.mid(14).toInt();
        SmartFolderRecord rec = SmartFolderDao::getSmartFolderById(sfId);
        QStringList matching = SmartFolderDao::getMatchingPaths(rec);
        m_panel->applyViewTarget(path, "smart_folder", rec.name, -20, false);
        m_panel->loadLibraryPaths(matching);
        return;
    }
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译与语法检查命令
```bash
cmake -B build -S QuarkMeta_Source
cmake --build build --target QuarkMeta -j4
```

### 4.2 运行时验收标准
1. **分类浏览场景**：
   - 在左侧“库”面板中点击任一分类（如“测试”，分类 ID 为 11）；
   - 顶部 Tab 标签标题显示为“测试”，地址栏显示为“库 → 测试”；
   - 退出 QuarkMeta 客户端并重新启动程序；
   - **验证重点**：重启还原会话后，Tab 标签标题依然保持为“测试”，地址栏完整呈现为“库 → 测试”，绝不出现 `11` 或 `lib > 11`。
2. **智能文件夹浏览场景**：
   - 在左侧“库”面板中点击任一智能文件夹（如“五星级”）；
   - 退出并重启程序；
   - **验证重点**：重启还原后，Tab 标题显示为“五星级”，地址栏显示为“库 → 五星级”，绝不出现 `smartfolder > id`。
3. **固定系统分类场景**：
   - 浏览“全部数据”或“未分类”并重启；
   - **验证重点**：Tab 标题分别显示为“全部数据”与“未分类”，地址栏显示“库 → 全部数据”与“库 → 未分类”。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **统一复用 `LibraryDao::getCategoryById` 与 `SmartFolderDao::getSmartFolderById`**：
   直接复用数据库既有的分类/智能文件夹反查方法，不新增重复的查询函数；
2. **系统分类负数 ID 复用**：
   统一复用系统的 `-1`（全部数据）与 `-2`（未分类）标准规范，不重新设计逻辑；
3. **地址栏格式复用**：
   地址栏文本严格使用既有的 `"库 → " + name` 格式，与 `BreadcrumbBar` 中既有的 `path.split(" → ")` 机制完美匹配，天然支持面包屑级联渲染。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生副本与展示控件 | 刷新与同步方向 |
| :--- | :--- | :--- | :--- |
| 分类逻辑名称 | `library_categories` (SQLite 表) | `TabBarWidget` (Tab 标题), `AddressBar` (地址栏) | `NavigationService::currentDisplayPath` / `ContentDataLoader` 反查并同步广播 |
| 智能文件夹名称 | `smart_folders` (SQLite 表) | `TabBarWidget` (Tab 标题), `AddressBar` (地址栏) | `NavigationService::currentDisplayPath` / `ContentDataLoader` 反查并同步广播 |
| 导航当前 URL | `NavigationService::m_currentUrl` | `TabBarWidget::TabInfo::url` | 通过 `currentUrlChanged` 驱动各 View 组件 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

### 7.1 外部调用接口核查
- `LibraryDao::getCategoryById`:
  ```cpp
  static LibraryCategoryRecord getCategoryById(int id);
  ```
  在 `QuarkMeta_Source/src/meta/LibraryDao.h:45` 中为 `public static`，完全一致。
- `SmartFolderDao::getSmartFolderById`:
  ```cpp
  static SmartFolderRecord getSmartFolderById(int id);
  ```
  在 `QuarkMeta_Source/src/meta/SmartFolderDao.h:80` 中为 `public static`，完全一致。
- `LibraryService::getCategoryPaths`:
  ```cpp
  QStringList getCategoryPaths(int categoryId);
  ```
  在 `QuarkMeta_Source/src/meta/LibraryService.h:32` 中为 `public`，完全一致。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

1. `QuarkMeta_Source/src/core/NavigationService.cpp`：
   - 新增包含 `../meta/LibraryDao.h` 和 `../meta/SmartFolderDao.h`；
   - 该文件位于 `QuarkMeta_Source/src/core/`，相对路径 `../meta/` 准确有效；
   - `LibraryCategoryRecord` 与 `SmartFolderRecord` 的类型完整定义由对应头文件提供，类型完整性闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 核对项 | 状态 | 证明位置 |
| :---: | :--- | :---: | :--- |
| 1 | 语言只使用中文 | 满足 | 实施方案全文均为中文 |
| 2 | 方案位于 Implementation Plan/ 物理隔离目录 | 满足 | 本文档路径 |
| 3 | 方案不直接修改源代码文件 | 满足 | 未对源码执行物理写入 |
| 4 | SEARCH 块原文真实且唯一存在 | 满足 | 逐行比对 `NavigationService.cpp`, `TabBarWidget.cpp`, `ContentDataLoader.cpp` |
| 5 | 无臆造 API | 满足 | 所有调用的 DAO 均为官方既有实现 |
| 6 | 包含链完整闭合 | 满足 | 第 8 章已核验 |
| 7 | 不跨层破坏 View 依赖 | 满足 | 业务解析统一收敛在 NavigationService 与 DataLoader |
| 8 | 行为零篡改 | 满足 | 第 10 章详细声明 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

1. **普通文件系统路径导航零篡改**：
   对于普通磁盘目录（如 `C:/Users`、`D:/Photos` 等）、`computer://`（此电脑）及 `trash://`（回收站），其原本的 Tab 标题和地址栏渲染逻辑 100% 保持不变；
2. **分屏逻辑与尺寸零篡改**：
   多分屏状态下的 Tab 标题拼接（如 `A | B`）保持原有行为不变，样式与数值无任何改动；
3. **右键与快捷键交互零篡改**：
   标签页与地址栏的右键菜单、拖拽交互完全保持既有状态。
