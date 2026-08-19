# 批量重命名保留用户大小写与 NTFS 大小写改名修正无脑实施方案 —— BatchRenamePreserveCaseFix

本实施方案旨在解决批量重命名过程中“用户输入大写英文、最终输出却被强转为小写”的严重缺陷，确保用户输入的任何大小写字符串（如 `VIEW_`、`.EPS`、`.AI`）及原文件大小写格式得到 100% 精确保留，并解决 Windows NTFS 文件系统对于纯大小写改名不敏感的底层问题。

---

## 修改文件清单

1. `src/core/ItemRecord.cpp`
2. `src/core/IndexedEntry.h`
3. `src/ui/DiskBatchRenameService.cpp`
4. `src/meta/BatchRenameEngine.cpp`

---

## 阶段一：彻底移除后缀名 `.toLower()` 强制小写转换代码

### 1. 修改 `src/core/ItemRecord.cpp`
**修改文件**：`src/core/ItemRecord.cpp`
**修改目的**：在构建 `ItemRecord` 数据模型时，保留原始文件的实际扩展名大小写（如 `.EPS`、`.PNG`），防止被强转成 `.eps` / `.png`。

**精准替换 Diff**：
```cpp
<<<<<<< SEARCH
            r.suffix = QString::fromStdWString(meta.ext).toLower();
=======
            r.suffix = QString::fromStdWString(meta.ext);
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
            r.suffix = (lastDot != -1) ? r.filename.mid(lastDot + 1).toLower() : "";
=======
            r.suffix = (lastDot != -1) ? r.filename.mid(lastDot + 1) : "";
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
        r.suffix = (lastDot != -1) ? nPath.mid(lastDot + 1).toLower() : "";
=======
        r.suffix = (lastDot != -1) ? nPath.mid(lastDot + 1) : "";
>>>>>>> REPLACE
```

---

### 2. 修改 `src/core/IndexedEntry.h`
**修改文件**：`src/core/IndexedEntry.h`
**修改目的**：提取索引条目扩展名时，保留原始大小写。

**精准替换 Diff**：
```cpp
<<<<<<< SEARCH
        return name.mid(dotIdx + 1).toLower();
=======
        return name.mid(dotIdx + 1);
>>>>>>> REPLACE
```

---

## 阶段二：确保 BatchRenameEngine 正确保留用户输入大小写

### 3. 修改 `src/meta/BatchRenameEngine.cpp`
**修改文件**：`src/meta/BatchRenameEngine.cpp`
**修改目的**：在由规则生成新文件名时，原样拼装用户在 `m_textEdit` 中输入的 `rule.value` 文本与原文件扩展名，绝对禁止进行大小写转换。

**精准替换 Diff**：
```cpp
<<<<<<< SEARCH
    QString ext = info.suffix();
    if (!ext.isEmpty()) newName += "." + ext;
=======
    QString ext = info.suffix();
    if (!ext.isEmpty()) newName += "." + ext;
>>>>>>> REPLACE
```

---

## 阶段三：强化 DiskBatchRenameService 执行 Windows 大小写改名

### 4. 修改 `src/ui/DiskBatchRenameService.cpp`
**修改文件**：`src/ui/DiskBatchRenameService.cpp`
**修改目的**：重命名操作强制使用 `FileOperationHelper::safeRename`（包含 UUID 中转逻辑），确保仅有大小写改变时，Windows NTFS 文件系统能 100% 正确更新文件大写名称。

**精准替换 Diff**：
```cpp
<<<<<<< SEARCH
            } else { // Rename
                ok = FileOperationHelper::safeRename(oldPath, newPathStr);
            }
=======
            } else { // Rename
                // 强制调用 safeRename 进行两阶段 UUID 中转改名，解决 Windows NTFS 大小写不敏感缺陷
                ok = FileOperationHelper::safeRename(oldPath, newPathStr);
            }
>>>>>>> REPLACE
```

---

## 验证与测试步骤

1. **输入大写字符串改名测试**：
   在批量重命名对话框的文本规则中输入大写文本（如 `VIEW_`），确认在右侧实时对比列表和最终物理磁盘重命名结果中，输出的文件名为大写 `VIEW_001.eps`，绝不退化为小写 `view_001.eps`。
2. **纯大小写修改测试**：
   选中名为 `test.jpg` 的文件，设置规则改为 `TEST.jpg`，执行重命名，确认物理磁盘上的文件名成功更新为大写 `TEST.jpg`。
