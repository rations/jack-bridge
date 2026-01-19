/****************************************************************************
** Meta object code from reading C++ file 'qjackctlConnect.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/qjackctlConnect.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qjackctlConnect.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_qjackctlClientListView_t {
    uint offsetsAndSizes[10];
    char stringdata0[23];
    char stringdata1[16];
    char stringdata2[1];
    char stringdata3[12];
    char stringdata4[12];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_qjackctlClientListView_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_qjackctlClientListView_t qt_meta_stringdata_qjackctlClientListView = {
    {
        QT_MOC_LITERAL(0, 22),  // "qjackctlClientListView"
        QT_MOC_LITERAL(23, 15),  // "startRenameSlot"
        QT_MOC_LITERAL(39, 0),  // ""
        QT_MOC_LITERAL(40, 11),  // "renamedSlot"
        QT_MOC_LITERAL(52, 11)   // "timeoutSlot"
    },
    "qjackctlClientListView",
    "startRenameSlot",
    "",
    "renamedSlot",
    "timeoutSlot"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_qjackctlClientListView[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   32,    2, 0x09,    1 /* Protected */,
       3,    0,   33,    2, 0x09,    2 /* Protected */,
       4,    0,   34,    2, 0x09,    3 /* Protected */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject qjackctlClientListView::staticMetaObject = { {
    QMetaObject::SuperData::link<QTreeWidget::staticMetaObject>(),
    qt_meta_stringdata_qjackctlClientListView.offsetsAndSizes,
    qt_meta_data_qjackctlClientListView,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_qjackctlClientListView_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<qjackctlClientListView, std::true_type>,
        // method 'startRenameSlot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'renamedSlot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'timeoutSlot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void qjackctlClientListView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<qjackctlClientListView *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->startRenameSlot(); break;
        case 1: _t->renamedSlot(); break;
        case 2: _t->timeoutSlot(); break;
        default: ;
        }
    }
    (void)_a;
}

const QMetaObject *qjackctlClientListView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *qjackctlClientListView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_qjackctlClientListView.stringdata0))
        return static_cast<void*>(this);
    return QTreeWidget::qt_metacast(_clname);
}

int qjackctlClientListView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTreeWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 3;
    }
    return _id;
}
namespace {
struct qt_meta_stringdata_qjackctlConnectorView_t {
    uint offsetsAndSizes[6];
    char stringdata0[22];
    char stringdata1[16];
    char stringdata2[1];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_qjackctlConnectorView_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_qjackctlConnectorView_t qt_meta_stringdata_qjackctlConnectorView = {
    {
        QT_MOC_LITERAL(0, 21),  // "qjackctlConnectorView"
        QT_MOC_LITERAL(22, 15),  // "contentsChanged"
        QT_MOC_LITERAL(38, 0)   // ""
    },
    "qjackctlConnectorView",
    "contentsChanged",
    ""
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_qjackctlConnectorView[] = {

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

Q_CONSTINIT const QMetaObject qjackctlConnectorView::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_qjackctlConnectorView.offsetsAndSizes,
    qt_meta_data_qjackctlConnectorView,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_qjackctlConnectorView_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<qjackctlConnectorView, std::true_type>,
        // method 'contentsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void qjackctlConnectorView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<qjackctlConnectorView *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->contentsChanged(); break;
        default: ;
        }
    }
    (void)_a;
}

const QMetaObject *qjackctlConnectorView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *qjackctlConnectorView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_qjackctlConnectorView.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int qjackctlConnectorView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
struct qt_meta_stringdata_qjackctlConnectView_t {
    uint offsetsAndSizes[6];
    char stringdata0[20];
    char stringdata1[15];
    char stringdata2[1];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_qjackctlConnectView_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_qjackctlConnectView_t qt_meta_stringdata_qjackctlConnectView = {
    {
        QT_MOC_LITERAL(0, 19),  // "qjackctlConnectView"
        QT_MOC_LITERAL(20, 14),  // "aliasesChanged"
        QT_MOC_LITERAL(35, 0)   // ""
    },
    "qjackctlConnectView",
    "aliasesChanged",
    ""
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_qjackctlConnectView[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   20,    2, 0x06,    1 /* Public */,

 // signals: parameters
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject qjackctlConnectView::staticMetaObject = { {
    QMetaObject::SuperData::link<QSplitter::staticMetaObject>(),
    qt_meta_stringdata_qjackctlConnectView.offsetsAndSizes,
    qt_meta_data_qjackctlConnectView,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_qjackctlConnectView_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<qjackctlConnectView, std::true_type>,
        // method 'aliasesChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void qjackctlConnectView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<qjackctlConnectView *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->aliasesChanged(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (qjackctlConnectView::*)();
            if (_t _q_method = &qjackctlConnectView::aliasesChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
    (void)_a;
}

const QMetaObject *qjackctlConnectView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *qjackctlConnectView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_qjackctlConnectView.stringdata0))
        return static_cast<void*>(this);
    return QSplitter::qt_metacast(_clname);
}

int qjackctlConnectView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QSplitter::qt_metacall(_c, _id, _a);
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

// SIGNAL 0
void qjackctlConnectView::aliasesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
namespace {
struct qt_meta_stringdata_qjackctlConnect_t {
    uint offsetsAndSizes[26];
    char stringdata0[16];
    char stringdata1[15];
    char stringdata2[1];
    char stringdata3[11];
    char stringdata4[18];
    char stringdata5[14];
    char stringdata6[8];
    char stringdata7[16];
    char stringdata8[19];
    char stringdata9[14];
    char stringdata10[10];
    char stringdata11[15];
    char stringdata12[7];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_qjackctlConnect_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_qjackctlConnect_t qt_meta_stringdata_qjackctlConnect = {
    {
        QT_MOC_LITERAL(0, 15),  // "qjackctlConnect"
        QT_MOC_LITERAL(16, 14),  // "connectChanged"
        QT_MOC_LITERAL(31, 0),  // ""
        QT_MOC_LITERAL(32, 10),  // "connecting"
        QT_MOC_LITERAL(43, 17),  // "qjackctlPortItem*"
        QT_MOC_LITERAL(61, 13),  // "disconnecting"
        QT_MOC_LITERAL(75, 7),  // "refresh"
        QT_MOC_LITERAL(83, 15),  // "connectSelected"
        QT_MOC_LITERAL(99, 18),  // "disconnectSelected"
        QT_MOC_LITERAL(118, 13),  // "disconnectAll"
        QT_MOC_LITERAL(132, 9),  // "expandAll"
        QT_MOC_LITERAL(142, 14),  // "updateContents"
        QT_MOC_LITERAL(157, 6)   // "bClear"
    },
    "qjackctlConnect",
    "connectChanged",
    "",
    "connecting",
    "qjackctlPortItem*",
    "disconnecting",
    "refresh",
    "connectSelected",
    "disconnectSelected",
    "disconnectAll",
    "expandAll",
    "updateContents",
    "bClear"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_qjackctlConnect[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   68,    2, 0x06,    1 /* Public */,
       3,    2,   69,    2, 0x06,    2 /* Public */,
       5,    2,   74,    2, 0x06,    5 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       6,    0,   79,    2, 0x0a,    8 /* Public */,
       7,    0,   80,    2, 0x0a,    9 /* Public */,
       8,    0,   81,    2, 0x0a,   10 /* Public */,
       9,    0,   82,    2, 0x0a,   11 /* Public */,
      10,    0,   83,    2, 0x0a,   12 /* Public */,
      11,    1,   84,    2, 0x0a,   13 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4, 0x80000000 | 4,    2,    2,
    QMetaType::Void, 0x80000000 | 4, 0x80000000 | 4,    2,    2,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   12,

       0        // eod
};

Q_CONSTINIT const QMetaObject qjackctlConnect::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_qjackctlConnect.offsetsAndSizes,
    qt_meta_data_qjackctlConnect,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_qjackctlConnect_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<qjackctlConnect, std::true_type>,
        // method 'connectChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'connecting'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qjackctlPortItem *, std::false_type>,
        QtPrivate::TypeAndForceComplete<qjackctlPortItem *, std::false_type>,
        // method 'disconnecting'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qjackctlPortItem *, std::false_type>,
        QtPrivate::TypeAndForceComplete<qjackctlPortItem *, std::false_type>,
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
        // method 'updateContents'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>
    >,
    nullptr
} };

void qjackctlConnect::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<qjackctlConnect *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->connectChanged(); break;
        case 1: _t->connecting((*reinterpret_cast< std::add_pointer_t<qjackctlPortItem*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qjackctlPortItem*>>(_a[2]))); break;
        case 2: _t->disconnecting((*reinterpret_cast< std::add_pointer_t<qjackctlPortItem*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qjackctlPortItem*>>(_a[2]))); break;
        case 3: _t->refresh(); break;
        case 4: { bool _r = _t->connectSelected();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 5: { bool _r = _t->disconnectSelected();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 6: { bool _r = _t->disconnectAll();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 7: _t->expandAll(); break;
        case 8: _t->updateContents((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (qjackctlConnect::*)();
            if (_t _q_method = &qjackctlConnect::connectChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (qjackctlConnect::*)(qjackctlPortItem * , qjackctlPortItem * );
            if (_t _q_method = &qjackctlConnect::connecting; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (qjackctlConnect::*)(qjackctlPortItem * , qjackctlPortItem * );
            if (_t _q_method = &qjackctlConnect::disconnecting; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject *qjackctlConnect::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *qjackctlConnect::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_qjackctlConnect.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int qjackctlConnect::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void qjackctlConnect::connectChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void qjackctlConnect::connecting(qjackctlPortItem * _t1, qjackctlPortItem * _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void qjackctlConnect::disconnecting(qjackctlPortItem * _t1, qjackctlPortItem * _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
