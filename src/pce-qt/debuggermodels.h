#pragma once
#include "pce/debugger_interface.h"
#include <QtCore/QAbstractListModel>
#include <QtCore/QAbstractTableModel>
#include <map>

class DebuggerCodeModel : public QAbstractTableModel
{
public:
  DebuggerCodeModel(DebuggerInterface* intf, QObject* parent = nullptr);
  virtual ~DebuggerCodeModel();

  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  // Returns the row for this instruction pointer
  void resetCodeView(LinearMemoryAddress start_address);
  int getRowForAddress(LinearMemoryAddress address);
  bool getAddressForRow(LinearMemoryAddress* address, int row) const;

  int updateInstructionPointer();

private:
  DebuggerInterface* m_interface;

  LinearMemoryAddress m_start_instruction_pointer = 0;
  LinearMemoryAddress m_last_instruction_pointer = 0;
  mutable std::map<int, LinearMemoryAddress> m_row_ips;
};

class DebuggerRegistersModel : public QAbstractListModel
{
public:
  DebuggerRegistersModel(DebuggerInterface* intf, QObject* parent = nullptr);
  virtual ~DebuggerRegistersModel();

  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  void invalidateView();

private:
  DebuggerInterface* m_interface;
};

class DebuggerStackModel : public QAbstractListModel
{
public:
  DebuggerStackModel(DebuggerInterface* intf, QObject* parent = nullptr);
  virtual ~DebuggerStackModel();

  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  void invalidateView();

private:
  DebuggerInterface* m_interface;
};
