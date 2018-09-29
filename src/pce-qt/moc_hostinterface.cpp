/****************************************************************************
** Meta object code from reading C++ file 'hostinterface.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.7.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "hostinterface.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'hostinterface.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.7.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_QtHostInterface_t
{
  QByteArrayData data[22];
  char stringdata0[306];
};
#define QT_MOC_LITERAL(idx, ofs, len)                                                                                  \
  Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(                                                             \
    len, qptrdiff(offsetof(qt_meta_stringdata_QtHostInterface_t, stringdata0) + ofs - idx * sizeof(QByteArrayData)))
static const qt_meta_stringdata_QtHostInterface_t qt_meta_stringdata_QtHostInterface = {
  {
    QT_MOC_LITERAL(0, 0, 15),    // "QtHostInterface"
    QT_MOC_LITERAL(1, 16, 19),   // "onSystemInitialized"
    QT_MOC_LITERAL(2, 36, 0),    // ""
    QT_MOC_LITERAL(3, 37, 15),   // "onSystemDestroy"
    QT_MOC_LITERAL(4, 53, 18),   // "onSimulationPaused"
    QT_MOC_LITERAL(5, 72, 19),   // "onSimulationResumed"
    QT_MOC_LITERAL(6, 92, 23),   // "onSimulationSpeedUpdate"
    QT_MOC_LITERAL(7, 116, 13),  // "speed_percent"
    QT_MOC_LITERAL(8, 130, 3),   // "vps"
    QT_MOC_LITERAL(9, 134, 15),  // "onStatusMessage"
    QT_MOC_LITERAL(10, 150, 7),  // "message"
    QT_MOC_LITERAL(11, 158, 17), // "onDebuggerEnabled"
    QT_MOC_LITERAL(12, 176, 7),  // "enabled"
    QT_MOC_LITERAL(13, 184, 15), // "startSimulation"
    QT_MOC_LITERAL(14, 200, 8),  // "filename"
    QT_MOC_LITERAL(15, 209, 12), // "start_paused"
    QT_MOC_LITERAL(16, 222, 15), // "pauseSimulation"
    QT_MOC_LITERAL(17, 238, 6),  // "paused"
    QT_MOC_LITERAL(18, 245, 15), // "resetSimulation"
    QT_MOC_LITERAL(19, 261, 14), // "stopSimulation"
    QT_MOC_LITERAL(20, 276, 14), // "sendCtrlAltDel"
    QT_MOC_LITERAL(21, 291, 14)  // "enableDebugger"

  },
  "QtHostInterface\0onSystemInitialized\0"
  "\0onSystemDestroy\0onSimulationPaused\0"
  "onSimulationResumed\0onSimulationSpeedUpdate\0"
  "speed_percent\0vps\0onStatusMessage\0"
  "message\0onDebuggerEnabled\0enabled\0"
  "startSimulation\0filename\0start_paused\0"
  "pauseSimulation\0paused\0resetSimulation\0"
  "stopSimulation\0sendCtrlAltDel\0"
  "enableDebugger"};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_QtHostInterface[] = {

  // content:
  7,      // revision
  0,      // classname
  0, 0,   // classinfo
  13, 14, // methods
  0, 0,   // properties
  0, 0,   // enums/sets
  0, 0,   // constructors
  0,      // flags
  7,      // signalCount

  // signals: name, argc, parameters, tag, flags
  1, 0, 79, 2, 0x06 /* Public */, 3, 0, 80, 2, 0x06 /* Public */, 4, 0, 81, 2, 0x06 /* Public */, 5, 0, 82, 2,
  0x06 /* Public */, 6, 2, 83, 2, 0x06 /* Public */, 9, 1, 88, 2, 0x06 /* Public */, 11, 1, 91, 2, 0x06 /* Public */,

  // slots: name, argc, parameters, tag, flags
  13, 2, 94, 2, 0x0a /* Public */, 16, 1, 99, 2, 0x0a /* Public */, 18, 0, 102, 2, 0x0a /* Public */, 19, 0, 103, 2,
  0x0a /* Public */, 20, 0, 104, 2, 0x0a /* Public */, 21, 1, 105, 2, 0x0a /* Public */,

  // signals: parameters
  QMetaType::Void, QMetaType::Void, QMetaType::Void, QMetaType::Void, QMetaType::Void, QMetaType::Float,
  QMetaType::Float, 7, 8, QMetaType::Void, QMetaType::QString, 10, QMetaType::Void, QMetaType::Bool, 12,

  // slots: parameters
  QMetaType::Void, QMetaType::QString, QMetaType::Bool, 14, 15, QMetaType::Void, QMetaType::Bool, 17, QMetaType::Void,
  QMetaType::Void, QMetaType::Void, QMetaType::Void, QMetaType::Bool, 12,

  0 // eod
};

void QtHostInterface::qt_static_metacall(QObject* _o, QMetaObject::Call _c, int _id, void** _a)
{
  if (_c == QMetaObject::InvokeMetaMethod)
  {
    Q_ASSERT(staticMetaObject.cast(_o));
    QtHostInterface* _t = static_cast<QtHostInterface*>(_o);
    Q_UNUSED(_t)
    switch (_id)
    {
      case 0:
        _t->onSystemInitialized();
        break;
      case 1:
        _t->onSystemDestroy();
        break;
      case 2:
        _t->onSimulationPaused();
        break;
      case 3:
        _t->onSimulationResumed();
        break;
      case 4:
        _t->onSimulationSpeedUpdate((*reinterpret_cast<float(*)>(_a[1])), (*reinterpret_cast<float(*)>(_a[2])));
        break;
      case 5:
        _t->onStatusMessage((*reinterpret_cast<QString(*)>(_a[1])));
        break;
      case 6:
        _t->onDebuggerEnabled((*reinterpret_cast<bool(*)>(_a[1])));
        break;
      case 7:
        _t->startSimulation((*reinterpret_cast<const QString(*)>(_a[1])), (*reinterpret_cast<bool(*)>(_a[2])));
        break;
      case 8:
        _t->pauseSimulation((*reinterpret_cast<bool(*)>(_a[1])));
        break;
      case 9:
        _t->resetSimulation();
        break;
      case 10:
        _t->stopSimulation();
        break;
      case 11:
        _t->sendCtrlAltDel();
        break;
      case 12:
        _t->enableDebugger((*reinterpret_cast<bool(*)>(_a[1])));
        break;
      default:;
    }
  }
  else if (_c == QMetaObject::IndexOfMethod)
  {
    int* result = reinterpret_cast<int*>(_a[0]);
    void** func = reinterpret_cast<void**>(_a[1]);
    {
      typedef void (QtHostInterface::*_t)();
      if (*reinterpret_cast<_t*>(func) == static_cast<_t>(&QtHostInterface::onSystemInitialized))
      {
        *result = 0;
        return;
      }
    }
    {
      typedef void (QtHostInterface::*_t)();
      if (*reinterpret_cast<_t*>(func) == static_cast<_t>(&QtHostInterface::onSystemDestroy))
      {
        *result = 1;
        return;
      }
    }
    {
      typedef void (QtHostInterface::*_t)();
      if (*reinterpret_cast<_t*>(func) == static_cast<_t>(&QtHostInterface::onSimulationPaused))
      {
        *result = 2;
        return;
      }
    }
    {
      typedef void (QtHostInterface::*_t)();
      if (*reinterpret_cast<_t*>(func) == static_cast<_t>(&QtHostInterface::onSimulationResumed))
      {
        *result = 3;
        return;
      }
    }
    {
      typedef void (QtHostInterface::*_t)(float, float);
      if (*reinterpret_cast<_t*>(func) == static_cast<_t>(&QtHostInterface::onSimulationSpeedUpdate))
      {
        *result = 4;
        return;
      }
    }
    {
      typedef void (QtHostInterface::*_t)(QString);
      if (*reinterpret_cast<_t*>(func) == static_cast<_t>(&QtHostInterface::onStatusMessage))
      {
        *result = 5;
        return;
      }
    }
    {
      typedef void (QtHostInterface::*_t)(bool);
      if (*reinterpret_cast<_t*>(func) == static_cast<_t>(&QtHostInterface::onDebuggerEnabled))
      {
        *result = 6;
        return;
      }
    }
  }
}

const QMetaObject QtHostInterface::staticMetaObject = {
  {&QThread::staticMetaObject, qt_meta_stringdata_QtHostInterface.data, qt_meta_data_QtHostInterface,
   qt_static_metacall, Q_NULLPTR, Q_NULLPTR}};

const QMetaObject* QtHostInterface::metaObject() const
{
  return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void* QtHostInterface::qt_metacast(const char* _clname)
{
  if (!_clname)
    return Q_NULLPTR;
  if (!strcmp(_clname, qt_meta_stringdata_QtHostInterface.stringdata0))
    return static_cast<void*>(const_cast<QtHostInterface*>(this));
  if (!strcmp(_clname, "HostInterface"))
    return static_cast<HostInterface*>(const_cast<QtHostInterface*>(this));
  return QThread::qt_metacast(_clname);
}

int QtHostInterface::qt_metacall(QMetaObject::Call _c, int _id, void** _a)
{
  _id = QThread::qt_metacall(_c, _id, _a);
  if (_id < 0)
    return _id;
  if (_c == QMetaObject::InvokeMetaMethod)
  {
    if (_id < 13)
      qt_static_metacall(this, _c, _id, _a);
    _id -= 13;
  }
  else if (_c == QMetaObject::RegisterMethodArgumentMetaType)
  {
    if (_id < 13)
      *reinterpret_cast<int*>(_a[0]) = -1;
    _id -= 13;
  }
  return _id;
}

// SIGNAL 0
void QtHostInterface::onSystemInitialized()
{
  QMetaObject::activate(this, &staticMetaObject, 0, Q_NULLPTR);
}

// SIGNAL 1
void QtHostInterface::onSystemDestroy()
{
  QMetaObject::activate(this, &staticMetaObject, 1, Q_NULLPTR);
}

// SIGNAL 2
void QtHostInterface::onSimulationPaused()
{
  QMetaObject::activate(this, &staticMetaObject, 2, Q_NULLPTR);
}

// SIGNAL 3
void QtHostInterface::onSimulationResumed()
{
  QMetaObject::activate(this, &staticMetaObject, 3, Q_NULLPTR);
}

// SIGNAL 4
void QtHostInterface::onSimulationSpeedUpdate(float _t1, float _t2)
{
  void* _a[] = {Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)),
                const_cast<void*>(reinterpret_cast<const void*>(&_t2))};
  QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void QtHostInterface::onStatusMessage(QString _t1)
{
  void* _a[] = {Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1))};
  QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void QtHostInterface::onDebuggerEnabled(bool _t1)
{
  void* _a[] = {Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1))};
  QMetaObject::activate(this, &staticMetaObject, 6, _a);
}
QT_END_MOC_NAMESPACE
