#ifndef APPLOCKWIDGET_H
#define APPLOCKWIDGET_H

#include <QWidget>
#include <QString>

class QLineEdit;
class QPushButton;

/**
 * 应用锁屏页。
 * - 作为 QuickWindow 根层 QStackedWidget 的一页整体替换主界面，不再覆盖在主界面之上。
 * - 背景由自己绘制，不依赖 style.qss，样式表异常也不会透出下层内容。
 * - 不持有密码：回车时只发出 passwordSubmitted，由外部决定对错。
 */
class AppLockWidget : public QWidget {
    Q_OBJECT
public:
    explicit AppLockWidget(const QString& hint, QWidget* parent = nullptr);

    void focusInput();
    void rejectPassword();   // 密码错误：清空输入并抖动
    void playUnlockFade();   // 密码正确：淡出，结束后发出 unlockFinished

signals:
    void passwordSubmitted(const QString& password);
    void unlockFinished();

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QLineEdit* m_pwdEdit = nullptr;
    QPushButton* m_closeBtn = nullptr;
    bool m_unlocking = false;
};

#endif // APPLOCKWIDGET_H
