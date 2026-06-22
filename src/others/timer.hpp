#pragma once
#include <bits/stdc++.h>
using namespace std;

// 呼ぶ回数が多いとかなり重くなるので注意(2sで7e7回程度)
struct Timer {
  chrono::steady_clock::time_point p = chrono::steady_clock::now();
  // ミリ秒単位で返す.
  inline double operator()() const {
    return chrono::duration<double, milli>(chrono::steady_clock::now() - p).count();
  }
  inline void reset() { p = chrono::steady_clock::now(); }
} cl;
