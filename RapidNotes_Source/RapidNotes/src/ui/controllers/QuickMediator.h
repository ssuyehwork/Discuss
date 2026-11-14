#ifndef QUICKMEDIATOR_H
#define QUICKMEDIATOR_H

#include <QObject>
#include <QList>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QByteArray>
#include "../../core/DatabaseManager.h"

class QuickWindow;

namespace RapidNotes {

class QuickMediator : public QObject {
    Q_OBJECT
public:
    explicit QuickMediator(QuickWindow* window, QObject* parent = nullptr);
    ~QuickMediator() override = default;

    // --- 笔记核心元数据写操作 (通过 CoreEngine 命令通道，保证 SSOT 与全局事件广播) ---
    void requestMoveToCategory(const QList<int>& ids, int categoryId);
    void requestToggleFavorite(const QList<int>& ids);
    void requestSetFavorite(const QList<int>& ids, bool favorite);
    void requestTogglePinned(const QList<int>& ids, bool pinned);
    void requestMoveToTrash(const QList<int>& ids);
    void requestPermanentDelete(const QList<int>& ids);
    void requestSetRating(const QList<int>& ids, int rating);
    void requestSetColor(const QList<int>& ids, const QString& color);
    void requestSetTags(const QList<int>& ids, const QStringList& tags);
    void requestAddTags(const QList<int>& ids, const QStringList& tags);
    void requestRemoveTags(const QList<int>& ids, const QStringList& tags);
    void requestRenameNote(int id, const QString& newTitle);
    void requestSetNoteRemark(int id, const QString& remark);
    void requestSetNoteURL(int id, const QString& url);
    void requestRecordAccess(int id);

    // --- 笔记基础创建与入库 ---
    int  requestAddNote(const QString& title, const QString& content, const QStringList& tags = {},
                        const QString& remark = "", int categoryId = -1,
                        const QString& itemType = "text", const QByteArray& dataBlob = QByteArray());

    // --- 回收站专项操作 ---
    void requestRestoreFromTrash(const QList<int>& noteIds, const QList<int>& catIds = {});
    void requestRestoreAllTrash();
    void requestEmptyTrash();

    // --- 高级业务逻辑调度 (按行拆分、合并、批量标签覆盖、重复上一次操作) ---
    int  requestCreateByLine(const QString& text, int targetCategoryId);
    bool requestMergeNotes(const QList<int>& ids, int targetCategoryId);
    bool requestPasteTagsBatch(const QList<int>& ids, const QStringList& tags);
    bool requestRepeatLastAction(const QList<int>& targetIds);

    // --- 列表排序与位移调度 ---
    bool requestMoveNote(int id, DatabaseManager::MoveDirection dir, const QString& filterType, const QVariant& filterValue);
    bool requestMoveNotesToRow(const QList<int>& ids, int targetRow, const QString& filterType, const QVariant& filterValue);
    bool requestReorderNotes(const QString& filterType, const QVariant& filterValue, bool ascending, const QVariantMap& criteria = {});

    // --- 分类管理调度 ---
    int  requestAddCategory(const QString& name, int parentId = -1, const QString& color = "");
    bool requestRenameCategory(int catId, const QString& name);
    bool requestDeleteCategories(const QList<int>& catIds, bool hardDelete);
    bool requestToggleCategoryPinned(int catId);
    bool requestSetCategoryColor(int catId, const QString& color);
    bool requestSetCategoryPresetTags(int catId, const QString& tags);
    bool requestMoveCategory(int id, DatabaseManager::MoveDirection direction);
    bool requestReorderCategories(int parentId, bool ascending);
    bool requestReorderAllCategories(bool ascending);
    bool requestSetCategorySortMode(int catId, const QString& mode);

    // --- 分类密码保护与锁定状态调度 ---
    bool requestSetCategoryPassword(int catId, const QString& password, const QString& hint);
    bool requestVerifyCategoryPassword(int catId, const QString& password);
    bool requestRemoveCategoryPassword(int catId);
    void requestLockCategory(int catId);
    void requestLockAllCategories();
    bool requestToggleLockedCategoriesVisibility();

    // --- 自动归档配置调度 ---
    void requestSetAutoCategorize(bool enabled, int targetCatId = -1);

private slots:
    void onCentralEventTriggered();

private:
    QuickWindow* m_window = nullptr;
};

} // namespace RapidNotes

#endif // QUICKMEDIATOR_H