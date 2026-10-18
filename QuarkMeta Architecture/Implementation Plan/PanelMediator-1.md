# PanelMediator 实施方案 (PanelMediator-1.md)

### 【第 1 关：起草前核查（Pre-Draft Evidence）】
1. **已读文件清单**：
   - `src/ui/PanelMediator.h`（已用工具 `read_file` 物理读取）
   - `src/ui/PanelMediator.cpp`（已用工具 `read_file` 物理读取）
2. **签名原文摘录**：
   - `void updateMetaPanelFromPanel(ContentPanel* panel);`（`PanelMediator.cpp` 内闭包函数）
3. **既有 API 检索记录**：
   - 检索关键词：`multiPreviewItems` / `kMultiPreviewMaxCards`
   - 结果：`PanelMediator.cpp` 第 525~610 行在多选更新时收集多选预览卡片项。
4. **Memories.md 相关条款**：
   - 用户意图优先与逻辑归一化。

---

## 1. Overview（概述与解决的问题）

本实施方案用于纠正 `PanelMediator.cpp` 中多选预览卡片收集的逻辑问题：
- **现存缺陷**：原代码在遍历选中路径 `paths` 时，当收集满 `5` 个项目后就触发 `break`/截断，导致多选超过 5 个项目时（例如选中 6 个项目），**后选中的项目（如第 6 个项目）被抛弃，只展示了前 5 个项目**。
- **重构方案**：改为**滑动窗口/末尾截取策略**，优先保证**收集并展示最后 5 个选中的项目**（例如选中 6 个项目时，收集第 2~6 个项目）。最后一个选中的项目（最末尾项）依然作为最顶层卡片进行展示。

---

## 2. Modified Files List（影响文件清单）

- `src/ui/PanelMediator.cpp`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 修改点 1：`src/ui/PanelMediator.cpp` - 调整多选预览卡片收集逻辑（截取最后 5 项）

<<<<<<< SEARCH
            QList<MetaPreviewItem> multiPreviewItems;

            for (int i = 0; i < paths.size(); ++i) {
                const QString& path = paths[i];
=======
            QList<MetaPreviewItem> multiPreviewItems;
            int previewStartIdx = qMax(0, paths.size() - MetaPanel::kMultiPreviewMaxCards);

            for (int i = 0; i < paths.size(); ++i) {
                const QString& path = paths[i];
>>>>>>> REPLACE

<<<<<<< SEARCH
                    if (multiPreviewItems.size() < MetaPanel::kMultiPreviewMaxCards) {
                        MetaPreviewItem item;
                        QVariant decData = idx.data(Qt::DecorationRole);
                        bool hasThumb = idx.data(HasThumbnailRole).toBool();
                        if (hasThumb) {
                            item.hasThumbnail = true;
                            if (decData.canConvert<QIcon>()) {
                                item.thumbnail = decData.value<QIcon>().pixmap(128, 128);
                            } else if (decData.canConvert<QPixmap>()) {
                                item.thumbnail = decData.value<QPixmap>();
                            }
                        } else {
                            item.hasThumbnail = false;
                            if (decData.canConvert<QIcon>()) {
                                item.icon = decData.value<QIcon>();
                            }
                        }
                        if ((item.hasThumbnail && !item.thumbnail.isNull()) || (!item.hasThumbnail && !item.icon.isNull())) {
                            multiPreviewItems.append(item);
                        }
                    }
=======
                    if (i >= previewStartIdx && multiPreviewItems.size() < MetaPanel::kMultiPreviewMaxCards) {
                        MetaPreviewItem item;
                        QVariant decData = idx.data(Qt::DecorationRole);
                        bool hasThumb = idx.data(HasThumbnailRole).toBool();
                        if (hasThumb) {
                            item.hasThumbnail = true;
                            if (decData.canConvert<QIcon>()) {
                                item.thumbnail = decData.value<QIcon>().pixmap(128, 128);
                            } else if (decData.canConvert<QPixmap>()) {
                                item.thumbnail = decData.value<QPixmap>();
                            }
                        } else {
                            item.hasThumbnail = false;
                            if (decData.canConvert<QIcon>()) {
                                item.icon = decData.value<QIcon>();
                            }
                        }
                        if ((item.hasThumbnail && !item.thumbnail.isNull()) || (!item.hasThumbnail && !item.icon.isNull())) {
                            multiPreviewItems.append(item);
                        }
                    }
>>>>>>> REPLACE

<<<<<<< SEARCH
                    if (multiPreviewItems.size() < MetaPanel::kMultiPreviewMaxCards) {
                        MetaPreviewItem item;
                        item.hasThumbnail = false;
                        QFileInfo fi(path);
                        if (fi.isDir()) {
                            item.icon = UiHelper::getIcon("folder", QColor("#3498db"), 128);
                        } else {
                            item.icon = UiHelper::getIcon("file_eps", QColor("#888888"), 128);
                        }
                        if (!item.icon.isNull()) {
                            multiPreviewItems.append(item);
                        }
                    }
=======
                    if (i >= previewStartIdx && multiPreviewItems.size() < MetaPanel::kMultiPreviewMaxCards) {
                        MetaPreviewItem item;
                        item.hasThumbnail = false;
                        QFileInfo fi(path);
                        if (fi.isDir()) {
                            item.icon = UiHelper::getIcon("folder", QColor("#3498db"), 128);
                        } else {
                            item.icon = UiHelper::getIcon("file_eps", QColor("#888888"), 128);
                        }
                        if (!item.icon.isNull()) {
                            multiPreviewItems.append(item);
                        }
                    }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译验证**：
   在终端或构建系统中执行常规 CMake / MSVC 编译。
2. **UI 细节核对**：
   - 连续选中 6 个项目，观察元数据面板顶部的堆叠预览图。
   - 确认显示的 5 张卡片包含的是**最后选中的 5 个项目**（第 2~6 个项目），其中第 6 个（最后选中的）项目呈现在最顶层。第 1 个早先选中的项目被顺延抛弃，符合预期逻辑。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）

本次修改仅调整 `PanelMediator.cpp` 中的收集起始索引 `previewStartIdx`，完全复用既有的多选数据广播架构。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

- 无接口签名修改。

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

不增加、删除或修改任何 `#include` 头文件包含行。

---

## 8. Compliance Self-Check Table（合规核对表）

| 检查项 | 来源条款 | 结论 | 对应方案位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | 文档标题 (PanelMediator-1.md) |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/` |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 | 第 3 章全部 SEARCH 块 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册，且有配套 .cpp | 3.3 | 不涉及 | 未新建或修改 Q_OBJECT 声明 |
| 第 1~9 章齐全 | 3.4 | 满足 | 第 1~9 章完整覆盖 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 第 6 章 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 不涉及 | 未修改头文件包含 |
| 已复用既有 SSOT 入口，无另起炉灶；旧分散实现已列入删除 | 2.4 | 满足 | 第 5 章 |
| 无内联 `setStyleSheet`，样式走 style.qss | 4.1 | 不涉及 | 不涉及 CSS 修改 |
| 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 均未采用 |
| 批量操作为一次聚合广播 | 4.3 | 不涉及 | 不涉及文件批量 IO |
| 迁移的所有数值原样照抄 | 4.4 | 满足 | 第 9 章 |
| 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories.md | 满足 | 100% 精确落实用户指定的“显示后五个”要求 |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅影响 `PanelMediator.cpp` |

---

## 9. Zero-Value-Alteration Check（数值零篡改核对）

| 参数/数值描述 | 原始数值 | 新数值 | 变更理由/依据 |
|---|---|---|---|
| 多选预览收集起始索引 (`previewStartIdx`) | `0` (截取前5项) | `qMax(0, paths.size() - 5)` (截取后5项) | 用户明确要求：应该是显示后五个，而不该是前五个 |
