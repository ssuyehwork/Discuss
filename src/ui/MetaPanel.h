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
    void adjustFlowHeights();
    void addInfoRow(QVBoxLayout* layout, const QString& label, QLabel*& valueLabel);
    QWidget* createCollapsibleSection(const QString& title, QWidget* contentWidget, bool defaultExpanded = true);

    QVBoxLayout* m_mainLayout = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_container = nullptr;
    QVBoxLayout* m_containerLayout = nullptr;

    // 1. 顶部预览与色板区 (有则显，无则完全隐藏)
    QWidget* m_topPreviewBox = nullptr;
    QLabel* m_lblImagePreview = nullptr;
    FlowLayout* m_paletteFlowLayout = nullptr;

    // 2. 文件名编辑区 (大字高亮)
    ElasticEdit* m_nameEdit = nullptr;

    // 3. 备注说明区 (可折叠)
    ElasticEdit* m_noteEdit = nullptr;

    // 4. 关联网址区 (可折叠，一体化容器与独立跳转按钮)
    QWidget* m_linkBox = nullptr;
    QLineEdit* m_linkEdit = nullptr;
    QAction* m_actOpenLink = nullptr;

    // 5. 星级评级 + 颜色色标条 (8 色圆点)
    QWidget* m_ratingColorBox = nullptr;
    QList<QPushButton*> m_starBtns;
    QList<QPushButton*> m_colorBtns;
    int m_currentRating = 0;
    std::wstring m_currentColor;

    // 6. 标签管理区 (可折叠)
    QWidget* m_tagBox = nullptr;
    QWidget* m_tagContainer = nullptr;
    FlowLayout* m_tagFlowLayout = nullptr;
    QPushButton* m_btnAddTagBig = nullptr;
    QPushButton* m_btnAddTagSmall = nullptr;
    QPointer<TagSelectorOverlay> m_tagSelectorOverlay;

    // 7. 基础物理属性区 (可折叠)
    QWidget* m_infoSectionWidget = nullptr;
    QLabel* lblType = nullptr;
    QLabel* lblSize = nullptr;
    QLabel* lblDimensions = nullptr;
    QLabel* lblCtime = nullptr;
    QLabel* lblMtime = nullptr;
    QLabel* lblAtime = nullptr;
    QLabel* lblEncrypted = nullptr;

    // 8. 物理路径区 (可折叠)
    QLineEdit* m_pathEdit = nullptr;
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