# Vanilla 1.12.2 — 紫颂植物（Chorus Plant / Chorus Flower）

来源（cleanroom 白名单）：
- [Minecraft Wiki — Chorus Plant](https://minecraft.wiki/w/Chorus_Plant)（含 2017-10-29 旧版本，1.12 时代）
- [Minecraft Wiki — Chorus Flower](https://minecraft.wiki/w/Chorus_Flower)（含 2017-12-29 旧版本）
- [Minecraft Wiki — Chorus Plant (feature)](https://minecraft.wiki/w/Chorus_Plant_(feature))
- [中文 Wiki — 作物机制 § 紫颂花](https://zh.minecraft.wiki/w/%E4%BD%9C%E7%89%A9%E6%9C%BA%E5%88%B6)（生长算法与概率表的权威描述）
- [Minecraft Wiki — Chorus Fruit](https://minecraft.wiki/w/Chorus_Fruit)
- [Minecraft Wiki — Popped Chorus Fruit](https://minecraft.wiki/w/Popped_Chorus_Fruit)
- [Minecraft Wiki — End Highlands](https://minecraft.wiki/w/End_Highlands)
- 官方数据包 JSON（公开数据报告，1.13+）：`chorus_plant` 的 placed_feature（见 §4，仅作行为规格）。
- 1.12.2 客户端存档实测（Cleanroom 0.6.13-alpha，`DataVersion 1343`）：主世界手工放置的紫颂植株元数据为 0（见 §2.1）。

本文只写规格，不含实现。1.12.2 为硬编码世界生成，1.13 起改为数据驱动；数据包数值与新版本可能不同，凡引用数据包处均标注。

---

## 0. 当前 Cuberite 实现状态（基线，`master` @ `f94517b`）

已有（**不要重复实现**）：

| 项 | 位置 |
|---|---|
| 方块 ID `E_BLOCK_CHORUS_PLANT = 199`、`E_BLOCK_CHORUS_FLOWER = 200` | `src/BlockType.h` |
| 物品 ID `E_ITEM_CHORUS_FRUIT = 432`、`E_ITEM_POPPED_CHORUS_FRUIT = 433` | `src/BlockType.h` |
| 硬度 0.4、透明/固体等分类 | `src/BlockInfo.cpp` |
| 音效枚举 `SFX_RANDOM_CHORUS_FLOWER_GROW / DEATH` | `src/EffectID.h` |
| 吃紫颂果（4 饱食 / 2.4 饱和 + 传送 + `item.chorus_fruit.teleport`） | `src/Items/ItemChorusFruit.h` |
| 饱食/创造下仍可食用紫颂果 | `src/ClientHandle.cpp` |
| 熔炉：紫颂果 → 烤紫颂果（0.1） | `Server/furnace.txt` |
| 沙砾模拟排除紫颂 | `src/Simulator/SandSimulator.cpp` |
| 方块处理：`cDefaultBlockHandler`（空行为） | `src/Blocks/BlockHandler.cpp` |

缺失（本规格要覆盖）：

1. **自然生成**：`src/Generating/` 无任何紫颂引用；末地外岛只有末地石噪声。
2. **生长行为**：`cDefaultBlockHandler::OnUpdate` 为空 → 紫颂花不生长、不分枝、不死亡。
3. **支撑/自动破坏**：`CanBeAt` 默认恒 `true`，`OnNeighborChanged` 默认不会因失去支撑而破坏。
4. **掉落**：走默认「掉落自身」，与原版不一致（§2.3、§3.4）。
5. **紫颂花被弹射物/爆炸破坏**：未实现（默认行为不符）。

---

## 1. 方块与物品基础数据

| | 紫颂植株 Chorus Plant | 紫颂花 Chorus Flower |
|---|---|---|
| 数字 ID（1.12.2） | 199 | 200 |
| 透明 | 是 | 是 |
| 发光 | 否 | 否 |
| 最佳工具 | 斧 | 斧（剑也快） |
| 可燃/岩浆点燃 | 否 | 否 |
| 音效组 | 木质（Wood） | 木质（Wood） |
| 堆叠 | 64 | 64 |
| 可再生 | 是 | 是 |
| 生存可否获得 | **否**，即使精准采集也掉落不了本体（仅创造） | 是（直接破坏掉落自身） |

其他：紫颂植株碰撞箱不是完整方块（1.9 起）——Cuberite 需确认是否已用正确的 bounding box（`GetPlacementCollisionBox` 等）。紫颂花是「花」类方块，蜜蜂可授粉（1.20.3+，1.12.2 无蜜蜂，忽略）。

---

## 2. 紫颂植株（Chorus Plant）方块

### 2.1 方块状态：六个方向连接

- `down / east / north / south / up / west` 各为布尔，表示该方向是否延伸出枝干。
- **1.9–1.12 的关键限制**：该版本区块方块数据只有 **4 bit**，而六个布尔需要 6 bit，无法完整传输。证据：
  - 官方的旧→新升级数据报告（本仓库 `Server/Protocol/UpgradeBlockTypePalette.txt`）只列出一条 `199:0` → `chorus_plant(全部 false)`；反向映射也只有全 false 一条。
  - 4-bit 上限的物理约束。
  - **存档实测**：1.12.2 存档（Cleanroom 0.6.13-alpha，`DataVersion 1343`）中，手工放置并加速随机刻长成的紫颂树共 134 个方块（118 个 `199` + 16 个 `200`），**全部 118 个 `199` 的 `Data` 元数据 = 0**（含上下、东西、南北相邻的各种情形）。
  - **客户端实测**：同一客户端 F3 调试屏指着 (0,72,-1) 时，区块数据里该方块为 `199 + meta 0`，而 F3 显示的方块状态是 `minecraft:chorus_plant  down=false east=true north=false south=false up=true west=false` —— 与该方块邻居（上方、东侧是紫颂植株，其余空气）完全一致。这**直接证明连接状态由客户端从邻居推导**。
  ⇒ **服务端不存储连接信息（`meta` 恒 0）**；六个方向的连接只能由**客户端**在渲染时根据相邻方块推导（实际状态）。服务端无需计算/更新连接位。
- 1.13 起连接成为显式方块状态，服务端需要维护；但本仓库目标协议为 1.8–1.12.2，**本特性不要求服务端计算连接位**（若同时要考虑 1.13+ 客户端，另见 §7）。

> 备注：「客户端推导」是唯一可能（服务端未提供任何连接数据），但未直接观察客户端渲染；这不影响服务端实现——服务端照常发送 `id=199, meta=0` 即可。

### 2.2 支撑与自动破坏（核心行为）

紫颂植株在邻居变化后检查六个相邻方块；不满足则**自动破坏**（有机会掉紫颂果，见 §2.3）：

1. **横向过厚规则**：若至少一个水平相邻方块是紫颂植株，则**除非至少一个垂直相邻方块（上或下）是空气**，否则破坏。
   （即相邻两个植株不能在垂直方向都被堵住。）
2. **支撑规则**：**除非**满足以下任一条件，否则破坏：
   - 下方是紫颂植株或末地石；或
   - 存在一个水平相邻的紫颂植株，且**那个植株自身**的下方是紫颂植株或末地石。

解读：植株可以「侧向」接在另一株已受支撑的植株上，但不能悬空。

**实测确定的规则**（用户在同一 1.12.2 客户端手工搭建的树上做的受控实验）：

- (1,72,-1)：下方=空气、上方=空气，水平南侧 (1,72,0) 是植株且其下方 (1,71,0) 也是植株 → 存活。
- (0,72,-1)：下方=空气、上方 (0,73,-1) 是植株、水平仅东侧 (1,72,-1)（其下方是空气）→ 存活；**打断上方的 (0,73,-1) 后，(0,72,-1) 随之掉落**。

⇒ 满足以下**任一条**即可存活（另加横向过厚规则）：

1. 下方是紫颂植株或末地石；或
2. **上方是紫颂植株或末地石**（悬垂支撑；实测：去掉上方植株即掉落）；或
3. 存在一个水平相邻的紫颂植株，且**该相邻植株的下方**是紫颂植株或末地石。

第 3 条要求邻居自身有下方支撑——实测中 (0,72,-1) 的东邻 (1,72,-1) 下方是空气，因此**不能**靠它支撑，去掉上方植株后即掉落。

Cuberite 落点：
- `cBlockHandler::OnNeighborChanged` 默认实现已经会在 `CanBeAt` 返回 false 时 `DropBlockAsPickups`，所以只需为紫颂植株实现正确的 `CanBeAt`。
- `CanBeAt(const cChunk &, Vector3i, NIBBLETYPE)` 里用 `cChunk::UnboundedRelGetBlock` 读跨区块邻居。

### 2.3 掉落

- 破坏紫颂植株掉落 **0–1 个紫颂果**，每个方块独立 50% 概率；**不受时运影响**。
- 原版即使创造模式破坏也掉紫颂果。
- 本体不可在生存获得（精准采集无效）。

Cuberite 落点：`cChorusPlantHandler::ConvertToPickups` 返回 `cItem(E_ITEM_CHORUS_FRUIT, 1)` 或空。注意默认 `cDefaultItemHandler` 是否允许商品掉落——需要显式覆盖，避免掉本体。

### 2.4 连锁破坏

破坏一株的任一方块后，其上方的植株/花因 §2.2 的支撑检查被逐个破坏（可能一次掉大量紫颂果，原版有性能抖动）。Cuberite 依赖 `OnNeighborChanged` 的逐个传播即可。

---

## 3. 紫颂花（Chorus Flower）方块

### 3.1 方块状态：`age` = 0..5

- 0–4：正常（白色）；5：枯萎（紫色），不再生长。
- **玩家放置**的花 `age = 0`。
- **作为地物自然生成**的花 `age = 5`（已停止生长）。

### 3.2 放置条件（`CanBeAt`）

紫颂花必须在：
- 末地石上方，或
- 紫颂植株上方，或
- 上方为空气、且**恰好有一个**水平相邻的紫颂植株。

不满足时方块**自动破坏且不掉落任何东西**（注意与「玩家破坏」不同）。

Cuberite：`cChorusFlowerHandler::CanBeAt` 实现；失败时需**不掉落**（默认 `OnNeighborChanged` → `DropBlockAsPickups` 会掉落，需要覆盖 `OnNeighborChanged` 或提供「无掉落破坏」路径）。

### 3.3 生长算法（随机刻）

入口：`cBlockHandler::OnUpdate`。Cuberite 已有随机刻：`cChunk::TickBlocks` 每个 section 每 tick 随机抽 3 个方块调用 `OnUpdate`（`src/Chunk.cpp`）。

**触发前提**
- 仅当上方方块是空气、且 `age < 5` 时，随机刻才尝试生长。
- 若上方被堵住，**本次不尝试生长，且 `age` 不变**（与「失败即 age=5」不同）。
- 骨粉对紫颂花无效。

**向上生长**

先按「花下方结构」查表得到向上生长概率；若抽中，再检查「目标方块（花上方一格）本身、其上方一格、其水平四邻」是否全为空气：
- 成功：目标方块变为**同 age** 的紫颂花；原花方块变为紫颂植株。
- 否则向上生长失败。

概率表（`F`=花，`P`=紫颂植株，`S`=末地石，`A`=空气；从上到下排列）：

| 下方结构 | 向上生长概率 |
|---|---|
| `F / S` | 100% |
| `F / P / S` | 100% |
| `F / P / P / S` | 60% |
| `F / P / P / P / S` | 40% |
| `F / P / P / P / P / S` | 20% |
| `F / A` | 100% |
| `F / P / A` | 100% |
| `F / P / P / A` | 50% |
| `F / P / P / P / A` | 25% |
| `F / P / P / P / P / A` | 0% |
| `F / P / P / P / P / P`（无空气/末地石） | 0% |

**水平分枝**（仅当向上生长失败或未尝试，且 `age ≤ 3`）

- 先取「分枝尝试次数」：基础为 **0–3 等概率**；查下表，若命中「增加一次」则变为 **1–4 等概率**：

| 下方结构 | 是否 +1 次尝试 |
|---|---|
| `F / S` | 否 |
| `F / P / S` … `F / P / P / P / P / S`（下方末端是末地石） | **是** |
| 所有 `F / A`、`F / P / A` …（下方是空气或全植株） | 否 |

- 每次尝试随机选一个水平方向；若该方向目标方块、其下方方块、以及其它水平方向均为空气，则成功：目标方块变为 `age + 1` 的紫颂花。
- 只要至少一次分枝成功，原花方块变为紫颂植株。

**失败与终止**
- 若本轮向上与全部分枝都失败：该花的 `age` 直接置为 **5**。
- `age = 5` 的花不再生长。

**高度分布（供验收参考）**
- 完整植株高度 5–22（罕见 22），多数 13–16。
- 花的高度 3–22，多数 11–16。
- 一株可收获 1–8 朵花，均值约 3.7。
- **实测（加速随机刻，手工放置）**：3 朵 `age=0` 的花最终长成 2 棵连通树（118 植株 + 16 花），**16 朵花全部 `age=5`**，两棵树高度分别为 **22** 与 **14**；全部 16 朵花放置合法。与「停止生长时 `age=5`」「高度 5–22」一致。

**音效**
- 成功生长一次：`block.chorus_flower.grow`（1.9+ 命名；1.8 数字 ID `SFX_RANDOM_CHORUS_FLOWER_GROW = 1033`）。
- `age` 达到 5（枯萎）：`block.chorus_flower.death`（`SFX_RANDOM_CHORUS_FLOWER_DEATH = 1034`）。

Cuberite 落点：`cChorusFlowerHandler::OnUpdate`，用 `GetRandomProvider()`；`a_Chunk.GetWorld()->BroadcastSoundEffect("block.chorus_flower.grow", Pos, 1.0f, 1.0f)`。

### 3.4 破坏与掉落

- 被**玩家直接破坏**、或被**爆炸**破坏（按爆炸类型掉率）、或被**多种弹射物**击中：掉落**自身**。
  - 1.12.2 相关弹射物：箭、药箭、光灵箭、雪球、鸡蛋、火球、小火球、龙息火球、凋灵之首等（具体以 1.12.2 存在者为准）。
- **不掉落**的情况：被活塞/水流推动、以及下方支撑方块被破坏（自然消失）。
- 支撑方块被破坏时，花**无掉落**消失。

Cuberite 落点：
- 直接破坏 → `ConvertToPickups` 掉自身。
- 支撑消失 → 覆写 `OnNeighborChanged`，`CanBeAt` 失败时用**无掉落**方式移除（而不是默认 `DropBlockAsPickups`）。
- 弹射物击中 → 需要弹射物命中回调把紫颂花打碎并掉自身（当前 Cuberite 可能没有通用机制，需要评估；若代价太大可作为后续增量）。
- 活塞/水流推动 → 需评估 Cuberite 的推动路径。

---

## 4. 自然生成（紫颂树 feature）

### 4.1 来源描述

- 生成于末地外岛的 **End Highlands** 生物群系（Java 版）。
- 组成：紫颂植株枝干 + 枝顶的紫颂花（自然生成的花 `age = 5`）。
- 高度 5–22，多数 < 16。
- 是紫颂果与紫颂花的唯一自然来源。

### 4.2 放置参数（官方数据包 placed_feature `chorus_plant`，**1.13+ 数据报告**）

```json
{
  "feature": "minecraft:chorus_plant",
  "placement": [
    { "type": "minecraft:count", "count": { "type": "minecraft:uniform", "min_inclusive": 0, "max_inclusive": 4 } },
    { "type": "minecraft:in_square" },
    { "type": "minecraft:heightmap", "heightmap": "MOTION_BLOCKING" },
    { "type": "minecraft:biome" }
  ]
}
```

即：每个 End Highlands 区块 **0–4 次**尝试；每次在区块内随机取一列，取 MOTION_BLOCKING 高度；再经过群系过滤。

> **版本注意**：1.12.2 无数据包，世界生成为硬编码。这里假设 1.12.2 使用相同的 0–4 次/区块与高度图规则（无法用 1.12.2 允许来源独立核实，标为假设）。

### 4.3 feature 的树生成算法——**未文档化**

- wiki 只给出「高度 5–22、枝顶花 age=5」，**没有**给出 feature 内部如何长成树。
- 同时 zh wiki 明确指出：**地物自然生成的花 `age = 5`（放置即停止生长）**。这说明 feature **不是**「放一朵 age=0 的花再走随机刻算法」，而是直接构造整棵树并在枝顶放已枯萎的花。
- ⇒ 本规格把 feature 的精确算法列为**待确认**。可选实现路径：
  1. 用 §3.3 的生长算法，从末地石上的 age=0 花开始模拟整轮生长（用世界随机源），直到终止——能产出符合 §3.3 高度分布的树，并与「枝顶花枯萎」自然吻合；作为**近似**，标注偏差。
  2. 若能通过合法实测总结出 feature 的树形规律，再据以精确实现。

### 4.4 Cuberite 接入点

- 末地世界当前 `Finishers=EnderDragonFightStructures`，且该 token 已额外挂上自然回归折跃门 finisher（见 `feature/generating-end-return-gateway`）。
- 本特性建议同样挂在既有 End finisher 链上（或新 finisher），实现：
  1. 每区块 0–4 次尝试（用与 `cEnderDragonReturnGatewayGen` 相同的 `std::minstd_rand` + 世界种子/区块坐标，保证确定、跨平台）。
  2. 每次随机取列、取地表高度；**仅当该列地表为末地石**、且该处为空气时，才生成。
  3. 用「距离 > 1024」近似 End Highlands（与回归折跃门一致）。
- 生成可复用 §4.3 的生长算法（放在可由 finisher 与方块的 `OnUpdate` 共用的地方，例如一个 `cChorusPlant` 工具类）。
- 生成出来的花直接写 `age = 5`（若按路径 1 生长，则最终自然停在 5）。

---

## 5. 紫颂果（Chorus Fruit）——复核已实现部分

原版行为（用于核对现有实现）：

- 食用恢复 4 饥饿 / 2.4 饱和；**饱食时也能吃**。
- 传送：最多 **16** 次尝试，在 **±8**（三轴）内选随机目的地，方式同末影人；可传送进 2 格高空间（爬行玩家/狐狸 1 格）。
- 会尽量避开流体，但可穿固体；可落在火/仙人掌/岩浆块上。
- 成功后有末影人式传送音（`item.chorus_fruit.teleport`）。
- 冷却 **1 秒**（对背包内所有紫颂果生效）。
- 食用后**清除全部摔落伤害**。

**现有实现**：`cItemChorusFruit.h` 调 `cPawn::FindTeleportDestination(*World, 2, 16, Destination, Pos, 8)`，广播 `item.chorus_fruit.teleport`，移除一个物品。

**待核对/可能的偏差**：
- 冷却 1 秒是否实现；
- 是否清除摔落伤害；
- `FindTeleportDestination` 是否避开流体、是否允许穿固体、是否覆盖「16 次尝试/±8」的精确语义。

（本规格不要求立即修这些，列为后续增量。）

烤紫颂果：熔炉 0.1 经验，已实现（`Server/furnace.txt`）。不可食用。

---

## 6. Cuberite 集成点清单（实现时）

| 改动 | 位置 |
|---|---|
| 新 `cChorusFlowerHandler`（`age`、`OnUpdate` 生长、`CanBeAt`、`OnNeighborChanged` 无掉落、`ConvertToPickups`） | `src/Blocks/BlockChorusFlower.h`（新） |
| 新 `cChorusPlantHandler`（`CanBeAt` 支撑、`ConvertToPickups` 紫颂果） | `src/Blocks/BlockChorusPlant.h`（新） |
| 全局常量/注册：替换两处 `cDefaultBlockHandler` | `src/Blocks/BlockHandler.cpp`（`constexpr` 定义 + `case` 返回） |
| 编译 | `src/Blocks/CMakeLists.txt` |
| 自然生成 finisher | `src/Generating/`（新文件 + `ComposableGenerator.cpp` 注册 + `CMakeLists.txt`） |
| 共用树生成工具（可选） | 例如 `src/Generating/ChorusPlant.h` 或 `src/Blocks/` 内静态函数 |
| 音效 | `cWorld::BroadcastSoundEffect` |
| 随机刻 | 现有 `cChunk::TickBlocks` → `OnUpdate`，无需改引擎 |

不改：`src/Registries/*`（生成物）、`Server/Plugins/Core`（submodule）、任何 `lib/`。

---

## 7. 偏差与待确认（实现前需明确）

1. **1.12.2 紫颂植株连接元数据**：**已实测确认**服务端不存储连接（`meta` 恒 0，见 §2.1），客户端渲染时自行推导。实现时服务端只需按普通方块发送 `199 + meta 0`，无需维护连接位。
2. **自然 feature 的树生成算法未文档化**（§4.3）；路径 1 为近似，需标注。
3. **placed_feature 的 0–4 次/区块来自 1.13+ 数据包**，1.12.2 硬编码，假设一致。
4. **End Highlands 近似**：Cuberite 末地无生物群系，用「距离 > 1024」代理（与折跃门实现一致）。
5. **PRNG**：自然生成用本仓库的 `std::minstd_rand`（确定性），与 vanilla 装饰器 PRNG 不同；只保证分布/规则一致。
6. **紫颂花被弹射物破坏**、**活塞/水流推动不掉落**：原版行为明确，但 Cuberite 是否有对应机制需评估，可能拆为后续增量。
7. **生长概率表的措辞**：向上成功需检查「目标、目标上方、目标水平四邻均为空气」（zh wiki 表述）；英文 wiki 表述略含糊，以上表为准，实现时逐条对照。

---

## 8. 验收与测试计划

门（沿用 AGENTS §4）：
- `cd src && lua CheckBasicStyle.lua`
- `cmake --build build`
- `cd build && ctest --output-on-failure -E "UrlClient-test|Google-test"`

单元测试（`tests/`）：
- `cChorusFlowerHandler`：给定固定随机种子，验证「向上表概率/分枝次数/失败 age=5」在大量样本下的分布落在期望区间。
- `CanBeAt`：构造各种邻居组合，验证支撑/自动破坏规则（尤其侧向接枝与「垂直空气」规则）。
- `ConvertToPickups`：紫颂植株只掉紫颂果、不掉本体；紫颂花掉自身。
- finisher：给定固定种子扫描区块，验证「只在末地石地表、距离 > 1024、高度 5–22」以及结构合法性（无悬空、枝顶花 age=5）。

（无 vanilla oracle：以「wiki 规格 + 单元测试」为准，实机对照后续补。）

---

## 9. 分支拆分建议

本特性体量较大，建议拆成两个可独立评审的分支：

1. `feature/blocks-chorus-plant`：方块处理器（`age`、生长、支撑、掉落）——行为核心。
2. `feature/generating-chorus-plant`：外岛自然生成 finisher。

本文档随第 1 个分支落地；第 2 个分支引用本文档。若维护者希望一次做完，则合并为一个分支。
