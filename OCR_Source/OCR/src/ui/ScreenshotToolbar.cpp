#include "ScreenshotToolbar.h"
#include "ScreenshotTool.h"
#include "ToolTipOverlay.h"
#include "IconHelper.h"

#include <QPainter>
#include <QMenu>
#include <QWidgetAction>
#include <QColorDialog>
#include <QSettings>
#include <QComboBox>
#include <QAbstractItemView>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static QList<QColor> getStandardColorList() {
    return {
        QColor(255, 0, 0), QColor(255, 165, 0), QColor(255, 255, 0), QColor(0, 255, 0),
        QColor(0, 255, 255), QColor(0, 0, 255), QColor(128, 0, 128), QColor(0, 0, 0), QColor(255, 255, 255)
    };
}

QIcon IconFactory::createArrowStyleIcon(ArrowStyle style) {
        // 增大图标画布，以适应无文本的宽型菜单
        QPixmap pix(120, 32);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(Qt::white, 2.5, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
        p.setBrush(Qt::white);

        QPointF start(10, 16), end(110, 16);
        QPointF dir = end - start;
        double angle = std::atan2(dir.y(), dir.x());
        double len = 100.0;

        bool isOutline = (style == ArrowStyle::OutlineSingle || style == ArrowStyle::OutlineDouble || style == ArrowStyle::OutlineDot);

        if (style == ArrowStyle::SolidSingle || style == ArrowStyle::OutlineSingle) {
            double hLen = 22;
            double bWid = 10;
            double wLen = 18;
            double wWid = 3;
            QPointF unit_dir = dir / len;
            QPointF perp_dir(-unit_dir.y(), unit_dir.x());

            if (isOutline) {
                p.setPen(QPen(Qt::white, 2.0));
                p.setBrush(Qt::transparent);
            } else {
                p.setPen(Qt::NoPen);
                p.setBrush(Qt::white);
            }
            p.drawPolygon(QPolygonF() << end
                << end - unit_dir * hLen + perp_dir * bWid
                << end - unit_dir * wLen + perp_dir * wWid
                << start
                << end - unit_dir * wLen - perp_dir * wWid
                << end - unit_dir * hLen - perp_dir * bWid);
        } else if (style == ArrowStyle::Thin) {
            p.setPen(QPen(Qt::white, 3.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.drawLine(start, end);
            double headSize = 18;
            p.drawLine(end, end - QPointF(headSize * std::cos(angle - 0.5), headSize * std::sin(angle - 0.5)));
            p.drawLine(end, end - QPointF(headSize * std::cos(angle + 0.5), headSize * std::sin(angle + 0.5)));
        } else if (style == ArrowStyle::SolidDouble || style == ArrowStyle::OutlineDouble) {
            p.setPen(QPen(Qt::white, 2.0));
            if (isOutline) p.setBrush(Qt::transparent); else p.setBrush(Qt::white);
            auto drawH = [&](const QPointF& e, double ang) {
                QPointF du(std::cos(ang), std::sin(ang));
                QPointF dp(-du.y(), du.x());
                p.drawPolygon(QPolygonF() << e << e - du * 16 + dp * 8 << e - du * 13 + dp * 2 << e - du * 13 - dp * 2 << e - du * 16 - dp * 8);
            };
            p.drawLine(start + (dir/len)*12, end - (dir/len)*12);
            drawH(end, angle); drawH(start, angle + M_PI);
        } else if (style == ArrowStyle::SolidDot || style == ArrowStyle::OutlineDot) {
            p.setPen(QPen(Qt::white, 2.0));
            if (isOutline) p.setBrush(Qt::transparent); else p.setBrush(Qt::white);
            p.drawLine(start, end - (dir/len)*12);
            p.drawEllipse(start, 5, 5);
            QPointF du(std::cos(angle), std::sin(angle));
            QPointF dp(-du.y(), du.x());
            p.drawPolygon(QPolygonF() << end << end - du * 16 + dp * 8 << end - du * 13 + dp * 2 << end - du * 13 - dp * 2 << end - du * 16 - dp * 8);
        } else if (style == ArrowStyle::Dimension) {
            p.setPen(QPen(Qt::white, 2.5, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
            p.drawLine(start, end);
            QPointF du(std::cos(angle), std::sin(angle));
            QPointF dp(-du.y(), du.x());
            p.drawLine(start + dp * 8, start - dp * 8);
            p.drawLine(end + dp * 8, end - dp * 8);
        }
        return QIcon(pix);
    }


SelectionInfoBar::SelectionInfoBar(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFixedSize(180, 28);
    hide();
}
void SelectionInfoBar::updateInfo(const QRect& rect) {
    m_text = QString("%1, %2 | %3 x %4").arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height());
    update();
}
void SelectionInfoBar::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor(0, 0, 0, 200)); p.setPen(Qt::NoPen);
    p.drawRoundedRect(rect(), 4, 4);
    p.setPen(Qt::white); p.setFont(QFont("Arial", 9));
    p.drawText(rect(), Qt::AlignCenter, m_text);
}


ScreenshotToolbar::ScreenshotToolbar(ScreenshotTool* tool)
    : QWidget(nullptr, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
{
    m_tool = tool;
    setObjectName("ScreenshotToolbar");
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_AlwaysShowToolTips);
    setMouseTracking(true);

        setAttribute(Qt::WA_StyledBackground);

    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(6, 6, 6, 6);
    outerLayout->setSpacing(0);
    outerLayout->setSizeConstraint(QLayout::SetFixedSize);

    QFrame* mainContainer = new QFrame(this);
    mainContainer->setObjectName("MainContainer");
    mainContainer->setAttribute(Qt::WA_StyledBackground);
    outerLayout->addWidget(mainContainer);

    auto* mainLayout = new QVBoxLayout(mainContainer);
    mainLayout->setContentsMargins(4, 2, 4, 2); mainLayout->setSpacing(0);
    // [CRITICAL] 设置尺寸约束为 SetFixedSize，确保工具栏在子部件隐藏时能自动收缩高度，防止出现多余背景 / Use SetFixedSize to ensure toolbar shrinks when options are hidden
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    QWidget* toolRow = new QWidget(mainContainer);
    toolRow->setObjectName("ToolRow");
    auto* layout = new QHBoxLayout(toolRow);
    layout->setContentsMargins(6, 4, 6, 4); layout->setSpacing(2);

    // 2026-04-xx 按照宪法 [第五定律]：统一修正 ToolTip 格式为“半角空格引导+全角括号”
    addToolButton(layout, ScreenshotToolType::Rect, "screenshot_rect", "矩形 （R）");
    addToolButton(layout, ScreenshotToolType::Ellipse, "screenshot_ellipse", "椭圆 （E）");
    addToolButton(layout, ScreenshotToolType::Arrow, "screenshot_arrow", "箭头 （A）");
    addToolButton(layout, ScreenshotToolType::Line, "screenshot_line", "直线 （L）");

    addToolButton(layout, ScreenshotToolType::Pen, "screenshot_pen", "画笔 （P）");
    addToolButton(layout, ScreenshotToolType::Marker, "screenshot_marker", "记号笔 （M）");
    addToolButton(layout, ScreenshotToolType::Text, "screenshot_text", "文字 （T）");
    addToolButton(layout, ScreenshotToolType::Mosaic, "screenshot_mosaic", "画笔马赛克 （Z）");
    addToolButton(layout, ScreenshotToolType::MosaicRect, "screenshot_rect", "矩形马赛克 （M）");
    addToolButton(layout, ScreenshotToolType::Eraser, "screenshot_eraser", "橡皮擦 （X）");
    addToolButton(layout, ScreenshotToolType::Picker, "screen_picker", "取色器 （C）");

    layout->addSpacing(8);

    QFrame* divider = new QFrame();
    divider->setFrameShape(QFrame::VLine);
    divider->setFrameShadow(QFrame::Plain);
    divider->setObjectName("ScreenshotTool_divider_1");
    divider->setFixedWidth(1);
    divider->setFixedHeight(20);
    layout->addWidget(divider);

    layout->addSpacing(8);

    addActionButton(layout, "undo", "撤销 （Ctrl + Z）", [tool]{ tool->undo(); });
    addActionButton(layout, "redo", "重做 （Ctrl + Shift + Z）", [tool]{ tool->redo(); });
    addActionButton(layout, "screenshot_pin", "置顶截图 （F）", [tool]{ tool->pin(); });
    addActionButton(layout, "screenshot_ocr", "截图取文 （O）", [tool]{ tool->executeOCR(); });
    addActionButton(layout, "screenshot_save", "保存", [tool]{ tool->save(); });
    addActionButton(layout, "screenshot_copy", "复制 （Ctrl + C）", [tool]{ tool->copyToClipboard(); });
    addActionButton(layout, "screenshot_close", "取消 （Esc）", [tool]{ tool->cancel(); });
    addActionButton(layout, "screenshot_confirm", "完成 （Enter）", [tool]{ tool->confirm(); });

    mainLayout->addWidget(toolRow);

    // [CRITICAL] 水平分割线，用于分隔工具行和选项行 / Horizontal divider between tool row and option row
    m_horizontalDivider = new QFrame();
    m_horizontalDivider->setFrameShape(QFrame::HLine);
    m_horizontalDivider->setFrameShadow(QFrame::Plain);
    m_horizontalDivider->setObjectName("ScreenshotTool_m_horizontalDivider_2");
    m_horizontalDivider->setFixedHeight(1);
    m_horizontalDivider->setVisible(false);
    mainLayout->addWidget(m_horizontalDivider);

    createOptionWidget();
    mainLayout->addWidget(m_optionWidget);
}

void ScreenshotToolbar::addToolButton(QBoxLayout* layout, ScreenshotToolType type, const QString& iconName, const QString& tip) {
    auto* btn = new QPushButton();
    btn->setAttribute(Qt::WA_StyledBackground);
    btn->setIcon(IconHelper::getIcon(iconName)); btn->setIconSize(QSize(20, 20));
    // btn->setToolTip(tip);
    btn->setProperty("tooltipText", tip);
    btn->installEventFilter(this);
    btn->setCheckable(true); btn->setFixedSize(32, 32);
    layout->addWidget(btn); m_buttons[type] = btn;
    connect(btn, &QPushButton::clicked, [this, type]{ selectTool(type); });
}

void ScreenshotToolbar::addActionButton(QBoxLayout* layout, const QString& iconName, const QString& tip, std::function<void()> func) {
    auto* btn = new QPushButton();
    btn->setAttribute(Qt::WA_StyledBackground);
    btn->setIcon(IconHelper::getIcon(iconName)); btn->setIconSize(QSize(20, 20));
    // btn->setToolTip(tip);
    btn->setProperty("tooltipText", tip);
    btn->installEventFilter(this);
    btn->setFixedSize(32, 32);
    layout->addWidget(btn); connect(btn, &QPushButton::clicked, func);
}

void ScreenshotToolbar::createOptionWidget() {
    m_optionWidget = new QWidget; m_optionWidget->setObjectName("OptionWidget");
    m_optionWidget->setAttribute(Qt::WA_TranslucentBackground);
    auto* layout = new QHBoxLayout(m_optionWidget); layout->setContentsMargins(8, 4, 8, 4); layout->setSpacing(4);

    // 1. 箭头样式按钮 (迁移至最左侧)
    m_arrowStyleBtn = new QPushButton(); m_arrowStyleBtn->setFixedSize(56, 24);
    updateArrowButtonIcon(m_tool->m_currentArrowStyle);
    // 2026-04-xx 按照宪法规范修正格式
    m_arrowStyleBtn->setProperty("tooltipText", "箭头样式 （W）");
    m_arrowStyleBtn->installEventFilter(this);
    connect(m_arrowStyleBtn, &QPushButton::clicked, this, &ScreenshotToolbar::showArrowMenu);
    layout->addWidget(m_arrowStyleBtn);

    // 2. 形状填充选项 (Rect/Ellipse)
    m_outlineBtn = new QPushButton();
    m_outlineBtn->setCheckable(true);
    m_outlineBtn->setFixedSize(24, 24);
    m_outlineBtn->setIcon(IconHelper::getIcon("screenshot_rect", "#ffffff"));
    // 2026-04-xx 按照宪法规范修正格式
    m_outlineBtn->setProperty("tooltipText", "虚心 （Hollow）");
    m_outlineBtn->installEventFilter(this);
    m_outlineBtn->setObjectName("ScreenshotTool_m_outlineBtn_3"); // 2026-03-xx 统一选中色

    m_solidBtn = new QPushButton();
    m_solidBtn->setCheckable(true);
    m_solidBtn->setFixedSize(24, 24);
    m_solidBtn->setIcon(IconHelper::getIcon("screenshot_fill", "#ffffff"));
    // 2026-04-xx 按照宪法规范修正格式
    m_solidBtn->setProperty("tooltipText", "实心 （Solid）");
    m_solidBtn->installEventFilter(this);
    m_solidBtn->setObjectName("ScreenshotTool_m_solidBtn_4"); // 2026-03-xx 统一选中色

    auto* fillGroup = new QButtonGroup(this);
    fillGroup->addButton(m_outlineBtn);
    fillGroup->addButton(m_solidBtn);
    layout->addWidget(m_outlineBtn);
    layout->addWidget(m_solidBtn);
    if (m_tool->m_fillEnabled) m_solidBtn->setChecked(true); else m_outlineBtn->setChecked(true);
    connect(m_outlineBtn, &QPushButton::clicked, [this]{ m_tool->setFillEnabled(false); });
    connect(m_solidBtn, &QPushButton::clicked, [this]{ m_tool->setFillEnabled(true); });

    // 3. 文字选项 (Text) - 采用独立胶囊布局 (Independent capsule layout)
    m_textOptionWidget = new QWidget(m_optionWidget);
    m_textOptionWidget->setAttribute(Qt::WA_TranslucentBackground);
    m_textOptionWidget->setObjectName("ScreenshotTool_m_textOptionWidget_5");
    auto* textOptionLayout = new QHBoxLayout(m_textOptionWidget);
    textOptionLayout->setContentsMargins(0, 0, 0, 0); textOptionLayout->setSpacing(4);

    auto createCapsule = [this](QWidget* content, int width = -1) {
        QWidget* capsule = new QWidget(m_textOptionWidget);
        capsule->setFixedHeight(28);
        if (width > 0) capsule->setFixedWidth(width);
        capsule->setAttribute(Qt::WA_StyledBackground);
        capsule->setObjectName("ScreenshotTool_capsule_6");
        auto* l = new QHBoxLayout(capsule);
        l->setContentsMargins(4, 0, 4, 0);
        l->setSpacing(0);
        l->addWidget(content);
        return capsule;
    };

    m_boldBtn = new QPushButton(); m_boldBtn->setCheckable(true); m_boldBtn->setFixedSize(24, 24);
    m_boldBtn->setIcon(IconHelper::getIcon("bold", "#ffffff"));
    // 2026-04-xx 按照宪法规范修正格式
    m_boldBtn->setProperty("tooltipText", "加粗 （Bold）");
    m_boldBtn->installEventFilter(this);
    m_boldBtn->setObjectName("ScreenshotTool_m_boldBtn_7"); // 2026-03-xx 统一悬停/选中色
    m_boldBtn->setChecked(m_tool->m_currentBold);
    connect(m_boldBtn, &QPushButton::toggled, [this](bool checked){ m_tool->setBold(checked); });
    textOptionLayout->addWidget(createCapsule(m_boldBtn, 32));

    m_italicBtn = new QPushButton(); m_italicBtn->setCheckable(true); m_italicBtn->setFixedSize(24, 24);
    m_italicBtn->setIcon(IconHelper::getIcon("italic", "#ffffff"));
    // 2026-04-xx 按照宪法规范修正格式
    m_italicBtn->setProperty("tooltipText", "倾斜 （Italic）");
    m_italicBtn->installEventFilter(this);
    m_italicBtn->setObjectName("ScreenshotTool_m_italicBtn_8"); // 2026-03-xx 统一悬停/选中色
    m_italicBtn->setChecked(m_tool->m_currentItalic);
    connect(m_italicBtn, &QPushButton::toggled, [this](bool checked){ m_tool->setItalic(checked); });
    textOptionLayout->addWidget(createCapsule(m_italicBtn, 32));

    auto* fontCombo = new QComboBox(); fontCombo->addItems({"微软雅黑", "宋体", "黑体", "楷体", "Arial", "Consolas"});
    fontCombo->setCurrentText(m_tool->m_currentFontFamily); fontCombo->setFixedSize(100, 28);
    fontCombo->setAttribute(Qt::WA_StyledBackground);
    fontCombo->setAttribute(Qt::WA_TranslucentBackground);
    fontCombo->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    fontCombo->setObjectName("ScreenshotTool_fontCombo_1");
    connect(fontCombo, &QComboBox::currentTextChanged, [this](const QString& font){ m_tool->setFontFamily(font); });
    fontCombo->view()->window()->setAttribute(Qt::WA_TranslucentBackground);
    fontCombo->view()->window()->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    fontCombo->view()->setFixedWidth(100);
    textOptionLayout->addWidget(fontCombo);

    auto* sizeCombo = new QComboBox();
    for(int s : {10, 12, 14, 16, 18, 20, 24, 28, 32, 36, 48}) sizeCombo->addItem(QString::number(s), s);
    sizeCombo->setCurrentIndex(sizeCombo->findData(m_tool->m_currentFontSize)); sizeCombo->setFixedSize(50, 28);
    sizeCombo->setAttribute(Qt::WA_StyledBackground);
    sizeCombo->setAttribute(Qt::WA_TranslucentBackground);
    sizeCombo->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sizeCombo->setObjectName("ScreenshotTool_sizeCombo_2");
    connect(sizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, sizeCombo](int index){ m_tool->setFontSize(sizeCombo->itemData(index).toInt()); });
    sizeCombo->view()->window()->setAttribute(Qt::WA_TranslucentBackground);
    sizeCombo->view()->window()->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    sizeCombo->view()->setFixedWidth(50);
    textOptionLayout->addWidget(sizeCombo);

    layout->addWidget(m_textOptionWidget);

    // [CRITICAL] 垂直分割线，用于分隔工具选项和粗细选项 / Vertical divider between tool options and size options
    m_textDivider = new QFrame();
    m_textDivider->setFrameShape(QFrame::VLine);
    m_textDivider->setFrameShadow(QFrame::Plain);
    m_textDivider->setObjectName("ScreenshotTool_m_textDivider_9");
    m_textDivider->setFixedWidth(1);
    m_textDivider->setFixedHeight(16);
    layout->addSpacing(4);
    layout->addWidget(m_textDivider);
    layout->addSpacing(4);

    // 4. 描边粗细 (Size Buttons)
    int sizes[] = {2, 4, 8};
    m_sizeGroup = new QButtonGroup(this);
    for(int s : sizes) {
        auto* btn = new QPushButton; btn->setProperty("sizeBtn", true); btn->setFixedSize(14 + s, 14 + s);
        btn->setCheckable(true); if(s == m_tool->m_currentStrokeWidth) btn->setChecked(true);
        layout->addWidget(btn); m_sizeGroup->addButton(btn);
        connect(btn, &QPushButton::clicked, [this, s]{ m_tool->setDrawWidth(s); });
    }

    // 分隔符
    QFrame* colorDivider = new QFrame();
    colorDivider->setFrameShape(QFrame::VLine);
    colorDivider->setFrameShadow(QFrame::Plain);
    colorDivider->setObjectName("ScreenshotTool_colorDivider_10");
    colorDivider->setFixedWidth(1);
    colorDivider->setFixedHeight(16);
    layout->addSpacing(4);
    layout->addWidget(colorDivider);
    layout->addSpacing(4);

    // 5. 标准颜色选择区域 (9色)
    m_colorGroup = new QButtonGroup(this);
    m_colorGroup->setExclusive(false); // 改为非独占，实现全覆盖高亮
    for(const auto& c : getStandardColorList()) {
        auto* btn = new QPushButton; btn->setProperty("colorBtn", true); btn->setProperty("colorValue", c);
        btn->setFixedSize(20, 20); btn->setCheckable(true);
        // [CRITICAL] 采用“等色边框”方案：非选中时边框颜色与背景一致，选中时变为白色。
        // 此变量极其重要：padding: 0px 和 border-color: %1 共同解决了 QSS 渲染时父容器背景色露底（黑色背景）的问题。
        // [CRITICAL] This variable is extremely important: padding: 0px and border-color: %1
        // solve the issue of parent container background bleeding through (black background issue).
        btn->setStyleSheet(QString("QPushButton { background-color: %1; border: 2px solid %1; border-radius: 2px; padding: 0px; } "
                                   "QPushButton:hover { background-color: #3e3e42; border-color: #3e3e42; } " // 2026-03-xx 统一悬停色
                                   "QPushButton:checked { background-color: %1; border-color: white; }").arg(c.name()));
        layout->addWidget(btn); m_colorGroup->addButton(btn);
        connect(btn, &QPushButton::clicked, [this, c]{ m_tool->setDrawColor(c); });
    }

    // 分隔符 (右侧)
    QFrame* colorDividerRight = new QFrame();
    colorDividerRight->setFrameShape(QFrame::VLine);
    colorDividerRight->setFrameShadow(QFrame::Plain);
    colorDividerRight->setObjectName("ScreenshotTool_colorDividerRight_11");
    colorDividerRight->setFixedWidth(1);
    colorDividerRight->setFixedHeight(16);
    layout->addSpacing(4);
    layout->addWidget(colorDividerRight);
    layout->addSpacing(4);

    // 6. 最近颜色展示区 (填满红色方框区域)
    auto* recentContainer = new QWidget(m_optionWidget);
    recentContainer->setObjectName("ScreenshotTool_recentContainer_12");
    m_recentLayout = new QHBoxLayout(recentContainer);
    m_recentLayout->setContentsMargins(0, 0, 0, 0); m_recentLayout->setSpacing(4);
    layout->addWidget(recentContainer);

    QSettings settings("OCR", "Screenshot");
    for (const QString& name : settings.value("recentColors").toStringList()) { addRecentColor(QColor(name), false); }

    // --- 弹性间距 ---
    layout->addStretch();

    // 7. 右侧控制按钮统一管理
    // 色轮按钮 (图标换成调色盘)
    m_wheelBtn = new QPushButton(); m_wheelBtn->setFixedSize(32, 32);
    // [CRITICAL] 调色盘图标内部已设为彩色。此处传入当前颜色仅为染其外轮廓，以指示当前选色。
    // [CRITICAL] The palette icon now has fixed internal colors. Tinting here only affects the outline to indicate selection.
    m_wheelBtn->setIcon(IconHelper::getIcon("palette", m_tool->m_currentColor.name()));
    m_wheelBtn->setIconSize(QSize(20, 20));
    // 2026-04-xx 按照宪法规范修正格式
    m_wheelBtn->setProperty("tooltipText", "自定义颜色 （C）");
    m_wheelBtn->installEventFilter(this);
    connect(m_wheelBtn, &QPushButton::clicked, [this]{
        QColorDialog dialog(m_tool->m_currentColor, m_tool);
        dialog.setWindowTitle("选择标注颜色"); dialog.setOptions(QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
        dialog.setWindowFlags(dialog.windowFlags() | Qt::WindowStaysOnTopHint);
        if (dialog.exec() == QDialog::Accepted) { QColor c = dialog.selectedColor(); m_tool->setDrawColor(c); addRecentColor(c); }
        m_tool->activateWindow(); m_tool->updateToolbarPosition();
    });
    layout->addWidget(m_wheelBtn);

    // 调色盘 (图标换成金色五角星)
    m_paletteBtn = new QPushButton(); m_paletteBtn->setFixedSize(32, 32);
    // 2026-05-03 按照用户要求：修正图标调用错误，将 Favorite_filled 改为 star_filled
    m_paletteBtn->setIcon(IconHelper::getIcon("star_filled", "#FFD700"));
    m_paletteBtn->setIconSize(QSize(20, 20));
    // 2026-04-xx 按照宪法规范修正格式
    m_paletteBtn->setProperty("tooltipText", "颜色收藏夹 （G）");
    m_paletteBtn->installEventFilter(this);
    m_paletteBtn->setCursor(Qt::PointingHandCursor);
    layout->addWidget(m_paletteBtn);

    QList<QColor> fullColors = {
        QColor(255, 0, 0), QColor(255, 69, 0), QColor(255, 165, 0), QColor(255, 215, 0),
        QColor(255, 255, 0), QColor(154, 205, 50), QColor(0, 255, 0), QColor(34, 139, 34),
        QColor(0, 255, 255), QColor(0, 191, 255), QColor(0, 120, 255), QColor(0, 0, 255),
        QColor(138, 43, 226), QColor(128, 0, 128), QColor(255, 0, 255), QColor(255, 192, 203),
        QColor(255, 255, 255), QColor(192, 192, 192), QColor(128, 128, 128), QColor(0, 0, 0)
    };
    connect(m_paletteBtn, &QPushButton::clicked, [this, fullColors]{
        QMenu* menu = new QMenu(this);
        IconHelper::setupMenu(menu);
        menu->setObjectName("ScreenshotTool_menu_13");
        QWidget* gridContainer = new QWidget; QGridLayout* grid = new QGridLayout(gridContainer);
        grid->setContentsMargins(4, 4, 4, 4); grid->setSpacing(4);
        int row = 0, col = 0;
        for (const auto& c : fullColors) {
            auto* btn = new QPushButton; btn->setFixedSize(22, 22);
            btn->setStyleSheet(QString("background-color: %1; border: 1px solid #555; border-radius: 2px;").arg(c.name()));
            grid->addWidget(btn, row, col);
            connect(btn, &QPushButton::clicked, [this, c, menu]{ m_tool->setDrawColor(c); menu->close(); });
            if (++col >= 5) { col = 0; row++; }
        }
        QWidgetAction* action = new QWidgetAction(menu); action->setDefaultWidget(gridContainer); menu->addAction(action);
        menu->exec(QCursor::pos()); delete menu;
    });

    // 移除按钮
    m_removeColorBtn = new QPushButton("×");
    m_removeColorBtn->setFixedSize(18, 24);
    // m_removeColorBtn->setToolTip("移除选中的最近颜色");
    m_removeColorBtn->setProperty("tooltipText", "移除选中的最近颜色");
    m_removeColorBtn->installEventFilter(this);
    m_removeColorBtn->setCursor(Qt::PointingHandCursor);
    m_removeColorBtn->setObjectName("ScreenshotTool_m_removeColorBtn_3");
    connect(m_removeColorBtn, &QPushButton::clicked, [this]{
        QColor current = m_tool->m_currentColor;
        for (int i = 0; i < m_recentLayout->count(); ++i) {
            auto* item = m_recentLayout->itemAt(i); auto* btn = qobject_cast<QPushButton*>(item->widget());
            if (btn && btn->property("colorValue").value<QColor>().rgba() == current.rgba()) {
                m_recentLayout->removeItem(item); btn->deleteLater(); delete item;
                QSettings settings("OCR", "Screenshot"); QStringList recent = settings.value("recentColors").toStringList();
                recent.removeAll(current.name()); settings.setValue("recentColors", recent);
                break;
            }
        }
    });
    layout->addWidget(m_removeColorBtn);

    m_optionWidget->setVisible(false);
    syncColorSelection(m_tool->m_currentColor);
}

void ScreenshotToolbar::showArrowMenu() {
    QMenu menu(this);
    IconHelper::setupMenu(&menu);
    // 使用 QWidgetAction 替代 QMenu::setIconSize 以获得更好的版本兼容性和视觉控制
    menu.setObjectName("ScreenshotTool_this_4");

    auto addAct = [&](ArrowStyle s) {
        QWidgetAction* action = new QWidgetAction(&menu);
        QPushButton* btn = new QPushButton();
        btn->setFixedSize(120, 36);
        btn->setIcon(IconFactory::createArrowStyleIcon(s));
        btn->setIconSize(QSize(100, 24));
        btn->setCursor(Qt::PointingHandCursor);
        btn->setObjectName("ScreenshotTool_btn_5");
        connect(btn, &QPushButton::clicked, [this, s, &menu]{
            m_tool->setArrowStyle(s);
            updateArrowButtonIcon(s);
            menu.close();
        });
        action->setDefaultWidget(btn);
        menu.addAction(action);
    };

    addAct(ArrowStyle::SolidSingle);
    addAct(ArrowStyle::SolidDouble);
    addAct(ArrowStyle::OutlineSingle);
    addAct(ArrowStyle::OutlineDouble);
    addAct(ArrowStyle::Dimension);
    addAct(ArrowStyle::Thin);
    addAct(ArrowStyle::SolidDot);
    addAct(ArrowStyle::OutlineDot);

    menu.exec(mapToGlobal(m_arrowStyleBtn->geometry().bottomLeft()));
}

void ScreenshotToolbar::addRecentColor(const QColor& c, bool save) {
    if (!m_recentLayout || !m_wheelBtn) return;

    // 1. 优先同步高亮状态 (Ensure highlight is synced even if not added to recent)
    syncColorSelection(c);

    // 2. 如果是 9 种标准色之一，则不重复添加到“最近颜色”
    for (const auto& sc : getStandardColorList()) {
        if (sc.rgba() == c.rgba()) return;
    }

    // 3. 检查是否已经在最近颜色列表中
    for (int i = 0; i < m_recentLayout->count(); ++i) {
        auto* b = qobject_cast<QPushButton*>(m_recentLayout->itemAt(i)->widget());
        if (b && b->property("colorValue").value<QColor>().rgba() == c.rgba()) return;
    }

    // 4. 限制最近颜色数量 (增加到 15 以填满区域)
    if (m_recentLayout->count() >= 15) {
        auto* item = m_recentLayout->takeAt(0);
        if (item->widget()) {
            if (m_colorGroup) m_colorGroup->removeButton(qobject_cast<QPushButton*>(item->widget()));
            item->widget()->deleteLater();
        }
        delete item;
    }

    // 5. 创建并添加新的最近颜色按钮
    auto* btn = new QPushButton; btn->setFixedSize(20, 20);
    btn->setProperty("colorBtn", true);
    btn->setProperty("colorValue", c);
    btn->setCheckable(true);
    // [CRITICAL] 采用与标准色一致的“等色边框”方案，这是解决颜色按钮黑色背景问题的核心逻辑。
    // [CRITICAL] This logic is the core fix for the color button's black background issue.
    btn->setStyleSheet(QString("QPushButton { background-color: %1; border: 2px solid %1; border-radius: 2px; padding: 0px; } "
                               "QPushButton:hover { background-color: #3e3e42; border-color: #3e3e42; } " // 2026-03-xx 统一悬停色
                               "QPushButton:checked { background-color: %1; border-color: white; }").arg(c.name()));
    // btn->setToolTip(c.name());
    btn->setProperty("tooltipText", c.name());
    btn->installEventFilter(this);
    m_recentLayout->addWidget(btn);
    if (m_colorGroup) m_colorGroup->addButton(btn);

    connect(btn, &QPushButton::clicked, [this, c]{
        m_tool->setDrawColor(c);
    });

    if (save) {
        QSettings settings("OCR", "Screenshot");
        QStringList recent = settings.value("recentColors").toStringList();
        if (!recent.contains(c.name())) {
            recent.append(c.name());
            if (recent.size() > 15) recent.removeFirst();
            settings.setValue("recentColors", recent);
        }
    }

    // 再次调用以确保新添加的按钮也被正确选中
    syncColorSelection(c);
}

void ScreenshotToolbar::syncColorSelection(const QColor& color) {
    if (!m_colorGroup) return;

    // 暂时阻塞信号，避免循环触发
    m_colorGroup->blockSignals(true);
    // m_colorGroup 包含标准色和最近色按钮，实现全覆盖同步 (Cover both standard and recent colors)
    for (auto* btn : m_colorGroup->buttons()) {
        QColor btnColor = btn->property("colorValue").value<QColor>();
        // 使用 rgba 进行精确对比，实现全覆盖高亮 (All coverage highlight)
        if (btnColor.rgba() == color.rgba()) {
            btn->setChecked(true);
        } else {
            btn->setChecked(false);
        }
    }

    // [CRITICAL] 同步更新调色盘图标的外轮廓颜色，而内部色点保持彩色。
    if (m_wheelBtn) {
        m_wheelBtn->setIcon(IconHelper::getIcon("palette", color.name()));
    }

    m_colorGroup->blockSignals(false);
}

void ScreenshotToolbar::updateArrowButtonIcon(ArrowStyle style) {
    m_arrowStyleBtn->setIcon(IconFactory::createArrowStyleIcon(style));
    m_arrowStyleBtn->setIconSize(QSize(48, 16));
}

void ScreenshotToolbar::selectTool(ScreenshotToolType type) {
    for(auto* b : std::as_const(m_buttons)) b->setChecked(false);
    if(m_buttons.contains(type)) m_buttons[type]->setChecked(true);

    // 恢复原来的两行逻辑：显示就展示选项卡
    bool hasOptions = (type != ScreenshotToolType::None);
    m_optionWidget->setVisible(hasOptions);
    if (m_horizontalDivider) m_horizontalDivider->setVisible(hasOptions);

    bool isArrow = (type == ScreenshotToolType::Arrow);
    bool isRectOrEllipse = (type == ScreenshotToolType::Rect || type == ScreenshotToolType::Ellipse);
    bool isText = (type == ScreenshotToolType::Text);

    m_arrowStyleBtn->setVisible(isArrow);
    m_outlineBtn->setVisible(isRectOrEllipse);
    m_solidBtn->setVisible(isRectOrEllipse);
    m_textOptionWidget->setVisible(isText);

    if (m_textDivider) {
        m_textDivider->setVisible(isArrow || isRectOrEllipse || isText);
    }

    m_tool->setTool(type);

    // [CRITICAL] 必须调用此系列方法以强制触发窗口尺寸重新计算，消除高度异常 / Force UI update to recalculate size
    m_optionWidget->updateGeometry();
    if (m_horizontalDivider) m_horizontalDivider->updateGeometry();
    if (layout()) layout()->activate();
    adjustSize();
    m_tool->updateToolbarPosition();
}

void ScreenshotToolbar::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true; m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
    }
}
void ScreenshotToolbar::mouseMoveEvent(QMouseEvent *event) {
    if (m_isDragging) move(event->globalPosition().toPoint() - m_dragPosition);
}
void ScreenshotToolbar::mouseReleaseEvent(QMouseEvent *) { m_isDragging = false; }
void ScreenshotToolbar::paintEvent(QPaintEvent *) {
    // 作为透明容器，不再执行默认绘制逻辑，完全由 MainContainer 处理背景
}

void ScreenshotToolbar::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        // [MODIFIED] 截图工具条按 Esc 不直接关闭，统一由 ScreenshotTool 控制
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

bool ScreenshotToolbar::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::HoverEnter) {
        QString text = watched->property("tooltipText").toString();
        if (!text.isEmpty()) {
            ToolTipOverlay::instance()->showText(QCursor::pos(), text);
        }
    } else if (event->type() == QEvent::HoverLeave) {
        ToolTipOverlay::hideTip();
    }
    return QWidget::eventFilter(watched, event);
}
