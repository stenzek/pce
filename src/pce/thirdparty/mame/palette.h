// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/******************************************************************************

    palette.h

    Core palette routines.

***************************************************************************/
#pragma once
#include "../../types.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// an rgb15_t is a single combined 15-bit R,G,B value
typedef uint16_t rgb15_t;

// ======================> rgb_t

// an rgb_t is a single combined R,G,B (and optionally alpha) value
class rgb_t
{
public:
  // construction/destruction
  constexpr rgb_t() : m_data(0) {}
  constexpr rgb_t(u32 data) : m_data(data) {}
  constexpr rgb_t(u8 r, u8 g, u8 b) : m_data((255 << 24) | (r << 16) | (g << 8) | b) {}
  constexpr rgb_t(u8 a, u8 r, u8 g, u8 b) : m_data((a << 24) | (r << 16) | (g << 8) | b) {}

  // getters
  constexpr u8 a() const { return m_data >> 24; }
  constexpr u8 r() const { return m_data >> 16; }
  constexpr u8 g() const { return m_data >> 8; }
  constexpr u8 b() const { return m_data >> 0; }
  constexpr rgb15_t as_rgb15() const { return ((r() >> 3) << 10) | ((g() >> 3) << 5) | ((b() >> 3) << 0); }
  constexpr u8 brightness() const { return (r() * 222 + g() * 707 + b() * 71) / 1000; }
  constexpr u32 const* ptr() const { return &m_data; }
  void expand_rgb(u8& r, u8& g, u8& b) const
  {
    r = m_data >> 16;
    g = m_data >> 8;
    b = m_data >> 0;
  }
  void expand_rgb(int& r, int& g, int& b) const
  {
    r = (m_data >> 16) & 0xff;
    g = (m_data >> 8) & 0xff;
    b = (m_data >> 0) & 0xff;
  }

  // setters
  rgb_t& set_a(u8 a)
  {
    m_data &= ~0xff000000;
    m_data |= a << 24;
    return *this;
  }
  rgb_t& set_r(u8 r)
  {
    m_data &= ~0x00ff0000;
    m_data |= r << 16;
    return *this;
  }
  rgb_t& set_g(u8 g)
  {
    m_data &= ~0x0000ff00;
    m_data |= g << 8;
    return *this;
  }
  rgb_t& set_b(u8 b)
  {
    m_data &= ~0x000000ff;
    m_data |= b << 0;
    return *this;
  }

  // implicit conversion operators
  constexpr operator u32() const { return m_data; }

  // operations
  rgb_t& scale8(u8 scale)
  {
    m_data = rgb_t(clamphi((a() * scale) >> 8), clamphi((r() * scale) >> 8), clamphi((g() * scale) >> 8),
                   clamphi((b() * scale) >> 8));
    return *this;
  }

  // assignment operators
  rgb_t& operator=(u32 rhs)
  {
    m_data = rhs;
    return *this;
  }
  rgb_t& operator+=(const rgb_t& rhs)
  {
    m_data = u32(*this + rhs);
    return *this;
  }
  rgb_t& operator-=(const rgb_t& rhs)
  {
    m_data = u32(*this - rhs);
    return *this;
  }

  // arithmetic operators
  constexpr rgb_t operator+(const rgb_t& rhs) const
  {
    return rgb_t(clamphi(a() + rhs.a()), clamphi(r() + rhs.r()), clamphi(g() + rhs.g()), clamphi(b() + rhs.b()));
  }
  constexpr rgb_t operator-(const rgb_t& rhs) const
  {
    return rgb_t(clamplo(a() - rhs.a()), clamplo(r() - rhs.r()), clamplo(g() - rhs.g()), clamplo(b() - rhs.b()));
  }

  // static helpers
  static constexpr u8 clamp(s32 value) { return (value < 0) ? 0 : (value > 255) ? 255 : value; }
  static constexpr u8 clamphi(s32 value) { return (value > 255) ? 255 : value; }
  static constexpr u8 clamplo(s32 value) { return (value < 0) ? 0 : value; }

  // constant factories
  static constexpr rgb_t black() { return rgb_t(0, 0, 0); }
  static constexpr rgb_t white() { return rgb_t(255, 255, 255); }
  static constexpr rgb_t green() { return rgb_t(0, 255, 0); }
  static constexpr rgb_t amber() { return rgb_t(247, 170, 0); }
  static constexpr rgb_t transparent() { return rgb_t(0, 0, 0, 0); }

private:
  u32 m_data;
};

//**************************************************************************
//  INLINE FUNCTIONS
//**************************************************************************

//-------------------------------------------------
//  palexpand - expand a palette value to 8 bits
//-------------------------------------------------

template<int _NumBits>
inline uint8_t palexpand(uint8_t bits)
{
  if constexpr (_NumBits == 1)
  {
    return (bits & 1) ? 0xff : 0x00;
  }
  else if constexpr (_NumBits == 2)
  {
    bits &= 3;
    return (bits << 6) | (bits << 4) | (bits << 2) | bits;
  }
  else if constexpr (_NumBits == 3)
  {
    bits &= 7;
    return (bits << 5) | (bits << 2) | (bits >> 1);
  }
  else if constexpr (_NumBits == 4)
  {
    bits &= 0xf;
    return (bits << 4) | bits;
  }
  else if constexpr (_NumBits == 5)
  {
    bits &= 0x1f;
    return (bits << 3) | (bits >> 2);
  }
  else if constexpr (_NumBits == 6)
  {
    bits &= 0x3f;
    return (bits << 2) | (bits >> 4);
  }
  else if constexpr (_NumBits == 7)
  {
    bits &= 0x7f;
    return (bits << 1) | (bits >> 6);
  }
  else
  {
    return bits;
  }
}

//-------------------------------------------------
//  palxbit - convert an x-bit value to 8 bits
//-------------------------------------------------

inline uint8_t pal1bit(uint8_t bits)
{
  return palexpand<1>(bits);
}
inline uint8_t pal2bit(uint8_t bits)
{
  return palexpand<2>(bits);
}
inline uint8_t pal3bit(uint8_t bits)
{
  return palexpand<3>(bits);
}
inline uint8_t pal4bit(uint8_t bits)
{
  return palexpand<4>(bits);
}
inline uint8_t pal5bit(uint8_t bits)
{
  return palexpand<5>(bits);
}
inline uint8_t pal6bit(uint8_t bits)
{
  return palexpand<6>(bits);
}
inline uint8_t pal7bit(uint8_t bits)
{
  return palexpand<7>(bits);
}

//-------------------------------------------------
//  rgbexpand - expand a 32-bit raw data to 8-bit
//  RGB
//-------------------------------------------------

template<int _RBits, int _GBits, int _BBits>
inline rgb_t rgbexpand(uint32_t data, uint8_t rshift, uint8_t gshift, uint8_t bshift)
{
  return rgb_t(palexpand<_RBits>(data >> rshift), palexpand<_GBits>(data >> gshift), palexpand<_BBits>(data >> bshift));
}

//-------------------------------------------------
//  palxxx - create an x-x-x color by extracting
//  bits from a uint32_t
//-------------------------------------------------

inline rgb_t pal332(uint32_t data, uint8_t rshift, uint8_t gshift, uint8_t bshift)
{
  return rgbexpand<3, 3, 2>(data, rshift, gshift, bshift);
}
inline rgb_t pal444(uint32_t data, uint8_t rshift, uint8_t gshift, uint8_t bshift)
{
  return rgbexpand<4, 4, 4>(data, rshift, gshift, bshift);
}
inline rgb_t pal555(uint32_t data, uint8_t rshift, uint8_t gshift, uint8_t bshift)
{
  return rgbexpand<5, 5, 5>(data, rshift, gshift, bshift);
}
inline rgb_t pal565(uint32_t data, uint8_t rshift, uint8_t gshift, uint8_t bshift)
{
  return rgbexpand<5, 6, 5>(data, rshift, gshift, bshift);
}
inline rgb_t pal888(uint32_t data, uint8_t rshift, uint8_t gshift, uint8_t bshift)
{
  return rgbexpand<8, 8, 8>(data, rshift, gshift, bshift);
}