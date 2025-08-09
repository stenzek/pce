#pragma once

#include "component.h"

class AudioComponent : public Component
{
  DECLARE_OBJECT_TYPE_INFO(AudioComponent, Component);
  DECLARE_OBJECT_NO_FACTORY(AudioComponent);
  DECLARE_OBJECT_NO_PROPERTIES(AudioComponent);

public:
  AudioComponent(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  virtual ~AudioComponent();

  /// Synchronize audio output and generate all pending audio samples.
  virtual void SynchronizeAudioOutput() = 0;
};
