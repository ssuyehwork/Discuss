#pragma once

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QTimer>
#include <QEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QPointer>
#include <QAction>
#include "components/ElasticEdit.h"
#include "MetaPreviewWidget.h"
#include "MetaRatingColorWidget.h"
#include "MetaTagSection.h"
#include "MetaInfoSection.h"

namespace QuarkMeta {

class MetaPanel : public QFrame {
    Q_OBJECT
public:
    explicit MetaPanel(QWidget* parent = nullptr);
    ~MetaPanel() override = default;

    void updateInfo(const QString& name, const QString& type, const QString& size,
                    const QString& ctime, const QString& mtime, const QString& atime,
                    const QString& path, bool encrypted, int width = 0, int height = 0);

    void setImagePreview(const QPixmap& pixmap);
    void setSelectedPaths(const QStringList& paths);
    void setPalettes(const QVector<QPair<QColor, float>>& palette);
    void setTags(const QStringList& tags);
    void setNote(const QString& note);
    void setNote(const std::wstring& note);
    void setURL(const QString& url);
    void setURL(const std::wstring& url);
    void setRating(int rating);
    void setColor(const std::wstring& color);
    void setPinned(bool pinned) { Q_UNUSED(pinned); }

signals:
    void tagAddRequested(const QStringList& paths, const QString& tag);
    void tagRemoveRequested(const QStringList& paths, const QString& tag);
    void metadataChanged(int rating, const std::wstring& color);
    void noteEdited(const QStringList& paths, const QString& newNote);
    void linkEdited(const QStringList& paths, const QString& newLink);
    void primaryColorChanged(const QString& path, const QColor& color);
    void tagsChanged(const QStringList& paths, const QStringList& tags);
    void searchByColor(const QColor& color);
    void renameRequested(const QString& oldPath, const QString& newPath);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void initUi();
    void updateControlsState(bool hasSelection);
    QWidget* createCollapsibleSection(const QString& title, QWidget* contentWidget, bool defaultExpanded = true);

    QVBoxLayout* m_mainLayout = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_container = nullptr;
    QVBoxLayout* m_containerLayout = nullptr;

    // 子功能组件
    MetaPreviewWidget* m_previewWidget = nullptr;
    ElasticEdit* m_nameEdit = nullptr;
    ElasticEdit* m_noteEdit = nullptr;
    QWidget* m_linkBox = nullptr;
    QLineEdit* m_linkEdit = nullptr;
    QPushButton* m_btnOpenLink = nullptr;

    MetaRatingColorWidget* m_ratingColorWidget = nullptr;
    MetaTagSection* m_tagSection = nullptr;
    MetaInfoSection* m_infoSection = nullptr;

    QStringList m_selectedPaths;
    bool m_isInternalUpdating = false;
    bool m_isUserEditing = false;
};

} // namespace QuarkMeta
