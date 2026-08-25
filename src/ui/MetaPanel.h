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
#include "components/ElasticEdit.h"
#include <QPointer>
#include <QPushButton>
#include <QToolButton>
#include "TagSelectorOverlay.h"
#include "components/TagPill.h"
#include "components/FlowLayout.h"
#include "components/ColorPill.h"

namespace QuarkMeta {

class MetaPanel : public QFrame {
    Q_OBJECT
public:
    explicit MetaPanel(QWidget* parent = nullptr);
    ~MetaPanel() override = default;

    void updateInfo(const QString& name, const QString& type, const QString& size,
                    const QString& ctime, const QString& mtime, const QString& atime,
                    const QString& path, bool encrypted, int width = 0, int height = 0);

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
    void updateControlsState(bool hasSelection);
    void initUi();
    void adjustFlowHeights();
    void addInfoRow(QVBoxLayout* layout, const QString& label, QLabel*& valueLabel);
    QFrame* createSeparator();
    
    // Collapsible Section Builder
    QWidget* createCollapsibleSection(const QString& title, QWidget* contentWidget, bool defaultExpanded = true);

    QVBoxLayout* m_mainLayout = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_container = nullptr;
    QVBoxLayout* m_containerLayout = nullptr;

    // 1. 顶部预览与色板区
    QWidget* m_topPreviewBox = nullptr;
    QLabel* m_lblImagePreview = nullptr;
    FlowLayout* m_paletteFlowLayout = nullptr;

    // 2. 文件名大字高亮编辑框
    ElasticEdit* m_nameEdit = nullptr;

    // 3. 星级评级 + 颜色色标条
    QWidget* m_ratingColorBox = nullptr;
    QList<QPushButton*> m_starBtns;
    int m_currentRating = 0;
    std::wstring m_currentColor;
    QList<QPushButton*> m_colorBtns;

    // 4. 标签管理区
    QWidget* m_tagBox = nullptr;
    QWidget* m_tagContainer = nullptr;
    FlowLayout* m_tagFlowLayout = nullptr;
    QPushButton* m_btnAddTagBig = nullptr;
    QPushButton* m_btnAddTagSmall = nullptr;
    QPointer<TagSelectorOverlay> m_tagSelectorOverlay;

    // 5. 备注说明区
    ElasticEdit* m_noteEdit = nullptr;

    // 6. 关联网址区
    QWidget* m_linkBox = nullptr;
    ElasticEdit* m_linkEdit = nullptr;
    QPushButton* m_btnOpenLink = nullptr;

    // 7. 基础物理属性区
    QWidget* m_infoSectionWidget = nullptr;
    QLabel* lblType = nullptr, *lblSize = nullptr, *lblDimensions = nullptr;
    QLabel* lblCtime = nullptr, *lblMtime = nullptr, *lblAtime = nullptr;
    QLabel* lblEncrypted = nullptr;

    // 8. 物理路径区
    ElasticEdit* m_pathEdit = nullptr;
    QPushButton* m_btnCopyPath = nullptr;
    QPushButton* m_btnOpenLocation = nullptr;

    QStringList m_selectedPaths;
    QList<TagPill*> m_tagPool;
    QList<ColorPill*> m_colorPool;
    QTimer* m_adjustTimer = nullptr;
    bool m_isInternalUpdating = false;
    bool m_isUserEditing = false;

private slots:
    void onTagDeleted(const QString& text);
    void setAsPrimaryColor(const QColor& color);
    void openTagSelectorOverlay(QWidget* targetAnchor);
};

} // namespace QuarkMeta
