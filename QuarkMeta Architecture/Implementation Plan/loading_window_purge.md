# 彻底清退 LoadingWindow 加载窗口实施方案 (Purge LoadingWindow Implementation Plan)

## 1. Overview（概述与解决的问题）

本实施方案旨在彻底清除已移除的 `LoadingWindow` 加载窗口在主入口 `src/main.cpp` 中的所有残留代码与头文件引用，消除 `未声明的标识符: loadingWin` / `C1083 无法打开包括文件: LoadingWindow.h` 编译报错，恢复主程序的纯粹快速启动：
1. **彻底解耦 `main.cpp`**：移除 `#include "ui/LoadingWindow.h"` 以及 `loadingWin` 对象的实例化、`updateStatus` 状态更新和 `onInitializationFinished` 回调调用。
2. **清理 CMake 构建列表**：确保 `CMakeLists.txt` 中不包含 `LoadingWindow.cpp` 和 `LoadingWindow.h`。

---

## 2. Modified Files List（影响文件清单）

1. `src/main.cpp`
2. `CMakeLists.txt`
3. `QuarkMeta Architecture/QuarkMeta-Architecture-Planning.md`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/main.cpp`
从 `main.cpp` 中彻底抹除 `LoadingWindow.h` 引用及 `loadingWin` 实例。

```
<<<<<<< SEARCH
#include "ui/LoadingWindow.h"
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 显示启动加载窗口
    QuarkMeta::LoadingWindow loadingWin;
    loadingWin.show();
    loadingWin.updateStatus("正在初始化系统核心组件...");
    qApp->processEvents();

    // -------------------------------------------------------------
    // 重构 5：多段启动。MainWindow 放置于栈上局部作用域，利用 RAII 自动且安全析构，规避 Double Free
    // -------------------------------------------------------------
    QuarkMeta::MainWindow w;
    loadingWin.onInitializationFinished();
=======
    // -------------------------------------------------------------
    // 重构 5：多段启动。MainWindow 放置于栈上局部作用域，利用 RAII 自动且安全析构，规避 Double Free
    // -------------------------------------------------------------
    QuarkMeta::MainWindow w;
>>>>>>> REPLACE
```

---

### 3.2 `CMakeLists.txt`
确保 `CMakeLists.txt` 中不包含 `LoadingWindow` 编译项。

```
<<<<<<< SEARCH
    src/ui/LoadingWindow.cpp
    src/ui/LoadingWindow.h
=======
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译确认**：
   在命令行运行 CMake 编译，验证彻底移除 `LoadingWindow` 之后 `main.cpp` 无任何未声明标识符编译报错：
   ```bash
   cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```
2. **功能验证**：
   启动程序，确认 `MainWindow` 快速且直接地实例化展开，无任何卡顿或头文件加载异常。
