#include "YBaseLib/Assert.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/StringConverter.h"
#include "host_interface.h"
#include "pce/types.h"
#include <SDL.h>
#include <cstdio>

static s32 s_load_save_state_index = -1;

// SDL requires the entry point declared without c++ decoration
#undef main
int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::fprintf(stderr, "Usage: %s <path to system ini> [save state index]\n", argv[0]);
    return -1;
  }

  RegisterAllTypes();

  // set log flags
  // g_pLog->SetConsoleOutputParams(true, nullptr, LOGLEVEL_PROFILE);
  g_pLog->SetConsoleOutputParams(true, "Bus HW::Serial", LOGLEVEL_PROFILE);
  // g_pLog->SetConsoleOutputParams(true, "CPU_X86::CPU Bus HW::Serial", LOGLEVEL_PROFILE);

#ifdef Y_BUILD_CONFIG_RELEASE
  g_pLog->SetFilterLevel(LOGLEVEL_INFO);
#else
  g_pLog->SetFilterLevel(LOGLEVEL_PROFILE);
#endif

  // init sdl
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
  {
    Panic("SDL initialization failed");
    return -1;
  }

  // create display and host interface
  std::unique_ptr<SDLHostInterface> host_interface = SDLHostInterface::Create();
  if (!host_interface)
  {
    Panic("Failed to create host interface");
    SDL_Quit();
    return -1;
  }

  // create system
  if (argc > 2)
    s_load_save_state_index = StringConverter::StringToInt32(argv[2]);
  if (!host_interface->CreateSystem(argv[1], s_load_save_state_index))
  {
    host_interface.reset();
    SDL_Quit();
    return -1;
  }

  // run
  host_interface->Run();

  // done
  host_interface.reset();
  SDL_Quit();
  return 0;
}
