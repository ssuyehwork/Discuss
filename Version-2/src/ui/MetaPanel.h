#pragma once

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QTimer>
#include <QEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QPointer>
#include <QAction>
#include <QSet>
#include "components/ElasticEdit.h"
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

    void setImagePreview(const QPixmap& pixmap);
    void setSelectedPaths(const QStringList& paths);
    void setPalettes(const QVector<QPair<QColor, float>>& palette);
    void setTags(const QStringList& tags);
    void setNote(const QString& note);
    void setNote(const std::wstring& note);
    void setURL(const QString& url);
    void setURL(const std::wstring& url);
    void setRating(int rating, bool fromUser = false);
    void setColor(const QString& hexColor, bool fromUser = false);
    void setColor(const std::wstring& color, bool fromUser = false);
    void setPinned(bool pinned) { Q_UNUSED(pinned); }

signals:
    // 解耦且携带路径的纯净领域信号
    void ratingChanged(const QStringList& paths, int rating);
    void colorChanged(const QStringList& paths, const QString& hexColor);
    void tagAddRequested(const QStringList& paths, const QString& tag);
    void tagRemoveRequested(const QStringList& paths, const QString& tag);
    void noteEdited(const QStringList& paths, const QString& newNote);
    void linkEdited(const QStringList& paths, const QString& newLink);
    void primaryColorChanged(const QString& path, const QColor& color);
    void renameRequested(const QString& oldPath, const QString& newPath);
    void searchByColor(const QColor& color);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void initUi();
    void updateControlsState(bool hasSelection, bool isMultiSelection, bool isReadOnly);
    void adjustFlowHeights();
    void addInfoRow(QVBoxLayout* layout, const QString& label, QLabel*& valueLabel);
    QWidget* createCollapsibleSection(const QString& title, QWidget* contentWidget, bool defaultExpanded = true);

    QVBoxLayout* m_mainLayout = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_container = nullptr;
    QVBoxLayout* m_containerLayout = nullptr;

    // 1. 顶部预览与色板区
    QWidget* m_topPreviewBox = nullptr;
    QLabel* m_lblImagePreview = nullptr;
    QWidget* m_paletteContainer = nullptr;
    FlowLayout* m_paletteFlowLayout = nullptr;

    // 2. 文件名编辑区
    ElasticEdit* m_nameEdit = nullptr;

    // 3. 备注说明区
    ElasticEdit* m_noteEdit = nullptr;

    // 4. 关联网址区
    QWidget* m_linkBox = nullptr;
    QLineEdit* m_linkEdit = nullptr;
    QAction* m_actOpenLink = nullptr;

    // 5. 星级评级 + 颜色色标条
    QWidget* m_ratingColorBox = nullptr;
    QList<QPushButton*> m_starBtns;
    QList<QPushButton*> m_colorBtns;
    int m_currentRating = 0;
    QString m_currentColorHex;

    // 6. 标签管理区
    QWidget* m_tagBox = nullptr;
    QWidget* m_tagContainer = nullptr;
    FlowLayout* m_tagFlowLayout = nullptr;
    QPushButton* m_btnAddTagBig = nullptr;
    QPushButton* m_btnAddTagSmall = nullptr;
    QPointer<TagSelectorOverlay> m_tagSelectorOverlay;

    // 7. 基础物理属性区
    QWidget* m_infoSectionWidget = nullptr;
    QLabel* lblType = nullptr;
    QLabel* lblSize = nullptr;
    QLabel* lblDimensions = nullptr;
    QLabel* lblCtime = nullptr;
    QLabel* lblMtime = nullptr;
    QLabel* lblAtime = nullptr;
    QLabel* lblEncrypted = nullptr;

    // 8. 物理路径区
    QLineEdit* m_pathEdit = nullptr;
    QPushButton* m_btnCopyPath = nullptr;
    QPushButton* m_btnOpenLocation = nullptr;

    QStringList m_selectedPaths;
    QStringList m_editingPathsSnapshot; // 防失焦时序竞态的路径快照
    QSet<QString> m_currentTagsSet;
    QTimer* m_adjustTimer = nullptr;
    bool m_isInternalUpdating = false;
    bool m_isUserEditing = false;
    bool m_isReadOnlyMode = false;

private slots:
    void onTagDeleted(const QString& text);
    void openTagSelectorOverlay(QWidget* targetAnchor);
    void setAsPrimaryColor(const QColor& color);
};

} // namespace QuarkMeta