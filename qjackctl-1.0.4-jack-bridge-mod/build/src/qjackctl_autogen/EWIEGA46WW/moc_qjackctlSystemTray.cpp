/****************************************************************************
** Meta object code from reading C++ file 'qjackctlSystemTray.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/qjackctlSystemTray.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qjackctlSystemTray.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_qjackctlSystemTray_t {
    uint offsetsAndSizes[14];
    char stringdata0[19];
    char stringdata1[8];
    char stringdata2[1];
    char stringdata3[14];
    char stringdata4[14];
    char stringdata5[10];
    char stringdata6[34];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_qjackctlSystemTray_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_qjackctlSystemTray_t qt_meta_stringdata_qjackctlSystemTray = {
    {
        QT_MOC_LITERAL(0, 18),  // "qjackctlSystemTray"
        QT_MOC_LITERAL(19, 7),  // "clicked"
        QT_MOC_LITERAL(27, 0),  // ""
        QT_MOC_LITERAL(28, 13),  // "doubleClicked"
        QT_MOC_LITERAL(42, 13),  // "middleClicked"
        QT_MOC_LITERAL(56, 9),  // "activated"
        QT_MOC_LITERAL(66, 33)   // "QSystemTrayIcon::ActivationRe..."
    },
    "qjackctlSystemTray",
    "clicked",
    "",
    "doubleClicked",
    "middleClicked",
    "activated",
    "QSystemTrayIcon::ActivationReason"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_qjackctlSystemTray[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   38,    2, 0x06,    1 /* Public */,
       3,    0,   39,    2, 0x06,    2 /* Public */,
       4,    0,   40,    2, 0x06,    3 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       5,    1,   41,    2, 0x09,    4 /* Protected */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 6,    2,

       0        // eod
};

Q_CONSTINIT const QMetaObject qjackctlSystemTray::staticMetaObject = { {
    QMetaObject::SuperData::link<QSystemTrayIcon::staticMetaObject>(),
    qt_meta_stringdata_qjackctlSystemTray.offsetsAndSizes,
    qt_meta_data_qjackctlSystemTray,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_qjackctlSystemTray_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<qjackctlSystemTray, std::true_type>,
        // method 'clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'doubleClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'middleClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'activated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QSystemTrayIcon::ActivationReason, std::false_type>
    >,
    nullptr
} };

void qjackctlSystemTray::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<qjackctlSystemTray *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->clicked(); break;
        case 1: _t->doubleClicked(); break;
        case 2: _t->middleClicked(); break;
        case 3: _t->activated((*reinterpret_cast< std::add_pointer_t<QSystemTrayIcon::ActivationReason>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (qjackctlSystemTray::*)();
            if (_t _q_method = &qjackctlSystemTray::clicked; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (qjackctlSystemTray::*)();
            if (_t _q_method = &qjackctlSystemTray::doubleClicked; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (qjackctlSystemTray::*)();
            if (_t _q_method = &qjackctlSystemTray::middleClicked; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject *qjackctlSystemTray::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *qjackctlSystemTray::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_qjackctlSystemTray.stringdata0))
        return static_cast<void*>(this);
    return QSystemTrayIcon::qt_metacast(_clname);
}

int qjackctlSystemTray::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QSystemTrayIcon::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void qjackctlSystemTray::clicked()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void qjackctlSystemTray::doubleClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void qjackctlSystemTray::middleClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
