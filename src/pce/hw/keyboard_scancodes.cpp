#include "pce/hw/keyboard_scancodes.h"

namespace HW {

const uint8 KeyboardScanCodes::Set1Mapping[NumGenScanCodes][2][MAX_SCAN_CODE_LENGTH]{
  // Make code  break code
  {{0x00}, {0x00}},                                     // GenScanCode_None
  {{0x01}, {0x81}},                                     // GenScanCode_Escape
  {{0x3B}, {0xBB}},                                     // GenScanCode_F1
  {{0x3C}, {0xBC}},                                     // GenScanCode_F2
  {{0x3D}, {0xBD}},                                     // GenScanCode_F3
  {{0x3E}, {0xBE}},                                     // GenScanCode_F4
  {{0x3F}, {0xBF}},                                     // GenScanCode_F5
  {{0x40}, {0xC0}},                                     // GenScanCode_F6
  {{0x41}, {0xC1}},                                     // GenScanCode_F7
  {{0x42}, {0xC2}},                                     // GenScanCode_F8
  {{0x43}, {0xC3}},                                     // GenScanCode_F9
  {{0x44}, {0xC4}},                                     // GenScanCode_F10
  {{0x57}, {0xD7}},                                     // GenScanCode_F11
  {{0x58}, {0xD8}},                                     // GenScanCode_F12
  {{0x29}, {0xA9}},                                     // GenScanCode_Backtick
  {{0x02}, {0x82}},                                     // GenScanCode_1
  {{0x03}, {0x83}},                                     // GenScanCode_2
  {{0x04}, {0x84}},                                     // GenScanCode_3
  {{0x05}, {0x85}},                                     // GenScanCode_4
  {{0x06}, {0x86}},                                     // GenScanCode_5
  {{0x07}, {0x87}},                                     // GenScanCode_6
  {{0x08}, {0x88}},                                     // GenScanCode_7
  {{0x09}, {0x89}},                                     // GenScanCode_8
  {{0x0A}, {0x8A}},                                     // GenScanCode_9
  {{0x0B}, {0x8B}},                                     // GenScanCode_0
  {{0x0C}, {0x8C}},                                     // GenScanCode_Minus
  {{0x0D}, {0x8D}},                                     // GenScanCode_Equals
  {{0x0E}, {0x8E}},                                     // GenScanCode_Backspace
  {{0x0F}, {0x8F}},                                     // GenScanCode_Tab
  {{0x10}, {0x90}},                                     // GenScanCode_Q
  {{0x11}, {0x91}},                                     // GenScanCode_W
  {{0x12}, {0x92}},                                     // GenScanCode_E
  {{0x13}, {0x93}},                                     // GenScanCode_R
  {{0x14}, {0x94}},                                     // GenScanCode_T
  {{0x15}, {0x95}},                                     // GenScanCode_Y
  {{0x16}, {0x96}},                                     // GenScanCode_U
  {{0x17}, {0x97}},                                     // GenScanCode_I
  {{0x18}, {0x98}},                                     // GenScanCode_O
  {{0x19}, {0x99}},                                     // GenScanCode_P
  {{0x1A}, {0x9A}},                                     // GenScanCode_LeftBracket
  {{0x1B}, {0x9B}},                                     // GenScanCode_RightBracket
  {{0x2B}, {0xAB}},                                     // GenScanCode_Backslash
  {{0x3A}, {0xBA}},                                     // GenScanCode_CapsLock
  {{0x1E}, {0x9E}},                                     // GenScanCode_A
  {{0x1F}, {0x9F}},                                     // GenScanCode_S
  {{0x20}, {0xA0}},                                     // GenScanCode_D
  {{0x21}, {0xA1}},                                     // GenScanCode_F
  {{0x22}, {0xA2}},                                     // GenScanCode_G
  {{0x23}, {0xA3}},                                     // GenScanCode_H
  {{0x24}, {0xA4}},                                     // GenScanCode_J
  {{0x25}, {0xA5}},                                     // GenScanCode_K
  {{0x26}, {0xA6}},                                     // GenScanCode_L
  {{0x27}, {0xA7}},                                     // GenScanCode_Semicolon
  {{0x28}, {0xA8}},                                     // GenScanCode_Quote
  {{0x1C}, {0x9C}},                                     // GenScanCode_Return
  {{0x2A}, {0xAA}},                                     // GenScanCode_LeftShift
  {{0x2C}, {0xAC}},                                     // GenScanCode_Z
  {{0x2D}, {0xAD}},                                     // GenScanCode_X
  {{0x2E}, {0xAE}},                                     // GenScanCode_C
  {{0x2F}, {0xAF}},                                     // GenScanCode_V
  {{0x30}, {0xB0}},                                     // GenScanCode_B
  {{0x31}, {0xB1}},                                     // GenScanCode_N
  {{0x32}, {0xB2}},                                     // GenScanCode_M
  {{0x33}, {0xB3}},                                     // GenScanCode_Comma
  {{0x34}, {0xB4}},                                     // GenScanCode_Period
  {{0x35}, {0xB5}},                                     // GenScanCode_Slash
  {{0x36}, {0xB6}},                                     // GenScanCode_RightShift
  {{0x1D}, {0x9D}},                                     // GenScanCode_LeftControl
  {{0xE0, 0x5B}, {0xE0, 0xDB}},                         // GenScanCode_LeftSuper
  {{0x38}, {0xB8}},                                     // GenScanCode_LeftAlt
  {{0x39}, {0xB9}},                                     // GenScanCode_Space
  {{0xE0, 0x38}, {0xE0, 0xB8}},                         // GenScanCode_RightAlt
  {{0xE0, 0x5C}, {0xE0, 0xDC}},                         // GenScanCode_RightSuper
  {{0xE0, 0x5D}, {0xE0, 0xDD}},                         // GenScanCode_RightMenu
  {{0xE0, 0x1D}, {0xE0, 0x9D}},                         // GenScanCode_RightControl
  {{0xE0, 0x2A, 0xE0, 0x37}, {0xE0, 0xB7, 0xE0, 0xAA}}, // GenScanCode_PrintScreen
  {{0x46}, {0xC6}},                                     // GenScanCode_ScrollLock
  {{0xE1, 0x1D, 0x45, 0xE9, 0x9D, 0xC5}, {}},           // GenScanCode_PauseBreak
  {{0xE0, 0x52}, {0xE0, 0xD2}},                         // GenScanCode_Insert
  {{0xE0, 0x47}, {0xE0, 0x97}},                         // GenScanCode_Home
  {{0xE0, 0x49}, {0xE0, 0xC9}},                         // GenScanCode_PageUp
  {{0xE0, 0x53}, {0xE0, 0xD3}},                         // GenScanCode_Delete
  {{0xE0, 0x4F}, {0xE0, 0xCF}},                         // GenScanCode_End
  {{0xE0, 0x51}, {0xE0, 0xD1}},                         // GenScanCode_PageDown
  {{0xE0, 0x48}, {0xE0, 0xC8}},                         // GenScanCode_Up
  {{0xE0, 0x4B}, {0xE0, 0xCB}},                         // GenScanCode_Left
  {{0xE0, 0x50}, {0xE0, 0xD0}},                         // GenScanCode_Down
  {{0xE0, 0x4D}, {0xE0, 0xCD}},                         // GenScanCode_Right
  {{0x45}, {0xC5}},                                     // GenScanCode_Numpad_NumLock
  {{0xE0, 0x35}, {0xE0, 0xB5}},                         // GenScanCode_Numpad_Divide
  {{0x37}, {0xB7}},                                     // GenScanCode_Numpad_Multiply
  {{0x4A}, {0xCA}},                                     // GenScanCode_Numpad_Minus
  {{0x4E}, {0xCE}},                                     // GenScanCode_Numpad_Plus
  {{0xE0, 0x1C}, {0xE0, 0x9C}},                         // GenScanCode_Numpad_Enter
  {{0x4F}, {0xCF}},                                     // GenScanCode_Numpad_1
  {{0x50}, {0xD0}},                                     // GenScanCode_Numpad_2
  {{0x51}, {0xD1}},                                     // GenScanCode_Numpad_3
  {{0x4B}, {0xCB}},                                     // GenScanCode_Numpad_4
  {{0x4C}, {0xCC}},                                     // GenScanCode_Numpad_5
  {{0x4D}, {0xCD}},                                     // GenScanCode_Numpad_6
  {{0x47}, {0xC7}},                                     // GenScanCode_Numpad_7
  {{0x48}, {0xC8}},                                     // GenScanCode_Numpad_8
  {{0x49}, {0xC9}},                                     // GenScanCode_Numpad_9
  {{0x52}, {0xD2}},                                     // GenScanCode_Numpad_0
  {{0x53}, {0xD3}}                                      // GenScanCode_Numpad_Decimal
};

const uint8 KeyboardScanCodes::Set2Mapping[NumGenScanCodes][2][MAX_SCAN_CODE_LENGTH]{
  // Make code  break code
  {{0x00}, {0x00}},                                                 // GenScanCode_None
  {{0x76}, {0xF0, 0x76}},                                           // GenScanCode_Escape
  {{0x05}, {0xF0, 0x05}},                                           // GenScanCode_F1
  {{0x06}, {0xF0, 0x06}},                                           // GenScanCode_F2
  {{0x04}, {0xF0, 0x04}},                                           // GenScanCode_F3
  {{0x0C}, {0xF0, 0x0C}},                                           // GenScanCode_F4
  {{0x03}, {0xF0, 0x03}},                                           // GenScanCode_F5
  {{0x0B}, {0xF0, 0x0B}},                                           // GenScanCode_F6
  {{0x83}, {0xF0, 0x83}},                                           // GenScanCode_F7
  {{0x0A}, {0xF0, 0x0A}},                                           // GenScanCode_F8
  {{0x01}, {0xF0, 0x01}},                                           // GenScanCode_F9
  {{0x09}, {0xF0, 0x09}},                                           // GenScanCode_F10
  {{0x78}, {0xF0, 0x78}},                                           // GenScanCode_F11
  {{0x07}, {0xF0, 0x07}},                                           // GenScanCode_F12
  {{0x0E}, {0xF0, 0x0E}},                                           // GenScanCode_Backtick
  {{0x16}, {0xF0, 0x16}},                                           // GenScanCode_1
  {{0x1E}, {0xF0, 0x1E}},                                           // GenScanCode_2
  {{0x26}, {0xF0, 0x26}},                                           // GenScanCode_3
  {{0x25}, {0xF0, 0x25}},                                           // GenScanCode_4
  {{0x2E}, {0xF0, 0x2E}},                                           // GenScanCode_5
  {{0x36}, {0xF0, 0x36}},                                           // GenScanCode_6
  {{0x3D}, {0xF0, 0x3D}},                                           // GenScanCode_7
  {{0x3E}, {0xF0, 0x3E}},                                           // GenScanCode_8
  {{0x46}, {0xF0, 0x46}},                                           // GenScanCode_9
  {{0x45}, {0xF0, 0x45}},                                           // GenScanCode_0
  {{0x4E}, {0xF0, 0x4E}},                                           // GenScanCode_Minus
  {{0x55}, {0xF0, 0x55}},                                           // GenScanCode_Equals
  {{0x66}, {0xF0, 0x66}},                                           // GenScanCode_Backspace
  {{0x0D}, {0xF0, 0x0D}},                                           // GenScanCode_Tab
  {{0x15}, {0xF0, 0x15}},                                           // GenScanCode_Q
  {{0x1D}, {0xF0, 0x1D}},                                           // GenScanCode_W
  {{0x24}, {0xF0, 0x24}},                                           // GenScanCode_E
  {{0x2D}, {0xF0, 0x2D}},                                           // GenScanCode_R
  {{0x2C}, {0xF0, 0x2C}},                                           // GenScanCode_T
  {{0x35}, {0xF0, 0x35}},                                           // GenScanCode_Y
  {{0x3C}, {0xF0, 0x3C}},                                           // GenScanCode_U
  {{0x43}, {0xF0, 0x43}},                                           // GenScanCode_I
  {{0x44}, {0xF0, 0x44}},                                           // GenScanCode_O
  {{0x4D}, {0xF0, 0x4D}},                                           // GenScanCode_P
  {{0x54}, {0xF0, 0x54}},                                           // GenScanCode_LeftBrackeT
  {{0x5B}, {0xF0, 0x5B}},                                           // GenScanCode_RightBracket
  {{0x5D}, {0xF0, 0x5D}},                                           // GenScanCode_Backslash
  {{0x58}, {0xF0, 0x58}},                                           // GenScanCode_CapsLock
  {{0x1C}, {0xF0, 0x1C}},                                           // GenScanCode_A
  {{0x1B}, {0xF0, 0x1B}},                                           // GenScanCode_S
  {{0x23}, {0xF0, 0x23}},                                           // GenScanCode_D
  {{0x2B}, {0xF0, 0x2B}},                                           // GenScanCode_F
  {{0x34}, {0xF0, 0x34}},                                           // GenScanCode_G
  {{0x33}, {0xF0, 0x33}},                                           // GenScanCode_H
  {{0x3B}, {0xF0, 0x3B}},                                           // GenScanCode_J
  {{0x42}, {0xF0, 0x42}},                                           // GenScanCode_K
  {{0x4B}, {0xF0, 0x4B}},                                           // GenScanCode_L
  {{0x4C}, {0xF0, 0x4C}},                                           // GenScanCode_Semicolon
  {{0x52}, {0xF0, 0x52}},                                           // GenScanCode_Quote
  {{0x5A}, {0xF0, 0x5A}},                                           // GenScanCode_Return
  {{0x12}, {0xF0, 0x12}},                                           // GenScanCode_LeftShift
  {{0x1A}, {0xF0, 0x1A}},                                           // GenScanCode_Z
  {{0x22}, {0xF0, 0x22}},                                           // GenScanCode_X
  {{0x21}, {0xF0, 0x21}},                                           // GenScanCode_C
  {{0x2A}, {0xF0, 0x2A}},                                           // GenScanCode_V
  {{0x32}, {0xF0, 0x32}},                                           // GenScanCode_B
  {{0x31}, {0xF0, 0x31}},                                           // GenScanCode_N
  {{0x3A}, {0xF0, 0x3A}},                                           // GenScanCode_M
  {{0x41}, {0xF0, 0x41}},                                           // GenScanCode_Comma
  {{0x49}, {0xF0, 0x49}},                                           // GenScanCode_Period
  {{0x4A}, {0xF0, 0x4A}},                                           // GenScanCode_Slash
  {{0x59}, {0xF0, 0x59}},                                           // GenScanCode_RightShift
  {{0x14}, {0xF0, 0x14}},                                           // GenScanCode_LeftControl
  {{0xE0, 0x1F}, {0xE0, 0xF0, 0x1F}},                               // GenScanCode_LeftSuper
  {{0x11}, {0xF0, 0x11}},                                           // GenScanCode_LeftAlt
  {{0x29}, {0xF0, 0x29}},                                           // GenScanCode_Space
  {{0xE0, 0x11}, {0xE0, 0xF0, 0x11}},                               // GenScanCode_RightAlt
  {{0xE0, 0x27}, {0xE0, 0xF0, 0x27}},                               // GenScanCode_RightSuper
  {{0xE0, 0x2F}, {0xE0, 0xF0, 0x2F}},                               // GenScanCode_RightMenu
  {{0xE0, 0x14}, {0xE0, 0xF0, 0x14}},                               // GenScanCode_RightControl
  {{0xE0, 0x12, 0xE0, 0x7C}, {0xE0, 0xF0, 0x7C, 0xE0, 0xF0, 0x12}}, // GenScanCode_PrintScreen
  {{0x7E}, {0xF0, 0x7E}},                                           // GenScanCode_ScrollLock
  {{0xE1, 0x14, 0x77, 0xE1, 0xF0, 0x14, 0xE0, 0x77}, {}},           // GenScanCode_PauseBreak
  {{0xE0, 0x70}, {0xE0, 0xF0, 0x70}},                               // GenScanCode_Insert
  {{0xE0, 0x5C}, {0xE0, 0xF0, 0x5C}},                               // GenScanCode_Home
  {{0xE0, 0x7D}, {0xE0, 0xF0, 0x7D}},                               // GenScanCode_PageUp
  {{0xE0, 0x71}, {0xE0, 0xF0, 0x71}},                               // GenScanCode_Delete
  {{0xE0, 0x69}, {0xE0, 0xF0, 0x69}},                               // GenScanCode_End
  {{0xE0, 0x7A}, {0xE0, 0xF0, 0x7A}},                               // GenScanCode_PageDown
  {{0xE0, 0x75}, {0xE0, 0xF0, 0x75}},                               // GenScanCode_Up
  {{0xE0, 0x6B}, {0xE0, 0xF0, 0x6B}},                               // GenScanCode_Left
  {{0xE0, 0x72}, {0xE0, 0xF0, 0x72}},                               // GenScanCode_Down
  {{0xE0, 0x74}, {0xE0, 0xF0, 0x74}},                               // GenScanCode_Right
  {{0x77}, {0xF0, 0x74}},                                           // GenScanCode_Numpad_NumLock
  {{0xE0, 0x4A}, {0xE0, 0xF0, 0x4A}},                               // GenScanCode_Numpad_Divide
  {{0x7C}, {0xF0, 0x7C}},                                           // GenScanCode_Numpad_Multiply
  {{0x7B}, {0xF0, 0x7B}},                                           // GenScanCode_Numpad_Minus
  {{0x79}, {0xF0, 0x79}},                                           // GenScanCode_Numpad_Plus
  {{0xE0, 0x5A}, {0xE0, 0xF0, 0x5A}},                               // GenScanCode_Numpad_Enter
  {{0x69}, {0xF0, 0x69}},                                           // GenScanCode_Numpad_1
  {{0x72}, {0xF0, 0x72}},                                           // GenScanCode_Numpad_2
  {{0x7A}, {0xF0, 0x7A}},                                           // GenScanCode_Numpad_3
  {{0x7B}, {0xF0, 0x6B}},                                           // GenScanCode_Numpad_4
  {{0x73}, {0xF0, 0x73}},                                           // GenScanCode_Numpad_5
  {{0x74}, {0xF0, 0x74}},                                           // GenScanCode_Numpad_6
  {{0x6C}, {0xF0, 0x6C}},                                           // GenScanCode_Numpad_7
  {{0x75}, {0xF0, 0x75}},                                           // GenScanCode_Numpad_8
  {{0x7D}, {0xF0, 0x7D}},                                           // GenScanCode_Numpad_9
  {{0x70}, {0xF0, 0x70}},                                           // GenScanCode_Numpad_0
  {{0x71}, {0xF0, 0x71}}                                            // GenScanCode_Numpad_Decimal
};

} // namespace HW