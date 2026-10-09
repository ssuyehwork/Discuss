#include "AutoColorGroup.h"
#include "components/InlineHueSlider.h"
#include "components/ColorBlock.h"
#include "components/FlowLayout.h"
#include "ToolTipOverlay.h"
#include "AppConfig.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QTimer>
#include <QEvent>
#include <QMouseEvent>
#include <QCursor>

namespace QuarkMeta {

namespace {

class SliderAreaToolTipFilter : public QObject {
public:
    explicit SliderAreaToolTipFilter(QSlider* slider, QObject* parent = nullptr)
        : QObject(parent), m_slider(slider) {}

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (!m_slider) return false;

        switch (event->type()) {
            case QEvent::MouseMove:
            case QEvent::MouseButtonPress: {
                QPoint globalPos = QCursor::pos();
                ToolTipOverlay::instance()->showText(globalPos, QString("%1%").arg(m_slider->value()), 0);
                break;
            }
            case QEvent::MouseButtonRelease:
            case QEvent::Leave: {
                ToolTipOverlay::hideTip();
                break;
            }
            default:
                break;
        }
        return false;
    }

private:
    QSlider* m_slider = nullptr;
};

} // namespace

void AutoColorGroup::pushRecent(const QString& hex) {
    if (hex.isEmpty()) return;
    QString hexUpper = hex.toUpper();
    if (!hexUpper.startsWith('#')) {
        hexUpper = "#" + hexUpper;
    }

    QStringList recent = AppConfig::instance().value("Filter/RecentColors").toStringList();
    recent.removeAll(hexUpper);
    recent.prepend(hexUpper);
    while (recent.size() > 50) {
        recent.removeLast();
    }
    AppConfig::instance().setValue("Filter/RecentColors", recent);
}

void AutoColorGroup::populate(QWidget* parentWidget,
                             QVBoxLayout* contentLayout,
                             FilterStateModel* filterModel,
                             std::function<void()> requestRebuild) {
    if (!contentLayout || !filterModel) return;

    QWidget* container = new QWidget(parentWidget ? parentWidget : contentLayout->parentWidget());
    container->setObjectName("AutoColorGroupContainer");
    QVBoxLayout* groupLayout = new QVBoxLayout(container);
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(4);

    const FilterState currentState = filterModel->state();

    // 1. 色相条 InlineHueSlider
    QWidget* hueContainer = new QWidget(container);
    QHBoxLayout* hueLayout = new QHBoxLayout(hueContainer);
    hueLayout->setContentsMargins(5, 0, 5, 0);
    hueLayout->setSpacing(0);

    InlineHueSlider* hueSlider = new InlineHueSlider(hueContainer);
    hueLayout->addWidget(hueSlider);
    groupLayout->addWidget(hueContainer);

    QObject::connect(hueSlider, &InlineHueSlider::sliderReleased, container, [filterModel, hueSlider, requestRebuild]() {
        int h = hueSlider->hue();
        QColor col;
        if (h == 1000) col = Qt::black;
        else if (h == 1001) col = QColor("#808080");
        else if (h == 1002) col = Qt::white;
        else col = QColor::fromHsv(h, 220, 220);

        QString hex = col.name().toUpper();
        pushRecent(hex);

        FilterState st = filterModel->state();
        st.colors = QStringList{ hex };
        filterModel->setState(st);

        if (requestRebuild) requestRebuild();
    });

    // 2. "准确度:" 行
    QWidget* accRow = new QWidget(container);
    QHBoxLayout* accLayout = new QHBoxLayout(accRow);
    accLayout->setContentsMargins(10, 4, 10, 4);
    accLayout->setSpacing(8);

    QLabel* accLabel = new QLabel("准确度:", accRow);
    accLabel->setObjectName("FilterColorSliderLabel");
    QSlider* accSlider = new QSlider(Qt::Horizontal, accRow);
    accSlider->setObjectName("FilterColorSlider");
    accSlider->setRange(0, 100);
    accSlider->setValue(currentState.colorTolerance);

    accLayout->addWidget(accLabel);
    accLayout->addWidget(accSlider, 1);
    groupLayout->addWidget(accRow);

    // 3. "占比:" 行
    QWidget* areaRow = new QWidget(container);
    QHBoxLayout* areaLayout = new QHBoxLayout(areaRow);
    areaLayout->setContentsMargins(10, 4, 10, 4);
    areaLayout->setSpacing(8);

    QLabel* areaLabel = new QLabel("占比:", areaRow);
    areaLabel->setObjectName("FilterColorSliderLabel");
    QSlider* areaSlider = new QSlider(Qt::Horizontal, areaRow);
    areaSlider->setObjectName("FilterColorSlider");
    areaSlider->setRange(0, 100);
    areaSlider->setValue(currentState.minColorArea);

    areaLayout->addWidget(areaLabel);
    areaLayout->addWidget(areaSlider, 1);
    groupLayout->addWidget(areaRow);

    // 防抖 100ms 定时器
    QTimer* debounceTimer = new QTimer(container);
    debounceTimer->setSingleShot(true);
    debounceTimer->setInterval(100);

    auto onSliderValueChanged = [debounceTimer]() {
        debounceTimer->start();
    };

    QObject::connect(accSlider, &QSlider::valueChanged, container, onSliderValueChanged);
    QObject::connect(areaSlider, &QSlider::valueChanged, container, onSliderValueChanged);

    QObject::connect(debounceTimer, &QTimer::timeout, container, [filterModel, accSlider, areaSlider]() {
        FilterState st = filterModel->state();
        st.colorTolerance = accSlider->value();
        st.minColorArea = areaSlider->value();
        filterModel->setState(st);
    });

    // 占比滑条事件过滤器：鼠标在上面移动、按下或拖动时显示 ToolTipOverlay 百分比，离开或释放隐去
    SliderAreaToolTipFilter* tipFilter = new SliderAreaToolTipFilter(areaSlider, container);
    areaSlider->installEventFilter(tipFilter);

    // 4. 小标题 "标准色系" + 12 个色块
    QLabel* lblStandard = new QLabel("标准色系", container);
    lblStandard->setObjectName("FilterColorSubTitle");
    groupLayout->addWidget(lblStandard);

    QWidget* stdColorWidget = new QWidget(container);
    FlowLayout* stdFlow = new FlowLayout(stdColorWidget, 0, 2, 2);

    static const QStringList standardHexes = {
        "#E24B4A", "#EF9F27", "#FECF0E", "#639922", "#1D9E75", "#378ADD",
        "#7F77DD", "#E91E63", "#000000", "#808080", "#FFFFFF", "#795548"
    };

    auto handleColorBlockClick = [filterModel, requestRebuild](const QString& hex) {
        QString hexUpper = hex.toUpper();
        FilterState st = filterModel->state();
        if (st.colors.contains(hexUpper)) {
            st.colors.removeAll(hexUpper);
        } else {
            st.colors = QStringList{ hexUpper };
            pushRecent(hexUpper);
        }
        filterModel->setState(st);
        if (requestRebuild) requestRebuild();
    };

    for (const QString& hex : standardHexes) {
        ColorBlock* cb = new ColorBlock(QColor(hex), stdColorWidget);
        cb->setCount(-1);
        cb->setChecked(currentState.colors.contains(hex.toUpper()));
        QObject::connect(cb, &ColorBlock::clicked, stdColorWidget, [handleColorBlockClick, hex](const QColor&) {
            handleColorBlockClick(hex);
        });
        stdFlow->addWidget(cb);
    }
    groupLayout->addWidget(stdColorWidget);

    // 5. 小标题 "最近筛选" + 最近使用色块
    QStringList recentColors = AppConfig::instance().value("Filter/RecentColors").toStringList();
    if (!recentColors.isEmpty()) {
        QLabel* lblRecent = new QLabel("最近筛选", container);
        lblRecent->setObjectName("FilterColorSubTitleRecent");
        groupLayout->addWidget(lblRecent);

        QWidget* recentColorWidget = new QWidget(container);
        FlowLayout* recentFlow = new FlowLayout(recentColorWidget, 0, 2, 2);

        for (const QString& hex : recentColors) {
            ColorBlock* cb = new ColorBlock(QColor(hex), recentColorWidget);
            cb->setCount(-1);
            cb->setChecked(currentState.colors.contains(hex.toUpper()));
            QObject::connect(cb, &ColorBlock::clicked, recentColorWidget, [handleColorBlockClick, hex](const QColor&) {
                handleColorBlockClick(hex);
            });
            recentFlow->addWidget(cb);
        }
        groupLayout->addWidget(recentColorWidget);
    }

    contentLayout->addWidget(container);
}

} // namespace QuarkMeta
