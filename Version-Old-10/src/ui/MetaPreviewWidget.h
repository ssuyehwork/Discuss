#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QPixmap>
#include <QVector>
#include <QPair>
#include <QColor>
#include "components/FlowLayout.h"
#include "components/ColorPill.h"

namespace QuarkMeta {

class MetaPreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit MetaPreviewWidget(QWidget* parent = nullptr);
    ~MetaPreviewWidget() override = default;

    void setImagePreview(const QPixmap& pixmap);
    void setPalettes(const QVector<QPair<QColor, float>>& palette);
    bool hasContent() const;

signals:
    void searchByColor(const QColor& color);

private:
    QVBoxLayout* m_layout = nullptr;
    QLabel* m_lblImagePreview = nullptr;
    FlowLayout* m_paletteFlowLayout = nullptr;
    QList<ColorPill*> m_colorPool;
};

} // namespace QuarkMeta
