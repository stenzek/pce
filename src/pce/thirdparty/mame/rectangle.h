// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    bitmap.h

    Core bitmap routines.

***************************************************************************/
#pragma once
#include "../../types.h"

// ======================> rectangle

// rectangles describe a bitmap portion
class rectangle
{
public:
  // construction/destruction
  constexpr rectangle() {}
  constexpr rectangle(s32 minx, s32 maxx, s32 miny, s32 maxy) : min_x(minx), max_x(maxx), min_y(miny), max_y(maxy) {}

  // getters
  constexpr s32 left() const { return min_x; }
  constexpr s32 right() const { return max_x; }
  constexpr s32 top() const { return min_y; }
  constexpr s32 bottom() const { return max_y; }

  // compute intersection with another rect
  rectangle& operator&=(const rectangle& src)
  {
    if (src.min_x > min_x)
      min_x = src.min_x;
    if (src.max_x < max_x)
      max_x = src.max_x;
    if (src.min_y > min_y)
      min_y = src.min_y;
    if (src.max_y < max_y)
      max_y = src.max_y;
    return *this;
  }

  // compute union with another rect
  rectangle& operator|=(const rectangle& src)
  {
    if (src.min_x < min_x)
      min_x = src.min_x;
    if (src.max_x > max_x)
      max_x = src.max_x;
    if (src.min_y < min_y)
      min_y = src.min_y;
    if (src.max_y > max_y)
      max_y = src.max_y;
    return *this;
  }

  // comparisons
  constexpr bool operator==(const rectangle& rhs) const
  {
    return min_x == rhs.min_x && max_x == rhs.max_x && min_y == rhs.min_y && max_y == rhs.max_y;
  }
  constexpr bool operator!=(const rectangle& rhs) const
  {
    return min_x != rhs.min_x || max_x != rhs.max_x || min_y != rhs.min_y || max_y != rhs.max_y;
  }
  constexpr bool operator>(const rectangle& rhs) const
  {
    return min_x < rhs.min_x && min_y < rhs.min_y && max_x > rhs.max_x && max_y > rhs.max_y;
  }
  constexpr bool operator>=(const rectangle& rhs) const
  {
    return min_x <= rhs.min_x && min_y <= rhs.min_y && max_x >= rhs.max_x && max_y >= rhs.max_y;
  }
  constexpr bool operator<(const rectangle& rhs) const
  {
    return min_x >= rhs.min_x || min_y >= rhs.min_y || max_x <= rhs.max_x || max_y <= rhs.max_y;
  }
  constexpr bool operator<=(const rectangle& rhs) const
  {
    return min_x > rhs.min_x || min_y > rhs.min_y || max_x < rhs.max_x || max_y < rhs.max_y;
  }

  // other helpers
  constexpr bool empty() const { return (min_x > max_x) || (min_y > max_y); }
  constexpr bool contains(s32 x, s32 y) const { return (x >= min_x) && (x <= max_x) && (y >= min_y) && (y <= max_y); }
  constexpr bool contains(const rectangle& rect) const
  {
    return (min_x <= rect.min_x) && (max_x >= rect.max_x) && (min_y <= rect.min_y) && (max_y >= rect.max_y);
  }
  constexpr s32 width() const { return max_x + 1 - min_x; }
  constexpr s32 height() const { return max_y + 1 - min_y; }
  constexpr s32 xcenter() const { return (min_x + max_x + 1) / 2; }
  constexpr s32 ycenter() const { return (min_y + max_y + 1) / 2; }

  // setters
  void set(s32 minx, s32 maxx, s32 miny, s32 maxy)
  {
    min_x = minx;
    max_x = maxx;
    min_y = miny;
    max_y = maxy;
  }
  void setx(s32 minx, s32 maxx)
  {
    min_x = minx;
    max_x = maxx;
  }
  void sety(s32 miny, s32 maxy)
  {
    min_y = miny;
    max_y = maxy;
  }
  void set_width(s32 width) { max_x = min_x + width - 1; }
  void set_height(s32 height) { max_y = min_y + height - 1; }
  void set_origin(s32 x, s32 y)
  {
    max_x += x - min_x;
    max_y += y - min_y;
    min_x = x;
    min_y = y;
  }
  void set_size(s32 width, s32 height)
  {
    set_width(width);
    set_height(height);
  }

  // offset helpers
  void offset(s32 xdelta, s32 ydelta)
  {
    min_x += xdelta;
    max_x += xdelta;
    min_y += ydelta;
    max_y += ydelta;
  }
  void offsetx(s32 delta)
  {
    min_x += delta;
    max_x += delta;
  }
  void offsety(s32 delta)
  {
    min_y += delta;
    max_y += delta;
  }

  // internal state
  s32 min_x = 0; // minimum X, or left coordinate
  s32 max_x = 0; // maximum X, or right coordinate (inclusive)
  s32 min_y = 0; // minimum Y, or top coordinate
  s32 max_y = 0; // maximum Y, or bottom coordinate (inclusive)
};
