# grid_transform

[Source](../src/grid/grid_transform.hpp)

## 概要

2次元の `random_access_range`（主に `vector<vector<T>>`）に対して、転置・90度回転を行う関数テンプレート群。入力が非矩形（行ごとにサイズが異なる）の場合は `assert` で落ちる。

## テンプレートパラメータ

| パラメータ | デフォルト | 説明 |
|---|---|---|
| `R` | — | 外側のコンテナ型。`random_access_range` かつ `R(int, Inner)` で構築できること |
| `Inner` | `ranges::range_value_t<R>` | 内側のコンテナ型。`random_access_range` かつ `Inner(int, T)` で構築できること |
| `T` | `ranges::range_value_t<Inner>` | 要素の型。デフォルト構築可能であること |

## 関数

### transpose

```cpp
R transpose(R const& v)
```

2次元 range を転置して返す（H×W → W×H）。`res[j][i] = v[i][j]`。

**制約**

- `v` のすべての行のサイズが等しいこと

**計算量**

- $O(H \times W)$

**使用例**

```cpp
vector<vector<int>> g = {{1,2,3},{4,5,6}};
auto t = transpose(g);
// t = {{1,4},{2,5},{3,6}}
```

---

### rotate90

```cpp
R rotate90(R const& v)
```

2次元 range を +90度（時計回り）回転して返す（H×W → W×H）。`res[j][h-1-i] = v[i][j]`。

**制約**

- `v` のすべての行のサイズが等しいこと

**計算量**

- $O(H \times W)$

**使用例**

```cpp
vector<vector<int>> g = {{1,2,3},{4,5,6}};
auto r = rotate90(g);
// r = {{4,1},{5,2},{6,3}}
```
