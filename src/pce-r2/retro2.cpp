#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Error.h"

#include "common/log.h"

#include "pce/host_interface.h"
#include "pce/system.h"
#include "pce/types.h"

#include <retro2/retro2_core.h>
#include <retro2/retro2_core_exports.h>
#include <retro2/retro2_error.h>
#include <retro2/retro2_framebuffer.h>
#include <retro2/retro2_interface_loader.h>
#include <retro2/retro2_log.h>
#include <retro2/retro2_session.h>
#include <retro2/retro2_settings.h>

IRetro2Error R2Error;
IRetro2Log R2Log;
IRetro2Session R2Session;
IRetro2Settings R2Settings;
IRetro2FramebufferVideo R2FramebufferVideo;

static HostInterface s_host_interface;

#define SYSTEM static_cast<System*>(userdata)

static size_t R2CoreSessionGetMaxStateBufferSize(void*)
{
  return 128 * 1024 * 1024;
}

static bool R2CoreSessionLoadStateFromBuffer(void* userdata, const uint8_t* buffer, size_t buffer_size,
                                             uint32_t data_version, Retro2Error* error)
{
  ReadOnlyMemoryByteStream* stream = new ReadOnlyMemoryByteStream(buffer, static_cast<u32>(buffer_size));
  if (!SYSTEM->LoadState(stream))
  {
    R2Error.SetErrorString(error, "LoadState() failed");
    stream->Release();
    return false;
  }

  stream->Release();
  return true;
}

static bool R2CoreSessionSaveStateToBuffer(void* userdata, uint8_t* buffer, size_t buffer_size, size_t* written_size,
                                           uint32_t* data_version, Retro2Error* error)
{
  MemoryByteStream* stream = new MemoryByteStream(buffer, static_cast<u32>(buffer_size));
  if (!SYSTEM->SaveState(stream))
  {
    R2Error.SetErrorString(error, "SaveState() failed");
    stream->Release();
    return false;
  }

  *data_version = 1;
  *written_size = stream->GetPosition();
  stream->Release();
  return true;
}

static void R2CoreSessionSettingsChanged(void* userdata)
{
  // SYSTEM->UpdateSettings();
}
static bool R2CoreSessionProviderStartSessionFromPath(const Retro2StartSessionParams* params, const char* path,
                                                      PFN_Retro2SetCoreSessionInterface set_core_session,
                                                      Retro2Error* error)
{
  Error lerror;
  if (!s_host_interface.CreateSystem(path, &lerror))
  {
    R2Error.SetErrorString(error, lerror.GetErrorDescription());
    return false;
  }

  s_host_interface.SetCPUBackend(CPU::BackendType::CachedInterpreter);

  IRetro2CoreSession core_session = {};
  core_session.Shutdown = [](void* userdata) { s_host_interface.ShutdownSystem(); };
  core_session.Reset = [](void* userdata) { SYSTEM->Reset(); };
  core_session.SettingsChanged = R2CoreSessionSettingsChanged;
  core_session.SetControllerInputValue = [](void* userdata, uint32_t controller_index, uint32_t input_index,
                                            float value) {
    SYSTEM->ReceiveHostInputValue(controller_index, input_index, value);
  };
  core_session.RunLoop = [](void* userdata) { SYSTEM->Run(); };
  core_session.GetMaxStateBufferSize = R2CoreSessionGetMaxStateBufferSize;
  core_session.LoadStateFromBuffer = R2CoreSessionLoadStateFromBuffer;
  core_session.SaveStateToBuffer = R2CoreSessionSaveStateToBuffer;

  if (!set_core_session(&core_session, RETRO2_CORE_SESSION_INTERFACE_VERSION, s_host_interface.GetSystem(), "pc",
                        RETRO2_SESSION_RUN_MODE_RUNLOOP, error))
  {
    return false;
  }

  // system->SetMemoryMap();
  return true;
}

bool Retro2GetCoreSessionProvider(IRetro2CoreSessionProvider* prov, Retro2Error* error)
{
  static constexpr IRetro2CoreSessionProvider our_prov = {
    nullptr,                                   // Initialize
    nullptr,                                   // Shutdown
    R2CoreSessionProviderStartSessionFromPath, // StartSessionFromPath
    nullptr,                                   // StartSessionFromMemory
    nullptr,                                   // StartSessionWithoutContent
  };

  std::memcpy(prov, &our_prov, sizeof(our_prov));
  return true;
}

bool Retro2CoreInit(const Retro2CoreInitParams* params, Retro2CoreInitInfo* info, Retro2Error* error)
{
  R2Error = *params->IError;

  static constexpr const Retro2InterfaceLoadEntry s_load_entries[] = {
    // clang-format off
    RETRO2_INTERFACE_LOAD_ENTRY(R2Log, LOG),
    RETRO2_INTERFACE_LOAD_ENTRY(R2Session, SESSION),
    // RETRO2_INTERFACE_LOAD_ENTRY(R2Settings, SETTINGS),
    // clang-format on
  };
  if (!Retro2LoadInterfaces(params, s_load_entries, std::size(s_load_entries), error))
    return false;

  Log::Initialize();

  info->CoreName = "r2pce";
  info->MinimumSaveStateVersion = 1;
  info->MaximumSaveStateVersion = 1;

  RegisterAllTypes();

  return true;
}

void Retro2CoreShutdown() {}
