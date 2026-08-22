#include "FilterPanel.h"
#include "../core/AppConfig.h"
#include <QSet>
#include "ToolTipOverlay.h"
#include "UiHelper.h"
#include "StyleLibrary.h"
#include "ColorPicker.h"
#include "SearchHistoryPanel.h"
#include "../core/SearchHistoryService.h"
#include <QPushButton>
#include <QMouseEvent>
#include <QCursor>
#include <QGuiApplication>
#include <QScreen>
#include <QPainter>
#include <QComboBox>
#include <QButtonGroup>

using namespace QuarkMeta::Style;

namespace QuarkMeta {

// ─── 颜色映射表 ────────────────────────────────────────────────────
QMap<QString, QColor> FilterPanel::s_colorMap() {
    return {
        { "",        QColor("#888780") },
        { "#E24B4A", QColor("#E24B4A") },
        { "#EF9F27", QColor("#EF9F27") },
        { "#FECF0E", QColor("#FECF0E") },
        { "#639922", QColor("#639922") },
        { "#1D9E75", QColor("#1D9E75") },
        { "#378ADD", QColor("#378ADD") },
        { "#7F77DD", QColor("#7F77DD") },
        { "#5F5E5A", QColor("#5F5E5A") },
        { "#000000", QColor("#000000") },
        { "#FFFFFF", QColor("#FFFFFF") }
    };
}

static QString ratingDisplayName(int r) {
    return r == 0 ? "无评级" : QString("★").repeated(r);
}

// ─── 自定义勾选框 ──────────────────────────────────────────────────
StyledCheckBox::StyledCheckBox(QWidget* parent) : QCheckBox(parent) {
    setFixedSize(15, 15);
}

void StyledCheckBox::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    bool checked = isChecked();
    
    // 使用 QRectF + 0.5px 内缩，确保笔触四边粗细完全一致
    QRectF rect(0.5, 0.5, width() - 1.0, height() - 1.0);
    QColor borderColor = checked ? QColor("#378ADD") : QColor("#444444");
    
    painter.setPen(QPen(borderColor, 1.0));
    painter.setBrush(QColor("#1E1E1E"));
    painter.drawRoundedRect(rect, 2.0, 2.0);

    if (checked) {
        QPen pen(QColor("#378ADD"), 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        QPolygonF checkMark;
        checkMark << QPointF(2.5, 7.5)
                  << QPointF(5.5, 11.0)
                  << QPointF(12.0, 3.5);
        painter.drawPolyline(checkMark);
    }
}

// ─── 可整行点击的行控件 ────────────────────────────────────────────
ClickableRow::ClickableRow(StyledCheckBox* cb, QWidget* parent)
    : QWidget(parent), m_cb(cb) {
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_StyledBackground);
}

void ClickableRow::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        QPoint local = m_cb->mapFromGlobal(e->globalPosition().toPoint());
        if (!m_cb->rect().contains(local)) {
            m_cb->setChecked(!m_cb->isChecked());
        }
    }
    QWidget::mousePressEvent(e);
}

void ClickableRow::enterEvent(QEnterEvent* e) {
    setStyleSheet("QWidget { background: #2A2A2A; border-radius: 4px; }");
    QWidget::enterEvent(e);
}

void ClickableRow::leaveEvent(QEvent* e) {
    setStyleSheet("");
    QWidget::leaveEvent(e);
}

// ─── FilterPanel ──────────────────────────────────────────────────

void FilterPanel::syncUIFromFilterState() {
    updateHeaderStatus();
    
    QList<StyledCheckBox*> allCheckBoxes = findChildren<StyledCheckBox*>();
    for (auto* cb : allCheckBoxes) {
        ClickableRow* row = qobject_cast<ClickableRow*>(cb->parentWidget());
        if (!row) continue;
        
        QLabel* labelWidget = row->findChild<QLabel*>();
        if (!labelWidget) continue;
        
        QString text = labelWidget->text();
        bool shouldCheck = false;
        
        // 1. 评级匹配
        if (text == "无评级") shouldCheck = m_filter.ratings.contains(0);
        else if (text.contains("★")) shouldCheck = m_filter.ratings.contains(text.count("★"));
        
        // 2. 颜色匹配
        else if (text == "无色标") shouldCheck = (m_filter.colors.contains("无色标") || m_filter.colors.contains(""));
        else if (m_filter.colors.contains(text)) shouldCheck = true;

        // 3. 类型/日期匹配
        else if (m_filter.types.contains(text)) shouldCheck = true;
        else if (m_filter.createDates.contains(text)) shouldCheck = true;
        else if (m_filter.modifyDates.contains(text)) shouldCheck = true;
        
        // 4. 链接/备注/比例匹配
        else if (text == "有链接") shouldCheck = (m_filter.linkPresence == FilterState::Yes);
        else if (text == "无链接") shouldCheck = (m_filter.linkPresence == FilterState::No);
        else if (text == "有备注") shouldCheck = (m_filter.notePresence == FilterState::Yes);
        else if (text == "重复项") shouldCheck = (m_filter.duplicatePresence == FilterState::DuplicateOnly);
        else if (text == "未重复") shouldCheck = (m_filter.duplicatePresence == FilterState::UniqueOnly);
        else if (text == "无备注") shouldCheck = (m_filter.notePresence == FilterState::No);
        else if (text == "横图") shouldCheck = (m_filter.ratio == FilterState::Horizontal);
        else if (text == "竖图") shouldCheck = (m_filter.ratio == FilterState::Vertical);
        else if (text == "方形") shouldCheck = (m_filter.ratio == FilterState::Square);
        else if (text == "16:9") shouldCheck = (m_filter.ratio == FilterState::Ratio169);
        else if (text == "无缩略图 (失败/跳过)") shouldCheck = m_filter.noThumbnailOnly;

        cb->blockSignals(true);
        cb->setChecked(shouldCheck);
        cb->blockSignals(false);
    }
}

FilterPanel::FilterPanel(QWidget* parent) : QFrame(parent) {
    setContextMenuPolicy(Qt::CustomContextMenu);

    setObjectName("FilterContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);
    setStyleSheet("color: #EEEEEE;");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // 顶部标题栏
    QWidget* topBar = new QWidget(this);
    topBar->setObjectName("ContainerHeader");
    topBar->setFixedHeight(32);
    topBar->setStyleSheet(
        "QWidget#ContainerHeader {"
        "  background-color: #252526;"
        "  border-bottom: none;"
        "}"
    );
    QHBoxLayout* topL = new QHBoxLayout(topBar);
    topL->setContentsMargins(15, 0, 5, 0);
    topL->setSpacing(5);

    m_iconLabel = new QLabel(topBar);
    topL->addWidget(m_iconLabel);

    m_titleLabel = new QLabel("筛选", topBar);
    topL->addWidget(m_titleLabel);

    m_btnClearAll = new QPushButton(topBar);
    m_btnClearAll->setFixedSize(24, 24);
    m_btnClearAll->setIcon(UiHelper::getIcon("reset_filter", QColor("#B0B0B0")));
    m_btnClearAll->setIconSize(QSize(16, 16));
    m_btnClearAll->setFlat(true);
    m_btnClearAll->setCursor(Qt::PointingHandCursor);
    m_btnClearAll->setProperty("tooltipText", "重置所有筛选条件");
    m_btnClearAll->installEventFilter(this);
    m_btnClearAll->setStyleSheet(
        "QPushButton { background: transparent; border: none; border-radius: 4px; }"
        "QPushButton:hover { background: #3E3E42; }"
        "QPushButton:pressed { background: #4E4E52; }");
    connect(m_btnClearAll, &QPushButton::clicked, this, [this]() { clearAllFilters(true); });

    m_btnPin = new QPushButton(topBar);
    m_btnPin->setFixedSize(24, 24);
    m_btnPin->setIcon(UiHelper::getIcon("pin_tilted", QColor("#B0B0B0")));
    m_btnPin->setIconSize(QSize(18, 18));
    m_btnPin->setFlat(true);
    m_btnPin->setCursor(Qt::PointingHandCursor);
    m_btnPin->setProperty("tooltipText", "锁定当前筛选条件");
    m_btnPin->installEventFilter(this);
    m_btnPin->setStyleSheet(
        "QPushButton { background: transparent; border: none; border-radius: 4px; }"
        "QPushButton:hover { background: #3E3E42; }"
        "QPushButton:pressed { background: #4E4E52; }");
    connect(m_btnPin, &QPushButton::clicked, this, [this]() {
        m_isFilterPinned = !m_isFilterPinned;
        if (m_isFilterPinned) {
            m_btnPin->setIcon(UiHelper::getIcon("pin", Style::ActiveOrange));
            m_btnPin->setProperty("tooltipText", "当前筛选条件已锁定（目录切换不重置）");
        } else {
            m_btnPin->setIcon(UiHelper::getIcon("pin_tilted", QColor("#B0B0B0")));
            m_btnPin->setProperty("tooltipText", "锁定当前筛选条件");
        }
    });

    m_btnToggleGroups = new QPushButton(topBar);
    m_btnToggleGroups->setFixedSize(24, 24);
    m_btnToggleGroups->setFlat(true);
    m_btnToggleGroups->setCursor(Qt::PointingHandCursor);
    m_btnToggleGroups->installEventFilter(this);
    m_btnToggleGroups->setStyleSheet(
        "QPushButton { background: transparent; border: none; border-radius: 4px; }"
        "QPushButton:hover { background: #3E3E42; }"
        "QPushButton:pressed { background: #4E4E52; }");
    connect(m_btnToggleGroups, &QPushButton::clicked, this, &FilterPanel::onToggleAllGroupsClicked);

    topL->addStretch();
    topL->addWidget(m_btnPin, 0, Qt::AlignVCenter);
    topL->addWidget(m_btnToggleGroups, 0, Qt::AlignVCenter);
    topL->addWidget(m_btnClearAll, 0, Qt::AlignVCenter);
    m_mainLayout->addWidget(topBar);

    updateHeaderStatus();

    // 滚动内容区
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setStyleSheet(
        "QScrollArea { border: none; background: transparent; }"
        "QScrollBar:vertical { border-left: 1px solid #333333; }"
    );

    m_container = new QWidget(m_scrollArea);
    m_container->setStyleSheet("QWidget { background: transparent; }");
    m_containerLayout = new QVBoxLayout(m_container);
    m_containerLayout->setContentsMargins(0, 0, 0, 10); 
    m_containerLayout->setSpacing(0);
    m_containerLayout->addStretch();

    m_scrollArea->setWidget(m_container);
    m_mainLayout->addWidget(m_scrollArea, 1);

    m_historyPanel = new SearchHistoryPanel(this);

    connect(this, &FilterPanel::filterChanged, this, &FilterPanel::updateHeaderStatus);
}

void FilterPanel::saveFilterHistory(const QString& key, const QString& text) {
    if (text.trimmed().isEmpty()) return;
    SearchHistoryService::instance().appendSearch(key, text);
}

QStringList FilterPanel::getFilterHistory(const QString& key) const {
    return SearchHistoryService::instance().getHistory(key);
}

bool FilterPanel::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseButtonDblClick) {
        QLineEdit* edit = qobject_cast<QLineEdit*>(watched);
        if (edit && edit->objectName() == "FilterSearchEdit") {
            QString key;
            if (edit == m_editType) key = "Type";
            else if (edit == m_editCreateDate) key = "CreateDate";
            else if (edit == m_editModifyDate) key = "ModifyDate";

            if (!key.isEmpty()) {
                m_historyPanel->setCategory(key);
                QStringList history = getFilterHistory(key);
                m_historyPanel->setHistory(history, "最近搜索");
                
                m_historyPanel->disconnect(this);

                connect(m_historyPanel, &SearchHistoryPanel::historyItemClicked, this, [this, edit, key](const QString& text) {
                    edit->setText(text);
                    if (edit == m_editType) m_filter.typeFilterText = text;
                    else if (edit == m_editCreateDate) m_filter.createDateFilterText = text;
                    else if (edit == m_editModifyDate) m_filter.modifyDateFilterText = text;

                    saveFilterHistory(key, text);
                    emit filterChanged(m_filter);
                    m_historyPanel->hide();
                });

                m_historyPanel->showBelow(edit);
                return true;
            }
        }
    }

    if (event->type() == QEvent::HoverEnter) {
        QString text = watched->property("tooltipText").toString();
        if (!text.isEmpty()) {
            ToolTipOverlay::instance()->showText(QCursor::pos(), text, 0);
        }
    } else if (event->type() == QEvent::HoverLeave || event->type() == QEvent::MouseButtonRelease) {
        ToolTipOverlay::hideTip();
    }
    
    return QWidget::eventFilter(watched, event);
}

// ─── populate ─────────────────────────────────────────────────────
void FilterPanel::populateStats(const QuarkMeta::ScanStats& stats) {
    m_currentStats = stats;
    m_ratingCounts = stats.ratingCounts;
    m_colorCounts = stats.colorCounts;
    m_typeCounts = stats.typeCounts;
    m_createDateCounts = stats.createDateCounts;
    m_modifyDateCounts = stats.modifyDateCounts;
    m_emptyFolderCount = stats.emptyFolderCount;

    rebuildGroups();
}

void FilterPanel::populate(
    const QMap<int, int>&       ratingCounts,
    const QMap<QString, int>&   colorCounts,
    const QMap<QString, int>&   typeCounts,
    const QMap<QString, int>&   createDateCounts,
    const QMap<QString, int>&   modifyDateCounts,
    int                         emptyFolderCount)
{
    if (ratingCounts.isEmpty() && colorCounts.isEmpty() &&
        typeCounts.isEmpty() && createDateCounts.isEmpty() && modifyDateCounts.isEmpty() &&
        emptyFolderCount == 0 &&
        m_filter.typeFilterText.isEmpty() && m_filter.createDateFilterText.isEmpty() &&
        m_filter.modifyDateFilterText.isEmpty()) {
        return;
    }

    auto getNonZeroColorKeys = [](const QMap<QString, int>& counts) {
        QSet<QString> keys;
        for (auto it = counts.begin(); it != counts.end(); ++it) {
            if (it.value() > 0) keys.insert(it.key());
        }
        return keys;
    };

    bool structureChanged = (m_ratingCounts.keys() != ratingCounts.keys() ||
                             getNonZeroColorKeys(m_colorCounts) != getNonZeroColorKeys(colorCounts) ||
                             m_typeCounts.keys() != typeCounts.keys() ||
                             m_createDateCounts.keys() != createDateCounts.keys() ||
                             m_modifyDateCounts.keys() != modifyDateCounts.keys() ||
                             m_emptyFolderCount != emptyFolderCount);

    m_ratingCounts     = ratingCounts;
    m_colorCounts      = colorCounts;
    m_typeCounts       = typeCounts;
    m_createDateCounts = createDateCounts;
    m_modifyDateCounts = modifyDateCounts;
    m_emptyFolderCount = emptyFolderCount;

    if (structureChanged) {
        rebuildGroups();
    } else {
        syncUIFromFilterState();
        QList<ClickableRow*> rows = m_container->findChildren<ClickableRow*>();
        for (auto* row : rows) {
             QList<QLabel*> labels = row->findChildren<QLabel*>();
             if (labels.size() >= 2) {
                 QLabel* cntLabel = labels.last();
                 QLabel* nameLabel = labels.at(labels.size() - 2);
                 QString name = nameLabel->text();
                 
                 int count = 0;
                 if (name == "无评级") count = m_ratingCounts.value(0, 0);
                 else if (name.contains("★")) count = m_ratingCounts.value(name.count("★"), 0);
                 else if (name == "空文件夹") count = m_emptyFolderCount;
                 else if (name == "文件夹") count = m_typeCounts.value("folder", 0);
                 else if (name == "文件") count = m_typeCounts.value("file", 0);
                 else if (m_typeCounts.contains(name)) count = m_typeCounts.value(name, 0);
                 else if (m_createDateCounts.contains(name)) count = m_createDateCounts.value(name, 0);
                 else if (m_modifyDateCounts.contains(name)) count = m_modifyDateCounts.value(name, 0);
                 else if (name == "红色") count = m_colorCounts.value("#E24B4A", m_colorCounts.value("红色", 0));
                 else if (name == "橙色") count = m_colorCounts.value("#EF9F27", m_colorCounts.value("橙色", 0));
                 else if (name == "黄色") count = m_colorCounts.value("#FECF0E", m_colorCounts.value("黄色", 0));
                 else if (name == "绿色") count = m_colorCounts.value("#639922", m_colorCounts.value("绿色", 0));
                 else if (name == "青色") count = m_colorCounts.value("#1D9E75", m_colorCounts.value("青色", 0));
                 else if (name == "蓝色") count = m_colorCounts.value("#378ADD", m_colorCounts.value("蓝色", 0));
                 else if (name == "紫色") count = m_colorCounts.value("#7F77DD", m_colorCounts.value("紫色", 0));
                 else if (name == "灰色") count = m_colorCounts.value("#5F5E5A", m_colorCounts.value("灰色", 0));
                 else if (name == "无色标") count = m_colorCounts.value("", m_colorCounts.value("无色标", 0));
                 else if (name == "未重复") count = m_currentStats.uniqueCount;
                 else if (name == "重复项") count = m_currentStats.duplicateCount;
                 else if (name == "横图") count = m_currentStats.ratioHorizontalCount;
                 else if (name == "竖图") count = m_currentStats.ratioVerticalCount;
                 else if (name == "方形") count = m_currentStats.ratioSquareCount;
                 else if (name == "16:9") count = m_currentStats.ratio169Count;
                 else if (name == "有链接") count = m_currentStats.hasLinkCount;
                 else if (name == "无链接") count = m_currentStats.noLinkCount;
                 else if (name == "有备注") count = m_currentStats.hasNoteCount;
                 else if (name == "无备注") count = m_currentStats.noNoteCount;
                 else if (name == "无缩略图 (失败/跳过)") count = m_currentStats.noThumbnailCount;

                 cntLabel->setText(QString::number(count));
             }
        }
    }
}

void FilterPanel::rebuildDateCheckboxes(bool isCreateDate, bool descending) {
    QVBoxLayout* layout = isCreateDate ? m_createDateLayout : m_modifyDateLayout;
    if (!layout) return;

    while (layout->count() > 1) {
        QLayoutItem* item = layout->takeAt(1);
        if (item->widget()) {
            item->widget()->hide();
            item->widget()->setParent(nullptr);
            item->widget()->deleteLater();
        }
        delete item;
    }

    const QMap<QString, int>& counts = isCreateDate ? m_createDateCounts : m_modifyDateCounts;
    QStringList& selected = isCreateDate ? m_filter.createDates : m_filter.modifyDates;

    QStringList dates = counts.keys();
    std::sort(dates.begin(), dates.end(), [descending](const QString& a, const QString& b) {
        return descending ? (a > b) : (a < b);
    });

    for (const QString& d : dates) {
        QCheckBox* cb = addFilterRow(layout, d, counts[d]);
        cb->blockSignals(true);
        cb->setChecked(selected.contains(d));
        cb->blockSignals(false);
        connect(cb, &QCheckBox::toggled, this, [this, isCreateDate, d](bool on) {
            QStringList& targetList = isCreateDate ? m_filter.createDates : m_filter.modifyDates;
            if (on) { if (!targetList.contains(d)) targetList.append(d); }
            else targetList.removeAll(d);
            emit filterChanged(m_filter);
        });
    }
}

// ─── rebuildGroups ────────────────────────────────────────────────
void FilterPanel::rebuildGroups() {
    updateHeaderStatus();
    m_groupHeaders.clear();

    m_editType = nullptr;
    m_editCreateDate = nullptr;
    m_editModifyDate = nullptr;
    m_createDateLayout = nullptr;
    m_modifyDateLayout = nullptr;

    while (m_containerLayout->count() > 1) {
        QLayoutItem* item = m_containerLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->hide();
            item->widget()->setParent(nullptr);
            item->widget()->deleteLater();
        }
        delete item;
    }

    // ── 1. 评级 ──────────────────────────────────────────────
    if (!m_ratingCounts.isEmpty()) {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("评级", gl);
        m_groupRating = g;
        for (int r : {0, 1, 2, 3, 4, 5}) {
            if (!m_ratingCounts.contains(r) || m_ratingCounts[r] <= 0) continue;
            QCheckBox* cb = addFilterRow(gl, ratingDisplayName(r), m_ratingCounts[r]);
            cb->blockSignals(true);
            cb->setChecked(m_filter.ratings.contains(r));
            cb->blockSignals(false);
            connect(cb, &QCheckBox::toggled, this, [this, r](bool on) {
                if (on) { if (!m_filter.ratings.contains(r)) m_filter.ratings.append(r); }
                else m_filter.ratings.removeAll(r);
                emit filterChanged(m_filter);
            });
        }

        if (m_currentStats.noThumbnailCount > 0 || m_filter.noThumbnailOnly) {
            QCheckBox* cb = addFilterRow(gl, "无缩略图 (失败/跳过)", m_currentStats.noThumbnailCount);
            cb->blockSignals(true);
            cb->setChecked(m_filter.noThumbnailOnly);
            cb->blockSignals(false);
            connect(cb, &QCheckBox::toggled, this, [this](bool on) {
                m_filter.noThumbnailOnly = on;
                emit filterChanged(m_filter);
            });
        }
        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }

    // ── 2. 颜色标记 (动态呈现：有对应文件时才显示) ────────────
    {
        static const struct { QString name; QString hex; QColor color; } colorsList[] = {
            {"无色标", "",        QColor("#808080")},
            {"红色",   "#E24B4A", QColor("#E24B4A")},
            {"橙色",   "#EF9F27", QColor("#EF9F27")},
            {"黄色",   "#FECF0E", QColor("#FECF0E")},
            {"绿色",   "#639922", QColor("#639922")},
            {"青色",   "#1D9E75", QColor("#1D9E75")},
            {"蓝色",   "#378ADD", QColor("#378ADD")},
            {"紫色",   "#7F77DD", QColor("#7F77DD")},
            {"灰色",   "#5F5E5A", QColor("#5F5E5A")}
        };

        bool hasAnyColor = false;
        for (const auto& item : colorsList) {
            int cnt = m_colorCounts.value(item.hex, m_colorCounts.value(item.name, 0));
            bool isChecked = (m_filter.colors.contains(item.name) || m_filter.colors.contains(item.hex));
            if (cnt > 0 || isChecked) {
                hasAnyColor = true;
                break;
            }
        }

        if (hasAnyColor) {
            QVBoxLayout* gl = nullptr;
            QHBoxLayout* hdrLayout = nullptr;
            QWidget* g = buildGroup("颜色标记", gl, &hdrLayout);
            m_groupColor = g;

            for (const auto& item : colorsList) {
                int cnt = m_colorCounts.value(item.hex, m_colorCounts.value(item.name, 0));
                bool isChecked = (m_filter.colors.contains(item.name) || m_filter.colors.contains(item.hex));

                // 🚨 动态规则：仅在有对应文件或当前已勾选时才显示！
                if (cnt == 0 && !isChecked) {
                    continue;
                }

                QCheckBox* cb = addFilterRow(gl, item.name, cnt, item.color);
                cb->setChecked(isChecked);
                connect(cb, &QCheckBox::checkStateChanged, this, [this, name = item.name, hex = item.hex](Qt::CheckState state) {
                    if (state == Qt::Checked) {
                        if (!m_filter.colors.contains(name)) m_filter.colors.append(name);
                    } else {
                        m_filter.colors.removeAll(name);
                        m_filter.colors.removeAll(hex);
                    }
                    emit filterChanged(m_filter);
                });
            }

            m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
        }
    }

    // ── 4. 文件类型 ──────────────────────────────────────────
    if (!m_typeCounts.isEmpty() || !m_filter.typeFilterText.isEmpty() || m_emptyFolderCount > 0) {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("文件类型", gl);

        QWidget* wType = new QWidget(g);
        QHBoxLayout* lType = new QHBoxLayout(wType);
        lType->setContentsMargins(5, 6, 5, 4);
        lType->setSpacing(0);

        m_editType = new QLineEdit(wType);
        m_editType->setClearButtonEnabled(true);
        m_editType->setPlaceholderText("例： png / 文件夹...");
        m_editType->setText(m_filter.typeFilterText);
        m_editType->setObjectName("FilterSearchEdit");
        m_editType->setFixedHeight(22);
        m_editType->setStyleSheet(
            "QLineEdit#FilterSearchEdit {"
            "  background: #2D2D2D;"
            "  color: #CCCCCC;"
            "  border: 1px solid #444444;"
            "  border-radius: 4px;"
            "  padding: 0px 6px;"
            "  font-size: 11px;"
            "}"
            "QLineEdit#FilterSearchEdit:focus { border-color: #378ADD; color: #FFFFFF; }"
        );
        m_editType->installEventFilter(this);
        connect(m_editType, &QLineEdit::returnPressed, this, [this]() {
            m_filter.typeFilterText = m_editType->text();
            saveFilterHistory("Type", m_filter.typeFilterText);
            emit filterChanged(m_filter);
        });
        connect(m_editType, &QLineEdit::textChanged, this, [this](const QString& text) {
            if (text.isEmpty() && !m_filter.typeFilterText.isEmpty()) {
                m_filter.typeFilterText = "";
                emit filterChanged(m_filter);
            }
        });
        lType->addWidget(m_editType);
        gl->addWidget(wType);

        if (m_emptyFolderCount > 0) {
            QCheckBox* cb = addFilterRow(gl, "空文件夹", m_emptyFolderCount);
            cb->blockSignals(true);
            cb->setChecked(m_filter.types.contains("空文件夹"));
            cb->blockSignals(false);
            connect(cb, &QCheckBox::toggled, this, [this](bool on) {
                if (on) { if (!m_filter.types.contains("空文件夹")) m_filter.types.append("空文件夹"); }
                else    m_filter.types.removeAll("空文件夹");
                emit filterChanged(m_filter);
            });
        }

        if (m_typeCounts.contains("folder") && m_typeCounts["folder"] > 0) {
            QCheckBox* cb = addFilterRow(gl, "文件夹", m_typeCounts["folder"]);
            cb->blockSignals(true);
            cb->setChecked(m_filter.types.contains("folder"));
            cb->blockSignals(false);
            connect(cb, &QCheckBox::toggled, this, [this](bool on) {
                if (on) { if (!m_filter.types.contains("folder")) m_filter.types.append("folder"); }
                else    m_filter.types.removeAll("folder");
                emit filterChanged(m_filter);
            });
        }
        if (m_typeCounts.contains("file") && m_typeCounts["file"] > 0) {
            QCheckBox* cb = addFilterRow(gl, "文件", m_typeCounts["file"]);
            cb->blockSignals(true);
            cb->setChecked(m_filter.types.contains("file"));
            cb->blockSignals(false);
            connect(cb, &QCheckBox::toggled, this, [this](bool on) {
                if (on) { if (!m_filter.types.contains("file")) m_filter.types.append("file"); }
                else    m_filter.types.removeAll("file");
                emit filterChanged(m_filter);
            });
        }
        QStringList exts = m_typeCounts.keys(); exts.sort();
        for (const QString& ext : exts) {
            if (ext == "folder" || ext == "file" || ext == "空文件夹" || m_typeCounts[ext] <= 0) continue;
            QString label = ext.isEmpty() ? "无扩展名" : ext;
            QCheckBox* cb = addFilterRow(gl, label, m_typeCounts[ext]);
            cb->blockSignals(true);
            cb->setChecked(m_filter.types.contains(ext));
            cb->blockSignals(false);
            connect(cb, &QCheckBox::toggled, this, [this, ext](bool on) {
                if (on) { if (!m_filter.types.contains(ext)) m_filter.types.append(ext); }
                else m_filter.types.removeAll(ext);
                emit filterChanged(m_filter);
            });
        }
        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }

    // ── 5. 创建日期 (Plan-92: 排序支持) ──────────────────────────
    if (!m_createDateCounts.isEmpty() || !m_filter.createDateFilterText.isEmpty()) {
        QVBoxLayout* gl = nullptr;
        QHBoxLayout* hdrLayout = nullptr;
        QWidget* g = buildGroup("创建日期", gl, &hdrLayout);
        m_createDateLayout = gl;

        QPushButton* btnSort = new QPushButton(g);
        btnSort->setFixedSize(16, 16);
        btnSort->setIcon(UiHelper::getIcon(m_createDateDesc ? "arrow_down" : "arrow_up", QColor("#B0B0B0")));
        btnSort->setFlat(true);
        btnSort->setCursor(Qt::PointingHandCursor);
        btnSort->setStyleSheet("QPushButton { background: transparent; border: none; } QPushButton:hover { background: #3E3E42; border-radius: 2px; }");
        hdrLayout->addWidget(btnSort);
        connect(btnSort, &QPushButton::clicked, this, [this, btnSort]() {
            m_createDateDesc = !m_createDateDesc;
            btnSort->setIcon(UiHelper::getIcon(m_createDateDesc ? "arrow_down" : "arrow_up", QColor("#B0B0B0")));
            rebuildDateCheckboxes(true, m_createDateDesc);
        });

        QWidget* wCreateDate = new QWidget(g);
        QHBoxLayout* lCreateDate = new QHBoxLayout(wCreateDate);
        lCreateDate->setContentsMargins(5, 6, 5, 4);
        lCreateDate->setSpacing(0);

        m_editCreateDate = new QLineEdit(wCreateDate);
        m_editCreateDate->setClearButtonEnabled(true);
        m_editCreateDate->setPlaceholderText("例： 2025 / 03-2025...");
        m_editCreateDate->setText(m_filter.createDateFilterText);
        m_editCreateDate->setObjectName("FilterSearchEdit");
        m_editCreateDate->setFixedHeight(22);
        m_editCreateDate->setStyleSheet(
            "QLineEdit#FilterSearchEdit {"
            "  background: #2D2D2D;"
            "  color: #CCCCCC;"
            "  border: 1px solid #444444;"
            "  border-radius: 4px;"
            "  padding: 0px 6px;"
            "  font-size: 11px;"
            "}"
            "QLineEdit#FilterSearchEdit:focus { border-color: #378ADD; color: #FFFFFF; }"
        );
        m_editCreateDate->installEventFilter(this);
        connect(m_editCreateDate, &QLineEdit::returnPressed, this, [this]() {
            m_filter.createDateFilterText = m_editCreateDate->text();
            saveFilterHistory("CreateDate", m_filter.createDateFilterText);
            emit filterChanged(m_filter);
        });
        connect(m_editCreateDate, &QLineEdit::textChanged, this, [this](const QString& text) {
            if (text.isEmpty() && !m_filter.createDateFilterText.isEmpty()) {
                m_filter.createDateFilterText = "";
                emit filterChanged(m_filter);
            }
        });
        lCreateDate->addWidget(m_editCreateDate);
        gl->addWidget(wCreateDate);

        rebuildDateCheckboxes(true, m_createDateDesc);
        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }

    // ── 6. 修改日期 (Plan-92: 排序支持) ──────────────────────────
    if (!m_modifyDateCounts.isEmpty() || !m_filter.modifyDateFilterText.isEmpty()) {
        QVBoxLayout* gl = nullptr;
        QHBoxLayout* hdrLayout = nullptr;
        QWidget* g = buildGroup("修改日期", gl, &hdrLayout);
        m_modifyDateLayout = gl;

        QPushButton* btnSort = new QPushButton(g);
        btnSort->setFixedSize(16, 16);
        btnSort->setIcon(UiHelper::getIcon(m_modifyDateDesc ? "arrow_down" : "arrow_up", QColor("#B0B0B0")));
        btnSort->setFlat(true);
        btnSort->setCursor(Qt::PointingHandCursor);
        btnSort->setStyleSheet("QPushButton { background: transparent; border: none; } QPushButton:hover { background: #3E3E42; border-radius: 2px; }");
        hdrLayout->addWidget(btnSort);
        connect(btnSort, &QPushButton::clicked, this, [this, btnSort]() {
            m_modifyDateDesc = !m_modifyDateDesc;
            btnSort->setIcon(UiHelper::getIcon(m_modifyDateDesc ? "arrow_down" : "arrow_up", QColor("#B0B0B0")));
            rebuildDateCheckboxes(false, m_modifyDateDesc);
        });

        QWidget* wModifyDate = new QWidget(g);
        QHBoxLayout* lModifyDate = new QHBoxLayout(wModifyDate);
        lModifyDate->setContentsMargins(5, 6, 5, 4);
        lModifyDate->setSpacing(0);

        m_editModifyDate = new QLineEdit(wModifyDate);
        m_editModifyDate->setClearButtonEnabled(true);
        m_editModifyDate->setPlaceholderText("例： 2025 / 03-2025...");
        m_editModifyDate->setText(m_filter.modifyDateFilterText);
        m_editModifyDate->setObjectName("FilterSearchEdit");
        m_editModifyDate->setFixedHeight(22);
        m_editModifyDate->setStyleSheet(
            "QLineEdit#FilterSearchEdit {"
            "  background: #2D2D2D;"
            "  color: #CCCCCC;"
            "  border: 1px solid #444444;"
            "  border-radius: 4px;"
            "  padding: 0px 6px;"
            "  font-size: 11px;"
            "}"
            "QLineEdit#FilterSearchEdit:focus { border-color: #378ADD; color: #FFFFFF; }"
        );
        m_editModifyDate->installEventFilter(this);
        connect(m_editModifyDate, &QLineEdit::returnPressed, this, [this]() {
            m_filter.modifyDateFilterText = m_editModifyDate->text();
            saveFilterHistory("ModifyDate", m_filter.modifyDateFilterText);
            emit filterChanged(m_filter);
        });
        connect(m_editModifyDate, &QLineEdit::textChanged, this, [this](const QString& text) {
            if (text.isEmpty() && !m_filter.modifyDateFilterText.isEmpty()) {
                m_filter.modifyDateFilterText = "";
                emit filterChanged(m_filter);
            }
        });
        lModifyDate->addWidget(m_editModifyDate);
        gl->addWidget(wModifyDate);

        rebuildDateCheckboxes(false, m_modifyDateDesc);
        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }

    // ── 7. 链接 (独立主选项) ──────────────────────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("链接", gl);
        m_groupLink = g;

        QButtonGroup* linkGroup = new QButtonGroup(g);
        linkGroup->setExclusive(false);

        QCheckBox* cbYes = addFilterRow(gl, "有链接", m_currentStats.hasLinkCount);
        if (m_filter.linkPresence == FilterState::Yes) cbYes->setChecked(true);
        connect(cbYes, &QCheckBox::toggled, this, [this, linkGroup, cbYes](bool on) {
            if (on) {
                for (QAbstractButton* b : linkGroup->buttons()) if (b != cbYes && b->isChecked()) b->setChecked(false);
                m_filter.linkPresence = FilterState::Yes;
            } else m_filter.linkPresence = FilterState::All;
            emit filterChanged(m_filter);
        });
        linkGroup->addButton(cbYes);

        QCheckBox* cbNo = addFilterRow(gl, "无链接", m_currentStats.noLinkCount);
        if (m_filter.linkPresence == FilterState::No) cbNo->setChecked(true);
        connect(cbNo, &QCheckBox::toggled, this, [this, linkGroup, cbNo](bool on) {
            if (on) {
                for (QAbstractButton* b : linkGroup->buttons()) if (b != cbNo && b->isChecked()) b->setChecked(false);
                m_filter.linkPresence = FilterState::No;
            } else m_filter.linkPresence = FilterState::All;
            emit filterChanged(m_filter);
        });
        linkGroup->addButton(cbNo);

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }

    // ── 8. 备注 (独立主选项) ──────────────────────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("备注", gl);
        m_groupNote = g;

        QButtonGroup* noteGroup = new QButtonGroup(g);
        noteGroup->setExclusive(false);

        QCheckBox* cbYes = addFilterRow(gl, "有备注", m_currentStats.hasNoteCount);
        if (m_filter.notePresence == FilterState::Yes) cbYes->setChecked(true);
        connect(cbYes, &QCheckBox::toggled, this, [this, noteGroup, cbYes](bool on) {
            if (on) {
                for (QAbstractButton* b : noteGroup->buttons()) if (b != cbYes && b->isChecked()) b->setChecked(false);
                m_filter.notePresence = FilterState::Yes;
            } else m_filter.notePresence = FilterState::All;
            emit filterChanged(m_filter);
        });
        noteGroup->addButton(cbYes);

        QCheckBox* cbNo = addFilterRow(gl, "无备注", m_currentStats.noNoteCount);
        if (m_filter.notePresence == FilterState::No) cbNo->setChecked(true);
        connect(cbNo, &QCheckBox::toggled, this, [this, noteGroup, cbNo](bool on) {
            if (on) {
                for (QAbstractButton* b : noteGroup->buttons()) if (b != cbNo && b->isChecked()) b->setChecked(false);
                m_filter.notePresence = FilterState::No;
            } else m_filter.notePresence = FilterState::All;
            emit filterChanged(m_filter);
        });
        noteGroup->addButton(cbNo);

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }

    // ── 9. 文件大小 (独立主选项) ──────────────────────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("文件大小", gl);

        QHBoxLayout* hs = new QHBoxLayout();
        hs->setContentsMargins(5, 4, 5, 8);
        hs->setSpacing(8);
        
        QLineEdit* minEdit = new QLineEdit(g);
        minEdit->setClearButtonEnabled(true);
        QLineEdit* maxEdit = new QLineEdit(g);
        maxEdit->setClearButtonEnabled(true);
        QComboBox* unitCombo = new QComboBox(g);
        unitCombo->addItems({"KB", "MB", "GB"});
        unitCombo->setCurrentIndex(1);

        auto sizeEditStyle = "QLineEdit { background: #2D2D2D; color: #EEE; border: 1px solid #444; border-radius: 4px; padding: 2px 4px; font-size: 11px; }";
        minEdit->setStyleSheet(sizeEditStyle);
        maxEdit->setStyleSheet(sizeEditStyle);
        minEdit->setPlaceholderText("最小");
        maxEdit->setPlaceholderText("最大");
        minEdit->setFixedHeight(24);
        maxEdit->setFixedHeight(24);

        QString arrowPath = UiHelper::getSvgTempFilePath("menu_triangle", QColor("#AAAAAA"));
        unitCombo->setFixedHeight(24);
        unitCombo->setFixedWidth(52); 
        unitCombo->setStyleSheet(QString(
            "QComboBox { background: #2D2D2D; color: #EEEEEE; border: 1px solid #444444; border-radius: 4px; font-size: 11px; padding-left: 6px; }"
            "QComboBox::drop-down { border: none; width: 18px; }"
            "QComboBox::down-arrow { image: url(%1); width: 10px; height: 10px; }"
            "QComboBox QAbstractItemView { background-color: #252526; color: #EEEEEE; selection-background-color: #3E3E42; border: 1px solid #444444; outline: none; }"
        ).arg(arrowPath));

        hs->addWidget(minEdit);
        QLabel* sep = new QLabel("-", g); sep->setStyleSheet("color: #AAA;"); hs->addWidget(sep);
        hs->addWidget(maxEdit);
        hs->addWidget(unitCombo);
        gl->addLayout(hs);

        auto updateSizeFilter = [this, minEdit, maxEdit, unitCombo]() {
            auto toBytes = [](const QString& txt, const QString& unit) -> long long {
                if (txt.isEmpty()) return -1;
                bool ok;
                double val = txt.toDouble(&ok);
                if (!ok) return -1;
                long long factor = 1024;
                if (unit == "MB") factor = 1024 * 1024;
                else if (unit == "GB") factor = 1024 * 1024 * 1024;
                return (long long)(val * factor);
            };
            m_filter.minSize = toBytes(minEdit->text(), unitCombo->currentText());
            m_filter.maxSize = toBytes(maxEdit->text(), unitCombo->currentText());
            emit filterChanged(m_filter);
        };

        connect(minEdit, &QLineEdit::editingFinished, this, updateSizeFilter);
        connect(minEdit, &QLineEdit::textChanged, this, [updateSizeFilter](const QString& text) {
            if (text.isEmpty()) updateSizeFilter();
        });
        connect(maxEdit, &QLineEdit::editingFinished, this, updateSizeFilter);
        connect(maxEdit, &QLineEdit::textChanged, this, [updateSizeFilter](const QString& text) {
            if (text.isEmpty()) updateSizeFilter();
        });
        connect(unitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [updateSizeFilter](int){ updateSizeFilter(); });

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }

    // ── 11. 图像比例 (独立主选项) ──────────────────────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("图像比例", gl);
        m_groupRatio = g;

        QButtonGroup* ratioGroup = new QButtonGroup(g);
        ratioGroup->setExclusive(false);

        static const QList<std::tuple<FilterState::AspectRatio, QString, int>> ratioItems = {
            {FilterState::Horizontal, "横图", m_currentStats.ratioHorizontalCount},
            {FilterState::Vertical, "竖图", m_currentStats.ratioVerticalCount},
            {FilterState::Square, "方形", m_currentStats.ratioSquareCount},
            {FilterState::Ratio169, "16:9", m_currentStats.ratio169Count}
        };
        for (const auto& [ratio, label, count] : ratioItems) {
            QCheckBox* cb = addFilterRow(gl, label, count);
            if (m_filter.ratio == ratio) cb->setChecked(true);
            connect(cb, &QCheckBox::toggled, this, [this, ratio, ratioGroup, cb](bool on) {
                if (on) {
                    for (QAbstractButton* b : ratioGroup->buttons()) if (b != cb && b->isChecked()) b->setChecked(false);
                    m_filter.ratio = ratio;
                } else m_filter.ratio = FilterState::AspectAny;
                emit filterChanged(m_filter);
            });
            ratioGroup->addButton(cb);
        }
        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }

    // ── 12. 重复状态 (最底部独立主选项) ───────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("重复状态", gl);
        m_groupDuplicate = g;

        QButtonGroup* dupGroup = new QButtonGroup(g);
        dupGroup->setExclusive(false);

        static const QList<std::tuple<FilterState::DuplicatePresence, QString, int>> dupItems = {
            {FilterState::DuplicateOnly, "重复项", m_currentStats.duplicateCount},
            {FilterState::UniqueOnly, "未重复", m_currentStats.uniqueCount}
        };
        for (const auto& [presence, label, count] : dupItems) {
            QCheckBox* cb = addFilterRow(gl, label, count);
            if (m_filter.duplicatePresence == presence) cb->setChecked(true);
            connect(cb, &QCheckBox::toggled, this, [this, presence, dupGroup, cb](bool on) {
                if (on) {
                    for (QAbstractButton* b : dupGroup->buttons()) if (b != cb && b->isChecked()) b->setChecked(false);
                    m_filter.duplicatePresence = presence;
                } else m_filter.duplicatePresence = FilterState::DupAll;
                emit filterChanged(m_filter);
            });
            dupGroup->addButton(cb);
        }
        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
}

// ─── buildGroup ───────────────────────────────────────────────────
QWidget* FilterPanel::buildGroup(const QString& title, QVBoxLayout*& outContentLayout,
                                  QHBoxLayout** outHdrLayout) {
    QWidget* wrapper = new QWidget(m_container);
    wrapper->setAttribute(Qt::WA_StyledBackground, true);
    wrapper->setStyleSheet("background: transparent;");
    QVBoxLayout* wl = new QVBoxLayout(wrapper);
    wl->setContentsMargins(0, 0, 0, 0);
    wl->setSpacing(0);

    QWidget* hdrRow = new QWidget(wrapper);
    hdrRow->setObjectName("GroupHdrRow");
    hdrRow->setFixedHeight(24);
    hdrRow->setAttribute(Qt::WA_StyledBackground, true);
    hdrRow->setStyleSheet(
        "QWidget#GroupHdrRow {"
        "  background: #252526;"
        "  border-top: 1px solid #333333;"
        "}");

    QHBoxLayout* hdrRowLayout = new QHBoxLayout(hdrRow);
    hdrRowLayout->setContentsMargins(0, 0, 0, 0);
    hdrRowLayout->setSpacing(0);

    QPushButton* hdr = new QPushButton(title, hdrRow);
    hdr->setCheckable(true);

    bool isCollapsed = AppConfig::instance().getValue(QString("FilterPanel/Collapsed_%1").arg(title), false).toBool();
    hdr->setChecked(!isCollapsed);

    hdr->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    hdr->setFixedHeight(24);
    hdr->setStyleSheet(
        "QPushButton {"
        "  background: transparent;"
        "  border: none;"
        "  color: #AAAAAA;"
        "  font-size: 11px;"
        "  font-weight: 600;"
        "  text-align: left;"
        "  padding-left: 5px;"
        "  padding-right: 5px;"
        "  padding-top: 0px;"
        "  padding-bottom: 0px;"
        "  margin: 0px;"
        "}"
        "QPushButton:hover { color: #EEEEEE; }"
        "QPushButton:pressed { background: transparent; }");
    hdrRowLayout->addWidget(hdr);

    if (outHdrLayout) *outHdrLayout = hdrRowLayout;

    QWidget* content = new QWidget(wrapper);
    content->setAttribute(Qt::WA_StyledBackground, true);
    content->setStyleSheet("background: transparent;");
    outContentLayout = new QVBoxLayout(content);
    outContentLayout->setContentsMargins(0, 0, 0, 0);
    outContentLayout->setSpacing(0);
    content->setVisible(!isCollapsed);

    connect(hdr, &QPushButton::toggled, this, [title, content](bool checked) {
        content->setVisible(checked);
        AppConfig::instance().setValue(QString("FilterPanel/Collapsed_%1").arg(title), !checked);
    });

    m_groupHeaders.append(hdr);
    bool allCollapsed = AppConfig::instance().getValue("FilterPanel/AllGroupsCollapsed", false).toBool();
    if (allCollapsed) {
        hdr->setChecked(false);
    }

    wl->addWidget(hdrRow);
    wl->addWidget(content);
    return wrapper;
}

// ─── addFilterRow ─────────────────────────────────────────────────
QCheckBox* FilterPanel::addFilterRow(QVBoxLayout* layout, const QString& label, int count, const QColor& dotColor) {
    StyledCheckBox* cb = new StyledCheckBox();

    ClickableRow* row = new ClickableRow(cb);
    row->setFixedHeight(24);

    QHBoxLayout* rl = new QHBoxLayout(row);
    rl->setContentsMargins(5, 0, 5, 0);
    rl->setSpacing(5);
    rl->addWidget(cb);

    if (dotColor.isValid() && dotColor != Qt::transparent) {
        QLabel* dot = new QLabel(row);
        dot->setFixedSize(10, 10);
        dot->setStyleSheet(QString("background: %1; border-radius: 5px;").arg(dotColor.name()));
        rl->addWidget(dot);
    }

    QLabel* lbl = new QLabel(label, row);
    lbl->setStyleSheet("font-size: 12px; color: #CCCCCC; background: transparent;");
    rl->addWidget(lbl, 1);

    QLabel* cnt = new QLabel(QString::number(count), row);
    cnt->setStyleSheet("font-size: 11px; color: #555555; background: transparent;");
    cnt->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    rl->addWidget(cnt);

    layout->addWidget(row);
    return cb;
}

// ─── clearAllFilters ──────────────────────────────────────────────
void FilterPanel::clearAllFilters(bool force) {
    if (!force && m_isFilterPinned) {
        return;
    }

    if (force && m_isFilterPinned) {
        m_isFilterPinned = false;
        if (m_btnPin) {
            m_btnPin->setIcon(UiHelper::getIcon("pin_tilted", QColor("#B0B0B0")));
            m_btnPin->setProperty("tooltipText", "锁定当前筛选条件");
        }
    }

    m_filter = FilterState{};
    m_filter.duplicatePresence = FilterState::DupAll;

    if (m_editType) m_editType->clear();
    if (m_editCreateDate) m_editCreateDate->clear();
    if (m_editModifyDate) m_editModifyDate->clear();
    
    rebuildGroups();
    emit filterChanged(m_filter);
}

void FilterPanel::updateHeaderStatus() {
    if (!m_iconLabel || !m_titleLabel || !m_btnClearAll || !m_btnToggleGroups) return;
    
    bool active = !m_filter.isEmpty();
    
    QColor brandYellow = QColor("#f1c40f");
    m_iconLabel->setPixmap(UiHelper::getIcon("filter_funnel_outline", brandYellow, 18).pixmap(18, 18));
    m_titleLabel->setStyleSheet(QString("font-size: 13px; font-weight: bold; color: %1; background: transparent; border: none;").arg(brandYellow.name()));
    m_titleLabel->style()->unpolish(m_titleLabel);
    m_titleLabel->style()->polish(m_titleLabel);
    m_titleLabel->update();

    QColor btnColor = active ? brandYellow : QColor("#B0B0B0");
    m_btnClearAll->setIcon(UiHelper::getIcon("reset_filter", btnColor));

    bool allCollapsed = AppConfig::instance().getValue("FilterPanel/AllGroupsCollapsed", false).toBool();
    m_btnToggleGroups->setIcon(UiHelper::getIcon(allCollapsed ? "chevrons_up" : "chevrons_down", QColor("#B0B0B0"), 16));
    m_btnToggleGroups->setProperty("tooltipText", allCollapsed ? "展开所有分组" : "折叠所有分组");
}

void FilterPanel::onToggleAllGroupsClicked() {
    bool currentlyCollapsed = AppConfig::instance().getValue("FilterPanel/AllGroupsCollapsed", false).toBool();
    bool targetCollapsed = !currentlyCollapsed;

    for (QPushButton* hdr : m_groupHeaders) {
        if (hdr) hdr->setChecked(!targetCollapsed);
    }

    AppConfig::instance().setValue("FilterPanel/AllGroupsCollapsed", targetCollapsed);
    updateHeaderStatus();
}

void FilterPanel::setMirrorSource(bool isMirror) {
    Q_UNUSED(isMirror);
}

void FilterPanel::selectColor(const QColor& color) {
    QString hex = color.name().toUpper();
    
    m_filter.colors.clear();
    m_filter.colors.append(hex);

    rebuildGroups();
    emit filterChanged(m_filter);
}

} // namespace QuarkMeta