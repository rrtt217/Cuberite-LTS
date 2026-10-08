# 连接器的旋转不变量（生成器内部一致性）

> 这不是 vanilla 行为规格，而是 `cPiece` 连接器旋转的**内部一致性要求**：
> 只要违反它，piece 生成器的 BFS 就无法把两个 piece 接起来。
> 权威来源是代码里对方块面语义的定义（`cPiece::cConnector::eDirection` 的注释）与
> 位置旋转 `cPiece::RotatePos()`——方向只是位置所在面的外法线。

## 1. 问题

`cPiece::cConnector::RotateDirection()`（180° 旋转）把两个纯 Z 朝向返回了自身：

```cpp
case dirZM:       return dirZM;   // 应为 dirZP
case dirZP:       return dirZP;   // 应为 dirZM
```

X 轴（`dirXM ↔ dirXP`）与全部 corner 变体都正确翻转，90° 的
`RotateDirectionCCW()` / `RotateDirectionCW()` 也正确，只有这两个纯 Z 朝向被漏掉。

后果：piece 被以 180° 摆下时，连接器的**位置**旋转到盒子的另一个面，
**方向**却指着旧的面，于是 `AddDirection()` 算出的新连接器落回父件盒子**内部**；
BFS 用它当作邻居连接器的落点（`TryPlacePieceAtConnector()`），
候选 piece 于是全部与父件的 hitbox 相交而被否掉——结构长不出来。

发现过程见 [vanilla-1.12.2-stronghold.md](vanilla-1.12.2-stronghold.md) 第 5.7 节：
要塞「只生成传送门房」的概率依结构位置而变，正是因为起始件抽到的旋转角不同。

## 2. 规格（不变量）

对**任意** piece、**任意**连接器、**任意**旋转量 `n ∈ {0,1,2,3}`：

1. **外步出盒**：令 `P' = RotatePos(C.pos, n)`、`D' = RotateDirection(C.dir, n)`，
   则 `AddDirection(P', D')` 必须**不在**该 piece 旋转后的包围盒内。
   这是 BFS 能接上邻居的充要几何条件（邻居连接器就落在这一点）。
2. **轴向饱和**：`D'` 所指的每一条轴，都必须与其符号同向地到达或越过盒面：
   `z-` 的连接器必须 `z <= MinZ`（而不是落在盒内），`x+` 必须 `x >= MaxX`，依此类推；
   corner 朝向（`dirY?_X?_Z?`）对 Y、X、Z 三条轴同时成立。

不变量对**全部 14 个方向**成立，与数据里恰好用到哪些方向无关——旋转表是全域函数。

## 3. 修复

`cPiece::cConnector::RotateDirection()` 的两个纯 Z 分支改为互相交换（2 行）。
判据不是"照抄对称性"，而是以 `RotatePos()` 为基准逐项推导：
旋转后连接器所在的面决定它的方向，180° 必须同时翻转 X 与 Z（Y 不变）。

同轮核对结论（供后续审阅）：`RotateDirectionCCW()` / `RotateDirectionCW()` 的
6 个纯轴朝向与 8 个 corner 变体、以及 `RotateDirection()` 的 8 个 corner 变体，
按上述推导全部正确；这些现在也都被第 4 节的测试覆盖。

## 4. 验证

新增 [tests/Generating/ConnectorRotationTest.cpp](../tests/Generating/ConnectorRotationTest.cpp)（`ConnectorRotationTest`，工作目录 `Server/`）：

1. **合成件覆盖全方向**：内嵌一个 3×3×3 的 cubeset，14 个连接器覆盖全部 14 个方向，
   逐个、逐旋转量（0–3，不论该 piece 是否允许该旋转）检查第 2 节两条不变量。
   修复前该测试以如下信息失败：
   `connector (1, 1, 0) type 0 direction 2 rotated by 2: stepping along the rotated direction reaches (1, 1, 1), which is inside the box (0, 0, 0) .. (2, 2, 2)`。
2. **生产数据回归**：递归遍历 `Prefabs/**/*.cubeset`，对每个能加载的池、从其起始件沿连接器
   可达的每个 piece 做同样检查（不可达的 piece 不会参与生成，故不检查）。

**行为证据（BFS 级，临时探针，已回退、未提交）**：对 Stronghold 数据在 25 个网格单元上各驱动一次
`cPieceGeneratorBFSTree::PlacePieces()`，数放置的 piece 数：

| | 分布（25 个单元） | `<= 2` 个 piece 的单元 |
|---|---|---|
| 修复前 | `1,1,1,1,1,1,1,2,2,2,4,5,7,10,17,38,44,47,54,82,92,93,102,108,109` | 8 / 25 |
| 修复后 | `2,58,67,68,72,72,89,113,115,125,153,160,162,169,179,183,206,224,231,253,264,289,298,320,335` | 1 / 25 |

即：修复不仅让原本"卡在起始件"的结构长起来，也让原本健康的结构长得更大
（最大 109 → 335），因为 180° 的错误影响树中任意深度的任意一次旋转。

## 5. 残余限制

修复后仍有 1/25 的单元只长出 2 个 piece。这与 180° 缺陷无关，而是社区蓝图本身的几何限制
（40 个 piece 的 hitbox 全等于整块包围盒），见
[vanilla-1.12.2-stronghold.md](vanilla-1.12.2-stronghold.md) 第 5.6 节。
要塞布局若要 vanilla 化，需另立分支重建 piece 几何。

## 6. 影响面

任何「允许 180° 旋转且带 Z 朝向连接器」的 prefab，因此受影响的不止要塞：
下界要塞、村庄等使用 `AllowedRotations = 7` 的 cubeset 都在内。
`PieceRotation` 既有测试**不会**发现它——它只检查代数性质（CW/CCW 互逆、四次旋转恒等、
是置换），而这条错误恰好满足全部三条。

## 7. 来源

- 内部一致性要求，权威为仓库自身：
  [src/Generating/PiecePool.h](../src/Generating/PiecePool.h) 对 `cPiece::cConnector::eDirection`
  的注释（各方块面的语义）与 [src/Generating/PiecePool.cpp](../src/Generating/PiecePool.cpp) 的
  `cPiece::RotatePos()`（位置旋转的事实基准）。
- BFS 的连接语义：[src/Generating/PieceGeneratorBFSTree.cpp](../src/Generating/PieceGeneratorBFSTree.cpp)
  的 `TryPlacePieceAtConnector()` 与 `CheckConnection()`。
- 现象与定位过程见 [vanilla-1.12.2-stronghold.md](vanilla-1.12.2-stronghold.md) 第 5.7 节。
- 本项**不需要** vanilla 来源：它不是 vanilla 行为，而是本实现内部的自洽性。
