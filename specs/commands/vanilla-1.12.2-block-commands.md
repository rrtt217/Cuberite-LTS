# 纯 Lua 命令族（五）：`/setblock`、`/fill`、`/clone`、`/testforblocks`

分支：`feature/core-block-commands` ｜ 级别：P2 ｜ 前置：C2（坐标 origin）｜
编排见 [roadmap §4.2](vanilla-1.12.2-command-layer-roadmap.md)

**为什么归为一支**：四条命令共用同一套方块参数解析（`<block> [dataValue]`、方块状态与 SNBT 的拒绝
理由）、同一套区域几何（两角点归一化、体积上限、已加载区块判定），且都受同一组限制——**Lua 拿不到
NBT**、**没有方块状态注册表**。单列四支会让同一套解析与限制被评审四遍。

## 1. 现状

| 命令 | Core 现状 | 所需 C++ API | 判定 |
|---|---|---|---|
| `/setblock` | 无 | `cWorld:SetBlock`（ManualBindings）、`GetBlockTypeMeta`、`DropBlockAsPickups`、`ResolveBlockType()`（C4 已在 [command_execute.lua:336](../../Server/Plugins/Core/command_execute.lua#L336) 导出） | **API 齐，缺命令** |
| `/fill` | 无 | 同上 + `cBlockArea`（可选，本支用逐方块 API，见 §3） | **API 齐，缺命令** |
| `/clone` | 无 | `cBlockArea:Read/Write`（ManualBindings，[ManualBindings_BlockArea.cpp:559](../../src/Bindings/ManualBindings_BlockArea.cpp#L559)、[:740](../../src/Bindings/ManualBindings_BlockArea.cpp#L740)） | **API 齐，缺命令** |
| `/testforblocks` | 无 | `cBlockArea:Read` + `GetRelBlockType/Meta` | **API 齐，缺命令** |

## 2. 行为规格

来源（四个 1.12.2 期修订，全部早于首个 1.13 快照 17w43a）：
[/setblock rev 1135361](https://minecraft.wiki/w/Commands/setblock?oldid=1135361)、
[/fill rev 1128681](https://minecraft.wiki/w/Commands/fill?oldid=1128681)、
[/clone rev 1122916](https://minecraft.wiki/w/Commands/clone?oldid=1122916)、
[/testforblocks rev 1123086](https://minecraft.wiki/w/Commands/testforblocks?oldid=1123086)。

### 2.1 共用：方块参数

1. 1.12.2 的方块参数是**方块名**（例 `minecraft:stone` 或 `stone`），数据值是**单独的可选参数**
   `[dataValue|state]`，范围 0–15；状态写法（`stone[variant=1]`）是 1.13+ 的替换语法。
   → Core 只收名字 + 数字数据值；带 `[`/`]` 的 token 拒绝（理由：没有方块状态注册表），
   数字 ID 拒绝（1.8 起命令不再接受数字 ID〔待核：本页只写「block id (for example minecraft:stone)」，
   数字 ID 的移除时间取自仓库 C4 的既有结论，未在本页复核〕）。
2. 可选的 `[dataTag]`（SNBT）需要 NBT 通路，Lua 侧完全没有 → 以 `{` 开头的参数**明确拒绝**
   （「Data tags are not supported yet」），不静默忽略。
3. 坐标接受 `~` 相对记号，基准是命令 origin（C2）；方块坐标向下取整；`y` 必须在 0–255。

### 2.2 `/setblock <x> <y> <z> <block> [dataValue] [oldBlockHandling] [dataTag]`

1. `oldBlockHandling`：`destroy` | `keep` | `replace`，缺省 `replace`。
2. `destroy`：旧方块**连同内容**按被玩家破坏处理掉落（原文「drops both itself and its contents
   (as if destroyed by a player)」）；`keep`：只有空气会被替换；`replace`：旧方块不掉落、无声音。
3. Result：「Fails if the arguments are not specified correctly, **if tried to change a non-air block in
   keep mode, tried to replace a block with an identical copy in keep or replace mode**, or if the
   coordinates are in an unloaded chunk.」
4. 权限等级 2〔待核：同前几支，infobox 不可抓〕。

### 2.3 `/fill <x1> <y1> <z1> <x2> <y2> <z2> <block> [dataValue] [oldBlockHandling] [dataTag]`

1. 两角点定义闭区间；体积 = `(x大−x小+1)×(y大−y小+1)×(z大−z小+1)`，**上限 32768**。
2. `oldBlockHandling`：`destroy` | `hollow` | `keep` | `outline` | `replace`，缺省 `replace`。
   - `destroy`：区域内所有方块（包括与新方块相同的）掉落实体；
   - `hollow`：只把**外壳**换成新方块，内部变空气（内容掉实体、方块本身不掉）；任一维 < 3 时退化为 `replace`；
   - `keep`：只替换空气；
   - `outline`：只换外壳，内部不动；任一维 < 3 时退化为 `replace`；
   - `replace`：全部替换；可再带 `[replaceTileName] [replaceDataValue]` 只替换匹配的方块
     （仅当 handling 显式为 `replace` 时才有这两个参数）。
3. Result：「Fails if … the block volume … is greater than 32768, if dataValue or dataTag are invalid …
   or **if no blocks were changed**」。
4. 权限等级 2〔待核〕。

### 2.4 `/clone <x1> <y1> <z1> <x2> <y2> <z2> <x> <y> <z> [maskMode] [cloneMode] [TileName] [dataValue]`

1. 源区域由两角点给出，目标区域以 `<x> <y> <z>` 为**最小角**；源体积上限 32768。
2. `maskMode`：`replace`（缺省）| `masked`（只复制非空气，目标空气格保持）|
   `filtered`（只复制 `TileName` 指定的方块；`TileName` 必填，可带 `dataValue` 过滤数据值）。
3. `cloneMode`：`normal`（缺省）| `force`（源与目标重叠时强制）| `move`（复制后把源区域清空；
   `filtered` 时只清掉被复制的方块）。
4. Result：「Fails if … the block volume of the source region is greater than 32768 …,
   **if the source and destination region overlap (unless using force clone mode)**,
   or if one or both regions isn't currently rendered.」
5. 方块实体（箱子内容、告示牌文字等）：本支在 `replace` 模式下用 `cBlockArea` 连方块实体一起复制；
   `masked`/`filtered` 走逐方块 API，**不复制方块实体内容**（记录偏差）。
6. 权限等级 2〔待核〕。

### 2.5 `/testforblocks <x1> <y1> <z1> <x2> <y2> <z2> <x> <y> <z> [mode]`

1. 源区域体积上限 **524288**；`mode`：`all`（缺省，逐方块完全一致）| `masked`（源区域的空气匹配任何方块）。
2. Result：「Fails if the arguments are not specified correctly, or if the source and destination regions
   do not match. On success, returns the number of matching blocks (**the total number of blocks in `all`
   mode, or the number of source region non-air blocks in `masked` mode**).」
3. 权限等级 2〔待核〕。

### 2.6 已加载区块

三条 Result 都要求区域「rendered / loaded」。Cuberite 的 `GetBlockTypeMeta` 在区块未加载时返回
`BlockValid = false`，`cBlockArea:Read` 同样失败 → 本支据此判定，不触发区块生成
（在 tick 线程同步生成会卡服；同一理由记录在 movement 支的规格 §4）。

## 3. 实现映射（Core，纯 Lua）

| 规格点 | 落点 |
|---|---|
| 四条命令的参数切分、模式白名单、`dataTag`/`state` 拒绝、体积与坐标上限 | 新 `command_blocks.lua`：`ParseSetblockLine()`、`ParseFillLine()`、`ParseCloneLine()`、`ParseTestforblocksLine()`、`NormalizeBox()`、`GetVolume()`、`IsValidBlockPosition()` |
| 取/写方块、掉落、已加载判定 | `World:GetBlockTypeMeta(Vector3i)`、`World:SetBlock(Vector3i, type, meta)`、`World:DropBlockAsPickups(Vector3i, nil, nil)` |
| 方块名 → type | C4 已导出的 `ResolveBlockType()`（[command_execute.lua:336](../../Server/Plugins/Core/command_execute.lua#L336)），不重复实现 |
| `/clone` 的带方块实体复制 | `cBlockArea:Read(World, p1, p2, baTypes+baMetas+baBlockEntities)` + `:Write(World, min)` |
| 命令注册 | `Info.lua` 四条 in-game + 四条控制台 |

## 4. 已知偏差与未核实项

1. **`[dataTag]` 与 `<state>` 全部拒绝**：Lua 拿不到 NBT、也没有方块状态注册表；错误信息明确说明，
   不静默忽略（与 `/summon` 的处理一致）。
2. **方块实体的内容**：`/fill`、`/setblock` 的 `dataTag` 不支持；`/clone` 只在 `replace` 模式整体复制
   方块实体，`masked`/`filtered` 不复制内容；`destroy`/`hollow` 的「内容掉落实体」也没有实现
   （清掉方块实体时不会先倒出箱内物品）——**物品丢失**，已记录。
3. **性能**：`/fill` 与 `/testforblocks` 用逐方块 Lua 循环（上限 32768 / 524288）。极大区域
   （尤其 `testforblocks` 的 524288）会明显慢于原版，可能拖慢 tick〔待核：实测 524288 体积的耗时〕。
4. **反馈文案**：`Block placed` / `Successfully filled N blocks` / `Successfully cloned N blocks` /
   `The blocks match` 等为拟写，未与原版逐字比对〔待核：原版专用服逐条抄录〕。
5. **方块名解析沿用 C4**：`ResolveBlockType()` 走 `StringToItem` + `IsValidBlock`，不区分大小写；
   原版对大小写与命名空间前缀的处理未逐条复核〔待核〕。
6. **`/fill` 的 `destroy` 掉落工具**：`DropBlockAsPickups()` 的 `a_Digger`/`a_Tool` 传 nil（空手），
   与原版「as if mined with an unenchanted diamond shovel or pickaxe」不同（如树叶、藤蔓的掉落差异）
   〔待核：原版各材质方块的掉落表〕。
7. **`/clone` 的 `move` + 方块实体**：用 `cBlockArea` 写入后把源区域设为空气，源区域的方块实体被移除，
   但内容未掉落——见第 2 条。
8. **`/testforblocks` 的返回值**：成功计数按规格（`all` = 体积，`masked` = 源非空气数），
   但失败时的计数（0）与原版是否一致未核实〔待核〕。

## 5. 测试与验收

### 5.1 纯 Lua 测试

`lua tests/test_block_commands.lua` → **69 passed, 0 failed**。覆盖：

- 四个解析器：缺参/多余参数、数据值 0–15、模式白名单、未知模式、`{` 开头的 SNBT 拒绝、
  **数字方块 ID 拒绝**、`[state]` 拒绝、`/fill` 的 replace 过滤语法（必须显式 dataValue + 显式
  `replace`）、`/clone` 的 filtered 必须给方块名、`/testforblocks` 的 all/masked。
- 几何：角点归一化、体积、盒相交、方块坐标范围（y 0–255、x/z ±30,000,000）。
- handler（fake 世界 + fake `cBlockArea`）：
  - `/setblock` 放置、相同方块拒绝、keep 非空气拒绝、destroy 掉落旧方块；
  - `/fill` 4 格填充与计数、无变化失败、32768 上限、3×3×3 hollow 清空内部并保留外壳；
  - `/clone` 复制与计数、重叠拒绝、filtered 只复制匹配方块；
  - `/testforblocks` all 计数 = 体积、mismatch 失败、masked 只用源非空气块计数。

### 5.2 静态检查（按 [AGENTS.md §4.1](../../AGENTS.md) 选门）

| 门 | 结果 |
|---|---|
| 1 风格 | 不适用（无 `src/*.cpp|h` 改动） |
| 2 编译 | 不适用（纯 Lua） |
| 3 ctest | 不适用（同上） |
| 4 绑定依赖 | 不适用 |
| 5 行为规格 | 本文件 + §5.1/§5.3 |
| 6 APIDump | 不适用 |

- `luac -p`：新增/改动文件通过。
- `luacheck`：`command_blocks.lua`、`cmd_setblock.lua`、`cmd_fill.lua`、`cmd_clone.lua`、
  `cmd_testforblocks.lua` **0 warning / 0 error**。
- `Info.lua`：新增 8 个 handler 名（4 in-game + 4 console），按既有 W113 同型增长。

### 5.3 真实服务器探针

本机没有客户端，空载服务器**不加载出生点区块**（`GetBlockTypeMeta` 返回 `BlockValid = false`），
因此用临时探针插件（`build/probe-block-run`，不入库）先 `cWorld:ChunkStay()` 载入测试区块，
再用 `cRoot:Get():GetPluginManager():ExecuteCommandFromOrigin()` 逐条执行命令。日志原文：

```
PROBE [setblock 0 100 0 stone] => Block placed (count 1)
PROBE [testforblock 0 100 0 stone] => The block at 0, 100, 0 is "stone" (count 1)
PROBE [setblock 0 100 0 stone] => That block is already the same (count 0)
PROBE [setblock 0 100 0 dirt 0 keep] => That block is not air (count 0)
PROBE [fill 0 100 0 2 100 2 stone] => Successfully filled 8 blocks (count 8)
PROBE [fill 0 100 0 2 100 2 stone] => No blocks were changed (count 0)
PROBE [clone 0 100 0 2 100 2 5 100 5] => Successfully cloned 9 blocks (count 9)
PROBE [testforblocks 0 100 0 2 100 2 5 100 5] => The blocks match (count 9)
PROBE [setblock 1 100 1 dirt] => Block placed (count 1)
PROBE [testforblocks 0 100 0 2 100 2 5 100 5] => The blocks do not match (count 0)
PROBE [clone 5 100 5 7 100 7 10 100 10] => Successfully cloned 9 blocks (count 9)
PROBE [testforblocks 5 100 5 7 100 7 10 100 10] => The blocks match (count 9)
PROBE [clone 0 100 0 1 100 1 0 100 0] => The source and destination regions overlap (count 0)
PROBE [fill 0 100 0 100 200 100 stone] => The region has more than 32768 blocks (count 0)
PROBE [setblock 0 100 0 stone 0 replace {CustomName:x}] => Data tags are not supported yet (count 0)
PROBE [fill 20 100 20 22 102 22 stone 0 hollow] => Successfully filled 26 blocks (count 26)
PROBE [setblock 30 100 30 stone 0 destroy] => Block placed (count 1)
PROBE [testforblocks 0 100 0 2 100 2 5 100 5 masked] => The blocks do not match (count 0)
PROBE [setblock 0 100 0 1] => Invalid block ID "1": 1.12.2 takes block names, not numeric IDs (count 0)
PROBE [setblock 0 100 0 stone[variant=1]] => Block states are not supported yet (count 0)
PROBE [fill 0 100 0 2 100 2 1] => Invalid block ID "1": 1.12.2 takes block names, not numeric IDs (count 0)
PROBE [clone 0 100 0 2 100 2 5 100 5 filtered normal 1] => Invalid block ID "1": ... (count 0)
PROBE [fill 0 100 0 2 100 2 stone] => Successfully filled 1 blocks (count 1)
PROBE [testforblocks 0 100 0 2 100 2 5 100 5] => The blocks match (count 9)
```

（最后几条里 `stone` 的填充只改了 1 格，因为其余 8 格在前一轮探针里已经是石头；这正好验证了
「无变化不算 changed」。）

### 5.4 未覆盖

- **空载服务器的区块前提**：§5.3 用临时插件强制载入区块；真实玩家在场时区块自然加载。没有玩家时
  `/setblock` 等会报「unloaded chunk」——这与原版（出生点区块常驻）不同，属 Cuberite 的既有世界
  加载策略，不在本支修改〔待核：原版空载服务器对 `/setblock` 远处区块的行为〕。
- **方块实体的端到端**：`/clone` 的 `replace` 走 `cBlockArea` 带方块实体，但探针只用普通方块，
  未验证箱子内容/告示牌文字的复制〔待核：摆箱子放物品、clone 后开箱核对〕。
- **性能**：未测 32768/524288 体积的耗时（§4 第 3 条）。
- **玩家路径与权限闸门**：无客户端〔待核〕。
