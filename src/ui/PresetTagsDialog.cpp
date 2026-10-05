#include "PresetTagsDialog.h"
#include "UiHelper.h"
#include "../meta/FavoriteDao.h"
#include "../meta/FavoriteService.h"
#include "../meta/LibraryDao.h"
#include "../meta/MetadataManager.h"
#include "../core/CoreEngine.h"
#include "ToolTipOverlay.h"
#include <QCursor>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QApplication>
#include <QScreen>
#include <QTimer>
#include <QKeyEvent>
#include <QListWidget>
#include <QScrollArea>

namespace QuarkMeta {

PresetTagsDialog::PresetTagsDialog(int categoryId, QWidget* parent, bool isLibrary)
    : FramelessDialog("设置自动标签", parent), m_categoryId(categoryId), m_isLibrary(isLibrary) {
    setFixedWidth(420);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);

    initUi();
    loadTags();
    populateTagPills();
    installEventFilter(this);
}

PresetTagsDialog::~PresetTagsDialog() {}

void PresetTagsDialog::initUi() {
    QVBoxLayout* mainL = new QVBoxLayout(m_contentArea);
    mainL->setContentsMargins(15, 15, 15, 15);
    mainL->setSpacing(12);

    QLabel* folderNameLabel = new QLabel("分类/文件夹名", this);
    folderNameLabel->setObjectName("PresetFolderLabel");
    mainL->addWidget(folderNameLabel);

    m_folderNameEdit = new QLineEdit(this);
    m_folderNameEdit->setReadOnly(true);
    m_folderNameEdit->setFixedHeight(28);
    m_folderNameEdit->setObjectName("PresetFolderNameEdit");
    mainL->addWidget(m_folderNameEdit);

    QLabel* tagLabel = new QLabel("自动添加标签", this);
    tagLabel->setObjectName("PresetTagLabel");
    mainL->addWidget(tagLabel);

    m_tagContainer = new QFrame(this);
    m_tagContainer->setObjectName("PresetTagContainer");
    m_tagContainer->setFrameShape(QFrame::StyledPanel);
    m_tagContainer->setCursor(Qt::PointingHandCursor);
    m_tagContainer->setMinimumHeight(42);

    m_flowLayout = new FlowLayout(m_tagContainer, 8, 8, 8);
    m_tagContainer->setLayout(m_flowLayout);
    m_tagContainer->installEventFilter(this);

    mainL->addWidget(m_tagContainer, 1);

    QHBoxLayout* bottomL = new QHBoxLayout();
    bottomL->setSpacing(10);
    bottomL->addStretch();

    QPushButton* btnSave = new QPushButton("保存设置", this);
    btnSave->setFixedSize(90, 28);
    btnSave->setCursor(Qt::PointingHandCursor);
    btnSave->setObjectName("PresetBtnSave");
    connect(btnSave, &QPushButton::clicked, this, &PresetTagsDialog::onSaveClicked);
    bottomL->addWidget(btnSave);

    QPushButton* btnCancel = new QPushButton("取消", this);
    btnCancel->setFixedSize(70, 28);
    btnCancel->setCursor(Qt::PointingHandCursor);
    btnCancel->setObjectName("PresetBtnCancel");
    connect(btnCancel, &QPushButton::clicked, this, &PresetTagsDialog::onCancelClicked);
    bottomL->addWidget(btnCancel);

    mainL->addLayout(bottomL);
}

void PresetTagsDialog::loadTags() {
    if (m_isLibrary) {
        auto list = LibraryDao::getAllCategories();
        for (const auto& rec : list) {
            if (rec.id == m_categoryId) {
                m_categoryName = rec.name;
                m_presetTags = rec.presetTags;
                m_initialPresetTags = rec.presetTags;
                break;
            }
        }
    } else {
        auto list = FavoriteDao::getAllFavorites();
        for (const auto& rec : list) {
            if (rec.id == m_categoryId) {
                m_categoryName = rec.name;
                m_presetTags = rec.presetTags;
                m_initialPresetTags = rec.presetTags;
                break;
            }
        }
    }
    m_folderNameEdit->setText(m_categoryName);
}

void PresetTagsDialog::populateTagPills() {
    QLayoutItem* item;
    while ((item = m_flowLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    for (const QString& tag : m_presetTags) {
        TagPill* pill = new TagPill(tag, m_tagContainer);
        connect(pill, &TagPill::deleteRequested, this, &PresetTagsDialog::onTagDeleted);
        m_flowLayout->addWidget(pill);
    }

    recalculateAdaptiveHeight();
}

void PresetTagsDialog::onTagDeleted(const QString& tag) {
    m_presetTags.removeAll(tag);
    populateTagPills();
}

void PresetTagsDialog::recalculateAdaptiveHeight() {
    m_flowLayout->activate();
    int flowContentHeight = m_flowLayout->heightForWidth(m_tagContainer->width());
    
    int tagContainerHeight = qMax(42, flowContentHeight + 10);
    m_tagContainer->setFixedHeight(tagContainerHeight);

    layout()->activate();
    int requiredTotalHeight = sizeHint().height();

    setFixedHeight(requiredTotalHeight);
}

void PresetTagsDialog::onTagContainerClicked() {
    if (m_selectorOverlay) {
        m_selectorOverlay->raise();
        m_selectorOverlay->activateWindow();
        return;
    }

    m_selectorOverlay = new TagSelectorOverlay(m_presetTags, window());
    
    QPoint globalPos = m_tagContainer->mapToGlobal(QPoint(0, m_tagContainer->height() + 4));
    m_selectorOverlay->move(globalPos);
    m_selectorOverlay->show();
    m_selectorOverlay->raise();
    m_selectorOverlay->activateWindow();

    connect(m_selectorOverlay, &TagSelectorOverlay::selectionChanged, this, [this](const QStringList& selected) {
        m_presetTags = selected;
        populateTagPills();
    });

    connect(m_selectorOverlay, &TagSelectorOverlay::overlayClosed, this, [this]() {
        m_selectorOverlay = nullptr;
    });
}

void PresetTagsDialog::mousePressEvent(QMouseEvent* event) {
    QDialog::mousePressEvent(event);
}

void PresetTagsDialog::mouseMoveEvent(QMouseEvent* event) {
    QDialog::mouseMoveEvent(event);
}

void PresetTagsDialog::mouseReleaseEvent(QMouseEvent* event) {
    QDialog::mouseReleaseEvent(event);
}

void PresetTagsDialog::onSaveClicked() {
    QStringList newlyAddedTags;
    for (const QString& tag : m_presetTags) {
        if (!m_initialPresetTags.contains(tag)) {
            newlyAddedTags.append(tag);
        }
    }

    if (m_isLibrary) {
        LibraryDao::updatePresetTags(m_categoryId, m_presetTags);
        if (!newlyAddedTags.isEmpty()) {
            QStringList associatedPaths = LibraryDao::getCategoryPaths(m_categoryId);
            if (!associatedPaths.isEmpty()) {
                for (const QString& tag : newlyAddedTags) {
                    AppCommand cmd;
                    cmd.type = AppCommandType::AddTag;
                    cmd.targetPaths = associatedPaths;
                    cmd.params["tag"] = tag;
                    CoreEngine::instance().executeCommand(cmd);
                }
                ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已自动追溯将 %1 个新预设标签绑定至既有 %2 个项目").arg(newlyAddedTags.size()).arg(associatedPaths.size()), 2000, QColor("#2ecc71"));
            }
        }
    } else {
        FavoriteDao::updatePresetTags(m_categoryId, m_presetTags);
    }
    accept();
}

void PresetTagsDialog::onCancelClicked() {
    reject();
}

bool PresetTagsDialog::eventFilter(QObject* obj, QEvent* event) {
    if ((obj == m_tagContainer || (m_tagContainer && m_tagContainer->isAncestorOf(qobject_cast<QWidget*>(obj)))) &&
        (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease)) {
        
        // 如果点击发生在 TagPill（胶囊项）及其子控件上，不响应调起弹窗
        QWidget* targetWidget = qobject_cast<QWidget*>(obj);
        while (targetWidget && targetWidget != m_tagContainer) {
            if (qobject_cast<TagPill*>(targetWidget)) {
                return FramelessDialog::eventFilter(obj, event);
            }
            targetWidget = targetWidget->parentWidget();
        }

        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            if (event->type() == QEvent::MouseButtonRelease) {
                onTagContainerClicked();
            }
            return true;
        }
    }
    return FramelessDialog::eventFilter(obj, event);
}

} // namespace QuarkMeta
