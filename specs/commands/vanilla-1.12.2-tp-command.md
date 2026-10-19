# 纯 Lua 命令族（补）：`/tp`（1.12.2 四形态）

分支：`feature/core-tp-command` ｜ 级别：P2 ｜ 前置：C1 选择器、C2 坐标 origin、
[movement 支](vanilla-1.12.2-movement-commands.md) 的坐标校验与旋转解析

**为什么单独成支**：1.12.2 的 `/tp` 与 `/teleport` 是**两条不同命令**——前者 target 可省略、
有「传送到另一个玩家」形态、`~` 按**被传送目标**解析；后者 target 必填、无该形态、`~` 按**命令执行位置**
解析。两者直到 **1.13（17w45a）** 才合并。Core 现有 `/tp` 只覆盖四形态中的两个（且缺旋转），
补齐它既不能并进 `/teleport`（会提前引入 1.13 行为），也不涉及新 C++ 通路。

## 1. 现状

| 1.12.2 形态 | Core 现状 | 判定 |
|---|---|---|
| `tp <destination player>` | ✅ 有（`/tp <player>`，`-h` 为 Core 扩展） | 保留 |
| `tp <x> <y> <z> [<yaw> <pitch>]` | ⚠️ 只有 `tp <x> <y> <z>`，**缺 `[<yaw> <pitch>]`** | 补齐 |
| `tp <target player> <destination player>` | ❌ 3 个 token 被当成 `tp <玩家> -h` | 补齐 |
| `tp <target player> <x> <y> <z> [<yaw> <pitch>]` | ❌ 落到 Usage | 补齐 |

## 2. 行为规格

来源（均于 2026-10-18 经 minecraft.wiki 的 MediaWiki API 抓取原文）：

- [Commands/tp/Before Java Edition 1.13](https://minecraft.wiki/w/Commands/tp/Before_Java_Edition_1.13)
  —— 1.13 之前的 `/tp` 专属文档（保留页）；
- [Commands/teleport rev 1139121](https://minecraft.wiki/w/Commands/teleport?oldid=1139121)
  （2017-08-18，1.12.2 期）—— 两者的差异说明与 `/teleport` 语法；
- 本仓库 [command-coordinates §2.1b](vanilla-1.12.2-command-coordinates.md) —— `~` 基准的既有结论。

### 2.1 语法

```
tp <destination player>
tp <x> <y> <z> [<yaw> <pitch>]
tp <target player> <destination player>
tp <target player> <x> <y> <z> [<yaw> <pitch>]
```

原文（Commands/tp/Before Java Edition 1.13 §Syntax）即上表四种；History 记 1.3.1（12w25a）起可传送到
任意坐标、1.8（14w03a）起有旋转参数。

### 2.2 参数

1. `<target player>`：「Specifies the entity(s) to be teleported. Must be a player name, a target selector,
   or a UUID.」+「**Permits entities other than players.**」→ 可命中多个实体，含非玩家。
2. `<destination player>`：「Specifies the entity to teleport the target(s) to. Must be a player name,
   a target selector, or a UUID.」+「**Permits entity other than players.**」；
   Result 要求它「resolve to a **single** entity」。
3. `<x> <y> <z>`：「`x` and `z` must fall within the range -30,000,000 to 30,000,000 (**exclusive**) …
   `y` must be within the range -4096 to 4096, **inclusive**.」
4. `<yaw>` / `<pitch>`：可选且**成对**出现；「Tilde notation can be used to specify **a rotation relative
   to the target's previous rotation**.」yaw：-180.0 正北、-90.0 正东、0.0 正南、90.0 正西；pitch：-90.0
   正上 … 90.0 正下。
5. 无参、缺坐标、只给一个旋转值、多余参数 → 参数错误。

### 2.3 相对坐标与相对旋转的基准（与 `/teleport` 的核心差异）

- 坐标：原文「May use tilde … to specify a position relative to **the target's current position** – an
  exception to tilde notation's usual rule of being relative to the executor of the command.
  `/teleport` can be used to teleport relative to executor of the command.」
  → 每个被传送实体**各自**解析 `~`（多目标时基准不同）。
- 旋转：`~` 相对**该目标当前旋转**。
- 对照 `/teleport`（rev 1139121 首段）：「`/teleport` uses coordinates relative to the command's execution,
  while `/tp` uses coordinates relative to the teleported targets.」

### 2.4 形态歧义（第一个参数是目标还是坐标）

四种形态由「第一个参数是 `<target player>` 还是 `<x>`」区分。本支的判定规则：

- 第 2 个 token 是**坐标**（绝对数字，或以 `~` 开头）→ 自传形态（形态 2）；
- 否则 → target 形态（形态 3 / 4）；只有 1 个参数时按形态 1 处理。

依据：玩家名只含 `[A-Za-z0-9_]` 且不能以数字开头，选择器以 `@` 开头，因此坐标 token 与实体 token
不相交〔推测：Brigadier 命令树把 `/tp` 的下一层同时挂 `targets`（entity）与 `x`（double）两个节点，
按参数类型择一，置信度高〕。

`^`（局部坐标）在 1.12.2 **不存在**（[command-coordinates §2.3](vanilla-1.12.2-command-coordinates.md)：
1.13 17w50a 才加入），因此以 `^` 开头的 token **不**按坐标解析：它落入 target 名分支，按普通实体名解析
（例如 `tp ^ 1 2 3` 会因找不到实体而失败）；若 token 数也不构成任何形态，则报 Usage（如 `tp ^ ^ ^`）。
（Commands/tp/Before Java Edition 1.13 的 Arguments 里出现 “tilde and caret notation”，是 1.13 之后
补写的措辞，与本仓库已核实的 `^` 版本边界冲突，以后者为准。）

### 2.5 维度

Result 原文：「Fails **individually for each entity** referenced by `<target player>` that is **not in the
same dimension** as the `<destination player>`, if specified.」
→ 该规则针对**同时给了 target 与目的地实体**的形态（形态 3）：不同维度的目标逐个失败，其余目标照常传送。
形态 2、4（坐标形态）不涉及维度切换——在每个目标自己的世界里按坐标传送。
形态 1（执行者 → 目的地实体）没有显式的 `<target player>` 参数，上述子句不覆盖：**保留 Core 既有的跨世界
搬运行为**，其与原版是否一致未核实（见 §4 第 5 条）。

### 2.6 Result

- 「Fails if the arguments are not specified correctly, if `<target player>` fails to resolve to one or
  more entities (named players must be online), or if `<destination player>` fails to resolve to a single
  entity (a named player must be online).」
- 成功时 success count = 实际被传送的目标数。

### 2.7 反馈文案（拟写，未与原版逐字比对）

沿用 Core 既有风格，逐目标一行：

| 形态 | 文案 |
|---|---|
| 1（自传 → 实体） | `You teleported to <destination>`（沿用现有） |
| 2（自传 → 坐标） | `You teleported to [X:<x> Y:<y> Z:<z>]`（沿用现有） |
| 3（目标 → 实体） | `Teleported <target> to <destination>` |
| 4（目标 → 坐标） | `Teleported <target> to <x>, <y>, <z>`（与 `/teleport` 一致） |

失败：`Player "<name>" not found`（名字形态，沿用选择器层）、`No entity was found`（选择器命中 0 个）、
`The destination must be a single entity`（目的地命中多个）、
`<target> is not in the same dimension as <destination>`（§2.5）、坐标/旋转参数错误沿用
`/teleport` 的文案。

### 2.8 1.12.2 之后（不引入）

- 1.13（17w45a）`/tp` 与 `/teleport` 合并为同一命令；`facing`（18w01a）、`^` 局部坐标（17w50a）、
  `execute` 的 `as/at/run` 形态都不在本支范围内。
- `-h` 是 Core 的既有扩展（不通知目的地），**保留**但不写入 vanilla 语法。

## 3. 实现映射

| 规格点 | 落点 |
|---|---|
| 四形态切分、`[<yaw> <pitch>]` 成对、形态歧义（§2.4）、Usage 文案 | 新 `command_tp.lua`：`IsTPCoordinateToken()`、`ParseTPLine()`（纯函数） |
| 坐标范围/旋转解析 | 复用 `command_movement.lua` 的 `IsValidTeleportPosition()`、`ParseRotationOffset()` |
| `~` 按目标解析、旋转按目标解析 | `teleport.lua`：对每个目标用 `ParseCommandCoords(..., {x = GetPosX(), ...})`、`ApplyOffset` |
| 目标/目的地解析 | C1 的 `ResolveTargets(token, MakeSelectorContext(Player, Origin), "entity")` |
| 逐目标执行与跨世界目标 | C1 的 `ApplyToTargets()`（同世界当拍、跨世界延迟一拍） |
| 形态 1 的既有行为（通知目的地 / `-h`） | 保留 `TeleportToPlayer()` |
| 注册 | `Info.lua`：`/tp` 的 HelpString 与 ParameterCombinations 补齐四形态；控制台 `tp` 改绑同一实现（命令方块走控制台命令表，必须与游戏内一致） |

## 4. 已知偏差与未核实项

1. **反馈文案**：§2.7 全部为拟写，未在原版 1.12.2 专用服逐字核对〔待核：原版逐条执行并抄录〕。
2. **跨世界目标**：沿用 C1 的延迟机制。形态 4 的延迟目标在**它自己的世界**里按同一组绝对坐标传送
   （与 `/teleport` 的既有偏差一致）；形态 3 的延迟目标按 §2.5 应失败，本支**不传送、不发逐目标提示**
   （延迟线程不得触碰执行者）〔待核：原版对跨维度目标的提示文本〕。
3. **目的地命中多个实体的提示文本**：拟写〔待核〕。
4. **UUID 字面量**：原文允许 `<target player>` 是 UUID；Core 选择器层只认名字与 `@` 选择器，
   UUID 形式**不支持**（与 C1 一致）〔待核：原版把 UUID 当名字还是实体标识〕。
5. **形态 1 的跨维度**：上述 Result 子句只覆盖形态 3；形态 1（`tp <destination>`）跨维度时本支**保留**
   Core 既有的 `MoveToWorld` 行为（不改变历史行为），与原版是否一致**未核实**〔待核：原版 1.12.2
   用两个不同维度的玩家验证 `tp <player>`〕。
6. **`^`**：1.12.2 无此记号，按普通 token 处理（见 §2.4）。
7. **控制台 `tp`**：改绑到本支的四形态实现（`HandleConsoleTPCommand` → `HandleTPCommand(..., nil, ...)`）。
   理由：Cuberite 的命令方块走**控制台命令表**，若控制台 `tp` 仍是旧实现，命令方块里的
   `/tp @p ~ ~10 ~` 会落到旧逻辑（实测返回 `Player not found`）。旧控制台实现支持的
   `tp <target> <destination>` / `tp <target> <x> <y> <z>` 两种形态仍被覆盖，只有反馈文案变化
   （`You teleported X to [X:..]` → `Teleported X to ..`）与新增的跨维度逐目标失败属于行为变更。
8. **非玩家目标的显示名**：沿用 C1 的 `GetTargetName()`（玩家名 / 生物名 / `?`）〔待核〕。
9. **`tp <destination>` 的「通知目的地」**：vanilla 无此行为（`-h` 是 Core 扩展），默认仍通知，
   与历史行为一致。

## 5. 测试与验收

### 5.1 纯 Lua 测试

`lua tests/test_tp_command.lua`（Core 插件目录）。覆盖：

- `ParseTPLine`：四形态与各自的最小/最大参数；`[<yaw> <pitch>]` 必须成对；
  形态歧义（`tp 100 ~3 100` vs `tp @p 100 64 100`）；`tp Alice`（形态 1）；`tp ^ ^ ^` 不按坐标解析；
  `-h` 兼容；缺参/多余参数 → Usage。
- handler（fake 目标/选择器层）：形态 4 的 `~` 按**每个目标自身**位置解析（两个不同位置的目标得到
  不同落点）；旋转 `~` 按目标自身旋转；形态 2 的 `~` 按执行者；形态 1/3 的目标/目的地解析与
  跨维度拒绝；目的地命中 0 个 / 多个的失败；参数错误时不传送任何目标、success count 0。

### 5.2 静态检查（按 [AGENTS.md §4.1](../../AGENTS.md) 选门）

本支只改 `Server/Plugins/Core/`（Lua）与父仓库 `specs/` + gitlink，不改 `src/`：

| 门 | 结果 |
|---|---|
| 1 风格 | 不适用（无 `.cpp`/`.h`） |
| 2 编译 | 不适用（纯 Lua；服务器二进制未改） |
| 3 ctest | 不适用（同上） |
| 4 绑定依赖 | 不适用 |
| 5 行为规格 | 本文件 + §5.1 + §5.3 |
| 6 APIDump | 不适用（未新增/导出 C++ API） |

- `luac -p`：Core 目录全部 Lua 文件通过。
- `luacheck`（配置取 `build/Server/.luacheckrc`）：新增/修改文件 0 warning。

### 5.3 实机探针（真实服务器 + mineflayer bot）

1. `tp 100 ~3 100`（形态 2）：按执行者位置解析 `~3`。
2. `tp ~ ~ ~ ~10 ~`（形态 2 + 旋转）：yaw 相对执行者 +10。
3. `tp @p ~ ~ ~ ~10 ~`（形态 4 + 旋转，wiki 示例）：目标=最近玩家，旋转按目标。
4. `tp @a @s`（形态 3，wiki 示例）：所有玩家传到执行者。
5. 函数文件 `tp @s ~ ~5 ~` + `setblock ~ ~-1 ~ emerald_block`（第 6 轮 anchor 用例）：
   bot 被抬高 5 格、原脚下变绿宝石块。
6. 跨维度：第二个维度的目标用 `tp @a[world?]`（本仓库选择器跨世界）验证逐目标失败或按 §4.2 处理〔待核〕。

记录命令、聊天栏原文与服务端 Lua 读回值作为证据。

### 5.4 实机证据（2026-10-19，build/Server + mineflayer bot `TestBot`）

| 用例 | 观察 |
|---|---|
| `tp`（无参） | `Usage: /tp <destination> \| <x> <y> <z> [<yaw> <pitch>] \| <target> <destination> \| <target> <x> <y> <z> [<yaw> <pitch>]` |
| `tp 100 ~3 100`（形态 2） | `You teleported to [X:100 Y:103 Z:100]`（`~3` 按执行者 y=100 解析） |
| `tp ~ ~ ~ ~10 ~`（形态 2 + 旋转） | 服务端 Lua 读回 yaw 100 → **110**（相对执行者自身） |
| `tp @e[type=pig] ~ ~5 ~`（形态 4，多目标） | 每只猪按**自己**的 y +5：`229.8→229.8,69`、`228.6→228.6,69`、`20→20,69`、`139.5,66→139.5,71` |
| `tp @e[type=pig] ~ ~ ~ ~10 ~`（形态 4 + 旋转） | 每只猪 yaw 各自 +10：`0→10`、`-34→-24`、`-45.9→-35.9`、`37.9→47.9` |
| `tp @e[type=pig] @s`（形态 3） | 全部猪被传到执行者位置 (100,64,-20)，聊天栏 `Teleported pig to TestBot` |
| `/function test:anchor`（`tp @s ~ ~5 ~` + `setblock ~ ~-1 ~ emerald_block`） | 执行前 (0,100,0) → 执行后 **(0,105,0)**，(0,99,0) 由 stone 变 emerald_block（133）——即 1.12.2 的 `tp` 目标形态与 origin 锚定同时成立 |
| 命令方块 `tp @p ~ ~10 ~`（无玩家 sender） | Previous Output `Teleported TestBot to 0, 110, 0`（`~10` 按目标自身 y=100 解析） |
| 控制台 `tp TestBot 0 90 0` | `Teleported TestBot to 0, 90, 0` |
| 控制台 `tp TestBot`（形态 1，无执行者） | `No player was found to teleport` |

修复前对照（同一文档第 6 轮复测）：`tp @s ~ ~5 ~` 只返回 Usage，函数照报 `Executed 2 command(s)` 而 bot 不动。
