/****************************************************************************
** Meta object code from reading C++ file 'qjackctlPatchbay.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/qjackctlPatchbay.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qjackctlPatchbay.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_qjackctlSocketList_t {
    uint offsetsAndSizes[18];
    char stringdata0[19];
    char stringdata1[14];
    char stringdata2[1];
    char stringdata3[17];
    char stringdata4[15];
    char stringdata5[15];
    char stringdata6[20];
    char stringdata7[17];
    char stringdata8[19];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_qjackctlSocketList_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_qjackctlSocketList_t qt_meta_stringdata_qjackctlSocketList = {
    {
        QT_MOC_LITERAL(0, 18),  // "qjackctlSocketList"
        QT_MOC_LITERAL(19, 13),  // "addSocketItem"
        QT_MOC_LITERAL(33, 0),  // ""
        QT_MOC_LITERAL(34, 16),  // "removeSocketItem"
        QT_MOC_LITERAL(51, 14),  // "editSocketItem"
        QT_MOC_LITERAL(66, 14),  // "copySocketItem"
        QT_MOC_LITERAL(81, 19),  // "exclusiveSocketItem"
        QT_MOC_LITERAL(101, 16),  // "moveUpSocketItem"
        QT_MOC_LITERAL(118, 18)   // "moveDownSocketItem"
    },
    "qjackctlSocketList",
    "addSocketItem",
    "",
    "removeSocketItem",
    "editSocketItem",
    "copySocketItem",
    "exclusiveSocketItem",
    "moveUpSocketItem",
    "moveDownSocketItem"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_qjackctlSocketList[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   56,    2, 0x0a,    1 /* Public */,
       3,    0,   57,    2, 0x0a,    2 /* Public */,
       4,    0,   58,    2, 0x0a,    3 /* Public */,
       5,    0,   59,    2, 0x0a,    4 /* Public */,
       6,    0,   60,    2, 0x0a,    5 /* Public */,
       7,    0,   61,    2, 0x0a,    6 /* Public */,
       8,    0,   62,    2, 0x0a,    7 /* Public */,

 // slots: parameters
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Bool,

       0        // eod
};

Q_CONSTINIT const QMetaObject qjackctlSocketList::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_qjackctlSocketList.offsetsAndSizes,
    qt_meta_data_qjackctlSocketList,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_qjackctlSocketList_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<qjackctlSocketList, std::true_type>,
        // method 'addSocketItem'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'removeSocketItem'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'editSocketItem'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'copySocketItem'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'exclusiveSocketItem'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'moveUpSocketItem'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'moveDownSocketItem'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>
    >,
    nullptr
} };

void qjackctlSocketList::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<qjackctlSocketList *>(_o);
        (void)_t;
        switch (_id) {
        case 0: { bool _r = _t->addSocketItem();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 1: { bool _r = _t->removeSocketItem();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 2: { bool _r = _t->editSocketItem();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 3: { bool _r = _t->copySocketItem();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 4: { bool _r = _t->exclusiveSocketItem();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 5: { bool _r = _t->moveUpSocketItem();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 6: { bool _r = _t->moveDownSocketItem();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
}

const QMetaObject *qjackctlSocketList::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *qjackctlSocketList::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_qjackctlSocketList.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int qjackctlSocketList::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }
    return _id;
}
namespace {
struct qt_meta_stringdata_qjackctlSocketListView_t {
    uint offsetsAndSizes[6];
    char stringdata0[23];
    char stringdata1[12];
    char stringdata2[1];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_qjackctlSocketListView_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_qjackctlSocketListView_t qt_meta_stringdata_qjackctlSocketListView = {
    {
        QT_MOC_LITERAL(0, 22),  // "qjackctlSocketListView"
        QT_MOC_LITERAL(23, 11),  // "timeoutSlot"
        QT_MOC_LITERAL(35, 0)   // ""
    },
    "qjackctlSocketListView",
    "timeoutSlot",
    ""
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_qjackctlSocketListView[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   20,    2, 0x09,    1 /* Protected */,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject qjackctlSocketListView::staticMetaObject = { {
    QMetaObject::SuperData::link<QTreeWidget::staticMetaObject>(),
    qt_meta_stringdata_qjackctlSocketListView.offsetsAndSizes,
    qt_meta_data_qjackctlSocketListView,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_qjackctlSocketListView_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<qjackctlSocketListView, std::true_type>,
        // method 'timeoutSlot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void qjackctlSocketListView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<qjackctlSocketListView *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->timeoutSlot(); break;
        default: ;
        }
    }
    (void)_a;
}

const QMetaObject *qjackctlSocketListView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *qjackctlSocketListView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_qjackctlSocketListView.stringdata0))
        return static_cast<void*>(this);
    return QTreeWidget::qt_metacast(_clname);
}

int qjackctlSocketListView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTreeWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 1;
    }
    return _id;
}
namespace {
struct qt_meta_stringdata_qjackctlPatchworkView_t {
    uint offsetsAndSizes[6];
    char stringdata0[22];
    char stringdata1[16];
    char stringdata2[1];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_qjackctlPatchworkView_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_qjackctlPatchworkView_t qt_meta_stringdata_qjackctlPatchworkView = {
    {
        QT_MOC_LITERAL(0, 21),  // "qjackctlPatchworkView"
        QT_MOC_LITERAL(22, 15),  // "contentsChanged"
        QT_MOC_LITERAL(38, 0)   // ""
    },
    "qjackctlPatchworkView",
    "contentsChanged",
    ""
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_qjackctlPatchworkView[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   20,    2, 0x09,    1 /* Protected */,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject qjackctlPatchworkView::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_qjackctlPatchworkView.offsetsAndSizes,
    qt_meta_data_qjackctlPatchworkView,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_qjackctlPatchworkView_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<qjackctlPatchworkView, std::true_type>,
        // method 'contentsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void qjackctlPatchworkView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<qjackctlPatchworkView *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->contentsChanged(); break;
        default: ;
        }
    }
    (void)_a;
}

const QMetaObject *qjackctlPatchworkView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *qjackctlPatchworkView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_qjackctlPatchworkView.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int qjackctlPatchworkView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 1;
    }
    return _id;
}
namespace {
struct qt_meta_stringdata_qjackctlPatchbayView_t {
    uint offsetsAndSizes[18];
    char stringdata0[21];
    char stringdata1[16];
    char stringdata2[1];
    char stringdata3[12];
    char stringdata4[4];
    char stringdata5[20];
    char stringdata6[12];
    char stringdata7[20];
    char stringdata8[9];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_qjackctlPatchbayView_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_qjackctlPatchbayView_t qt_meta_stringdata_qjackctlPatchbayView = {
    {
        QT_MOC_LITERAL(0, 20),  // "qjackctlPatchbayView"
        QT_MOC_LITERAL(21, 15),  // "contentsChanged"
        QT_MOC_LITERAL(37, 0),  // ""
        QT_MOC_LITERAL(38, 11),  // "contextMenu"
        QT_MOC_LITERAL(50, 3),  // "pos"
        QT_MOC_LITERAL(54, 19),  // "qjackctlSocketList*"
        QT_MOC_LITERAL(74, 11),  // "pSocketList"
        QT_MOC_LITERAL(86, 19),  // "activateForwardMenu"
        QT_MOC_LITERAL(106, 8)   // "QAction*"
    },
    "qjackctlPatchbayView",
    "contentsChanged",
    "",
    "contextMenu",
    "pos",
    "qjackctlSocketList*",
    "pSocketList",
    "activateForwardMenu",
    "QAction*"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_qjackctlPatchbayView[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   32,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       3,    2,   33,    2, 0x0a,    2 /* Public */,
       7,    1,   38,    2, 0x0a,    5 /* Public */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QPoint, 0x80000000 | 5,    4,    6,
    QMetaType::Void, 0x80000000 | 8,    2,

       0        // eod
};

Q_CONSTINIT const QMetaObject qjackctlPatchbayView::staticMetaObject = { {
    QMetaObject::SuperData::link<QSplitter::staticMetaObject>(),
    qt_meta_stringdata_qjackctlPatchbayView.offsetsAndSizes,
    qt_meta_data_qjackctlPatchbayView,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_qjackctlPatchbayView_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<qjackctlPatchbayView, std::true_type>,
        // method 'contentsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'contextMenu'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPoint &, std::false_type>,
        QtPrivate::TypeAndForceComplete<qjackctlSocketList *, std::false_type>,
        // method 'activateForwardMenu'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QAction *, std::false_type>
    >,
    nullptr
} };

void qjackctlPatchbayView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<qjackctlPatchbayView *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->contentsChanged(); break;
        case 1: _t->contextMenu((*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qjackctlSocketList*>>(_a[2]))); break;
        case 2: _t->activateForwardMenu((*reinterpret_cast< std::add_pointer_t<QAction*>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 1:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< qjackctlSocketList* >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAction* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (qjackctlPatchbayView::*)();
            if (_t _q_method = &qjackctlPatchbayView::contentsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *qjackctlPatchbayView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *qjackctlPatchbayView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_qjackctlPatchbayView.stringdata0))
        return static_cast<void*>(this);
    return QSplitter::qt_metacast(_clname);
}

int qjackctlPatchbayView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QSplitter::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void qjackctlPatchbayView::contentsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
namespace {
struct qt_meta_stringdata_qjackctlPatchbay_t {
    uint offsetsAndSizes[18];
    char stringdata0[17];
    char stringdata1[8];
    char stringdata2[1];
    char stringdata3[16];
    char stringdata4[19];
    char stringdata5[14];
    char stringdata6[10];
    char stringdata7[6];
    char stringdata8[20];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_qjackctlPatchbay_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_qjackctlPatchbay_t qt_meta_stringdata_qjackctlPatchbay = {
    {
        QT_MOC_LITERAL(0, 16),  // "qjackctlPatchbay"
        QT_MOC_LITERAL(17, 7),  // "refresh"
        QT_MOC_LITERAL(25, 0),  // ""
        QT_MOC_LITERAL(26, 15),  // "connectSelected"
        QT_MOC_LITERAL(42, 18),  // "disconnectSelected"
        QT_MOC_LITERAL(61, 13),  // "disconnectAll"
        QT_MOC_LITERAL(75, 9),  // "expandAll"
        QT_MOC_LITERAL(85, 5),  // "clear"
        QT_MOC_LITERAL(91, 19)   // "connectionsSnapshot"
    },
    "qjackctlPatchbay",
    "refresh",
    "",
    "connectSelected",
    "disconnectSelected",
    "disconnectAll",
    "expandAll",
    "clear",
    "connectionsSnapshot"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_qjackctlPatchbay[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   56,    2, 0x0a,    1 /* Public */,
       3,    0,   57,    2, 0x0a,    2 /* Public */,
       4,    0,   58,    2, 0x0a,    3 /* Public */,
       5,    0,   59,    2, 0x0a,    4 /* Public */,
       6,    0,   60,    2, 0x0a,    5 /* Public */,
       7,    0,   61,    2, 0x0a,    6 /* Public */,
       8,    0,   62,    2, 0x0a,    7 /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject qjackctlPatchbay::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_qjackctlPatchbay.offsetsAndSizes,
    qt_meta_data_qjackctlPatchbay,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_qjackctlPatchbay_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<qjackctlPatchbay, std::true_type>,
        // method 'refresh'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'connectSelected'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'disconnectSelected'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'disconnectAll'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'expandAll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clear'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'connectionsSnapshot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void qjackctlPatchbay::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<qjackctlPatchbay *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->refresh(); break;
        case 1: { bool _r = _t->connectSelected();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 2: { bool _r = _t->disconnectSelected();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 3: { bool _r = _t->disconnectAll();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 4: _t->expandAll(); break;
        case 5: _t->clear(); break;
        case 6: _t->connectionsSnapshot(); break;
        default: ;
        }
    }
}

const QMetaObject *qjackctlPatchbay::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *qjackctlPatchbay::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_qjackctlPatchbay.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int qjackctlPatchbay::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
