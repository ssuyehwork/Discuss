# ContentViewCoordinator-1 Implementation Plan

## 1. Overview
修复 `src/ui/controllers/ContentViewCoordinator.cpp` 编译时引发的 MSVC C2039（`refreshVisibleThumbnails` 不是 `ContentViewCoordinator` 的成员）、C2065（`m_panel` 未声明）以及 C3861（`currentActiveViews` 找不到标识符）错误。
根因是在 `src/ui/controllers/ContentViewCoordinator.h` 中漏掉了 `void refreshVisibleThumbnails();` 物理成员函数声明，导致 MSVC 在编译 `.cpp` 时无法将 `refreshVisibleThumbnails()` 识别为类成员函数，进而引发连锁的未声明标识符报错。

## 2. Modified Files List
- `src/ui/controllers/ContentViewCoordinator.h`

## 3. Detailed Line-by-Line Changes

```
<<<<<<< SEARCH
    // 缩略图视口行号探测与触发

    // 分区高度与统计同步（原样移植，零数值变动）
=======
    // 缩略图视口行号探测与触发
    void refreshVisibleThumbnails();

    // 分区高度与统计同步（原样移植，零数值变动）
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. 执行 CMake 构建验证 MSVC 编译：
   `cmake --build QuarkMeta_Build --config Release`
2. 确认 `ContentViewCoordinator.cpp` 编译通过，C2039、C2065、C3861 错误彻底消失。

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 本方案仅补齐头文件中遗漏的成员函数物理声明，无任何冗余逻辑。

## 6. Header API Signature Verification
- `void refreshVisibleThumbnails();`
  - 属于 `ContentViewCoordinator` 的公有成员函数，与 `.cpp` 中第 191 行的函数物理签名 100% 保持一致。

## 7. Header Inclusion Chain & Type Completeness Check
- 所在文件：`src/ui/controllers/ContentViewCoordinator.h`
- 引入头文件已具备 `<QObject>`、`<QAbstractItemView>` 等完全定义，补充声明后物理闭合。
