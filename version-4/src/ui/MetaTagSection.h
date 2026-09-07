#pragma once

#include <QWidget>
#include <QPushButton>
#include <QStringList>
#include <QPointer>
#include "components/FlowLayout.h"
#include "components/TagPill.h"
#include "TagSelectorOverlay.h"

namespace QuarkMeta {

class MetaTagSection : public QWidget {
    Q_OBJECT
public:
    explicit MetaTagSection(QWidget* parent = nullptr);
    ~MetaTagSection() override = default;

    void setTags(const QStringList& tags, const QStringList& paths);
    void setSelectedPaths(const QStringList& paths);

signals:
    void tagAddRequested(const QStringList& paths, const QString& tag);
    void tagRemoveRequested(const QStringList& paths, const QString& tag);
    void tagsChanged(const QStringList& paths, const QStringList& tags);

private:
    FlowLayout* m_tagFlowLayout = nullptr;
    QPushButton* m_btnAddTagBig = nullptr;
    QPushButton* m_btnAddTagSmall = nullptr;
    QPointer<TagSelectorOverlay> m_tagSelectorOverlay;

    QStringList m_selectedPaths;
    QList<TagPill*> m_tagPool;

    void openTagSelectorOverlay(QWidget* targetAnchor);
};

} // namespace QuarkMeta
