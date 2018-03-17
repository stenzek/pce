#pragma once
#include "YBaseLib/Common.h"
#include "pce/scancodes.h"
#include <SDL_scancode.h>

// Map a SDL scancode to a generic scancode
bool MapSDLScanCode(GenScanCode* gen_scancode, SDL_Scancode sdl_scancode);
