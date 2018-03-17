#pragma once
#include "YBaseLib/Common.h"
#include "pce/scancodes.h"
#include <QtCore/qnamespace.h>

// Map a QT key to a generic scancode
bool MapQTKeyToGenScanCode(GenScanCode* gen_scancode, Qt::Key key);
