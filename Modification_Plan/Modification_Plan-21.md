# 修复 CMake 编译因缺失模型源码引发的 ItemRecord 未声明错误 —— Modification_Plan-21.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
在对项目进行 CMake 构建编译时，发生严重的模板实例化与符号未定义错误，提示 `“ItemRecord”: 未声明的标识符`、`"std::vector": "ItemRecord" 不是参数 "_Ty" 的有效模板类型参数` 等。
该问题发生的背景是：之前重构抽离了 `src/ui/models/` 模型子目录，但由于移除了 GLOB 自动搜集源文件机制后，未在 CMakeLists.txt 的 `SOURCES` 中显式登记这些新模块的 5 个文件，导致构建系统无法对其进行 MOC 及 C++ 预处理和编译。

## 2. 问题定位
- 报错模块：CMake 构建过程。
- 问题根因：
  1. `src/ui/models/ItemModelBase.h`、`src/ui/models/DiskItemModel.h`、`src/ui/models/DiskItemModel.cpp`、`src/ui/models/LibraryAssetModel.h` 以及 `src/ui/models/LibraryAssetModel.cpp` 未在 `CMakeLists.txt` 的 `SOURCES` 列表中列出，导致未被编译。
  2. `CMakeLists.txt` 中的 `target_include_directories` 未包含 `${CMAKE_CURRENT_SOURCE_DIR}/src/ui/models` 路径，当使用相对或绝对路径 `#include "ItemRecord.h"` 包含时，引发未声明标识符错误。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | CMake 编译时提示 ItemRecord 未声明的标识符，如何修复？ | 在 CMakeLists.txt 中补全 src/ui/models/ 目录下的 5 个模型文件到 SOURCES 列表中，并配置其头文件路径 | ✅ |

## 4. 详细解决方案
本部分由执行者 AI 角色直接读取，并严格、机械地按照给出的 Git merge diff 代码块进行物理替换，不得做任何自由发挥或脑补改动。

### 4.1 修改 CMakeLists.txt 包含缺失模型文件
将 5 个模型源文件及头文件加入到 `SOURCES` 列表中，并将 `${CMAKE_CURRENT_SOURCE_DIR}/src/ui/models` 引入到包含路径。

在 `CMakeLists.txt` 文件中：

```
<<<<<<< SEARCH
    src/ui/ToolTipOverlay.cpp
    src/ui/ToolTipOverlay.h
    src/ui/TreeItemDelegate.h
=======
    src/ui/ToolTipOverlay.cpp
    src/ui/ToolTipOverlay.h
    src/ui/TreeItemDelegate.h
    src/ui/models/ItemModelBase.h
    src/ui/models/DiskItemModel.h
    src/ui/models/DiskItemModel.cpp
    src/ui/models/LibraryAssetModel.h
    src/ui/models/LibraryAssetModel.cpp
>>>>>>> REPLACE
```

在 `CMakeLists.txt` 文件的包含路径部分：

```
<<<<<<< SEARCH
target_include_directories(ArcMeta PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/meta
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mft
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crypto
    ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/libtiff
)
=======
target_include_directories(ArcMeta PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/meta
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mft
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crypto
    ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/libtiff
)
>>>>>>> REPLACE
```

## 5. 修改边界声明【范围】
本次方案涉及范围：
- [ ] 模块/文件：`CMakeLists.txt`

明确禁止越界修改的范围：
- [ ] C++ 源文件与头文件（`src/` 目录下的所有 `.cpp`/`.h` 业务文件）——不修改

## 6. 实现准则与预警【核心】
1. 必须确保模型文件的路径拼写完全正确：`src/ui/models/ItemModelBase.h`、`src/ui/models/DiskItemModel.h`、`src/ui/models/DiskItemModel.cpp`、`src/ui/models/LibraryAssetModel.h`、`src/ui/models/LibraryAssetModel.cpp`。
2. 必须加入头文件包含路径：`${CMAKE_CURRENT_SOURCE_DIR}/src/ui/models`，否则在后续 MOC 预处理或某些模块引用这些类时可能由于找不到头文件从而依旧报错。
3. 纯物理构建系统配置更新，严格不包含业务逻辑代码，做到开箱即用，避免脑补产生其他编译警告。

## 7. Memories.md 合规检查
本次修改仅涉及构建配置 `CMakeLists.txt`，未对任何 UI 组件或底层业务模式做修改，也未引入新组件。

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 构建系统配置 | 配置 CMake 依赖和编译选项时，保证所有相关的模型头文件、源文件与依赖都被精确包含入内，保证无遗漏且目录配置正确。 | ✅ |

## 8. 待确认事项（可选）
无。
