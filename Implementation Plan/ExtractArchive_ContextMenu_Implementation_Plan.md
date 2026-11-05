# 内容面板右键菜单 — "解压压缩包"功能实施方案

**方案版本**：v1.0  
**日期**：2026-11-05  
**关联 Memories.md 条目**：`[新增记录] 内容面板右键菜单 — "解压压缩包"功能设计理念`

---

## 已读物理证据清单（第 1 关：Gate 1）

| 文件 | 确认已读 | 关键物理签名 |
|---|---|---|
| `src/ui/ContentPanel.h` | ✅ | `enum ContextAction { ... ActionOpenDirectoryInExplorer };`（第 63-70 行） |
| `src/ui/controllers/ContentContextMenu.cpp` | ✅ | 场景2非DriveRoot分支 L336-341：`menu.addSeparator()` 后接 `ActionCopy/Cut/Duplicate` |
| `src/ui/controllers/ContentContextMenu.cpp` | ✅ | switch 末尾 L1051-1058：`default: break; } } } // namespace QuarkMeta` |
| `src/util/ShellHelper.h` | ✅ | 最后方法 `static void ensureHidden(const std::wstring& path);`（第 73-74 行） |
| `src/util/ShellHelper.cpp` | ✅ | 文件末尾 `ensureHidden` 实现，使用 `SetFileAttributesW`（第 237-243 行） |
| `src/ui/SvgIcons.h` | ✅ | 已存在 `"zip"` 专属拉链压缩包图标键（第 2472 行），语义精准 |
| `CMakeLists.txt` | ✅ | 已注册 `ShellHelper.h/.cpp`、`ContentContextMenu.h/.cpp`、`ContentPanel.h` |

**防重复造轮子检索记录**：
- `Select-String "extractArchive|extract_archive|Expand-Archive|unarchive" src/` → `SvgIcons.h` 中 `"unarchive"` 图标已存在，逻辑层无现有实现，确认无重复造轮子
- `Select-String "zip" src/ui/SvgIcons.h` → 命中：`{"zip", ...}` 专属拉链压缩包图标键已存在，直接复用

---

## Chapter 1: Overview（概述与根因分析）

### 问题根因
内容面板（`ContentPanel`）的右键上下文菜单（由 `ContentContextMenu` 承载）**缺失"解压压缩包"功能入口**，用户需要离开应用去操作，打断工作流。

### 设计目标
1. 在右键菜单**场景 2（选中具体项目）** 的非 `isDriveRoot` 分支，新增**一级菜单项"解压压缩包"**，仅在选中项为压缩包文件时显示；
2. 解压策略：`.zip` 格式通过 PowerShell `Expand-Archive` 实现原生解压；其余格式（`.7z`、`.rar`、`.tar.*` 等）调用系统关联程序处理；
3. 解压目标：自动在当前目录下创建**同名子文件夹**（若冲突则递增编号），解压完成后调用 `refreshAll()` 刷新面板；
4. SVG 图标：复用图标库已有的专属拉链压缩包 `"zip"` 键，满足语义精准契约；
5. **异步非阻塞**：`.zip` 通过 `QProcess::start()` 后台运行，`QPointer<ContentPanel>` 安全生命周期保护，GUI 主线程回调；
6. Toast 提示：使用 `ToolTipOverlay::instance()->showToast(...)` 固定定位，**严禁追随鼠标**（无鼠标坐标参数）。

### 涉及的修改逻辑

```
ContentPanel.h          ← 枚举追加 ActionExtractArchive
ShellHelper.h           ← 新增 static void extractArchive(const QString& archivePath, const QString& destDir)
ShellHelper.cpp         ← 实现 extractArchive（PowerShell + ShellExecuteExW 双路）
ContentContextMenu.cpp  ← 新增菜单项（isArchive 判断 + addAction）+ switch case + #include <QProcess>/<QTimer>
```

---

## Chapter 2: Modified Files List（影响文件清单）

| 操作类型 | 文件相对路径 |
|---|---|
| **修改** | `src/ui/ContentPanel.h` |
| **修改** | `src/util/ShellHelper.h` |
| **修改** | `src/util/ShellHelper.cpp` |
| **修改** | `src/ui/controllers/ContentContextMenu.cpp` |
| 不涉及 | `CMakeLists.txt`（所有文件已注册，无需修改） |
| 不涉及 | `src/ui/SvgIcons.h`（`"unarchive"` 图标已存在，直接复用） |

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/ContentPanel.h` — 枚举追加 `ActionExtractArchive`

```
<<<<<<< SEARCH
        ActionCopyName, ActionCopyPath, ActionAddToFavorites, ActionRefresh, ActionReextractThumbnail, ActionBatchCreate, ActionFileCompare,
        ActionOpenDirectoryInExplorer
    };
=======
        ActionCopyName, ActionCopyPath, ActionAddToFavorites, ActionRefresh, ActionReextractThumbnail, ActionBatchCreate, ActionFileCompare,
        ActionOpenDirectoryInExplorer, ActionExtractArchive
    };
>>>>>>> REPLACE
```

---

### 3.2 `src/util/ShellHelper.h` — 新增 `extractArchive` 静态方法声明

```
<<<<<<< SEARCH
    /**
     * @brief 物理设置文件/文件夹隐藏属性 (解耦自 DatabaseManager)
     */
    static void ensureHidden(const std::wstring& path);

};

} // namespace QuarkMeta
=======
    /**
     * @brief 物理设置文件/文件夹隐藏属性 (解耦自 DatabaseManager)
     */
    static void ensureHidden(const std::wstring& path);

    /**
     * @brief 解压压缩包到指定目标目录
     * @param archivePath 压缩包物理路径（支持 .zip / .7z / .rar / .tar.* 等格式）
     * @param destDir     解压目标目录（调用方负责确保该目录已创建）
     * @note  .zip 格式使用 PowerShell Expand-Archive 原生后台解压；
     *        其余格式通过 ShellExecuteExW 委托系统关联程序（7-Zip / WinRAR）处理。
     */
    static void extractArchive(const QString& archivePath, const QString& destDir);

};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

---

### 3.3 `src/util/ShellHelper.cpp` — 实现 `extractArchive`

```
<<<<<<< SEARCH
void ShellHelper::ensureHidden(const std::wstring& path) {
#ifdef Q_OS_WIN
    SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_HIDDEN);
#else
    Q_UNUSED(path);
#endif
}

} // namespace QuarkMeta
=======
void ShellHelper::ensureHidden(const std::wstring& path) {
#ifdef Q_OS_WIN
    SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_HIDDEN);
#else
    Q_UNUSED(path);
#endif
}

void ShellHelper::extractArchive(const QString& archivePath, const QString& destDir) {
#ifdef Q_OS_WIN
    QString ext = QFileInfo(archivePath).suffix().toLower();

    // .zip 格式：使用 PowerShell Expand-Archive 原生后台解压（startDetached 完全异步）
    if (ext == QStringLiteral("zip")) {
        QString nativeSrc  = QDir::toNativeSeparators(archivePath);
        QString nativeDest = QDir::toNativeSeparators(destDir);
        // -Force 允许向已存在目录写入（调用方已通过递增编号保证子文件夹唯一性）
        QString script = QString(
            "Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force"
        ).arg(nativeSrc, nativeDest);

        QProcess::startDetached(
            QStringLiteral("powershell"),
            QStringList() << QStringLiteral("-NoProfile")
                          << QStringLiteral("-NonInteractive")
                          << QStringLiteral("-WindowStyle") << QStringLiteral("Hidden")
                          << QStringLiteral("-Command") << script
        );
    } else {
        // 其他格式（.7z / .rar / .tar.* 等）：委托系统关联程序
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.fMask   = SEE_MASK_NOCLOSEPROCESS;
        sei.lpVerb  = L"open";
        std::wstring wpath = QDir::toNativeSeparators(archivePath).toStdWString();
        sei.lpFile  = wpath.c_str();
        sei.nShow   = SW_SHOW;
        ShellExecuteExW(&sei);
        if (sei.hProcess) CloseHandle(sei.hProcess);
    }
#else
    Q_UNUSED(archivePath);
    Q_UNUSED(destDir);
#endif
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

---

### 3.4 `src/ui/controllers/ContentContextMenu.cpp` — 补充头文件

> 新增 `QProcess` 与 `QTimer` 的显式 include，确保头文件闭合。

```
<<<<<<< SEARCH
#include <QFileDialog>
=======
#include <QFileDialog>
#include <QProcess>
#include <QTimer>
>>>>>>> REPLACE
```

---

### 3.5 `src/ui/controllers/ContentContextMenu.cpp` — 新增菜单项（场景 2 非 DriveRoot 分支）

> **插入位置**：在场景 2 非 `isDriveRoot` 分支里，紧在 `menu.addSeparator()` + `ActionCopy / ActionCut / ActionDuplicate` 之前，添加"解压压缩包"一级菜单项。逻辑上"解压"是原地内容展开操作，优先于剪贴板操作出现。

```
<<<<<<< SEARCH
            menu.addSeparator();

            menu.addAction(UiHelper::getIcon("copy", QColor("#EEEEEE"), 18), "复制（Ctrl+C）")->setData(ContentPanel::ActionCopy);
            menu.addAction(UiHelper::getIcon("cut", QColor("#EEEEEE"), 18), "剪切（Ctrl+X）")->setData(ContentPanel::ActionCut);
            menu.addAction(UiHelper::getIcon("copy", QColor("#EEEEEE"), 18), "创建副本")->setData(ContentPanel::ActionDuplicate);
=======
            // ---- 解压压缩包（仅对压缩格式文件显示）----
            if (!isFolder) {
                static const QStringList kArchiveExts = {
                    "zip", "7z", "rar", "tar", "gz", "bz2", "xz"
                };
                QString fileExt = itemInfo.suffix().toLower();
                bool isArchiveFile = kArchiveExts.contains(fileExt);
                if (isArchiveFile) {
                    menu.addAction(UiHelper::getIcon("zip", QColor("#EEEEEE"), 18), "解压压缩包")->setData(ContentPanel::ActionExtractArchive);
                }
            }

            menu.addSeparator();

            menu.addAction(UiHelper::getIcon("copy", QColor("#EEEEEE"), 18), "复制（Ctrl+C）")->setData(ContentPanel::ActionCopy);
            menu.addAction(UiHelper::getIcon("cut", QColor("#EEEEEE"), 18), "剪切（Ctrl+X）")->setData(ContentPanel::ActionCut);
            menu.addAction(UiHelper::getIcon("copy", QColor("#EEEEEE"), 18), "创建副本")->setData(ContentPanel::ActionDuplicate);
>>>>>>> REPLACE
```

---

### 3.6 `src/ui/controllers/ContentContextMenu.cpp` — 新增 switch case 处理

```
<<<<<<< SEARCH
        default:
            break;
    }
}

} // namespace QuarkMeta
=======
        case ContentPanel::ActionExtractArchive: {
            // 确定压缩包路径：仅处理第一个选中文件
            QStringList selectedPaths = m_panel->getSelectedPaths();
            QString archivePath = !selectedPaths.isEmpty() ? selectedPaths.first() : path;
            if (archivePath.isEmpty() || !QFileInfo::exists(archivePath)) break;

            QFileInfo archiveInfo(archivePath);
            QString parentDir = archiveInfo.absolutePath();
            QString baseName  = archiveInfo.completeBaseName();

            // 自动生成唯一目标子文件夹名（同名递增编号）
            QString destDir = QDir(parentDir).filePath(baseName);
            if (QDir(destDir).exists()) {
                int counter = 1;
                while (QDir(QDir(parentDir).filePath(
                    QString("%1-%2").arg(baseName).arg(counter))).exists()) {
                    ++counter;
                }
                destDir = QDir(parentDir).filePath(QString("%1-%2").arg(baseName).arg(counter));
            }
            QDir().mkpath(destDir);

            QString ext = archiveInfo.suffix().toLower();

            if (ext == QStringLiteral("zip")) {
                // .zip：通过 QProcess 追踪进程退出，完成后安全刷新面板
                ToolTipOverlay::instance()->showToast(
                    QString("正在解压 %1 ...").arg(archiveInfo.fileName()), 2000, QColor("#3498db")
                );

                QPointer<ContentPanel> weakPanel(m_panel);
                QString capturedArchive = archivePath;
                QString capturedDest    = destDir;

                QProcess* proc = new QProcess();
                QObject::connect(
                    proc,
                    QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    [weakPanel, proc, capturedArchive, capturedDest](int exitCode, QProcess::ExitStatus) {
                        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, exitCode, capturedArchive, capturedDest, proc]() {
                            if (weakPanel) weakPanel->refreshAll();
                            QFileInfo info(capturedArchive);
                            if (exitCode == 0) {
                                ToolTipOverlay::instance()->showToast(
                                    QString("解压完成：%1").arg(info.fileName()), 2000, QColor("#2ecc71")
                                );
                            } else {
                                // 失败时清理可能为空的目标目录
                                QDir failDir(capturedDest);
                                if (failDir.exists() && failDir.isEmpty()) failDir.rmdir(capturedDest);
                                ToolTipOverlay::instance()->showToast(
                                    QString("解压失败：%1").arg(info.fileName()), 2000, QColor("#e81123")
                                );
                            }
                            proc->deleteLater();
                        });
                    }
                );

                QString nativeSrc  = QDir::toNativeSeparators(capturedArchive);
                QString nativeDest = QDir::toNativeSeparators(capturedDest);
                QString script = QString(
                    "Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force"
                ).arg(nativeSrc, nativeDest);
                proc->start(QStringLiteral("powershell"),
                    QStringList() << QStringLiteral("-NoProfile")
                                  << QStringLiteral("-NonInteractive")
                                  << QStringLiteral("-WindowStyle") << QStringLiteral("Hidden")
                                  << QStringLiteral("-Command") << script);
            } else {
                // 其他格式：委托系统关联程序（7-Zip / WinRAR），无法精确追踪退出
                ShellHelper::extractArchive(archivePath, destDir);
                ToolTipOverlay::instance()->showToast(
                    QString("已委托系统程序处理，解压完成后将自动刷新"), 2500, QColor("#3498db")
                );
                QPointer<ContentPanel> weakPanel(m_panel);
                // 延迟 1.5s 刷新，给系统程序完成初始化的时间
                QTimer::singleShot(1500, QCoreApplication::instance(), [weakPanel]() {
                    if (weakPanel) weakPanel->refreshAll();
                });
            }
            break;
        }
        default:
            break;
    }
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 编译验证步骤
```bash
cmake --build . --config Release --target QuarkMeta
```
预期：**0 errors, 0 warnings**（与本次修改相关）

### 运行时验收标准

| 场景 | 操作步骤 | 预期结果 |
|---|---|---|
| `.zip` 文件解压 | 右键选中 `test.zip` → 点击"解压压缩包" | 当前目录出现 `test/` 子文件夹，内含解压文件；Toast "解压完成" |
| `.7z` 文件解压 | 右键选中 `test.7z` → 点击"解压压缩包" | 系统 7-Zip 程序被调用打开；Toast 显示委托提示；1.5s 后面板自动刷新 |
| 同名冲突处理 | 目录下已存在 `test/`，右键 `test.zip` 解压 | 自动创建 `test-1/`，不覆盖；Toast "解压完成" |
| 文件夹不显示 | 右键选中任意文件夹 | **不出现**"解压压缩包"菜单项 |
| 非压缩文件不显示 | 右键选中 `.jpg` 或 `.txt` | **不出现**"解压压缩包"菜单项 |
| 多选处理 | 同时选中多个压缩包后右键 | 菜单项出现；仅对 `selectedPaths.first()` 执行解压 |

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

| 功能 | 复用的官方 SSOT 入口 | 位置 |
|---|---|---|
| 刷新面板 | `ContentPanel::refreshAll()` | `ContentPanel.h` 公有槽函数 |
| Toast 提示 | `ToolTipOverlay::instance()->showToast(...)` | 全局单例，严格不追随鼠标 |
| 路径转换 | `QDir::toNativeSeparators()` | Qt 标准 API |
| 进程异步 | `QProcess::start()` + `QProcess::finished` 信号 | Qt 标准 API |
| 生命周期保护 | `QPointer<ContentPanel> weakPanel` | Qt 标准 API |
| SVG 图标 | 复用已存在的 `"zip"` 图标键 | `SvgIcons.h` |

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | SSOT 来源 | 派生/消费点 | 同步方向 |
|---|---|---|---|
| 当前选中路径 | `m_panel->getSelectedPaths()` | switch case ActionExtractArchive | 只读 |
| 压缩包扩展名检测 | `QFileInfo::suffix().toLower()` | `isArchiveFile` 判定 & 路由逻辑 | 只读计算 |
| 目标子文件夹路径 | 局部计算（`baseName + 递增编号`） | `ShellHelper::extractArchive` / `QProcess` | 局部生成后传入 |
| 面板刷新 | `ContentPanel::refreshAll()` SSOT | switch case 完成后统一调用 | 写 → 视图同步 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

### 新增接口：`ShellHelper::extractArchive`

| 检查项 | 内容 |
|---|---|
| **声明位置** | `src/util/ShellHelper.h`，`class ShellHelper { public:` 块内 |
| **声明签名** | `static void extractArchive(const QString& archivePath, const QString& destDir);` |
| **定义位置** | `src/util/ShellHelper.cpp`，`namespace QuarkMeta` 内 |
| **调用点** | `ContentContextMenu.cpp` switch case `ActionExtractArchive`，非 `.zip` 分支 |
| **访问级别** | `public static`，调用方已包含 `../../util/ShellHelper.h` ✅ |

### 现有接口调用核查

| 调用 | 物理签名位置 | 访问级别 |
|---|---|---|
| `m_panel->getSelectedPaths()` | `ContentPanel.h` L199，public | ✅ |
| `m_panel->refreshAll()` | `ContentPanel.h` L237，public slot | ✅ |
| `ToolTipOverlay::instance()->showToast(...)` | 全局单例，已在多处调用 | ✅ |
| `QProcess::finished` 信号 | Qt 5.x 标准信号 | ✅ |
| `QOverload<int, QProcess::ExitStatus>::of(...)` | Qt 5.x 标准语法 | ✅ |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

### `src/util/ShellHelper.cpp` 新增代码所需头文件

| 类型/API | `#include` 状态 |
|---|---|
| `QFileInfo` | `#include <QFileInfo>` ✅（第 2 行） |
| `QDir` | `#include <QDir>` ✅（第 6 行） |
| `QProcess::startDetached` | `#include <QProcess>` ✅（第 7 行） |
| `SHELLEXECUTEINFOW` / `ShellExecuteExW` | `#include <shellapi.h>` ✅（第 15 行） |
| `CloseHandle` | `#include <windows.h>` ✅（第 14 行） |

### `src/ui/controllers/ContentContextMenu.cpp` 新增代码所需头文件

| 类型/API | `#include` 状态 |
|---|---|
| `QFileInfo` | `#include <QFileInfo>` ✅（已有） |
| `QDir` | Qt 体系间接包含 ✅ |
| `QPointer` | `#include <QPointer>` ✅（第 52 行） |
| `QProcess` | ❌ 未有 → 方案 3.4 新增 `#include <QProcess>` ✅ |
| `QTimer` | ❌ 未有 → 方案 3.4 新增 `#include <QTimer>` ✅ |
| `ShellHelper` | `#include "../../util/ShellHelper.h"` ✅（第 20 行） |
| `ToolTipOverlay` | `#include "../ToolTipOverlay.h"` ✅（第 11 行） |
| `ContentPanel::ActionExtractArchive` | 通过 `#include "../ContentPanel.h"` ✅（第 2 行） |
| `QCoreApplication` | `#include <QApplication>` ✅（第 49 行，Qt 体系包含） |

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 编号 | 检查项 | 状态 | 位置/说明 |
|---|---|---|---|
| 1 | 方案已通过第 0 关需求确认 | ✅ 满足 | 用户明确确认三项设计决策 |
| 2 | 物理读取所有涉及文件（Gate 1） | ✅ 满足 | 见已读物理证据清单 |
| 3 | SEARCH 块原文真实存在且唯一匹配 | ✅ 满足 | 通过 `get-content` 逐行核查物理文件 |
| 4 | 新增接口声明、定义、调用点签名一致 | ✅ 满足 | Chapter 7 已逐项列出 |
| 5 | 被修改的 `#include` 间接包含链闭合 | ✅ 满足 | Chapter 8 全量核查，补充了 `QProcess`/`QTimer` |
| 6 | 新建/删除文件已在 CMakeLists.txt 同步 | ✅ 不涉及 | 无新建/删除文件，仅修改已注册文件 |
| 7 | 跨类调用访问级别均为 public | ✅ 满足 | Chapter 7 全量核查 |
| 8 | 方案未触及范围外文件 | ✅ 满足 | 仅修改 4 个目标文件 |
| 9 | 未采用笨拙浪费计算资源的手法 | ✅ 满足 | QProcess 异步非阻塞，扩展名匹配仅一次字符串比较 |
| 10 | 代码中无打补丁行为 | ✅ 满足 | 通过 ShellHelper 新增专用方法，遵循架构分层 |
| 11 | 遵循 Memories.md 规范 | ✅ 满足 | `"zip"` 图标 `#EEEEEE` 中性单色 |
| 12 | ToolTipOverlay 不追随鼠标 | ✅ 满足 | `showToast(...)` 均无鼠标坐标追踪参数 |
| 13 | 100% 语义化矢量图标（`#EEEEEE`） | ✅ 满足 | 复用 `"zip"` 图标，`QColor("#EEEEEE")` |
| 14 | 异步回调按值捕获（无悬空引用） | ✅ 满足 | `capturedArchive`、`capturedDest` 按值捕获；`weakPanel` 用 `QPointer` |
| 15 | 无 Lambda 循环引用 | ✅ 满足 | `proc` 通过 `deleteLater()` 安全释放，无循环引用 |
| 16 | 后台线程 UI 操作回归主线程 | ✅ 满足 | `QProcess::finished` 通过 `QMetaObject::invokeMethod` 回调主线程 |
| 17 | Qt 信号槽使用 C++11 函数指针语法 | ✅ 满足 | `QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished)` |
| 18 | 对外 `.h` 签名冻结（无破坏既有 API） | ✅ 满足 | 枚举末尾追加，`ShellHelper.h` 仅新增不修改既有声明 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 行为零篡改声明

| 既有行为 | 本方案影响 | 结论 |
|---|---|---|
| 现有右键菜单所有项（打开、复制、剪切、移动到、删除等） | 无任何修改 | ✅ 完全保留 |
| 场景 1（回收站）菜单 | 无任何修改 | ✅ 完全保留 |
| 场景 3（空白处）菜单 | 无任何修改 | ✅ 完全保留 |
| `isDriveRoot` 分支（盘符根目录）菜单 | 无任何修改 | ✅ 完全保留 |
| `isFolder = true` 时的文件夹右键菜单 | 新增的 `isArchiveFile` 判断只在 `!isFolder` 下进入 | ✅ 文件夹无影响 |
| `refreshAll()` 刷新机制 | 解压完成后统一通过 `weakPanel->refreshAll()` 刷新 | ✅ 遵循 SSOT |

### 数值零篡改声明

| 数值类型 | 值 | 来源对齐 |
|---|---|---|
| 菜单图标尺寸 | `18`（像素） | 与其他菜单项完全一致 |
| 菜单图标颜色 | `QColor("#EEEEEE")` | 与其他菜单项完全一致 |
| Toast 颜色 - 进行中 | `QColor("#3498db")` | 与其他"进行中" Toast 一致 |
| Toast 颜色 - 成功 | `QColor("#2ecc71")` | 与其他"成功" Toast 一致 |
| Toast 颜色 - 失败 | `QColor("#e81123")` | 与其他"失败" Toast 一致 |
| Toast 时长 - 进行中 | `2000`（ms） | 与其他"任务启动" Toast 一致 |
| Toast 时长 - 完成/失败 | `2000`（ms） | 与其他操作结果 Toast 一致 |
| 延迟刷新（非 zip 格式） | `1500`（ms） | 合理估计系统程序初始化时间 |
