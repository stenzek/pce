#pragma once
#include "pce/component.h"
#include <functional>

class CPUBase;

class InterruptController : public Component
{
  DECLARE_OBJECT_TYPE_INFO(InterruptController, Component);
  DECLARE_OBJECT_NO_FACTORY(InterruptController);
  DECLARE_OBJECT_NO_PROPERTIES(InterruptController);

public:
  InterruptController(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  virtual ~InterruptController();

  // Request the index of the highest priority awaiting interrupt.
  // Acknowledges the interrupt has been received.
  virtual uint32 GetInterruptNumber() = 0;

  // Request an interrupt with the specified vector number
  // This is a edge-triggered interrupt, it is only executed once
  virtual void TriggerInterrupt(uint32 interrupt);

  // Sets the interrupt line high/low.
  virtual void SetInterruptState(uint32 interrupt, bool active) = 0;

  // Raise/lower the interrupt line for the specified vector number
  // This is level-triggered, so can be executed more than once
  void RaiseInterrupt(uint32 interrupt);
  void LowerInterrupt(uint32 interrupt);
};
