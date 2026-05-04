#pragma once
#include <bits/stdc++.h>
#include <execution>
#include <immintrin.h>
using namespace std;

namespace HashMap {
// clang-format off
template<class T> constexpr T default_e() { return T{}; }
template<class T> constexpr uint16_t default_meta(size_t const& h) {
  if constexpr(is_integral_v<T> && sizeof(T) == 4) return h >> 17 & 0x7fff;
  else return h >> (sizeof(size_t) * 8 - (16 - 1));
}
template<class T> size_t shift_hash(T const& t) { size_t h = hash<T>()(t); return (h << 10) + h + (h >> 10); }
// clang-format on
// string等コンストラクタ呼び出し必須な型に非対応.
// keyが数値でシーケンシャルアクセス->ランダムアクセスやランダムアクセス->シーケンシャルアクセスとなると非常に遅いので.
// 必要なら自前のhash関数やshift_hashを使用する.
// make_metaの返り値(uint16_t)retは1<<15未満.必要なら&0x7fff
template<class Key, class Value = Key, auto f_e = default_e<Value>,
         auto make_hash = hash<Key>{}, auto make_meta = default_meta<Key>, auto eq = equal_to<Key>{}>
struct swiss_table {
  static_assert(is_invocable_r_v<Value, decltype(f_e)>, "f_e must work as Value()");
  static_assert(is_invocable_r_v<size_t, decltype(make_hash), Key>, "make_hash must work as size_t(Key)");
  static_assert(is_invocable_r_v<uint16_t, decltype(make_meta), size_t>,
                "make_mata must work as uint16_t(size_t)");
  static_assert(is_invocable_r_v<bool, decltype(eq), Key, Key>, "eq must work as bool(Key, Key)");
  static constexpr Value e = f_e();
  static constexpr bool def_hash = is_same_v<decltype(make_hash), hash<Key>>;
  static constexpr uint16_t e_bit = 0x8000;
  static inline bool fast = false;
  static inline uint8_t min_ep = 3;

  using Hash = size_t;
  Key* key = nullptr;
  Value* val = nullptr;
  uint16_t* meta = nullptr;
  uint8_t ep = 0;
  uint32_t mask = 0;
  uint32_t cnt = 0, bucket = 0;
  constexpr swiss_table() {}
  constexpr swiss_table(vector<pair<Key, Value>> const& v) { rehash(vector(v)); }
  constexpr swiss_table(vector<pair<Key, Value>>&& v) { rehash(move(v)); }
  template<convertible_to<pair<Key, Value>>... Pair>
  constexpr swiss_table(Pair... p) { rehash(vector{forward<Pair>(p)...}); }
  constexpr swiss_table(swiss_table const& ot): ep(ot.ep), mask(ot.mask), cnt(ot.cnt), bucket(ot.bucket) {
    cp(meta, ot.meta), cp(key, ot.key), cp(val, ot.val);
  }
  constexpr swiss_table(swiss_table&& ot) noexcept
      : key(ot.key), val(ot.val), meta(ot.meta), ep(ot.ep), mask(ot.mask), cnt(ot.cnt), bucket(ot.bucket) {
    ot.key = nullptr, ot.val = nullptr, ot.meta = nullptr;
  }
  constexpr swiss_table& operator=(swiss_table const& ot) {
    if(!fast && key) free(key), free(val), free(meta);
    ep = ot.ep, mask = ot.mask, cnt = ot.cnt, bucket = ot.bucket;
    cp(meta, ot.meta), cp(key, ot.key), cp(val, ot.val);
    return *this;
  }
  constexpr swiss_table& operator=(swiss_table&& ot) noexcept {
    if(!fast && key) free(key), free(val), free(meta);
    key = ot.key, val = ot.val, meta = ot.meta, ep = ot.ep, mask = ot.mask, cnt = ot.cnt, bucket = ot.bucket;
    ot.key = nullptr, ot.val = nullptr, ot.meta = nullptr;
    return *this;
  }
  template<class T1>
  constexpr void cp(T1*& target, T1* source) {
    if(!source) {
      target = nullptr;
      return;
    }
    target = (T1*)malloc(sizeof(T1) * bucket);
    if constexpr(is_integral_v<T1> && !(64 % sizeof(T1))) {
      static constexpr int sz = 64 / sizeof(T1);
      for(int i = 0, lim = int(bucket) - sz; i < lim; i += sz)
        _mm512_storeu_si512(target + i, _mm512_loadu_si512(source + i));
      if(bucket > sz) [[likely]]
        _mm512_storeu_si512(target + bucket - sz, _mm512_loadu_si512(source + bucket - sz));
      else for_each_n(execution::par_unseq, ranges::views::iota(uint32_t(0), bucket).begin(), bucket,
                      [&](uint32_t i) { *(target + i) = *(source + i); });
    } else {
      int i = 0;
      for(int lim = int(bucket) - 32; i < lim; i += 32) {
        uint32_t exist = ~_mm512_movepi16_mask(_mm512_loadu_si512(meta + i));
        for(size_t j = i + countr_zero(exist), lim2 = i + 32; j < lim2; j = i + countr_zero(exist &= exist - 1))
          *(target + j) = *(source + j);
      }
      for_each_n(execution::par_unseq, ranges::views::iota(uint32_t(i), bucket).begin(), bucket - i,
                 [&](uint32_t j) { if(!(*(meta + j) & e_bit)) *(target + j) = *(source + j); });
    }
  }
  constexpr ~swiss_table() {
    if(!fast) free(key), free(val), free(meta);
  }

  constexpr size_t size() const { return cnt; }
  template<convertible_to<Key> K>
  constexpr Value& operator[](K&& k) {
    Hash h = make_hash(k);
    if(key) [[likely]] {
      auto [id, b] = find_id(k, h);
      if(b) return *(val + id);
      else if(id - (h & mask) != 32) [[likely]] {
        *(key + id) = forward<K>(k);
        *(meta + id) = make_meta(h);
        ++cnt;
        return *(val + id) = e;
      }
    }
    rehash(vector{pair<Key, Value>(k, e)});
    return *(val + find_id(k, h).first);
  }
  // 要素を追加する(keyはまだ存在しないことがわかっているとき)
  template<convertible_to<Key> K, convertible_to<Value> V>
  constexpr Value* emplace(K&& k, V&& v) {
    Hash h = make_hash(k);
    if(key) [[likely]] {
      uint32_t id = h & mask;
      int d = countr_zero(_mm512_movepi16_mask(_mm512_loadu_si512((__m512i*)(meta + id))));
      if(d != 32) [[likely]] {
        int n_id = id + d;
        *(key + n_id) = forward<K>(k);
        *(val + n_id) = forward<V>(v);
        *(meta + n_id) = make_meta(h);
        ++cnt;
        return val + n_id;
      }
    }
    rehash(vector{pair<Key, Value>(k, forward<V>(v))});
    return val + find_id(k, h).first;
  }
  constexpr Value* find(Key const& k) {
    if(!key) [[unlikely]]
      return nullptr;
    auto [id, b] = find_id(k);
    if(b) return val + id;
    else return nullptr;
  }
  constexpr vector<pair<Key const&, Value&>> all_element() const {
    vector<pair<Key const&, Value&>> ret;
    if(!key) return ret;
    ret.reserve(cnt);
    for(int i = 0, lim = int(bucket) - 32; i < lim; i += 32) {
      uint32_t exist = ~_mm512_movepi16_mask(_mm512_loadu_si512(meta + i));
      for(size_t j = i + countr_zero(exist), lim2 = i + 32; j < lim2; j = i + countr_zero(exist &= exist - 1))
        ret.emplace_back(*(key + j), *(val + j));
    }
    for(size_t i = bucket / 32 * 32; i < bucket; ++i)
      if(!(*(meta + i) & e_bit)) ret.emplace_back(*(key + i), *(val + i));
    return ret;
  }

  // private:
  constexpr pair<int, bool>
  find_id(Key const& k) const { return find_id(k, make_hash(k)); }
  // kが所定の位置からいくつずれているかを返す(second=true)存在しなければ埋まっていない最も近い場所を返す(second=false)
  constexpr pair<int, bool> find_id(Key const& k, Hash h) const {
    Hash id = h & mask;
    __m512i table = _mm512_loadu_si512((__m512i*)(meta + id));
    __mmask32 same = _mm512_cmpeq_epi16_mask(_mm512_set1_epi16(make_meta(h)), table);
    Key* k_id = key + id;
    for(Key *p = k_id + countr_zero(same), *lim = k_id + 32; p < lim; p = k_id + countr_zero(same &= same - 1))
      if(eq(*p, k)) return pair(p - key, true);
    return pair(id + countr_zero(_mm512_movepi16_mask(table)), false);
  }
  constexpr void rehash(vector<pair<Key, Value>>&& v) {
    uint32_t p_sz = v.size(), n_sz = cnt + p_sz;
    Key* key_list = (Key*)malloc(sizeof(Key) * n_sz);
    Value* val_list = (Value*)malloc(sizeof(Value) * n_sz);
    uint16_t* meta_list = (uint16_t*)malloc(2 * n_sz);
    move_list(key_list, key), move_list(val_list, val), move_list(meta_list, meta);
    for_each_n(execution::par_unseq, ranges::views::iota(uint32_t(0), p_sz).begin(), p_sz, [&](int i) {
      *(meta_list + cnt + i) = make_meta(make_hash(v[i].first));
      *(key_list + cnt + i) = move(v[i].first);
      *(val_list + cnt + i) = move(v[i].second);
    });
    uint32_t* hs = nullptr;
    if constexpr(is_integral_v<Key> && sizeof(Key) == 4 && def_hash) hs = (uint32_t*)key_list;
    else {
      hs = (uint32_t*)malloc(4 * n_sz);
      if constexpr(is_integral_v<Key> && sizeof(Key) == 8 && def_hash) {
        for(int i = 0, lim = n_sz - 8; i < lim; i += 8)
          _mm256_storeu_si256((__m256i*)(hs + i), _mm512_cvtepi64_epi32(_mm512_loadu_epi64(key_list + i)));
        if(n_sz >= 8) [[likely]]
          _mm256_storeu_si256((__m256i*)(hs + n_sz - 8),
                              _mm512_cvtepi64_epi32(_mm512_loadu_epi64(key_list + n_sz - 8)));
        else
          for_each_n(execution::par_unseq, ranges::views::iota(uint32_t(0), n_sz).begin(), n_sz,
                     [&](uint32_t i) { *(hs + i) = uint32_t(*(key_list + i)); });
      } else transform(execution::par_unseq, key_list, key_list + n_sz, hs,
                       [](Key const& k) { return uint32_t(make_hash(k)); });
    }
    cnt = n_sz;
    if(!fast && key) free(key), free(val), free(meta);
    uint8_t n_ep = max<int>({min_ep, ep + 1, countr_zero(bit_ceil(n_sz))});
    static __m512i const zero = _mm512_set1_epi64(0);
    uint32_t* pos;
    while(true) {
      if(n_ep >= 32) assert("size over" && false);
      uint32_t const sz = 1 << n_ep, temp_mask = sz - 1;
      uint32_t* counts = (uint32_t*)malloc(4 * sz);
      for(int i = 0, lim = int(sz) - 16; i < lim; i += 16) _mm512_storeu_si512(counts + i, zero);
      if(sz >= 16) [[likely]]
        _mm512_storeu_si512(counts + sz - 16, zero);
      else
        for_each_n(execution::par_unseq, ranges::views::iota(uint32_t(0), sz).begin(), sz,
                   [&](uint32_t i) { *(counts + i) = 0; });
      for(uint32_t i = 0; i < n_sz; ++i) ++*(counts + (*(hs + i) & temp_mask));
      bool can = true;
      pos = (uint32_t*)malloc(sz * 4);
      *pos = 0;
      for(uint32_t i = 1; i < sz; ++i) {
        uint32_t pos_i = max(i, *(pos + i - 1) + *(counts + i - 1));
        if(pos_i >= i + 32) [[unlikely]] {
          can = false;
          break;
        }
        *(pos + i) = pos_i;
      }
      can = can && *(pos + sz - 1) + *(counts + sz - 1) < sz + 32;
      if(!fast) free(counts);
      if(can) [[likely]]
        break;
      if(!fast) free(pos);
      ++n_ep;
    }
    ep = n_ep, mask = (1 << ep) - 1, bucket = (1 << ep) + 31;
    key = (Key*)malloc(sizeof(Key) * bucket);
    val = (Value*)malloc(sizeof(Value) * bucket);
    meta = (uint16_t*)malloc(2 * bucket);
    static __m512i const _e_bit = _mm512_set1_epi16(e_bit);
    for(uint32_t i = 0, lim = bucket - 32; i < lim; i += 32) _mm512_storeu_si512(meta + i, _e_bit);
    _mm512_storeu_si512(meta + bucket - 32, _e_bit);
    for(uint32_t i = 0; i < cnt; ++i) {
      uint32_t h = *(hs + i) & mask;
      *(key + *(pos + h)) = move(*(key_list + i));
      *(val + *(pos + h)) = move(*(val_list + i));
      *(meta + (*(pos + h))++) = *(meta_list + i);
    }
    if(!fast) {
      if constexpr(!(is_integral_v<Key> && sizeof(Key) == 4 && def_hash)) free(hs);
      free(pos), free(key_list), free(val_list), free(meta_list);
    }
  }
  template<class T1>
  constexpr void move_list(T1* target, T1* before) {
    uint32_t i = 0, id = 0;
    if constexpr(is_integral_v<T1> && sizeof(T1) == 2) {
      if(bucket >= 32) [[likely]]
        for(uint32_t lim = bucket - 32; i < lim; i += 32) {
          __mmask32 exist = ~_mm512_movepi16_mask(_mm512_loadu_si512(meta + i));
          _mm512_mask_compressstoreu_epi16(target + id, exist, _mm512_loadu_si512(before + i));
          id += popcount(exist);
        }
    }
    if constexpr(is_integral_v<T1> && sizeof(T1) == 4) {
      if(bucket >= 16) [[likely]]
        for(uint32_t lim = bucket - 16; i < lim; i += 16) {
          __mmask16 exist = ~_mm256_movepi16_mask(_mm256_loadu_si256((__m256i*)(meta + i)));
          _mm512_mask_compressstoreu_epi32(target + id, exist, _mm512_loadu_si512(before + i));
          id += popcount(exist);
        }
    }
    if constexpr(is_integral_v<T1> && sizeof(T1) == 8) {
      if(bucket >= 8) [[likely]]
        for(uint32_t lim = bucket - 8; i < lim; i += 8) {
          __mmask8 exist = ~_mm_movepi16_mask(_mm_loadu_si128((__m128i*)(meta + i)));
          _mm512_mask_compressstoreu_epi64(target + id, exist, _mm512_loadu_si512(before + i));
          id += popcount(exist);
        }
    }
    for(T1* p = target + id; i < bucket; ++i)
      if(!(*(meta + i) & e_bit)) *(p++) = move(*(before + i));
  }
};
} // namespace HashMap
using HashMap::default_meta;
using HashMap::shift_hash;
// string等コンストラクタ呼び出し必須な型に非対応.
// keyが数値でシーケンシャルアクセス->ランダムアクセスやランダムアクセス->シーケンシャルアクセスとなると非常に遅いので.
// 必要なら自前のhash関数やshift_hashを使用する.
// make_metaの返り値(uint16_t)retは1<<15未満.必要なら&0x7fff
template<class Key, class Value = Key, auto f_e = HashMap::default_e<Value>,
         auto make_hash = hash<Key>{}, auto make_meta = default_meta<Key>, auto eq = equal_to<Key>{}>
using hashmap = HashMap::swiss_table<Key, Value, f_e, make_hash, make_meta, eq>;