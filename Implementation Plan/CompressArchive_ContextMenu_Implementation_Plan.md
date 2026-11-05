# 内容面板右键菜单 — "压缩为 ZIP"（打包）功能实施方案

**方案版本**：v1.0  
**日期**：2026-11-05  
**关联 Memories.md 条目**：`[新增记录] 内容面板右键菜单 — "压缩为 ZIP"（打包）功能设计理念`

---

## 已读物理证据清单（第 1 关：Gate 1）

| 文件 | 确认已读 | 关键物理签名 |
|---|---|---|
| `src/ui/ContentPanel.h` | ✅ | `enum ContextAction { ... ActionOpenDirectoryInExplorer, ActionExtractArchive };`（第 63-70 行） |
| `src/ui/controllers/ContentContextMenu.cpp` | ✅ | 场景 2 非 DriveRoot 分支 L338-350：`ActionExtractArchive` 之后接 `menu.addSeparator()` |
| `src/ui/controllers/ContentContextMenu.cpp` | ✅ | switch 末尾 L1148-1155：`case ActionExtractArchive` 之后，`default: break; } } } // namespace QuarkMeta` |
| `src/ui/SvgIcons.h` | ✅ | 已存在 `"zip"` 专属拉链压缩包图标键（第 2472 行），语义精准 |
| `CMakeLists.txt` | ✅ | `ContentContextMenu.cpp` 与 `ContentPanel.h` 均已完整注册 |

**防重复造轮子检索记录**：
- `Select-String "ActionCompressToZip|Compress-Archive" src/` → 逻辑层无现有实现，确认无重复造轮子
- `Select-String "zip" src/ui/SvgIcons.h` → 命中：`{"zip", ...}` 图标键已存在，直接复用

---

## Chapter 1: Overview（概述与根因分析）

### 问题根因
内容面板（`ContentPanel`）的右键上下文菜单虽然已有“解压压缩包”功能，但**缺少对称的“打包/压缩为 ZIP”能力**。用户若需将选中的单项或多项文件/文件夹打包分享，必须退出应用借由第三方工具完成，破坏了文件管理的基础能力闭环。

### 设计目标
1. 在右键菜单**场景 2（选中具体项目）** 的非 `isDriveRoot` 分支，新增**一级菜单项"压缩为 ZIP"**，用户只要选中 1 个或多个文件/文件夹即可使用；
2. 命名与冲突策略：
   - 单选时以选中项的主文件名命名（如 `doc.txt` → `doc.zip`，`images/` → `images.zip`）；
   - 多选时以第一个选中项的名称命名；
   - 若当前目录已存在同名 `.zip` 文件，自动递增编号命名（如 `doc-1.zip`、`doc-2.zip`），绝不覆盖冲洗已有文件；
3. 执行机制：通过 Windows 原生 PowerShell `Compress-Archive` 在后台静默异步执行，通过 `QProcess` 监听完成状态，无界面假死卡顿；
4. 视图更新与反馈：打包开始与完成均通过 `ToolTipOverlay::showToast` 给出反馈，完成后统一通过官方 SSOT `ContentPanel::refreshAll()` 原地刷新视图；
5. 图标与视觉：统一绑定专属拉链压缩包图标键 `"zip"`，遵循 `#EEEEEE` 中性单色标准。

### 涉及的修改逻辑

```
ContentPanel.h          ← 枚举追加 ActionCompressToZip
ContentContextMenu.cpp  ← 新增菜单项（menu.addAction）+ switch case（ActionCompressToZip）
```

---

## Chapter 2: Modified Files List（影响文件清单）

| 操作类型 | 文件相对路径 |
|---|---|
| **修改** | `src/ui/ContentPanel.h` |
| **修改** | `src/ui/controllers/ContentContextMenu.cpp` |
| 不涉及 | `CMakeLists.txt`（所有文件已注册，无需修改） |
| 不涉及 | `src/ui/SvgIcons.h`（`"zip"` 图标已存在，直接复用） |

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/ContentPanel.h` — 枚举追加 `ActionCompressToZip`

```
<<<<<<< SEARCH
        ActionCopyName, ActionCopyPath, ActionAddToFavorites, ActionRefresh, ActionReextractThumbnail, ActionBatchCreate, ActionFileCompare,
        ActionOpenDirectoryInExplorer, ActionExtractArchive
    };
=======
        ActionCopyName, ActionCopyPath, ActionAddToFavorites, ActionRefresh, ActionReextractThumbnail, ActionBatchCreate, ActionFileCompare,
        ActionOpenDirectoryInExplorer, ActionExtractArchive, ActionCompressToZip
    };
>>>>>>> REPLACE
```

---

### 3.2 `src/ui/controllers/ContentContextMenu.cpp` — 新增菜单项

> **插入位置**：在“解压压缩包”判断块之后、`menu.addSeparator()` 之前，添加“压缩为 ZIP”一级菜单项。

```
<<<<<<< SEARCH
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

            // ---- 压缩为 ZIP（选中任意项目即可打包）----
            menu.addAction(UiHelper::getIcon("zip", QColor("#EEEEEE"), 18), "压缩为 ZIP")->setData(ContentPanel::ActionCompressToZip);

            menu.addSeparator();
>>>>>>> REPLACE
```

---

### 3.3 `src/ui/controllers/ContentContextMenu.cpp` — 新增 switch case 处理

```
<<<<<<< SEARCH
            break;
        }
        default:
            break;
    }
}

} // namespace QuarkMeta
=======
            break;
        }
        case ContentPanel::ActionCompressToZip: {
            QStringList selectedPaths = m_panel->getSelectedPaths();
            if (selectedPaths.isEmpty() && !path.isEmpty()) {
                selectedPaths << path;
            }
            if (selectedPaths.isEmpty()) break;

            QString firstItem = selectedPaths.first();
            QFileInfo firstInfo(firstItem);
            QString parentDir = firstInfo.isDir() && selectedPaths.size() == 1
                ? firstInfo.dir().absolutePath()
                : (currentPath.isEmpty() || currentPath.contains("://") ? firstInfo.absolutePath() : currentPath);

            // 基础名称：文件夹取目录名，文件取主文件名（无后缀）
            QString baseName = firstInfo.isDir() ? firstInfo.fileName() : firstInfo.completeBaseName();
            if (baseName.isEmpty()) baseName = QStringLiteral("Archive");

            // 自动计算递增编号目标 zip 路径，防冲洗覆盖
            QString destZip = QDir(parentDir).filePath(baseName + QStringLiteral(".zip"));
            if (QFile::exists(destZip)) {
                int counter = 1;
                while (QFile::exists(QDir(parentDir).filePath(QString("%1-%2.zip").arg(baseName).arg(counter)))) {
                    ++counter;
                }
                destZip = QDir(parentDir).filePath(QString("%1-%2.zip").arg(baseName).arg(counter));
            }

            QFileInfo destZipInfo(destZip);
            ToolTipOverlay::instance()->showToast(
                QString("正在将 %1 个项目压缩为 %2 ...").arg(selectedPaths.size()).arg(destZipInfo.fileName()),
                2000,
                QColor("#3498db")
            );

            // 构造 PowerShell 脚本：使用单引号与转义处理路径
            QStringList nativePaths;
            for (const QString& p : selectedPaths) {
                QString nPath = QDir::toNativeSeparators(p);
                nPath.replace("'", "''"); // PowerShell 单引号转义
                nativePaths << QString("'%1'").arg(nPath);
            }
            QString nativeDest = QDir::toNativeSeparators(destZip);
            nativeDest.replace("'", "''");

            QString script = QString(
                "$files = @(%1); Compress-Archive -LiteralPath $files -DestinationPath '%2' -Force"
            ).arg(nativePaths.join(", "), nativeDest);

            QPointer<ContentPanel> weakPanel(m_panel);
            QString capturedDestZip = destZip;

            QProcess* proc = new QProcess();
            QObject::connect(
                proc,
                QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [weakPanel, proc, capturedDestZip](int exitCode, QProcess::ExitStatus) {
                    QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, exitCode, capturedDestZip, proc]() {
                        if (weakPanel) weakPanel->refreshAll();
                        QFileInfo info(capturedDestZip);
                        if (exitCode == 0) {
                            ToolTipOverlay::instance()->showToast(
                                QString("压缩完成：%1").arg(info.fileName()), 2000, QColor("#2ecc71")
                            );
                        } else {
                            // 若压缩失败且产生了残缺文件，则进行清理
                            if (QFile::exists(capturedDestZip)) {
                                QFile::remove(capturedDestZip);
                            }
                            ToolTipOverlay::instance()->showToast(
                                QString("压缩失败：%1").arg(info.fileName()), 2000, QColor("#e81123")
                            );
                        }
                        proc->deleteLater();
                    });
                }
            );

            proc->start(
                QStringLiteral("powershell"),
                QStringList() << QStringLiteral("-NoProfile")
                              << QStringLiteral("-NonInteractive")
                              << QStringLiteral("-WindowStyle") << QStringLiteral("Hidden")
                              << QStringLiteral("-Command") << script
            );
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
| 单文件压缩 | 选中 `note.txt` 右键 → 点击"压缩为 ZIP" | 当前目录下生成 `note.zip`；Toast 提示“压缩完成”；面板原地刷新 |
| 单文件夹压缩 | 选中 `images` 文件夹右键 → 点击"压缩为 ZIP" | 当前目录下生成 `images.zip`；Toast 提示“压缩完成”；面板原地刷新 |
| 多选批量压缩 | 同时选中 3 个文件右键 → 点击"压缩为 ZIP" | 当前目录下生成以首个项目命名的 `.zip`；包内含所有 3 个文件 |
| 同名冲突解决 | 目录下已存在 `note.zip` 时再次压缩 `note.txt` | 自动生成 `note-1.zip`，不覆盖已有文件；Toast 提示对应文件名 |
| 空白处右键 | 在空白处右键 | **不出现**“压缩为 ZIP”项 |
| 图标与视觉核对 | 查看菜单项视觉 | 渲染经典的带拉链矢量图标 `"zip"`，图标颜色为 `#EEEEEE` |

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

| 功能 | 复用的官方 SSOT 入口 | 位置 |
|---|---|---|
| 刷新面板 | `ContentPanel::refreshAll()` | `ContentPanel.h` 公有槽函数 |
| Toast 提示 | `ToolTipOverlay::instance()->showToast(...)` | 全局单例，严格不追随鼠标 |
| 路径转换 | `QDir::toNativeSeparators()` | Qt 标准 API |
| 进程异步 | `QProcess::start()` + `QProcess::finished` 信号 | Qt 标准 API |
| 生命周期保护 | `QPointer<ContentPanel> weakPanel` | Qt 标准 API |
| 矢量图标 | 复用已有的 `"zip"` 图标键 | `SvgIcons.h` |

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | SSOT 来源 | 派生/消费点 | 同步方向 |
|---|---|---|---|
| 当前选中路径 | `m_panel->getSelectedPaths()` | switch case ActionCompressToZip | 只读 |
| 目标 `.zip` 路径 | 局部计算（`baseName + 递增编号`） | PowerShell 脚本参数 | 局部生成后传入 |
| 面板刷新 | `ContentPanel::refreshAll()` SSOT | switch case 完成后统一调用 | 写 → 视图同步 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

| 调用 | 物理签名位置 | 访问级别 |
|---|---|---|
| `m_panel->getSelectedPaths()` | `ContentPanel.h` L199 | ✅ public |
| `m_panel->refreshAll()` | `ContentPanel.h` L237 | ✅ public slot |
| `ToolTipOverlay::instance()->showToast(...)` | 全局单例，已在多处调用 | ✅ public |
| `QProcess::finished` 信号 | Qt 5.x/6.x 标准信号 | ✅ |
| `QOverload<int, QProcess::ExitStatus>::of(...)` | Qt 标准语法 | ✅ |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

| 类型/API | `#include` 状态 |
|---|---|
| `QProcess` | `#include <QProcess>` ✅（`ContentContextMenu.cpp` 第 31 行已有） |
| `QTimer` | `#include <QTimer>` ✅（`ContentContextMenu.cpp` 第 32 行已有） |
| `QFileInfo` | `#include <QFileInfo>` ✅（第 47 行） |
| `QDir` | `#include <QDir>` ✅（第 48 行） |
| `QPointer` | `#include <QPointer>` ✅（第 54 行） |
| `ToolTipOverlay` | `#include "../ToolTipOverlay.h"` ✅（第 11 行） |
| `ContentPanel::ActionCompressToZip` | 通过 `#include "../ContentPanel.h"` ✅（第 2 行） |

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 编号 | 检查项 | 状态 | 位置/说明 |
|---|---|---|---|
| 1 | 方案已通过第 0 关需求确认 | ✅ 满足 | 用户明确表示“达成共识” |
| 2 | 物理读取所有涉及文件（Gate 1） | ✅ 满足 | 见已读物理证据清单 |
| 3 | SEARCH 块原文真实存在且唯一匹配 | ✅ 满足 | 见验证步骤 |
| 4 | 新增接口声明、定义、调用点签名一致 | ✅ 满足 | 枚举与 switch case 签名一致 |
| 5 | 被修改的 `#include` 间接包含链闭合 | ✅ 满足 | Chapter 8 全量核查，头文件完整闭合 |
| 6 | 新建/删除文件已在 CMakeLists.txt 同步 | ✅ 不涉及 | 无新建/删除文件，仅修改已注册文件 |
| 7 | 跨类调用访问级别均为 public | ✅ 满足 | Chapter 7 全量核查 |
| 8 | 方案未触及范围外文件 | ✅ 满足 | 仅涉及 2 个目标文件 |
| 9 | 未采用笨拙浪费计算资源的手法 | ✅ 满足 | PowerShell 后台异步非阻塞，无轮询 |
| 10 | 代码中无打补丁行为 | ✅ 满足 | 遵循既有架构，通过 ContentContextMenu 控制层组装 |
| 11 | 遵循 Memories.md 规范 | ✅ 满足 | 专属 `"zip"` 图标，`#EEEEEE` 中性单色 |
| 12 | ToolTipOverlay 不追随鼠标 | ✅ 满足 | `showToast(...)` 均无鼠标坐标追踪参数 |
| 13 | 100% 语义化矢量图标（`#EEEEEE`） | ✅ 满足 | 复用 `"zip"` 图标，`QColor("#EEEEEE")` |
| 14 | 异步回调按值捕获（无悬空引用） | ✅ 满足 | `capturedDestZip` 按值捕获；`weakPanel` 用 `QPointer` |
| 15 | 无 Lambda 循环引用 | ✅ 满足 | `proc` 通过 `deleteLater()` 安全释放 |
| 16 | 后台线程 UI 操作回归主线程 | ✅ 满足 | `QProcess::finished` 通过 `QMetaObject::invokeMethod` 回调主线程 |
| 17 | Qt 信号槽使用 C++11 函数指针语法 | ✅ 满足 | `QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished)` |
| 18 | 对外 `.h` 签名冻结（无破坏既有 API） | ✅ 满足 | 枚举末尾追加，无修改既有签名 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 行为零篡改声明

| 既有行为 | 本方案影响 | 结论 |
|---|---|---|
| 既有“解压压缩包”功能 | 无任何修改 | ✅ 完全保留 |
| 现有右键菜单其他所有项 | 无任何修改 | ✅ 完全保留 |
| 场景 1（回收站）菜单 | 无任何修改 | ✅ 完全保留 |
| 场景 3（空白处）菜单 | 无任何修改 | ✅ 完全保留 |
| `refreshAll()` 刷新机制 | 压缩完成后统一通过 `weakPanel->refreshAll()` 刷新 | ✅ 遵循 SSOT |

### 数值零篡改声明

| 数值类型 | 值 | 来源对齐 |
|---|---|---|
| 菜单图标尺寸 | `18`（像素） | 与其他菜单项完全一致 |
| 菜单图标颜色 | `QColor("#EEEEEE")` | 与其他菜单项完全一致 |
| Toast 颜色 - 进行中 | `QColor("#3498db")` | 与其他"进行中" Toast 一致 |
| Toast 颜色 - 成功 | `QColor("#2ecc71")` | 与其他"成功" Toast 一致 |
| Toast 颜色 - 失败 | `QColor("#e81123")` | 与其他"失败" Toast 一致 |
| Toast 时长 | `2000`（ms） | 与其他操作 Toast 一致 |
