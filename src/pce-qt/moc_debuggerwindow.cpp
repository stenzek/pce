/****************************************************************************
** Meta object code from reading C++ file 'debuggerwindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.7.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "debuggerwindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'debuggerwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.7.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_DebuggerWindow_t
{
  QByteArrayData data[9];
  char stringdata0[146];
};
#define QT_MOC_LITERAL(idx, ofs, len)                                                                                  \
  Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(                                                             \
    len, qptrdiff(offsetof(qt_meta_stringdata_DebuggerWindow_t, stringdata0) + ofs - idx * sizeof(QByteArrayData)))
static const qt_meta_stringdata_DebuggerWindow_t qt_meta_stringdata_DebuggerWindow = {
  {
    QT_MOC_LITERAL(0, 0, 14),  // "DebuggerWindow"
    QT_MOC_LITERAL(1, 15, 18), // "onSimulationPaused"
    QT_MOC_LITERAL(2, 34, 0),  // ""
    QT_MOC_LITERAL(3, 35, 19), // "onSimulationResumed"
    QT_MOC_LITERAL(4, 55, 10), // "refreshAll"
    QT_MOC_LITERAL(5, 66, 22), // "onCloseActionTriggered"
    QT_MOC_LITERAL(6, 89, 20), // "onRunActionTriggered"
    QT_MOC_LITERAL(7, 110, 7), // "checked"
    QT_MOC_LITERAL(8, 118, 27) // "onSingleStepActionTriggered"

  },
  "DebuggerWindow\0onSimulationPaused\0\0"
  "onSimulationResumed\0refreshAll\0"
  "onCloseActionTriggered\0onRunActionTriggered\0"
  "checked\0onSingleStepActionTriggered"};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DebuggerWindow[] = {

  // content:
  7,     // revision
  0,     // classname
  0, 0,  // classinfo
  6, 14, // methods
  0, 0,  // properties
  0, 0,  // enums/sets
  0, 0,  // constructors
  0,     // flags
  0,     // signalCount

  // slots: name, argc, parameters, tag, flags
  1, 0, 44, 2, 0x0a /* Public */, 3, 0, 45, 2, 0x0a /* Public */, 4, 0, 46, 2, 0x0a /* Public */, 5, 0, 47, 2,
  0x0a /* Public */, 6, 1, 48, 2, 0x0a /* Public */, 8, 0, 51, 2, 0x0a /* Public */,

  // slots: parameters
  QMetaType::Void, QMetaType::Void, QMetaType::Void, QMetaType::Void, QMetaType::Void, QMetaType::Bool, 7,
  QMetaType::Void,

  0 // eod
};

void DebuggerWindow::qt_static_metacall(QObject* _o, QMetaObject::Call _c, int _id, void** _a)
{
  if (_c == QMetaObject::InvokeMetaMethod)
  {
    Q_ASSERT(staticMetaObject.cast(_o));
    DebuggerWindow* _t = static_cast<DebuggerWindow*>(_o);
    Q_UNUSED(_t)
    switch (_id)
    {
      case 0:
        _t->onSimulationPaused();
        break;
      case 1:
        _t->onSimulationResumed();
        break;
      case 2:
        _t->refreshAll();
        break;
      case 3:
        _t->onCloseActionTriggered();
        break;
      case 4:
        _t->onRunActionTriggered((*reinterpret_cast<bool(*)>(_a[1])));
        break;
      case 5:
        _t->onSingleStepActionTriggered();
        break;
      default:;
    }
  }
}

const QMetaObject DebuggerWindow::staticMetaObject = {
  {&QMainWindow::staticMetaObject, qt_meta_stringdata_DebuggerWindow.data, qt_meta_data_DebuggerWindow,
   qt_static_metacall, Q_NULLPTR, Q_NULLPTR}};

const QMetaObject* DebuggerWindow::metaObject() const
{
  return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void* DebuggerWindow::qt_metacast(const char* _clname)
{
  if (!_clname)
    return Q_NULLPTR;
  if (!strcmp(_clname, qt_meta_stringdata_DebuggerWindow.stringdata0))
    return static_cast<void*>(const_cast<DebuggerWindow*>(this));
  return QMainWindow::qt_metacast(_clname);
}

int DebuggerWindow::qt_metacall(QMetaObject::Call _c, int _id, void** _a)
{
  _id = QMainWindow::qt_metacall(_c, _id, _a);
  if (_id < 0)
    return _id;
  if (_c == QMetaObject::InvokeMetaMethod)
  {
    if (_id < 6)
      qt_static_metacall(this, _c, _id, _a);
    _id -= 6;
  }
  else if (_c == QMetaObject::RegisterMethodArgumentMetaType)
  {
    if (_id < 6)
      *reinterpret_cast<int*>(_a[0]) = -1;
    _id -= 6;
  }
  return _id;
}
QT_END_MOC_NAMESPACE
