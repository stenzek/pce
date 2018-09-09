/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.7.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "mainwindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.7.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_MainWindow_t
{
  QByteArrayData data[22];
  char stringdata0[379];
};
#define QT_MOC_LITERAL(idx, ofs, len)                                                                                  \
  Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(                                                             \
    len, qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs - idx * sizeof(QByteArrayData)))
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
  {
    QT_MOC_LITERAL(0, 0, 10),    // "MainWindow"
    QT_MOC_LITERAL(1, 11, 29),   // "onEnableDebuggerActionToggled"
    QT_MOC_LITERAL(2, 41, 0),    // ""
    QT_MOC_LITERAL(3, 42, 7),    // "checked"
    QT_MOC_LITERAL(4, 50, 22),   // "onResetActionTriggered"
    QT_MOC_LITERAL(5, 73, 22),   // "onAboutActionTriggered"
    QT_MOC_LITERAL(6, 96, 24),   // "onChangeFloppyATriggered"
    QT_MOC_LITERAL(7, 121, 24),  // "onChangeFloppyBTriggered"
    QT_MOC_LITERAL(8, 146, 25),  // "onDisplayWidgetKeyPressed"
    QT_MOC_LITERAL(9, 172, 10),  // "QKeyEvent*"
    QT_MOC_LITERAL(10, 183, 5),  // "event"
    QT_MOC_LITERAL(11, 189, 26), // "onDisplayWidgetKeyReleased"
    QT_MOC_LITERAL(12, 216, 19), // "onSystemInitialized"
    QT_MOC_LITERAL(13, 236, 15), // "onSystemDestroy"
    QT_MOC_LITERAL(14, 252, 18), // "onSimulationPaused"
    QT_MOC_LITERAL(15, 271, 19), // "onSimulationResumed"
    QT_MOC_LITERAL(16, 291, 23), // "onSimulationSpeedUpdate"
    QT_MOC_LITERAL(17, 315, 13), // "speed_percent"
    QT_MOC_LITERAL(18, 329, 15), // "onStatusMessage"
    QT_MOC_LITERAL(19, 345, 7),  // "message"
    QT_MOC_LITERAL(20, 353, 17), // "onDebuggerEnabled"
    QT_MOC_LITERAL(21, 371, 7)   // "enabled"

  },
  "MainWindow\0onEnableDebuggerActionToggled\0"
  "\0checked\0onResetActionTriggered\0"
  "onAboutActionTriggered\0onChangeFloppyATriggered\0"
  "onChangeFloppyBTriggered\0"
  "onDisplayWidgetKeyPressed\0QKeyEvent*\0"
  "event\0onDisplayWidgetKeyReleased\0"
  "onSystemInitialized\0onSystemDestroy\0"
  "onSimulationPaused\0onSimulationResumed\0"
  "onSimulationSpeedUpdate\0speed_percent\0"
  "onStatusMessage\0message\0onDebuggerEnabled\0"
  "enabled"};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

  // content:
  7,      // revision
  0,      // classname
  0, 0,   // classinfo
  14, 14, // methods
  0, 0,   // properties
  0, 0,   // enums/sets
  0, 0,   // constructors
  0,      // flags
  0,      // signalCount

  // slots: name, argc, parameters, tag, flags
  1, 1, 84, 2, 0x0a /* Public */, 4, 0, 87, 2, 0x0a /* Public */, 5, 0, 88, 2, 0x0a /* Public */, 6, 0, 89, 2,
  0x0a /* Public */, 7, 0, 90, 2, 0x0a /* Public */, 8, 1, 91, 2, 0x0a /* Public */, 11, 1, 94, 2, 0x0a /* Public */,
  12, 0, 97, 2, 0x08 /* Private */, 13, 0, 98, 2, 0x08 /* Private */, 14, 0, 99, 2, 0x08 /* Private */, 15, 0, 100, 2,
  0x08 /* Private */, 16, 1, 101, 2, 0x08 /* Private */, 18, 1, 104, 2, 0x08 /* Private */, 20, 1, 107, 2,
  0x08 /* Private */,

  // slots: parameters
  QMetaType::Void, QMetaType::Bool, 3, QMetaType::Void, QMetaType::Void, QMetaType::Void, QMetaType::Void,
  QMetaType::Void, 0x80000000 | 9, 10, QMetaType::Void, 0x80000000 | 9, 10, QMetaType::Void, QMetaType::Void,
  QMetaType::Void, QMetaType::Void, QMetaType::Void, QMetaType::Float, 17, QMetaType::Void, QMetaType::QString, 19,
  QMetaType::Void, QMetaType::Bool, 21,

  0 // eod
};

void MainWindow::qt_static_metacall(QObject* _o, QMetaObject::Call _c, int _id, void** _a)
{
  if (_c == QMetaObject::InvokeMetaMethod)
  {
    Q_ASSERT(staticMetaObject.cast(_o));
    MainWindow* _t = static_cast<MainWindow*>(_o);
    Q_UNUSED(_t)
    switch (_id)
    {
      case 0:
        _t->onEnableDebuggerActionToggled((*reinterpret_cast<bool(*)>(_a[1])));
        break;
      case 1:
        _t->onResetActionTriggered();
        break;
      case 2:
        _t->onAboutActionTriggered();
        break;
      case 3:
        _t->onChangeFloppyATriggered();
        break;
      case 4:
        _t->onChangeFloppyBTriggered();
        break;
      case 5:
        _t->onDisplayWidgetKeyPressed((*reinterpret_cast<QKeyEvent*(*)>(_a[1])));
        break;
      case 6:
        _t->onDisplayWidgetKeyReleased((*reinterpret_cast<QKeyEvent*(*)>(_a[1])));
        break;
      case 7:
        _t->onSystemInitialized();
        break;
      case 8:
        _t->onSystemDestroy();
        break;
      case 9:
        _t->onSimulationPaused();
        break;
      case 10:
        _t->onSimulationResumed();
        break;
      case 11:
        _t->onSimulationSpeedUpdate((*reinterpret_cast<float(*)>(_a[1])));
        break;
      case 12:
        _t->onStatusMessage((*reinterpret_cast<QString(*)>(_a[1])));
        break;
      case 13:
        _t->onDebuggerEnabled((*reinterpret_cast<bool(*)>(_a[1])));
        break;
      default:;
    }
  }
}

const QMetaObject MainWindow::staticMetaObject = {{&QMainWindow::staticMetaObject, qt_meta_stringdata_MainWindow.data,
                                                   qt_meta_data_MainWindow, qt_static_metacall, Q_NULLPTR, Q_NULLPTR}};

const QMetaObject* MainWindow::metaObject() const
{
  return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void* MainWindow::qt_metacast(const char* _clname)
{
  if (!_clname)
    return Q_NULLPTR;
  if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
    return static_cast<void*>(const_cast<MainWindow*>(this));
  return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void** _a)
{
  _id = QMainWindow::qt_metacall(_c, _id, _a);
  if (_id < 0)
    return _id;
  if (_c == QMetaObject::InvokeMetaMethod)
  {
    if (_id < 14)
      qt_static_metacall(this, _c, _id, _a);
    _id -= 14;
  }
  else if (_c == QMetaObject::RegisterMethodArgumentMetaType)
  {
    if (_id < 14)
      *reinterpret_cast<int*>(_a[0]) = -1;
    _id -= 14;
  }
  return _id;
}
QT_END_MOC_NAMESPACE
