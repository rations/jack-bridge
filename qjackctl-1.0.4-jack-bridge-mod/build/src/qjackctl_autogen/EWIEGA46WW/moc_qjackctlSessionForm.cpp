/****************************************************************************
** Meta object code from reading C++ file 'qjackctlSessionForm.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/qjackctlSessionForm.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qjackctlSessionForm.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_qjackctlSessionInfraClientItemEditor_t {
    uint offsetsAndSizes[12];
    char stringdata0[37];
    char stringdata1[13];
    char stringdata2[1];
    char stringdata3[11];
    char stringdata4[10];
    char stringdata5[11];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_qjackctlSessionInfraClientItemEditor_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_qjackctlSessionInfraClientItemEditor_t qt_meta_stringdata_qjackctlSessionInfraClientItemEditor = {
    {
        QT_MOC_LITERAL(0, 36),  // "qjackctlSessionInfraClientIte..."
        QT_MOC_LITERAL(37, 12),  // "finishSignal"
        QT_MOC_LITERAL(50, 0),  // ""
        QT_MOC_LITERAL(51, 10),  // "browseSlot"
        QT_MOC_LITERAL(62, 9),  // "resetSlot"
        QT_MOC_LITERAL(72, 10)   // "finishSlot"
    },
    "qjackctlSessionInfraClientItemEditor",
    "finishSignal",
    "",
    "browseSlot",
    "resetSlot",
    "finishSlot"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_qjackctlSessionInfraClientItemEditor[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   38,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       3,    0,   39,    2, 0x09,    2 /* Protected */,
       4,    0,   40,    2, 0x09,    3 /* Protected */,
       5,    0,   41,    2, 0x09,    4 /* Protected */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject qjackctlSessionInfraClientItemEditor::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_qjackctlSessionInfraClientItemEditor.offsetsAndSizes,
    qt_meta_data_qjackctlSessionInfraClientItemEditor,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_qjackctlSessionInfraClientItemEditor_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<qjackctlSessionInfraClientItemEditor, std::true_type>,
        // method 'finishSignal'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'browseSlot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'resetSlot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'finishSlot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void qjackctlSessionInfraClientItemEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<qjackctlSessionInfraClientItemEditor *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->finishSignal(); break;
        case 1: _t->browseSlot(); break;
        case 2: _t->resetSlot(); break;
        case 3: _t->finishSlot(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (qjackctlSessionInfraClientItemEditor::*)();
            if (_t _q_method = &qjackctlSessionInfraClientItemEditor::finishSignal; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
    (void)_a;
}

const QMetaObject *qjackctlSessionInfraClientItemEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *qjackctlSessionInfraClientItemEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_qjackctlSessionInfraClientItemEditor.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int qjackctlSessionInfraClientItemEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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
void qjackctlSessionInfraClientItemEditor::finishSignal()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
namespace {
struct qt_meta_stringdata_qjackctlSessionInfraClientItemDelegate_t {
    uint offsetsAndSizes[6];
    char stringdata0[39];
    char stringdata1[13];
    char stringdata2[1];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_qjackctlSessionInfraClientItemDelegate_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_qjackctlSessionInfraClientItemDelegate_t qt_meta_stringdata_qjackctlSessionInfraClientItemDelegate = {
    {
        QT_MOC_LITERAL(0, 38),  // "qjackctlSessionInfraClientIte..."
        QT_MOC_LITERAL(39, 12),  // "commitEditor"
        QT_MOC_LITERAL(52, 0)   // ""
    },
    "qjackctlSessionInfraClientItemDelegate",
    "commitEditor",
    ""
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_qjackctlSessionInfraClientItemDelegate[] = {

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

Q_CONSTINIT const QMetaObject qjackctlSessionInfraClientItemDelegate::staticMetaObject = { {
    QMetaObject::SuperData::link<QItemDelegate::staticMetaObject>(),
    qt_meta_stringdata_qjackctlSessionInfraClientItemDelegate.offsetsAndSizes,
    qt_meta_data_qjackctlSessionInfraClientItemDelegate,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_qjackctlSessionInfraClientItemDelegate_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<qjackctlSessionInfraClientItemDelegate, std::true_type>,
        // method 'commitEditor'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void qjackctlSessionInfraClientItemDelegate::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<qjackctlSessionInfraClientItemDelegate *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->commitEditor(); break;
        default: ;
        }
    }
    (void)_a;
}

const QMetaObject *qjackctlSessionInfraClientItemDelegate::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *qjackctlSessionInfraClientItemDelegate::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_qjackctlSessionInfraClientItemDelegate.stringdata0))
        return static_cast<void*>(this);
    return QItemDelegate::qt_metacast(_clname);
}

int qjackctlSessionInfraClientItemDelegate::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QItemDelegate::qt_metacall(_c, _id, _a);
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
struct qt_meta_stringdata_qjackctlSessionForm_t {
    uint offsetsAndSizes[40];
    char stringdata0[20];
    char stringdata1[12];
    char stringdata2[1];
    char stringdata3[16];
    char stringdata4[23];
    char stringdata5[24];
    char stringdata6[19];
    char stringdata7[14];
    char stringdata8[14];
    char stringdata9[17];
    char stringdata10[16];
    char stringdata11[23];
    char stringdata12[4];
    char stringdata13[15];
    char stringdata14[16];
    char stringdata15[22];
    char stringdata16[18];
    char stringdata17[18];
    char stringdata18[19];
    char stringdata19[23];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_qjackctlSessionForm_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_qjackctlSessionForm_t qt_meta_stringdata_qjackctlSessionForm = {
    {
        QT_MOC_LITERAL(0, 19),  // "qjackctlSessionForm"
        QT_MOC_LITERAL(20, 11),  // "loadSession"
        QT_MOC_LITERAL(32, 0),  // ""
        QT_MOC_LITERAL(33, 15),  // "saveSessionSave"
        QT_MOC_LITERAL(49, 22),  // "saveSessionSaveAndQuit"
        QT_MOC_LITERAL(72, 23),  // "saveSessionSaveTemplate"
        QT_MOC_LITERAL(96, 18),  // "saveSessionVersion"
        QT_MOC_LITERAL(115, 13),  // "updateSession"
        QT_MOC_LITERAL(129, 13),  // "recentSession"
        QT_MOC_LITERAL(143, 16),  // "updateRecentMenu"
        QT_MOC_LITERAL(160, 15),  // "clearRecentMenu"
        QT_MOC_LITERAL(176, 22),  // "sessionViewContextMenu"
        QT_MOC_LITERAL(199, 3),  // "pos"
        QT_MOC_LITERAL(203, 14),  // "addInfraClient"
        QT_MOC_LITERAL(218, 15),  // "editInfraClient"
        QT_MOC_LITERAL(234, 21),  // "editInfraClientCommit"
        QT_MOC_LITERAL(256, 17),  // "removeInfraClient"
        QT_MOC_LITERAL(274, 17),  // "selectInfraClient"
        QT_MOC_LITERAL(292, 18),  // "updateInfraClients"
        QT_MOC_LITERAL(311, 22)   // "infraClientContextMenu"
    },
    "qjackctlSessionForm",
    "loadSession",
    "",
    "saveSessionSave",
    "saveSessionSaveAndQuit",
    "saveSessionSaveTemplate",
    "saveSessionVersion",
    "updateSession",
    "recentSession",
    "updateRecentMenu",
    "clearRecentMenu",
    "sessionViewContextMenu",
    "pos",
    "addInfraClient",
    "editInfraClient",
    "editInfraClientCommit",
    "removeInfraClient",
    "selectInfraClient",
    "updateInfraClients",
    "infraClientContextMenu"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_qjackctlSessionForm[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      17,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  116,    2, 0x0a,    1 /* Public */,
       3,    0,  117,    2, 0x0a,    2 /* Public */,
       4,    0,  118,    2, 0x0a,    3 /* Public */,
       5,    0,  119,    2, 0x0a,    4 /* Public */,
       6,    1,  120,    2, 0x0a,    5 /* Public */,
       7,    0,  123,    2, 0x0a,    7 /* Public */,
       8,    0,  124,    2, 0x09,    8 /* Protected */,
       9,    0,  125,    2, 0x09,    9 /* Protected */,
      10,    0,  126,    2, 0x09,   10 /* Protected */,
      11,    1,  127,    2, 0x09,   11 /* Protected */,
      13,    0,  130,    2, 0x09,   13 /* Protected */,
      14,    0,  131,    2, 0x09,   14 /* Protected */,
      15,    0,  132,    2, 0x09,   15 /* Protected */,
      16,    0,  133,    2, 0x09,   16 /* Protected */,
      17,    0,  134,    2, 0x09,   17 /* Protected */,
      18,    0,  135,    2, 0x09,   18 /* Protected */,
      19,    1,  136,    2, 0x09,   19 /* Protected */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPoint,   12,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPoint,   12,

       0        // eod
};

Q_CONSTINIT const QMetaObject qjackctlSessionForm::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_qjackctlSessionForm.offsetsAndSizes,
    qt_meta_data_qjackctlSessionForm,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_qjackctlSessionForm_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<qjackctlSessionForm, std::true_type>,
        // method 'loadSession'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'saveSessionSave'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'saveSessionSaveAndQuit'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'saveSessionSaveTemplate'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'saveSessionVersion'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'updateSession'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'recentSession'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateRecentMenu'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clearRecentMenu'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sessionViewContextMenu'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPoint &, std::false_type>,
        // method 'addInfraClient'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'editInfraClient'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'editInfraClientCommit'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'removeInfraClient'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'selectInfraClient'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateInfraClients'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'infraClientContextMenu'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPoint &, std::false_type>
    >,
    nullptr
} };

void qjackctlSessionForm::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<qjackctlSessionForm *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->loadSession(); break;
        case 1: _t->saveSessionSave(); break;
        case 2: _t->saveSessionSaveAndQuit(); break;
        case 3: _t->saveSessionSaveTemplate(); break;
        case 4: _t->saveSessionVersion((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 5: _t->updateSession(); break;
        case 6: _t->recentSession(); break;
        case 7: _t->updateRecentMenu(); break;
        case 8: _t->clearRecentMenu(); break;
        case 9: _t->sessionViewContextMenu((*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[1]))); break;
        case 10: _t->addInfraClient(); break;
        case 11: _t->editInfraClient(); break;
        case 12: _t->editInfraClientCommit(); break;
        case 13: _t->removeInfraClient(); break;
        case 14: _t->selectInfraClient(); break;
        case 15: _t->updateInfraClients(); break;
        case 16: _t->infraClientContextMenu((*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *qjackctlSessionForm::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *qjackctlSessionForm::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_qjackctlSessionForm.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int qjackctlSessionForm::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 17)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 17;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
