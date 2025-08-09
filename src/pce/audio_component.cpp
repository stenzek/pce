#include "audio_component.h"

DEFINE_OBJECT_TYPE_INFO(AudioComponent);

AudioComponent::AudioComponent(const String& identifier, const ObjectTypeInfo* type_info /*= &s_type_info*/)
  : BaseClass(identifier, type_info)
{
}

AudioComponent::~AudioComponent() = default;
