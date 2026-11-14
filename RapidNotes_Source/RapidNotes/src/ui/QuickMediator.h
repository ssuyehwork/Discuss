#ifndef QUICKMEDIATOR_H
#define QUICKMEDIATOR_H

#include <QObject>
#include <QList>
#include <QStringList>
#include <QVariantMap>

class QuickWindow;

namespace RapidNotes {

class QuickMediator : public QObject {
    Q_OBJECT
public:
    explicit QuickMediator(QuickWindow* window, QObject* parent = nullptr);
    ~QuickMediator() override = default;

    void requestMoveToCategory(const QList<int>& ids, int categoryId);
    void requestToggleFavorite(const QList<int>& ids);
    void requestMoveToTrash(const QList<int>& ids);
    void requestPermanentDelete(const QList<int>& ids);
    void requestSetRating(const QList<int>& ids, int rating);
    void requestSetColor(const QList<int>& ids, const QString& color);
    void requestSetTags(const QList<int>& ids, const QStringList& tags);
    void requestAddTags(const QList<int>& ids, const QStringList& tags);
    void requestSetFavorite(const QList<int>& ids, bool favorite);
    void requestSetPinned(const QList<int>& ids, bool pinned);
    void requestRestoreFromTrash(const QList<int>& ids);
    void requestRenameNote(int id, const QString& newTitle);

    // 分类 / 回收站（统一经中央大脑执行）
    void requestAddCategory(const QString& name, int parentId = -1);
    void requestToggleCategoryPinned(int categoryId);
    void requestSetCategoryColor(int categoryId, const QString& color);
    void requestSetCategoryPresetTags(int categoryId, const QString& tags);
    void requestSetCategorySortMode(int categoryId, const QString& mode);
    void requestSoftDeleteCategories(const QList<int>& ids);
    void requestHardDeleteCategories(const QList<int>& ids);
    void requestRestoreCategories(const QList<int>& ids);
    bool requestMoveCategory(int categoryId, int direction);
    bool requestReorderCategories(int parentId, bool ascending);
    bool requestReorderAllCategories(bool ascending);
    bool requestEmptyTrash();
    bool requestRestoreAllFromTrash();

    // 撤销 / 重做
    bool requestUndo();
    bool requestRedo();

private slots:
    void onMetadataUpdated(const QStringList& paths);
    void onItemsDeleted(const QStringList& paths);

private:
    QuickWindow* m_window = nullptr;
};

} // namespace RapidNotes

#endif // QUICKMEDIATOR_H
