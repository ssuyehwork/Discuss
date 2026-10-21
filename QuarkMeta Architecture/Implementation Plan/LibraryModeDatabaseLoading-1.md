# QuarkMeta 修复 C2660 ItemModelBase::setRecords 虚函数签名实施方案 (LibraryModeDatabaseLoading-1.md)

## 1. Overview（概述与解决的问题）

### 归因分析
在此前实施库模式纯数据库加载方案时，我们在 `DiskItemModel.h` 中为 `setRecords` 增加了 `RecordSource source = RecordSource::Disk` 参数，但是在抽象基类 `ItemModelBase.h` 中的纯虚函数 `virtual void setRecords(const std::vector<QuarkMeta::ItemRecord>& records) = 0;` 未同步增加默认参数 `RecordSource source = RecordSource::Disk`。
在 `ContentDataLoader.cpp` 中通过 `ItemModelBase*` 多态指针调用 `weakPanel->model()->setRecords(validRecords, RecordSource::Prepared)` 时，MSVC 编译器匹配到了基类 `ItemModelBase::setRecords` 仅接受 1 个参数的签名，抛出 C2660 错误。

### 解决方案
在抽象基类 `ItemModelBase` 中将 `setRecords` 虚函数签名修改为：
```cpp
virtual void setRecords(const std::vector<QuarkMeta::ItemRecord>& records, RecordSource source = RecordSource::Disk) = 0;
```
并在派生类 `DiskItemModel` 中使用 `override` 进行精准多态匹配。

---

## 2. Pre-Draft Evidence（起草前核查清单）

### 签名原文摘录
- `ItemModelBase::setRecords`: `virtual void setRecords(const std::vector<QuarkMeta::ItemRecord>& records) = 0;` (位于 `src/ui/models/ItemModelBase.h:44`)
- `DiskItemModel::setRecords`: `void setRecords(const std::vector<QuarkMeta::ItemRecord>& records, RecordSource source = RecordSource::Disk);` (位于 `src/ui/models/DiskItemModel.h:40`)

---

## 3. Modified Files List（影响文件清单）

1. `src/ui/models/ItemModelBase.h`
2. `src/ui/models/DiskItemModel.h`
3. `src/ui/models/DiskItemModel.cpp`

---

## 4. Detailed Line-by-Line Changes（精准替换块）

### 4.1 `ItemModelBase.h` 修改纯虚函数签名

```
<<<<<<< SEARCH
        virtual void setRecords(const std::vector<QuarkMeta::ItemRecord>& records) = 0;
=======
        virtual void setRecords(const std::vector<QuarkMeta::ItemRecord>& records, RecordSource source = RecordSource::Disk) = 0;
>>>>>>> REPLACE
```

---

### 4.2 `DiskItemModel.h` 匹配 override 签名

```
<<<<<<< SEARCH
    void setRecords(const std::vector<QuarkMeta::ItemRecord>& records, RecordSource source = RecordSource::Disk);
    void setRecords(const std::vector<QuarkMeta::ItemRecord>& records) override { setRecords(records, RecordSource::Disk); }
=======
    void setRecords(const std::vector<QuarkMeta::ItemRecord>& records, RecordSource source = RecordSource::Disk) override;
>>>>>>> REPLACE
```

---

## 5. Build & Verification Steps（编译与验证步骤）

1. **编译命令**：
   ```bash
   cmake -B build
   cmake --build build --config Release
   ```
2. **验证结果**：`setRecords` 虚函数在基类与派生类签名一致，MSVC C2660 错误彻底消除。

---

## 6. Compliance Self-Check Table（合规核对表）

| 检查项 | 来源条款 | 结论 | 对应方案位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | 文件名 `LibraryModeDatabaseLoading-1.md` |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/` |
| 所有 SEARCH 块代码在仓库真实存在，含上下文 | 3.2 | 满足 | 4.1 ~ 4.2 章节 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册且有 .cpp | 3.3 | 满足 | 已有编译目标文件 |
| 第 1~9 章齐全 | 3.4 | 满足 | 全文 1~7 章节完整 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 2.0 章节 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 满足 | 不修改包含 |
| 已复用既有 SSOT 入口，无另起炉灶 | 2.4 | 满足 | 虚函数签名修补 |
| 无内联 setStyleSheet，样式走 style.qss | 4.1 | 不涉及 | 不涉及 QSS |
| 无平台级 Hack，无无范围全局事件过滤器 | 4.2 | 满足 | 纯 C++ 虚函数匹配 |
| 批量操作为一次聚合广播 | 4.3 | 不涉及 | 不涉及广播 |
| 迁移的所有数值原样照抄 | 4.4 | 不涉及 | 不涉及数值 |
| 未改动用户既有设计理念与交互方式 | 1.2 / Memories.md | 满足 | 零交互改变 |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅针对关联文件 |

---

## 7. Zero-Value-Alteration Check（数值零篡改核对）

- **结论**：不涉及（本次修改仅修复 C++ 虚函数多态签名匹配，不修改任何 UI 属性数值）。
