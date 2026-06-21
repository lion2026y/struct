# line_segment

[Source](../src/geometry/line_segment.hpp)

## 概要

2次元線分を表す構造体。始点 `s` と終点 `t` を持つ。`point` に依存する。

競プロでの主な用途：点と線分の距離計算、線分の方向ベクトル取得。

注意: 直線ではなく**線分**として扱う（コメント `// 点と線分の距離(直線ではないことに注意)` 参照）。

## テンプレートパラメータ

| パラメータ | デフォルト | 説明 |
|---|---|---|
| `T` | `long double` | 座標の型。`std::floating_point` を満たす型のみ使用可能（コンセプト制約） |

## コンストラクタ

```cpp
line_segment<T> seg(point<T> t)
line_segment<T> seg(point<T> s, point<T> t)
```

- `seg(t)` : 始点 $(0, 0)$、終点 `t` で初期化する。
- `seg(s, t)` : 始点 `s`、終点 `t` で初期化する。

**計算量**

- $O(1)$

**使用例**

```cpp
point<long double> a(0, 0), b(3, 4);
line_segment<> seg(a, b);
```

## 関数

### dir

```cpp
point<T> obj.dir()
```

線分の方向ベクトル `t - s` を返す。

**計算量**

- $O(1)$

---

### length

```cpp
T obj.length()
```

線分の長さ $|t - s|$ を返す。

**計算量**

- $O(1)$

---

### unit

```cpp
point<T> obj.unit()
```

線分の方向の単位ベクトルを返す。

**制約**

- 長さが $0$ でないこと

**計算量**

- $O(1)$

---

### dist

```cpp
T obj.dist(point<T> p = {})
```

点 `p` から線分までの最短距離を返す（直線ではなく線分）。`p` を省略すると原点からの距離を返す。

- 垂線の足が線分の内側に落ちる場合：垂線距離 $|cross(s-p, t-p)| / \mathrm{length()}$
- 落ちない場合：端点 `s` または `t` までの距離

**計算量**

- $O(1)$

**使用例**

```cpp
point<long double> a(0, 0), b(4, 0);
line_segment<> seg(a, b);
point<long double> p(2, 3);
long double d = seg.dist(p); // 3.0
```
