#pragma once
#include "pce/component.h"
#include <functional>

class CPU;

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
  virtual u32 GetInterruptNumber() = 0;

  // Request an interrupt with the specified vector number
  // This is a edge-triggered interrupt, it is only executed once
  virtual void TriggerInterrupt(u32 interrupt);

  // Sets the interrupt line high/low.
  virtual void SetInterruptState(u32 interrupt, bool active) = 0;

  // Raise/lower the interrupt line for the specified vector number
  // This is level-triggered, so can be executed more than once
  void RaiseInterrupt(u32 interrupt);
  void LowerInterrupt(u32 interrupt);
};
