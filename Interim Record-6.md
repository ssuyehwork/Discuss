# 全工程符号依赖比对矩阵与死枚举图谱记录 (Interim Record-6.md)

## 一、 比对矩阵概况
本记录通过自动化符号矩阵图谱分析引擎（`audit_matrix.py`），对 `ModelContract.h` 定义的 22 个全局 CommonRole 枚举值在全工程 203 个文件中的交叉引用频次与使用分布进行了全量物理比对。

---

## 二、 符号引用频次比对矩阵与死枚举事实

| 枚举名称 (CommonRole) | 关联引用文件数 | 在全工程中的使用状态 |
|---|---|---|
| `TypeRole` | 8 个文件 | 正常使用 |
| **`IdRole`** | 2 个文件 (`ModelContract.h`, `ContentPanel.cpp`) | **死枚举 / 残留** |
| **`NameRole`** | 1 个文件 (`ModelContract.h`) | **零引用孤立死枚举** |
| `PathRole` | 9 个文件 | 正常使用 |
| `ColorRole` | 7 个文件 | 正常使用 |
| `RatingRole` | 7 个文件 | 正常使用 |
| `TagsRole` | 4 个文件 | 正常使用 |
| `PinnedRole` | 5 个文件 | 正常使用 |
| `IsLockedRole` | 5 个文件 | 正常使用 |
| `EncryptedRole` | 3 个文件 | 正常使用 |
| **`EncryptHintRole`** | 1 个文件 (`ModelContract.h`) | **零引用孤立死枚举** |
| `ManagedRole` | 6 个文件 | 僵尸 Role 分支 |
| `IsEmptyRole` | 5 个文件 | 正常使用 |
| `RegistrationProgressRole` | 6 个文件 | 废弃 Role |
| `AspectRatioRole` | 5 个文件 | 正常使用 |
| `HasThumbnailRole` | 6 个文件 | 正常使用 |
| **`PalettesRole`** | 1 个文件 (`ModelContract.h`) | **零引用孤立死枚举** |
| **`CountRole`** | 1 个文件 (`ModelContract.h`) | **零引用孤立死枚举** |
| `IsGroupHeaderRole` | 5 个文件 | 废弃 Role |
| **`GroupNameRole`** | 1 个文件 (`ModelContract.h`) | **零引用孤立死枚举** |
| `IsDiskTrashRole` | 2 个文件 | 正常使用 |
| `DiskTrashIdRole` | 2 个文件 | 正常使用 |

---

## 三、 比对结论
符号矩阵图谱比对精确表明：`ModelContract.h` 中定义的 `NameRole`、`EncryptHintRole`、`PalettesRole`、`CountRole`、`GroupNameRole` 五个 Role **在全工程 203 个源码文件中只有头文件本身 1 处定义，全局引用数为 0**，属于完全悬空的孤立死枚举；`IdRole` 也仅在已废弃的 `ContentPanel.cpp` 分支中存在残留。
