#pragma once
#include <QString>
#include <QWidget>
#include "../FramelessDialogBase.h"

namespace RapidNotes {

class FramelessMessageBox : public FramelessDialog {
    Q_OBJECT
public:
    explicit FramelessMessageBox(const QString& title, const QString& text, QWidget* parent = nullptr,
                                 bool okOnly = false, const QString& okText = "确定");

    signals:
    void confirmed();
    void cancelled();

protected:
    void showEvent(QShowEvent* event) override;

private:
    QPushButton* m_btnOk = nullptr;

public:
    static void information(QWidget* parent, const QString& title, const QString& text);
    static void warning(QWidget* parent, const QString& title, const QString& text);
    static bool question(QWidget* parent, const QString& title, const QString& text); 
    static void critical(QWidget* parent, const QString& title, const QString& text);
};

} // namespace RapidNotes

using FramelessMessageBox = RapidNotes::FramelessMessageBox;
