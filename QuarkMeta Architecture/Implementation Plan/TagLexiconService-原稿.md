# QuarkMeta 全局标签词库服务实施方案 (TagLexiconService)

## 1. 目标与范围
- 新建 `TagLexiconService`：统一收敛 SQLite `global.db` 词库数据表（`tags` 与 `tag_groups`）的增删改查、标签分组、颜色映射及前缀联想自动补全（`querySuggestions`），物理隔离磁盘 I/O。
- 净化 `CoreEngine.cpp`：彻底清除 `RenameTag` 与 `RemoveGlobalTag` 中扫描全盘 `.QuarkMeta.json` 的伪级联代码，使词库维护与文件标注彻底解耦。
- 改造 `TagManagerDialog` 与 `TagSelectorOverlay`：词库管理与输入联想 100% 接入 `TagLexiconService`。

---

## 2. 新增模块设计与完整代码实现

### 2.1 `src/core/TagLexiconService.h`
```cpp
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QMutex>

namespace QuarkMeta {

struct TagEntry {
    int id = -1;
    QString name;
    int groupId = -1;
    QString colorHex;
    int sortOrder = 0;
};

struct TagGroup {
    int id = -1;
    QString name;
    QString colorHex;
    int sortOrder = 0;
    QList<TagEntry> tags;
};

class TagLexiconService : public QObject {
    Q_OBJECT

public:
    static TagLexiconService& instance();

    // 1. 词条联想与查询 (供 MetaPanel / TagSelectorOverlay 极速自动补全)
    QStringList querySuggestions(const QString& prefix = "", int limit = 20) const;
    QList<TagGroup> getAllTagGroups() const;
    QStringList getAllTagNames() const;

    // 2. 词条管理 (只操作 global.db，不扫描物理目录)
    bool addTag(const QString& tagName, int groupId = -1, const QString& colorHex = "");
    bool renameTag(const QString& oldName, const QString& newName);
    bool deleteTag(const QString& tagName);
    bool setTagColor(const QString& tagName, const QString& colorHex);

    // 3. 标签分组管理
    bool createGroup(const QString& groupName, const QString& colorHex = "");
    bool renameGroup(int groupId, const QString& newName);
    bool deleteGroup(int groupId);
    bool moveTagToGroup(const QString& tagName, int targetGroupId);

signals:
    void lexiconChanged();

private:
    explicit TagLexiconService(QObject* parent = nullptr);
    ~TagLexiconService() override = default;
    TagLexiconService(const TagLexiconService&) = delete;
    TagLexiconService& operator=(const TagLexiconService&) = delete;

    void initSchema();
    mutable QMutex m_dbMutex;
};

} // namespace QuarkMeta
```

### 2.2 `src/core/TagLexiconService.cpp`
```cpp
#include "TagLexiconService.h"
#include "../meta/DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QMutexLocker>

namespace QuarkMeta {

TagLexiconService& TagLexiconService::instance() {
    static TagLexiconService s_instance;
    return s_instance;
}

TagLexiconService::TagLexiconService(QObject* parent) : QObject(parent) {
    initSchema();
}

void TagLexiconService::initSchema() {
    QMutexLocker locker(&m_dbMutex);
    QSqlQuery query(DatabaseManager::instance().database());

    query.exec("CREATE TABLE IF NOT EXISTS tag_groups ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "name TEXT UNIQUE NOT NULL, "
               "color TEXT, "
               "sort_order INTEGER DEFAULT 0)");

    query.exec("CREATE TABLE IF NOT EXISTS tags ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "name TEXT UNIQUE NOT NULL, "
               "group_id INTEGER DEFAULT -1, "
               "color TEXT, "
               "icon TEXT, "
               "pin_state INTEGER DEFAULT 0, "
               "use_count INTEGER DEFAULT 0, "
               "sort_order INTEGER DEFAULT 0)");

    query.exec("CREATE INDEX IF NOT EXISTS idx_tags_name ON tags(name)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_tags_group ON tags(group_id)");
}

QStringList TagLexiconService::querySuggestions(const QString& prefix, int limit) const {
    QMutexLocker locker(&m_dbMutex);
    QStringList result;
    QSqlQuery query(DatabaseManager::instance().database());

    if (prefix.trimmed().isEmpty()) {
        query.prepare("SELECT name FROM tags ORDER BY use_count DESC, id DESC LIMIT :limit");
        query.bindValue(":limit", limit);
    } else {
        query.prepare("SELECT name FROM tags WHERE name LIKE :pattern ORDER BY use_count DESC, id DESC LIMIT :limit");
        query.bindValue(":pattern", prefix.trimmed() + "%");
        query.bindValue(":limit", limit);
    }

    if (query.exec()) {
        while (query.next()) {
            result << query.value(0).toString();
        }
    }
    return result;
}

QList<TagGroup> TagLexiconService::getAllTagGroups() const {
    QMutexLocker locker(&m_dbMutex);
    QList<TagGroup> groups;
    QSqlQuery query(DatabaseManager::instance().database());

    // 1. 查询所有有效分组
    query.exec("SELECT id, name, color, sort_order FROM tag_groups ORDER BY sort_order ASC, id ASC");
    while (query.next()) {
        TagGroup g;
        g.id = query.value(0).toInt();
        g.name = query.value(1).toString();
        g.colorHex = query.value(2).toString();
        g.sortOrder = query.value(3).toInt();
        groups.append(g);
    }

    // 2. 查询所有标签并挂载到对应分组
    QSqlQuery tagQuery(DatabaseManager::instance().database());
    tagQuery.exec("SELECT id, name, group_id, color, sort_order FROM tags ORDER BY sort_order ASC, id ASC");
    
    TagGroup defaultGroup;
    defaultGroup.id = -1;
    defaultGroup.name = "默认标签";

    while (tagQuery.next()) {
        TagEntry e;
        e.id = tagQuery.value(0).toInt();
        e.name = tagQuery.value(1).toString();
        e.groupId = tagQuery.value(2).toInt();
        e.colorHex = tagQuery.value(3).toString();
        e.sortOrder = tagQuery.value(4).toInt();

        bool assigned = false;
        for (auto& g : groups) {
            if (g.id == e.groupId) {
                g.tags.append(e);
                assigned = true;
                break;
            }
        }
        if (!assigned) {
            defaultGroup.tags.append(e);
        }
    }

    if (!defaultGroup.tags.isEmpty()) {
        groups.prepend(defaultGroup);
    }

    return groups;
}

QStringList TagLexiconService::getAllTagNames() const {
    QMutexLocker locker(&m_dbMutex);
    QStringList names;
    QSqlQuery query(DatabaseManager::instance().database());
    if (query.exec("SELECT name FROM tags ORDER BY name ASC")) {
        while (query.next()) {
            names << query.value(0).toString();
        }
    }
    return names;
}

bool TagLexiconService::addTag(const QString& tagName, int groupId, const QString& colorHex) {
    QString cleanName = tagName.trimmed();
    if (cleanName.isEmpty()) return false;

    QMutexLocker locker(&m_dbMutex);
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("INSERT INTO tags (name, group_id, color) VALUES (:name, :group_id, :color) "
                  "ON CONFLICT(name) DO UPDATE SET use_count = use_count + 1");
    query.bindValue(":name", cleanName);
    query.bindValue(":group_id", groupId);
    query.bindValue(":color", colorHex);

    bool ok = query.exec();
    if (ok) emit lexiconChanged();
    return ok;
}

bool TagLexiconService::renameTag(const QString& oldName, const QString& newName) {
    QString cleanOld = oldName.trimmed();
    QString cleanNew = newName.trimmed();
    if (cleanOld.isEmpty() || cleanNew.isEmpty() || cleanOld == cleanNew) return false;

    QMutexLocker locker(&m_dbMutex);
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("UPDATE tags SET name = :newName WHERE name = :oldName");
    query.bindValue(":newName", cleanNew);
    query.bindValue(":oldName", cleanOld);

    bool ok = query.exec();
    if (ok) emit lexiconChanged();
    return ok;
}

bool TagLexiconService::deleteTag(const QString& tagName) {
    QString cleanName = tagName.trimmed();
    if (cleanName.isEmpty()) return false;

    QMutexLocker locker(&m_dbMutex);
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("DELETE FROM tags WHERE name = :name");
    query.bindValue(":name", cleanName);

    bool ok = query.exec();
    if (ok) emit lexiconChanged();
    return ok;
}

bool TagLexiconService::setTagColor(const QString& tagName, const QString& colorHex) {
    QString cleanName = tagName.trimmed();
    if (cleanName.isEmpty()) return false;

    QMutexLocker locker(&m_dbMutex);
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("UPDATE tags SET color = :color WHERE name = :name");
    query.bindValue(":color", colorHex);
    query.bindValue(":name", cleanName);

    bool ok = query.exec();
    if (ok) emit lexiconChanged();
    return ok;
}

bool TagLexiconService::createGroup(const QString& groupName, const QString& colorHex) {
    QString cleanName = groupName.trimmed();
    if (cleanName.isEmpty()) return false;

    QMutexLocker locker(&m_dbMutex);
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("INSERT INTO tag_groups (name, color) VALUES (:name, :color)");
    query.bindValue(":name", cleanName);
    query.bindValue(":color", colorHex);

    bool ok = query.exec();
    if (ok) emit lexiconChanged();
    return ok;
}

bool TagLexiconService::renameGroup(int groupId, const QString& newName) {
    QString cleanName = newName.trimmed();
    if (groupId <= 0 || cleanName.isEmpty()) return false;

    QMutexLocker locker(&m_dbMutex);
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("UPDATE tag_groups SET name = :name WHERE id = :id");
    query.bindValue(":name", cleanName);
    query.bindValue(":id", groupId);

    bool ok = query.exec();
    if (ok) emit lexiconChanged();
    return ok;
}

bool TagLexiconService::deleteGroup(int groupId) {
    if (groupId <= 0) return false;

    QMutexLocker locker(&m_dbMutex);
    QSqlDatabase db = DatabaseManager::instance().database();
    db.transaction();

    QSqlQuery updateTags(db);
    updateTags.prepare("UPDATE tags SET group_id = -1 WHERE group_id = :id");
    updateTags.bindValue(":id", groupId);
    updateTags.exec();

    QSqlQuery delGroup(db);
    delGroup.prepare("DELETE FROM tag_groups WHERE id = :id");
    delGroup.bindValue(":id", groupId);
    bool ok = delGroup.exec();

    if (ok) {
        db.commit();
        emit lexiconChanged();
    } else {
        db.rollback();
    }
    return ok;
}

bool TagLexiconService::moveTagToGroup(const QString& tagName, int targetGroupId) {
    QString cleanName = tagName.trimmed();
    if (cleanName.isEmpty()) return false;

    QMutexLocker locker(&m_dbMutex);
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("UPDATE tags SET group_id = :groupId WHERE name = :name");
    query.bindValue(":groupId", targetGroupId);
    query.bindValue(":name", cleanName);

    bool ok = query.exec();
    if (ok) emit lexiconChanged();
    return ok;
}

} // namespace QuarkMeta
```

---

## 3. `CoreEngine.cpp` 净化与解耦

修改 `CoreEngine::executeCommand`，彻底移除对全盘扫描 `MetadataManager::renameTag` 和 `removeTag` 的违规调用：

```cpp
#include "TagLexiconService.h"

bool CoreEngine::executeCommand(const AppCommand& cmd) {
    // ... [前置校验保持不变] ...

    switch (cmd.type) {
    // ...
    case AppCommandType::AddTag: {
        QString tag = cmd.params.value("tag").toString().trimmed();
        if (tag.isEmpty()) break;

        // 1. 向全局词库登记新词条 (仅写 SQLite 词典)
        TagLexiconService::instance().addTag(tag, -1);

        // 2. 仅为当前选中的物理文件打标 (.QuarkMeta.json)
        for (const QString& path : cmd.targetPaths) {
            auto meta = MetadataManager::instance().getMeta(path.toStdWString());
            QStringList curTags = meta.tags;
            if (!curTags.contains(tag)) {
                curTags.append(tag);
                MetadataManager::instance().setTags(path.toStdWString(), curTags, false);
            }
        }
        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        ev.paths = cmd.targetPaths;
        CentralEventHub::instance().publishEvent(ev);
        break;
    }
    case AppCommandType::RemoveTag: {
        QString tag = cmd.params.value("tag").toString().trimmed();
        // 仅为当前选中的物理文件解绑标签，绝不删 SQLite 词库词条
        for (const QString& path : cmd.targetPaths) {
            auto meta = MetadataManager::instance().getMeta(path.toStdWString());
            QStringList curTags = meta.tags;
            curTags.removeAll(tag);
            MetadataManager::instance().setTags(path.toStdWString(), curTags, false);
        }
        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        ev.paths = cmd.targetPaths;
        CentralEventHub::instance().publishEvent(ev);
        break;
    }
    case AppCommandType::RenameTag: {
        QString oldTag = cmd.params.value("oldTag").toString().trimmed();
        QString newTag = cmd.params.value("newTag").toString().trimmed();
        // 🚀 仅重命名 SQLite 词库中的词条，彻底删除全盘扫描代码！
        TagLexiconService::instance().renameTag(oldTag, newTag);
        break;
    }
    case AppCommandType::RemoveGlobalTag: {
        QString tag = cmd.params.value("tag").toString().trimmed();
        // 🚀 仅从 SQLite 词库中注销该词条，彻底删除全盘扫描代码！
        TagLexiconService::instance().deleteTag(tag);
        break;
    }
    // ... 其余分支保持不变 ...
    }
    return true;
}
```

---

## 4. UI 层（`TagSelectorOverlay` / `MetaPanel`）接入

### 4.1 `TagSelectorOverlay.cpp` 极速前缀联想
```cpp
#include "TagLexiconService.h"

void TagSelectorOverlay::updateSuggestions(const QString& prefix) {
    // 毫秒级直接从 TagLexiconService 提取联想列表
    QStringList candidates = TagLexiconService::instance().querySuggestions(prefix, 15);
    populateList(candidates);
}
```

### 4.2 `TagManagerController.cpp` 接入
```cpp
#include "TagLexiconService.h"

// 标签管理器中的增、删、改、分组移动全部 1 行直连 TagLexiconService：
void TagManagerController::handleAddTag(const QString& name, int groupId) {
    TagLexiconService::instance().addTag(name, groupId);
}

void TagManagerController::handleDeleteTag(const QString& name) {
    TagLexiconService::instance().deleteTag(name);
}

void TagManagerController::handleRenameTag(const QString& oldName, const QString& newName) {
    TagLexiconService::instance().renameTag(oldName, newName);
}
```

---

## 5. `CMakeLists.txt` 构建配置注册
```cmake
set(CORE_SOURCES
    # ... 现有源文件 ...
    src/core/TagLexiconService.h
    src/core/TagLexiconService.cpp
)
```