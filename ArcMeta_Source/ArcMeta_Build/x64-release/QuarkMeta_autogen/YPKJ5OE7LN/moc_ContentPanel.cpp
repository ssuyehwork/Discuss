/****************************************************************************
** Meta object code from reading C++ file 'ContentPanel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../ArcMeta/src/ui/ContentPanel.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ContentPanel.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN7ArcMeta16FilterProxyModelE_t {};
} // unnamed namespace

template <> constexpr inline auto ArcMeta::FilterProxyModel::qt_create_metaobjectdata<qt_meta_tag_ZN7ArcMeta16FilterProxyModelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ArcMeta::FilterProxyModel"
    };

    QtMocHelpers::UintData qt_methods {
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<FilterProxyModel, qt_meta_tag_ZN7ArcMeta16FilterProxyModelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ArcMeta::FilterProxyModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QSortFilterProxyModel::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta16FilterProxyModelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta16FilterProxyModelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN7ArcMeta16FilterProxyModelE_t>.metaTypes,
    nullptr
} };

void ArcMeta::FilterProxyModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<FilterProxyModel *>(_o);
    (void)_t;
    (void)_c;
    (void)_id;
    (void)_a;
}

const QMetaObject *ArcMeta::FilterProxyModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ArcMeta::FilterProxyModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta16FilterProxyModelE_t>.strings))
        return static_cast<void*>(this);
    return QSortFilterProxyModel::qt_metacast(_clname);
}

int ArcMeta::FilterProxyModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QSortFilterProxyModel::qt_metacall(_c, _id, _a);
    return _id;
}
namespace {
struct qt_meta_tag_ZN7ArcMeta12ContentPanelE_t {};
} // unnamed namespace

template <> constexpr inline auto ArcMeta::ContentPanel::qt_create_metaobjectdata<qt_meta_tag_ZN7ArcMeta12ContentPanelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ArcMeta::ContentPanel",
        "zoomLevelChanged",
        "",
        "level",
        "viewModeChanged",
        "ViewMode",
        "mode",
        "requestQuickLook",
        "path",
        "selectionChanged",
        "paths",
        "directorySelected",
        "requestAddFavorite",
        "dataSourceChanged",
        "source",
        "directoryStatsReady",
        "ArcMeta::ScanStats",
        "stats",
        "requestCreateSubCategory",
        "parentCategoryId",
        "categoryClicked",
        "categoryId",
        "statusBarStatsUpdated",
        "fileCount",
        "folderCount",
        "totalCount",
        "setZoomLevel",
        "onSelectionChanged",
        "onCustomContextMenuRequested",
        "QPoint",
        "pos",
        "onDoubleClicked",
        "QModelIndex",
        "index",
        "onPathsDropped",
        "targetIndex",
        "loadDirectory",
        "recursive",
        "setPendingSelectName",
        "name",
        "edit",
        "refreshAll",
        "updateItemMetadata",
        "migrateModelCache",
        "oldPath",
        "newPath",
        "clearFolderCache",
        "folderPath",
        "search",
        "query",
        "applyFilters",
        "FilterState",
        "state",
        "createNewItem",
        "type",
        "previewFile",
        "loadPaths",
        "reqId",
        "appendPaths",
        "currentLoadRequestId",
        "loadCategory",
        "loadCategories",
        "QList<int>",
        "categoryIds",
        "getCurrentCategoryType",
        "setCurrentCategoryType"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'zoomLevelChanged'
        QtMocHelpers::SignalData<void(int)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Signal 'viewModeChanged'
        QtMocHelpers::SignalData<void(enum ViewMode)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 5, 6 },
        }}),
        // Signal 'requestQuickLook'
        QtMocHelpers::SignalData<void(const QString &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Signal 'selectionChanged'
        QtMocHelpers::SignalData<void(const QStringList &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QStringList, 10 },
        }}),
        // Signal 'directorySelected'
        QtMocHelpers::SignalData<void(const QString &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Signal 'requestAddFavorite'
        QtMocHelpers::SignalData<void(const QStringList &)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QStringList, 10 },
        }}),
        // Signal 'dataSourceChanged'
        QtMocHelpers::SignalData<void(const QString &)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 14 },
        }}),
        // Signal 'directoryStatsReady'
        QtMocHelpers::SignalData<void(const ArcMeta::ScanStats &)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 16, 17 },
        }}),
        // Signal 'requestCreateSubCategory'
        QtMocHelpers::SignalData<void(int)>(18, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 19 },
        }}),
        // Signal 'categoryClicked'
        QtMocHelpers::SignalData<void(int)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 21 },
        }}),
        // Signal 'statusBarStatsUpdated'
        QtMocHelpers::SignalData<void(int, int, int)>(22, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 23 }, { QMetaType::Int, 24 }, { QMetaType::Int, 25 },
        }}),
        // Slot 'setZoomLevel'
        QtMocHelpers::SlotData<void(int)>(26, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Slot 'onSelectionChanged'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onCustomContextMenuRequested'
        QtMocHelpers::SlotData<void(const QPoint &)>(28, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 29, 30 },
        }}),
        // Slot 'onDoubleClicked'
        QtMocHelpers::SlotData<void(const QModelIndex &)>(31, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 32, 33 },
        }}),
        // Slot 'onPathsDropped'
        QtMocHelpers::SlotData<void(const QStringList &, const QModelIndex &)>(34, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QStringList, 10 }, { 0x80000000 | 32, 35 },
        }}),
        // Slot 'loadDirectory'
        QtMocHelpers::SlotData<void(const QString &, bool)>(36, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 }, { QMetaType::Bool, 37 },
        }}),
        // Slot 'loadDirectory'
        QtMocHelpers::SlotData<void(const QString &)>(36, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Slot 'setPendingSelectName'
        QtMocHelpers::SlotData<void(const QString &, bool)>(38, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 39 }, { QMetaType::Bool, 40 },
        }}),
        // Slot 'setPendingSelectName'
        QtMocHelpers::SlotData<void(const QString &)>(38, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { QMetaType::QString, 39 },
        }}),
        // Slot 'refreshAll'
        QtMocHelpers::SlotData<void()>(41, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'updateItemMetadata'
        QtMocHelpers::SlotData<void(const QString &)>(42, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Slot 'migrateModelCache'
        QtMocHelpers::SlotData<void(const QString &, const QString &)>(43, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 44 }, { QMetaType::QString, 45 },
        }}),
        // Slot 'clearFolderCache'
        QtMocHelpers::SlotData<void(const QString &)>(46, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 47 },
        }}),
        // Slot 'search'
        QtMocHelpers::SlotData<void(const QString &)>(48, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 49 },
        }}),
        // Slot 'applyFilters'
        QtMocHelpers::SlotData<void(const FilterState &)>(50, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 51, 52 },
        }}),
        // Slot 'applyFilters'
        QtMocHelpers::SlotData<void()>(50, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'createNewItem'
        QtMocHelpers::SlotData<void(const QString &)>(53, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 54 },
        }}),
        // Slot 'previewFile'
        QtMocHelpers::SlotData<void(const QString &)>(55, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Slot 'loadPaths'
        QtMocHelpers::SlotData<void(const QStringList &, int)>(56, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QStringList, 10 }, { QMetaType::Int, 57 },
        }}),
        // Slot 'loadPaths'
        QtMocHelpers::SlotData<void(const QStringList &)>(56, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { QMetaType::QStringList, 10 },
        }}),
        // Slot 'appendPaths'
        QtMocHelpers::SlotData<void(const QStringList &, int)>(58, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QStringList, 10 }, { QMetaType::Int, 57 },
        }}),
        // Slot 'appendPaths'
        QtMocHelpers::SlotData<void(const QStringList &)>(58, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { QMetaType::QStringList, 10 },
        }}),
        // Slot 'currentLoadRequestId'
        QtMocHelpers::SlotData<int() const>(59, 2, QMC::AccessPublic, QMetaType::Int),
        // Slot 'loadCategory'
        QtMocHelpers::SlotData<void(int)>(60, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 21 },
        }}),
        // Slot 'loadCategories'
        QtMocHelpers::SlotData<void(const QList<int> &)>(61, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 62, 63 },
        }}),
        // Slot 'getCurrentCategoryType'
        QtMocHelpers::SlotData<QString() const>(64, 2, QMC::AccessPublic, QMetaType::QString),
        // Slot 'setCurrentCategoryType'
        QtMocHelpers::SlotData<void(const QString &)>(65, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 54 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ContentPanel, qt_meta_tag_ZN7ArcMeta12ContentPanelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ArcMeta::ContentPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QFrame::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta12ContentPanelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta12ContentPanelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN7ArcMeta12ContentPanelE_t>.metaTypes,
    nullptr
} };

void ArcMeta::ContentPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ContentPanel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->zoomLevelChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 1: _t->viewModeChanged((*reinterpret_cast<std::add_pointer_t<enum ViewMode>>(_a[1]))); break;
        case 2: _t->requestQuickLook((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->selectionChanged((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1]))); break;
        case 4: _t->directorySelected((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->requestAddFavorite((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1]))); break;
        case 6: _t->dataSourceChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->directoryStatsReady((*reinterpret_cast<std::add_pointer_t<ArcMeta::ScanStats>>(_a[1]))); break;
        case 8: _t->requestCreateSubCategory((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 9: _t->categoryClicked((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 10: _t->statusBarStatsUpdated((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 11: _t->setZoomLevel((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 12: _t->onSelectionChanged(); break;
        case 13: _t->onCustomContextMenuRequested((*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[1]))); break;
        case 14: _t->onDoubleClicked((*reinterpret_cast<std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 15: _t->onPathsDropped((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QModelIndex>>(_a[2]))); break;
        case 16: _t->loadDirectory((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 17: _t->loadDirectory((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 18: _t->setPendingSelectName((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 19: _t->setPendingSelectName((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 20: _t->refreshAll(); break;
        case 21: _t->updateItemMetadata((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 22: _t->migrateModelCache((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 23: _t->clearFolderCache((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 24: _t->search((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 25: _t->applyFilters((*reinterpret_cast<std::add_pointer_t<FilterState>>(_a[1]))); break;
        case 26: _t->applyFilters(); break;
        case 27: _t->createNewItem((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 28: _t->previewFile((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 29: _t->loadPaths((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 30: _t->loadPaths((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1]))); break;
        case 31: _t->appendPaths((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 32: _t->appendPaths((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1]))); break;
        case 33: { int _r = _t->currentLoadRequestId();
            if (_a[0]) *reinterpret_cast<int*>(_a[0]) = std::move(_r); }  break;
        case 34: _t->loadCategory((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 35: _t->loadCategories((*reinterpret_cast<std::add_pointer_t<QList<int>>>(_a[1]))); break;
        case 36: { QString _r = _t->getCurrentCategoryType();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 37: _t->setCurrentCategoryType((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 35:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<int> >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ContentPanel::*)(int )>(_a, &ContentPanel::zoomLevelChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ContentPanel::*)(ViewMode )>(_a, &ContentPanel::viewModeChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ContentPanel::*)(const QString & )>(_a, &ContentPanel::requestQuickLook, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (ContentPanel::*)(const QStringList & )>(_a, &ContentPanel::selectionChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (ContentPanel::*)(const QString & )>(_a, &ContentPanel::directorySelected, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (ContentPanel::*)(const QStringList & )>(_a, &ContentPanel::requestAddFavorite, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (ContentPanel::*)(const QString & )>(_a, &ContentPanel::dataSourceChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (ContentPanel::*)(const ArcMeta::ScanStats & )>(_a, &ContentPanel::directoryStatsReady, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (ContentPanel::*)(int )>(_a, &ContentPanel::requestCreateSubCategory, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (ContentPanel::*)(int )>(_a, &ContentPanel::categoryClicked, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (ContentPanel::*)(int , int , int )>(_a, &ContentPanel::statusBarStatsUpdated, 10))
            return;
    }
}

const QMetaObject *ArcMeta::ContentPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ArcMeta::ContentPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ArcMeta12ContentPanelE_t>.strings))
        return static_cast<void*>(this);
    return QFrame::qt_metacast(_clname);
}

int ArcMeta::ContentPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QFrame::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 38)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 38;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 38)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 38;
    }
    return _id;
}

// SIGNAL 0
void ArcMeta::ContentPanel::zoomLevelChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void ArcMeta::ContentPanel::viewModeChanged(ViewMode _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void ArcMeta::ContentPanel::requestQuickLook(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void ArcMeta::ContentPanel::selectionChanged(const QStringList & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void ArcMeta::ContentPanel::directorySelected(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void ArcMeta::ContentPanel::requestAddFavorite(const QStringList & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void ArcMeta::ContentPanel::dataSourceChanged(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void ArcMeta::ContentPanel::directoryStatsReady(const ArcMeta::ScanStats & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}

// SIGNAL 8
void ArcMeta::ContentPanel::requestCreateSubCategory(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1);
}

// SIGNAL 9
void ArcMeta::ContentPanel::categoryClicked(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1);
}

// SIGNAL 10
void ArcMeta::ContentPanel::statusBarStatsUpdated(int _t1, int _t2, int _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 10, nullptr, _t1, _t2, _t3);
}
QT_WARNING_POP
