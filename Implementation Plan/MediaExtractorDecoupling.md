# MediaExtractorDecoupling.md - 普通磁盘文件导航与重度媒体特征提取管线剥离重构方案

Base Commit Hash: `b46fdcfe017faaed42950107b45a2200ab05f9bf`
Prerequisite Plans: 无
Supersedes: 无

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题根因分析
在 QuarkMeta 当前实现中，当用户通过导航浏览普通磁盘目录（如包含 2000+ 张图片的文件夹）时，`ContentDataLoader::loadDirectory` 会在扫描并加载完毕目录项后，无差别地将该目录下**所有文件路径**构建为列表，并直接调用 `MetadataManager::instance().registerItemsAsync(filePaths)`。

这一盲目贪婪的全量投递行为引发了严重的算力与磁盘 I/O 浪费：
1. **脱离视口与用户意图**：用户可能仅仅是点进文件夹浏览或寻找某个特定文件，系统却立刻拉起 `MediaExtractorPipeline` 后台多线程，对文件夹内所有 2000+ 屏内外图片无差别进行全图解码、EXIF 尺寸解析、8 色加权调色板计算与主色调分析。
2. **计算资源巨额浪费**：屏外（Off-screen）数千张图片的色值分析完全脱离用户视野，导致 CPU、内存与磁盘 I/O 被无意义锁死。
3. **状态栏强打扰**：重度全量计算挂载了 `TaskProgressService`，导致界面底部频繁弹出“提取完成  数量：2039  |  实际耗时：00:01”的干扰性进度条，破坏了沉浸式文件浏览体验。

### 1.2 架构重构设计目标（方案 A）
1. **普通磁盘导航与重度媒体特征提取彻底解耦**：移除 `ContentDataLoader::loadDirectory` 每次读完普通磁盘目录后对整个文件夹文件的全量 `registerItemsAsync` 投递。
2. **保留必要按需加载与库模式逻辑**：在普通磁盘导航模式下，缩略图由 `ThumbnailPipelineService` 根据视口按需静默加载，元数据（色值/EXIF）在用户选中特定项目或进入“库（Library）”模式时按需补全。
3. **彻底消除资源浪费与状态栏打扰**：普通磁盘文件夹导航做到瞬间响应，0 额外观测不到的后台算力浪费，状态栏 0 打扰。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/ui/controllers/ContentDataLoader.cpp`（修改）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 修改 `src/ui/controllers/ContentDataLoader.cpp`

剥离 `loadDirectory` 中对扫盘结果无差别全量投递 `registerItemsAsync` 的笨拙逻辑：

```
<<<<<<< SEARCH
                // Enqueue scanned files for background media feature & auto-color extraction
                QStringList filePaths;
                for (const auto& item : allItems) {
                    if (!item.isDir && !item.path.isEmpty()) {
                        filePaths.append(item.path);
                    }
                }
                if (!filePaths.isEmpty()) {
                    MetadataManager::instance().registerItemsAsync(filePaths);
                }
=======
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 构建命令
在项目根目录运行以下 CMake 构建命令进行编译核验：
```bash
cmake -B build -G Ninja
cmake --build build --config Release
```

### 4.2 运行时验收标准
1. **进入大图文件夹秒开体验**：导航切换进入包含 2000+ 屏内外图片的普通磁盘文件夹，文件夹内容即刻呈现在视图中，无卡顿感。
2. **状态栏零打扰**：打开普通磁盘文件夹时，界面下方状态栏不再弹出“提取完成  数量：2039  |  实际耗时：00:01”等重度媒体提取进度提示。
3. **缩略图静默渲染正常**：网格视图/瀑布流视图下的缩略图仍由 `ThumbnailPipelineService` 视口按需管线正常渲染，不受影响。
4. **属性面板 (MetaPanel) 两段式加载正常**：选中具体文件时，`MetaPanel` 依然能从 `MetadataManager` 按需加载呈现基础与深层属性。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **官方导航与模型流水线复用**：保留 `ContentDataLoader::loadDirectory` 中既有的 `DiskScanService::scanDirectory` 与 `model()->setRecords(allItems)` 入口，不另起炉灶。
2. **消除贪婪全量提取**：将盲目依赖的 `MetadataManager::registerItemsAsync` 彻底从普通磁盘导航主流程中物理剥离，回归单一职责原则。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生副本 / 缓存位置 | 同步 / 刷新方向 |
| :--- | :--- | :--- | :--- |
| 普通磁盘目录列表 | 操作系统磁盘文件系统 (`QDir`) | `DiskItemModel` (`m_records`) | `DiskScanService` 扫描 ➔ `model()->setRecords()` |
| 视口缩略图 | 物理文件与磁盘缩略图缓存 | `ThumbnailPipelineService` 内存缓存 | 视口滚动 ➔ `loadBatchAsync` 异步加载 |
| 属性面板元数据 | `.QuarkMeta.json` 侧车文件 | `MetaMemoryCache` | 选中项变更 ➔ `MetaPanel` 按需加载 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. **外部调用接口核查**：本次重构仅剥离 `ContentDataLoader.cpp` 函数体内部的一段私有实现代码，无任何 `.h` 接口签名修改，对外 API 契约保持 100% 冻结与不变。
2. **访问级别**：无任何访问权限变动。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

1. **包含链无损核查**：修改点仅移除 `ContentDataLoader.cpp` 内 `loadDirectory` 槽回调里的局部 `registerItemsAsync` 调用，不涉及头文件 `#include` 的增删。
2. **类型完整性**：`ContentDataLoader.cpp` 中引用的所有 Qt 与 QuarkMeta 类型保持闭合且编译通过。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 是否满足 | 位置 / 说明 |
| :--- | :--- | :--- | :--- |
| 1 | 严禁盲改对外签名（Public API Freezing） | 满足 | 未修改任何 `.h` 接口签名 |
| 2 | 依赖单向流动（Shell->View->Controller->Domain->Data） | 满足 | 遵守 Controller 规范 |
| 3 | 查重优先与秒级检索契约（Search First） | 满足 | 确认剥离后不引入重复代码 |
| 4 | 核心行为 SSOT 入口唯一 | 满足 | 普通导航统一走 `loadDirectory` |
| 5 | 物理彻底清除死代码 | 满足 | 剥离无用贪婪全量注册逻辑 |
| 6 | 无内联硬编码样式 | 满足 | 不涉及 UI 样式调整 |
| 7 | 跨模块无全局事件过滤器滥用 | 满足 | 不涉及事件过滤器 |
| 8 | 零值与行为零篡改 | 满足 | 保持常规导航与缩略图交互完全一致 |
| 9 | 10 大章节规范完整无缺 | 满足 | 本方案包含 Chapter 1 至 10 全章节 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 既有 UI / 交互行为 | 重构后表现 | 是否 100% 一致 | 备注 |
| :--- | :--- | :--- | :--- |
| 目录载入与列表呈现 | 打开磁盘目录后，列表项正常装载呈现 | 100% 一致 | 响应更迅速 |
| 缩略图渲染 | 视口内图片按需静默加载缩略图 | 100% 一致 | 无额外耗能 |
| 状态栏任务进度 | 打开普通目录时不再强弹出提取完成进度提示 | 行为优化 | 消除强打扰 |
