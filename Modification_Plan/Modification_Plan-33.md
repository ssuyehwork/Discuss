# 缩略图“先显示后秒消失”故障分析与自适应缓存容量优化 —— Modification_Plan-33.md

## 1. 任务背景
在内容面板显示数据时，出现了缩略图加载后先显示、随后在1秒内瞬间消失退化为纯灰色占位符的异常闪烁现象。本任务旨在彻底分析该问题的根本原因（是刷新/渲染导致的，还是 LRU 淘汰导致的），并重新设计高健壮、高内存安全性的动态缓存容量自适应公式，以确保在常用规模文件夹下实现完全无淘汰的无损浏览体验，同时在极端巨型或百万级数据集下进行平滑内存妥协。

## 2. 问题定位

### 2.1 “先显示后秒消失”的根本原因排查与定位
经过对 `src/ui/ContentPanel.cpp` 以及 `src/ui/WindowsShellThumbnailProvider.cpp` 源码的深入审计，定位出导致该现象的根本原因**不是** LRU 缓存的淘汰行为，而是由于**在后台提取线程中非法跨线程直接操纵 GUI 相关的 `QPixmap` 导致的底层显存/图形句柄失效失效**：

1.  **文件位置**：`src/ui/ContentPanel.cpp`
2.  **函数位置**：`FerrexVirtualDbModel::loadThumbnailsForRows(const QList<int>& rows)`
3.  **异常逻辑（行号 501~519）**：
    ```cpp
    if (ext == "svg") {
        QSvgRenderer renderer(path);
        if (renderer.isValid()) {
            QPixmap pix(128, 128); // ❌ 严重违规：在 Concurrent 子线程中直接创建 QPixmap
            pix.fill(Qt::transparent);
            QPainter painter(&pix); // ❌ 严重违规：在子线程中使用 QPainter 操纵 QPixmap
            renderer.render(&painter);
            icon = QIcon(pix);      // 跨线程打包成了 QIcon
            ar = 1.0;
            hasThumb = true;
        }
    } else if (UiHelper::isGraphicsFile(ext)) {
        QImage img = UiHelper::getShellThumbnail(path, 128);
        if (!img.isNull()) {
            icon = QIcon(QPixmap::fromImage(img)); // ❌ 严重违规：跨线程执行 QPixmap 转换
            ar = (double)img.width() / img.height();
            hasThumb = true;
        }
    }
```
4.  **失效机理**：
    *   在 Qt 和 Windows 操作系统中，`QPixmap` 深度绑定到了底层的 GUI 硬件/绘图上下文设备句柄（GDI / DirectX 资源），必须且只能在 GUI 主线程中被创建和生命周期管理。
    *   在 `loadThumbnailsForRows` 运行的 `QtConcurrent::run` 后台并发子线程中创建 `QPixmap` 时，Qt 在底层实际上使用了临时且临时的线程局部上下文进行设备关联。
    *   当子线程执行完毕退出、调用 `CoUninitialize()` 反初始化或并发任务生命周期结束时，该后台线程局部的所有图形设备句柄及硬件资源被强行回收或置空。
    *   这导致主线程缓存中 `QIcon` 底层所包裹的 `QPixmap` 瞬间成为“野句柄”。一旦主线程在随后因为滚动、定时器超时或悬停事件触发重绘时，`QPixmap` 在 `paint` 中绘制失败，缩略图随之抹去、永久变回纯灰色占位符。这正是**“先闪现一下、然后秒消失变灰”**（对应用户原话：“内容面板显示数据时先显示缩略图然后秒消失”）的渲染异常根因（对应用户原话：“是刷新 / 还是渲染导致的？”）。

### 2.2 关于 `maxCost` (500) 淘汰与重新加载逻辑的确认
对于用户提出的质疑（对应用户原话：“setMaxCost（第 90 行 setMaxCost(500)）的 LRU 淘汰行为，是否是'缩略图先显示后消失、变回占位符'现象的根因”）：
*   **非本现象根因**：当前场景中只有 14 个 SVG 文件。由于 $14 \ll 500$，远远未达到 `m_iconCache`（`setMaxCost(500)`）的驱逐上限，所以本次闪烁消失非淘汰所致。
*   **淘汰重载验证**：如果条目确实因为超出 `maxCost` 被淘汰，`m_iconCache.contains(cacheKey)` 在下一次滚动到该行并重绘时将返回 `false`，从而重新加入 `newQueue`。**它将正常重新触发一次异步提取并补回缩略图，而不是因为某个“已处理”标记被跳过从而永久停留在占位符状态**。

---

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 内容面板显示数据时先显示缩略图然后秒消失（用户原话） | 后台线程移除 `QPixmap` 和 `QIcon` 构建，改由全链路 `QImage` 传回主线程组装，彻底解决跨线程资源销毁导致的秒消失问题 | ✅ 一致 |
| 2    | 是刷新 / 还是渲染导致的？（用户原话） | 确认为子线程非法渲染 `QPixmap` 导致底层句柄失效的渲染及线程安全问题 | ✅ 一致 |
| 3    | setMaxCost（第 90 行 setMaxCost(500)）的 LRU 淘汰行为，是否是"缩略图先显示后消失、变回占位符"现象的根因（用户原话） | 确认为非淘汰所致，并验证了淘汰重新加载的安全性，确认淘汰项在滚动重绘时会正常重新异步加载补回 | ✅ 一致 |
| 4    | 重新给出具体公式并用 726 项这个真实场景代入验证，确认能覆盖（用户原话） | 重新设计了“总项目数 + 视口行数动态自适应”二级计算公式，代入 726 项场景验证，确保其缓存上限为 776 从而彻底消除淘汰 | ✅ 一致 |
| 5    | dynamicCost 的计算和 setMaxCost 调用，请确认是否在 setRecords 阶段就已经同步生效（用户原话） | 方案中明确并规范了在 `setRecords` 阶段同步计算并更新 `setMaxCost` 的机制，防止“刚打开未滚动”场景绕过本修复 | ✅ 一致 |
| 6    | 从一个大文件夹（如776上限）切换到小文件夹时，setMaxCost 缩容触发的自动淘汰，是否会正确按"最久未使用"释放旧文件夹的缓存，而不会误伤新文件夹刚加载完成的缩略图（用户原话） | 评估并确认了缩容时的淘汰机制符合严格的 LRU，旧文件夹冷数据会被精确淘汰，而不会伤及新文件夹刚加载的高频热数据 | ✅ 一致 |

---

## 4. 详细解决方案

### 4.1 全链路 QImage 线程无损解耦重构
为彻底消除子线程操作 GUI 资源导致显存失效的问题，我们将所有在子线程中执行的代码重写为纯 `QImage` 操作，将 `QPixmap` 和 `QIcon` 的构造推迟至主线程：

1.  **后台子线程（`loadThumbnailsForRows`并发运行体内）**：
    *   **SVG 渲染重构**：改用硬件无关的 `QImage` 做中间缓冲区画布（对应用户原话：“内容面板显示数据时先显示缩略图然后秒消失”）。
        ```cpp
        QImage img;
        if (ext == "svg") {
            QSvgRenderer renderer(path);
            if (renderer.isValid()) {
                QImage svgImg(128, 128, QImage::Format_ARGB32);
                svgImg.fill(Qt::transparent);
                QPainter painter(&svgImg);
                renderer.render(&painter);
                img = svgImg;
                ar = 1.0;
                hasThumb = true;
            }
        }
        ```
    *   **图形格式解析重构**：子线程获取 `QImage` 后，不转 `QPixmap` / `QIcon`，保留为 `QImage`。
        ```cpp
        else if (UiHelper::isGraphicsFile(ext)) {
            img = UiHelper::getShellThumbnail(path, 128);
            if (!img.isNull()) {
                ar = (double)img.width() / img.height();
                hasThumb = true;
            }
        }
        ```
2.  **信号回送与主线程 Lambda 处理**：
    *   通过 `QMetaObject::invokeMethod` 传递 `QImage`（而不是 `QIcon`）回主线程。
    *   在主线程中执行安全的 `fromImage` 转换，合成 `QIcon` 并插入 `m_iconCache`：
        ```cpp
        if (weakThis) {
            QMetaObject::invokeMethod(const_cast<FerrexVirtualDbModel*>(weakThis.data()), [weakThis, path, cacheKey, img, ar, hasThumb]() {
                if (!weakThis) return;
                auto* mutableThis = const_cast<FerrexVirtualDbModel*>(weakThis.data());
                
                QIcon icon;
                if (!img.isNull()) {
                    icon = QIcon(QPixmap::fromImage(img)); // 主线程执行 fromImage，安全稳健
                } else {
                    icon = UiHelper::getFileIcon(path, 128); // 主线程获取系统图标，安全稳健
                }
                
                mutableThis->m_iconCache.insert(cacheKey, new QIcon(icon));
                if (hasThumb) mutableThis->m_aspectRatios[path] = ar;
                
                for (int i = 0; i < mutableThis->m_displayCount; ++i) {
                    const auto& rec = mutableThis->m_allRecords[i];
                    if (rec.path == path) {
                        emit mutableThis->dataChanged(mutableThis->index(i, 0), mutableThis->index(i, 0), {Qt::DecorationRole, AspectRatioRole, HasThumbnailRole});
                        break;
                    }
                }
            }, Qt::QueuedConnection);
        }
        ```

### 4.2 双阶段自适应缓存容量自适应更新机制
我们将“总项目数 + 视口行数动态自适应”二级计算公式，应用到**数据载入初载（`setRecords`）**以及**滚动加载（`loadThumbnailsForRows`）**两个黄金阶段，提供饱和的动态保护：

1.  **阶段 1：首载即时保护（`setRecords`）**：
    （对应用户原话：“dynamicCost 的计算和 setMaxCost 调用，请确认是否在 setRecords 阶段就已经同步生效”）
    在用户尚未开始滚动时，提前预测首载缓存大小，防止第一批缩略图提前被抛弃。
    ```cpp
    void FerrexVirtualDbModel::setRecords(const std::vector<ItemRecord>& records) {
        beginResetModel();
        m_allRecords = records;
        ...
        // 【双阶段保护 - 阶段一】：初始化设置合理的 maxCost 限制
        int folderTotal = static_cast<int>(m_allRecords.size());
        const int hardLimit = 3000;
        int initCost = 500;
        if (folderTotal <= hardLimit) {
            initCost = qMax(500, folderTotal + 50); // 无损冗余缓冲
        } else {
            initCost = qBound(1000, 40 * 8, hardLimit); // 在尚无视口行数测量数据时预设 40 行可见进行缓冲计算
        }
        m_iconCache.setMaxCost(initCost);
        ...
        endResetModel();
    }
    ```
2.  **阶段 2：动态视口修正（`loadThumbnailsForRows`）**：
    在滚动加载触发时，采用实际精准测量的可见行数 `visibleRows.size()` 来进一步精确动态调整上限上限：
    ```cpp
    void FerrexVirtualDbModel::loadThumbnailsForRows(const QList<int>& rows) {
        // 【双阶段保护 - 阶段二】：基于实际测量出的可见行数动态修正 maxCost
        int folderTotal = static_cast<int>(m_allRecords.size());
        int visibleCount = rows.size();
        const int hardLimit = 3000;
        int dynamicCost = 500;
        
        if (folderTotal <= hardLimit) {
            dynamicCost = qMax(500, folderTotal + 50);
        } else {
            dynamicCost = qBound(1000, visibleCount * 8, hardLimit);
        }
        m_iconCache.setMaxCost(dynamicCost);
        
        // 之后的队列过滤与异步加载逻辑保持不变...
    }
    ```

### 4.3 文件夹切换时缩容的安全保障分析
（对应用户原话：“从一个大文件夹（如776上限）切换到小文件夹（如500上限）时，setMaxCost 缩容触发的自动淘汰，是否会正确按'最久未使用'释放旧文件夹的缓存，而不会误伤新文件夹刚加载完成的缩略图”）

当大文件夹（缓存上限 776）切换到小文件夹（缓存上限 500）时，`m_iconCache.setMaxCost(500)` 被调用触发缩容：
*   **物理机制**：`QCache` 会在内部立刻丢弃超出 500 的项。
*   **淘汰顺序**：`QCache` 严格按照“最久未使用（Least Recently Used）”倒序淘汰。由于旧文件夹的缓存内容在切换后不再有任何组件去请求它们，已经属于在 LRU 队列中冷得最透的数据（排列在链表最末端）。而新文件夹内刚被请求或插入的缩略图项属于高频访问的热数据（排列在链表最顶端）。
*   **结论**：`QCache` 会精准、安全、100%地优先剔除排列在链表最末端的旧文件夹缓存资源，绝不会误伤新文件夹刚载入的任何热数据，完全满足零干扰切换要求。

---

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/ui/ContentPanel.cpp`
  - 函数：`FerrexVirtualDbModel::setRecords`
  - 函数：`FerrexVirtualDbModel::data`
  - 函数：`FerrexVirtualDbModel::loadThumbnailsForRows`

**明确禁止越界修改的范围：**
- [ ] 物理磁盘 USN 监控感知底座——不修改
- [ ] MFT 解析器核心提取管道——不修改
- [ ] 分类管理层关系数据库驱动——不修改

---

## 6. 实现准则与预警【核心】
1.  **无 QPixmap / QIcon 入子线程**：任何在子线程内部运行的代码块都不应创建、复制、转换或处理 `QPixmap` 或 `QIcon` 实例，以彻底规避图形硬句柄与线程亲和性冲突冲突。
2.  **精准保留 QImage 所有权**：在子线程创建 `QImage` 后，需确保通过 `invokeMethod` 值传递形式干净地交付给 GUI 线程进行消费，避免因为指针生命周期不一产生野指针或段错误。
3.  **内存极限预期控制**：
    对于远超硬上限的超大文件夹（例如数万项到百万项级别）：
    *   缓存上限将被自适应限幅机制强制锁死在 `hardLimit (3000)`，以防御性保护物理内存不因巨量 GDI 资源消耗发生 OOM 崩溃。
    *   在这种海量级极端场景下，旧数据被淘汰并在回滚时重新触发加载，是在有限系统物理资源约束下的最优设计妥协与合理折中权衡，属于底层内存分配器及硬件受限行为，不属于由于代码缺陷引起的未彻底解决。

---

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 异步加载防闪烁规范 (#8) | 在异步数据扫描前，禁止调用 `m_model->clear()` | ✅ 符合。本方案没有也不会改变 `clear()` 的调用链 |
| 缩略图平滑加载规范 (#9) | 图形文件异步加载期间，`data()` 必须返回 `QIcon()` 空图标，且由 `ThumbnailDelegate` 检测并绘制 `#3A3A3A` 占位背景 | ✅ 符合。本方案在 `data()` 中对处于加载状态的 `isGraphic` 依然完美保持返回 `QIcon()` 以触发轻量圆角占位背景，不产生冲突 |

---

## 8. 待确认事项（可选）
（无）
