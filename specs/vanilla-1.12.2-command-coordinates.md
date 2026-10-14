# Vanilla 1.12.2 命令坐标与执行位置（command origin）

> 范围：本规格只覆盖 **1.12.2** 的「坐标参数如何解析」与「执行位置（command origin）由谁提供、
> 选择器和坐标命令怎么用它」。落地目标是让 Core 插件按 vanilla 语义解析 `~`，并把选择器与
> 坐标命令接到引擎已有的 `cCommandOrigin`（[src/CommandOrigin.h](../src/CommandOrigin.h)）上。
>
> **明确不在范围内**：局部坐标 `^`。它是 **1.13（17w50a）** 才加入的（§2.3），按
> [AGENTS.md](../AGENTS.md) §9「不引入 1.13+ 行为」，1.12.2 parity 下必须把它当**解析错误**。

## 1. 结论速览

| 记号 | 1.12.2 是否支持 | 语义 |
|---|---|---|
| `x y z`（绝对） | ✅ | 世界坐标 |
| `~Δ`（相对） | ✅ | 沿世界轴相对**执行位置**偏移；单独 `~` = 偏移 0；可与绝对坐标逐轴混用 |
| `^Δ`（局部） | ❌ | 1.12.2 没有这个概念，解析失败 |

## 2. 允许来源事实

### 2.1 相对世界坐标

[Minecraft Wiki: Coordinates](https://minecraft.wiki/w/Coordinates)（2026-10-13 抓取）原文：

> "A number following a tilde (~) describes an offset from execution position along one of the world
> axes, and a lone tilde assumes an offset of 0."
> "`~ ~ ~` means the command's current position."
> "Relative world coordinates can mix with absolute coordinates; for example, `/tp ~ 64 ~` keeps the
> sender's X and Z positions unchanged but teleports them to an absolute height of 64 blocks."

→ 解析规则：`~` = 执行位置 + 0；`~N` = 执行位置 + N；逐轴独立，可与绝对坐标混用。
**注意 `/tp` 是例外，见 §2.1b。**

### 2.1b `/tp` 的例外：相对坐标基于**被传送的实体**（1.13 之前）

[命令/tp/Java版17w45a前](https://zh.minecraft.wiki/w/%E5%91%BD%E4%BB%A4/tp/Java%E7%89%8817w45a%E5%89%8D)
（2026-10-13 抓取）原文：

> "此命令与 `teleport` 之间的主要差别是：`tp` 使用基于**传送目标当前位置**的相对坐标，而 `teleport`
> 使用基于**命令执行位置**的相对坐标。"
> "可以使用波浪号（`~`）来指定基于**传送目标当前位置**的相对坐标——这是一种特殊情况，大多数情况下，
> 波浪号使用基于命令执行位置的相对坐标。如果需要基于命令执行位置的传送，可以使用 `teleport` 命令。"

同页 History：1.10（16w21b）曾把 `tp` 改为"基于命令执行者"，**1.10 pre1 又恢复**为"基于被传送实体"；
1.13（17w45a）`tp` 与 `teleport` 合并。

→ **1.12.2 的 `/tp` 必须按"被传送实体"的位置解析 `~`**（in-game 的 `/tp <x> <y> <z>` 传送的是执行者，
所以基准就是执行玩家；console 的 `tp <目标> <x> <y> <z>` 基准是**目标玩家**）。
`/summon`、`/spawnpoint` 等其它命令仍按 §2.1 的通用规则（命令执行位置）。

### 2.1c 选择器的维度/世界范围（跨世界是 vanilla 行为）

同一份 1.12.2 时代原文（[Commands?oldid=1152284](https://minecraft.wiki/w/Commands?oldid=1152284)）：

> `@e` — "all entities (including players)"；`@a` — "all players"，**都没有维度限定**；
> `[r=R]` — "Using a value under 0 **restricts the targets to the same dimension the command was run from**
> without applying a restriction on distance from the location of execution."

→ **裸 `@e`/`@a`/`@r`（不带位置/距离参数）在 1.12.2 就是跨维度的**；只有带位置/距离语义时才被钉在执行维度。
因此本仓库**不能**靠"把选择器限制在源世界"来规避跨世界访问——那会破坏 parity。
（`r<0` 这条"限定在本维度"的语义也属于 parity 范围，见 C1 规格。）

### 2.2 「位置」的精确含义

同页：

> "An entity's coordinates are actually the coordinates of the center at the bottom of entity's
> collision box."
> "The position of a block is actually the coordinates of the point at the lower northwest corner of
> the block, that is, the integer coordinates obtained by rounding down the coordinates inside the
> block."

→ 实体的执行位置 = 碰撞箱**底面中心**（玩家的 `GetPosX/Y/Z` 即此值）；方块位置 = 方块西北下角（整数）。

### 2.3 `^`（局部坐标）是 1.13 才出现的

同页 History 表：

> | Java Edition 1.13 | 17w50a | Local coordinates can now be specified in commands using `^`. |

→ **1.12.2 不存在 `^`**。本仓库必须把它当解析错误处理，**不得实现**（`^` 的 +X 左 / +Y 上 /
+Z 前方、"不能与世界坐标混用" 等规则同样只属于 1.13+）。

### 2.4 命令方块的执行位置

[Minecraft Wiki: Command Block](https://minecraft.wiki/w/Command_Block)（同日抓取）给出了命令方块的
NBT 字段（`SuccessCount` int 0…2147483647、`LastOutput` 含时间戳、`LastExecution`、`UpdateLastExecution`）
与「chain 命令方块同一 tick 内执行」（Java 1.12 / 17w16a），但**没有给出命令方块作为命令源时的执行位置数值**。

本仓库现状（[CommandBlockEntity.cpp](../src/BlockEntities/CommandBlockEntity.cpp)，C3 引入）：

修正前（C3 的**推测**值，错）与修正后：

```cpp
// 修正前：Vector3d(m_Pos.x + 0.5, m_Pos.y + 1, m_Pos.z + 0.5)
const cCommandOrigin Origin(
    GetWorld(),
    Vector3d(m_Pos.x + 0.5, m_Pos.y, m_Pos.z + 0.5),
    nullptr
);
```

**已核实（维护者在真实 1.12.2 上实测，2026-10-13）**：
- 命令方块执行 `setblock ~ ~ ~ air` → **命令方块自己消失**；
- 命令方块执行 `summon`/相对坐标 → 落在 `(x + 0.5, y, z + 0.5)`，即 **x / z 取方块中心，y 不变**。

→ 执行位置 = `m_Pos + (0.5, 0, 0.5)`。本仓库原先的 `y + 1` **是错的**，已在本分支修正为
`Vector3d(m_Pos.x + 0.5, m_Pos.y, m_Pos.z + 0.5)`（[CommandBlockEntity.cpp](../src/BlockEntities/CommandBlockEntity.cpp)）。

### 2.5 控制台 / RCON 的执行位置

**1.12.2 时代的 wiki 出处**：[Commands#Target selectors（2017-10 修订 revid 1152284）](https://minecraft.wiki/w/Commands?oldid=1152284)
里 `@p` 词条原文："When run by the console, the origin of selection will be at (0, 0, 0)."

**已核实（维护者在真实 1.12.2 控制台实测，2026-10-13）**：
- `spawnpoint Harry00025 ~ ~ ~` → `Set Harry00025's spawn point to (0, 0, 0)`；
- 同向印证：`summon zombie ~ ~ ~` → `Cannot summon the object out of the world`、
  `setblock ~ ~ ~ air` → `Cannot place block outside of the world`。

→ 控制台执行位置 = **(0, 0, 0)**（`/spawnpoint` 按**命令执行位置**解析 `~`，同时印证 §2.1 的通用规则）。
本仓库引擎侧（[src/Server.cpp](../src/Server.cpp)）本来就给空 origin 补默认世界、位置保持 `(0, 0, 0)`
→ **与 vanilla 一致，无需修改**。

## 3. 开工前的现状

| 位置 | 现状 | 问题 |
|---|---|---|
| [InfoReg.lua](../Server/Plugins/InfoReg.lua) | 绑定 `info.Handler`；多命令包成 `function(a_Split, a_Player, a_EntireCommand)`，内部调用只传 3 个参数 | 引擎把 origin 作为**第 4 个 Lua 参数**传进来（[ManualBindings.cpp:85-88](../src/Bindings/ManualBindings.cpp#L85-L88)），但**多命令路径把它丢了**，Core 也无一处声明该参数 |
| [target_selectors.lua](../Server/Plugins/Core/target_selectors.lua) `MakeSelectorContext` | `a_Player == nil` 时返回 `nil`；`GetSelectorOrigin` 于是回退到**默认世界 + 世界出生点**，注释自称 "an approximation until the command origin lands with C2 [推测]" | C2 已落地，占位没换：命令方块/控制台的选择器世界与位置都是错的 |
| [teleport.lua](../Server/Plugins/Core/teleport.lua) | `loadstring(arg:gsub("~", "return " .. a_Player:GetPosX() .. "+0"))` | ①`a_Player` 为 nil（命令方块/控制台）时**直接抛 Lua 错误**（该表达式与有无 `~` 无关）；②机制是 `loadstring`。注意它的**基准（执行玩家）本身是对的** —— 1.12.2 的 `/tp` 是例外，基准就是被传送实体（§2.1b）；`console.lua` 那条的基准则必须是**目标玩家** |
| [console.lua](../Server/Plugins/Core/console.lua) | `Player:TeleportToCoords(Split[3], Split[4], Split[5])` 直接传字符串 | 纯绝对坐标，连 `tonumber` 都没有，`~` 会当字符串传给 C++ |
| [cmd_summon.lua](../Server/Plugins/Core/cmd_summon.lua) / [cmd_spawnpoint.lua](../Server/Plugins/Core/cmd_spawnpoint.lua) | `RelativeCommandCoord(token, base)`，base 是玩家或目标玩家位置 | 基准不是 origin；无玩家时退化成「相对 0」 |
| [core_functions.lua](../Server/Plugins/Core/core_functions.lua) `RelativeCommandCoord` | 只认 `~`，`^` 会被 `tonumber` 判为非法 | 保留但**不再被调用**（改为指向新解析器），避免破坏仍在调用它的第三方 Lua |

## 4. 实现

### 4.1 引擎 → Lua：origin 透传
- `InfoReg.lua`：`MultiCommandHandler` 与其所有递归调用、以及多命令/控制台包装闭包都增加第 4 个参数
  `a_Origin`，并把它透传给 `a_CmdInfo.Handler` / `Subcommand.Handler`。
- 兼容性：既有处理器只声明 3 个形参时，Lua 会忽略多余实参 → **旧插件/旧命令不受影响**。

### 4.2 新模块 `Server/Plugins/Core/command_coords.lua`
Core 目录下的 `*.lua` 由引擎自动加载（[PluginLua.cpp:110-150](../src/Bindings/PluginLua.cpp#L110-L150)，
`Info.lua` 最后），因此新文件无需注册。

```lua
GetOriginPosition(a_Origin)          -- → {x=,y=,z=} 或 nil（无位置信息）
ParseCommandCoord(a_Token, a_Axis, a_Position)  -- → number 或 nil, reason
ParseCommandCoords(a_X, a_Y, a_Z, a_Position)   -- → x, y, z 或 nil, reason
```
规则（§2.1、§2.3）：
- `~` / `~N`：`a_Position[axis] + (N or 0)`；
- `^...`：**拒绝**，reason 指明 1.12.2 不支持局部坐标；
- 其它：`tonumber`，失败即报错；
- `a_Position == nil`（无位置）：`~` 视为相对 0，并在调用方给出可见的失败信息。

### 4.3 Core 接受 origin
- `MakeSelectorContext(a_Player, a_Origin)`：世界与位置取自 origin（**不再需要玩家**）；
  `Executor` 仍优先取 `a_Player`，否则取 `a_Origin:GetPlayer()`。
  `ResolveTargets(...)` 等既有调用点全部改为传 origin。
- 坐标命令（**基准按命令区分**，见 §2.1 与 §2.1b）：
  - in-game `/tp x y z`：基准 = **执行玩家**（被传送实体），用解析器；**无玩家可传送**时返回失败信息
    （不再抛 Lua 错误）。原 `loadstring` hack 删除。
  - console/命令方块 `tp <目标> x y z`：基准 = **目标玩家**（1.12.2 的 `/tp` 例外）。为此必须**先找到目标**
    再解析坐标，`console.lua` 相应重构。
  - `/summon`：基准 = **命令执行位置**（origin），世界取 `Origin:GetWorld()`（原来固定用默认世界）。
  - `/spawnpoint`：基准 = **命令执行位置**（origin）。旧的"用目标玩家位置"对 console/命令方块是错的
    （vanilla 控制台实测 `spawnpoint <玩家> ~ ~ ~` → `(0, 0, 0)`，即执行位置）。

## 5. 测试

`Server/Plugins/Core/tests/test_command_coords.lua`（纯 Lua，`lua tests/test_command_coords.lua`）：
绝对坐标、`~`、`~N`、`~-N`、逐轴混用、`^` 被拒、非法 token、无位置时的行为、
`MakeSelectorContext` 在「有 origin 无玩家」下给出正确的世界与位置。

## 6. 待核

### 6.1 命令方块的执行位置偏移 —— **已核实并修正**
维护者在真实 1.12.2 上实测：命令方块 `setblock ~ ~ ~ air` → **命令方块自己消失**；
`x/z + 0.5、y 不变`。→ 引擎值已从 `y + 1` 改为 `y`（§2.4）。

附注：本仓库**尚未实现 `/setblock`**（Core 的坐标命令目前只有 `/tp`、`/summon`、`/spawnpoint`、`/regen`），
所以这条判别只能在真实 1.12.2 上做；本仓库侧可用 `summon armor_stand ~ ~ ~` + 读存档 `Pos` 读出自己的值。

### 6.2 控制台 / RCON 的执行位置 —— **已核实**
vanilla 1.12.2 控制台实测 `/spawnpoint Harry00025 ~ ~ ~` → `Set Harry00025's spawn point to (0, 0, 0)`
（`/spawnpoint` 按命令执行位置解析 `~`），故控制台执行位置 = `(0, 0, 0)`。
本仓库引擎本来就给空 origin 补默认世界、位置保持 `(0, 0, 0)` → **与 vanilla 一致，无需修改**。

### 6.3 非法坐标的错误文本
1.12.2 对 `tp ~a ~ ~` 这类输入的确切提示未核实；本实现沿用 Core 既有风格
（`'<token>' is not a valid number`），只保证**行为**（失败、不执行）与来源一致。

## 7. 跨世界选择器的处理（方案 D2）

### 7.1 为什么必须处理

1.12.2 的裸 `@a`/`@e`/`@r` 是**跨维度**的（§2.1c），所以不能把选择器限制在源世界来规避。
而严格 C 让命令方块**在源世界的锁内**执行 Lua，于是出现了实测到的环
（[命令执行线程 spec §7](vanilla-1.12.2-command-execution-threading.md) 与 DeadlockDetect dump）：

```
源世界 W 的线程:  W.chunkmap(已持) → plugin(执行 Lua) → 想要 X.chunkmap   （裸 @e 枚举别的世界）
世界 X 的线程:    X.chunkmap(已持) → 想要 plugin                          （钩子在 world 锁下回调）
```

### 7.2 D2 的设计（本分支的引擎部分已落地）

**规则：Core 只允许锁"本命令正在执行的那个世界"；其它世界一律不取锁。**

- **引擎（已实现）**
  - [Server.cpp](../src/Server.cpp) 的 plugin 分发前取 **origin 世界**的锁 → console / RCON 也变成
    `world → plugin`（与命令方块同形）。`cWorld::cLock` 递归，命令方块路径不受影响。
  - 新增 **[World:SetCommandBlockSuccessCount(BlockX, BlockY, BlockZ, SuccessCount)](../src/Bindings/ManualBindings_World.cpp)**
    （手写绑定 + [World.h](../src/World.h) 声明在 tolua 区域外 + APIDump 文档）：按坐标**无副作用**地写
    命令方块的 `SuccessCount`（不排队方块实体重发），方块不存在就返回 false。
- **Core（待实现，本分支下一步）**
  - 源世界：**实时枚举 + 立即动作**，保持严格 C 的"实际计数 + 同 tick"（最常见情形零变化）。
  - 其它世界：**只排队**——`World:QueueTask(X)` 里在 X 的世界线程上实时解析并执行，把实际数报回；
    全部报回后，再 `World:QueueTask(W)` 到**源世界**把总数写回（两个跳都只锁自己那个世界）。
- **引擎侧"玩家命令对齐本世界锁"暂缓**：`ProcessProtocolIn` 目前不在 world 锁内
  （[World.cpp:1338](../src/World.cpp#L1338) vs `m_ChunkMap.Tick` [:1350](../src/World.cpp#L1350)）。
  在那里取本世界锁可能引入 world↔client 锁的新序，且**不是本次复现的路径**（复现是命令方块）。
  记入 §8。

### 7.3 已知偏差（D2 固有权衡）

"每世界一线程"意味着跨世界部分**不可能**同时做到"值准确"和"执行时刻就正确"；D2 选择值最终准确：

- 执行时刻写入的 `SuccessCount` = **源世界实际数**；其它世界的实际数在它们的队列任务跑完后由回写
  **修正为总数**（比较器上表现为"先一个数、随后跳一次"）；
- `LastOutput` **不**跟随修正（只修计数），因此跨世界命令的消息只反映源世界那部分；
- 只有在 **executor 为 nil 且位置非 (0,0,0)**（即命令方块；控制台是 (0,0,0)）时才登记回写——
  避免误写玩家脚下的方块或控制台的 (0,0,0) 处方块；
- **玩家发出的命令暂不排队**（仍走旧的"实时跨世界"路径）：排队闭包会捕获 executor 引用，跨 tick 使用
  entity userdata 违反 C1 规格的安全约定（[vanilla-1.12.2-command-target-selectors.md](vanilla-1.12.2-command-target-selectors.md) §233）。
  这条留到 §8。

## 8. 本分支未做（记录，等下达）

1. **Core 侧 D2**（源世界实时 / 其它世界排队 + 回报 + 回写）——引擎已经就位，Core 尚未改。
2. **玩家命令的本世界锁对齐**（`ProcessProtocolIn`），含 world↔client 锁序评估。
3. **玩家命令的跨世界排队**：需先给"动作闭包不得捕获 executor 引用"定一个机制（例如把 executor 名字
   传入动作、或让 Core 的动作统一按 EntityID/名字重解析）。
4. 1.12.2 的 `r<0`（"仅执行维度、不限距离"）语义仍是 C1 规格里的待办，与本节相关。
