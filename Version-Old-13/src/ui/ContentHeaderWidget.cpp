#include "ContentHeaderWidget.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "../core/AppConfig.h"
#include <QEvent>
#include <QCursor>
#include <QStyle>
#include <QMouseEvent>
#include <QApplication>
#include "ContentPanel.h"

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
    m_layout->setContentsMargins(15, 0, 8, 0);
    m_layout->setSpacing(6);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(18, 18);
    m_iconLabel->setPixmap(UiHelper::getIcon("image_picture", QColor("#41F2F2"), 18).pixmap(18, 18));

    m_titleLabel = new QLabel("内容", this);
    m_titleLabel->setObjectName("ContentHeaderTitle");

    m_layout->addWidget(m_iconLabel);
    m_layout->addWidget(m_titleLabel);
    m_layout->addStretch();

    m_btnSplitView = new QPushButton(this);
    m_btnSplitView->setFixedSize(24, 24);
    m_btnSplitView->setIcon(UiHelper::getIcon("columns", QColor("#888888"), 18));
    m_btnSplitView->setProperty("tooltipText", "双窗格分栏视图");
    m_btnSplitView->setObjectName("ViewModeToolBtn");
    m_btnSplitView->installEventFilter(this);

    connect(m_btnSplitView, &QPushButton::clicked, this, [this]() {
        emit splitViewRequested();
    });

    m_layout->addWidget(m_btnSplitView, 0, Qt::AlignVCenter);

    auto setupToggleBtn = [this](QPushButton*& btn, const QString& iconKey, const QColor& activeColor, bool defaultChecked, const QString& tooltip) {
        btn = new QPushButton(this);
        btn->setCheckable(true);
        btn->setFixedSize(24, 24);
        btn->setChecked(defaultChecked);
        btn->setIcon(UiHelper::getIcon(iconKey, defaultChecked ? activeColor : QColor("#888888"), 16));
        btn->setProperty("tooltipText", tooltip);
        btn->setObjectName("ViewModeToolBtn");
        btn->installEventFilter(this);
        m_layout->addWidget(btn, 0, Qt::AlignVCenter);
    };

    setupToggleBtn(m_btnToggleHidden, "eye", QColor("#3498db"), m_filterState.showHidden, "显示/隐藏隐藏项目");
    connect(m_btnToggleHidden, &QPushButton::clicked, this, [this]() {
        m_filterState.showHidden = m_btnToggleHidden->isChecked();
        m_btnToggleHidden->setIcon(UiHelper::getIcon("eye", m_filterState.showHidden ? QColor("#3498db") : QColor("#888888"), 16));
        emit filterStateChanged(m_filterState);
    });

    m_btnLayers = new QPushButton(this);
    m_btnLayers->setCheckable(true);
    m_btnLayers->setFixedSize(24, 24);
    m_btnLayers->setIcon(UiHelper::getIcon("layers", QColor("#888888"), 18));
    m_btnLayers->setProperty("tooltipText", "显示子文件夹中的项目");
    m_btnLayers->setObjectName("ViewModeToolBtn");
    m_btnLayers->installEventFilter(this);

    connect(m_btnLayers, &QPushButton::clicked, this, [this]() {
        bool checked = m_btnLayers->isChecked();
        m_btnLayers->setIcon(UiHelper::getIcon("layers", checked ? QColor("#2ecc71") : QColor("#888888"), 18));
        emit recursiveToggled(checked);
    });

    m_layout->addWidget(m_btnLayers, 0, Qt::AlignVCenter);
}

void ContentHeaderWidget::setTitle(const QString& title) {
    if (m_titleLabel) {
        m_titleLabel->setText(title.isEmpty() ? "内容" : title);
    }
}

void ContentHeaderWidget::setFilterState(const FilterState& state) {
    m_filterState = state;
    if (m_btnToggleHidden) {
        m_btnToggleHidden->setChecked(state.showHidden);
        m_btnToggleHidden->setIcon(UiHelper::getIcon("eye", state.showHidden ? QColor("#3498db") : QColor("#888888"), 16));
    }
}

void ContentHeaderWidget::setRecursive(bool recursive) {
    if (m_btnLayers) {
        m_btnLayers->setChecked(recursive);
        m_btnLayers->setIcon(UiHelper::getIcon("layers", recursive ? QColor("#2ecc71") : QColor("#888888"), 18));
    }
}

void ContentHeaderWidget::setLayersEnabled(bool enabled, const QString& tooltip) {
    if (m_btnLayers) {
        m_btnLayers->setEnabled(enabled);
        m_btnLayers->setProperty("tooltipText", tooltip);
    }
}

void ContentHeaderWidget::setActive(bool active) {
    setProperty("activePane", active ? "true" : "false");
    style()->unpolish(this);
    style()->polish(this);
}

ContentPanel* ContentHeaderWidget::owningPanel() const {
    QWidget* w = parentWidget();
    while (w) {
        ContentPanel* panel = qobject_cast<ContentPanel*>(w);
        if (panel) return panel;
        w = w->parentWidget();
    }
    return nullptr;
}

bool ContentHeaderWidget::evaluateOrientationToggle(const QPoint& currentPos, Qt::Orientation& targetOri) const {
    ContentPanel* panel = owningPanel();
    if (!panel || !panel->isSplitMode() || m_dragStartPos.isNull()) {
        return false;
    }

    Qt::Orientation currentOri = panel->splitOrientation();
    QPoint delta = currentPos - m_dragStartPos;
    int threshold = QApplication::startDragDistance();

    if (currentOri == Qt::Horizontal && delta.y() >= threshold) {
        targetOri = Qt::Vertical;
        return true;
    } else if (currentOri == Qt::Vertical && delta.x() >= threshold) {
        targetOri = Qt::Horizontal;
        return true;
    }

    targetOri = currentOri;
    return false;
}

void ContentHeaderWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QWidget* child = childAt(event->pos());
        if (!qobject_cast<QPushButton*>(child)) {
            ContentPanel* panel = owningPanel();
            if (panel && panel->isSplitMode()) {
                m_dragStartPos = event->pos();
                m_isDraggingHeader = false;
                event->accept();
                return;
            }
        }
    }
    QWidget::mousePressEvent(event);
}

void ContentHeaderWidget::mouseMoveEvent(QMouseEvent* event) {
    if ((event->buttons() & Qt::LeftButton) && !m_dragStartPos.isNull()) {
        if ((event->pos() - m_dragStartPos).manhattanLength() >= QApplication::startDragDistance()) {
            ContentPanel* panel = owningPanel();
            if (panel && panel->isSplitMode()) {
                Qt::Orientation targetOri;
                evaluateOrientationToggle(event->pos(), targetOri);

                if (!m_isDraggingHeader) {
                    m_isDraggingHeader = true;
                    emit orientationDragStarted(targetOri);
                }
                emit orientationDragUpdated(event->globalPosition().toPoint());
                event->accept();
                return;
            }
        }
    }
    QWidget::mouseMoveEvent(event);
}

void ContentHeaderWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (m_isDraggingHeader) {
            ContentPanel* panel = owningPanel();
            if (panel && panel->isSplitMode()) {
                Qt::Orientation targetOri;
                bool shouldToggle = evaluateOrientationToggle(event->pos(), targetOri);

                emit orientationDragEnded(shouldToggle);
                if (shouldToggle) {
                    emit orientationToggleRequested(targetOri);
                }
            }
            m_isDraggingHeader = false;
            m_dragStartPos = QPoint();
            event->accept();
            return;
        }
        m_dragStartPos = QPoint();
    }
    QWidget::mouseReleaseEvent(event);
}

bool ContentHeaderWidget::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::ToolTip) {
        QString text = watched->property("tooltipText").toString();
        if (!text.isEmpty()) {
            ToolTipOverlay::instance()->showText(QCursor::pos(), text, 0);
            return true;
        }
    } else if (event->type() == QEvent::Leave || event->type() == QEvent::MouseButtonPress) {
        ToolTipOverlay::hideTip();
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace QuarkMeta
