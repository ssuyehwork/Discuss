/****************************************************************************
** Meta object code from reading C++ file 'MetaPanel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../ArcMeta/src/ui/MetaPanel.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MetaPanel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN7ArcMeta9MetaPanelE_t {};
} // unnamed namespace

template <> constexpr inline auto ArcMeta::MetaPanel::qt_create_metaobjectdata<qt_meta_tag_ZN7ArcMeta9MetaPanelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ArcMeta::MetaPanel",
        "tagAddRequested",
        "",
        "paths",
        "tag",
        "tagRemoveRequested",
        "metadataChanged",
        "rating",
        "std::wstring",
        "color",
        "noteEdited",
        "newNote",
        "linkEdited",
        "newLink",
        "primaryColorChanged",
        "path",
        "QColor",
        "tagsChanged",
        "tags",
        "searchByColor",
        "renameRequested",
        "oldPath",
        "newPath",
        "unbindCategoryRequested",
        "categoryId",
        "bindCategoryRequested",
        "onTagAdded",
        "onTagDeleted",
        "text",
        "setAsPrimaryColor"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'tagAddRequested'
        QtMocHelpers::SignalData<void(const QStringList &, const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QStringList, 3 }, { QMetaType::QString, 4 },
        }}),
        // Signal 'tagRemoveRequested'
        QtMocHelpers::SignalData<void(const QStringList &, const QString &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QStringList, 3 }, { QMetaType::QString, 4 },
        }}),
        // Signal 'metadataChanged'
        QtMocHelpers::SignalData<void(int, const std::wstring &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 7 }, { 0x80000000 | 8, 9 },
        }}),
        // Signal 'noteEdited'
        QtMocHelpers::SignalData<void(const QStringList &, const QString &)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QStringList, 3 }, { QMetaType::QString, 11 },
        }}),
        // Signal 'linkEdited'
        QtMocHelpers::SignalData<void(const QStringList &, const QString &)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QStringList, 3 }, { QMetaType::QString, 13 },
        }}),
        // Signal 'primaryColorChanged'
        QtMocHelpers::SignalData<void(const QString &, const QColor &)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 15 }, { 0x80000000 | 16, 9 },
        }}),
        // Signal 'tagsChanged'
        QtMocHelpers::SignalData<void(const QStringList &, const QStringList &)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QStringList, 3 }, { QMetaType::QStringList, 18 },
        }}),
        // Signal 'searchByColor'
        QtMocHelpers::SignalData<void(const QColor &)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 16, 9 },
        }}),
        // Signal 'renameRequested'
        QtMocHelpers::SignalData<void(const QString &, const QString &)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 21 }, { QMetaType::QString, 22 },
        }}),
        // Signal 'unbindCategoryRequested'
        QtMocHelpers::SignalData<void(const QString &, int)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 15 }, { QMetaType::Int, 24 },
        }}),
        // Signal 'bindCategoryRequested'
        QtMocHelpers::SignalData<void(const QString &)>(25, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 15 },
        }}),
        // Slot 'onTagAdded'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onTagDeleted'
        QtMocHelpers::SlotData<void(const QString &)>(27, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 28 },
        }}),
        // Slot 'setAsPrimaryColor'
        QtMocHelpers::SlotData<void(const QColor &)>(29, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 16, 9 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MetaPanel, qt_meta_tag_ZN7ArcMeta9MetaPanelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ArcMeta::MetaPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QFrame::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta9MetaPanelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta9MetaPanelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN7ArcMeta9MetaPanelE_t>.metaTypes,
    nullptr
} };

void ArcMeta::MetaPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MetaPanel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->tagAddRequested((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 1: _t->tagRemoveRequested((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 2: _t->metadataChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<std::wstring>>(_a[2]))); break;
        case 3: _t->noteEdited((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 4: _t->linkEdited((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 5: _t->primaryColorChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QColor>>(_a[2]))); break;
        case 6: _t->tagsChanged((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[2]))); break;
        case 7: _t->searchByColor((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1]))); break;
        case 8: _t->renameRequested((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 9: _t->unbindCategoryRequested((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 10: _t->bindCategoryRequested((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 11: _t->onTagAdded(); break;
        case 12: _t->onTagDeleted((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 13: _t->setAsPrimaryColor((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (MetaPanel::*)(const QStringList & , const QString & )>(_a, &MetaPanel::tagAddRequested, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (MetaPanel::*)(const QStringList & , const QString & )>(_a, &MetaPanel::tagRemoveRequested, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (MetaPanel::*)(int , const std::wstring & )>(_a, &MetaPanel::metadataChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (MetaPanel::*)(const QStringList & , const QString & )>(_a, &MetaPanel::noteEdited, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (MetaPanel::*)(const QStringList & , const QString & )>(_a, &MetaPanel::linkEdited, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (MetaPanel::*)(const QString & , const QColor & )>(_a, &MetaPanel::primaryColorChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (MetaPanel::*)(const QStringList & , const QStringList & )>(_a, &MetaPanel::tagsChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (MetaPanel::*)(const QColor & )>(_a, &MetaPanel::searchByColor, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (MetaPanel::*)(const QString & , const QString & )>(_a, &MetaPanel::renameRequested, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (MetaPanel::*)(const QString & , int )>(_a, &MetaPanel::unbindCategoryRequested, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (MetaPanel::*)(const QString & )>(_a, &MetaPanel::bindCategoryRequested, 10))
            return;
    }
}

const QMetaObject *ArcMeta::MetaPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ArcMeta::MetaPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta9MetaPanelE_t>.strings))
        return static_cast<void*>(this);
    return QFrame::qt_metacast(_clname);
}

int ArcMeta::MetaPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QFrame::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void ArcMeta::MetaPanel::tagAddRequested(const QStringList & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void ArcMeta::MetaPanel::tagRemoveRequested(const QStringList & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void ArcMeta::MetaPanel::metadataChanged(int _t1, const std::wstring & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void ArcMeta::MetaPanel::noteEdited(const QStringList & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2);
}

// SIGNAL 4
void ArcMeta::MetaPanel::linkEdited(const QStringList & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}

// SIGNAL 5
void ArcMeta::MetaPanel::primaryColorChanged(const QString & _t1, const QColor & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1, _t2);
}

// SIGNAL 6
void ArcMeta::MetaPanel::tagsChanged(const QStringList & _t1, const QStringList & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1, _t2);
}

// SIGNAL 7
void ArcMeta::MetaPanel::searchByColor(const QColor & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}

// SIGNAL 8
void ArcMeta::MetaPanel::renameRequested(const QString & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1, _t2);
}

// SIGNAL 9
void ArcMeta::MetaPanel::unbindCategoryRequested(const QString & _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1, _t2);
}

// SIGNAL 10
void ArcMeta::MetaPanel::bindCategoryRequested(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 10, nullptr, _t1);
}
QT_WARNING_POP
