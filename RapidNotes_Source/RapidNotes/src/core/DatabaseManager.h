#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QVariant>
#include <QVariantList>
#include <QRecursiveMutex>
#include <QStringList>
#include <QSet>
#include <QMutex>
#include <QTimer>
#include <mutex>
#include "../meta/sqlite3.h"

class SqlTransaction {
public:
    explicit SqlTransaction(sqlite3* db);
    ~SqlTransaction();

    bool commit();
    void rollback();

private:
    sqlite3* m_db;
    bool m_committed = false;
    bool m_isNested = false;
};

namespace RapidNotes {
    using ::SqlTransaction;
}

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    enum MoveDirection { Up, Down, Top, Bottom };
    static constexpr int DEFAULT_PAGE_SIZE = 500;
    
    static DatabaseManager& instance();

    // 统一初始化与退出
    bool init(const QString& dbPath = QString());
    void shutdown();
    void closeAndPack() { shutdown(); }

    QString getLastError() const { return m_lastError; }
    void logStartup(const QString& msg);

    // 路径单一真实数据源 (SSOT)
    static QString getAppDir();
    static QString getGlobalDbPath();
    static QString getBackupDir();
    QString getDbPath() const { return getGlobalDbPath(); }

    // 原生句柄与互斥锁 (供 QuarkMeta 全套 DAO 使用，杜绝野指针)
    sqlite3* getGlobalDb();
    static std::mutex& getGlobalMutex() { return instance().m_globalDbMutex; }
    static void logSqlError(const char* location, sqlite3* db, const QString& sql = QString());

    // Qt SQL 连接与锁 (供 RapidNotes 笔记模块使用)
    QSqlDatabase database() const { return m_db; }
    QRecursiveMutex* dbMutex() { return &m_mutex; }
    
    // 笔记与分类 CRUD 操作
    int addNote(const QString& title, const QString& content, const QStringList& tags = QStringList(), 
                const QString& color = "", int categoryId = -1, 
                const QString& itemType = "text", const QByteArray& dataBlob = QByteArray(),
                const QString& sourceApp = "", const QString& sourceTitle = "",
                const QString& remark = "");
    bool updateNote(int id, const QString& title, const QString& content, const QStringList& tags, 
                    const QString& color = "", int categoryId = -1,
                    const QString& itemType = "text", const QByteArray& dataBlob = QByteArray(),
                    const QString& sourceApp = "", const QString& sourceTitle = "",
                    const QString& remark = "");
    bool deleteNotesBatch(const QList<int>& ids);
    bool updateNoteState(int id, const QString& column, const QVariant& value);
    bool updateNoteStateBatch(const QList<int>& ids, const QString& column, const QVariant& value);
    bool softDeleteNotes(const QList<int>& ids);
    bool toggleNoteState(int id, const QString& column);
    bool moveNotesToCategory(const QList<int>& noteIds, int catId);
    bool recordAccess(int id);
    bool recordAccessBatch(const QList<int>& ids);
    bool toggleNoteFavoriteBatch(const QList<int>& ids);
    bool moveNote(int id, MoveDirection direction, const QString& filterType, const QVariant& filterValue, const QVariantMap& criteria = QVariantMap());
    bool moveNotesToRow(const QList<int>& ids, int targetRow, const QString& filterType, const QVariant& filterValue, const QVariantMap& criteria = QVariantMap());
    bool reorderNotes(const QString& filterType, const QVariant& filterValue, bool ascending, const QVariantMap& criteria = QVariantMap());

    // 分类管理
    int addCategory(const QString& name, int parentId = -1, const QString& color = "");
    int getOrCreateCategoryByName(const QString& name, int parentId = -1, const QString& color = "");
    bool renameCategory(int id, const QString& name);
    bool toggleCategoryPinned(int id);
    bool setCategoryColor(int id, const QString& color);
    bool softDeleteCategories(const QList<int>& ids);
    bool hardDeleteCategories(const QList<int>& ids);
    bool restoreCategories(const QList<int>& ids);
    bool moveCategory(int id, MoveDirection direction);
    bool reorderCategories(int parentId, bool ascending);
    bool reorderAllCategories(bool ascending);
    QString getCategorySortMode(int catId);
    bool setCategorySortMode(int catId, const QString& mode);
    bool updateCategoryOrder(int parentId, const QList<int>& categoryIds);
    bool updatePinnedCategoryOrder(const QList<int>& categoryIds);
    QList<QVariantMap> getAllCategories();
    QList<QVariantMap> getChildCategories(int parentId);
    bool emptyTrash();
    bool restoreAllFromTrash();

    // 密码保护与分类锁定
    bool setCategoryPassword(int id, const QString& password, const QString& hint);
    bool removeCategoryPassword(int id);
    bool verifyCategoryPassword(int id, const QString& password);
    bool isCategoryLocked(int id);
    void lockCategory(int id);
    void lockAllCategories();
    void unlockCategory(int id);
    bool isLockedCategoriesHidden() const { return m_lockedCategoriesHidden; }
    void toggleLockedCategoriesVisibility();

    // 预设标签
    bool setCategoryPresetTags(int catId, const QString& tags);
    QString getCategoryPresetTags(int catId);

    // 标签字典管理
    bool addTagsToNote(int noteId, const QStringList& tags);
    bool addTagsToNotesBatch(const QList<int>& noteIds, const QStringList& tags);
    bool removeTagsFromNotesBatch(const QList<int>& noteIds, const QStringList& tags);
    bool renameTagGlobally(const QString& oldName, const QString& newName);
    bool deleteTagGlobally(const QString& tagName);
    void syncNoteTags(int noteId, const QStringList& tags);

    // 查询与统计
    QList<QVariantMap> searchNotes(const QString& keyword, const QString& filterType = "all", const QVariant& filterValue = -1, int page = -1, int pageSize = DEFAULT_PAGE_SIZE, const QVariantMap& criteria = QVariantMap());
    QList<QVariantMap> searchNotesSummary(const QString& keyword, const QString& filterType = "all", const QVariant& filterValue = -1, int page = -1, int pageSize = DEFAULT_PAGE_SIZE, const QVariantMap& criteria = QVariantMap());
    int getNotesCount(const QString& keyword, const QString& filterType = "all", const QVariant& filterValue = -1, const QVariantMap& criteria = QVariantMap());
    QStringList getAllTags();
    QList<QVariantMap> getRecentTagsWithCounts(int limit = 20);
    QVariantMap getNoteById(int id);
    QList<QVariantMap> getNotesByIds(const QList<int>& ids);   // 批量读取，保持传入顺序
    int getLastCreatedNoteId();
    QVariantMap getCounts();
    QVariantMap getFilterStats(const QString& keyword = "", const QString& filterType = "all", const QVariant& filterValue = -1, const QVariantMap& criteria = QVariantMap());

    // 待办事项管理
    struct Todo {
        int id = -1;
        QString title;
        QString content;
        QDateTime startTime;
        QDateTime endTime;
        int status = 0;
        QDateTime reminderTime;
        int priority = 0;
        QString color;
        int noteId = -1;
        int repeatMode = 0;
        int parentId = -1;
        int progress = 0;
        QDateTime createdAt;
        QDateTime updatedAt;
    };
    int addTodo(const Todo& todo);
    bool updateTodo(const Todo& todo);
    bool deleteTodo(int id);
    bool deleteTodosBatch(const QList<int>& ids);
    QList<Todo> getTodosByDate(const QDate& date);
    QList<Todo> getAllPendingTodos();
    QList<Todo> getAllTodos();

    // 授权
    QVariantMap getTrialStatus(bool validate = true);
    void resetActivation();
    bool verifyActivationCode(const QString& code);

    // 异步排队操作
    void addNoteAsync(const QString& title, const QString& content, const QStringList& tags = QStringList(),
                      const QString& color = "", int categoryId = -1,
                      const QString& itemType = "text", const QByteArray& dataBlob = QByteArray(),
                      const QString& sourceApp = "", const QString& sourceTitle = "",
                      const QString& remark = "");

    void beginBatch();
    void endBatch();
    void rollbackBatch();

    bool isAutoCategorizeEnabled() const { return m_autoCategorizeEnabled; }
    void setAutoCategorizeEnabled(bool enabled);
    int activeCategoryId() const { return m_activeCategoryId; }
    void setActiveCategoryId(int id);

    QString getCategoryNameById(int id);
    QVariantMap getRootCategory(int catId);
    void notifyAppLockSettingsChanged() { emit appLockSettingsChanged(); }

    static void performDailyBackup();
    void backupDatabase();   // 按时间戳全量归档备份（保留最近3份）

signals:
    void noteAdded(const QVariantMap& note);
    void noteUpdated();
    void categoriesChanged();
    void todoChanged();
    void autoCategorizeEnabledChanged(bool enabled);
    void activeCategoryIdChanged(int id);
    void appLockSettingsChanged();

private:
    DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager() override;
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool initNativeDb(const QString& dbPath);
    bool initQtSqlDb(const QString& dbPath);
    bool createAllTables();
    bool renameLegacyNoteTagDict();
    bool migrateNoteTagsSchema();
    void ensureTagsExist(const QStringList& names);
    void refreshNoteTagsCache(const QList<int>& ids);
    void refreshNoteColors(const QList<int>& ids);
    bool ensureSearchIndex();
    void appendKeywordFilter(QString& whereClause, QVariantList& params, const QString& keyword);
    bool m_ftsAvailable = false;

    void applySecurityFilter(QString& whereClause, QVariantList& params, const QString& filterType);
    void applyCommonFilters(QString& whereClause, QVariantList& params, const QString& filterType, const QVariant& filterValue, const QVariantMap& criteria);
    void backupDatabaseLatest();
    bool flushDatabase(const QString& source = "Unknown");
    bool tryRecoverFromBackup();
    void handleAutoSave();
    void markDirty();
    void saveTrialToFile(const QVariantMap& status);
    QVariantMap loadTrialFromFile();

    // 原生 SQLite 连接 (QuarkMeta 核心)
    sqlite3* m_nativeDb = nullptr;
    std::mutex m_globalDbMutex;

    // Qt SQL 连接 (RapidNotes 核心)
    QSqlDatabase m_db;
    QString m_dbPath;
    QString m_lastError;
    QRecursiveMutex m_mutex;

    QTimer* m_autoSaveTimer = nullptr;
    bool m_isDirty = false;
    QDateTime m_lastActivityTime;
    
    bool m_isBatchMode = false;
    bool m_isInitialized = false;
    QVariantMap m_cachedTrialStatus;
    QSet<int> m_unlockedCategories;
    
    bool m_autoCategorizeEnabled = false;
    int m_activeCategoryId = -1;
    bool m_lockedCategoriesHidden = false;

    static QStringList s_tagClipboard;
    static QMutex s_tagClipboardMutex;

public:
    static void setTagClipboard(const QStringList& tags);
    static QStringList getTagClipboard();
};

namespace RapidNotes {
    using ::DatabaseManager;
}

#endif // DATABASEMANAGER_H