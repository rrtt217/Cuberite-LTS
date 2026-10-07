# Vanilla 1.12.2 — 末地城（End City / End Ship）

来源（cleanroom 白名单）：
- [Minecraft Wiki — End City](https://minecraft.wiki/w/End_City)（正文、生成规则、结构目录、掉落概览）
- [Minecraft Wiki — End City/Structure](https://minecraft.wiki/w/End_City/Structure)（各蓝图子页；逐方块 ASCII 蓝图，本规格的房间几何以此为准）
- 蓝图子页：[/Base](https://minecraft.wiki/w/End_City/Structure/Base)、[/Small Tower](https://minecraft.wiki/w/End_City/Structure/Small_Tower)、[/Large Tower](https://minecraft.wiki/w/End_City/Structure/Large_Tower)、[/Small Room](https://minecraft.wiki/w/End_City/Structure/Small_Room)、[/Large Room](https://minecraft.wiki/w/End_City/Structure/Large_Room)、[/Loot Room](https://minecraft.wiki/w/End_City/Structure/Loot_Room)、[/Empty Room](https://minecraft.wiki/w/End_City/Structure/Empty_Room)、[/Bridge](https://minecraft.wiki/w/End_City/Structure/Bridge)、[/Ship](https://minecraft.wiki/w/End_City/Structure/Ship)
- [Minecraft Wiki — End Ship](https://minecraft.wiki/w/End_Ship)（重定向至 End City 的 End Ship 节）
- 1.12.2 客户端存档（Cleanroom 0.6.13-alpha）实机观察 + Cuberite 末地实机（后续验收补）

本文只写规格 + 架构判定，不含实现。凡 wiki 未明示、由推断得到的部分，一律标注 **推测/待确认**。

---

## 0. 当前 Cuberite 实现状态（基线，master）

### 0.1 已具备（不要重复造）

| 项 | 位置 |
|---|---|
| 方块 ID：E_BLOCK_PURPUR_BLOCK/PILLAR/STAIRS/SLAB/DOUBLE_SLAB（201-205） | src/BlockType.h:220-224 |
| E_BLOCK_END_BRICKS = 206、E_BLOCK_END_ROD = 198 | src/BlockType.h:217、src/BlockType.h:225 |
| E_BLOCK_CHEST、E_BLOCK_ENDER_CHEST = 130、E_BLOCK_BREWING_STAND = 117 | src/BlockType.h:145 |
| E_BLOCK_HEAD = 144（meta E_META_HEAD_DRAGON = 5）、E_BLOCK_STANDING_BANNER = 176 / E_BLOCK_WALL_BANNER = 177 | src/BlockType.h:159、src/BlockType.h:195-196 |
| E_BLOCK_STAINED_GLASS = 95（品红色 meta） | src/BlockType.h:110 |
| 方块实体：cChestEntity、cEnderChestEntity、cBrewingstandEntity、cBannerEntity、cMobHeadEntity | src/BlockEntities/ |
| 实体：cItemFrame | src/Entities/ItemFrame.h |
| mtShulker 枚举 + 存档 NBT 读写（仅能加载/保存，不能生成） | src/Mobs/MonsterTypes.h:57、src/WorldStorage/WSSAnvil.cpp:3149 |
| 通用多区块结构框架：cGridStructGen（格点 + LRU 缓存 + 按区块绘制） | src/Generating/GridStructGen.h、src/Generating/GridStructGen.cpp |
| 预制件系统：cPrefab（char-map 图像、4 向旋转、merge、align-to-ground）、cPrefabPiecePool、.cubeset 加载 | src/Generating/Prefab.h、src/Generating/PrefabPiecePool.h |
| 参照实现：cVillageGen（cGridStructGen + cPrefabPiecePool + cPieceGeneratorBFSTree） | src/Generating/VillageGen.cpp |
| cBlockArea 支持 baBlockEntities；cChunkDesc::WriteBlockArea 会连方块实体一起 merge | src/BlockArea.h:53、src/Generating/ChunkDesc.cpp:271 |
| 结构生成时手动填充箱子战利品的既有做法 | src/Generating/MineShafts.cpp:746、src/Generating/DungeonRoomsFinisher.cpp:187 |
| 末地 finisher 链 | Finishers = EnderDragonFightStructures（src/Generating/ComposableGenerator.cpp:268） |

### 0.2 缺失（本规格覆盖）

1. 末地城生成：src/Generating/ 无任何 End City 引用。
2. 末地船生成。
3. 潜影贝生成：cMonster::NewMonsterFromType 无 mtShulker 分支（src/Mobs/Monster.cpp:1262），无 cShulker 类——这是硬依赖，见 8.1。
4. 末地城战利品表：Cuberite 用硬编码概率数组，末地城无对应实现。
5. 品红色旗帜图案 NBT、酿造台药水内容、物品展示框里的鞘翅：cPrefab 的 char-map 只能定位 type+meta，装不了 NBT；需结构生成器额外写方块实体/实体。

---

## 1. 基础数据

| | 值 |
|---|---|
| 结构 ID（1.12.2 数字 / 1.13+ nameid） | 无独立 ID（1.12.2 硬编码）/ minecraft:end_city |
| 维度 | 末地（dimEnd） |
| 生物群系（Java） | End Midlands、End Highlands（大岛）；Cuberite 末地只有 biEnd = 9（src/BiomeDef.h:32-33） |
| 组成方块 | Purpur（块/柱/台阶/半砖）、End Stone Bricks、End Rod、Magenta Stained Glass、Ladder、Chest、Ender Chest、Brewing Stand、Obsidian、Magenta Banner、Dragon Head、Item Frame(鞘翅) |
| 生物 | Shulker |
| 蓝图尺寸 | 末地船 24 高 x 29 长 x 13 宽（/Ship）；城市可超 100 格高（wiki Trivia） |

---

## 2. 生成规则

### 2.1 格点（wiki 明文）

> "End cities are generated in a noticeable grid. They are located only in chunks numbered 0-8 ± a multiple of 20."

即：结构原点的区块坐标 == 0..8 (mod 20)，两个轴向独立。例：x_chunks 0-8、z_chunks 80-88。

解读（推测）：按标准结构间距/间隔语义，spacing = 20 区块、separation = 11 区块，原点区块在 [0, 20-11) = 0..8。本规格以 wiki 的「0..8 mod 20」为准；separation=11 仅作解释，不额外采信。

### 2.2 地形/群系门（待确认/近似）

- 生成于末地外岛的大岛，生物群系 End Midlands / End Highlands。
- Cuberite 无这两个群系，必须用几何代理。参考紫颂树分支的做法：距离世界原点 > 1024 块（与 cEnderDragonReturnGatewayGen、紫颂树 finisher 一致）。
- 另需「地面足够平坦」：末地城塔基要落在末地石地表。具体规则 wiki 未给，待确认。建议：取结构 8x8 区块足迹的高度图，要求高度差 <= 阈值且地表为 E_BLOCK_END_STONE；阈值待实机标定。
- Y 放置：wiki 只说「通常生成在平坦处」，未给算法。待确认；参考原版是「塔基贴地、上方留出整座城的高度」。实现时以足迹高度图的最高点为基座 Y，并向下补齐到地表。

### 2.3 与紫颂树/回归折跃门的关系

- 三者都在外岛。紫颂树可生成在末地城房间里（wiki 画廊 EnderHouseplant.png：「A chorus tree generated inside of a room」）——说明两者会重叠。谁覆盖谁由 finisher 顺序决定，待确认（列为实现后实机观察项）。
- 建议：末地城 finisher 排在紫颂树之前或之后需要单独验证；不得用「末地城清空房间内紫颂树」这类无来源的规则。

---

## 3. 城市布局（骨架）

### 3.1 wiki 明文的构件

| 类别 | 内容 |
|---|---|
| 基础层 | Base floors：每座城最底部的空房间，三层，每层比下一层更宽 |
| 房间 | Banner Room（外挂旗帜 + 天花板潜影贝）、Small Room（空）、Large Room（复杂楼梯通向塔或战利品房）、Loot Room（2 箱子 + 小跑酷）、Empty Room（入口） |
| 塔 | Small tower：空心，单条紫颂台阶螺旋梯；Fat tower / Skyscraper：三倍直径，双螺旋（台阶 + End Rod），潜影贝多 |
| 连接 | 桥（bridge_piece 直段 / bridge_gentle_stairs 缓坡 / bridge_steep_stairs 陡坡 / bridge_end 拱门），桥末端可能是 End ship |
| 桥/船概率 | 每座塔的每个方向 50% 生成桥；每座桥 12.5% 生成末地船；每座城最多 1 艘船 |

### 3.2 骨架算法——wiki 未文档化，标为待确认

wiki 只描述构件与概率，没有给出「三层基础层如何加宽、之后选小塔还是胖塔、塔加多高、何时收顶」的精确递归。给出如下推断骨架（实现按此做，但必须标注近似，实机对照后再修）：

1. 在原点放入口层（base_floor / Empty Room），Y 贴地。
2. 依次叠第一、二、三层基础房间（base_floor -> second_floor_1|2 -> third_floor_1|2），每层外廓按蓝图放大；层间用 *_roof 收口。
3. 到顶后决定塔型（small vs fat）——选择概率 wiki 未给，待确认；可先按等概率或由高度决定。
4. 塔按「塔基 -> 塔身 xN -> 塔顶」竖直堆叠；Small tower 用 tower_base/tower_piece/tower_top，Fat tower 用 fat_tower_base/fat_tower_middle xN/fat_tower_top。塔身层数决定城市高度（可 > 100 格）。
5. 对每座塔的每个水平方向独立掷 50%：命中则从该方向接桥；桥由若干 bridge_piece / *_stairs 组成，末端按概率接 bridge_end。
6. 桥末端掷 12.5% 生成末地船；全城只允许一艘（用一个 bool 全局锁）。
7. 塔顶/房间按蓝图放潜影贝（第 4 节的固定数量）、箱子、旗帜等。

> 与紫颂树 feature 同样的问题：官方是硬编码递归，wiki 不公开算法。路径 1（本推断骨架）为近似；不得凭模型记忆补精确常量。

### 3.3 房间几何来源

End City/Structure 的每个子页都给出 layered blueprint（逐层 ASCII 图 + 字符表 + 面向/半砖朝向说明）与材料统计。这是允许来源（Minecraft Wiki 的机制/结构描述），可据此手工重画预制件；不得使用 minecraft.jar 里的 data/minecraft/structures/end_city/*.nbt（Mojang 资源，cleanroom 禁止）。

每页蓝图均声明了半砖 Bottom/Top、楼梯朝向、柱头方向等；cPrefab 的 char-map 可编码 type + meta，足够表达。

### 3.4 桥端（bridge_end）与两端的接缝

来源：wiki 的 [/Bridge](https://minecraft.wiki/w/End_City/Structure/Bridge) 把这块叫 **Dock**（材料 21 Purpur Block / 4 Purpur Slab / 3 Purpur Stairs / 2 End Rod / 1 Purpur Pillar，与本仓库 `BridgeEnd` 蓝图一致），[/Empty_Room](https://minecraft.wiki/w/End_City/Structure/Empty_Room) 给出房间几何；接缝形状另有 1.12.2 客户端实测观察。

- **桥的两端各有 bridge_end**：塔端（桥的起点）和远端（接房间 / 末地船 / 另一座塔）。
- Dock 蓝图的**近端那排**是 `BSSSB` 甲板 + End Rod（层 2 的 `E   E`），**远端那排**是 ` BBB `/`BBBBB` 加层 3/4 的 `BH HB` / `HBBBH` 顶冠。远端朝目的构件，近端朝桥。
- 接**大塔 / 小塔**（桥的起点）：拱门远端那排就落在塔外墙那一格里 —— 与墙面齐平并嵌入墙内，塔墙上因此出现 3 格宽的拱门洞。
- 接**房间**：拱门下沿（层 0 的 `BBBBB`/`BSSSB`）与房间第一层的地面同层；沿桥轴向上，拱门远端那排紧邻房间地板的第一格，**不重叠也不留空隙**；横向房门洞的格子与拱门洞的 3 格对齐。

实现约束（本仓库蓝图坐标体系，local y = wiki 层号 + 1）：

- 桥构件（BridgePiece / 两种楼梯 / BridgeEnd）的甲板在 **local y=1**，而塔层与房间的地板在 **local y=0**。桥必须放在 `a_BranchY - 1`，甲板块才落在塔层地板块那一层。
- 楼梯最上一级踏面在 **local y=5**（wiki 层 4），其上方 local y6 只是栏杆。因此楼梯之后 Y 前进量是 `5 - 1 = 4`；旧代码用「结构顶（local y6）+ 1 = 6」，会让后面每一件（包括拱门）比楼梯出口高 2 格 —— 这就是「拱门和桥脱节」。
- 房间的包围盒比它的地板宽（屋顶 / 上层地板悬挑）；把房间按门洞对齐会把拱门压进房间，拱门顶冠被屋顶盖掉。正确做法是沿桥轴把房间的起点定在拱门远端那排，横向再按门洞列对齐。
- 门洞 carve 盒是 3×3×3（沿桥轴 ±1），会向后吃掉一格拱门；接房间时应把 carve 中心放到房间一侧。

### 3.5 原版 piece 名单（权威）

来源：[End City](https://minecraft.wiki/w/End_City) 的 “Structure details” 表 —— 它列出 `data/minecraft/structures/end_city` 里的全部 20 个结构名，各附投影图 `File:End city <name>.png`：

`base_floor`、`base_roof`、`second_floor_1`、`second_floor_2`、`second_roof`、`third_floor_1`、`third_floor_2`、`third_roof`、`tower_base`、`tower_piece`、`tower_floor`、`tower_top`、`fat_tower_base`、`fat_tower_middle`、`fat_tower_top`、`bridge_piece`、`bridge_gentle_stairs`、`bridge_steep_stairs`、`bridge_end`、`ship`。

**没有** `loot_room` / `empty_room` / `small_room` / `large_room` 这几个结构名 —— 「战利品房 / 空房间 / 旗帜房 / 大房间」是房间**类型**（见 End City 正文），落到实件上分别是 `third_floor_2`（+ `fat_tower_top`）、`base_floor`、`tower_top`、以及 `/Large_Room` 页那两种带复杂楼梯的房间。

拆件关系（已核对材料数）：

| 实件 | 等于 |
|---|---|
| `base_floor` | 本仓库 `EmptyRoom` 的第 0–3 层，**也是** `BaseRoom` 的第一层（同一件，无顶） |
| `base_roof` | `EmptyRoom` 的第 4–5 层（100 Purpur Block + 44 Stairs + 4 End Rod） |
| `EmptyRoom`（旧合称） | `base_floor` + `base_roof` |
| `BaseRoom`（旧合称） | `base_floor` + `base_roof` + `second_floor_*` + `second_roof` + `third_floor_*` + `third_roof` 按 18×18 帧叠起来的整栋 |

`base_floor` 的投影图（`File:End city base_floor.png`）确认它是**敞口无顶**的小房间：地面 + 3 层墙，墙顶一圈就是最高处。

因此桥端房间必须用 `base_floor`（无顶）而不是合并件 —— 否则屋顶悬挑正好落在拱门顶冠 `HBBBH` 那一层，把它整排盖掉。

### 3.6 本仓库已落地的件与残留偏差

已按上面的拆分实现（材料数与 Wiki 表逐项核对）：

| 件名 | 状态 |
|---|---|
| `BaseFloor`（12×12×4） | ✓ 68 PB / 54 ESB / 12 glass / 12 pillar / 2 stairs，与 `base_floor` 完全一致 |
| `BaseRoof`（12×12×2） | ✓ 100 PB / 44 stairs / 4 rod，与 `base_roof` 完全一致 |
| `SecondFloor1`（18×18×7） | /Base 画布第 **1–7** 层：第 1–3 层只保留它的半砖螺旋梯（`L-P-L` 斜列，位于第一层屋顶**下面**、`base_floor` 里没有这些格），第 4–7 层是屋顶 + 二层墙。装配时从画布第 1 层放下，因此这三层楼梯会覆盖到第一层结构上 |
| `ThirdFloor1`（18×18×4） | /Base 画布第 8–11 层，含下面那层屋顶；64 stairs / 32 glass 与 `third_floor_1` 表吻合 |
| `SecondRoof` / `ThirdRoof` | 对应画布第 8 层 / 第 12–13 层；`second_roof` 只在基座提前封顶时用 |
| `TowerBase` / `TowerPiece` / `TowerTop` | ✓ 与 `tower_base` / `tower_piece` / `tower_top` 完全一致 |
| `BridgePiece` / `BridgeGentleStairs` / `BridgeEnd` | ✓ 完全一致 |
| `BridgeSteepStairs` | 按 /Bridge 的 ASCII 图逐格转录（16 PB / 1 pillar）；Wiki 材料表写 15 PB / 2 pillar，两者自相矛盾，以图为准 |
| `FatTowerTop` | ✓ 完全一致（玻璃原误用紫色，已改回品红） |
| `Ship` | ✓ 完全一致 |
| `FatTowerBase`（14×13×4） | ✓ 84 pillar / 80 PB / 4 stairs / 4 rod / 3 slab，与 `fat_tower_base` **完全一致**；由 `FatTower` 第 0–3 层切出 |
| `FatTowerMiddle`（14×13×8） | 由 `FatTower` 第 4–11 层切出；Wiki 表写 164 pillar / 62 PB，我们转录出 172 / 56，差 8 / 6 格（以 /Large_Tower 的 ASCII 图为准） |
| `TowerFloor` | 由 `TowerPiece` 底面固化推导（Wiki 未给该件蓝图），材料数与 Wiki 表不符，待重做 |
| `LootRoom2` / `LootRoom3` | 实为 Wiki `/Large_Room` 的两层 / 三层变体；帧被补齐到 30×18 / 31×16（内容只占其中一段），**待按内容重框** |

`base_floor` 的包围盒与其地板不同范围（地板 x=1..9，屋顶件 x=0..11），所以与桥拱门对接时一律用**最下层范围**（`m_FloorMinX/MaxX/MinZ/MaxZ`）定位，不用整件包围盒。

---

## 4. 构件目录（源自 End City 的 Structure details）

| nameid（1.13+） | 说明 | 固定潜影贝 | 蓝图 |
|---|---|---|---|
| end_city/base_floor | 入口/空房间，最底层 | 2 | /Base、/Empty Room |
| end_city/base_roof | 基础层屋顶 | - | /Base |
| end_city/second_floor_1 | 第二层（空，台阶入口） | - | /Base |
| end_city/second_floor_2 | 第二层（中心螺旋梯 + 楼梯雕像） | 1 | /Base |
| end_city/second_roof | 第二层屋顶，更大 | - | /Base |
| end_city/third_floor_1 | 第三层（中央紫颂柱 + 楼梯） | - | /Base |
| end_city/third_floor_2 | 第二战利品房（普通箱 + 末影箱） | 2 | /Loot Room |
| end_city/third_roof | 第三层屋顶，最大 | - | /Base |
| end_city/tower_base | 小塔入口，带梯子 | - | /Small Tower |
| end_city/tower_floor | 小塔实心层 | - | /Small Tower |
| end_city/tower_piece | 小塔中段，螺旋梯 | - | /Small Tower |
| end_city/tower_top | 小塔顶，外挂品红旗帜 | 1 | /Small Room、/Small Tower |
| end_city/fat_tower_base | 胖塔入口 | - | /Large Tower |
| end_city/fat_tower_middle | 胖塔中段，双螺旋 | 4 | /Large Tower |
| end_city/fat_tower_top | 胖塔战利品房（2 箱 + 上台楼梯） | - | /Large Room、/Loot Room |
| end_city/bridge_end | 桥端拱门 | - | /Bridge |
| end_city/bridge_piece | 直桥段 | - | /Bridge |
| end_city/bridge_gentle_stairs | 缓坡桥 | - | /Bridge |
| end_city/bridge_steep_stairs | 陡坡桥 | - | /Bridge |
| end_city/ship | 末地船 | 3 | /Ship |

辅助房间（不单独占 nameid）：Small Room（/Small Room，天花板潜影贝 1、品红黑纹旗帜）、Large Room（/Large Room，两层）、Loot Room（/Loot Room，2 箱）、Empty Room（/Empty Room，入口）。

> 潜影贝数量以 wiki「Structure details」表的 "always generate" 为准；实现时按构件硬编码。

---

## 5. 掉落（Chest loot）

- wiki 页用 LootChest 模板生成战利品表（模板，正文 raw 里不含数据）。可获得的代表物品：附魔铁/钻石工具武器盔甲、金苹果、末影珍珠、命名牌、马鞍、铁锭/金锭、钻石、绿宝石、甜菜种子、药水，以及 1.11 起加入的诅咒附魔。
- 精确权重/条目：待取。1.12.2 为硬编码 loot table；实现时需要一份允许来源的条目+权重，落成 Cuberite 的 GenerateRandomLoot* 概率数组（参照 src/Generating/DungeonRoomsFinisher.cpp:228-233）。
- 末地船另有：2 个箱子、酿造台（含 2 瓶治疗 II）、物品展示框中的鞘翅、船首龙首（E_BLOCK_HEAD meta 5）。

---

## 6. 架构判定：当前 Cuberite 下用什么形式生成末地城

> 这是本次要回答的核心问题。

### 6.1 候选形式

| 方案 | 复用 | 适配度 |
|---|---|---|
| A. cPieceStructuresGen + .cubeset + cPieceGeneratorBFSTree | 完全数据驱动 | 不适配 |
| B. 新 cEndCityGen : cGridStructGen，自写骨架引擎 + 复用 cPrefab 几何 | 格点/缓存/绘制全复用 | 推荐 |
| C. cSinglePieceStructuresGen | 单件 | 不适配 |
| D. 裸 cFinishGen（仿 cEnderDragonFightStructuresGen） | 无 | 可作备选 |

### 6.2 为什么排除 A（纯 BFS 预制件池）

- 末地城骨架是算法化的：基础层按固定次序加宽、随后在小塔/胖塔间做决策、塔竖直堆叠固定层数、每方向 50% 接桥、全城至多 1 船。cPieceGeneratorBFSTree 是按连接器 + 权重随机扩展的随机树，无法自然表达「固定次序的线性塔身」「全局唯一一艘船」「每个塔每方向的独立 50%」。
- 虽然可像 cVillage 那样包一层带状态的 cPiecePool（src/Generating/VillageGen.cpp:231），但会把强次序逻辑塞进权重里，脆弱且难评审。
- 结论：不采用纯 BFS 池。

### 6.3 为什么排除 C/D

- C（cSinglePieceStructuresGen）：只放一件预制件，无法表达多塔+桥+船。
- D（裸 cFinishGen）：跨区块结构的确定性缓存、按区块求交、LRU 都要自己重写，等于把 cGridStructGen 抄一遍。除非要偏离格点语义，否则没必要。

### 6.4 推荐：方案 B

新建 cEndCityGen : cGridStructGen，内含一个 cGridStructGen::cStructure 派生，做两件事：

1. 骨架引擎（自写）：在 CreateStructure(gridX, gridZ, originX, originZ) 里用「世界种子 + 格点坐标」确定性地生成整座城的放置列表，每个元素 = {cPrefab*、绝对坐标、CCW 旋转数、是否船}。骨架按 3.2 的推断算法。
2. 绘制：DrawIntoChunk(cChunkDesc&) 对每个与当前区块相交的房间调 cPrefab::Draw(a_Chunk, coords, rotations)（src/Generating/Prefab.cpp:142），随后按记录补写方块实体（箱子/末影箱/酿造台/旗帜）与实体（潜影贝/物品展示框）。

关键复用点
- cGridStructGen 已提供格点划分、跨区块确定性重建、LRU 缓存、按区块求交（src/Generating/GridStructGen.cpp:117-235）。
- cPrefab 提供 char-map 图像、4 向旋转、merge 策略、efs* 贴地扩展（src/Generating/Prefab.h）。骨架引擎不需要连接器，直接构造放置即可（cPlacedPiece 只是可选载体，也可用自有轻量 struct + 直接调 cPrefab::Draw）。
- 房间几何从 3.3 的 wiki 蓝图手工重画。两种承载方式：
  - B1（推荐）：编译进 C++ 的 cPrefab::sDef 数组（如 EndCityPieces.cpp），零运行时文件依赖，和末地城骨架一起评审。
  - B2：.cubeset 放到 Server/Prefabs/EndCity/，由生成器加载；数据可热改，但 cubeset 描述体量大、且骨架仍要在 C++ 里写。

格点参数映射（重要）
- GridSizeX = GridSizeZ = 20 * 16 = 320 块。
- cGridStructGen 自身把原点取成 GridX 加上 [0, MaxOffset) 的噪声偏移（src/Generating/GridStructGen.cpp:192-193），与 wiki 的「原点区块在 0..8」不同。推荐：在 CreateStructure 内忽略传入的 a_OriginX/Z，自己按 hash(gridX, gridZ) % 9 取区块偏移（0..8），并把 MaxOffset 设为 >= 8 区块（=128 块）仅作为求交包围盒的余量；MaxStructureSize 取 >= 8 区块（城市可 >100 高、船 29 长，水平包围至少 8 区块）。
- 该映射必须单测（第 10 节）。

接入点
- 新增 finisher token（建议 EndCity），在 ComposableGenerator::InitFinishGens 注册，并追加到 End 默认 Finishers（当前 EnderDragonFightStructures，src/Generating/ComposableGenerator.cpp:268）。
- 构造时传入 *m_CompositedHeightCache（不是 ShapeGen/CompositionGen）——沿用紫颂树分支修复过的性能教训，避免为邻域重复生成完整地形。
- 使用的随机源与末地其他 finisher 一致（std::minstd_rand / cNoise），保证确定性、跨平台；只保证分布/规则一致，不保证与 vanilla PRNG 同序。

### 6.5 生成顺序与命名

- 建议文件：src/Generating/EndCityGen.h/.cpp（骨架）+ src/Generating/EndCityPieces.h/.cpp（cPrefab::sDef 数据，若选 B1）。
- 与紫颂树 finisher 的先后由实机决定；默认建议末地城在紫颂树之后（先放树、后放城）。wiki 观察到的「房间里的紫颂树」只是说明城可覆盖树区域，不是硬证据——标为待确认。

### 6.6 本分支实现状态（feature/generating-end-city）

本分支已落地：

- src/Generating/EndCityGen.{h,cpp}：cEndCityGen : cGridStructGen（格点 320 块 / 原点区块 0..8 / 外岛距离门 / 平坦度门），骨架 = 入口（EmptyRoom）-> 基础层（BaseRoom）-> 小塔（SmallTowerBase + N x SmallTowerExtension + SmallRoom）或胖塔（N x LargeTower + LootRoom）-> 每方向 50% 桥（Bridge）-> 每桥 1/8 船（Ship，全城至多一艘）。
- src/Generating/EndCityBlueprintData.{h,cpp}：从 wiki layered blueprint **逐方块转录**的 14 个蓝图（BaseRoom、SmallTowerBase/Extension、LargeTower、SmallRoom、LargeRoomTwoStorey/ThreeStorey、LootRoom、EmptyRoom、Bridge/GentleStairs/SteepStairs/Dock、Ship）；运行时由 char map + 层串构建 cBlockArea，裁剪到非空气包围盒后包成 cPrefab。
- finisher token EndCity，已加入 End 默认 Finishers（EnderDragonFightStructures, EndCity）。
- tests/Generating/EndCityTest.cpp：格点规则、正常生成、三道拒绝门（虚空 / 内岛 / 不平坦）、确定性。

**已知偏差（待实机对照）**：

1. 方块布局来自 wiki 蓝图；但各构件之间的**锚点与拼接顺序**不是 vanilla 的（wiki 不公开 EndCityPieces 的递归与偏移），骨架按几何堆叠近似。
2. 楼梯 / 旗帜 / 龙首 / 梯子的**朝向**为最佳猜测（蓝图以文字+羊毛标注表示翻面，未逐块编码）。
3. 桥梁的缓/陡坡与船坞（BridgeGentleStairs/SteepStairs/Dock）、LargeRoom 两个变体、SmallRoom 之外的房间变体尚未接入骨架。
4. 潜影贝、战利品表、旗帜图案 NBT、酿造台内容、物品展示框均未实现（见第 8 节）。

---

## 7. Cuberite 集成点清单

| 改动 | 位置 |
|---|---|
| 新 cEndCityGen（cGridStructGen 派生 + 骨架引擎） | src/Generating/EndCityGen.{h,cpp}（新） |
| 房间预制件数据（B1） | src/Generating/EndCityPieces.{h,cpp}（新） |
| finisher 注册 + End 默认链 | src/Generating/ComposableGenerator.cpp |
| 构建列表 | src/Generating/CMakeLists.txt |
| 方块实体填充（箱子/末影箱/酿造台/旗帜） | 骨架 DrawIntoChunk，参照 src/Generating/MineShafts.cpp:746 |
| 实体生成（潜影贝/物品展示框） | a_Chunk.GetEntities()（src/Generating/ChunkDesc.h:240）——依赖 8.1 |
| 单测 | tests/Generating/（参照紫颂树分支的 ChorusTreeTest） |

不改：src/Registries/*（生成物）、Server/Plugins/Core 等 submodule、lib/*、运行期产物。

---

## 8. 硬依赖与缺口（实现前必须决策）

1. 潜影贝（Shulker）实体不存在：cMonster::NewMonsterFromType 无 mtShulker（src/Mobs/Monster.cpp:1262），src/Mobs/ 无 Shulker.{h,cpp}。要么先做 cShulker（独立较大特性：AI、壳、ShulkerFacingDirection 等元数据，见 src/Protocol/Protocol.h:247-249），要么本期只生成方块、不生成潜影贝（明显偏差，需维护者拍板）。
2. 箱子战利品表：需按允许来源整理 end-city 战利品条目/权重，落成概率数组。
3. 旗帜图案 NBT：蓝图要求「品红底 + 黑色 chevron + 黑色 inverted chevron」，char-map 只能给方块类型+meta，需直接写 cBannerEntity 的图案。
4. 酿造台内容：2 瓶治疗 II，需写 cBrewingstandEntity 槽位。
5. 物品展示框 + 鞘翅：cItemFrame 实体 + 内部物品，需实体生成器支持（cItemFrame 是否能由世界生成直接创建，待确认）。
6. 龙首：E_BLOCK_HEAD meta 5 + 朝向；cMobHeadEntity 可承载。
7. End Rod / 楼梯 / 半砖朝向：蓝图给了 rot90/rot180/rot270 与 Bottom/Top，需逐一映射到 Cuberite 的 meta 编码（参照 E_META_*）。

---

## 9. 偏差与待确认

1. 骨架算法未文档化（3.2）：本推断为近似，需实机对照后修。
2. 格点：wiki 只给「区块 0..8 mod 20」；spacing=20 / separation=11 为推测。
3. 群系门：Cuberite 无 End Midlands/Highlands，用「距离 > 1024 + 平坦度」代理（与紫颂树一致）。
4. Y 放置/贴地规则：wiki 未给，待确认。
5. 塔型选择概率、塔身层数：wiki 未给，待确认。
6. 紫颂树/末地城重叠与顺序：待实机观察。
7. PRNG 不同序：只保证规则/分布，不保证逐方块与 vanilla 相同。
8. 战利品权重：待取（第 5 节）。

---

## 10. 验收与测试计划

门（沿用 AGENTS 第 4 节）：CheckBasicStyle.lua、cmake --build build、ctest -E "UrlClient-test|Google-test"。

单元测试（tests/Generating/）：
- 格点：固定种子扫描大量区块，断言每个结构原点满足「区块坐标 == 0..8 (mod 20)」，且同原点的坐标可重复生成。
- 骨架不变量：每座城恰好一个入口；至多一艘船；塔身竖直连通；桥只从塔的水平方向出发；全城总高在 [下限, 上限] 内（上限 >100，wiki Trivia）。用 500 个种子跑，统计高度/船数分布。
- 房间合法性：所有房间不与已有房间的 hitbox 相交（复用一个 cCuboid 求交检查）；房间都在末地石地表之上。
- 潜影贝数量（若 8.1 实现）：逐构件核对固定数量。
- finisher 冒烟：在 cEndGen 上跑 8x8 区块，断言产出非空且全部方块属于允许集合。

（本环境无 vanilla oracle：以「wiki 规格 + 单测」为准，实机对照后续补。）

---

## 11. 分支拆分建议

体量很大，建议拆：
1. feature/generating-end-city：cEndCityGen 骨架 + 入口/基础层/塔/桥/船几何 + 格点与不变量单测（不含潜影贝、战利品、旗帜/酿造台 NBT）。
2. feature/mobs-shulker：cShulker 实体（独立特性）。
3. feature/generating-end-city-loot：箱子战利品表 + 旗帜图案 + 酿造台 + 物品展示框/鞘翅。

分支 1 先落；2、3 依赖 1（或独立）。合并顺序由维护者定。

---

## 附：来源一览

- [End City](https://minecraft.wiki/w/End_City) — 生成、结构、构件表、掉落概览
- [End City/Structure](https://minecraft.wiki/w/End_City/Structure) 及其 9 个子页 — 逐方块蓝图
- [End Ship](https://minecraft.wiki/w/End_Ship) — 重定向
- 概率明文：每塔每方向 50% 桥、每桥 12.5% 船、每城 <=1 船（End City / End ship / Generation）
- 蓝图几何：以各 /Structure/* 子页的 layered blueprint 为准

凡未有允许来源支持的数值/行为，均已标注 **推测/待确认**。
