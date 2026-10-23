# 阶段四实施方案：物理彻底删除 DualSectionPanel 与 CMake 构建瘦身

## 1. Overview（概述与解决的问题）

### 目标与背景
本方案为“四视图单滚动条架构（放弃双子视图）”重构的**阶段四（终局清场）**实施方案。
核心目标：
1. 在前三阶段完成列表视图、网格视图、瀑布流视图与分栏视图全部单视图化并全面解耦 `DualSectionPanel` 后，执行最终清场操作；
2. 物理彻底删除废弃的旧组件文件 `src/ui/DualSectionPanel.h` 与 `src/ui/DualSectionPanel.cpp`；
3. 更新根目录 `CMakeLists.txt`，移除 `DualSectionPanel` 相关的 `SOURCES` 与 `HEADERS` 编译目标及 MOC 注册，消除死代码隐患与构建冗余。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/DualSectionPanel.h`（物理彻底删除 🗑️）
2. `src/ui/DualSectionPanel.cpp`（物理彻底删除 🗑️）
3. `CMakeLists.txt`（移除 `DualSectionPanel` 构建注册）

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `CMakeLists.txt` 精准替换

```cmake
<<<<<<< SEARCH
    src/ui/DualSectionPanel.cpp
    src/ui/DualSectionPanel.h
=======
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译与验证方法）

1. **清除旧构建缓存并重新生成 CMake 项目**：
   ```bash
   cmake -B build -S .
   ```
2. **全工程编译测试**：
   ```bash
   cmake --build build --config Release
   ```
3. **验证结果**：
   - 确认无任何找不到 `DualSectionPanel.h` 的头文件包含错误或 MOC 符号未定义链接错误；
   - 确认全软件在零 `DualSectionPanel` 依赖下编译成功并流畅运行。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用自查）

- **物理死代码彻底清除**：完全符合《AGENTS.md》2.4 节“物理彻底清除死代码契约”，无任何旧分支保留；
- **构建瘦身**：移除废弃类对 MOC 引擎和编译器符号表的开销。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

*注：本阶段为纯物理删除与 CMake 清理阶段，不引入任何新 API 接口。*

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链核查表）

| 受影响文件 | 修改/新增 `#include` | 类型与枚举完整性核查 |
| :--- | :--- | :--- |
| `CMakeLists.txt` | 移除 `DualSectionPanel.h/.cpp` 注册行 | 构建链完整，无残余符号 |
