# 批量创建功能模块在 CMake 登记注册设计 —— Modification_Plan-52.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
在 `Modification_Plan-51.md` 中，我们为磁盘目录模式独占的“批量创建”设计了专有的非模态对话框 `BatchCreateDialog`（包含 `src/ui/BatchCreateDialog.h` 和 `src/ui/BatchCreateDialog.cpp`）。

根据项目《源码纯净》规范，全应用已彻底废除动态递归搜索（GLOB）的源文件查找方式，全部采取在 `CMakeLists.txt` 的 `SOURCES` 列表中显式登记注册的方式。因此，**任何新增的文件/模块必须在 `CMakeLists.txt` 中进行登记**，否则新增的代码文件不会参与编译，从而导致链接阶段报“无法解析的外部符号”等阻碍性错误。

本方案针对该注册流程提供明确的 CMakeLists.txt 物理修改指令。

## 2. 问题定位
- 修改文件：`CMakeLists.txt` 源码。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 新增模块难道不需要在“CMakeLists.txt”登记注册吗？ (用户原话) | 在 `CMakeLists.txt` 的显式 `SOURCES` 列表中对 `BatchCreateDialog.h/cpp` 进行显式注册 | ✅ |
| 2    | 保证一键编译成功率 100% (我的理解) | 精准对齐 `CMakeLists.txt` 的声明位置，不破坏任何其他的编译选项或库链接 | ✅ |

## 4. 详细解决方案
本部分由执行者 AI 角色直接读取，并严格、机械地按照给出的 Git merge diff 代码块进行物理替换，不得做任何自由发挥或脑补改动。

### 4.1 在 CMakeLists.txt 的 SOURCES 列表中登记新文件
在 `src/ui/BatchRenamePreviewDialog.h` 附近，以字典序（或与相邻 UI 文件的排序对齐）在 `SOURCES` 中登记 `src/ui/BatchCreateDialog.cpp` 与 `src/ui/BatchCreateDialog.h`。

```
<<<<<<< SEARCH
    src/ui/BatchRenameDialog.cpp
    src/ui/BatchRenameDialog.h
    src/ui/BatchRenamePreviewDialog.cpp
    src/ui/BatchRenamePreviewDialog.h
    src/ui/MemoryBatchRenameService.cpp
=======
    src/ui/BatchCreateDialog.cpp
    src/ui/BatchCreateDialog.h
    src/ui/BatchRenameDialog.cpp
    src/ui/BatchRenameDialog.h
    src/ui/BatchRenamePreviewDialog.cpp
    src/ui/BatchRenamePreviewDialog.h
    src/ui/MemoryBatchRenameService.cpp
>>>>>>> REPLACE
```

## 5. 修改边界声明【范围】
**本次方案涉及范围：**
- `CMakeLists.txt` —— 物理增补

**明确禁止越界修改的范围：**
- 所有的 `LIBTIFF_SOURCES` 依赖列表 —— 不修改
- 编译选项 (add_compile_options) —— 不修改

## 6. 实现准则与预警【核心】
1. **彻底规避编译中断**：在登记 `BatchCreateDialog.cpp` 时，必须确保其对应的 `.h` 头文件一同被写入 `SOURCES`，这是 Qt 的 MOC 编译器（AUTOMOC）能够自动扫描、识别和生成对应的 `moc_BatchCreateDialog.cpp` 的必备前提。
2. **遵守源码纯净和零残留原则**：CMake 文件的修改应保持行内对齐，避免因换行缩进不当留下语法地雷。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| CMake 构建规则 | 新增组件必须在构建列表中显式注册，彻底杜绝 GLOB 搜集，确保一键编译无警告。 | 符合。完成了 CMake 列表中对 `BatchCreateDialog` 的显式登记。 |

## 8. 待确认事项（可选）
无。
