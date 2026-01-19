/****************************************************************************
** Meta object code from reading C++ file 'qjackctlPatchbayRack.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/qjackctlPatchbayRack.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qjackctlPatchbayRack.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_qjackctlPatchbayRack_t {
    uint offsetsAndSizes[12];
    char stringdata0[21];
    char stringdata1[15];
    char stringdata2[1];
    char stringdata3[12];
    char stringdata4[11];
    char stringdata5[13];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_qjackctlPatchbayRack_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_qjackctlPatchbayRack_t qt_meta_stringdata_qjackctlPatchbayRack = {
    {
        QT_MOC_LITERAL(0, 20),  // "qjackctlPatchbayRack"
        QT_MOC_LITERAL(21, 14),  // "cableConnected"
        QT_MOC_LITERAL(36, 0),  // ""
        QT_MOC_LITERAL(37, 11),  // "sOutputPort"
        QT_MOC_LITERAL(49, 10),  // "sInputPort"
        QT_MOC_LITERAL(60, 12)   // "uiCableFlags"
    },
    "qjackctlPatchbayRack",
    "cableConnected",
    "",
    "sOutputPort",
    "sInputPort",
    "uiCableFlags"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_qjackctlPatchbayRack[] = {

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
       1,    3,   20,    2, 0x06,    1 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::UInt,    3,    4,    5,

       0        // eod
};

Q_CONSTINIT const QMetaObject qjackctlPatchbayRack::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_qjackctlPatchbayRack.offsetsAndSizes,
    qt_meta_data_qjackctlPatchbayRack,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_qjackctlPatchbayRack_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<qjackctlPatchbayRack, std::true_type>,
        // method 'cableConnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<unsigned int, std::false_type>
    >,
    nullptr
} };

void qjackctlPatchbayRack::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<qjackctlPatchbayRack *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->cableConnected((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<uint>>(_a[3]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (qjackctlPatchbayRack::*)(const QString & , const QString & , unsigned int );
            if (_t _q_method = &qjackctlPatchbayRack::cableConnected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *qjackctlPatchbayRack::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *qjackctlPatchbayRack::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_qjackctlPatchbayRack.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int qjackctlPatchbayRack::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
void qjackctlPatchbayRack::cableConnected(const QString & _t1, const QString & _t2, unsigned int _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
