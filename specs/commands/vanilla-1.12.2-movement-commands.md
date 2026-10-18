# 纯 Lua 命令族（四）：`/teleport`、`/spreadplayers`

分支：`feature/core-movement-commands` ｜ 级别：P2 ｜ 前置：C1（选择器）、C2（坐标 origin）｜
编排见 [roadmap §4.2](vanilla-1.12.2-command-layer-roadmap.md)

**为什么归为一支**：两条命令都只做「把已有实体挪到某处」，都消费 C1 的选择器与 C2 的坐标解析，
且都不改方块、不写存档、不需要新 C++ 通路；规模都是 S–M。区别只在目标位置：`/teleport` 是给定坐标，
`/spreadplayers` 是给定区域内随机找地表。

## 1. 现状

| 命令 | Core 现状 | 所需 C++ API | 判定 |
|---|---|---|---|
| `/teleport` | 无；只有 `/tp`（[teleport.lua](../../Server/Plugins/Core/teleport.lua)），且 `/tp` 的 `~` 按**被传送实体**解析（1.12.2 的 `/tp` 语义），`/teleport` 的 `~` 按**命令执行位置**解析 | `cEntity:TeleportToCoords`、`SetYaw`、`SetPitch`、`GetYaw`、`GetPitch`（均见 [Bindings/docs/cEntity.lua:1977](../../src/Bindings/docs/cEntity.lua#L1977) 起） | **API 齐，缺命令** |
| `/spreadplayers` | 无 | `cWorld:TryGetHeight`（ManualBindings，[ManualBindings_World.cpp:1849](../../src/Bindings/ManualBindings_World.cpp#L1849) 注册 `GetHeight`；`TryGetHeight` 同段）、`cWorld:GetBlock`、`cPlayer:GetTeam`（[docs/cPlayer.lua:599](../../src/Bindings/docs/cPlayer.lua#L599)）、`cScoreboard:ForEachTeam`（ManualBindings） | **API 齐，缺命令**；落点算法无允许来源文档，见 §2.2 第 6 条 |

## 2. 行为规格

### 2.1 `/teleport <target entity> <x> <y> <z> [<y-rot> <x-rot>]`

来源：[Commands/teleport 的 1.12.2 期修订](https://minecraft.wiki/w/Commands/teleport?oldid=1139121)
（rev 1139121，2017-08-18；1.13 于 2018-07-18 发布）。

1. 语法（Java Edition 行）：`teleport <target entity> <x> <y> <z> [<y-rot> <x-rot>]`。
   **1.12.2 没有** 1.13 的 `facing` 参数；`<destination: target>` 形态**只有 PE**，不实现。
2. `target entity`：玩家名或目标选择器，**可命中多个实体**；玩家名必须在线（「named players must be online」）。
3. 坐标范围：`x`、`z` 在 −30,000,000 … 30,000,000（**不含端点**）；`y` 在 −4096 … 4096（含端点）。
4. 相对坐标：页面明确「`/teleport` uses coordinates relative to **the command's execution**, while `/tp`
   uses coordinates relative to the teleported targets」→ `~` 的基准是命令 origin，**不是**被传送实体。
5. 旋转（可选，两个一起给）：`<y-rot>` 是水平角，`<x-rot>` 是垂直角（顺序：先 y-rot 后 x-rot）；
   页面：「Tilde notation can be used to specify a rotation **relative to the target's previous rotation**」
   → 旋转的 `~` 基准是**每个目标自己**的当前旋转（与坐标的基准不同）。
6. 「`/teleport` can send entities into chunks that have yet to be generated」→ 不要求目标区块已加载。
7. Result：「Fails if the arguments are not specified correctly, or if target entity fails to resolve to
   one or more entities (named players must be online).」
8. 权限等级 **2**〔待核：本页修订 infobox 无可抓取权限字段；核法＝原版专用服非 OP/OP 各一次〕。
9. 1.12.2 之后：1.13 起 `/teleport` 与 `/tp` 合并并加 `facing`；**不引入**。

### 2.2 `/spreadplayers <x> <z> <spreadDistance> <maxRange> <respectTeams> <player …>`

来源：[Commands/spreadplayers 的 1.12.2 期修订](https://minecraft.wiki/w/Commands/spreadplayers?oldid=1123067)
（rev 1123067，2017-07-09）。

1. 语法与参数：
   - `x z`：区域中心；「x and z must be between −30,000,000 and 30,000,000 (exclusive)」；
     「May use tilde notation to specify coordinates relative to the command's execution」；
   - `spreadDistance`：「Must be at least 0.0」；
   - `maxRange`：「Must be at least 1.0 greater than spreadDistance」（区域是以中心为中心的**正方形**）；
   - `respectTeams`：`true` / `false`；「If true, targets on the same team will be teleported to the same
     location」；
   - `player …`：「Must be one or more player names and/or target selectors separated by spaces
     (**`@e` is permitted to target entities other than players**)」。
2. Result：「Fails if the arguments are not specified correctly, **if there are too many targets to satisfy
   the spreadDistance requirement within the specified area**, or if a target is attempted to be spread to
   outside the world boundary.」
3. 落点：「Targets will always be placed **on the top block** at a location, and **never on lava**.」
4. 团队：`respectTeams=true` 时按队伍分组，每组一个落点；无队伍的实体各自成组。
5. 单位/取整：`spreadDistance`、`maxRange` 都是浮点；落点是方块位置（实体放在方块中心）。
6. **落点算法没有允许来源的文档**：页面只描述约束与结果，不描述采样过程。本支采用
   「在正方形内均匀随机采样、检查与已放置点的欧氏 XZ 距离、逐点最多 1000 次尝试」的算法
   〔推测：依据是「随机 + 最小距离」这一规格的唯一自然实现，置信度中；与原版的具体随机序列
   和尝试次数必然不同，属已记录偏差〕。
7. 地表高度：Cuberite 的 `TryGetHeight()` 返回**最高非空气方块**的 Y（[Chunk.cpp:1228](../../src/Chunk.cpp#L1228)，
   高度图由 [Chunk.cpp:1399-1414](../../src/Chunk.cpp#L1399) 维护）→ 站立 Y = 高度 + 1；
   要求该方块不是岩浆、且脚部与头部两格可站立〔推测：原版「top block / never on lava」的实现细节〕。
8. 权限等级 **2**〔待核，同上〕。

## 3. 实现映射（Core，纯 Lua）

| 规格点 | 落点 |
|---|---|
| `/teleport` 参数切分、坐标/旋转 token 校验、坐标范围 | 新 `command_movement.lua`：`ParseTeleportLine()`、`IsValidTeleportPosition()`、`ParseRotation()` |
| `/spreadplayers` 参数切分、距离/范围/布尔校验 | 同文件：`ParseSpreadplayersLine()` |
| 随机落点（可注入随机源与地表查询，便于测试） | 同文件：`PickSpreadPositions()` |
| `/teleport` 目标解析与执行 | 新 `cmd_teleport.lua`：`ResolveTargets(..., "entity")` + `ApplyToTargets`；坐标用 `ParseCommandCoords(..., GetOriginPosition(Origin))`，旋转逐目标解析 |
| `/spreadplayers` 目标解析、团队分组、地表查询、传送 | 新 `cmd_spreadplayers.lua`：逐 token `ResolveTargets` 合并去重；`cPlayer:GetTeam()` 作分组键；`TryGetHeight` + `GetBlock` 求地表 |
| 命令注册 | `Info.lua`：in-game `/teleport`、`/spreadplayers`；控制台 `spreadplayers`。控制台 `teleport` **保留** Core 既有语义（见 §4 第 4 条） |

## 4. 已知偏差与未核实项

1. **反馈文案**：`Teleported X to ...` / `Spread N players around ...` 为拟写，未与原版逐字比对
   〔待核：原版 1.12.2 专用服逐条执行并抄录〕。多目标时 `/teleport` 每目标一行。
2. **`/spreadplayers` 的落点算法**：见 §2.2 第 6 条〔推测〕；另有三条实现约束：
   - **只使用已加载区块**：`TryGetHeight()` 在区块未加载时返回 nil，本支把它当作「该候选点无效」
     并重试；重试耗尽即失败。原版会生成区块（本支不在 tick 线程里同步生成，避免卡服）——
     已记录偏差；
   - **世界边界**：Cuberite 没有世界边界状态，§2.2 第 2 条的「outside the world boundary」无法判定，
     只按区块加载与高度检查处理；
   - **团队**：`cPlayer:GetTeam()` 只对玩家有意义；非玩家实体一律各自成组。
3. **`/teleport` 的跨世界目标**：沿用 C1 的延迟机制——其他世界的目标在该世界线程里被传送到
   **同一组绝对坐标**（与原版「`setPositionAndUpdate` 只改当前世界坐标」一致）；本支的反馈只覆盖
   原点世界当拍命中的目标（[C1 §7](vanilla-1.12.2-command-target-selectors.md)）。
4. **控制台 `teleport`**：Core 此前只有控制台 `tp`（历史语义），没有 `teleport`；本支把控制台
   `teleport` 注册为 vanilla 语义，`tp` 保持历史语义不动。
5. **`/tp` 不动**：1.12.2 的 `/tp` 是另一条命令（`~` 按被传送实体解析），本支不合并、不改写；
   两者的差异见 [command-coordinates §2.1b](vanilla-1.12.2-command-coordinates.md)。
6. **旋转范围**：`SetYaw`/`SetPitch` 会 `WrapRotation()`（[Entity.cpp:2185](../../src/Entities/Entity.cpp#L2185)），
   本支不做额外范围校验；原版对 `<x-rot>` 是否在命令层就拒绝越界值未核实〔待核〕。
7. **`/teleport` 的 y 范围**：规格写 −4096…4096；Cuberite 的世界高度是 0–255，超出部分由
   `TeleportToCoords` 自行处理，本支只按规格做参数校验〔待核：原版把实体传到 y=300 的实际效果〕。
8. **反馈里的实体名**：本支沿用 C1 的 `GetTargetName()`——玩家用名字、生物用
   `cMonster:MobTypeToString()`（小写内部名，如 `pig`）、**非生物实体显示 `?`**（C1 没有它们的显示名）。
   原版用实体显示名（Item / Experience Orb 等）〔待核：原版对这些实体的反馈原文〕。

## 5. 测试与验收

### 5.1 纯 Lua 测试

`lua tests/test_movement_commands.lua` → **54 passed, 0 failed**。覆盖：

- `/teleport` 参数：缺参/缺坐标/多余参数/单个旋转值被拒；最小形态与旋转对解析。
- 目标位置校验：±30,000,000 开区间（29,999,999 接受、30,000,000 拒绝）、y ±4096 接受、4097 拒绝、NaN 拒绝。
- 旋转偏移：`~`、`~5`、`~-5.5`、绝对 `90`、非法值。
- `/spreadplayers` 参数：完整形态、两个目标 token、缺目标、负 `spreadDistance`、
  `maxRange < spreadDistance + 1`、非布尔 `respectTeams`、相对中心。
- 采样器（注入固定随机序列与地表查询）：单点落位、两点满足 `spreadDistance`、
  跳过不能站立的列、全部列不可用时失败。
- 两个 handler（fake 目标/世界）：坐标按 **origin** 解析、旋转按**目标**解析、
  空目标集失败且 success count 0；`/spreadplayers` 把目标放在地表上方一格（y = 高度 + 1）并给出反馈。

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
- `luacheck`：`command_movement.lua`、`cmd_teleport.lua`、`cmd_spreadplayers.lua` **0 warning / 0 error**
  （`Vector3i` 走配置里的引擎全局；`GetBlock` 用向量重载以免触发已弃用告警）。
- `Info.lua`：新增 5 个 handler 名（in-game 2 + console 3），按既有 W113 同型增长。

### 5.3 真实服务器探针（控制台路径，`build/Server`）

```
Executing console command: "summon pig 0 64 0"
Successfully summoned entity at [X:0 Y:64 Z:0]
Executing console command: "teleport @e[type=pig] 5 64 5"
Teleported pig to 5, 64, 5; Teleported pig to 5, 64, 5
Executing console command: "testfor @e[type=pig,r=2,x=5,y=64,z=5]"
Found 2 matching targets
Executing console command: "spreadplayers 0 0 2 10 false @e[type=pig]"
Spread 2 targets around 0, 0
Executing console command: "testfor @e[type=pig,r=10,x=0,y=64,z=0]"
Found 2 matching targets
Executing console command: "teleport @e 30000000 64 0"
The x and z coordinates must be between -30,000,000 and 30,000,000 (exclusive)
Executing console command: "teleport @e 0 64 0 90"
Usage: teleport <target entity> <x> <y> <z> [<y-rot> <x-rot>]
Executing console command: "teleport @e 0 64 0 90 0"
（每个被传送实体一行 "Teleported ... to 0, 64, 0"）
Executing console command: "teleport @e 0 64 0 90 0 1"
Usage: teleport <target entity> <x> <y> <z> [<y-rot> <x-rot>]
Executing console command: "spreadplayers 0 0 2 2.5 false @e"
The maximum range must be at least 1.0 greater than the spread distance
```

（探针世界里残留了上一轮的猪，因此出现 2 个目标；最后用 `kill @e[type=pig]` 清理。
`teleport` 的两次成功调用各自把当拍命中的实体逐条反馈。）

### 5.4 未覆盖

- **`/teleport` 的旋转在客户端上的效果**：本机无客户端，`SetYaw`/`SetPitch` 只由单元测试的 fake
  目标断言，未在真实客户端观察视角〔待核：真实客户端执行 `teleport @s ~ ~ ~ 90 0`〕。
- **`/spreadplayers` 的均匀性与大范围**：只在出生点附近的已加载区块内验证；跨未加载区块的大范围
  散布会因 §4 第 2 条失败或落点偏少〔待核：真实服务器在 500 格范围上执行，统计落点〕。
- **超多目标的失败路径**（「too many targets」）未构造；采样器的失败分支由单元测试覆盖。
- **玩家路径与权限闸门**：无客户端〔待核〕。
