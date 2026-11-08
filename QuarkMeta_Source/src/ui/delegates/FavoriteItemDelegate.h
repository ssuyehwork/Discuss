#pragma once

#include <QStyledItemDelegate>
#include <QString>
#include <QPainter>
#include <QFont>
#include <QColor>
#include <QRect>

namespace QuarkMeta {

class FavoriteItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit FavoriteItemDelegate(QObject* parent = nullptr);
    ~FavoriteItemDelegate() override = default;

    void setFilterKeyword(const QString& kw) { m_filterKeyword = kw; }
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    void paintHighlightedText(QPainter* painter, const QRect& rect, const QString& text, const QString& keyword, const QFont& font, const QColor& normalColor, const QColor& matchColor) const;
    QString m_filterKeyword;
};

} // namespace QuarkMeta
