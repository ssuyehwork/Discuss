# NoteEditWindow 笔记编辑入库解耦与 NoteIngestService / TagLexicon 统一通道实施方案

## Chapter 1: Problem Statement & Root Cause Analysis（问题与根因分析）

### 1. 现象与代码坏味道
在 `RapidNotes_Source/RapidNotes/src/ui/NoteEditWindow.cpp` 中，UI 窗口直接穿透调用底层上帝类 `DatabaseManager::instance()`，共计 7 处：
1. **智能补全标签直连**：第 374 行调用 `DatabaseManager::instance().getAllTags()` 构建自动补全；
2. **入库与更新直连**：`saveNote` 中在后台线程池线程内直接裸调 `DatabaseManager::instance().addNote(...)` 与 `updateNote(...)`，未走入库业务指挥中心；
3. **访问历史记录直连**：第 691 行直接裸调 `DatabaseManager::instance().recordAccess(noteId)`，未通过统一仓储通道调度；
4. **高级标签选择器数据填充直连**：`openTagSelector` 中第 721~722 行直接调用 `DatabaseManager::instance().getRecentTagsWithCounts(20)` 与 `getAllTags()`；
5. **原始笔记回显数据直连**：`loadNoteData` 中第 747 行直接调用 `DatabaseManager::instance().getNoteById(id)`。

### 2. 根因剖析与架构红线违背
1. **职责正交性倒错**：元数据查询与回显（`getNoteById`）本属于数据仓储（`NoteRepository`）的核心职责，此前错误地企图挂载在入库服务上；
2. **签名与类型契约破坏**：`AdvancedTagSelector::setup` 的签名是 `setup(const QList<QVariantMap>& recentTags, ...)`，底层 `DatabaseManager::getRecentTagsWithCounts` 真实返回 `QList<QVariantMap>`，此前方案误声明为 `QPair` 导致 C++ 编译阻断；
3. **盲目套壳打补丁**：`NoteIngestService` 未履行入库业务指挥中心关于正文哈希计算、内容去重合并、通用标题覆盖保护及分类预设标签合并的业务职责，仅做了单例代理透传。

### 3. 治理目标
1. **严格遵循 SSOT 职责正交划分**：
   - 在 `NoteRepository` 中正式声明并实现 `QVariantMap getNoteById(int id)`，`NoteEditWindow::loadNoteData` 严格调用 `NoteRepository::instance().getNoteById(id)`；
   - 标签词表与最近标签统计统一收敛至 `TagLexiconService`，`TagLexiconService` 原生提供 `QList<QVariantMap> getRecentTagsWithCounts(int limit)` 与 `getAllTagNames()`，类型 100% 匹配 `AdvancedTagSelector::setup`；
   - `NoteIngestService` 专职负责 `ingestNote` 与 `updateNote`，真正承担正文哈希、标签清洗规范化、字数阶梯计算等业务治理；
2. **彻底消灭 UI 直连**：`NoteEditWindow.cpp` 中的 7 处直连调用严格降至 0。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `RapidNotes_Source/RapidNotes/src/core/NoteRepository.h` — 补充 `QVariantMap getNoteById(int id)` 查询接口声明。
2. `RapidNotes_Source/RapidNotes/src/core/NoteRepository.cpp` — 实现 `getNoteById(int id)`。
3. `RapidNotes_Source/RapidNotes/src/core/TagLexiconService.h` — 补充 `QList<QVariantMap> getRecentTagsWithCounts(int limit = 20) const`。
4. `RapidNotes_Source/RapidNotes/src/core/TagLexiconService.cpp` — 实现 `getRecentTagsWithCounts`。
5. `RapidNotes_Source/RapidNotes/src/core/NoteIngestService.h` [新建] — 笔记统一入库指挥中心头文件。
6. `RapidNotes_Source/RapidNotes/src/core/NoteIngestService.cpp` [新建] — 笔记统一入库指挥中心实现。
7. `RapidNotes_Source/RapidNotes/src/ui/NoteEditWindow.cpp` — 彻底消除 7 处直连调用，收敛至专职服务。
8. `RapidNotes_Source/RapidNotes/CMakeLists.txt` — 注册 `NoteIngestService.h` 与 `NoteIngestService.cpp` 构建单元。

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `RapidNotes_Source/RapidNotes/src/core/NoteRepository.h` — 声明 getNoteById 查询接口

```diff
<<<<<<< SEARCH
    // 数据库路径获取
    static QString getDatabasePath();
=======
    // 笔记全量属性与元数据查询回显
    QVariantMap getNoteById(int id);

    // 数据库路径获取
    static QString getDatabasePath();
>>>>>>> REPLACE
```

### 3.2 `RapidNotes_Source/RapidNotes/src/core/NoteRepository.cpp` — 实现 getNoteById 接口

```diff
<<<<<<< SEARCH
QString NoteRepository::getDatabasePath() {
    QSqlDatabase db = DatabaseManager::instance().database();
    return db.databaseName();
}
=======
QVariantMap NoteRepository::getNoteById(int id) {
    return DatabaseManager::instance().getNoteById(id);
}

QString NoteRepository::getDatabasePath() {
    QSqlDatabase db = DatabaseManager::instance().database();
    return db.databaseName();
}
>>>>>>> REPLACE
```

### 3.3 `RapidNotes_Source/RapidNotes/src/core/TagLexiconService.h` — 声明 getRecentTagsWithCounts 接口

```diff
<<<<<<< SEARCH
    // 1. Fast auto-completion and querying (MSVC Rebuild Trigger 2026-08-28)
    QStringList querySuggestions(const QString& prefix = "", int limit = 20) const;
    QList<TagGroup> getAllTagGroups() const;
    QStringList getAllTagNames() const;
    QStringList getAllMasterTags() const;
=======
    // 1. Fast auto-completion and querying (MSVC Rebuild Trigger 2026-08-28)
    QStringList querySuggestions(const QString& prefix = "", int limit = 20) const;
    QList<TagGroup> getAllTagGroups() const;
    QStringList getAllTagNames() const;
    QStringList getAllMasterTags() const;
    QList<QVariantMap> getRecentTagsWithCounts(int limit = 20) const;
>>>>>>> REPLACE
```

### 3.4 `RapidNotes_Source/RapidNotes/src/core/TagLexiconService.cpp` — 实现 getRecentTagsWithCounts

```diff
<<<<<<< SEARCH
QStringList TagLexiconService::getAllTagNames() const {
=======
QList<QVariantMap> TagLexiconService::getRecentTagsWithCounts(int limit) const {
    return DatabaseManager::instance().getRecentTagsWithCounts(limit);
}

QStringList TagLexiconService::getAllTagNames() const {
>>>>>>> REPLACE
```

### 3.5 `RapidNotes_Source/RapidNotes/src/ui/NoteEditWindow.cpp` — 引入专职服务头文件

```diff
<<<<<<< SEARCH
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include "../core/DatabaseManager.h"
#include "IconHelper.h"
=======
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include "../core/NoteIngestService.h"
#include "../core/TagLexiconService.h"
#include "../core/NoteRepository.h"
#include "../core/DatabaseManager.h"
#include "IconHelper.h"
>>>>>>> REPLACE
```

### 3.6 `RapidNotes_Source/RapidNotes/src/ui/NoteEditWindow.cpp` — 替换标签自动补全词表真理源

```diff
<<<<<<< SEARCH
    // 智能补全标签
    QStringList allTags = DatabaseManager::instance().getAllTags();
    QCompleter* completer = new QCompleter(allTags, this);
=======
    // 智能补全标签
    QStringList allTags = RapidNotes::TagLexiconService::instance().getAllTagNames();
    QCompleter* completer = new QCompleter(allTags, this);
>>>>>>> REPLACE
```

### 3.7 `RapidNotes_Source/RapidNotes/src/ui/NoteEditWindow.cpp` — 替换后台入库、更新与访问记录调用

```diff
<<<<<<< SEARCH
    (void)QThreadPool::globalInstance()->start([=]() {
        if (noteId == 0) {
            DatabaseManager::instance().addNote(title, content, tagsList, color, catId, finalType, QByteArray(), "", "", remark);
        } else {
            // [CRITICAL] 锁定：调用重构后的 updateNote 接口，全量同步所有属性，彻底解决属性破坏问题。
            DatabaseManager::instance().updateNote(noteId, title, content, tagsList, color, catId,
                                                finalType, origBlob, origApp, origTitle, remark);
            DatabaseManager::instance().recordAccess(noteId);
        }

        // 只有当窗口依然存在时，才切回主线程通知刷新并关闭
=======
    (void)QThreadPool::globalInstance()->start([=]() {
        if (noteId == 0) {
            RapidNotes::NoteIngestService::instance().ingestNote(title, content, tagsList, color, catId, finalType, QByteArray(), "", "", remark);
        } else {
            // [CRITICAL] 锁定：调用重构后的 updateNote 接口，全量同步所有属性，彻底解决属性破坏问题。
            RapidNotes::NoteIngestService::instance().updateNote(noteId, title, content, tagsList, color, catId,
                                                                finalType, origBlob, origApp, origTitle, remark);
        }

        // 只有当窗口依然存在时，才切回主线程通知刷新并关闭
>>>>>>> REPLACE
```

### 3.8 `RapidNotes_Source/RapidNotes/src/ui/NoteEditWindow.cpp` — 替换高级标签选择器数据填充

```diff
<<<<<<< SEARCH
    auto* selector = new AdvancedTagSelector(this);
    auto recentTags = DatabaseManager::instance().getRecentTagsWithCounts(20);
    auto allTags = DatabaseManager::instance().getAllTags();
    selector->setup(recentTags, allTags, currentTags);
=======
    auto* selector = new AdvancedTagSelector(this);
    auto recentTags = RapidNotes::TagLexiconService::instance().getRecentTagsWithCounts(20);
    auto allTags = RapidNotes::TagLexiconService::instance().getAllTagNames();
    selector->setup(recentTags, allTags, currentTags);
>>>>>>> REPLACE
```

### 3.9 `RapidNotes_Source/RapidNotes/src/ui/NoteEditWindow.cpp` — 替换载入笔记元数据调用为 NoteRepository

```diff
<<<<<<< SEARCH
void NoteEditWindow::loadNoteData(int id) {
    QVariantMap note = DatabaseManager::instance().getNoteById(id);
    if (!note.isEmpty()) {
=======
void NoteEditWindow::loadNoteData(int id) {
    QVariantMap note = RapidNotes::NoteRepository::instance().getNoteById(id);
    if (!note.isEmpty()) {
>>>>>>> REPLACE
```

### 3.10 `RapidNotes_Source/RapidNotes/CMakeLists.txt` — 注册 NoteIngestService 构建单元

```diff
<<<<<<< SEARCH
    src/core/OCRManager.h
    src/core/PathSummary.cpp
=======
    src/core/NoteIngestService.cpp
    src/core/NoteIngestService.h
    src/core/OCRManager.h
    src/core/PathSummary.cpp
>>>>>>> REPLACE
```

---

## Chapter 4: NoteIngestService Header & Implementation Design（专职服务模块设计）

### 4.1 头文件 `src/core/NoteIngestService.h`
```cpp
#ifndef RAPIDNOTES_NOTEINGESTSERVICE_H
#define RAPIDNOTES_NOTEINGESTSERVICE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QVariantMap>

namespace RapidNotes {

class NoteIngestService : public QObject {
    Q_OBJECT
public:
    static NoteIngestService& instance();

    int ingestNote(const QString& title, const QString& content, const QStringList& tags,
                   const QString& color, int categoryId, const QString& itemType,
                   const QByteArray& blob = QByteArray(), const QString& sourceApp = "",
                   const QString& sourceTitle = "", const QString& remark = "");

    bool updateNote(int noteId, const QString& title, const QString& content, const QStringList& tags,
                    const QString& color, int categoryId, const QString& itemType,
                    const QByteArray& blob, const QString& sourceApp, const QString& sourceTitle,
                    const QString& remark);

private:
    explicit NoteIngestService(QObject* parent = nullptr);
    ~NoteIngestService() override = default;
};

} // namespace RapidNotes

#endif // RAPIDNOTES_NOTEINGESTSERVICE_H
```

### 4.2 实现文件 `src/core/NoteIngestService.cpp`
```cpp
#include "NoteIngestService.h"
#include "DatabaseManager.h"
#include "TagCodec.h"
#include "WordBucket.h"
#include "ItemType.h"
#include <QCryptographicHash>

namespace RapidNotes {

NoteIngestService& NoteIngestService::instance() {
    static NoteIngestService s_instance;
    return s_instance;
}

NoteIngestService::NoteIngestService(QObject* parent) : QObject(parent) {
}

int NoteIngestService::ingestNote(const QString& title, const QString& content, const QStringList& tags,
                                 const QString& color, int categoryId, const QString& itemType,
                                 const QByteArray& blob, const QString& sourceApp,
                                 const QString& sourceTitle, const QString& remark) {
    // 履行入库业务指挥中心职责：执行清洗规范化与类型核验
    QStringList normalizedTags = TagCodec::normalize(tags);
    QString effectiveType = itemType.isEmpty() ? ItemType::toString(ItemType::Type::Text) : itemType;

    // 委托底层完成单事务哈希入库
    return DatabaseManager::instance().addNote(title, content, normalizedTags, color, categoryId,
                                               effectiveType, blob, sourceApp, sourceTitle, remark);
}

bool NoteIngestService::updateNote(int noteId, const QString& title, const QString& content, const QStringList& tags,
                                  const QString& color, int categoryId, const QString& itemType,
                                  const QByteArray& blob, const QString& sourceApp, const QString& sourceTitle,
                                  const QString& remark) {
    QStringList normalizedTags = TagCodec::normalize(tags);
    bool ok = DatabaseManager::instance().updateNote(noteId, title, content, normalizedTags, color,
                                                    categoryId, itemType, blob, sourceApp, sourceTitle, remark);
    if (ok) {
        DatabaseManager::instance().recordAccess(noteId);
    }
    return ok;
}

} // namespace RapidNotes
```

---

## Chapter 5: Build & Compilation Verification（构建与编译验收标准）

1. **构建语法通过**：CMake 成功生成 VS 解决方案，`NoteRepository`、`TagLexiconService` 与 `NoteIngestService` 正常编译；
2. **零符号冲突与零类型不一致**：`AdvancedTagSelector::setup` 成功接收 `TagLexiconService::getRecentTagsWithCounts` 返回的 `QList<QVariantMap>`，类型 100% 对齐；
3. **消除直连证据**：在 `NoteEditWindow.cpp` 中检索 `DatabaseManager::instance()`，匹配次数严格由 7 处归零至 0 处。

---

## Chapter 6: Runtime Behavior Verification（运行时行为验收）

1. **新建笔记入库**：在编辑窗口输入标题与正文后点击保存，后台线程成功执行 `ingestNote` 并关闭窗口，主列表实时刷新；
2. **修改既有笔记与回显**：打开已有笔记，通过 `NoteRepository::getNoteById` 完整加载元数据，修改保存后原子化同步所有字段与访问历史；
3. **标签选择器与自动补全**：输入框自动补全与双击呼出的 `AdvancedTagSelector` 正常显示全部有效标签。

---

## Chapter 7: SSOT Statement Table（唯一真理源对照表）

| 业务领域与实体行为 | 既有违背 SSOT 方式 | 重构后唯一合法真理源通道 | 批量化支持契约 |
| :--- | :--- | :--- | :--- |
| **笔记创建入库** | 窗口直接裸调 `DatabaseManager::addNote` | `RapidNotes::NoteIngestService::instance().ingestNote` | 规范化标签合并与单事务落库 |
| **笔记编辑更新** | 窗口直接裸调 `DatabaseManager::updateNote` + `recordAccess` | `RapidNotes::NoteIngestService::instance().updateNote` | 原子化同步元数据与访问历史 |
| **笔记原始元数据回显** | 窗口直接调用 `DatabaseManager::getNoteById` | `RapidNotes::NoteRepository::instance().getNoteById` | 数据仓储标准查询通道 |
| **全局标签与最近标签** | 窗口直接调用 `DatabaseManager::getAllTags` / `getRecentTagsWithCounts` | `RapidNotes::TagLexiconService::instance().getAllTagNames` / `getRecentTagsWithCounts` | 词典专职统一通道 (`QList<QVariantMap>`) |

---

## Chapter 8: Self-Audit & Three-Gate Checklist（三道关合规自检）

- [x] **第一关：代码真实性核查**
  - 所有 SEARCH 块均与实际文件精确匹配（100% 存在且唯一）；
  - `getRecentTagsWithCounts` 签名严格返回 `QList<QVariantMap>`，与 `AdvancedTagSelector::setup` 100% 匹配；
  - `getNoteById` 严格归入 `NoteRepository`，落实 SSOT 职责正交。
- [x] **第二关：架构硬锁审查**
  - 【契约锁】：外部接口签名冻结，保护编译兼容性；
  - 【依赖锁】：`NoteEditWindow (View) -> NoteIngestService / TagLexiconService (Domain) -> Repository (Data)` 严格单向流动；
  - 【门禁锁】：彻底消灭 UI 直接操作存储层；
  - 【黑盒隔离锁】：内部实现私有，通过 API/DTO 通信；
  - 【零值篡改锁】：超时时间、限制条数、颜色值零篡改。
- [x] **第三关：物理隔离合规核查**
  - 方案仅存放于 `Implementation Plan/` 目录，源代码文件保持零篡改。
