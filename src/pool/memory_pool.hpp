#pragma once
#include <bits/stdc++.h>
using namespace std;

template<class T>
struct memory_pool {
  static inline bool fast = false;
  size_t sz, id, cnt; // cnt=確保した要素の数.
  stack<pair<T*, size_t>> pool;
  stack<T*> st;
  constexpr memory_pool(): sz(1), id(0), cnt(0) {}
  constexpr ~memory_pool() {
    if(!fast) clear();
  }

  // amortized O(1)
  template<class... Args>
  constexpr T* alloc(Args&&... args) {
    if(!st.empty()) {
      T* ret = st.top();
      st.pop();
      if constexpr(!is_trivially_destructible_v<T>)
        if(!fast) ret->~T();
      return new(ret) T(forward<Args>(args)...);
    }
    ++cnt;
    if(++id == sz) {
      sz <<= 1;
      pool.emplace((T*)malloc(sizeof(T) * sz), sz);
      id = 0;
    }
    return new(pool.top().first + id) T(forward<Args>(args)...);
  }
  // O(1)
  constexpr void free(T* p) { st.emplace(p); }
  // is_trivially_destructible_v<T>?O(logN):O(N)
  constexpr void clear() {
    if constexpr(!is_trivially_destructible_v<T>) {
      if(!pool.empty()) {
        auto [p, _] = pool.top();
        pool.pop();
        for(size_t i = 0; i <= id; ++i) (p + i)->~T();
        ::free(p);
      }
    }
    while(!pool.empty()) {
      auto [p, _sz] = pool.top();
      pool.pop();
      if constexpr(!is_trivially_destructible_v<T>)
        for(size_t i = 0; i < _sz; ++i) (p + i)->~T();
      ::free(p);
    }
    sz = 1, id = 0, cnt = 0;
    st = stack<T*>{};
  }
};
