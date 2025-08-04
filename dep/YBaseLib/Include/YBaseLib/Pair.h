#pragma once

template<typename left_t, typename right_t>
struct Pair
{
  Pair() {}
  Pair(const left_t& rLeft, const right_t& rRight) : Left(rLeft), Right(rRight) {}
  Pair(const Pair<left_t, right_t>& c) : Left(c.Left), Right(c.Right) {}
  ~Pair() {}

  Pair& operator=(const Pair<left_t, right_t>& p)
  {
    Left = p.Left;
    Right = p.Right;
    return *this;
  }

  bool operator==(const Pair<left_t, right_t>& p) const { return (Left == p.Left && Right == p.Right); }
  bool operator!=(const Pair<left_t, right_t>& p) const { return (Left != p.Left || Right != p.Right); }
  bool operator<(const Pair<left_t, right_t>& p) const { return (Left < p.Left && Right < p.Right); }
  bool operator>(const Pair<left_t, right_t>& p) const { return (Left > p.Left && Right > p.Right); }

  left_t Left;
  right_t Right;
};
