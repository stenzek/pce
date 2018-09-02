#include "dma_controller.h"

DEFINE_OBJECT_TYPE_INFO(DMAController);

DMAController::DMAController(const String& identifier, const ObjectTypeInfo* type_info /*= &s_type_info*/)
  : BaseClass(identifier, type_info)
{
}

DMAController::~DMAController() = default;
