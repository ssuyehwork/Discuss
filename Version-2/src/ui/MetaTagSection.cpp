#include "MetaTagSection.h"
#include "UiHelper.h"

namespace QuarkMeta {

MetaTagSection::MetaTagSection(QWidget* parent) : QWidget(parent) {
    m_tagFlowLayout = new FlowLayout(this, 0, 4, 4);

    m_btnAddTagBig = new QPushButton(" 添加标签", this);
    m_btnAddTagBig->setIcon(UiHelper::getIcon("add", QColor("#EEEEEE"), 14));
    m_btnAddTagBig->setCursor(Qt::PointingHandCursor);
    m_btnAddTagBig->setStyleSheet("QPushButton { background: #2A2A2A; border: 1px solid #3C3C3C; border-radius: 4px; color: #EEEEEE; padding: 4px 8px; font-size: 11px; } QPushButton:hover { background: #333333; }");

    m_btnAddTagSmall = new QPushButton("[+]", this);
    m_btnAddTagSmall->setCursor(Qt::PointingHandCursor);
    m_btnAddTagSmall->setStyleSheet("QPushButton { background: transparent; border: none; color: #888888; font-size: 11px; } QPushButton:hover { color: #FFFFFF; }");

    m_tagFlowLayout->addWidget(m_btnAddTagBig);

    connect(m_btnAddTagBig, &QPushButton::clicked, this, [this]() {
        openTagSelectorOverlay(m_btnAddTagBig);
    });
    connect(m_btnAddTagSmall, &QPushButton::clicked, this, [this]() {
        openTagSelectorOverlay(m_btnAddTagSmall);
    });
}

void MetaTagSection::setSelectedPaths(const QStringList& paths) {
    m_selectedPaths = paths;
}

void MetaTagSection::setTags(const QStringList& tags, const QStringList& paths) {
    m_selectedPaths = paths;
    while (QLayoutItem* item = m_tagFlowLayout->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->hide();
        }
        delete item;
    }

    for (auto* pill : m_tagPool) {
        pill->hide();
    }

    int idx = 0;
    for (const QString& tag : tags) {
        TagPill* pill = nullptr;
        if (idx < m_tagPool.size()) {
            pill = m_tagPool[idx];
            pill->setData(tag);
        } else {
            pill = new TagPill(tag, this);
            m_tagPool.append(pill);
            connect(pill, &TagPill::deleteRequested, this, [this](const QString& t) {
                emit tagRemoveRequested(m_selectedPaths, t);
            });
        }
        pill->show();
        m_tagFlowLayout->addWidget(pill);
        idx++;
    }

    if (tags.isEmpty()) {
        m_btnAddTagSmall->hide();
        m_btnAddTagBig->show();
        m_tagFlowLayout->addWidget(m_btnAddTagBig);
    } else {
        m_btnAddTagBig->hide();
        m_btnAddTagSmall->show();
        m_tagFlowLayout->addWidget(m_btnAddTagSmall);
    }
}

void MetaTagSection::openTagSelectorOverlay(QWidget* targetAnchor) {
    Q_UNUSED(targetAnchor);
    if (m_selectedPaths.isEmpty()) return;

    if (!m_tagSelectorOverlay) {
        m_tagSelectorOverlay = new TagSelectorOverlay({}, this);
        connect(m_tagSelectorOverlay, &TagSelectorOverlay::selectionChanged, this, [this](const QStringList& currentTags) {
            emit tagsChanged(m_selectedPaths, currentTags);
        });
    }

    m_tagSelectorOverlay->show();
}

} // namespace QuarkMeta
