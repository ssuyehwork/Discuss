# Implementation Plan - ItemRecord-2

## 1. Overview（概述与解决的问题）
本实施方案用于修复在 `src/core/ItemRecord.cpp` 中下沉 `ThumbnailState` 判定时调用 `ColorPaletteEngine::isGraphicsFile` 导致 MSVC 报 C2653（非类或命名空间名称）和 C3861（找不到标识符）编译错误。通过在 `ItemRecord.cpp` 物理包含头文件 `#include "../util/ColorPaletteEngine.h"` 闭合头文件依赖链。

---

## 2. Modified Files List（影响文件清单）
- `src/core/ItemRecord.cpp`

---

## 3. Detailed Line-by-Line Changes（包含 CMakeLists.txt 在内的精准替换块）

### `src/core/ItemRecord.cpp`
```cpp
<<<<<<< SEARCH
#include "ItemRecord.h"
#include "../meta/MetadataManager.h"
#include <QFileInfo>
=======
#include "ItemRecord.h"
#include "../meta/MetadataManager.h"
#include "../util/ColorPaletteEngine.h"
#include <QFileInfo>
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）
1. 编译 `src/core/ItemRecord.cpp`，确认 C2653 与 C3861 错误完全消除。
2. 确认 `ColorPaletteEngine::isGraphicsFile` 在 `ItemRecord::create` 与 `fromMetadata` 中正常通过类型与命名空间解析。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）
- 正确使用物理头文件包含，闭合依赖链。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用的成员函数 / 枚举 | 所在的头文件 | 精准物理签名 | 核查状态 |
| :--- | :--- | :--- | :--- |
| `ColorPaletteEngine::isGraphicsFile` | `src/util/ColorPaletteEngine.h` | `static bool isGraphicsFile(const QString& ext);` |  物理核实通过 |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

- `src/core/ItemRecord.cpp`:
  - 补充显式 `#include "../util/ColorPaletteEngine.h"`，闭合包含链。
