#include "interrupt_controller.h"

DEFINE_OBJECT_TYPE_INFO(InterruptController);

InterruptController::InterruptController(const String& identifier, const ObjectTypeInfo* type_info /*= &s_type_info*/)
  : BaseClass(identifier, type_info)
{
}

InterruptController::~InterruptController() = default;

void InterruptController::TriggerInterrupt(uint32 interrupt)
{
  SetInterruptState(interrupt, true);
  SetInterruptState(interrupt, false);
}

void InterruptController::RaiseInterrupt(uint32 interrupt)
{
  SetInterruptState(interrupt, true);
}

void InterruptController::LowerInterrupt(uint32 interrupt)
{
  SetInterruptState(interrupt, false);
}
