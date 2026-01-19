/****************************************************************************
** Meta object code from reading C++ file 'qjackctlSessionSaveForm.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/qjackctlSessionSaveForm.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qjackctlSessionSaveForm.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_qjackctlSessionSaveForm_t {
    uint offsetsAndSizes[18];
    char stringdata0[24];
    char stringdata1[7];
    char stringdata2[1];
    char stringdata3[7];
    char stringdata4[18];
    char stringdata5[13];
    char stringdata6[17];
    char stringdata7[12];
    char stringdata8[17];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_qjackctlSessionSaveForm_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_qjackctlSessionSaveForm_t qt_meta_stringdata_qjackctlSessionSaveForm = {
    {
        QT_MOC_LITERAL(0, 23),  // "qjackctlSessionSaveForm"
        QT_MOC_LITERAL(24, 6),  // "accept"
        QT_MOC_LITERAL(31, 0),  // ""
        QT_MOC_LITERAL(32, 6),  // "reject"
        QT_MOC_LITERAL(39, 17),  // "changeSessionName"
        QT_MOC_LITERAL(57, 12),  // "sSessionName"
        QT_MOC_LITERAL(70, 16),  // "changeSessionDir"
        QT_MOC_LITERAL(87, 11),  // "sSessionDir"
        QT_MOC_LITERAL(99, 16)   // "browseSessionDir"
    },
    "qjackctlSessionSaveForm",
    "accept",
    "",
    "reject",
    "changeSessionName",
    "sSessionName",
    "changeSessionDir",
    "sSessionDir",
    "browseSessionDir"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_qjackctlSessionSaveForm[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   44,    2, 0x09,    1 /* Protected */,
       3,    0,   45,    2, 0x09,    2 /* Protected */,
       4,    1,   46,    2, 0x09,    3 /* Protected */,
       6,    1,   49,    2, 0x09,    5 /* Protected */,
       8,    0,   52,    2, 0x09,    7 /* Protected */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject qjackctlSessionSaveForm::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_qjackctlSessionSaveForm.offsetsAndSizes,
    qt_meta_data_qjackctlSessionSaveForm,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_qjackctlSessionSaveForm_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<qjackctlSessionSaveForm, std::true_type>,
        // method 'accept'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'reject'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'changeSessionName'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'changeSessionDir'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'browseSessionDir'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void qjackctlSessionSaveForm::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<qjackctlSessionSaveForm *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->accept(); break;
        case 1: _t->reject(); break;
        case 2: _t->changeSessionName((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->changeSessionDir((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->browseSessionDir(); break;
        default: ;
        }
    }
}

const QMetaObject *qjackctlSessionSaveForm::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *qjackctlSessionSaveForm::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_qjackctlSessionSaveForm.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int qjackctlSessionSaveForm::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
