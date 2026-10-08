# Vanilla Minecraft Java 1.12.2 - Cubeset 方块实体内容

> 范围：让 `.cubeset` 能携带**方块实体内容**，并把现有结构按「能确定来源 ⇄ 不能确定」分别迁移或标注。
> 本规格只覆盖**阶段 0**（非战利品内容）。箱子/发射器的**战利品**属于阶段 1，见第 2 节。

## 1. 现状：缺的是表示层，不是运行时

运行时管线**已经就绪**：

- `cBlockArea` 有 `baBlockEntities`（[src/BlockArea.h:53](../src/BlockArea.h#L53)）；
- char-map 写块时 `SetRelBlockTypeMeta` → `RescanBlockEntities` 会造出**默认内容**的活对象（[src/BlockArea.cpp:814-822](../src/BlockArea.cpp#L814-L822)）；
- `CopyTo`（`CopyFrom` 转发到它）/ `RotateCCW` / `MirrorXY|YZ` 都会搬运并重定位 BE（[src/BlockArea.cpp:572-579](../src/BlockArea.cpp#L572-L579)、[:1077-1091](../src/BlockArea.cpp#L1077-L1091)、[:1205-1218](../src/BlockArea.cpp#L1205-L1218)）；
- `cPrefab::Draw` → `cChunkDesc::WriteBlockArea` → `cBlockArea::Merge` → `MergeBlockEntities` 会连内容一起搬（[src/BlockArea.cpp:2561](../src/BlockArea.cpp#L2561)、[:2574-2622](../src/BlockArea.cpp#L2574-L2622)）；
- 生成结束后 BE 被 move 进 chunk（[src/World.cpp:4193](../src/World.cpp#L4193)、[src/Chunk.cpp:370](../src/Chunk.cpp#L370)）。

断掉的是**文件的表达能力**：piece 只有 `BlockDefinitions`（字符 → type+meta）、`BlockData`、`Size`，没有任何承载 per-position 结构化数据的字段（[dev-docs/Cubeset file format.html](../dev-docs/Cubeset%20file%20format.html)）。而唯一的外部数据出口 `.schematic` 在 Cuberite 里也不读写 `TileEntities`（[src/WorldStorage/SchematicFileSerializer.cpp:118](../src/WorldStorage/SchematicFileSerializer.cpp#L118)、[:198-203](../src/WorldStorage/SchematicFileSerializer.cpp#L198-L203)）。

后果：prefab 里的刷怪笼一律是构造函数默认的**猪**（[src/BlockEntities/MobSpawnerEntity.cpp:17](../src/BlockEntities/MobSpawnerEntity.cpp#L17)），花盆一律为空，床一律为默认红色。这就是上游 [#2455](https://github.com/cuberite/cuberite/issues/2455)（下界要塞刷出猪）的根因。

## 2. 阶段划分

| 阶段 | 内容 | 依赖 |
|---|---|---|
| **0（本规格）** | 格式能携带**非战利品** BE 内容：刷怪笼生物类型、花盆内容 | 无 |
| 1 | 容器携带**战利品表名** + 「名字 → 规格」注册表（复用 `cLootProbab` / 泛化后的 `EndCityLootTable`） | 阶段 0（表名要有地方存） |
| 2 | conditions / functions / tags / 每世界覆盖 | 阶段 1 |

阶段 0 **不依赖**任何战利品引擎：刷怪笼/花盆与战利品无关（见第 4 节的清单）。

## 3. 阶段 0 的格式扩展

piece 表新增**可选**字段 `BlockEntities`，稀疏列表，坐标为 prefab 内相对坐标（与 `BlockData` 的 X/Y/Z 同一坐标系，原点在 piece 最小角）：

```lua
BlockEntities =
{
    -- 刷怪笼：Entity 是 cMonster::StringToMobType() 认的小写名
    { X = 10, Y = 7, Z = 3, Entity = "blaze" },

    -- 花盆：Item 是方块/物品类型 id，Meta 默认 0
    { X = 3, Y = 3, Z = 5, Item = 40, Meta = 0 },
},
```

设计取舍：

- **按块类型分派，而不是另设 `Type` 字段**：读取时看 `(X,Y,Z)` 上是什么方块，再要求对应的字段。这样数据不可能与图像不一致，也不需要维护一份类型名表。
- **稀疏坐标而非与图像平行的字符层**：阶段 0 每个 piece 只有 0–4 个 BE，稀疏表更省、更好手写、diff 友好；字符层要复刻整幅图，手工维护易错。
- **可选字段**：旧 cubeset（`CubesetFormatVersion = 1`）完全不受影响；不设该字段即维持今天的行为。
- **应用时机**：在 `LoadPrefabFromCubesetVer1()` 里、`SetAllowedRotations()` **之前**注入，因此已有的旋转/镜像代码会自动把 BE 一起变换（[src/Generating/PrefabPiecePool.cpp:312-328](../src/Generating/PrefabPiecePool.cpp#L312-L328)：312–318 是注入点，328 是 `SetAllowedRotations()`）。
- 阶段 0 支持的字段：`Entity`（mob spawner）、`Item`/`Meta`（flower pot）。其余字段（床上色、告示牌文字、旗帜图案…）**不做**，遇到即警告并跳过。
- **失败语义分两级**：
  - 条目本身不成立（没有 `X`/`Y`/`Z`、生物名未知、花盆内容非法、块类型尚不能携带内容）→ **加载失败**（与畸形的 Connectors 同级）；
  - 条目坐标上没有方块实体 → **只报警告并跳过**。生产构建里能携带内容的块类型在建模时必然已被赋予方块实体（[src/BlockArea.cpp:814-822](../src/BlockArea.cpp#L814-L822)），所以这只会是坐标写错；
    一条标注写错不该让整个结构作废。

## 4. 各结构的 BE 清单与处置

坐标取自各 cubeset 的 `BlockData` 反解（`x`/`y`/`z` 为 piece 内相对坐标）。

### 4.1 迁移（有可确定来源）

| 结构 | piece | 坐标 | 方块 | 迁移为 | 来源 |
|---|---|---|---|---|---|
| `NetherFort.cubeset` | `BlazePlatform` | (6,3,3) | mobspawner | **blaze** | Minecraft Wiki, Nether Fortress：*"Up to 2 blaze monster spawner platforms … with a blaze monster spawner in the center"*；仓库内证据：这两个 piece 就叫 `BlazePlatform` / `BlazePlatformOverhang` |
| `NetherFort.cubeset` | `BlazePlatformOverhang` | (10,7,3) | mobspawner | **blaze** | 同上 |
| `Stronghold.cubeset` | `Stronghold_253` | (6,3,5) | mobspawner | **silverfish** | Minecraft Wiki, Silverfish - Monster spawners：*"Silverfish monster spawners naturally generate in End portal rooms in strongholds"*（这两个 piece 正是末地传送门房） |
| `Stronghold.cubeset` | `Stronghold_27` | (6,3,7) | mobspawner | **silverfish** | 同上 |

### 4.2 标注（无来源或本来为空）

| 结构 | 方块 | 处置 |
|---|---|---|
| `DesertPyramid`、`JungleTemple`、`NetherFort`、`Stronghold`、`AlchemistVillage`、`JapaneseVillage`、`PlainsVillage`、`SandFlatRoofVillage` 的所有箱子 | chest | **阶段 1**（战利品）；本分支只在文件头标注 |
| `JungleTemple` 的两个发射器 (2,2,11)、(8,2,13) | dispenser | **阶段 1**（战利品/箭矢）；标注 |
| `WitchHut` (3,3,5)、`AlchemistVillage` (11,3,1)/(5,3,7)/(7,3,7)、`JapaneseVillage` 的 4 个 | flower pot | **来源不明**：Magma 社区作品，无来源可定内容；WitchHut 的 wiki 叙述与上游 bug MC-66154（"Flower pots generate empty in swamp huts"）相互矛盾。保持为空并标注 |
| `AlchemistVillage`/`JapaneseVillage`/`PlainsVillage`/`SandFlatRoofVillage` 的熔炉 | furnace | 1.12.2 村庄熔炉内容无来源可证；保持空并标注 |
| `PlainsVillage` `WoodenMill5x5` (3,2,6) | hopper | 该 piece 为社区作品，无对应 vanilla 结构；标注 |
| `SandVillage` `SmallHut` 的两张床 | bed | 1.12.2 床色存在 BE 里（[src/BlockEntities/BedEntity.h](../src/BlockEntities/BedEntity.h)），cubeset 暂不能携带 → 生成出来一律默认红；标注为阶段 0 未覆盖 |

## 5. 验证方式

### 5.1 自动化覆盖的边界（重要）

本功能**无法**在现有测试体系下被单元测试覆盖到"内容是否真的落到方块实体上"：生成器测试构建刻意不链接方块实体与物品系统——
`tests/Generating/Stubs.cpp` 让 `cBlockEntity::CreateByBlockType()` 恒返回 `nullptr`、`IsBlockEntityBlockType()` 恒返回 `false`，
因此 prefab 的图里**永远不存在**方块实体，`Entity` / `Item` 这些字段在测试构建里根本不会被读到。

`tests/Generating/CubesetBlockEntitiesTest.cpp` 因此只覆盖**可观测的解析契约**（其文件头逐条写明）：

1. 没有 `BlockEntities` 表的 piece 照常加载（既有 cubeset 不受影响）；
2. 格式正确的条目被接受，即便本构建里没有方块实体可填（条目被报告并跳过，piece 其余部分仍有效）；
3. 连坐标都没有的条目让加载失败；
4. 迁移过的生产 cubeset（`NetherFort.cubeset`、`Stronghold.cubeset`）仍能加载。

**它明确不覆盖**：内容是否落到方块实体上、数值语义是否正确（烈焰人 / 蠹虫）。这两条只能由真实服务器行为验证。
不要把"测试没红"当作本功能已被验证。

### 5.2 行为验证（2026-10-08 由维护者手动进行，结果：本功能当时完全无效）

维护者在真实 binary 上生成结构后，**所有刷怪笼都是默认的猪**。存档 NBT 佐证：

```
world:        1407 区块，含 MobSpawner 1 个  → EntityId: pig
world_nether:  759 区块，含 MobSpawner 30 个 → EntityId: pig ×33
```

而加载期**没有任何告警**——说明内容确实写进了 prefab，丢失发生在 **prefab → chunkdesc**。

**根因是共享代码里的一个既有坐标符号错误**：[src/BlockArea.cpp](../src/BlockArea.cpp) 的
`cBlockArea::MergeBlockEntities()` 用 `x + a_RelX` 去源图取方块实体，而块合并的约定是
`dest = src + a_RelX`（见同文件 `MergeByStrategy()` 的 `SrcOffX = max(0, -a_RelX)` / `DstOffX = max(0, a_RelX)`），
源坐标应为 `x - a_RelX`（y / z 同）。只要 prefab 原点不在区块原点，源坐标就是错的，取不到就地补一个**空**的方块实体
→ 默认猪。该 bug 一直潜伏，是因为方块实体的合并此前**没有消费者**——`EndCityLoot` 之所以要在绘制后自己回填，
正是绕开了它。本分支已修（独立提交）。

修正后**仍需一次真机复核**：确认存档里的 `EntityId` 变成 `blaze` / `silverfish`。
在复核通过前，本功能的状态是「根因已修，端到端未验证」。

### 5.3 数据来源核对

烈焰人刷怪笼与蠹虫刷怪笼均已取 Minecraft Wiki 原文（第 6 节），不再是推测。

### 5.4 前置依赖：蠹虫的刷怪规则

把 Stronghold 的刷怪笼标成 `silverfish`，会暴露 `cMobSpawner::CanSpawnHere()` 缺蠹虫分支的问题：
它落进 `default:` 分支打 `MG TODO: Write spawning rule for mob type 44` 并 `return false`，
即**刷怪笼永远不会生成生物**（维护者手动放置蠹虫刷怪笼时已观察到刷屏）。
本分支一并补上该规则（光强 ≤ 11，来源见第 6 节），因为它不是「可绕过的相关缺口」而是本功能的**前置依赖**：
不补上就无法验证「silverfish 刷怪笼生效」。烈焰人本来就有规则（`case mtBlaze`），不受影响。

## 6. 来源

- Minecraft Wiki, [Nether Fortress](https://minecraft.wiki/w/Nether_Fortress) - Structure / blaze monster spawner platforms（本轮已取原文）。
- Minecraft Wiki, [Silverfish](https://minecraft.wiki/w/Silverfish) - Monster spawners 小节（本轮已取原文）：
  *"Silverfish monster spawners naturally generate in End portal rooms in strongholds. Silverfish can spawn
  from spawners at light level 11 or lower."* —— 同时确认了刷怪笼的生物类型与光强规则。
- Minecraft Wiki, [Monster Spawner](https://minecraft.wiki/w/Monster_Spawner)、[Stronghold](https://minecraft.wiki/w/Stronghold)、[Witch Hut](https://minecraft.wiki/w/Witch_Hut)、[Flower Pot](https://minecraft.wiki/w/Flower_Pot)。
- 上游 issue [cuberite/cuberite#2455](https://github.com/cuberite/cuberite/issues/2455) - 现象与「需要新格式版本 + GalExport 支持」的结论。
- 上游 bug [MC-66154](https://bugs-legacy.mojang.com/browse/MC-66154) - "Flower pots generate empty in swamp huts"。
- 仓库内既有事实：第 1 节列出的代码位置；[tests/Generating/Test.cubeset](../tests/Generating/Test.cubeset)（无 BlockEntities 的回归样本）。

## 7. 不确定项

1. **WitchHut 花盆**：wiki 叙述（红蘑菇）与 MC-66154（生成为空）冲突，且未确认该 bug 在 1.12.2 的状态；保持为空。
2. **社区作品（Alchemist/Japanese 村庄、PlainsVillage 的漏斗与熔炉、SandVillage 的床）**：内容不可考，不做推测。
3. **GalExport**：新字段的写出需要 GalExport 侧配合；本仓库只能定义格式并回填现有文件。
4. **蠹虫刷怪笼的可观察性受要塞蓝图的限制**：现有 `Stronghold.cubeset` 是 Gallery 社区作品导出，
   **40 个 piece 的 hitbox 全部等于整块包围盒**，实测世界里要塞只长出了传送门房所在区块及紧邻的一个区块
   （见 [vanilla-1.12.2-stronghold.md](vanilla-1.12.2-stronghold.md) 的 5.6 节）。
   这不影响本功能的正确性，但会让「蠹虫刷怪笼」在实际游戏里难得一见。
