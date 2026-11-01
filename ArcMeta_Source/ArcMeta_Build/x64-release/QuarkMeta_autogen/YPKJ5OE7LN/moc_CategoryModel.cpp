/****************************************************************************
** Meta object code from reading C++ file 'CategoryModel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../ArcMeta/src/ui/CategoryModel.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CategoryModel.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN7ArcMeta13CategoryModelE_t {};
} // unnamed namespace

template <> constexpr inline auto ArcMeta::CategoryModel::qt_create_metaobjectdata<qt_meta_tag_ZN7ArcMeta13CategoryModelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ArcMeta::CategoryModel",
        "categoryRenameRequested",
        "",
        "catId",
        "newName",
        "categoryOrderChanged",
        "draggedId",
        "targetParentId",
        "insertRow",
        "refresh",
        "updateStatistics",
        "QMap<QString,int>",
        "sysCounts",
        "QMap<int,int>",
        "catCounts",
        "updateStatisticsWithSnapshot",
        "StatisticsSnapshot",
        "snapshot",
        "updateItemCounts",
        "updateSystemCounts",
        "data",
        "QVariant",
        "QModelIndex",
        "index",
        "role",
        "setData",
        "val",
        "mimeData",
        "QMimeData*",
        "QModelIndexList",
        "indexes",
        "supportedDropActions",
        "Qt::DropActions",
        "dropMimeData",
        "const QMimeData*",
        "Qt::DropAction",
        "action",
        "row",
        "column",
        "parent"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'categoryRenameRequested'
        QtMocHelpers::SignalData<void(int, const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::QString, 4 },
        }}),
        // Signal 'categoryOrderChanged'
        QtMocHelpers::SignalData<void(int, int, int)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 6 }, { QMetaType::Int, 7 }, { QMetaType::Int, 8 },
        }}),
        // Slot 'refresh'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'updateStatistics'
        QtMocHelpers::SlotData<void(const QMap<QString,int> &, const QMap<int,int> &)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 11, 12 }, { 0x80000000 | 13, 14 },
        }}),
        // Slot 'updateStatisticsWithSnapshot'
        QtMocHelpers::SlotData<void(const StatisticsSnapshot &)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 16, 17 },
        }}),
        // Slot 'updateItemCounts'
        QtMocHelpers::SlotData<void(const QMap<QString,int> &, const QMap<int,int> &)>(18, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 11, 12 }, { 0x80000000 | 13, 14 },
        }}),
        // Slot 'updateSystemCounts'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'data'
        QtMocHelpers::SlotData<QVariant(const QModelIndex &, int) const>(20, 2, QMC::AccessPublic, 0x80000000 | 21, {{
            { 0x80000000 | 22, 23 }, { QMetaType::Int, 24 },
        }}),
        // Slot 'data'
        QtMocHelpers::SlotData<QVariant(const QModelIndex &) const>(20, 2, QMC::AccessPublic | QMC::MethodCloned, 0x80000000 | 21, {{
            { 0x80000000 | 22, 23 },
        }}),
        // Slot 'setData'
        QtMocHelpers::SlotData<bool(const QModelIndex &, const QVariant &, int)>(25, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { 0x80000000 | 22, 23 }, { 0x80000000 | 21, 26 }, { QMetaType::Int, 24 },
        }}),
        // Slot 'setData'
        QtMocHelpers::SlotData<bool(const QModelIndex &, const QVariant &)>(25, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Bool, {{
            { 0x80000000 | 22, 23 }, { 0x80000000 | 21, 26 },
        }}),
        // Slot 'mimeData'
        QtMocHelpers::SlotData<QMimeData *(const QModelIndexList &) const>(27, 2, QMC::AccessPublic, 0x80000000 | 28, {{
            { 0x80000000 | 29, 30 },
        }}),
        // Slot 'supportedDropActions'
        QtMocHelpers::SlotData<Qt::DropActions() const>(31, 2, QMC::AccessPublic, 0x80000000 | 32),
        // Slot 'dropMimeData'
        QtMocHelpers::SlotData<bool(const QMimeData *, Qt::DropAction, int, int, const QModelIndex &)>(33, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { 0x80000000 | 34, 27 }, { 0x80000000 | 35, 36 }, { QMetaType::Int, 37 }, { QMetaType::Int, 38 },
            { 0x80000000 | 22, 39 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<CategoryModel, qt_meta_tag_ZN7ArcMeta13CategoryModelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ArcMeta::CategoryModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QStandardItemModel::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta13CategoryModelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta13CategoryModelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN7ArcMeta13CategoryModelE_t>.metaTypes,
    nullptr
} };

void ArcMeta::CategoryModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<CategoryModel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->categoryRenameRequested((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 1: _t->categoryOrderChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 2: _t->refresh(); break;
        case 3: _t->updateStatistics((*reinterpret_cast<std::add_pointer_t<QMap<QString,int>>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QMap<int,int>>>(_a[2]))); break;
        case 4: _t->updateStatisticsWithSnapshot((*reinterpret_cast<std::add_pointer_t<StatisticsSnapshot>>(_a[1]))); break;
        case 5: _t->updateItemCounts((*reinterpret_cast<std::add_pointer_t<QMap<QString,int>>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QMap<int,int>>>(_a[2]))); break;
        case 6: _t->updateSystemCounts(); break;
        case 7: { QVariant _r = _t->data((*reinterpret_cast<std::add_pointer_t<QModelIndex>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])));
            if (_a[0]) *reinterpret_cast<QVariant*>(_a[0]) = std::move(_r); }  break;
        case 8: { QVariant _r = _t->data((*reinterpret_cast<std::add_pointer_t<QModelIndex>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QVariant*>(_a[0]) = std::move(_r); }  break;
        case 9: { bool _r = _t->setData((*reinterpret_cast<std::add_pointer_t<QModelIndex>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QVariant>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 10: { bool _r = _t->setData((*reinterpret_cast<std::add_pointer_t<QModelIndex>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QVariant>>(_a[2])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 11: { QMimeData* _r = _t->mimeData((*reinterpret_cast<std::add_pointer_t<QModelIndexList>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QMimeData**>(_a[0]) = std::move(_r); }  break;
        case 12: { Qt::DropActions _r = _t->supportedDropActions();
            if (_a[0]) *reinterpret_cast<Qt::DropActions*>(_a[0]) = std::move(_r); }  break;
        case 13: { bool _r = _t->dropMimeData((*reinterpret_cast<std::add_pointer_t<const QMimeData*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<Qt::DropAction>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[4])),(*reinterpret_cast<std::add_pointer_t<QModelIndex>>(_a[5])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 11:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QModelIndexList >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (CategoryModel::*)(int , const QString & )>(_a, &CategoryModel::categoryRenameRequested, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (CategoryModel::*)(int , int , int )>(_a, &CategoryModel::categoryOrderChanged, 1))
            return;
    }
}

const QMetaObject *ArcMeta::CategoryModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ArcMeta::CategoryModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta13CategoryModelE_t>.strings))
        return static_cast<void*>(this);
    return QStandardItemModel::qt_metacast(_clname);
}

int ArcMeta::CategoryModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QStandardItemModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void ArcMeta::CategoryModel::categoryRenameRequested(int _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void ArcMeta::CategoryModel::categoryOrderChanged(int _t1, int _t2, int _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2, _t3);
}
QT_WARNING_POP
