# RBT (Red-Black Tree)

[Source](../src/self_balancing_BST/RedBlackTree.hpp)

## 概要

キーと値のペアを管理する平衡二分探索木（赤黒木）。キーの順序を保ちながら挿入・削除・検索を $O(\log N)$ で行う。各ノードに値を持ち、区間積クエリ（モノイド演算）や二分探索もサポートする。

競プロでの主な用途：

- 順序付き集合・写像（`insert` / `erase` / `find`）
- 区間積クエリ（`prod`）
- 二分探索（`binary_right` / `binary_left`）
- 木の分割・合併（`split` / `merge` / `meld`）

**注意点：**

- `merge` と `meld` は異なる操作である。`merge` は2つのRBTをキー順序を保ちながら連結し（キーの重複不可）、`meld` は2つのRBTをキーが重複した場合に `f` で値をマージしながら合体する。
- `binary_left` は `acl::segtree.min_left` とは仕様が異なる。
- 内部で `memory_pool` を使用しており、`pool` は `static` メンバなので同じテンプレートパラメータを持つ全インスタンスがプールを共有する。`clear()` を呼ぶとメモリがプールに返却される。

値の型 `Value` に対してテンプレートパラメータ `Q` と `f_e` が $(S, \cdot, e)$ のモノイドを形成していること、すなわち

- 結合律：$Q(a, Q(b, c)) = Q(Q(a, b), c)$
- 単位元：$Q(a, e) = Q(e, a) = a$ （$e$ = `f_e()`）

が成り立つことを要求する。

## テンプレートパラメータ

| パラメータ | デフォルト | 説明 |
|---|---|---|
| `Key` | — | キーの型 |
| `Value` | `Key` | 値の型（モノイドの台集合） |
| `Q` | `default_Q<Value>` (= `a + b`) | モノイド演算 $\cdot$。`Value(Value, Value)` として呼び出せること |
| `f_e` | `default_e<Value>` (= `Value{}`) | 単位元 $e$ を返す関数。`Value()` として呼び出せること |
| `comp` | `less<Key>{}` | キー比較関数（狭義全順序）。`bool(Key, Key)` として呼び出せること |

## 内部型

### node

木の各ノードを表す構造体。

| フィールド | 型 | 説明 |
|---|---|---|
| `key` | `Key` | キー |
| `val` | `Value` | このノードの値 |
| `que` | `Value` | この部分木全体の積（`Q` で畳み込んだ値） |
| `sz` | `int` | この部分木のノード数 |
| `min_ptr` | `ptr` | この部分木の最小キーノードへのポインタ |
| `max_ptr` | `ptr` | この部分木の最大キーノードへのポインタ |
| `par` | `ptr` | 親ノードへのポインタ |
| `ch[2]` | `ptr[2]` | 左・右の子ノードへのポインタ |
| `black` | `bool` | 黒ノードなら `true` |
| `rank` | `int` | 黒深さ |

**注意：** `ptr` 経由でフィールドを直接書き換えると木の整合性が壊れる。`val` を変更した場合は祖先ノードの `que` / `min_ptr` / `max_ptr` が更新されないため、`prod` や `min_element` / `max_element` の結果が不正になる。`key` の変更は BST 順序を壊す。

### ptr

```cpp
using ptr = node*;
```

`node` へのポインタ型。`find_ptr` / `kth_ptr` / `merge` / `split` 等の戻り値・引数で使用する。存在しないノードは `null`（番兵ノード）で表される。

## コンストラクタ

```cpp
RBT<Key, Value, Q, f_e> rbt;
RBT<Key, Value, Q, f_e> rbt(vector<pair<Key, Value>> v);
RBT<Key, Value, Q, f_e> rbt(vector<Value> const& v);
```

- `rbt()` : 空の状態で初期化する。
- `rbt(v)` : キーと値のペアの配列 `v` から構築する。ソートして $O(N)$ で構築。
- `rbt(v)` : 値の配列から構築する。キーはインデックス（0-indexed）になる。

**計算量**

- $O(N)$

**使用例**

```cpp
RBT<int> rbt;
RBT<int, int> rbt2(vector<pair<int,int>>{{1, 5}, {2, 8}, {3, 10}});
```

## 関数

### 基本操作

#### size

```cpp
int rbt.size()
```

要素数を返す。

**計算量**

- $O(\log N)$（内部でlazyな更新を評価する）

**使用例**

```cpp
rbt.insert(1, 10);
int s = rbt.size(); // 1
```

---

#### all_prod

```cpp
Value rbt.all_prod()
```

全要素の値の積 $Q(v_1, v_2, \ldots, v_N)$ をキー昇順で返す。木が空の場合は単位元 `e` を返す。

**計算量**

- $O(\log N)$

**使用例**

```cpp
RBT<int> rbt(vector<pair<int,int>>{{1,2},{2,3},{3,4}});
int total = rbt.all_prod(); // 2+3+4 = 9
```

---

#### min_element

```cpp
pair<Key, Value> rbt.min_element()
pair<Key, Value> rbt.min_element(ptr n)
```

最小キーを持つノードの `{key, value}` を返す。`n` を指定するとその部分木の最小を返す。木が空の場合は assert で落ちる。

**制約**

- 木が空であってはならない

**計算量**

- $O(\log N)$（内部でlazyな更新を評価する）

**使用例**

```cpp
auto [k, v] = rbt.min_element(); // キー最小の要素
```

---

#### max_element

```cpp
pair<Key, Value> rbt.max_element()
pair<Key, Value> rbt.max_element(ptr n)
```

最大キーを持つノードの `{key, value}` を返す。`n` を指定するとその部分木の最大を返す。木が空の場合は assert で落ちる。

**制約**

- 木が空であってはならない

**計算量**

- $O(\log N)$（内部でlazyな更新を評価する）

**使用例**

```cpp
auto [k, v] = rbt.max_element(); // キー最大の要素
```

---

### 検索系

#### find

```cpp
int rbt.find(Key const& k)
```

キー `k` が昇順で何番目か（0-indexed）を返す。存在しない場合は `-1` を返す。

**計算量**

- $O(\log N)$

**使用例**

```cpp
rbt.insert(3, 10); rbt.insert(5, 20);
int idx = rbt.find(5); // 1
int ng  = rbt.find(4); // -1
```

---

#### find_ptr

```cpp
ptr rbt.find_ptr(Key const& k)
```

キー `k` に対応するノードのポインタを返す。存在しない場合は `null` を返す。

**計算量**

- $O(\log N)$

---

#### kth

```cpp
pair<Key, Value> rbt.kth(int k)
```

昇順で `k` 番目（0-indexed）の `{key, value}` を返す。

**制約**

- $0 \leq k < \mathrm{size()}$

**計算量**

- $O(\log N)$

**使用例**

```cpp
auto [key, val] = rbt.kth(0); // 最小要素
```

---

#### kth_ptr

```cpp
ptr rbt.kth_ptr(int k)
```

昇順で `k` 番目（0-indexed）のノードのポインタを返す。

**制約**

- $0 \leq k < \mathrm{size()}$

**計算量**

- $O(\log N)$

---

### 挿入・更新

#### insert

```cpp
ptr rbt.insert(Key const& k, Value const& v = e)
```

キー `k`、値 `v` のノードを挿入し、そのノードのポインタを返す。キー `k` が既に存在する場合は挿入せず、既存ノードのポインタを返す。

**計算量**

- $O(\log N)$

**使用例**

```cpp
rbt.insert(3, 10);
rbt.insert(3, 99); // 何もしない。既存の{3,10}が返る
```

---

#### operator[]

```cpp
Value& rbt.operator[](Key const& k)
```

キー `k` に対応する値への参照を返す。存在しない場合はデフォルト値 `e` で挿入してから返す。

**計算量**

- $O(\log N)$

**使用例**

```cpp
rbt[5] = 100;
int v = rbt[5]; // 100
```

---

### 削除

#### erase

```cpp
bool rbt.erase(Key const& k)
```

キー `k` のノードを削除する。削除に成功した場合は `true`、キーが存在しなかった場合は `false` を返す。

**計算量**

- $O(\log N)$

**使用例**

```cpp
rbt.insert(3, 10);
bool ok = rbt.erase(3); // true
bool ng = rbt.erase(3); // false
```

---

### 区間積クエリ

#### prod

```cpp
Value rbt.prod(Key const& l, Key const& r)
```

キーが $[l, r)$ の範囲にある全ノードの値の積を返す。範囲が空の場合は単位元 `e` を返す。

**制約**

- $l \leq r$

**計算量**

- $O(\log N)$

**使用例**

```cpp
// {1:2, 2:3, 3:4} の [1,3) = {1:2, 2:3} の和
int s = rbt.prod(1, 3); // 5
```

---

### 二分探索

#### binary_right

```cpp
Key rbt.binary_right(auto const& f, Key const& l = numeric_limits<Key>::min(), Key const& INF = numeric_limits<Key>::max())
```

`f([l, r))` が `true` かつ `f([l, r])` が `false` になる `r` の一つを返す。そのような `r` が存在しない（全範囲で `f` が `true`）場合は `INF` を返す。

**制約**

- `f(e)` が `true` であること（単位元で `f` が成立すること）

**計算量**

- $O(\log N)$

**使用例**

```cpp
// 和が10未満になる最右端のキー
auto f = [](int v){ return v < 10; };
Key r = rbt.binary_right(f, 1); // [1, r) の和が10未満、[1, r] の和が10以上
```

---

#### binary_left

```cpp
Key rbt.binary_left(auto const& f, Key const& r = numeric_limits<Key>::max(), Key const& mINF = numeric_limits<Key>::min())
```

`f((l, r])` が `true` かつ `f([l, r])` が `false` になる `l` の一つを返す。そのような `l` が存在しない場合は `mINF` を返す。

**注意**：`acl::segtree.min_left` とは仕様が異なる。

**制約**

- `f(e)` が `true` であること

**計算量**

- $O(\log N)$

---

### 木の分割・合併

#### merge

```cpp
ptr rbt.merge(ptr l, ptr r)
```

2つの部分木ポインタ `l`, `r` をこの順に連結し、マージ後の根ポインタを返す。`l` の全キー < `r` の全キー である必要がある（重複不可）。

**制約**

- `l` の最大キー < `r` の最小キー

**計算量**

- $O(\log N)$

---

#### split

```cpp
pair<ptr, ptr> rbt.split(ptr n, Key const& k)
```

部分木 `n` をキー $[-\infty, k)$ と $[k, \infty)$ の2つに分割し、`{左の木, 右の木}` を返す。

**計算量**

- $O(\log N)$

---

#### meld

```cpp
RBT& rbt.meld(RBT& ot, function<Value(Value, Value)> const& f = Q)
```

別の RBT `ot` を `rbt` にマージする（`ot` は空になる）。キーが重複した場合、値を `f(rbt側の値, ot側の値)` で合成する。`merge` と異なりキーの重複を許す。

**計算量**

- $N = \min(\mathrm{size}(), \mathrm{ot.size}())$、$M = \max(\mathrm{size}(), \mathrm{ot.size}())$ として $O(N \log(1 + M/N))$

**使用例**

```cpp
RBT<int> a({{1,10},{3,30}}), b({{2,20},{3,99}});
a.meld(b); // a = {1:10, 2:20, 3:30+99=129} (デフォルトは加算)
```

---

### その他

#### all_get

```cpp
vector<pair<Key, Value>> rbt.all_get()
vector<pair<Key, Value>> rbt.all_get(ptr n)
```

全ノードの `{key, value}` をキー昇順で返す。`n` を指定するとその部分木のみ。

**計算量**

- $O(N)$

**使用例**

```cpp
auto v = rbt.all_get();
for(auto [k, val] : v) { /* ... */ }
```

---

#### swap

```cpp
RBT& rbt.swap(RBT& ot)
```

2つの RBT の内容を $O(1)$ で交換する。フリー関数 `swap(a, b)` でも呼び出せる。

**計算量**

- $O(1)$

---

#### clear

```cpp
void rbt.clear()
```

全ノードを解放して空の状態に戻す。メモリは `memory_pool` に返却される。

**計算量**

- $O(N)$
