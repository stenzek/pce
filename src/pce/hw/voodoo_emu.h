#pragma once
#include "../types.h"

class Display;
class TimingEvent;
class TimingManager;
struct voodoo_state;

voodoo_state* voodoo_init(int type, Display* display, TimingManager* timing_manager, TimingEvent* retrace_event);
void voodoo_set_pci_enable(voodoo_state* v, u32 value);
void voodoo_set_clock_enable(voodoo_state* v, bool enable);
void voodoo_retrace_event(voodoo_state* v);
void voodoo_shutdown(voodoo_state* v);

u32 voodoo_r(voodoo_state* v, u32 offset);
void voodoo_w(voodoo_state* v, u32 offset, u32 data, u32 mask);