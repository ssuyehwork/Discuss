# QuickLook 双击空白预览 —— 根因与待修方案（线程池竞争）

## 0. 范围声明（红线）

本次只解决"QuickLook 预览任务与批量缩略图提取任务抢占同一个全局线程池"这一个问题。**不涉及**上一份《待修清单》里已经修复的两处（缩略图缓存读写加锁、默认缩放模式），也不涉及 `MediaExtractorPipeline`/`DiskMediaExtractor` 内部其他任何逻辑的调整。

---

## 1. 根因（已用现有代码验证，不是猜测）

**现状**：`QuickLookWindow::renderImage()` 用 `QtConcurrent::run(...)` 提交预览加载任务，`MediaExtractorPipeline::dispatchWorkersIfNeeded()` 也用 `QtConcurrent::run(...)` 提交批量缩略图提取的 worker 循环——**两者共用同一个 `QThreadPool::globalInstance()`**，且全仓库没有任何地方调整过这个全局池的最大线程数，用的是 Qt 默认值（约等于 CPU 核心数）。

`MediaExtractorPipeline` 的 worker（`dispatchWorkerLoop`）是**长时间占用线程的循环**，只要队列没空就不会把线程还给池子。文件夹一打开，`registerItemsAsync` 会立刻把这批文件全部投给 `MediaExtractorPipeline::enqueueBatch()`，瞬间把全局池的线程拉满去跑批量提取。这时候提交的 QuickLook 预览任务，不管是哪种图片格式，都要排在这些长任务后面等空位——文件夹越大、提取任务越多，等待时间越长，表现为预览一直空白/卡在加载中。**这个瓶颈发生在图片格式判断之前，所以任何格式都会中招，跟具体解码逻辑无关。**

---

## 2. 修复方向：给 QuickLook 一个独立的专属线程池

不动 `MediaExtractorPipeline` 的调度逻辑（批量提取该怎么占线程还怎么占，那是它的正常工作方式，不是 bug），而是让 QuickLook 的预览任务**不再跟它抢同一个池子**。

### 2.1 `QuickLookWindow.h` 新增成员

```cpp
#include <QThreadPool>
// ...
private:
    QThreadPool m_previewThreadPool; // 专属线程池，只服务预览加载，不与批量提取共享
```

构造函数里初始化线程数（预览本来就该优先响应，1-2 个线程足够，不需要跟核心数挂钩）：
```cpp
m_previewThreadPool.setMaxThreadCount(2);
```

### 2.2 `QuickLookWindow.cpp` 里 `renderImage()` 的提交方式改动

**现状代码**（第 207 行附近）：
```cpp
(void)QtConcurrent::run([weakThis, path, ext]() {
    ...
});
```

**改为**（用带线程池参数的重载，逻辑体本身一个字不动，只改提交方式）：
```cpp
(void)QtConcurrent::run(&m_previewThreadPool, [weakThis, path, ext]() {
    ...
});
```

**红线**：lambda 内部的具体加载逻辑（SVG/AI/原生格式判断、回调里的过期检查 `m_currentPath != path`）**一律不动**，这次只改任务提交去了哪个线程池，不改任务本身做了什么。

---

## 3. 需要一并确认的点（不属于本次改动，但要留意）

`renderText()` 是否也用 `QtConcurrent::run` 提交到全局池——如果是，同样应该改到这个专属池里，否则文本文件预览会遗留同样的问题。**这一点需要看 `renderText()` 的具体实现才能确认要不要一起改**，如果你要我确认，把 `renderText` 那段代码位置告诉我，或者直接说"连 renderText 一起改"，我按同样的方式补一条。

---

## 4. 验证方法

1. 找一个几百张图、缩略图还没提取完的文件夹，**打开的瞬间立刻双击**其中一项，反复测试，确认不再出现空白/卡在加载中。
2. 打开同一个文件夹，等批量提取彻底跑完之后再双击，确认行为与之前一致（不应该因为这次改动引入新问题）。
3. 确认批量提取本身的速度没有因为"少了两个线程"而受到影响——`setMaxThreadCount(2)` 是给预览专用池的，不会占用批量提取那边的线程数，两个池子是完全独立的资源，理论上互不影响，但建议实际跑一次大文件夹确认一下。
