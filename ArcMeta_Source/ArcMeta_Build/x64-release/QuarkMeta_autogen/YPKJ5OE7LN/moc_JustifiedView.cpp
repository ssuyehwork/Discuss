/****************************************************************************
** Meta object code from reading C++ file 'JustifiedView.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../ArcMeta/src/ui/JustifiedView.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'JustifiedView.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN7ArcMeta13JustifiedViewE_t {};
} // unnamed namespace

template <> constexpr inline auto ArcMeta::JustifiedView::qt_create_metaobjectdata<qt_meta_tag_ZN7ArcMeta13JustifiedViewE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ArcMeta::JustifiedView",
        "dataChanged",
        "",
        "QModelIndex",
        "topLeft",
        "bottomRight",
        "QList<int>",
        "roles",
        "rowsInserted",
        "parent",
        "start",
        "end",
        "rowsAboutToBeRemoved",
        "onLayoutTimerTimeout"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'dataChanged'
        QtMocHelpers::SlotData<void(const QModelIndex &, const QModelIndex &, const QList<int> &)>(1, 2, QMC::AccessProtected, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { 0x80000000 | 3, 5 }, { 0x80000000 | 6, 7 },
        }}),
        // Slot 'dataChanged'
        QtMocHelpers::SlotData<void(const QModelIndex &, const QModelIndex &)>(1, 2, QMC::AccessProtected | QMC::MethodCloned, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { 0x80000000 | 3, 5 },
        }}),
        // Slot 'rowsInserted'
        QtMocHelpers::SlotData<void(const QModelIndex &, int, int)>(8, 2, QMC::AccessProtected, QMetaType::Void, {{
            { 0x80000000 | 3, 9 }, { QMetaType::Int, 10 }, { QMetaType::Int, 11 },
        }}),
        // Slot 'rowsAboutToBeRemoved'
        QtMocHelpers::SlotData<void(const QModelIndex &, int, int)>(12, 2, QMC::AccessProtected, QMetaType::Void, {{
            { 0x80000000 | 3, 9 }, { QMetaType::Int, 10 }, { QMetaType::Int, 11 },
        }}),
        // Slot 'onLayoutTimerTimeout'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessProtected, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<JustifiedView, qt_meta_tag_ZN7ArcMeta13JustifiedViewE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ArcMeta::JustifiedView::staticMetaObject = { {
    QMetaObject::SuperData::link<QAbstractItemView::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta13JustifiedViewE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta13JustifiedViewE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN7ArcMeta13JustifiedViewE_t>.metaTypes,
    nullptr
} };

void ArcMeta::JustifiedView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<JustifiedView *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->dataChanged((*reinterpret_cast<std::add_pointer_t<QModelIndex>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QModelIndex>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QList<int>>>(_a[3]))); break;
        case 1: _t->dataChanged((*reinterpret_cast<std::add_pointer_t<QModelIndex>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QModelIndex>>(_a[2]))); break;
        case 2: _t->rowsInserted((*reinterpret_cast<std::add_pointer_t<QModelIndex>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 3: _t->rowsAboutToBeRemoved((*reinterpret_cast<std::add_pointer_t<QModelIndex>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 4: _t->onLayoutTimerTimeout(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 2:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<int> >(); break;
            }
            break;
        }
    }
}

const QMetaObject *ArcMeta::JustifiedView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ArcMeta::JustifiedView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta13JustifiedViewE_t>.strings))
        return static_cast<void*>(this);
    return QAbstractItemView::qt_metacast(_clname);
}

int ArcMeta::JustifiedView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractItemView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}
QT_WARNING_POP
