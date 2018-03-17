#pragma once
#include "pce/component.h"
#include <functional>

class CPUBase;

class InterruptController : public Component
{
public:
  virtual ~InterruptController() {}

  // Request the index of the highest priority awaiting interrupt
  virtual uint32 GetPendingInterruptNumber() const = 0;

  // Request an interrupt with the specified vector number
  // This is a edge-triggered interrupt, it is only executed once
  virtual void TriggerInterrupt(uint32 interrupt) = 0;

  // Raise/lower the interrupt line for the specified vector number
  // This is level-triggered, so can be executed more than once
  virtual void RaiseInterrupt(uint32 interrupt) = 0;
  virtual void LowerInterrupt(uint32 interrupt) = 0;

  // Inform the interrupt controller that the specified interrupt has been acknowledged
  virtual void AcknowledgeInterrupt(uint32 interrupt) = 0;

  // Inform the interrupt controller that the specified interrupt has been serviced
  virtual void InterruptServiced(uint32 interrupt) = 0;
};
