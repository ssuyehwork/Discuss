#pragma once

#include <QWidget>
#include "QuickLookGraphicsView.h"
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QSlider>
#include <QHBoxLayout>
#include <QContextMenuEvent>
#include "QuickLookGraphicsView.h"

namespace QuarkMeta {

class QuickLookWindow : public QWidget {
    Q_OBJECT
public:
    static QuickLookWindow& instance();

    void previewFile(const QString& path);
    void preview(const QString& filePath);
    void closePreview();

signals:
    void ratingRequested(int rating);
    void colorRequested(const QString& color);
    void prevRequested();
    void nextRequested();
    void favoriteRequested(const QString& path);
    void deleteRequested(const QString& path);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    QuickLookWindow();
    ~QuickLookWindow() override;

    void setupUi();
    void renderImage(const QString& path);
    void renderText(const QString& path);
    void showContextMenu(const QPoint& globalPos);
    
    QString detectEncoding(const QByteArray& data);
    bool isBinary(const QByteArray& data);

    QuickLookGraphicsView* m_graphicsView = nullptr;
    QPlainTextEdit* m_textEdit = nullptr;
    QLabel* m_lblEmptyPrompt = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_infoLabel = nullptr;
    QWidget* m_container = nullptr;
    
    QString m_currentPath;
    bool m_ignoreDeactivate = false;
};

} // namespace QuarkMeta
