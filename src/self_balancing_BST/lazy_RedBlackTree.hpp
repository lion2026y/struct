#pragma once
#include "lion/memory_pool.hpp"
#include <bits/stdc++.h>
using namespace std;

#define rep(i, n) for(int i = 0; i < int(n); ++i)

// lazy Red-Black Tree
// need memory_pool,rep
// merge,meldに注意.
// binary_leftはacl::segtree.min_leftとは仕様が違うことに注意.
template<class T>
constexpr T default_Q(T const& a, T const& b) { return a + b; }
template<class T>
constexpr T default_e() { return T{}; }
template<class T1, class T2, class T3>
constexpr T2 default_effect(pair<T1, T2> const& p, T3 const& x, int c) {
  return p.first * x + p.second * c;
}
template<class T>
constexpr pair<T, T> default_composite(pair<T, T> const& a, pair<T, T> const& b) {
  return pair(a.first * b.first, a.first * b.second + a.second);
}
template<class T1, class T2>
constexpr pair<T1, T2> default_pair() { return pair(1, 0); }
template<class Key, class Value = Key, auto Q = default_Q<Value>, auto f_V_e = default_e<Value>,
         class Lazy = pair<Value, Value>, auto effect = default_effect<Value, Value, Value>,
         auto composite = default_composite<Value>, auto f_L_e = default_pair<Value, Value>,
         auto comp = less<Key>{}>
struct lazy_RBT {
  static_assert(is_invocable_r_v<Value, decltype(Q), Value, Value>, "Q must work as Value(Value,Value)");
  static_assert(is_invocable_r_v<Value, decltype(f_V_e)>, "f_V_e must work as Value())");
  static_assert(is_invocable_r_v<Value, decltype(effect), Lazy, Value, int>,
                "effect must work as Value(Lazy,Value,int)");
  static_assert(is_invocable_r_v<Lazy, decltype(composite), Lazy, Lazy>,
                "composite must work as Lazy(Lazy,Lazy)");
  static_assert(is_invocable_r_v<Lazy, decltype(f_L_e)>, "f_L_e must work as Lazy())");
  static_assert(is_invocable_r_v<bool, decltype(comp), Key, Key>, "comp must work as bool(Key,Key)");

  struct node;
  using ptr = node*;

  static inline Value const Value_e = f_V_e();
  static inline Lazy const Lazy_e = f_L_e();
  static inline memory_pool<node> pool;
  // 関数をKeyで呼ぶか.falseならindexで呼ぶ.
  static inline bool Key_call = true;
  template<bool B>
  using idx = conditional_t<B, Key const&, int>;
  template<bool B>
  using idx_ref = conditional_t<B, Key const&, int&>;
  template<bool B, class K>
  using idx_make = conditional_t<B, K&&, int>;
  using val_const_ptr = Value const*;

#define lazy_RBT_func(func, type, ...) \
  template<bool B, class... Args> \
    requires(!constructible_from<tuple<__VA_ARGS__>, Args...>) \
  constexpr type func(Args&&... args) { \
    cerr << "bad argument type\narguments are\n"; \
    ([&] { cerr << args << " "; }(), ...); \
    cerr << "\n"; \
    assert(false); \
    return type{}; \
  } \
  template<class... Args> \
  constexpr auto func(Args&&... args) { \
    if(Key_call) return this->template func<true>(forward<Args>(args)...); \
    else return this->template func<false>(forward<Args>(args)...); \
  }
  // 引数の型は非ユニバーサル参照.
#define lazy_RBT_make_node_func(func, type, ...) \
  template<bool B, class K, class... Args> \
    requires(!constructible_from<tuple<__VA_ARGS__>, Args...>) \
  constexpr type func(Args&&... args) { \
    cerr << "bad argument type\narguments are\n"; \
    ([&] { cerr << args << " "; }(), ...); \
    cerr << "\n"; \
    assert(false); \
    return type{}; \
  } \
  template<class T1, class... Args> \
  constexpr auto func(T1&& arg1, Args&&... args) { \
    if(Key_call) return this->template func<true, T1>(forward<T1>(arg1), forward<Args>(args)...); \
    else return this->template func<false, Key>(forward<T1>(arg1), forward<Args>(args)...); \
  }

  ptr root;
  constexpr lazy_RBT(): root(nullptr) {}
  constexpr lazy_RBT(lazy_RBT&& ot) noexcept: root(ot.root) { ot.root = nullptr; }

  // static constexpr bool eq(Key const& a, Key const& b) {
  //   if constexpr(same_as<decltype(comp), less<Key>>) return a == b;
  //   else return !comp(a, b) && !comp(b, a);
  // }

  static constexpr void use_key() { Key_call = true; }
  static constexpr void use_id() { Key_call = false; }

  constexpr int size() const {
    if(!root) [[unlikely]]
      return 0;
    return root->sz;
  }
  constexpr Value all_prod() {
    if(!root) [[unlikely]]
      return Value_e;
    return root->eval()->val;
  }
  template<bool B>
  static constexpr void check(idx<B> x, int l, int r) {
    if constexpr(!B) assert(l <= x && x < r);
  }
  template<bool B>
  static constexpr void go_down(ptr& n, idx_ref<B> k, bool b) {
    if(b) n = n->ch[0];
    else n->template change<B>(k), n = n->ch[1];
  }

  // O(log|l->rank-r->rank|)
  // l,rをmergeする.mergeした根のポインタを返す.
  constexpr ptr merge(ptr l, ptr r) {
    if(!l) [[unlikely]]
      return r;
    if(!r) [[unlikely]]
      return l;
    return merge_sub(l->paint(true), r->paint(true));
  }
  template<class... Args>
  constexpr ptr merge(ptr n, Args const&... args) { return merge(n, merge(args...)); }
  constexpr ptr merge_sub(ptr l, ptr r) {
    stack<tuple<ptr, ptr, bool>> st;
    while(l->rank != r->rank) {
      bool b = l->rank > r->rank;
      st.emplace(l, r, b);
      if(b) l = l->eval()->ch[1];
      else r = r->eval()->ch[0];
    }
    ptr ret = pool.alloc(l, r);
    if(!l->black) ret = rotate(ret, 1);
    if(!r->black) ret = rotate(ret, 0);
    bool b;
    while(!st.empty()) {
      tie(l, r, b) = st.top(), st.pop();
      if(b) {
        l->ch[1] = ret;
        if(!l->ch[1]->black && !l->ch[1]->ch[1]->black) {
          if(!l->ch[0]->black) {
            l->ch[0]->paint(true);
            l->ch[1]->paint(true);
            ret = l->paint(false)->update();
          } else {
            l->ch[1]->paint(true);
            l->paint(false);
            ret = rotate(l, 0);
          }
        } else ret = l->update();
      } else {
        r->ch[0] = ret;
        if(!r->ch[0]->black && !r->ch[0]->ch[0]->black) {
          if(!r->ch[1]->black) {
            r->ch[0]->paint(true);
            r->ch[1]->paint(true);
            ret = r->paint(false)->update();
          } else {
            r->ch[0]->paint(true);
            r->paint(false);
            ret = rotate(r, 1);
          }
        } else ret = r->update();
      }
    }
    return ret;
  }
  template<bool B, convertible_to<idx<B>>... Args>
  constexpr auto split(ptr n, Args... k) {
    constexpr size_t N = sizeof...(k) + 1;
    array<ptr, N> ret;
    int id = 0;
    auto f = [&](idx<B> ki) {
      auto [r, nt] = split_sub<B>(n, ki);
      ret[id++] = r;
      n = nt;
    };
    (..., f(k));
    ret[N - 1] = n;
    return ret;
  }
  template<bool B>
  constexpr pair<ptr, ptr> split_sub(ptr n, idx<B> k) {
    if(!n) [[unlikely]]
      return pair(nullptr, nullptr);
    check<B>(k, 0, n->sz + 1);
    stack<pair<ptr, bool>> st;
    while(!n->eval()->is_leaf() && !n->template split_now<B>(k)) [[likely]] {
      bool b = n->template down_left<B>(k);
      st.emplace(n, b);
      go_down<B>(n, k, b);
    }
    ptr l = nullptr, r = nullptr;
    if(n->is_leaf()) {
      if(n->template right<B>(k)) l = n;
      else r = n;
    } else l = n->ch[0], r = n->ch[1], pool.free(n);
    bool b;
    while(!st.empty()) [[likely]] {
      tie(n, b) = st.top(), st.pop();
      if(b) r = merge(r, n->ch[1]);
      else l = merge(n->ch[0], l);
      pool.free(n);
    }
    return pair(l, r);
  }
  // O(logN)
  // key=kなるnodeにvをsetする.新しく作ったかを返す.
  template<bool B, convertible_to<Key> K, convertible_to<Value> V>
  constexpr bool set(idx_make<B, K> k, V&& v) {
    check<B>(k, 0, size());
    if(!root) [[unlikely]] {
      if constexpr(B) root = pool.alloc(forward<K>(k), forward<V>(v));
      else root = pool.alloc(optional<Key>{}, forward<V>(v));
      return true;
    }
    ptr n = root;
    stack<pair<ptr, bool>> st;
    while(!n->is_leaf()) [[likely]] {
      bool b = n->eval()->template down_left<B>(k);
      st.emplace(n, b);
      go_down<B>(n, k, b);
    }
    bool ret = false, b1 = n->template left<B>(k), b2 = n->template right<B>(k);
    if(!b1 && !b2) n->val = v, n->rev = false, n->lazy = Lazy_e;
    else {
      ptr _temp;
      if constexpr(B) _temp = pool.alloc(forward<K>(k), forward<V>(v));
      else _temp = pool.alloc(optional<Key>{}, forward<V>(v));
      ret = true;
      if(b1) n = pool.alloc(_temp, n);
      else n = pool.alloc(n, _temp);
    }
    while(!st.empty()) [[likely]] {
      auto [p, b] = st.top();
      st.pop();
      if(n->rank == p->ch[b]->rank && (p->black || n->black)) p->ch[b ^ 1] = n, n = p->update();
      else {
        ptr c = p->ch[b];
        pool.free(p);
        if(b) n = merge(n, c);
        else n = merge(c, n);
      }
    }
    root = n;
    return ret;
  }
  lazy_RBT_make_node_func(set, bool, idx<B>, Value);
  // O(logN)
  // idの手前にnodeを作成する=id個目としてnodeを挿入する.
  template<convertible_to<Value> V>
  constexpr void emplace_between(int k, V&& v) {
    assert(!Key_call);
    check<false>(k, 0, size() + 1);
    if(!root) [[unlikely]] {
      root = pool.alloc(optional<Key>{}, forward<V>(v));
      return;
    }
    ptr n = root;
    stack<pair<ptr, bool>> st;
    while(!n->is_leaf()) [[likely]] {
      bool b = n->eval()->template down_left<false>(k);
      st.emplace(n, b);
      go_down<false>(n, k, b);
    }
    if(n->template right<false>(k)) n = pool.alloc(n, pool.alloc(optional<Key>{}, forward<V>(v)));
    else n = pool.alloc(pool.alloc(optional<Key>{}, forward<V>(v)), n);
    while(!st.empty()) [[likely]] {
      auto [p, b] = st.top();
      st.pop();
      if(n->rank == p->ch[b]->rank && (p->black || n->black)) p->ch[b ^ 1] = n, n = p->update();
      else {
        ptr c = p->ch[b];
        pool.free(p);
        if(b) n = merge(n, c);
        else n = merge(c, n);
      }
    }
    root = n;
  }
  // O(logN)
  // key=kなるnodeを削除する.もともと存在したかを返す.
  template<bool B>
  bool erase(idx<B> k) {
    check<B>(k, 0, size());
    if(!root) [[unlikely]]
      return false;
    ptr n = root;
    stack<pair<ptr, bool>> st;
    while(!n->is_leaf()) [[likely]] {
      bool b = n->eval()->template down_left<B>(k);
      st.emplace(n, b);
      go_down<B>(n, k, b);
    }
    bool ret = false;
    if(!n->template left<B>(k) && !n->template right<B>(k)) pool.free(n), n = nullptr, ret = true;
    while(!st.empty()) [[likely]] {
      auto [p, b] = st.top();
      st.pop();
      if(n && n->rank == p->ch[b]->rank && (p->black || n->black)) p->ch[b ^ 1] = n, n = p->update();
      else {
        ptr c = p->ch[b];
        pool.free(p);
        if(b) n = merge(n, c);
        else n = merge(c, n);
      }
    }
    root = n;
    return ret;
  }
  lazy_RBT_func(erase, bool, idx<B>);
  // O(logN)
  // key=kなるノードのvalueのポインタを返す.
  template<bool B>
  constexpr val_const_ptr get(idx<B> k) {
    check<B>(k, 0, size());
    if(!root) [[unlikely]]
      return nullptr;
    ptr r = get_sub<B>(root, k);
    if(r) return &r->val;
    else return nullptr;
  }
  lazy_RBT_func(get, val_const_ptr, idx<B>);
  // O(logN)
  // id=kなるnodeのkeyのoptionalを返す.*でアクセス可能.
  constexpr optional<Key> const& get_key(int k) {
    assert(!Key_call);
    check<false>(k, 0, size());
    return get_sub<false>(root, k)->key;
  }
  template<bool B>
  constexpr ptr get_sub(ptr n, idx<B> k) {
    while(!n->is_leaf()) [[likely]]
      go_down<B>(n, k, n->eval()->template down_left<B>(k));
    if(n->template left<B>(k) || n->template right<B>(k)) return nullptr;
    else return n->eval();
  }
  // k以上の最小の要素のid(0-indexed)を返す.
  constexpr int lower_bound(Key const& k) {
    if(!root) [[unlikely]]
      return 0;
    return bound_sub(root, k, true);
  }
  // k超過の最小の要素のid(0-indexed)を返す.
  constexpr int upper_bound(Key const& k) {
    if(!root) [[unlikely]]
      return 0;
    return bound_sub(root, k, false);
  }
  constexpr int bound_sub(ptr n, Key const& k, bool b) {
    assert(Key_call);
    int ret = 0;
    while(!n->eval()->is_leaf()) [[likely]] {
      if(n->template down_left<true>(k)) n = n->ch[0];
      else ret += n->ch[0]->sz, n = n->ch[1];
    }
    if(b) return ret + n->template left<true>(k);
    else return ret + !n->template right<true>(k);
  }
  // O(logN)
  // [l,r)の総積を返す.
  template<bool B>
  constexpr Value prod(idx<B> l, idx<B> r) {
    check<B>(l, 0, size() + 1), check<B>(r, 0, size() + 1);
    Value ret = Value_e;
    static constexpr auto f = [](Value& x, ptr n, bool b) {
      if(b) x = Q(n->val, x);
      else x = Q(x, n->val);
    };
    if constexpr(B) range_key(l, r, ret, f);
    else range_id(l, r, ret, f);
    return ret;
  }
  lazy_RBT_func(prod, Value, idx<B>, idx<B>);
  template<bool B>
  constexpr void apply(idx<B> l, idx<B> r, Lazy const& v) {
    stack<ptr> st;
    check<B>(l, 0, size() + 1), check<B>(r, 0, size() + 1);
    static constexpr auto f = [](Lazy const& x, ptr n, bool) { n->lazy = x; };
    if constexpr(B) st = range_key(l, r, v, f);
    else st = range_id(l, r, v, f);
    while(!st.empty()) [[likely]] {
      ptr n = st.top();
      st.pop();
      n->val = Q(n->ch[0]->eval()->val, n->ch[1]->eval()->val);
    }
  }
  lazy_RBT_func(apply, void, idx<B>, idx<B>, Lazy const&);
  constexpr stack<ptr> range_key(Key const& l, Key const& r, auto& x, auto const& f) {
    if(!root) [[unlikely]]
      return stack<ptr>{};
    queue<pair<ptr, bool>> q;
    stack<ptr> st;
    q.emplace(root, false);
    while(!q.empty()) [[likely]] {
      auto [n, b] = q.front();
      q.pop();
      if(!comp(*n->eval()->end[0]->key, l) && comp(*n->end[1]->key, r)) f(x, n, b);
      else if(!(comp(*n->end[1]->key, l) || !comp(*n->end[0]->key, r)))
        st.emplace(n), q.emplace(n->ch[0], false), q.emplace(n->ch[1], true);
    }
    return st;
  }
  constexpr stack<ptr> range_id(int l, int r, auto& x, auto const& f) {
    if(!root) [[unlikely]]
      return stack<ptr>{};
    queue<tuple<ptr, int, int, bool>> q;
    stack<ptr> st;
    q.emplace(root, l, r, false);
    while(!q.empty()) [[likely]] {
      auto [n, _l, _r, b] = q.front();
      q.pop();
      int sz = n->eval()->sz;
      if(_l <= 0 && sz <= _r) f(x, n, b);
      else if(!(sz <= _l || _r <= 0)) {
        st.emplace(n);
        q.emplace(n->ch[0], _l, _r, false), q.emplace(n->ch[1], _l - n->ch[0]->sz, _r - n->ch[0]->sz, true);
      }
    }
    return st;
  }
  // vectorの中身はValue const&ではなくreference_wrapper<Value const>なことに注意.
  constexpr vector<reference_wrapper<Value const>> get_vec_val() {
    vector<reference_wrapper<Value const>> ret;
    static constexpr auto f = [](ptr n) { return cref(n->val); };
    get_vec_sub(ret, f);
    return ret;
  }
  constexpr vector<pair<Key const&, Value const&>> get_vec() {
    vector<pair<Key const&, Value const&>> ret;
    static constexpr auto f = [&](ptr n) { return n->to_pair(); };
    get_vec_sub(ret, f);
    return ret;
  }
  constexpr void get_vec_sub(auto& v, auto const& f) {
    if(!root) [[unlikely]]
      return;
    v.reserve(size());
    stack<ptr> st;
    st.emplace(root);
    while(!st.empty()) [[likely]] {
      ptr n = st.top();
      st.pop();
      if(n->eval()->is_leaf()) v.emplace_back(f(n));
      else st.emplace(n->ch[1]), st.emplace(n->ch[0]);
    }
  }

  // private想定.
  // O(1)
  constexpr ptr rotate(ptr n, bool dir) {
    ptr r = n->ch[1 ^ dir];
    r->eval();
    n->ch[1 ^ dir] = r->ch[dir];
    r->ch[dir] = n;
    if(n == root) [[unlikely]]
      root = r;
    n->update();
    return r->update();
  }

  struct node {
    optional<Key> key{};
    Value val = Value_e;
    bool rev = false;
    Lazy lazy = Lazy_e;
    bool black = false;
    int rank = 0, sz = 0; // rank=黒深さ.
    ptr ch[2] = {nullptr, nullptr}, end[2] = {nullptr, nullptr};
    constexpr node() {}
    template<convertible_to<optional<Key>> K, convertible_to<Value> V>
    constexpr node(K&& _key, V&& _val)
        : key(forward<K>(_key)), val(forward<V>(_val)), black(true), rank(1), sz(1), end{this, this} {}
    constexpr node(ptr l, ptr r): ch{l, r} { update(); }

    constexpr bool is_leaf() const { return !ch[0]; }
    constexpr ptr eval() {
      if(!rev && lazy == Lazy_e) return this;
      if(rev) swap(ch[0], ch[1]), swap(end[0], end[1]);
      val = effect(lazy, val, sz);
      if(!is_leaf()) [[likely]]
        rep(i, 2) ch[i]->rev ^= rev, ch[i]->lazy = composite(lazy, ch[i]->lazy);
      rev = false, lazy = Lazy_e;
      return this;
    }
    constexpr ptr paint(bool bl) {
      if(black == bl) return this;
      black = bl;
      if(bl) ++rank;
      else --rank;
      return this;
    }
    constexpr ptr update() {
      assert(!is_leaf());
      rep(i, 2) ch[i]->eval();
      rank = ch[0]->rank + black;
      sz = ch[0]->sz + ch[1]->sz;
      val = Q(ch[0]->val, ch[1]->val);
      rep(i, 2) end[i] = ch[i]->end[i];
      return this;
    }
    constexpr pair<Key const&, Value const&> to_pair() const {
      assert(key);
      return pair<Key const&, Value const&>(*key, val);
    }

    template<bool B>
    constexpr bool down_left(idx<B> k) const {
      if constexpr(B) return comp(k, *ch[1]->end[0]->key);
      else return k < ch[0]->sz;
    }
    template<bool B>
    constexpr bool split_now(idx<B> k) const {
      if constexpr(B) return comp(*ch[0]->end[1]->key, k) && !comp(*ch[1]->end[0]->key, k);
      else return k == ch[0]->sz;
    }
    template<bool B>
    constexpr void change(idx_ref<B> k) const {
      if constexpr(!B) k = k - ch[0]->sz;
    }
    template<bool B>
    constexpr bool left(idx<B> k) const {
      if constexpr(B) return comp(k, *key);
      else return k == -1;
    }
    template<bool B>
    constexpr bool right(idx<B> k) const {
      if constexpr(B) return comp(*key, k);
      else return k == 1;
    }
  };
};