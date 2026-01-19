/****************************************************************************
** Meta object code from reading C++ file 'qjackctlMainForm.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/qjackctlMainForm.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qjackctlMainForm.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_qjackctlMainForm_t {
    uint offsetsAndSizes[88];
    char stringdata0[17];
    char stringdata1[10];
    char stringdata2[1];
    char stringdata3[9];
    char stringdata4[11];
    char stringdata5[14];
    char stringdata6[14];
    char stringdata7[15];
    char stringdata8[11];
    char stringdata9[17];
    char stringdata10[3];
    char stringdata11[15];
    char stringdata12[17];
    char stringdata13[11];
    char stringdata14[12];
    char stringdata15[10];
    char stringdata16[23];
    char stringdata17[13];
    char stringdata18[12];
    char stringdata19[14];
    char stringdata20[17];
    char stringdata21[18];
    char stringdata22[15];
    char stringdata23[10];
    char stringdata24[19];
    char stringdata25[19];
    char stringdata26[17];
    char stringdata27[15];
    char stringdata28[25];
    char stringdata29[19];
    char stringdata30[17];
    char stringdata31[18];
    char stringdata32[22];
    char stringdata33[19];
    char stringdata34[16];
    char stringdata35[16];
    char stringdata36[18];
    char stringdata37[14];
    char stringdata38[15];
    char stringdata39[14];
    char stringdata40[17];
    char stringdata41[20];
    char stringdata42[9];
    char stringdata43[13];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_qjackctlMainForm_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_qjackctlMainForm_t qt_meta_stringdata_qjackctlMainForm = {
    {
        QT_MOC_LITERAL(0, 16),  // "qjackctlMainForm"
        QT_MOC_LITERAL(17, 9),  // "startJack"
        QT_MOC_LITERAL(27, 0),  // ""
        QT_MOC_LITERAL(28, 8),  // "stopJack"
        QT_MOC_LITERAL(37, 10),  // "toggleJack"
        QT_MOC_LITERAL(48, 13),  // "showSetupForm"
        QT_MOC_LITERAL(62, 13),  // "showAboutForm"
        QT_MOC_LITERAL(76, 14),  // "resetXrunStats"
        QT_MOC_LITERAL(91, 10),  // "commitData"
        QT_MOC_LITERAL(102, 16),  // "QSessionManager&"
        QT_MOC_LITERAL(119, 2),  // "sm"
        QT_MOC_LITERAL(122, 14),  // "activatePreset"
        QT_MOC_LITERAL(137, 16),  // "activatePatchbay"
        QT_MOC_LITERAL(154, 10),  // "readStdout"
        QT_MOC_LITERAL(165, 11),  // "jackStarted"
        QT_MOC_LITERAL(177, 9),  // "jackError"
        QT_MOC_LITERAL(187, 22),  // "QProcess::ProcessError"
        QT_MOC_LITERAL(210, 12),  // "jackFinished"
        QT_MOC_LITERAL(223, 11),  // "jackCleanup"
        QT_MOC_LITERAL(235, 13),  // "jackStabilize"
        QT_MOC_LITERAL(249, 16),  // "stdoutNotifySlot"
        QT_MOC_LITERAL(266, 17),  // "sigtermNotifySlot"
        QT_MOC_LITERAL(284, 14),  // "alsaNotifySlot"
        QT_MOC_LITERAL(299, 9),  // "timerSlot"
        QT_MOC_LITERAL(309, 18),  // "jackConnectChanged"
        QT_MOC_LITERAL(328, 18),  // "alsaConnectChanged"
        QT_MOC_LITERAL(347, 16),  // "cableConnectSlot"
        QT_MOC_LITERAL(364, 14),  // "toggleMainForm"
        QT_MOC_LITERAL(379, 24),  // "toggleMessagesStatusForm"
        QT_MOC_LITERAL(404, 18),  // "toggleMessagesForm"
        QT_MOC_LITERAL(423, 16),  // "toggleStatusForm"
        QT_MOC_LITERAL(440, 17),  // "toggleSessionForm"
        QT_MOC_LITERAL(458, 21),  // "toggleConnectionsForm"
        QT_MOC_LITERAL(480, 18),  // "togglePatchbayForm"
        QT_MOC_LITERAL(499, 15),  // "toggleGraphForm"
        QT_MOC_LITERAL(515, 15),  // "transportRewind"
        QT_MOC_LITERAL(531, 17),  // "transportBackward"
        QT_MOC_LITERAL(549, 13),  // "transportPlay"
        QT_MOC_LITERAL(563, 14),  // "transportStart"
        QT_MOC_LITERAL(578, 13),  // "transportStop"
        QT_MOC_LITERAL(592, 16),  // "transportForward"
        QT_MOC_LITERAL(609, 19),  // "activatePresetsMenu"
        QT_MOC_LITERAL(629, 8),  // "QAction*"
        QT_MOC_LITERAL(638, 12)   // "quitMainForm"
    },
    "qjackctlMainForm",
    "startJack",
    "",
    "stopJack",
    "toggleJack",
    "showSetupForm",
    "showAboutForm",
    "resetXrunStats",
    "commitData",
    "QSessionManager&",
    "sm",
    "activatePreset",
    "activatePatchbay",
    "readStdout",
    "jackStarted",
    "jackError",
    "QProcess::ProcessError",
    "jackFinished",
    "jackCleanup",
    "jackStabilize",
    "stdoutNotifySlot",
    "sigtermNotifySlot",
    "alsaNotifySlot",
    "timerSlot",
    "jackConnectChanged",
    "alsaConnectChanged",
    "cableConnectSlot",
    "toggleMainForm",
    "toggleMessagesStatusForm",
    "toggleMessagesForm",
    "toggleStatusForm",
    "toggleSessionForm",
    "toggleConnectionsForm",
    "togglePatchbayForm",
    "toggleGraphForm",
    "transportRewind",
    "transportBackward",
    "transportPlay",
    "transportStart",
    "transportStop",
    "transportForward",
    "activatePresetsMenu",
    "QAction*",
    "quitMainForm"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_qjackctlMainForm[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      39,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  248,    2, 0x0a,    1 /* Public */,
       3,    0,  249,    2, 0x0a,    2 /* Public */,
       4,    0,  250,    2, 0x0a,    3 /* Public */,
       5,    0,  251,    2, 0x0a,    4 /* Public */,
       6,    0,  252,    2, 0x0a,    5 /* Public */,
       7,    0,  253,    2, 0x0a,    6 /* Public */,
       8,    1,  254,    2, 0x0a,    7 /* Public */,
      11,    1,  257,    2, 0x0a,    9 /* Public */,
      12,    1,  260,    2, 0x0a,   11 /* Public */,
      13,    0,  263,    2, 0x09,   13 /* Protected */,
      14,    0,  264,    2, 0x09,   14 /* Protected */,
      15,    1,  265,    2, 0x09,   15 /* Protected */,
      17,    0,  268,    2, 0x09,   17 /* Protected */,
      18,    0,  269,    2, 0x09,   18 /* Protected */,
      19,    0,  270,    2, 0x09,   19 /* Protected */,
      20,    1,  271,    2, 0x09,   20 /* Protected */,
      21,    1,  274,    2, 0x09,   22 /* Protected */,
      22,    1,  277,    2, 0x09,   24 /* Protected */,
      23,    0,  280,    2, 0x09,   26 /* Protected */,
      24,    0,  281,    2, 0x09,   27 /* Protected */,
      25,    0,  282,    2, 0x09,   28 /* Protected */,
      26,    3,  283,    2, 0x09,   29 /* Protected */,
      27,    0,  290,    2, 0x09,   33 /* Protected */,
      28,    0,  291,    2, 0x09,   34 /* Protected */,
      29,    0,  292,    2, 0x09,   35 /* Protected */,
      30,    0,  293,    2, 0x09,   36 /* Protected */,
      31,    0,  294,    2, 0x09,   37 /* Protected */,
      32,    0,  295,    2, 0x09,   38 /* Protected */,
      33,    0,  296,    2, 0x09,   39 /* Protected */,
      34,    0,  297,    2, 0x09,   40 /* Protected */,
      35,    0,  298,    2, 0x09,   41 /* Protected */,
      36,    0,  299,    2, 0x09,   42 /* Protected */,
      37,    1,  300,    2, 0x09,   43 /* Protected */,
      38,    0,  303,    2, 0x09,   45 /* Protected */,
      39,    0,  304,    2, 0x09,   46 /* Protected */,
      40,    0,  305,    2, 0x09,   47 /* Protected */,
      41,    1,  306,    2, 0x09,   48 /* Protected */,
      11,    1,  309,    2, 0x09,   50 /* Protected */,
      43,    0,  312,    2, 0x09,   52 /* Protected */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 9,   10,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 16,    2,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::UInt,    2,    2,    2,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 42,    2,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject qjackctlMainForm::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_qjackctlMainForm.offsetsAndSizes,
    qt_meta_data_qjackctlMainForm,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_qjackctlMainForm_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<qjackctlMainForm, std::true_type>,
        // method 'startJack'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stopJack'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleJack'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showSetupForm'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showAboutForm'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'resetXrunStats'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'commitData'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QSessionManager &, std::false_type>,
        // method 'activatePreset'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'activatePatchbay'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'readStdout'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'jackStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'jackError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QProcess::ProcessError, std::false_type>,
        // method 'jackFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'jackCleanup'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'jackStabilize'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stdoutNotifySlot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'sigtermNotifySlot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'alsaNotifySlot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'timerSlot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'jackConnectChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'alsaConnectChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'cableConnectSlot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<unsigned int, std::false_type>,
        // method 'toggleMainForm'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleMessagesStatusForm'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleMessagesForm'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleStatusForm'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleSessionForm'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleConnectionsForm'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'togglePatchbayForm'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleGraphForm'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'transportRewind'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'transportBackward'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'transportPlay'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'transportStart'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'transportStop'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'transportForward'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'activatePresetsMenu'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QAction *, std::false_type>,
        // method 'activatePreset'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'quitMainForm'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void qjackctlMainForm::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<qjackctlMainForm *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->startJack(); break;
        case 1: _t->stopJack(); break;
        case 2: _t->toggleJack(); break;
        case 3: _t->showSetupForm(); break;
        case 4: _t->showAboutForm(); break;
        case 5: _t->resetXrunStats(); break;
        case 6: _t->commitData((*reinterpret_cast< std::add_pointer_t<QSessionManager&>>(_a[1]))); break;
        case 7: _t->activatePreset((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: _t->activatePatchbay((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->readStdout(); break;
        case 10: _t->jackStarted(); break;
        case 11: _t->jackError((*reinterpret_cast< std::add_pointer_t<QProcess::ProcessError>>(_a[1]))); break;
        case 12: _t->jackFinished(); break;
        case 13: _t->jackCleanup(); break;
        case 14: _t->jackStabilize(); break;
        case 15: _t->stdoutNotifySlot((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 16: _t->sigtermNotifySlot((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 17: _t->alsaNotifySlot((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 18: _t->timerSlot(); break;
        case 19: _t->jackConnectChanged(); break;
        case 20: _t->alsaConnectChanged(); break;
        case 21: _t->cableConnectSlot((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<uint>>(_a[3]))); break;
        case 22: _t->toggleMainForm(); break;
        case 23: _t->toggleMessagesStatusForm(); break;
        case 24: _t->toggleMessagesForm(); break;
        case 25: _t->toggleStatusForm(); break;
        case 26: _t->toggleSessionForm(); break;
        case 27: _t->toggleConnectionsForm(); break;
        case 28: _t->togglePatchbayForm(); break;
        case 29: _t->toggleGraphForm(); break;
        case 30: _t->transportRewind(); break;
        case 31: _t->transportBackward(); break;
        case 32: _t->transportPlay((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 33: _t->transportStart(); break;
        case 34: _t->transportStop(); break;
        case 35: _t->transportForward(); break;
        case 36: _t->activatePresetsMenu((*reinterpret_cast< std::add_pointer_t<QAction*>>(_a[1]))); break;
        case 37: _t->activatePreset((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 38: _t->quitMainForm(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 36:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAction* >(); break;
            }
            break;
        }
    }
}

const QMetaObject *qjackctlMainForm::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *qjackctlMainForm::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_qjackctlMainForm.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int qjackctlMainForm::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 39)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 39;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 39)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 39;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
