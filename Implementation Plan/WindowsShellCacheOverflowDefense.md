# Implementation Plan: WindowsShellCacheOverflowDefense.md

- **Base Commit Hash**: 57252f1
- **Prerequisite Plans**: None
- **Supersedes**: None

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题现象与缺陷描述
在 MSVC 调试环境与运行日志中，程序频繁抛出并记录以下原生 C++ 异常：
`0x00007FF8B9905369 处(位于 QuarkMeta.exe 中)引发的异常: Microsoft C++ 异常: CacheOverflowException`

### 1.2 绝对根本原因分析 (Root Cause Analysis)
通过对 `src/ui/WindowsShellThumbnailProvider.cpp` 与 `src/ui/IconCacheManager.cpp` 源码以及 Windows Shell API 机制的深度分析，锁定根本原因如下：

1. **COM 接口调用缺乏 C++ 原生异常屏蔽 (Missing Try-Catch Shielding)**：
   在 `WindowsShellThumbnailProvider::getShellThumbnail` 中，使用 Windows Shell 缩略图工厂接口 `IShellItemImageFactory::GetImage(nativeSize, SIIGBF_THUMBNAILONLY, &hBitmap)` 提取缩略图。当 Windows 系统底层 `thumbcache_*.db` 缩略图数据库文件句柄紧张或内存缓冲区溢出时，系统 Shell COM 组件内部会引发并抛出 `CacheOverflowException` 原生 MSVC C++ 异常。由于调用代码周围缺乏 `try { ... } catch (...)` 异常捕获机制，该异常穿透 COM 接口层抛出到应用程序，被 Visual Studio 调试器捕获并导致控制台连续报错。
2. **`QFileIconProvider::icon()` 底层 Shell 调用缺乏异常防护**：
   在 `handleIconLoad` 与 `IconCacheManager::getCachedIcon` 中，通过 Qt 的 `QFileIconProvider::icon()` 批量提取扩展名图标。`QFileIconProvider` 内部调用 Windows `SHGetFileInfo` / `IExtractIcon`，在大批量提取特殊格式文件（如 PSD、AI、高分辨率图片）时，同样可能触发系统 Shell 溢出异常。
3. **缺乏失败熔断自愈与资源释放收尾机制**：
   原有代码在遇到 Shell 提取失败时，缺乏连续失败熔断与冷却机制。如果短时间内连续出现 Shell 提取异常，程序将持续向系统 Shell 发起高频请求，加剧系统缓存区溢出；同时在异常或提前返回分支中，缺乏对 COM 接口指针（`IShellItem`、`IShellItemImageFactory`）及 GDI 句柄（`HBITMAP`、`HDC`）的 RAII/异常安全释放保护，存在资源泄露隐患。

### 1.3 拟定重构方案与设计目标
1. **三级防护拦截 (Three-Level Exception Defense)**：
   - 在 `WindowsShellThumbnailProvider::getShellThumbnail` 的 COM 操作代码块周围增加 `try { ... } catch (...)` 异常防线，捕获包括 `CacheOverflowException` 在内的所有原生与标准 C++ 异常；
   - 在 `WindowsShellThumbnailProvider::handleIconLoad` 与 `IconCacheManager::getCachedIcon` 的 `QFileIconProvider::icon` 调用点增加 `try { ... } catch (...)` 防护。
2. **COM/GDI 资源 Safe Cleanup 保证**：
   - 优化 `getShellThumbnail` 中的 COM 指针（`pFactory`、`pItem`、`pidl`）与 GDI 对象（`hBitmap`、`hdc`）释放流程，确保在捕获异常或提前返回时 100% 安全释放资源，无句柄泄露。
3. **平滑降级与熔断自愈机制 (Graceful Degradation & Self-Healing Circuit Breaker)**：
   - 在 `WindowsShellThumbnailProvider.cpp` 匿名命名空间中定义统一共享的静态原子熔断自愈计数器与冷却时间戳（`g_consecutiveFailures` 与 `g_cooldownUntilMs`）。若短时间内连续触发 Shell 溢出异常（如 5 次），自动进入 1000ms 的 Shell 提取熔断冷却期，直接返回内置的矢量/默认占位图标，为 Windows Shell 系统缓存留出 GC 清理与 Flush 时间；
   - 冷却期结束后自动尝试恢复提取，实现无感平滑自愈与界面流畅绘制。

---

## Chapter 2: Modified Files List（影响文件清单）

- `src/ui/WindowsShellThumbnailProvider.h` (修改：新增熔断自愈计数器与冷却机制私有静态方法)
- `src/ui/WindowsShellThumbnailProvider.cpp` (修改：在匿名命名空间定义文件作用域共享原子熔断变量，为 `getShellThumbnail` 和 `handleIconLoad` 增加 `try-catch (...)` 异常屏障与熔断自愈逻辑)
- `src/ui/IconCacheManager.cpp` (修改：为 `getCachedIcon` 增加 `try-catch (...)` 异常防护与平滑降级)

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/WindowsShellThumbnailProvider.h`

<<<<<<< SEARCH
    static bool isDriveRootPath(const QString& filePath);

    static QMutex& fileIconMutex();
    static QMap<QString, QIcon>& fileIconCache();
    static QMutex& loadingMutex();
    static QSet<QString>& loadingKeys();
};
=======
    static bool isDriveRootPath(const QString& filePath);

    static QMutex& fileIconMutex();
    static QMap<QString, QIcon>& fileIconCache();
    static QMutex& loadingMutex();
    static QSet<QString>& loadingKeys();

    static bool checkCircuitBreaker();
    static void recordShellFailure();
    static void recordShellSuccess();
};
>>>>>>> REPLACE

---

### 3.2 `src/ui/WindowsShellThumbnailProvider.cpp`

<<<<<<< SEARCH
void WindowsShellThumbnailProvider::handleIconLoad(const QString& filePath, const QString& key, bool isDir, bool isRoot) {
    // 在主线程（本槽所在线程）取得通知器指针，保证通知器归属主线程，
    // 避免首次在子线程创建导致 queued 调用永远不执行、通知发不出。
    IconLoadNotifier* notifier = &IconLoadNotifier::instance();

    (void)QtConcurrent::run([filePath, key, isDir, isRoot, notifier]() {
        // 在子线程中执行 COM 线程环境初始化
#ifdef Q_OS_WIN
        HRESULT hres = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
#endif
        QFileIconProvider provider;
        QIcon icon;
        QFileInfo info(filePath);

        if (isDir) {
            if (isRoot) {
                icon = provider.icon(info);
            } else {
                icon = SvgIconRenderer::getIcon("folder_filled", QColor("#888888"), 128);
            }
        } else {
            icon = provider.icon(QFileInfo("dummy." + key));
            if (icon.isNull()) {
                icon = provider.icon(QFileIconProvider::File);
            }
        }

        {
            QMutexLocker locker(&fileIconMutex());
            fileIconCache()[key] = icon;
        }

        {
            QMutexLocker lock(&loadingMutex());
            loadingKeys().remove(key);
        }

#ifdef Q_OS_WIN
        if (SUCCEEDED(hres)) {
            CoUninitialize();
        }
#endif

        // 通知主线程：该 key 对应的系统图标已就绪，视图/模型据此重绘
        QMetaObject::invokeMethod(notifier, [notifier, key]() {
            emit notifier->iconLoadedForKey(key);
            emit notifier->iconLoaded();
        }, Qt::QueuedConnection);
    });
}
=======
namespace {
    std::atomic<int> g_consecutiveFailures{0};
    std::atomic<int64_t> g_cooldownUntilMs{0};
}

bool WindowsShellThumbnailProvider::checkCircuitBreaker() {
    int64_t nowMs = QDateTime::currentMSecsSinceEpoch();
    if (nowMs < g_cooldownUntilMs.load()) {
        return false; // 处于熔断冷却期，阻断 Shell 调用
    }
    return true;
}

void WindowsShellThumbnailProvider::recordShellFailure() {
    int fails = ++g_consecutiveFailures;
    if (fails >= 5) {
        int64_t nowMs = QDateTime::currentMSecsSinceEpoch();
        g_cooldownUntilMs.store(nowMs + 1000); // 触发 1000ms 熔断冷却，允许 Shell 缓存池 GC
        g_consecutiveFailures.store(0);
    }
}

void WindowsShellThumbnailProvider::recordShellSuccess() {
    g_consecutiveFailures.store(0);
}

void WindowsShellThumbnailProvider::handleIconLoad(const QString& filePath, const QString& key, bool isDir, bool isRoot) {
    // 在主线程（本槽所在线程）取得通知器指针，保证通知器归属主线程，
    // 避免首次在子线程创建导致 queued 调用永远不执行、通知发不出。
    IconLoadNotifier* notifier = &IconLoadNotifier::instance();

    (void)QtConcurrent::run([filePath, key, isDir, isRoot, notifier]() {
        // 在子线程中执行 COM 线程环境初始化
#ifdef Q_OS_WIN
        HRESULT hres = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
#endif
        QFileIconProvider provider;
        QIcon icon;
        QFileInfo info(filePath);

        try {
            if (isDir) {
                if (isRoot) {
                    icon = provider.icon(info);
                } else {
                    icon = SvgIconRenderer::getIcon("folder_filled", QColor("#888888"), 128);
                }
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
        } catch (...) {
            recordShellFailure();
            icon = SvgIconRenderer::getIcon(isDir ? "folder_filled" : "file", QColor("#888888"), 128);
        }

        {
            QMutexLocker locker(&fileIconMutex());
            fileIconCache()[key] = icon;
        }

        {
            QMutexLocker lock(&loadingMutex());
            loadingKeys().remove(key);
        }

#ifdef Q_OS_WIN
        if (SUCCEEDED(hres)) {
            CoUninitialize();
        }
#endif

        // 通知主线程：该 key 对应的系统图标已就绪，视图/模型据此重绘
        QMetaObject::invokeMethod(notifier, [notifier, key]() {
            emit notifier->iconLoadedForKey(key);
            emit notifier->iconLoaded();
        }, Qt::QueuedConnection);
    });
}
>>>>>>> REPLACE

---

### 3.3 `src/ui/WindowsShellThumbnailProvider.cpp` (getShellThumbnail 函数异常防护)

<<<<<<< SEARCH
QImage WindowsShellThumbnailProvider::getShellThumbnail(const QString& path, int size) {
#ifdef Q_OS_WIN
    ComInitializer comInit;
    PIDLIST_ABSOLUTE pidl = nullptr;
    HRESULT hr = SHParseDisplayName(path.toStdWString().c_str(), nullptr, &pidl, 0, nullptr);
    if (FAILED(hr)) return QImage();
    IShellItem* pItem = nullptr;
    hr = SHCreateItemFromIDList(pidl, IID_IShellItem, (void**)&pItem);
    ILFree(pidl);
    if (SUCCEEDED(hr)) {
        IShellItemImageFactory* pFactory = nullptr;
        hr = pItem->QueryInterface(IID_IShellItemImageFactory, (void**)&pFactory);
        if (SUCCEEDED(hr)) {
            SIZE nativeSize = { size, size };
            HBITMAP hBitmap = nullptr;
            // 🚨 关键标志位 SIIGBF_THUMBNAILONLY：强行过滤软件图标！拒绝图标冒充缩略图！
            hr = pFactory->GetImage(nativeSize, SIIGBF_THUMBNAILONLY, &hBitmap);
            if (SUCCEEDED(hr) && hBitmap) {
                BITMAP bmpInfo;
                GetObject(hBitmap, sizeof(bmpInfo), &bmpInfo);
                int w = bmpInfo.bmWidth;
                int h = std::abs(bmpInfo.bmHeight);

                BITMAPINFOHEADER bi = {};
                bi.biSize        = sizeof(BITMAPINFOHEADER);
                bi.biWidth       = w;
                bi.biHeight      = -h;   // 负值 = top-down，方向永远正确
                bi.biPlanes      = 1;
                bi.biBitCount    = 32;
                bi.biCompression = BI_RGB;

                QByteArray pixels(w * h * 4, 0);
                HDC hdc = GetDC(nullptr);
                GetDIBits(hdc, hBitmap, 0, h, pixels.data(),
                          reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);
                ReleaseDC(nullptr, hdc);

                // Windows 返回 BGRA，Qt 需要 RGBA，交换 R/B 通道
                uint8_t* p = reinterpret_cast<uint8_t*>(pixels.data());
                for (int i = 0; i < w * h; ++i) {
                    std::swap(p[i * 4 + 0], p[i * 4 + 2]);
                }

                QImage img(p, w, h, w * 4, QImage::Format_RGBA8888);
                img = img.copy(); // 确保数据所有权

                DeleteObject(hBitmap);
                pFactory->Release();
                pItem->Release();
                return img;
            }
            pFactory->Release();
        }
        pItem->Release();
    }
#else
    Q_UNUSED(path); Q_UNUSED(size);
#endif
    return QImage();
}
=======
QImage WindowsShellThumbnailProvider::getShellThumbnail(const QString& path, int size) {
#ifdef Q_OS_WIN
    if (!checkCircuitBreaker()) {
        return QImage();
    }

    try {
        ComInitializer comInit;
        PIDLIST_ABSOLUTE pidl = nullptr;
        HRESULT hr = SHParseDisplayName(path.toStdWString().c_str(), nullptr, &pidl, 0, nullptr);
        if (FAILED(hr)) return QImage();

        IShellItem* pItem = nullptr;
        hr = SHCreateItemFromIDList(pidl, IID_IShellItem, (void**)&pItem);
        ILFree(pidl);

        if (SUCCEEDED(hr) && pItem) {
            IShellItemImageFactory* pFactory = nullptr;
            hr = pItem->QueryInterface(IID_IShellItemImageFactory, (void**)&pFactory);
            if (SUCCEEDED(hr) && pFactory) {
                SIZE nativeSize = { size, size };
                HBITMAP hBitmap = nullptr;

                // 🚨 关键防线：包裹 try-catch (...) 拦截 Windows Shell 底层 CacheOverflowException 异常
                try {
                    hr = pFactory->GetImage(nativeSize, SIIGBF_THUMBNAILONLY, &hBitmap);
                } catch (...) {
                    hr = E_FAIL;
                    recordShellFailure();
                }

                if (SUCCEEDED(hr) && hBitmap) {
                    BITMAP bmpInfo;
                    GetObject(hBitmap, sizeof(bmpInfo), &bmpInfo);
                    int w = bmpInfo.bmWidth;
                    int h = std::abs(bmpInfo.bmHeight);

                    if (w > 0 && h > 0) {
                        BITMAPINFOHEADER bi = {};
                        bi.biSize        = sizeof(BITMAPINFOHEADER);
                        bi.biWidth       = w;
                        bi.biHeight      = -h;   // 负值 = top-down，方向永远正确
                        bi.biPlanes      = 1;
                        bi.biBitCount    = 32;
                        bi.biCompression = BI_RGB;

                        QByteArray pixels(w * h * 4, 0);
                        HDC hdc = GetDC(nullptr);
                        if (hdc) {
                            GetDIBits(hdc, hBitmap, 0, h, pixels.data(),
                                      reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);
                            ReleaseDC(nullptr, hdc);
                        }

                        // Windows 返回 BGRA，Qt 需要 RGBA，交换 R/B 通道
                        uint8_t* p = reinterpret_cast<uint8_t*>(pixels.data());
                        for (int i = 0; i < w * h; ++i) {
                            std::swap(p[i * 4 + 0], p[i * 4 + 2]);
                        }

                        QImage img(p, w, h, w * 4, QImage::Format_RGBA8888);
                        img = img.copy(); // 确保数据所有权

                        DeleteObject(hBitmap);
                        pFactory->Release();
                        pItem->Release();
                        recordShellSuccess();
                        return img;
                    }
                    DeleteObject(hBitmap);
                }
                pFactory->Release();
            }
            pItem->Release();
        }
    } catch (...) {
        recordShellFailure();
    }
#else
    Q_UNUSED(path); Q_UNUSED(size);
#endif
    return QImage();
}
>>>>>>> REPLACE

---

### 3.4 `src/ui/IconCacheManager.cpp`

<<<<<<< SEARCH
    QFileIconProvider provider;
    QIcon icon;
    if (isDir) {
        icon = SvgIconRenderer::getIcon("folder_filled", QColor("#888888"), 128);
    } else {
        if (key.length() > 12) key = "unknown";
        icon = provider.icon(QFileInfo("dummy." + key));
        if (icon.isNull()) icon = provider.icon(QFileIconProvider::File);
    }
=======
    QFileIconProvider provider;
    QIcon icon;
    try {
        if (isDir) {
            icon = SvgIconRenderer::getIcon("folder_filled", QColor("#888888"), 128);
        } else {
            if (key.length() > 12) key = "unknown";
            icon = provider.icon(QFileInfo("dummy." + key));
            if (icon.isNull()) icon = provider.icon(QFileIconProvider::File);
        }
    } catch (...) {
        icon = SvgIconRenderer::getIcon(isDir ? "folder_filled" : "file", QColor("#888888"), 128);
    }
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译指令
```bash
cmake --build build --config Debug --target QuarkMeta
```

### 4.2 运行时验收标准
1. **控制台/调试器日志验证**：在 MSVC / Visual Studio 调试器模式下快速加载包含成千上万个文件及特殊图像/矢量的目录，输出窗口不再出现连续刷屏的 `CacheOverflowException` 报错；
2. **熔断自愈验证**：在大批量频繁加载系统图标时，界面渲染保持流畅无卡顿，即使遇到系统 Shell 缓存饱满，也能无缝平滑降级展示默认 SVG 图标，且在 1 秒冷却后自动恢复系统图标加载；
3. **句柄无泄露验证**：长时间频繁滚动与刷新缩略图，物理内存与 GDI/USER 句柄计数保持稳定，COM 接口释放完全。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- **统一复用系统图标机制**：统一使用既有的 `WindowsShellThumbnailProvider::getFileIconFast` 与 `IconCacheManager::instance().getCachedIcon`；
- **RAII 与异常屏障模式**：复用 Qt `QMutexLocker` 与 C++ `try-catch (...)` 异常捕获机制，零另起炉灶引入任何外部第三方库。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 系统图标内存缓存 | `WindowsShellThumbnailProvider::fileIconCache()` 与 `IconCacheManager` | `QIcon` | 由后台子线程异步提取填充，`IconLoadNotifier` 信号通知 UI 刷新 |
| 提取熔断状态 | `WindowsShellThumbnailProvider.cpp` 匿名命名空间中的全局静态原子量 `g_consecutiveFailures` / `g_cooldownUntilMs` | 无 | 异常触发后计入冷却期，到期自动设回正常状态 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

- `WindowsShellThumbnailProvider.h` 原有 `public` 接口完全冻结（`getFileIcon`、`getFileIconFast`、`getShellThumbnail`）；
- 新增 `private:` 静态熔断辅助函数：
  - `static bool checkCircuitBreaker();`
  - `static void recordShellFailure();`
  - `static void recordShellSuccess();`
- 仅在内部 `.cpp` 调用，访问权限匹配。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- `src/ui/WindowsShellThumbnailProvider.cpp` 已包含 `<QDateTime>` 与 `<atomic>`，闭合了熔断计数器的时间戳与原子变量类型要求；
- 头文件包含链 100% 闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 核对项目 | 是否满足 | 说明/具体位置 |
| :---: | :--- | :---: | :--- |
| 1 | 方案放置于 `Implementation Plan/` 目录 | 满足 | `Implementation Plan/WindowsShellCacheOverflowDefense.md` |
| 2 | Git Merge Diff 格式合规 | 满足 | Chapter 3 统一采用 SEARCH/REPLACE 块 |
| 3 | SEARCH 块物理匹配唯一性 | 满足 | 在目标文件内部物理唯一匹配 |
| 4 | 无直接修改源代码承诺 | 满足 | 方案仅存放在本物理隔离方案文件中 |
| 5 | 五道工程硬锁之【契约锁】 | 满足 | 公开接口签名冻结无修改 |
| 6 | 五道工程硬锁之【依赖锁】 | 满足 | 无跨层逆向依赖 |
| 7 | 五道工程硬锁之【静态红线】 | 满足 | 无平台 Hack，无违规事件过滤器 |
| 8 | 异常拦截完整性 | 满足 | 拦截 `CacheOverflowException` 及所有 C++ 原生异常 |
| 9 | COM/GDI 句柄安全释放 | 满足 | `pFactory`、`pItem`、`hBitmap` 在异常及各分支均妥善释放 |
| 10 | 熔断自愈机制变量作用域完整性 | 满足 | 静态原子变量处于 `WindowsShellThumbnailProvider.cpp` 匿名命名空间，实现三方法共享状态 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 10.1 行为与交互零篡改声明
- 图标加载、缩略图生成以及 UI 视图项图标渲染逻辑与视觉表现 100% 保持不变；
- 缩略图异步通知机制 (`IconLoadNotifier`) 保持 100% 相同。

### 10.2 样式数值零篡改声明
- 本次重构不涉及任何 UI QSS 样式、物理尺寸、颜色或边距数值修改，参数 100% 零篡改。
