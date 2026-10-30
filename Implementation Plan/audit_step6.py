import os, sys

h_mp = open('src/ui/MetaPanel.h', 'r', encoding='utf-8').read()
cpp_mp = open('src/ui/MetaPanel.cpp', 'r', encoding='utf-8').read()

s1 = """protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;"""

s2 = """    QSet<QString> m_pendingRemovedTags;
    QStringList m_tagRemovalTargetSnapshot;
    QTimer* m_tagRemoveDebounceTimer = nullptr;"""

s3 = """#include "MetaPanel.h"
#include "ColorPicker.h" """

s4 = """void MetaPanel::initUi() {
    QWidget* header = new QWidget(this);"""

s5 = """void MetaPanel::resizeEvent(QResizeEvent* event) {
    QFrame::resizeEvent(event);"""

s6 = """bool MetaPanel::eventFilter(QObject* watched, QEvent* event) {"""

print("s1 match:", h_mp.count(s1))
print("s2 match:", h_mp.count(s2))
print("s3 match:", cpp_mp.count('#include "ColorPicker.h"'))
print("s4 match:", cpp_mp.count(s4))
print("s5 match:", cpp_mp.count(s5))
print("s6 match:", cpp_mp.count(s6))
