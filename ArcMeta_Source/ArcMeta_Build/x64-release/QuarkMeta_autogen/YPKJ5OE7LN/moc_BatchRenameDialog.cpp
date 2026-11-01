/****************************************************************************
** Meta object code from reading C++ file 'BatchRenameDialog.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../ArcMeta/src/ui/BatchRenameDialog.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'BatchRenameDialog.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN7ArcMeta17BatchRenameDialogE_t {};
} // unnamed namespace

template <> constexpr inline auto ArcMeta::BatchRenameDialog::qt_create_metaobjectdata<qt_meta_tag_ZN7ArcMeta17BatchRenameDialogE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ArcMeta::BatchRenameDialog",
        "onAddRow",
        "",
        "updatePreview",
        "onExecute",
        "onPreview",
        "onBrowseTarget",
        "onImportPreset",
        "onExportPreset",
        "onDeleteCurrentPreset",
        "scheduleAutoSave",
        "doAutoSave"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'onAddRow'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'updatePreview'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onExecute'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onPreview'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onBrowseTarget'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onImportPreset'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onExportPreset'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDeleteCurrentPreset'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'scheduleAutoSave'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'doAutoSave'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<BatchRenameDialog, qt_meta_tag_ZN7ArcMeta17BatchRenameDialogE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ArcMeta::BatchRenameDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<FramelessDialog::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta17BatchRenameDialogE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta17BatchRenameDialogE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN7ArcMeta17BatchRenameDialogE_t>.metaTypes,
    nullptr
} };

void ArcMeta::BatchRenameDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<BatchRenameDialog *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->onAddRow(); break;
        case 1: _t->updatePreview(); break;
        case 2: _t->onExecute(); break;
        case 3: _t->onPreview(); break;
        case 4: _t->onBrowseTarget(); break;
        case 5: _t->onImportPreset(); break;
        case 6: _t->onExportPreset(); break;
        case 7: _t->onDeleteCurrentPreset(); break;
        case 8: _t->scheduleAutoSave(); break;
        case 9: _t->doAutoSave(); break;
        default: ;
        }
    }
    (void)_a;
}

const QMetaObject *ArcMeta::BatchRenameDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ArcMeta::BatchRenameDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta17BatchRenameDialogE_t>.strings))
        return static_cast<void*>(this);
    return FramelessDialog::qt_metacast(_clname);
}

int ArcMeta::BatchRenameDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = FramelessDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    return _id;
}
QT_WARNING_POP
