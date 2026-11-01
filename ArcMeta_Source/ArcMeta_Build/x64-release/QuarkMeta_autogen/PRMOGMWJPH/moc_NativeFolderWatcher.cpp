/****************************************************************************
** Meta object code from reading C++ file 'NativeFolderWatcher.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../ArcMeta/src/core/NativeFolderWatcher.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'NativeFolderWatcher.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN7ArcMeta19NativeFolderWatcherE_t {};
} // unnamed namespace

template <> constexpr inline auto ArcMeta::NativeFolderWatcher::qt_create_metaobjectdata<qt_meta_tag_ZN7ArcMeta19NativeFolderWatcherE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ArcMeta::NativeFolderWatcher",
        "filesChanged",
        "",
        "QList<ArcMeta::FileWatcherEvent>",
        "events",
        "managedFolderRemoved",
        "std::wstring",
        "path",
        "processBatchQueue"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'filesChanged'
        QtMocHelpers::SignalData<void(const QList<ArcMeta::FileWatcherEvent> &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'managedFolderRemoved'
        QtMocHelpers::SignalData<void(const std::wstring &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 6, 7 },
        }}),
        // Slot 'processBatchQueue'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<NativeFolderWatcher, qt_meta_tag_ZN7ArcMeta19NativeFolderWatcherE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ArcMeta::NativeFolderWatcher::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta19NativeFolderWatcherE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta19NativeFolderWatcherE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN7ArcMeta19NativeFolderWatcherE_t>.metaTypes,
    nullptr
} };

void ArcMeta::NativeFolderWatcher::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<NativeFolderWatcher *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->filesChanged((*reinterpret_cast<std::add_pointer_t<QList<ArcMeta::FileWatcherEvent>>>(_a[1]))); break;
        case 1: _t->managedFolderRemoved((*reinterpret_cast<std::add_pointer_t<std::wstring>>(_a[1]))); break;
        case 2: _t->processBatchQueue(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<ArcMeta::FileWatcherEvent> >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (NativeFolderWatcher::*)(const QList<ArcMeta::FileWatcherEvent> & )>(_a, &NativeFolderWatcher::filesChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (NativeFolderWatcher::*)(const std::wstring & )>(_a, &NativeFolderWatcher::managedFolderRemoved, 1))
            return;
    }
}

const QMetaObject *ArcMeta::NativeFolderWatcher::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ArcMeta::NativeFolderWatcher::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta19NativeFolderWatcherE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ArcMeta::NativeFolderWatcher::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void ArcMeta::NativeFolderWatcher::filesChanged(const QList<ArcMeta::FileWatcherEvent> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void ArcMeta::NativeFolderWatcher::managedFolderRemoved(const std::wstring & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}
QT_WARNING_POP
