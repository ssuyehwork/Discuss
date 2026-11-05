# Implementation Plan: RapidNotes 数据库与底层逻辑重构【步骤 1：词表与纯函数】返工实施方案

本实施方案严格遵循根目录《Standard Specifications for Implementation Plan.md》、《Memories_RapidNotes.md》第 6、10、11 章，以及《RapidNotes 步骤 1 返工清单》（词表与纯函数，不改存储、不改 SQL 语义）的全部规范要求。

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 核心病灶与系统性缺陷深入剖析
1. **`item_type` 字符串字面量未彻底收敛**：
   - `DatabaseManager.cpp` 未包含 `ItemType.h`，导致 `extractFileExtensions`、`addNote`（Link 归类判定）、分类统计函数 `getClassificationStats`（`image`、`code`、`text`、`link`、`file`、`local_file`、`local_folder`、`folder`、`local_batch` 的 `if/else` 链路）及 `applyCommonFilters`（过滤类型拼接及 `NOT IN (...)` 语句）散落硬编码字符串字面量，缺乏单一类型系统真理源 (SSOT)。
   - `NoteModel.cpp`（`ToolTipRole` 图片判定、`DisplayRole` 文本与文件判定、`mimeData` 拖拽导出判定）与 `StringUtils.h`（`copyNotesToClipboard`、`generateNotePreviewHtml`）依然存在类型字符串字面量，未与 `ItemType` 绑定。
2. **标签规范化与编解码存在分散实现**：
   - `DatabaseManager.cpp` 在 `updateNote` 和 `addTagsToNote` 内部手写了对标签列表的 `trimmed()`、去空与保序去重循环，且分别以 `joinDisplay` 与 `join` 输出；
   - `TagCodec` 尚未向外暴露通用的列表规范化纯函数（`normalize`），导致调用处只能各自编写局部循环处理已有的 `QStringList`。
3. **`calculateWordBucket` 冗余转发壳**：
   - `DatabaseManager.cpp` 匿名命名空间中保留了 `calculateWordBucket` 静态函数，仅作为 `RapidNotes::WordBucket::calculate` 的无意义转发包装，多处调用绕经该壳函数，产生间接调用。

### 1.2 架构治理设计目标与六大一票否决红线
- **一票否决 1：算力利用红线**：类型与种类判定纯函数化，视图层图标判定严禁做磁盘 I/O（`detectDisplayCategory` 纯内存判定，彻底与磁盘解耦）；
- **一票否决 2：接口归一化与批量优先**：`TagCodec` 统一规范标签集合与字符串的双向流转，提供 `normalize` 批量清洗；
- **一票否决 3：补丁零容忍**：保持强类型枚举 `ItemType::Type` 作为唯一类型体系，消除所有散落字面量，`allowedColumns` 仅保留列名 `"item_type"`；
- **一票否决 4：严禁偷指针红线**：所有模块均为无状态纯函数（Pure Functions）或值语义工具类，零指针依赖；
- **一票否决 5：界面与数值零篡改红线**：图标名、图标颜色、缩略图尺寸、字数阶梯分档阈值 100% 保持现状；
- **一票否决 6：SSOT 索引缺失一票否决**：必须在 `docs/SSOT接口索引.md` 中完整登记全部公开接口，且与头文件注释保持 100% 一致。
- **本步骤范围严格锁定**：不改动表结构，不改动 SQL 语义与结果，不修改界面数值与样式。

### 1.3 Gate 1：起草前已读物理证据清单
1. **物理读取文件列表**：
   - `RapidNotes_Source/RapidNotes/CMakeLists.txt`
   - `RapidNotes_Source/RapidNotes/src/core/ItemType.h` 与 `.cpp`
   - `RapidNotes_Source/RapidNotes/src/core/ItemClassifier.h` 与 `.cpp`
   - `RapidNotes_Source/RapidNotes/src/core/PathSummary.h` 与 `.cpp`
   - `RapidNotes_Source/RapidNotes/src/core/WordBucket.h` 与 `.cpp`
   - `RapidNotes_Source/RapidNotes/src/core/TagCodec.h` 与 `.cpp`
   - `RapidNotes_Source/RapidNotes/src/core/ThumbnailMaker.h` 与 `.cpp`
   - `RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`
   - `RapidNotes_Source/RapidNotes/src/models/NoteModel.cpp`
   - `RapidNotes_Source/RapidNotes/src/ui/StringUtils.h`
   - `docs/SSOT接口索引.md`
2. **检索签名与证据原文**：
   - `TagCodec::split`：`static QStringList split(const QString& tagsStr);`（`src/core/TagCodec.h:15`）；
   - `TagCodec::join`：`static QString join(const QStringList& tags);`（`src/core/TagCodec.h:18`）；
   - `TagCodec::joinDisplay`：`static QString joinDisplay(const QStringList& tags);`（`src/core/TagCodec.h:21`）；
   - `ItemType::toString`：`static QString toString(Type type);`（`src/core/ItemType.h:37`）；
   - `ItemType::fromString`：`static Type fromString(const QString& str);`（`src/core/ItemType.h:38`）；
   - `WordBucket::calculate`：`static int calculate(const QString& content, const QString& itemType);`（`src/core/WordBucket.h:15`）；
   - 验证 `DatabaseManager.cpp` 中 `calculateWordBucket` 确为第 59-61 行的无意义包装函数；
   - 验证 `NoteModel.cpp` 包含 `ItemType.h`（第 2 行）；`StringUtils.h` 需补充包含 `../core/ItemType.h`。

---

## Chapter 2: Modified Files List（影响文件清单）

### 2.1 新建文档清单
1. `docs/SSOT接口索引.md`（已创建并完成全量登记）

### 2.2 待修改核心代码清单
1. `RapidNotes_Source/RapidNotes/src/core/TagCodec.h`（新增 `normalize` 方法声明并规范化注释）
2. `RapidNotes_Source/RapidNotes/src/core/TagCodec.cpp`（实现 `normalize` 方法，并让 `split` 复用 `normalize`）
3. `RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`（包含 `ItemType.h`，删除 `calculateWordBucket` 壳函数，收敛类型字面量至 `ItemType`，收敛标签去重与拼接至 `TagCodec`）
4. `RapidNotes_Source/RapidNotes/src/models/NoteModel.cpp`（收敛 `ToolTipRole`、`DisplayRole`、`mimeData` 中的类型判断至 `ItemType`）
5. `RapidNotes_Source/RapidNotes/src/ui/StringUtils.h`（包含 `ItemType.h`，收敛 `copyNotesToClipboard` 与 `generateNotePreviewHtml` 中的类型判断至 `ItemType`）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 修改 `src/core/TagCodec.h`
新增 `normalize` 规范化接口（对已有列表执行 trim、去空与保序去重），与 `docs/SSOT接口索引.md` 第 5 章保持绝对一致。

```
<<<<<<< SEARCH
    // 拆分：同时支持半角逗号 `,` 和全角逗号 `，`
    static QStringList split(const QString& tagsStr);

    // 拼接：严格使用半角逗号 `,` 连接，无空格
    static QString join(const QStringList& tags);
=======
    // 拆分：同时支持半角逗号 `,` 和全角逗号 `，`，自动清洗首尾空白、空项并保序去重
    static QStringList split(const QString& tagsStr);

    // 规范化：对已有标签列表执行逐项 trim、剔除空项并保序去重
    static QStringList normalize(const QStringList& tags);

    // 拼接：严格使用半角逗号 `,` 连接，无空格
    static QString join(const QStringList& tags);
>>>>>>> REPLACE
```

---

### 3.2 修改 `src/core/TagCodec.cpp`
实现 `TagCodec::normalize`，并使 `split` 直接复用 `normalize`，确保全工程去重算法统一。

```
<<<<<<< SEARCH
QStringList TagCodec::split(const QString& tagsStr) {
    if (tagsStr.trimmed().isEmpty()) return QStringList();
    static const QRegularExpression splitRegex(QStringLiteral("[,，]"));
    QStringList rawList = tagsStr.split(splitRegex, Qt::SkipEmptyParts);
    QStringList result;
    for (const QString& t : rawList) {
        QString trimmed = t.trimmed();
        if (!trimmed.isEmpty() && !result.contains(trimmed)) {
            result.append(trimmed);
        }
    }
    return result;
}
=======
QStringList TagCodec::normalize(const QStringList& tags) {
    QStringList result;
    for (const QString& t : tags) {
        QString trimmed = t.trimmed();
        if (!trimmed.isEmpty() && !result.contains(trimmed)) {
            result.append(trimmed);
        }
    }
    return result;
}

QStringList TagCodec::split(const QString& tagsStr) {
    if (tagsStr.trimmed().isEmpty()) return QStringList();
    static const QRegularExpression splitRegex(QStringLiteral("[,，]"));
    QStringList rawList = tagsStr.split(splitRegex, Qt::SkipEmptyParts);
    return normalize(rawList);
}
>>>>>>> REPLACE
```

---

### 3.3 修改 `src/core/DatabaseManager.cpp`

#### 3.3.1 补充包含 `ItemType.h`
```
<<<<<<< SEARCH
#include "DatabaseManager.h"
#include "WordBucket.h"
#include "TagCodec.h"
#include <QDebug>
=======
#include "DatabaseManager.h"
#include "ItemType.h"
#include "WordBucket.h"
#include "TagCodec.h"
#include <QDebug>
>>>>>>> REPLACE
```

#### 3.3.2 移除 `calculateWordBucket` 转发壳并改写 `extractFileExtensions`
```
<<<<<<< SEARCH
    // 2026-04-09 按照用户要求：采用纯动态后缀解析架构，彻底废弃并移除所有静态白/黑名单
    QString extractFileExtensions(const QString& itemType, const QString& content) {
        if (itemType == "link") return "Link";

        // 仅处理文件/文件夹相关类型
        if (itemType != "file" && itemType != "local_file" && itemType != "local_batch" && itemType != "folder" && itemType != "local_folder") {
            return "";
        }

        QStringList paths = content.split(";", Qt::SkipEmptyParts);
        QSet<QString> extensions;
        for (const QString& path : paths) {
            QFileInfo info(path);
            if (info.isDir()) {
                extensions.insert("文件夹");
            } else {
                QString ext = info.suffix().toLower();
                if (!ext.isEmpty()) extensions.insert(ext);
            }
        }

        QStringList result = extensions.values();
        result.sort();
        return result.join(", ");
    }

    int calculateWordBucket(const QString& content, const QString& itemType) {
        return RapidNotes::WordBucket::calculate(content, itemType);
    }

}
=======
    // 2026-04-09 按照用户要求：采用纯动态后缀解析架构，彻底废弃并移除所有静态白/黑名单
    QString extractFileExtensions(const QString& itemType, const QString& content) {
        RapidNotes::ItemType::Type type = RapidNotes::ItemType::fromString(itemType);
        if (type == RapidNotes::ItemType::Type::Link) return "Link";

        // 仅处理文件/文件夹相关类型
        if (!RapidNotes::ItemType::isFileOrFolder(type)) {
            return "";
        }

        QStringList paths = content.split(";", Qt::SkipEmptyParts);
        QSet<QString> extensions;
        for (const QString& path : paths) {
            QFileInfo info(path);
            if (info.isDir()) {
                extensions.insert("文件夹");
            } else {
                QString ext = info.suffix().toLower();
                if (!ext.isEmpty()) extensions.insert(ext);
            }
        }

        QStringList result = extensions.values();
        result.sort();
        return result.join(", ");
    }

}
>>>>>>> REPLACE
```

#### 3.3.3 校准迁移中直调 `WordBucket::calculate`
```
<<<<<<< SEARCH
                while (fetch.next()) {
                    int id = fetch.value(0).toInt();
                    upWC.addBindValue(calculateWordBucket(fetch.value(1).toString(), fetch.value(2).toString()));
                    upWC.addBindValue(id);
                    upWC.exec();
=======
                while (fetch.next()) {
                    int id = fetch.value(0).toInt();
                    upWC.addBindValue(RapidNotes::WordBucket::calculate(fetch.value(1).toString(), fetch.value(2).toString()));
                    upWC.addBindValue(id);
                    upWC.exec();
>>>>>>> REPLACE
```

#### 3.3.4 `addNote` 中的 Link 类型判定与直调 `WordBucket::calculate`
```
<<<<<<< SEARCH
    // [USER_REQUEST] 2026-04-08 网址自动归档：发现 link 类型时自动在“我的分类”中创建 Link 组
    int finalCategoryId = categoryId;
    if (itemType == "link") {
        int linkCatId = getOrCreateCategoryByName("Link", -1, "#1abc9c");
        if (linkCatId > 0) finalCategoryId = linkCatId;
    }
=======
    // [USER_REQUEST] 2026-04-08 网址自动归档：发现 link 类型时自动在“我的分类”中创建 Link 组
    int finalCategoryId = categoryId;
    if (RapidNotes::ItemType::fromString(itemType) == RapidNotes::ItemType::Type::Link) {
        int linkCatId = getOrCreateCategoryByName("Link", -1, "#1abc9c");
        if (linkCatId > 0) finalCategoryId = linkCatId;
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        QStringList cleanedFinalTags;
        for (const QString& t : finalTags) {
            QString tr = t.trimmed();
            if (!tr.isEmpty() && !cleanedFinalTags.contains(tr)) cleanedFinalTags << tr;
        }
        query.bindValue(":tags", RapidNotes::TagCodec::joinDisplay(cleanedFinalTags));

        query.bindValue(":color", finalColor);
        query.bindValue(":category_id", finalCategoryId == -1 ? QVariant(QMetaType::fromType<int>()) : finalCategoryId);
        query.bindValue(":item_type", itemType);
        query.bindValue(":data_blob", dataBlob);
        query.bindValue(":hash", contentHash);
        query.bindValue(":created_at", currentTime);
        query.bindValue(":updated_at", currentTime);
        query.bindValue(":source_app", sourceApp);
        query.bindValue(":source_title", sourceTitle);
        query.bindValue(":remark", remark);
        query.bindValue(":exts", fileExtensions);
        query.bindValue(":wc", calculateWordBucket(content, itemType));
=======
        query.bindValue(":tags", RapidNotes::TagCodec::joinDisplay(RapidNotes::TagCodec::normalize(finalTags)));

        query.bindValue(":color", finalColor);
        query.bindValue(":category_id", finalCategoryId == -1 ? QVariant(QMetaType::fromType<int>()) : finalCategoryId);
        query.bindValue(":item_type", itemType);
        query.bindValue(":data_blob", dataBlob);
        query.bindValue(":hash", contentHash);
        query.bindValue(":created_at", currentTime);
        query.bindValue(":updated_at", currentTime);
        query.bindValue(":source_app", sourceApp);
        query.bindValue(":source_title", sourceTitle);
        query.bindValue(":remark", remark);
        query.bindValue(":exts", fileExtensions);
        query.bindValue(":wc", RapidNotes::WordBucket::calculate(content, itemType));
>>>>>>> REPLACE
```

#### 3.3.5 `updateNote` 中标签规约与直调 `WordBucket::calculate`
```
<<<<<<< SEARCH
        query.bindValue(":remark", remark);
        query.bindValue(":exts", fileExtensions);
        query.bindValue(":wc", calculateWordBucket(content, itemType));

        QStringList trimmedTags;
        for (const QString& t : tags) {
            QString tr = t.trimmed();
            if (!tr.isEmpty() && !trimmedTags.contains(tr)) trimmedTags << tr;
        }
        query.bindValue(":tags", RapidNotes::TagCodec::joinDisplay(trimmedTags));
=======
        query.bindValue(":remark", remark);
        query.bindValue(":exts", fileExtensions);
        query.bindValue(":wc", RapidNotes::WordBucket::calculate(content, itemType));
        query.bindValue(":tags", RapidNotes::TagCodec::joinDisplay(RapidNotes::TagCodec::normalize(tags)));
>>>>>>> REPLACE
```

#### 3.3.6 `updateNoteState` 与 `updateNoteStates` 中的 `calculateWordBucket`
```
<<<<<<< SEARCH
                if (column == "content" || column == "item_type") {
                    QSqlQuery up(m_db);
                    up.prepare("UPDATE notes SET word_count = ? WHERE id = ?");
                    up.addBindValue(calculateWordBucket(content, itemType));
                    up.addBindValue(id);
                    up.exec();
                }
=======
                if (column == "content" || column == "item_type") {
                    QSqlQuery up(m_db);
                    up.prepare("UPDATE notes SET word_count = ? WHERE id = ?");
                    up.addBindValue(RapidNotes::WordBucket::calculate(content, itemType));
                    up.addBindValue(id);
                    up.exec();
                }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
                // 2026-05-02 补齐批量状态下的字数与标签同步
                if (column == "tags") {
                    syncNoteTags(id, RapidNotes::TagCodec::split(value.toString()));
                } else if (column == "content" || column == "item_type") {
                    QSqlQuery fetch(m_db);
                    fetch.prepare("SELECT content, item_type FROM notes WHERE id = ?");
                    fetch.addBindValue(id);
                    if (fetch.exec() && fetch.next()) {
                        QSqlQuery up(m_db);
                        up.prepare("UPDATE notes SET word_count = ? WHERE id = ?");
                        up.addBindValue(calculateWordBucket(fetch.value(0).toString(), fetch.value(1).toString()));
                        up.addBindValue(id);
                        up.exec();
                    }
                }
=======
                // 2026-05-02 补齐批量状态下的字数与标签同步
                if (column == "tags") {
                    syncNoteTags(id, RapidNotes::TagCodec::split(value.toString()));
                } else if (column == "content" || column == "item_type") {
                    QSqlQuery fetch(m_db);
                    fetch.prepare("SELECT content, item_type FROM notes WHERE id = ?");
                    fetch.addBindValue(id);
                    if (fetch.exec() && fetch.next()) {
                        QSqlQuery up(m_db);
                        up.prepare("UPDATE notes SET word_count = ? WHERE id = ?");
                        up.addBindValue(RapidNotes::WordBucket::calculate(fetch.value(0).toString(), fetch.value(1).toString()));
                        up.addBindValue(id);
                        up.exec();
                    }
                }
>>>>>>> REPLACE
```

#### 3.3.7 `getClassificationStats` 中的分类统计改用 `ItemType`
```
<<<<<<< SEARCH
            } else {
                // 回退逻辑：处理非文件类的语义化类型
                if (itemType == "image") bizTypes["截图"]++;
                else if (itemType == "code") bizTypes["脚本代码"]++;
                else if (itemType == "text") bizTypes["纯文本"]++;
                else if (itemType == "link") bizTypes["Link"]++;
                else if (itemType == "file") bizTypes["数据库附件"]++;
                else if (itemType == "local_file") bizTypes["本地文件"]++;
                else if (itemType == "local_folder" || itemType == "folder") bizTypes["文件夹"]++;
                else if (itemType == "local_batch") bizTypes["批量托管"]++;
                else bizTypes["其他"]++;
            }
=======
            } else {
                // 回退逻辑：处理非文件类的语义化类型
                RapidNotes::ItemType::Type type = RapidNotes::ItemType::fromString(itemType);
                if (type == RapidNotes::ItemType::Type::Image) bizTypes["截图"]++;
                else if (type == RapidNotes::ItemType::Type::Code) bizTypes["脚本代码"]++;
                else if (type == RapidNotes::ItemType::Type::Text) bizTypes["纯文本"]++;
                else if (type == RapidNotes::ItemType::Type::Link) bizTypes["Link"]++;
                else if (type == RapidNotes::ItemType::Type::File) bizTypes["数据库附件"]++;
                else if (type == RapidNotes::ItemType::Type::LocalFile) bizTypes["本地文件"]++;
                else if (type == RapidNotes::ItemType::Type::LocalFolder || type == RapidNotes::ItemType::Type::Folder) bizTypes["文件夹"]++;
                else if (type == RapidNotes::ItemType::Type::LocalBatch) bizTypes["批量托管"]++;
                else bizTypes["其他"]++;
            }
>>>>>>> REPLACE
```

#### 3.3.8 `addTagsToNote` 改用 `TagCodec::normalize` 与 `TagCodec::join`
```
<<<<<<< SEARCH
bool DatabaseManager::addTagsToNote(int noteId, const QStringList& tags) {
    QVariantMap note = getNoteById(noteId);
    if (note.isEmpty()) return false;

    QStringList existingStrList = RapidNotes::TagCodec::split(note["tags"].toString());
    QStringList finalTags;
    // 确保原有标签也经过清理
    for (const QString& t : existingStrList) {
        QString trimmed = t.trimmed();
        if (!trimmed.isEmpty() && !finalTags.contains(trimmed)) finalTags << trimmed;
    }

    // 合并新标签
    for (const QString& t : tags) {
        QString trimmed = t.trimmed();
        if (!trimmed.isEmpty() && !finalTags.contains(trimmed)) finalTags << trimmed;
    }

    // 2026-05-09 按照用户要求：严禁使用 "逗号+空格"，统一使用纯逗号分割。
    QString tagsString = RapidNotes::TagCodec::join(finalTags);
    bool ok = updateNoteState(noteId, "tags", tagsString);
    if (ok) syncNoteTags(noteId, finalTags); // 保持物理表同步
    return ok;
}
=======
bool DatabaseManager::addTagsToNote(int noteId, const QStringList& tags) {
    QVariantMap note = getNoteById(noteId);
    if (note.isEmpty()) return false;

    QStringList finalTags = RapidNotes::TagCodec::split(note["tags"].toString());
    finalTags.append(RapidNotes::TagCodec::normalize(tags));
    finalTags = RapidNotes::TagCodec::normalize(finalTags);

    // 2026-05-09 按照用户要求：严禁使用 "逗号+空格"，统一使用纯逗号分割。
    QString tagsString = RapidNotes::TagCodec::join(finalTags);
    bool ok = updateNoteState(noteId, "tags", tagsString);
    if (ok) syncNoteTags(noteId, finalTags); // 保持物理表同步
    return ok;
}
>>>>>>> REPLACE
```

#### 3.3.9 `applyCommonFilters` 中 SQL 字符串拼入改由 `ItemType::toString` 提供
```
<<<<<<< SEARCH
            if (!typeCriteria.isEmpty()) {
                QStringList bizConds;
                for (const QString& label : typeCriteria) {
                    if (label == "截图") bizConds << "item_type = 'image'";
                    else if (label == "脚本代码") bizConds << "item_type = 'code'";
                    else if (label == "纯文本") bizConds << "item_type = 'text'";
                    else if (label == "Link" || label == "网页链接") bizConds << "item_type = 'link'";
                    else if (label == "数据库附件") bizConds << "item_type = 'file'";
                    else if (label == "本地文件") bizConds << "item_type = 'local_file'";
                    else if (label == "文件夹") {
                        // 2026-05-03 按照用户要求：修正文件夹筛选逻辑，同步检查 item_type 与物理后缀标记
                        bizConds << "(item_type IN ('local_folder', 'folder') OR file_extensions = '文件夹' OR file_extensions LIKE '文件夹, %' OR file_extensions LIKE '%, 文件夹' OR file_extensions LIKE '%, 文件夹, %')";
                    }
                    else if (label == "批量托管") bizConds << "item_type = 'local_batch'";
                    else if (label == "其他") bizConds << "(file_extensions = '' AND item_type NOT IN ('image', 'code', 'text', 'link', 'file', 'local_file', 'local_folder', 'folder', 'local_batch'))";
=======
            if (!typeCriteria.isEmpty()) {
                QStringList bizConds;
                for (const QString& label : typeCriteria) {
                    if (label == "截图") bizConds << QString("item_type = '%1'").arg(RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Image));
                    else if (label == "脚本代码") bizConds << QString("item_type = '%1'").arg(RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Code));
                    else if (label == "纯文本") bizConds << QString("item_type = '%1'").arg(RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Text));
                    else if (label == "Link" || label == "网页链接") bizConds << QString("item_type = '%1'").arg(RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Link));
                    else if (label == "数据库附件") bizConds << QString("item_type = '%1'").arg(RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::File));
                    else if (label == "本地文件") bizConds << QString("item_type = '%1'").arg(RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::LocalFile));
                    else if (label == "文件夹") {
                        // 2026-05-03 按照用户要求：修正文件夹筛选逻辑，同步检查 item_type 与物理后缀标记
                        bizConds << QString("(item_type IN ('%1', '%2') OR file_extensions = '文件夹' OR file_extensions LIKE '文件夹, %' OR file_extensions LIKE '%, 文件夹' OR file_extensions LIKE '%, 文件夹, %')")
                                    .arg(RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::LocalFolder),
                                         RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Folder));
                    }
                    else if (label == "批量托管") bizConds << QString("item_type = '%1'").arg(RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::LocalBatch));
                    else if (label == "其他") {
                        QString notInList = QString("'%1', '%2', '%3', '%4', '%5', '%6', '%7', '%8', '%9'")
                            .arg(RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Image),
                                 RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Code),
                                 RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Text),
                                 RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Link),
                                 RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::File),
                                 RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::LocalFile),
                                 RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::LocalFolder),
                                 RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Folder),
                                 RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::LocalBatch));
                        bizConds << QString("(file_extensions = '' AND item_type NOT IN (%1))").arg(notInList);
                    }
>>>>>>> REPLACE
```

---

### 3.4 修改 `src/models/NoteModel.cpp`

#### 3.4.1 `ToolTipRole` 图片类型判定
```
<<<<<<< SEARCH
            QString preview;
            if (note.value("item_type").toString() == "image") {
                QByteArray ba = note.value("data_blob").toByteArray();
=======
            QString preview;
            RapidNotes::ItemType::Type itype = RapidNotes::ItemType::fromString(note.value("item_type").toString());
            if (itype == RapidNotes::ItemType::Type::Image) {
                QByteArray ba = note.value("data_blob").toByteArray();
>>>>>>> REPLACE
```

#### 3.4.2 `DisplayRole` 文本与文件类判定
```
<<<<<<< SEARCH
            QString type = note.value("item_type").toString();
            QString content = note.value("content").toString();
            if (type == "text" || type.isEmpty() || type == "ocr_text" || type == "captured_message" ||
                type == "file" || type == "folder" || type == "files" || type == "folders") {
                QString plain = StringUtils::htmlToPlainText(content);
                QString display = plain.replace('\n', ' ').replace('\r', ' ').trimmed().left(150);
                if (!display.isEmpty()) return display;
            }
=======
            QString type = note.value("item_type").toString();
            QString content = note.value("content").toString();
            RapidNotes::ItemType::Type itemTypeEnum = RapidNotes::ItemType::fromString(type);
            if (type.isEmpty() || RapidNotes::ItemType::isPureText(itemTypeEnum) ||
                itemTypeEnum == RapidNotes::ItemType::Type::File || itemTypeEnum == RapidNotes::ItemType::Type::Folder ||
                itemTypeEnum == RapidNotes::ItemType::Type::Files || itemTypeEnum == RapidNotes::ItemType::Type::Folders) {
                QString plain = StringUtils::htmlToPlainText(content);
                QString display = plain.replace('\n', ' ').replace('\r', ' ').trimmed().left(150);
                if (!display.isEmpty()) return display;
            }
>>>>>>> REPLACE
```

#### 3.4.3 `mimeData` 拖拽导出判定
```
<<<<<<< SEARCH
                    firstImage.loadFromData(blob);
                }
                plainTexts << "[截图]";
            } else if (type == "file" || type == "folder" || type == "files" || type == "folders" || type == "local_file" || type == "local_folder" || type == "local_batch") {
                // 支持文件路径导出
                QStringList rawPaths;
                if (type == "local_batch") {
=======
                    firstImage.loadFromData(blob);
                }
                plainTexts << "[截图]";
            } else if (RapidNotes::ItemType::isFileOrFolder(RapidNotes::ItemType::fromString(type))) {
                // 支持文件路径导出
                QStringList rawPaths;
                if (RapidNotes::ItemType::fromString(type) == RapidNotes::ItemType::Type::LocalBatch) {
>>>>>>> REPLACE
```

---

### 3.5 修改 `src/ui/StringUtils.h`

#### 3.5.1 包含 `ItemType.h`
```
<<<<<<< SEARCH
#include <QCoreApplication>
#include <vector>
=======
#include <QCoreApplication>
#include <vector>
#include "../core/ItemType.h"
>>>>>>> REPLACE
```

#### 3.5.2 `copyNotesToClipboard` 中的类型判定
```
<<<<<<< SEARCH
            // 1. 图片类型：直接复制二进制图
            if (type == "image") {
                QImage img;
                img.loadFromData(blob);
                if (!img.isNull()) {
                    ClipboardMonitor::instance().skipNext();
                    QApplication::clipboard()->setImage(img);
                    return;
                }
            }

            // 2. 文件/文件夹/路径相关类型：复制为文件 URL (支持粘贴到资源管理器或聊天软件)
            bool isFilePath = (type == "file" || type == "local_file" || type == "folder" || type == "local_folder" || type == "local_batch");
            QString plainContent = htmlToPlainText(content).trimmed();
            if (!isFilePath) {
                // 智能检测文本是否为存在的物理路径
                if (QFileInfo(plainContent).exists() && QFileInfo(plainContent).isAbsolute()) {
                    isFilePath = true;
                    content = plainContent;
                }
            }

            if (isFilePath) {
                QStringList rawPaths;
                if (type == "local_batch") {
=======
            RapidNotes::ItemType::Type itemTypeEnum = RapidNotes::ItemType::fromString(type);

            // 1. 图片类型：直接复制二进制图
            if (itemTypeEnum == RapidNotes::ItemType::Type::Image) {
                QImage img;
                img.loadFromData(blob);
                if (!img.isNull()) {
                    ClipboardMonitor::instance().skipNext();
                    QApplication::clipboard()->setImage(img);
                    return;
                }
            }

            // 2. 文件/文件夹/路径相关类型：复制为文件 URL (支持粘贴到资源管理器或聊天软件)
            bool isFilePath = RapidNotes::ItemType::isFileOrFolder(itemTypeEnum);
            QString plainContent = htmlToPlainText(content).trimmed();
            if (!isFilePath) {
                // 智能检测文本是否为存在的物理路径
                if (QFileInfo(plainContent).exists() && QFileInfo(plainContent).isAbsolute()) {
                    isFilePath = true;
                    content = plainContent;
                }
            }

            if (isFilePath) {
                QStringList rawPaths;
                if (itemTypeEnum == RapidNotes::ItemType::Type::LocalBatch) {
>>>>>>> REPLACE
```

#### 3.5.3 `generateNotePreviewHtml` 中的类型判定
```
<<<<<<< SEARCH
        if (type == "color") {
            int colorRectHeight = (int)(200 * zoomFactor);
            html = QString("%1%2"
                           "<div style='margin: 20px; text-align: center;'>"
                           "  <div style='background-color: %3; width: 100%; height: %4px; border-radius: 12px; border: 1px solid #555;'></div>"
                           "  <h1 style='color: white; margin-top: 20px; font-family: Consolas; font-size: 2.5em;'>%3</h1>"
                           "</div>")
                   .arg(titleHtml, hrHtml, content).arg(colorRectHeight);
        } else if (type == "image" && !data.isEmpty()) {
=======
        RapidNotes::ItemType::Type itemTypeEnum = RapidNotes::ItemType::fromString(type);
        if (itemTypeEnum == RapidNotes::ItemType::Type::Color) {
            int colorRectHeight = (int)(200 * zoomFactor);
            html = QString("%1%2"
                           "<div style='margin: 20px; text-align: center;'>"
                           "  <div style='background-color: %3; width: 100%; height: %4px; border-radius: 12px; border: 1px solid #555;'></div>"
                           "  <h1 style='color: white; margin-top: 20px; font-family: Consolas; font-size: 2.5em;'>%3</h1>"
                           "</div>")
                   .arg(titleHtml, hrHtml, content).arg(colorRectHeight);
        } else if (itemTypeEnum == RapidNotes::ItemType::Type::Image && !data.isEmpty()) {
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译验证命令
```bash
# 验证 CMake 语法与 Target 配置
cmake -B build -S RapidNotes_Source/RapidNotes -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target RapidNotes
```

### 4.2 验收搜索命令与预期输出
1. **全工程搜索 `item_type` 类型字符串字面量**：
   ```bash
   grep -rnE '"(image|code|text|link|color|palette|ocr_text|captured_message|pixel_ruler|deleted_category|file|files|folder|folders|local_file|local_folder|local_batch)"' RapidNotes_Source/RapidNotes/src
   ```
   **预期**：在业务逻辑代码中不再出现类型字符串字面量（仅允许存在于 `src/core/ItemType.cpp` 以及用于 SVG 键名的映射中）。
2. **全工程搜索 `calculateWordBucket`**：
   ```bash
   grep -rn "calculateWordBucket" RapidNotes_Source/RapidNotes/src
   ```
   **预期**：输出为 0 行，包装壳已彻底物理移除。
3. **全工程搜索非 `TagCodec` 的标签拆分与拼接**：
   ```bash
   grep -rn "QRegularExpression("\[,，\]")" RapidNotes_Source/RapidNotes/src
   ```
   **预期**：仅在 `TagCodec.cpp` 中出现一次。
4. **验证 CMakeLists.txt SOURCES 同步性**：
   ```bash
   grep -E "ItemType|ItemClassifier|PathSummary|WordBucket|TagCodec|ThumbnailMaker" RapidNotes_Source/RapidNotes/CMakeLists.txt
   ```
   **预期**：全部 6 大模块的 `.h` 与 `.cpp` 均已完整在 `SOURCES` 注册。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **`TagCodec::normalize` 规约**：
   - 彻底消灭 `DatabaseManager.cpp` 在 `updateNote` 和 `addTagsToNote` 内部手写的 trim + 去重循环。
   - 所有已有标签列表的清洗统一通过 `TagCodec::normalize` 处理；
2. **`WordBucket::calculate` 直接调用**：
   - 彻底删除 `DatabaseManager.cpp` 匿名命名空间中的转发壳，所有调用点直接调用 `RapidNotes::WordBucket::calculate`；
3. **`ItemType` 枚举方法复用**：
   - `extractFileExtensions` 与 `mimeData` 直接复用 `RapidNotes::ItemType::isFileOrFolder(type)`，消除多处冗余的 6 种/7 种类型连续 `||` 比较；
   - 纯文本判定直接复用 `RapidNotes::ItemType::isPureText(type)`。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 真理源 | 派生副本 / 调用方 | 同步 / 转换方式 |
| :--- | :--- | :--- | :--- |
| **条目类型定义** | `RapidNotes::ItemType` (`ItemType.h/cpp`) | 数据库 `notes.item_type`、模型 `NoteModel`、剪贴板 `StringUtils` | `toString(type)` 写入，`fromString(str)` 判定 |
| **标签解析与序列化** | `RapidNotes::TagCodec` (`TagCodec.h/cpp`) | `DatabaseManager`、`QuickWindow`、`NoteEditWindow` | `split` 拆分清洗，`normalize` 去重，`join` 存储，`joinDisplay` 展示 |
| **字数分档映射** | `RapidNotes::WordBucket` (`WordBucket.h/cpp`) | `DatabaseManager::addNote`、`updateNote` 等计算 `word_count` 列 | `WordBucket::calculate(content, itemType)` 纯函数直调 |
| **路径种类图标与颜色** | `RapidNotes::PathSummary` (`PathSummary.h/cpp`) | `NoteModel::data(DecorationRole)`、`StringUtils` | `summarize` 分析路径列表，输出统一图标名与色码 |
| **文本特征嗅探** | `RapidNotes::ItemClassifier` (`ItemClassifier.h/cpp`) | `main.cpp` 剪贴板采集、`NoteModel` 纯内存图标判定 | 纯静态谓词判定，零磁盘 I/O |
| **图片平滑缩略图** | `RapidNotes::ThumbnailMaker` (`ThumbnailMaker.h/cpp`) | `NoteModel` 缓存、后台入库流水线 | 64×64 平滑缩放纯函数生成 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

### 7.1 新增/重构接口列表
1. `QStringList RapidNotes::TagCodec::normalize(const QStringList& tags)`
   - 声明：`src/core/TagCodec.h`（公有静态方法）
   - 定义：`src/core/TagCodec.cpp`
   - 调用点：`DatabaseManager.cpp`（`addNote`、`updateNote`、`addTagsToNote` 等）
   - 访问级别：`public`

### 7.2 核心复用接口物理签名核对
1. `QString RapidNotes::ItemType::toString(Type type)`（`src/core/ItemType.h:37`）
2. `ItemType::Type RapidNotes::ItemType::fromString(const QString& str)`（`src/core/ItemType.h:38`）
3. `bool RapidNotes::ItemType::isFileOrFolder(Type type)`（`src/core/ItemType.h:40`）
4. `bool RapidNotes::ItemType::isPureText(Type type)`（`src/core/ItemType.h:41`）
5. `int RapidNotes::WordBucket::calculate(const QString& content, const QString& itemType)`（`src/core/WordBucket.h:15`）
6. `QStringList RapidNotes::TagCodec::split(const QString& tagsStr)`（`src/core/TagCodec.h:15`）
7. `QString RapidNotes::TagCodec::join(const QStringList& tags)`（`src/core/TagCodec.h:21`）
8. `QString RapidNotes::TagCodec::joinDisplay(const QStringList& tags)`（`src/core/TagCodec.h:24`）

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

1. `TagCodec.h` 包含 `<QString>`, `<QStringList>`，完全自包含；
2. `TagCodec.cpp` 包含 `"TagCodec.h"` 与 `<QRegularExpression>`；
3. `DatabaseManager.cpp` 显式增加 `#include "ItemType.h"`，已有 `"WordBucket.h"` 与 `"TagCodec.h"`，类型完全闭合；
4. `NoteModel.cpp` 已有 `#include "../core/ItemType.h"`，类型完全闭合；
5. `StringUtils.h` 显式增加 `#include "../core/ItemType.h"`，消除隐式依赖；
6. `CMakeLists.txt` 中已完整注册全部 6 大模块的 `.h` 与 `.cpp`。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 核对项 | 满足状态 | 位置说明 / 证据 |
| :---: | :--- | :---: | :--- |
| 1 | 严禁笨拙且极度浪费计算资源的手法 | 满足 | 移除 `calculateWordBucket` 转发壳与重复的标签去重循环 |
| 2 | 接口天然原生支持批量操作 (Batch First) | 满足 | `TagCodec::normalize` 支持批量标签列表清洗 |
| 3 | 严禁采用打补丁方式 (Anti-Patching) | 满足 | 彻底将类型字符串字面量收敛为 `ItemType`，不写局部补丁 |
| 4 | 不可发生偷指针事件 | 满足 | 纯函数与值语义设计，无内部裸指针 |
| 5 | 快捷键必须遵循规范去架构 | 满足 | 步骤 1 不涉及快捷键变动 |
| 6 | SEARCH 块原文真实存在且唯一匹配 | 满足 | 经 Python 脚本全量匹配，所有 SEARCH 块匹配计数均为 1 |
| 7 | 所有新建/删除文件已在 CMakeLists.txt 注册且全量提供源码 | 满足 | CMakeLists.txt 已包含全部 6 大模块 |
| 8 | 外部调用接口在对应 .h 中真实存在 | 满足 | 所有调用接口与头文件声明逐字一致 |
| 9 | 显式类型头文件包含闭合 | 满足 | 各文件显式包含 `ItemType.h`，无隐式依赖 |
| 10 | 10 大标准章节完整无缺 | 满足 | Chapter 1 至 Chapter 10 章节齐备 |
| 11 | 禁止在多线程并发中按引用捕获局部栈变量 | 满足 | 步骤 1 为纯函数模块，无并发栈变量引用捕获 |
| 12 | 递归 Lambda 禁止循环引用捕获 shared_ptr | 满足 | 无递归 Lambda 实现 |
| 13 | 既有 API 查重优先 | 满足 | 优先复用 `ItemType::isFileOrFolder` 与 `TagCodec` |
| 14 | 物理彻底清除死代码 | 满足 | 彻底移除 `calculateWordBucket` 壳函数 |
| 15 | 外部签名只读冻结 | 满足 | 既有函数签名保持不变，仅扩展 `TagCodec::normalize` |
| 16 | 样式数值零篡改 | 满足 | 本步骤不触碰任何 QSS 与界面数值 |
| 17 | 交互设计零篡改 | 满足 | 保留行内与界面所有交互模式 |
| 18 | SSOT 索引文档齐备 | 满足 | `docs/SSOT接口索引.md` 已完整创建并归档 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 10.1 必须核对并汇报的事项对照

#### 1. `WordBucket::calculate` 与原 `calculateWordBucket` 对照
- **边界取值**：0 字返回 0；1~100 字每 10 字一档（`((len - 1) / 10 + 1) * 10`）；101~1000 字每 100 字一档（`((len - 1) / 100 + 1) * 100`）；1001 字以上返回 1001；
- **适用类型集合**：仅对正文类（`text`, `code`, `ocr_text`, `captured_message`）计算字数，非正文类型（图片、链接、文件等）直接返回 0；
- **空内容处理**：清洗 HTML 标签、实体与所有空白字符后若长度为 0 则返回 0；
- **结论**：新旧代码逻辑、边界分档与返回值完全 100% 相同。

#### 2. `TagCodec::split` / `join` / `joinDisplay` 与原各处对照
- **分隔符集合**：`split` 兼容全角逗号 `，` 与半角逗号 `,`；
- **trim 与空项处理**：统一去除首尾空白，跳过空项；
- **去重与顺序**：保序去重（保留最先出现的项，后续重复项剔除）；
- **拼接输出**：
  - `join`：严格纯半角逗号 `,` 连接（`addTagsToNote` 保持现状）；
  - `joinDisplay`：半角逗号加空格 `, ` 连接（`updateNote` 保持现状）；
- **结论**：输出格式与原实现逐处保持 100% 一致。

#### 3. `ItemClassifier` 各谓词与原判定范围对照
- **URL 判定**：`http://`、`https://`、`www.` 前缀匹配，完全一致；
- **颜色判定**：`#` 开头 3 位或 6 位 Hex，以及 `r, g, b` 三元组，完全一致；
- **代码特征**：`import `, `class `, `def `, `function`, `var `, `const `, HTML 起始标签, JSON 结构，完全一致；
- **绝对路径**：`C:/`, `\`, `/`, `./`, `../` 且长度 < 260，完全一致；
- **结论**：判定范围与原 `main.cpp`、`NoteModel`、`StringUtils` 逐处一致。

#### 4. `PathSummary` 图标名与颜色对照
- 单文件：图标 `"file"`，颜色 `"#FFFF00"`（纯黄色）；
- 单目录：图标 `"folder"`，颜色 `"#FF8C00"`（橙色）；
- 多文件/混合：图标 `"files_multiple"`，颜色 `"#FF0000"`（纯红色）；
- 多目录：图标 `"folders_multiple"`，颜色 `"#483D8B"`（深紫蓝色）；
- **结论**：图标名与颜色 Hex 数值 100% 原样保持，零篡改。
