#pragma once
#include "point.hpp"
#include <bits/stdc++.h>
using namespace std;

// 線分.
template<floating_point T = long double>
struct line_segment {
  point<T> s, t;
  constexpr line_segment(point<T> const& _t = {}): s(0, 0), t(_t) {}
  constexpr line_segment(point<T> const& _s, point<T> const& _t): s(_s), t(_t) {}
  // clang-format off
  friend istream& operator>>(istream& i, line_segment& l) { i >> l.s >> l.t; return i; }
  friend ostream& operator<<(ostream& o, line_segment const& l) { o << l.s << "->" << l.t; return o; }
  // clang-format on

  constexpr point<T> dir() const { return t - s; }
  constexpr T length() const { return dir().length(); }
  constexpr point<T> unit() const { return dir().unit(); }
  // 点と線分の距離(直線ではないことに注意)
  constexpr T dist(point<T> const& p = {}) const {
    if((p - s).dot_product(dir()) <= 0 || dir().is_zero()) return p.dist(s);
    if((p - t).dot_product(-dir()) <= 0) return p.dist(t);
    return abs(cross_product(s - p, t - p)) / length();
  }
};
