#include "ContentHeaderWidget.h"
#include "UiHelper.h"
#include "../core/AppConfig.h"

namespace QuarkMeta {

ContentHeaderWidget::ContentHeaderWidget(QWidget* parent)
    : QWidget(parent) {
    setObjectName("ContentHeaderWidget");
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(32);
    initUi();
}

void ContentHeaderWidget::initUi() {
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(8, 0, 8, 0);
    m_layout->setSpacing(6);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(18, 18);
    m_iconLabel->setPixmap(UiHelper::getIcon("image_picture", QColor("#41F2F2"), 18).pixmap(18, 18));

    m_titleLabel = new QLabel("主视图", this);
    m_titleLabel->setObjectName("ContentHeaderTitle");

    m_layout->addWidget(m_iconLabel);
    m_layout->addWidget(m_titleLabel);
    m_layout->addStretch();

    auto setupToggleBtn = [this](QPushButton*& btn, const QString& iconKey, const QColor& activeColor, bool defaultChecked, const QString& tooltip) {
        btn = new QPushButton(this);
        btn->setCheckable(true);
        btn->setFixedSize(24, 24);
        btn->setChecked(defaultChecked);
        btn->setIcon(UiHelper::getIcon(iconKey, defaultChecked ? activeColor : QColor("#888888"), 16));
        btn->setProperty("tooltipText", tooltip);
        btn->setObjectName("ViewModeToolBtn");
        m_layout->addWidget(btn, 0, Qt::AlignVCenter);
    };

    setupToggleBtn(m_btnToggleHidden, "eye", QColor("#3498db"), m_filterState.showHidden, "显示/隐藏隐藏项目");
    connect(m_btnToggleHidden, &QPushButton::clicked, this, [this]() {
        m_filterState.showHidden = m_btnToggleHidden->isChecked();
        m_btnToggleHidden->setIcon(UiHelper::getIcon("eye", m_filterState.showHidden ? QColor("#3498db") : QColor("#888888"), 16));
        emit filterStateChanged(m_filterState);
    });

    setupToggleBtn(m_btnToggleFolders, "folder_filled", QColor("#FDB70A"), m_filterState.showFolders, "显示/隐藏文件夹");
    connect(m_btnToggleFolders, &QPushButton::clicked, this, [this]() {
        m_filterState.showFolders = m_btnToggleFolders->isChecked();
        m_btnToggleFolders->setIcon(UiHelper::getIcon("folder_filled", m_filterState.showFolders ? QColor("#FDB70A") : QColor("#B0B0B0"), 16));
        emit filterStateChanged(m_filterState);
    });

    setupToggleBtn(m_btnToggleFiles, "file", QColor("#2ecc71"), m_filterState.showFiles, "显示/隐藏文件");
    connect(m_btnToggleFiles, &QPushButton::clicked, this, [this]() {
        m_filterState.showFiles = m_btnToggleFiles->isChecked();
        m_btnToggleFiles->setIcon(UiHelper::getIcon("file", m_filterState.showFiles ? QColor("#2ecc71") : QColor("#B0B0B0"), 16));
        emit filterStateChanged(m_filterState);
    });

    m_btnLayers = new QPushButton(this);
    m_btnLayers->setCheckable(true);
    m_btnLayers->setFixedSize(24, 24);
    m_btnLayers->setIcon(UiHelper::getIcon("layers", QColor("#2ecc71"), 18));
    m_btnLayers->setProperty("tooltipText", "显示子文件夹中的项目");
    m_btnLayers->setObjectName("ViewModeToolBtn");

    connect(m_btnLayers, &QPushButton::clicked, this, [this]() {
        emit recursiveToggled(m_btnLayers->isChecked());
    });

    m_layout->addWidget(m_btnLayers, 0, Qt::AlignVCenter);
}

void ContentHeaderWidget::setFilterState(const FilterState& state) {
    m_filterState = state;
    if (m_btnToggleHidden) {
        m_btnToggleHidden->setChecked(state.showHidden);
        m_btnToggleHidden->setIcon(UiHelper::getIcon("eye", state.showHidden ? QColor("#3498db") : QColor("#888888"), 16));
    }
    if (m_btnToggleFolders) {
        m_btnToggleFolders->setChecked(state.showFolders);
        m_btnToggleFolders->setIcon(UiHelper::getIcon("folder_filled", state.showFolders ? QColor("#FDB70A") : QColor("#B0B0B0"), 16));
    }
    if (m_btnToggleFiles) {
        m_btnToggleFiles->setChecked(state.showFiles);
        m_btnToggleFiles->setIcon(UiHelper::getIcon("file", state.showFiles ? QColor("#2ecc71") : QColor("#B0B0B0"), 16));
    }
}

void ContentHeaderWidget::setRecursive(bool recursive) {
    if (m_btnLayers) {
        m_btnLayers->setChecked(recursive);
    }
}

void ContentHeaderWidget::setLayersEnabled(bool enabled, const QString& tooltip) {
    if (m_btnLayers) {
        m_btnLayers->setEnabled(enabled);
        m_btnLayers->setProperty("tooltipText", tooltip);
    }
}

} // namespace QuarkMeta
