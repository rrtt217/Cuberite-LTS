# Vanilla 1.12.2 — 要塞（Stronghold）

分支：`feature/generating-stronghold`。上游缺口：[#741 Implement fortress generator](https://github.com/cuberite/cuberite/issues/741)
（该 issue 里的 "fortress" 指**主世界的要塞**，madmaxoft 在 2014-05-15 明确澄清 "This is about the overworld
fortresses, not the nether ones"；下界要塞是已关闭的 [#742](https://github.com/cuberite/cuberite/issues/742)）。
它是末地流程清单 [#4907 The End MEGA issue](https://github.com/cuberite/cuberite/issues/4907) 的第一项。

> **阶段决定（维护者，2026）**：**本阶段不追求与原版要塞严格对齐**。第 2 节的 vanilla 描述只作为参考基线，
> 第 4 节列出的偏差是**刻意保留**的近似，不是待办事项；不要为了对齐环状分布、构件拓扑比例、预填眼概率等
> 去改动本分支的实现。若日后需要，另行立项。

## 1. 来源（cleanroom 白名单）

- [Minecraft Wiki — Stronghold](https://minecraft.wiki/w/Stronghold)（生成规则、环状分布、构件、构成方块、战利品）
- [Minecraft Wiki — Eye of Ender](https://minecraft.wiki/w/Eye_of_Ender)（为什么传送门房必须可激活：眼睛只指向要塞，12 眼开门）
- [Minecraft Wiki — End Portal (block) / End Portal Frame](https://minecraft.wiki/w/End_portal_frame)（12 框环形、每框 10% 预填眼）
- 构件数据来源：[cuberite/cuberite#5605](https://github.com/cuberite/cuberite/pull/5605) 附带的 `Fortress.cubeset`
  （GalExport 导出，由 @NiLSPACE 提供，构件出自 Gallery 服务器的社区作品；各 piece 的 `OriginData.CreatorName` 可查）
- 本仓库既有实现锚点：`cGridStructGen`、`cPrefabPiecePool`、`cPieceGeneratorBFSTree`、`cPieceStructuresGen`、
  `cBlockEndPortalFrameHandler::FindAndSetPortal`（见第 3 节表格）

## 2. 行为规格（vanilla 1.12.2）

### 2.1 分布

- 要塞是**主世界**结构，只生成在**未生成的区块**里；几乎所有生物群系都可能（偏置到 `#stronghold_biased_to` 群系标签，
  做法是水平搜索该标签的群系并把起点移过去）。
- 1.12.2 共 **128 个**要塞，分布在 8 个同心环上，环内等角分布：

  | 环 | 数量 | 距原点距离（格） |
  |---|---|---|
  | 1 | 3 | 1280–2816 |
  | 2 | 6 | 4352–5888 |
  | 3 | 10 | 7424–8960 |
  | 4 | 15 | 10496–12032 |
  | 5 | 21 | 13568–15104 |
  | 6 | 28 | 16640–18176 |
  | 7 | 36 | 19712–21248 |
  | 8 | 9 | 22784–24320 |

  （第 8 环按 36° 而非 40° 排布，即按 10 个位置生成、其中 1 个不生成。）
- Y：wiki 只说「几乎都在地下、可在任意 Y，必要时会切掉基岩」；1.12.2 的具体 Y 算法 wiki 未给出 → **待确认**。
- 要塞不会被峡谷覆盖；洞穴、废弃矿井、化石等可以覆盖大部分构件，但传送门房几乎不会被覆盖。

### 2.2 结构

- 生成从**螺旋楼梯房（start piece）**开始，向下接五向交叉口，再随机接其他房间；最多约 50 个房间、水平半径约 112 格。
- **传送门房（portal room）是唯一保证生成的房间**，且从不在起始楼梯 5 个房间以内；若本次生成的布局里没有传送门房，
  生成会重新开始。
- 传送门房由 **12 个末地传送门框架**围成 5×5 的环（缺 4 个角），中央 3×3 为传送门方块；
  每框有 **10%** 概率自带末地之眼（平均 1.2 个，全填满概率约 10⁻¹²）。
- 石砖变体比例：石砖 45% / 苔石砖 30% / 裂石砖 20% / 虫蚀石砖 5%。
- 构成方块：石砖（含变体）、石砖台阶/楼梯、圆石楼梯、铁栏杆、铁门 + 石按钮、橡木门、橡木栅栏、橡木木板、火把、梯子、
  蜘蛛网（图书馆）、书架、箱子、水、岩浆、蠹虫刷怪箱、末地传送门框架。

### 2.3 与末影之眼的关系（跨分支依赖）

- 末影之眼飞向**起始楼梯所在区块的区块坐标 0,~,0**（1.19 之前为区块中心 8,~,8）；起始楼梯中心固定在区块内 4,~,4。
- 开启末地门需要 12 个框架全部有眼；**框架朝向必须朝向门内侧**，否则 `FindAndSetPortal` 的顺时针遍历会失败
  （见 5.2 的穷举结论）。

## 3. 本分支实现

数据驱动，不新增生成器类：沿用 `cPieceStructuresGen`（= `cGridStructGen` + `cPrefabPiecePool` + `cPieceGeneratorBFSTree`），
把要塞作为主世界 finisher 链的一项。

| 项 | 内容 |
|---|---|
| 数据文件 | `Server/Prefabs/PieceStructures/Stronghold.cubeset`（40 个 piece，113 KB） |
| 放置 | 由 cubeset 元数据决定：`GridSizeX/Z = 512`、`MaxOffsetX/Z = 256`、`MaxStructureSizeX/Z = 384`、`MaxDepth = 10` |
| 起始件 | 只有 2 个传送门房 piece 是 `IsStarting = 1`，且它们的 `VerticalStrategy = Range｜20｜40` → 整个要塞在地下 Y 20–40 |
| 注册 | `cComposableGenerator::InitializeGeneratorDefaults()` 的 `dimOverworld` finishers 末尾加 `PieceStructures: Stronghold` |
| 顺序 | 放在**最后**：地形塑造（峡谷/洞穴/湖/矿/矿井）先跑，要塞后画，避免矿井等挖穿传送门房 |

### 3.1 对上游数据的修正（本分支相对 #5605 的差异）

1. **框架朝向**：原始导出的两个传送门房（`Fortress_253` 全部 meta 0、`Fortress_27` 为 0/1/3）**无法**被
   `cBlockEndPortalFrameHandler::FindAndSetPortal` 接受——眼睛能放进框架，但门永远不会出现。
   本分支把 12 个框架一律改为**朝向门内侧**（北 0 / 南 2 / 西 3 / 东 1）。
2. **改名**：文件与 group 由 `Fortress` 改为 `Stronghold`，`OriginData.ExportName` 由 `Fortress_*` 改为
   `Stronghold_*`（本仓库同时存在下界要塞 `NetherFort.cubeset`，旧名会持续误导）。
   `Name` / `GalleryName` 保持导出原样。

### 3.2 对已存在世界的影响（实测踩到，务必注意）

`InitializeGeneratorDefaults()` 用的是 `GetValueSet()` —— **只在 `world.ini` 缺少该键时才写入**；而世界首次
加载时就把 `[Generator] Finishers` 落盘了。因此**已存在的世界不会自动获得新 finisher**：本分支的代码改动对
这些世界等于没做，表现为"跑过去一片要塞都没有"。

本检出实测：`build/Server/world/world.ini` 的 `Finishers` 停在改动前的列表（末尾是 `ForestRocks`），
没有 `PieceStructures: Stronghold`，所以那些坐标处什么都不会生成。修复 = 手动把该项加到现有
`world.ini` 的 `[Generator] Finishers` 末尾（或删掉整行让它重新取默认值）：

```
Finishers=..., OverworldClumpFlowers, ForestRocks, PieceStructures: Stronghold
```

改完必须**重启服务器**（生成器只在启动时构建）。`world_nether` / `world_the_end` 的 ini 与当前代码默认值一致，
无需改动。另外：要塞只出现在**新生成**的区块里，改动之前已生成的 region 不会补上要塞。

这是 Cuberite 的既有语义（默认值只写一次），本分支不改动它。**"缺失 finisher"的启动警告或迁移机制已决定不做**：
维护者要求保留"用户应当可以关闭部分结构生成"的能力。

## 4. 已知偏差（相对上述 vanilla 规格，**刻意保留、本阶段不对齐**）

> 见开头的阶段决定：下列偏差是当前实现有意接受的近似，**不作为待办**；只为"要塞能生成 + 传送门可激活"负责。

1. **分布规则**：本分支是「每 512×512 格一个、偏移 ±256」的无限网格，**不是** 8 环 128 个等角分布。
   vanilla 环状分布需要新的放置策略（非 `cGridStructGen`）+ 群系偏置；#741 本身把"3 个或无限"列为可选。
   **本阶段不做。**
2. **构件不是 vanilla 布局**：cubeset 是 Gallery 社区作品导出（piece 名如 `Under 0..38`、`DiagonalCorridor`、
   `XCrossing`），不含 vanilla 的「螺旋楼梯 → 五向交叉 → 最多 50 房」拓扑；每个 piece 只有 1 个连接器，
   因此 BFS 只能连成链而非网状。传送门房本身是标准的 5×5 缺角环（12 框），与 vanilla 一致。
3. **框架不带预填眼**：vanilla 每框 10% 自带眼；本数据全部为空框，需要玩家放满 12 个眼。**本阶段不做。**
4. **石砖变体比例未按 vanilla 校验**（45/30/20/5）：数据里有石砖/苔石砖/裂石砖/虫蚀石砖。**本阶段不做。**
5. **图书馆/五向交叉等房间没有配比保证**：数据里存在含书架/箱子的房间，但随机链式生成不保证出现，也不保证数量上限。
6. **Y 范围 20–40 是数据自带**，不是 vanilla 算法；vanilla 的 Y 放置规则 wiki 未给出（见 2.1）。
7. **不含战利品表**：要塞箱子（数据里有 4 个）生成后为空；末地城那套战利品机制（`EndCityLoot`）未接入要塞。

## 5. 验证

### 5.1 验收门

- `cd src && lua CheckBasicStyle.lua`：0 违规。
- `cmake --build build`：exit 0。
- `cd build && ctest --output-on-failure -E "UrlClient-test|Google-test"`：全绿（新增 `StrongholdTest`）。
- 不涉及 `src/Bindings/` 与导出 API → 无需绑定/APIDump 检查。

### 5.2 数据不变量（自动化）

新增 `tests/Generating/StrongholdTest.cpp`（工作目录 = 服务器目录，用生产路径 `Prefabs/PieceStructures/Stronghold.cubeset`），
三条断言：

1. **数据**：cubeset 能被 `cPrefabPiecePool` 加载；起始件恰好 2 个，且 `GetStartingPieceHeight()` 落在 20–40
   （起始件缺 `VerticalStrategy` 时 `cPrefabPiecePool` 会回退到 `Fixed|150`，要塞就会浮在空中）。
2. **注册**：`cComposableGenerator::InitializeGeneratorDefaults(Ini, dimOverworld)` 写出的 `Generator.Finishers`
   含 `PieceStructures: Stronghold`。
3. **真实生成**：用 `cPieceStructuresGen` 加载 `Stronghold`，按 `cGridStructGen` 的同一表达式算出格 (0,0) 的
   结构原点，只生成覆盖该原点的少数区块，要求出现**完整 12 框环**（5×5 去 4 角），且框架 Y 落在 23–43。

框架朝向的等价性来自本轮复核的**穷举验证**：对「每边统一朝向」的 4⁴ = 256 种组合逐一跑
`FindAndSetPortal` 的镜像实现（12 框全部填眼、逐一尝试"最后填的那一帧"作起点），**只有全部朝向内侧那一种通过**
（12/12 起点可成功开门并生成 3×3 传送门）；原始导出两种朝向均为 0/12。因此"朝向内侧"等价于"门可激活"，
且不需要构造 `cChunkInterface`/`cWorldInterface`。

### 5.3 测试桩带来的限制（重要）

`tests/Generating` 目标用 `Stubs.cpp` 打桩了 `cBlockHandler::For()`，所以**在测试里**预制品旋转只转位置、
不转 meta（实测：同一门房 4 种旋转的 meta 保持不变，世界朝向变成切向）。
真实引擎不受影响：`cBlockEndPortalFrameHandler` 的 `cMetaRotator<..., ZP, XM, ZM, XP>` 把四个状态按
(N, E, S, W) = (0, 1, 2, 3) 交给 mixin，而 `MetaRotateCCW` 的循环是 `South→East→North→West→South`，
即 meta `0→3→2→1→0`——正是几何上"整体逆时针旋转 90° 后朝向仍朝内"所需的那个循环（推导见下）。

因此测试对**未旋转**的门房做严格朝向断言（`a_AllowMetaShift = false`），对生成路径只要求
"完整环 + 位于地下"，并允许方向值整体旋转一个常量（`a_AllowMetaShift = true`）——这一常量恰好就是
打桩缺失的 meta 旋转所产生的差异。要在测试中严格覆盖旋转后的朝向，需要把真实方块处理器链进测试目标，
不在本分支范围内（记为后续工作）。

朝向循环推导（用于核对引擎）：设世界坐标 +Z 为南、+X 为东。`cBlockArea::RotateCCW` 的位置映射为
`(x, z) → (z, SizeX-1-x)`，故局部 +X → 世界 −Z、局部 +Z → 世界 +X。未旋转环的"朝内"配置是
北边(z 最小)=0、南边(z 最大)=2、西边(x 最小)=3、东边(x 最大)=1。逆时针旋转后，原北边的框架落到西边，
必须取值 3，故 `rotate(0) = 3`；同理 `rotate(3) = 2`、`rotate(2) = 1`、`rotate(1) = 0`，与引擎一致。

### 5.4 实测坐标（本检出 build/Server 的 world，便于实机验证）

本检出 `build/Server/world/world.ini` 里 `[Seed] Seed = 234096503`（下界 1237527178、末地 1033961529）。
用该种子按 5.2 第 3 条的路径枚举格 (x,z) ∈ [−1,1]²，得到 9 个要塞，**每个都带传送门房**
（因为数据里只有 2 个 portal room 是 `IsStarting = 1`），按距离排序：

| 传送门框架范围（x / y / z） | 环心（建议 tp 目标） | 距原点 | region |
|---|---|---|---|
| x −194..−190, y 34, z −14..−10 | **(−192, 34, −12)** | **192（最近）** | `r.-1.-1` |
| x −46..−42, y 37, z 281..285 | (−44, 37, 283) | 286 | `r.-1.0` |
| x 466..470, y 36, z −14..−10 | (468, 36, −12) | 468 | `r.0.-1` |
| x 466..470, y 29, z −307..−303 | (468, 29, −305) | 558 | `r.0.-1` |
| x −631..−627, y 41, z −16..−12 | (−629, 41, −14) | 629 | `r.-2.-1` |
| x −487..−483, y 28, z −453..−449 | (−485, 28, −451) | 662 | `r.-1.-1` |
| x −555..−551, y 30, z 425..429 | (−553, 30, 427) | 698 | `r.-2.0` |
| x −46..−42, y 30, z −747..−743 | (−44, 30, −745) | 746 | `r.-1.-2` |
| x 539..543, y 43, z 642..646 | (541, 43, 644) | 841 | `r.1.1` |

截至本文更新时，`build/Server/world/region/` **只剩 `r.0.0`**（x,z ∈ 0..511 那一格）。上表 9 个传送门房没有
一个落在 `r.0.0` 里，因此**全部会在到达时重新生成**，最近的那个（环心 (−192, −12)，框架 y 34）可以直接用。

步骤：

1. 确认 `world.ini` 的 `Finishers` 已含 `PieceStructures: Stronghold`（见 3.2），并用**新构建**的二进制重启服务器。
2. `/tp <玩家> -192 34 -12`——落点即传送门房内部（5×5 环内的 3×3 为空）；也可换成表里任意一行。
3. 用 12 个末影之眼填满框架环（缺 4 角）；放入最后一枚时 `cBlockEndPortalFrameHandler::FindAndSetPortal`
   应生成 3×3 末地传送门。

注意：

- 要塞只生成在**新生成**的区块；改动前就已存在的 region 不会补上要塞。
- 结构可能在 region 边界被截断：例如环心 (−192, −12) 的要塞有部分构件会伸进已存在的 `r.0.0`，
  那部分（改动前生成的区块）不会出现；但**传送门房本身完整落在 `r.-1.-1`**，不影响开门。
- 世界种子由 `<world>/world.ini` 的 `[Seed] Seed` 决定；缺省时服务器会随机生成并写回，因此换世界需重新设定。

### 5.5 实机验证（维护者已做）

- **实测通过**：在 `build/Server` 的本检出世界里（种子 234096503，`world.ini` 按 3.2 加上
  `PieceStructures: Stronghold` 并重启后），**要塞可生成、末地传送门可被 12 枚末影之眼点亮**。
  即 5.2 的三条不变量与 3.2 的修复在实机成立。
- 仍未做：与洞穴/废弃矿井的覆盖顺序长距离观察；旋转后的门房朝向未逐例实机核对（引擎侧循环推导见 5.3）。

## 6. 变更清单

| 文件 | 说明 |
|---|---|
| `Server/Prefabs/PieceStructures/Stronghold.cubeset` | 新增（数据，含 3.1 的两处修正与来源头注释） |
| `src/Generating/ComposableGenerator.cpp` | 主世界默认 finishers 末尾加 `PieceStructures: Stronghold` |
| `tests/Generating/StrongholdTest.cpp` | 新增数据不变量测试 |
| `tests/Generating/CMakeLists.txt` | 注册 `StrongholdTest` |
| `specs/vanilla-1.12.2-stronghold.md` | 本文件 |

## 7. 未决 / 已决定

1. **数据许可**：cubeset 是 Gallery 社区作品导出（@NiLSPACE 提供，#5605 附带）。在本 fork 内分发需确认许可与署名方式。
2. **分布规则**（环状 128 个 + `#stronghold_biased_to` 群系偏置）：**本阶段不做**（见开头阶段决定与第 4 节）。
   现有网格近似即为最终形态，除非日后另行立项。
3. **框架预填眼 10%**（需要生成器支持按块随机替换，`cPrefab` 目前没有该机制）：**本阶段不做**。
4. **测试桩**：`tests/Generating` 打桩了 `cBlockHandler::For()`，无法在测试中覆盖"旋转后 meta 仍朝内"。
   若要补，需要把真实方块处理器链接进测试目标（或在测试中直接实例化 `cBlockHandler` 的真实实现）。
5. **已存在世界的 finisher 迁移**（见 3.2）：**决定不做**。维护者明确要求保留"用户应当可以关闭部分结构生成"的
   能力，因此不自动合并/迁移 `world.ini` 的 finisher，也不加"缺少默认结构 finisher"的启动警告。
   老世界需要手工把 `PieceStructures: Stronghold` 加进 `world.ini`。
