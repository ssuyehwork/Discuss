#include "TagSelectorOverlay.h"
#include "UiHelper.h"
#include "../core/TagLexiconService.h"
#include "../core/AppConfig.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QScreen>
#include <QScrollBar>

namespace QuarkMeta {

TagSelectorOverlay::TagSelectorOverlay(const QStringList& initialSelected, QWidget* parent)
    : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint), 
      m_selectedTags(initialSelected) 
{
    setObjectName("TagSelectorOverlay");
    setFrameShape(QFrame::StyledPanel);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_DeleteOnClose, false);

    initUi();
    loadTagsAndGroups();
    
    m_searchEdit->installEventFilter(this);
    m_tagGridWidget->installEventFilter(this);

    qApp->installEventFilter(this);
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget* old, QWidget* now) {
        Q_UNUSED(old);
        if (m_isClosing || !isVisible()) return;

        if (now && (now == this || this->isAncestorOf(now))) {
            return;
        }

        if (now && !this->isAncestorOf(now)) {
            closeOverlay();
        }
    });
}

TagSelectorOverlay::~TagSelectorOverlay() {
    if (qApp) {
        qApp->removeEventFilter(this);
    }
}

void TagSelectorOverlay::closeOverlay() {
    if (m_isClosing) return;
    m_isClosing = true;
    emit overlayClosed();
    close();
    deleteLater();
}

void TagSelectorOverlay::initUi() {
    QVBoxLayout* mainL = new QVBoxLayout(this);
    mainL->setContentsMargins(10, 10, 10, 10);
    mainL->setSpacing(8);

    QHBoxLayout* topSearchLayout = new QHBoxLayout();
    topSearchLayout->setContentsMargins(0, 0, 0, 0);
    topSearchLayout->setSpacing(6);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("搜索或新建标签...");
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFixedHeight(26);
    m_searchEdit->setObjectName("TagSelectorSearchEdit");
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this]() {
        filterTags();
    });
    topSearchLayout->addWidget(m_searchEdit, 1);

    m_btnToggleSidebar = new QPushButton(this);
    m_btnToggleSidebar->setFixedSize(26, 26);
    m_btnToggleSidebar->setCheckable(true);
    m_btnToggleSidebar->setChecked(true);
    m_btnToggleSidebar->setIcon(UiHelper::getIcon("sidebar", QColor("#AAAAAA"), 16));
    m_btnToggleSidebar->setIconSize(QSize(16, 16));
    m_btnToggleSidebar->setCursor(Qt::PointingHandCursor);
    m_btnToggleSidebar->setObjectName("TagSelectorToggleBtn");
    connect(m_btnToggleSidebar, &QPushButton::toggled, this, [this](bool checked) {
        m_groupList->setVisible(checked);
    });
    topSearchLayout->addWidget(m_btnToggleSidebar);

    mainL->addLayout(topSearchLayout);

    QHBoxLayout* bodyL = new QHBoxLayout();
    bodyL->setSpacing(8);

    m_groupList = new QListWidget(this);
    m_groupList->setFixedWidth(110);
    m_groupList->setFocusPolicy(Qt::StrongFocus);
    m_groupList->setObjectName("TagSelectorGroupList");
    connect(m_groupList, &QListWidget::currentRowChanged, this, [this]() {
        filterTags();
    });
    bodyL->addWidget(m_groupList);

    m_tagGridWidget = new QWidget(this);
    m_tagGridWidget->setAttribute(Qt::WA_StyledBackground, true);
    m_tagGridWidget->setObjectName("TagSelectorGridWidget");
    m_tagGridWidget->setFocusPolicy(Qt::StrongFocus);
    m_gridFlowLayout = new FlowLayout(m_tagGridWidget, 10, 6, 6);
    m_tagGridWidget->setLayout(m_gridFlowLayout);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setObjectName("TagSelectorScrollArea");
    m_scrollArea->setWidget(m_tagGridWidget);
    bodyL->addWidget(m_scrollArea, 1);

    mainL->addLayout(bodyL, 1);

    setMinimumSize(250, 150);
    QSize savedSize = AppConfig::instance().getValue("TagSelectorOverlay/Size", QSize(400, 240)).toSize();
    resize(savedSize.expandedTo(QSize(250, 150)));
}

void TagSelectorOverlay::loadTagsAndGroups() {
    m_lexiconGroups = TagLexiconService::instance().getAllTagGroups();
    
    m_groupList->clear();

    auto addGroupItem = [this](const QString& name, const QString& iconKey) {
        QListWidgetItem* item = new QListWidgetItem(name, m_groupList);
        item->setIcon(UiHelper::getIcon(iconKey, QColor("#AAAAAA"), 14));
        return item;
    };

    addGroupItem("全部", "all_data");
    addGroupItem("未分类", "uncategorized");

    for (const auto& grp : m_lexiconGroups) {
        if (grp.id > 0) {
            addGroupItem(grp.name, "folder_filled");
        }
    }

    m_groupList->setCurrentRow(0);
}

void TagSelectorOverlay::filterTags() {
    QString kw = m_searchEdit->text().trimmed();
    QString currentGrp = m_groupList->currentItem() ? m_groupList->currentItem()->text() : "全部";

    m_displayedTags.clear();

    if (currentGrp == "未分类") {
        QSet<QString> groupedTags;
        for (const auto& grp : m_lexiconGroups) {
            if (grp.id > 0) {
                for (const auto& t : grp.tags) groupedTags.insert(t.name);
            }
        }
        QStringList allMaster = TagLexiconService::instance().getAllTagNames();
        for (const QString& tag : allMaster) {
            if (groupedTags.contains(tag)) continue;
            if (!kw.isEmpty() && !tag.toLower().contains(kw.toLower())) continue;
            m_displayedTags.append(tag);
        }
    } else if (currentGrp != "全部" && currentGrp != "未分类") {
        for (const auto& grp : m_lexiconGroups) {
            if (grp.name == currentGrp) {
                for (const auto& t : grp.tags) {
                    if (!kw.isEmpty() && !t.name.toLower().contains(kw.toLower())) continue;
                    m_displayedTags.append(t.name);
                }
                break;
            }
        }
    } else {
        QStringList allMaster = TagLexiconService::instance().getAllTagNames();
        for (const QString& tag : allMaster) {
            if (!kw.isEmpty() && !tag.toLower().contains(kw.toLower())) continue;
            m_displayedTags.append(tag);
        }
    }

    if (!kw.isEmpty() && !m_displayedTags.contains(kw, Qt::CaseInsensitive)) {
        m_displayedTags.prepend(kw);
    }

    populateGrid();
}

void TagSelectorOverlay::populateGrid() {
    QLayoutItem* item;
    while ((item = m_gridFlowLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    m_tagButtons.clear();
    for (int i = 0; i < m_displayedTags.size(); ++i) {
        QString tag = m_displayedTags[i];

        QPushButton* btn = new QPushButton(m_tagGridWidget);
        btn->setObjectName("TagOverlayBtn");
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedHeight(22);

        m_tagButtons.append(btn);
        m_gridFlowLayout->addWidget(btn);

        connect(btn, &QPushButton::clicked, this, [this, tag]() {
            toggleTagSelection(tag);
        });
    }

    m_currentTagIndex = -1;
    updateSelectionHighlight();
}

void TagSelectorOverlay::toggleTagSelection(const QString& tag) {
    QString cleanTag = tag.trimmed();
    if (cleanTag.isEmpty()) return;

    if (m_selectedTags.contains(cleanTag)) {
        m_selectedTags.removeAll(cleanTag);
    } else {
        TagLexiconService::instance().addTag(cleanTag);
        m_selectedTags.append(cleanTag);
    }

    updateSelectionHighlight();
    emit selectionChanged(m_selectedTags);
}

void TagSelectorOverlay::updateSelectionHighlight() {
    for (int i = 0; i < m_displayedTags.size(); ++i) {
        QString tag = m_displayedTags[i];
        QPushButton* btn = m_tagButtons[i];

        bool isSelected = m_selectedTags.contains(tag);
        bool isFocused = (i == m_currentTagIndex);

        btn->setText(tag);

        if (isSelected) {
            btn->setIcon(UiHelper::getIcon("check", QColor("#FFFFFF"), 12));
        } else {
            btn->setIcon(UiHelper::getIcon("clock", QColor("#888888"), 12));
        }
        btn->setIconSize(QSize(12, 12));

        btn->setProperty("selected", isSelected);
        btn->setProperty("focused", isFocused);
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    }
}

void TagSelectorOverlay::handleGridNavigation(int key) {
    if (m_displayedTags.isEmpty()) return;
    int rowCount = m_displayedTags.size();
    if (m_currentTagIndex < 0) {
        m_currentTagIndex = 0;
    } else {
        if (key == Qt::Key_Left) {
            m_currentTagIndex = (m_currentTagIndex - 1 + rowCount) % rowCount;
        } else if (key == Qt::Key_Right) {
            m_currentTagIndex = (m_currentTagIndex + 1) % rowCount;
        } else if (key == Qt::Key_Up) {
            m_currentTagIndex = qMax(0, m_currentTagIndex - 4);
        } else if (key == Qt::Key_Down) {
            m_currentTagIndex = qMin(rowCount - 1, m_currentTagIndex + 4);
        }
    }
    updateSelectionHighlight();
}

void TagSelectorOverlay::handleGroupNavigation(int key) {
    int row = m_groupList->currentRow();
    int count = m_groupList->count();
    if (key == Qt::Key_Up) {
        m_groupList->setCurrentRow((row - 1 + count) % count);
    } else if (key == Qt::Key_Down) {
        m_groupList->setCurrentRow((row + 1) % count);
    }
}

void TagSelectorOverlay::resizeEvent(QResizeEvent* event) {
    QFrame::resizeEvent(event);
    if (isVisible()) {
        AppConfig::instance().setValue("TagSelectorOverlay/Size", size());
    }
}

void TagSelectorOverlay::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void TagSelectorOverlay::mouseMoveEvent(QMouseEvent* event) {
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        QPoint newPos = event->globalPosition().toPoint() - m_dragPos;
        QWidget* parentWin = parentWidget() ? parentWidget()->window() : nullptr;
        if (parentWin) {
            QRect parentGeom = parentWin->frameGeometry();
            int minX = parentGeom.left();
            int maxX = qMax(minX, parentGeom.right() - width());
            int minY = parentGeom.top();
            int maxY = qMax(minY, parentGeom.bottom() - height());
            newPos.setX(qBound(minX, newPos.x(), maxX));
            newPos.setY(qBound(minY, newPos.y(), maxY));
        }
        move(newPos);
        event->accept();
        return;
    }
    QFrame::mouseMoveEvent(event);
}

void TagSelectorOverlay::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        event->accept();
        return;
    }
    QFrame::mouseReleaseEvent(event);
}

void TagSelectorOverlay::changeEvent(QEvent* event) {
    if (event->type() == QEvent::ActivationChange || event->type() == QEvent::WindowDeactivate) {
        if (!isActiveWindow() && !this->isAncestorOf(QApplication::focusWidget())) {
            closeOverlay();
        }
    }
    QFrame::changeEvent(event);
}

bool TagSelectorOverlay::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        if (isVisible() && !geometry().contains(me->globalPosition().toPoint())) {
            closeOverlay();
            return false;
        }
    }

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* ke = static_cast<QKeyEvent*>(event);
        
        if (ke->key() == Qt::Key_Escape) {
            closeOverlay();
            ke->accept();
            return true;
        }

        if (ke->key() == Qt::Key_Tab) {
            if (m_searchEdit->hasFocus()) {
                m_groupList->setFocus();
            } else if (m_groupList->hasFocus()) {
                m_tagGridWidget->setFocus();
            } else {
                m_searchEdit->setFocus();
            }
            updateSelectionHighlight();
            ke->accept();
            return true;
        }

        if (ke->key() == Qt::Key_Up || ke->key() == Qt::Key_Down || ke->key() == Qt::Key_Left || ke->key() == Qt::Key_Right) {
            if (m_tagGridWidget->hasFocus() || obj == m_tagGridWidget) {
                handleGridNavigation(ke->key());
                ke->accept();
                return true;
            } else if (m_groupList->hasFocus()) {
                handleGroupNavigation(ke->key());
                ke->accept();
                return true;
            }
        }

        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            if (m_searchEdit->hasFocus()) {
                QString kw = m_searchEdit->text().trimmed();
                if (!kw.isEmpty()) {
                    toggleTagSelection(kw);
                    m_searchEdit->clear();
                }
            } else if (m_tagGridWidget->hasFocus() || obj == m_tagGridWidget) {
                if (m_currentTagIndex >= 0 && m_currentTagIndex < m_displayedTags.size()) {
                    toggleTagSelection(m_displayedTags[m_currentTagIndex]);
                }
            }
            ke->accept();
            return true;
        }
    }
    return QFrame::eventFilter(obj, event);
}

} // namespace QuarkMeta