#pragma once
#include "memory_pool.hpp"
#include <bits/stdc++.h>
using namespace std;

#define rep(i, n) for(int i = 0; i < int(n); ++i)

// Red-Black Tree
// need memory_pool,rep
// merge,meldに注意.
// binary_leftはacl::segtree.min_leftとは仕様が違うことに注意.
template<class T>
constexpr T default_Q(T const& a, T const& b) { return a + b; }
template<class T>
constexpr T default_e() { return T{}; }
template<class Key, class Value = Key, auto Q = default_Q<Value>, auto f_e = default_e<Value>,
         auto comp = less<Key>{}>
struct RBT {
  static_assert(is_invocable_r_v<Value, decltype(Q), Value, Value>, "Q must work as Value(Value,Value)");
  static_assert(is_invocable_r_v<Value, decltype(f_e)>, "f_e must work as Value()");
  static_assert(is_invocable_r_v<bool, decltype(comp), Key, Key>, "comp must work as bool(Key,Key)");

  struct node {
    Key key;
    Value val;
    bool black;
    int rank, sz; // rank=黒深さ.
    Value que;
    node *min_ptr, *max_ptr;
    node* par;
    node* ch[2];
    constexpr node()
        : key(Key{}), val(e), black(true), rank(1), sz(0), que(e),
          min_ptr(nullptr), max_ptr(nullptr), par(nullptr), ch{nullptr, nullptr} {}
    constexpr node(Key const& _key, Value const& _val, bool _black, node* n)
        : key(_key), val(_val), black(_black), rank(black + 1), sz(1), que(val),
          min_ptr(this), max_ptr(this), par(n), ch{n, n} {}
    constexpr node& operator=(node&& ot) noexcept {
      if(this == &ot) return *this;
      key = ot.key, val = ot.val, black = ot.black, rank = ot.rank, sz = ot.sz, que = ot.que;
      min_ptr = ot.min_ptr, max_ptr = ot.max_ptr, par = ot.par, ch[0] = ot.ch[0], ch[1] = ot.ch[1];
      if(min_ptr == &ot) min_ptr = this;
      if(max_ptr == &ot) max_ptr = this;
      ot.min_ptr = ot.max_ptr = ot.par = ot.ch[0] = ot.ch[1] = nullptr;
      return *this;
    }

    constexpr void paint(bool bl) {
      if(black == bl) return;
      black = bl;
      if(bl) rank++;
      else rank--;
    }
  };

  using ptr = node*;
  static inline Value const e = f_e();
  static inline memory_pool<node> pool;
  static inline ptr const null = pool.alloc();
  ptr root, lazy;
  constexpr RBT(): root(null), lazy(null) {}
  constexpr RBT(RBT&& ot): root(ot.root), lazy(ot.lazy) { ot.root = ot.lazy = nullptr; }
  constexpr RBT(vector<pair<Key, Value>> v): lazy(null) {
    sort(v.begin(), v.end());
    root = set(v, 0, v.size(), [&](int, pair<Key, Value> const& p) { return pool.alloc(p.first, p.second, true, null); });
  }
  constexpr RBT(vector<Value> const& v): lazy(null) {
    root = set(v, 0, v.size(), [&](int x, Value const& _val) { return pool.alloc(x, _val, true, null); });
  }
  template<class T>
  constexpr ptr set(vector<T> const& v, int l, int r, auto const& f) {
    if(l == r) return null;
    int m = (l + r) / 2;
    ptr n = f(m, v[m]);
    if(l + 1 == r) return n;
    return merge3(set(v, l, m, f), n, set(v, m + 1, r, f));
  }
  constexpr RBT& operator=(RBT&& ot) noexcept {
    if(this == &ot) return *this;
    clear();
    root = ot.root;
    lazy = ot.lazy;
    ot.root = ot.lazy = nullptr;
    return *this;
  }

  constexpr int size() {
    eval();
    return root->sz;
  }
  constexpr Value all_prod() {
    eval();
    return root->que;
  }
  constexpr pair<Key, Value> min_element() { return min_element(root); }
  constexpr pair<Key, Value> min_element(ptr n) {
    eval();
    assert(n != null && "This requires elements.");
    return to_pair(n->min_ptr);
  }
  constexpr pair<Key, Value> max_element() { return max_element(root); }
  constexpr pair<Key, Value> max_element(ptr n) {
    eval();
    assert(n != null && "This requires elements.");
    return to_pair(n->max_ptr);
  }
  // O(logN)
  // key=kなるptrを返す.もしなければnullを返す.cntは小さい方から何番目か(0-indeed)
  constexpr ptr find_ptr(Key const& k) {
    int t = 0;
    return find_ptr(k, t);
  }
  constexpr ptr find_ptr(Key const& k, int& cnt) {
    eval();
    ptr n = root;
    while(n != null) {
      if(comp(k, n->key)) n = n->ch[0];
      else if(comp(n->key, k)) {
        cnt += n->ch[0]->sz + 1;
        n = n->ch[1];
      } else {
        cnt += n->ch[0]->sz;
        return n;
      }
    }
    return null;
  }
  // O(logN)
  // kが小さい方から何番目か(0-indexed)を返す.
  constexpr int find(Key const& k) {
    int ret = 0;
    if(find_ptr(k, ret) == null) return -1;
    return ret;
  }
  constexpr pair<Key, Value> kth(int k) {
    ptr n = kth_sub(k);
    return pair<Key, Value>(n->key, n->val);
  }
  constexpr ptr kth_ptr(int k) {
    ptr n = kth_sub(k);
    lazy = n;
    return n;
  }
  // O(logN)
  //{k,v}を挿入する.もし既にkが存在したら何もしない.key=kなるptrを返す.
  constexpr ptr insert(Key const& k, Value const& v = e) {
    eval();
    ptr n = root;
    if(n == null) {
      ptr ret = pool.alloc(k, v, true, null);
      root = ret;
      lazy = ret;
      return ret;
    }
    while(true) {
      ptr c;
      if(comp(k, n->key)) c = n->ch[0];
      else if(comp(n->key, k)) c = n->ch[1];
      else {
        lazy = n;
        return n;
      }
      if(c == null) break;
      n = c;
    }
    ptr nw = pool.alloc(k, v, false, null);
    bool dir = comp(n->key, k);
    n->ch[dir] = nw;
    nw->par = n;

    n = nw;
    lazy = nw;
    while(true) {
      if(root == n) {
        n->paint(true);
        break;
      }
      ptr p = n->par;
      if(p->black) break;
      ptr g = p->par;
      if(g == null) {
        p->paint(true);
        break;
      }
      bool p_dir = get_dir(p);
      ptr u = g->ch[1 ^ p_dir];
      if(!u->black) {
        p->paint(true);
        u->paint(true);
        g->paint(false);
        n = g;
        continue;
      }
      bool n_dir = get_dir(n);
      if(n_dir != p_dir) {
        rotate(p, p_dir);
        swap(p, n);
      }
      g->paint(false);
      p->paint(true);
      rotate(g, 1 ^ p_dir);
      break;
    }
    return nw;
  }
  // O(logN)
  constexpr Value& operator[](Key const& k) { return insert(k)->val; }
  // O(logN)
  constexpr bool erase(Key const& k) {
    ptr t = find_ptr(k);
    if(t == null) return false;
    bool e_dir = t->ch[0] == null;
    if(!e_dir) {
      ptr mx = t->ch[0]->max_ptr;
      t->key = mx->key;
      t->val = mx->val;
      t = mx;
    }
    ptr n = t->ch[e_dir], p = t->par;
    bool dir = get_dir(t), bl = t->black;
    if(t == root) {
      pool.free(t);
      root = n;
      if(n != null) {
        n->paint(true);
        n->par = null;
      }
      return true;
    }
    pool.free(t);
    lazy = p;
    p->ch[dir] = n;
    if(n != null) n->par = p;
    if(!bl) return true;
    if(!n->black) {
      n->paint(true);
      return true;
    }

    while(true) {
      if(n == root) break;
      if(n != null) {
        p = n->par;
        dir = get_dir(n);
      }
      ptr b = p->ch[1 ^ dir], bc[2]{b->ch[0], b->ch[1]};
      if(!p->black && bc[0]->black && bc[1]->black) {
        b->paint(false);
        p->paint(true);
        break;
      }
      if(b->black && !bc[dir]->black && bc[1 ^ dir]->black) {
        bc[dir]->paint(true);
        b->paint(false);
        rotate(b, 1 ^ dir);
        bc[1 ^ dir] = b;
        b = bc[dir];
        bc[dir] = bc[dir]->ch[dir];
      }
      if(b->black && !bc[1 ^ dir]->black) {
        bc[1 ^ dir]->paint(true);
        b->paint(p->black);
        p->paint(true);
        rotate(p, dir);
        break;
      }
      if(!b->black) {
        b->paint(true);
        p->paint(false);
        rotate(p, dir);
        continue;
      }
      b->paint(false);
      n = p;
    }
    return true;
  }
  // O(logN)
  // l,rをこの順にmergeする.
  constexpr ptr merge(ptr l, ptr r) {
    if(l == null) return r;
    if(r == null) return l;
    auto [lt, m, nl] = split3(l, max_element(l).first);
    return merge3(lt, m, r);
  }
  // O(logN)
  //[-inf,k),[k,inf)に分ける.
  constexpr pair<ptr, ptr> split(ptr n, Key const& k) {
    auto [l, m, r] = split3(n, k);
    if(m == null) return pair<ptr, ptr>(l, r);
    return pair<ptr, ptr>(l, merge3(null, m, r));
  }
  // N=min,M=maxとしてO(Nlog(1+M/N))
  // 2つのRBTをmeldする.片方は初期化される.
  // fはkeyが同じnodeのvalをどうするか.
  constexpr RBT& meld(RBT& ot, function<Value(Value, Value)> const& f = Q) {
    if(this == &ot) return *this;
    eval();
    ot.eval();
    root = meld_sub(root, ot.root, f);
    ot.root = ot.lazy = null;
    return *this;
  }
  // O(logN)
  //[l,r)の総クエリを返す.
  constexpr Value prod(Key const& l, Key const& r) {
    eval();
    auto [out_l, ls, t] = split3(root, l);
    auto [x, rs, out_r] = split3(t, r);
    Value ret = Q(ls->val, x->que);
    if(rs == null) t = merge(x, out_r);
    else t = merge3(x, rs, out_r);
    if(ls == null) root = merge(out_l, t);
    else root = merge3(out_l, ls, t);
    return ret;
  }
  // O(logN)
  //(f([l,r))=true||r=l)&&(f([l,r])=false||r=INF)となるrの一つを返す.
  // constraint f(e)=true
  constexpr Key binary_right(auto const& f, Key const& l = numeric_limits<Key>::min(), Key const& INF = numeric_limits<Key>::max()) {
    assert(f(e));
    eval();
    Value v = e;
    ptr ret = right_sub(f, l, root, v);
    if(f(v)) return INF;
    else return ret->key;
  }
  // O(logN)
  //(f((l,r])=true||l=r)&&(f([l,r])=false||l=mINF)となるlの一つを返す.
  // constraint f(e)=true
  // acl::segtree.min_leftとは仕様が違うことに注意.
  constexpr Key binary_left(auto const& f, Key const& r = numeric_limits<Key>::max(), Key const& mINF = numeric_limits<Key>::min()) {
    assert(f(e));
    eval();
    Value v = e;
    ptr ret = left_sub(f, r, root, v);
    if(f(v)) return mINF;
    else return ret->key;
  }
  // O(N)
  constexpr vector<pair<Key, Value>> all_get() { return all_get(root); }
  constexpr vector<pair<Key, Value>> all_get(ptr n) {
    eval();
    vector<pair<Key, Value>> ret;
    all_get_sub(ret, n);
    return ret;
  }
  constexpr RBT& swap(RBT& ot) {
    if(this == &ot) return *this;
    swap(root, ot.root);
    swap(lazy, ot.lazy);
    return *this;
  }
  template<class T1>
  constexpr void swap(T1& a, T1& b) const { std::swap(a, b); }
  // メモリの解放.
  constexpr void clear() {
    clear_sub(root);
    root = lazy = null;
  }

  constexpr bool operator<(RBT& ot) { return size() < ot.size(); }

  // private想定.
  constexpr bool eq(Key a, Key b) { return !comp(a, b) && !comp(b, a); }
  constexpr void update(ptr p) {
    p->rank = p->ch[0]->rank + p->black;
    p->sz = 1 + p->ch[0]->sz + p->ch[1]->sz;
    p->que = Q(p->ch[0]->que, Q(p->val, p->ch[1]->que));
    p->min_ptr = p->ch[0] == null ? p : p->ch[0]->min_ptr;
    p->max_ptr = p->ch[1] == null ? p : p->ch[1]->max_ptr;
  }
  constexpr bool get_dir(ptr const& p) const { return p->par->ch[1] == p; }
  constexpr void link(ptr p, ptr c, bool const& dir) {
    if(p != null) { p->ch[dir] = c; }
    if(c != null) { c->par = p; }
  }
  constexpr void cut(ptr p, ptr c, bool const& dir) {
    if(p != null) { p->ch[dir] = null; }
    if(c != null) { c->par = null; }
  }
  // O(1)
  constexpr ptr rotate(ptr n, bool const& dir) {
    ptr r = n->ch[1 ^ dir], p = n->par;
    bool c = get_dir(n);
    n->ch[1 ^ dir] = r->ch[dir];
    if(r->ch[dir] != null) r->ch[dir]->par = n;
    r->ch[dir] = n;
    n->par = r;
    if(p != null) p->ch[c] = r;
    r->par = p;
    if(n == root) root = r;
    update(n);
    update(r);
    return r;
  }
  // O(logN)
  constexpr void eval() {
    while(lazy != null) {
      update(lazy);
      lazy = lazy->par;
    }
  }
  constexpr pair<Key, Value> to_pair(ptr n) { return pair<Key, Value>(n->key, n->val); }
  // O(logN)
  // 小さい方からk番目(0-indexed)を返す.
  constexpr ptr kth_sub(int k) {
    eval();
    ptr n = root;
    assert(0 <= k && k < n->sz && "This index is out of range.");
    while(true) {
      int s = n->ch[0]->sz;
      if(s == k) return n;
      if(s > k) n = n->ch[0];
      else {
        k -= s + 1;
        n = n->ch[1];
      }
    }
  }
  // O(log|l->rank-r->rank|)
  // l,k(単一ノード),rをmergeする.mergeした根のポインタを返す.
  constexpr ptr merge3(ptr l, ptr k, ptr r) {
    if(l != null) l->paint(true);
    if(r != null) r->paint(true);
    k->paint(false);
    if(l->rank == r->rank) {
      link(k, l, 0);
      link(k, r, 1);
      k->paint(true);
      update(k);
      return k;
    }
    ptr n;
    bool dir = 0;
    if(l->rank > r->rank) {
      while(l->ch[1]->rank != r->rank) l = l->ch[1];
      link(k, l->ch[1], 0);
      link(k, r, 1);
      link(l, k, 1);
      n = k->ch[0];
      if(!n->black && l->ch[0]->black) {
        rotate(k, 1);
        n = k;
      }
      if(n == null) n = k;
    } else {
      while(l->rank != r->ch[0]->rank) r = r->ch[0];
      link(k, r->ch[0], 1);
      link(k, l, 0);
      link(r, k, 0);
      n = k->ch[1];
      if(!n->black && r->ch[1]->black) {
        rotate(k, 0);
        n = k;
      }
      if(n == null) n = k;
      dir = 1;
    }
    if(!n->black) {
      while(!n->par->black && n->par->par != null && !n->par->par->ch[dir]->black) {
        ptr g = n->par->par;
        g->ch[0]->paint(true);
        g->ch[1]->paint(true);
        update(g->ch[1 ^ dir]);
        g->paint(false);
        update(g);
        n = g;
      }
      if(!n->par->black) {
        ptr p = n->par;
        if(p->par == null) {
          p->paint(true);
          n = p;
        } else {
          ptr g = p->par;
          p->paint(true);
          g->paint(false);
          rotate(g, dir);
          n = p;
        }
      }
    }
    while(n->par != null) {
      update(n);
      n = n->par;
    }
    update(n);
    return n;
  }
  constexpr tuple<ptr, ptr, ptr> split3(ptr n, Key const& k) {
    if(n == null) return tuple<ptr, ptr, ptr>(null, null, null);
    ptr lc = n->ch[0], rc = n->ch[1];
    cut(n, lc, 0);
    cut(n, rc, 1);
    update(n);
    if(comp(k, n->key)) {
      auto [l, m, r] = split3(lc, k);
      return tuple<ptr, ptr, ptr>(l, m, merge3(r, n, rc));
    }
    if(comp(n->key, k)) {
      auto [l, m, r] = split3(rc, k);
      return tuple<ptr, ptr, ptr>(merge3(lc, n, l), m, r);
    }
    return tuple<ptr, ptr, ptr>(lc, n, rc);
  }
  constexpr ptr meld_sub(ptr a, ptr b, function<Value(Value, Value)> const& f = Q) {
    if(a == null) return b;
    if(b == null) return a;
    // if(a->sz>b->sz) swap(a,b);//要検討.
    ptr lc = a->ch[0], rc = a->ch[1];
    cut(a, lc, 0);
    cut(a, rc, 1);
    auto [l, m, r] = split3(b, a->key);
    if(m != null) a->val = f(a->val, m->val);
    return merge3(meld_sub(lc, l), a, meld_sub(rc, r));
  }
  constexpr ptr right_sub(auto const& f, Key const& l, ptr n, Value& v) {
    if(n == null) return n;
    if(comp(n->key, l)) return right_sub(f, l, n->ch[1], v);
    Value x = Q(v, n->que);
    if(!comp(n->min_ptr->key, l) && f(x)) {
      v = x;
      return n;
    }
    ptr ret = right_sub(f, l, n->ch[0], v);
    if(!f(v)) return ret;
    v = Q(v, n->val);
    if(!f(v)) return n;
    return right_sub(f, l, n->ch[1], v);
  }
  constexpr ptr left_sub(auto const& f, Key const& r, ptr n, Value& v) {
    if(n == null) return n;
    if(comp(r, n->key)) return left_sub(f, r, n->ch[0], v);
    Value x = Q(n->que, v);
    if(!comp(r, n->max_ptr->key) && f(x)) {
      v = x;
      return n;
    }
    ptr ret = left_sub(f, r, n->ch[1], v);
    if(!f(v)) return ret;
    v = Q(n->val, v);
    if(!f(v)) return n;
    return left_sub(f, r, n->ch[0], v);
  }
  // O(N)
  constexpr void all_get_sub(vector<pair<Key, Value>>& ret, ptr p) const {
    if(p == null) return;
    all_get_sub(ret, p->ch[0]);
    ret.emplace_back(p->key, p->val);
    all_get_sub(ret, p->ch[1]);
  }
  constexpr void clear_sub(ptr n) {
    if(n == null) { return; }
    rep(i, 2) { clear_sub(n->ch[i]); }
    pool.free(n);
  }
};
template<class Key, class Value, auto Q, auto f_e, auto comp>
RBT<Key, Value, Q, f_e, comp>& swap(RBT<Key, Value, Q, f_e, comp>& a, RBT<Key, Value, Q, f_e, comp>& b) { return a.swap(b); }