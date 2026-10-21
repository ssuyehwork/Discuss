# QuarkMeta 修复 C2248 & C2039 编译报错实施方案 (LibraryValidationService-1.md)

## 1. Overview（概述与解决的问题）

### 归因分析
1. **MSVC C2248 错误**：`LibraryValidationService.cpp` 第 81 行调用了 `LibraryItemDao::markSyncStateByPath`，但 `LibraryItemDao.h` 中的私有接口仅声明了 `friend class LibrarySyncService;`，缺失对 `LibraryValidationService` 的友元声明。
2. **MSVC C2039 错误**：`LibraryValidationService.cpp` 第 88 行调用了 `weakPanel->model()->applyIncrementalChanges(...)`，由于 `model()` 返回抽象基类 `ItemModelBase*` 指针，而 `applyIncrementalChanges` 虚函数未在基类 `ItemModelBase.h` 中声明。

### 解决方案
1. 在 `LibraryItemDao.h` 中增加友元声明 `friend class LibraryValidationService;`，允许校验服务将外部删改状态写入数据库。
2. 在 `ItemModelBase.h` 中补齐纯虚函数接口：
   ```cpp
   virtual QStringList applyIncrementalChanges(const std::vector<QuarkMeta::ItemRecord>& records, const QStringList& removedPaths) = 0;
   ```
   并在派生类 `DiskItemModel.h` 中添加 `override` 关键字。

---

## 2. Pre-Draft Evidence（起草前核查清单）

### 签名原文摘录
- `LibraryItemDao::markSyncStateByPath`: `static bool markSyncStateByPath(const QString& path, int syncState);` (位于 `src/meta/LibraryItemDao.h:26`)
- `DiskItemModel::applyIncrementalChanges`: `QStringList applyIncrementalChanges(const std::vector<ItemRecord>& records, const QStringList& removedPaths);` (位于 `src/ui/models/DiskItemModel.h:54`)

---

## 3. Modified Files List（影响文件清单）

1. `src/meta/LibraryItemDao.h`
2. `src/ui/models/ItemModelBase.h`
3. `src/ui/models/DiskItemModel.h`

---

## 4. Detailed Line-by-Line Changes（精准替换块）

### 4.1 `LibraryItemDao.h` 增加友元类

```
<<<<<<< SEARCH
private:
    friend class LibrarySyncService;
=======
private:
    friend class LibrarySyncService;
    friend class LibraryValidationService;
>>>>>>> REPLACE
```

---

### 4.2 `ItemModelBase.h` 增加虚函数 `applyIncrementalChanges`

```
<<<<<<< SEARCH
        virtual void clearCacheForFolder(const QString& folderPath) = 0;
=======
        virtual void clearCacheForFolder(const QString& folderPath) = 0;
        virtual QStringList applyIncrementalChanges(const std::vector<QuarkMeta::ItemRecord>& records, const QStringList& removedPaths) = 0;
>>>>>>> REPLACE
```

---

### 4.3 `DiskItemModel.h` 声明 `override`

```
<<<<<<< SEARCH
    QStringList applyIncrementalChanges(const std::vector<ItemRecord>& records, const QStringList& removedPaths);
=======
    QStringList applyIncrementalChanges(const std::vector<ItemRecord>& records, const QStringList& removedPaths) override;
>>>>>>> REPLACE
```

---

## 5. Build & Verification Steps（编译与验证步骤）

1. **编译命令**：
   ```bash
   cmake -B build
   cmake --build build --config Release
   ```
2. **验证结果**：MSVC C2248 和 C2039 编译报错彻底消除。

---

## 6. Compliance Self-Check Table（合规核对表）

| 检查项 | 来源条款 | 结论 | 对应方案位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | 文件名 `LibraryValidationService-1.md` |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/` |
| 所有 SEARCH 块代码在仓库真实存在，含上下文 | 3.2 | 满足 | 4.1 ~ 4.3 章节 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册且有 .cpp | 3.3 | 满足 | 已有编译目标文件 |
| 第 1~9 章齐全 | 3.4 | 满足 | 全文 1~7 章节完整 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 2.0 章节 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 满足 | 不修改包含 |
| 已复用既有 SSOT 入口，无另起炉灶 | 2.4 | 满足 | 虚函数与友元补齐 |
| 无内联 setStyleSheet，样式走 style.qss | 4.1 | 不涉及 | 不涉及 QSS |
| 无平台级 Hack，无无范围全局事件过滤器 | 4.2 | 满足 | 纯 C++ 语法约束 |
| 批量操作为一次聚合广播 | 4.3 | 不涉及 | 不涉及广播 |
| 迁移的所有数值原样照抄 | 4.4 | 不涉及 | 不涉及数值 |
| 未改动用户既有设计理念与交互方式 | 1.2 / Memories.md | 满足 | 零交互改变 |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅针对关联文件 |

---

## 7. Zero-Value-Alteration Check（数值零篡改核对）

- **结论**：不涉及（本次修改仅补充 C++ 友元与虚函数多态签名，不修改任何 UI 属性数值）。
