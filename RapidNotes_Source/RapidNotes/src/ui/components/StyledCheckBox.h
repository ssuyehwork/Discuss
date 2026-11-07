#pragma once
#include <QCheckBox>

namespace RapidNotes {

class StyledCheckBox : public QCheckBox {
    Q_OBJECT
public:
    explicit StyledCheckBox(QWidget* parent = nullptr);
protected:
    void paintEvent(QPaintEvent* event) override;
};

} // namespace RapidNotes
