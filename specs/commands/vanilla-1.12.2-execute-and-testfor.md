# C4 `/execute`、`/testfor`、`/testforblock`（1.12.2 形态）

分支：`feature/core-execute-commands` ｜ 级别：P1 ｜ 前置：C1（选择器）、C2（执行上下文）、C3（成功计数）｜ 编排见 [roadmap](vanilla-1.12.2-command-layer-roadmap.md)

三条命令合并为一支：它们共享**同一套依赖**（选择器解析、原点覆盖、成功计数回传），且**同一批触点**（Core 新增 3 个 `cmd_*.lua` + `Info.lua` 注册 + C++ 侧一条「带原点执行命令」的通路）。单独立项每条都不足 200 行。

## 1. 现状

- Core **完全没有**这三条命令：`Server/Plugins/Core/` 无 `cmd_execute.lua` / `cmd_testfor*.lua`（目录清单实测），[Info.lua](../../Server/Plugins/Core/Info.lua) 亦无对应条目。
- 上游 [issue #4888](https://github.com/cuberite/cuberite/issues/4888) **未**跟踪这三条 → 属本 fork 自行发现的缺口（AGENTS §1：缺口由维护者逐条下达，本文仅为候选规格）。
- **前置已落地**：C1（选择器层 [target_selectors.lua](../../Server/Plugins/Core/target_selectors.lua)）、C2（`cCommandOrigin` + 引号切分 + handler 第 4 参）与 C3（成功计数回传、命令方块）均已在本检出 `master` 上。
- 已有可复用的「以他者身份执行」通路：Core `/do` 用 `cPluginManager:ExecuteCommand(a_Player, newSplit)`（[do.lua:13](../../Server/Plugins/Core/do.lua#L13)，C++ 侧 [PluginManager.h:362](../../src/Bindings/PluginManager.h#L362)）→ 能换身份，**不能换原点**，且 handler 拿不到成功计数。这正是本支 C++ 通路要解决的。
- 成功计数通路已存在：Lua handler 的第三个返回值经 [ManualBindings.cpp:80-84](../../src/Bindings/ManualBindings.cpp#L80) 写入 `cCommandOutputCallback::CommandExecuted(int)`，命令方块存为 `SuccessCount`（C3）。

## 2. 行为规格（1.12.2）

来源为本轮**已取回并逐条核对**的 1.12.2 时点修订（均为 wiki 的 `oldid` 固定修订，可复核）：

- [Commands/execute, rev 1130584（2017-07-26）](https://minecraft.wiki/w/Commands/execute?oldid=1130584)（1.12.2 前最后一版；与 2023 年拆分出的 [Commands/execute (old)](https://minecraft.wiki/w/Commands/execute_(old)) 现行页逐句一致）
- [Commands/testfor, rev 1167488（2017-11-18）](https://minecraft.wiki/w/Commands/testfor?oldid=1167488)
- [Commands/testforblock, rev 1166649（2017-11-12）](https://minecraft.wiki/w/Commands/testforblock?oldid=1166649)
- [Commands, rev 1152284（2017-09-29）](https://minecraft.wiki/w/Commands?oldid=1152284)（命令表：权限级别、History 版本边界）
- [Commands/setblock, rev 1135361（2017-08-08）](https://minecraft.wiki/w/Commands/setblock?oldid=1135361)（`block` 参数的写法）

### 2.1 `/execute`

1. **语法（JE）**：`execute <entity> <x> <y> <z> <command …>`；带检测的备用语法 `execute <entity> <x> <y> <z> detect <x2> <y2> <z2> <block> <dataValue|state> <command …>`。
2. **权限**：命令方块/命令表均记 **op（级别 2）**。
3. **`entity`**：「Must be a player name or target selector. **If more than one entity is selected, the command will be run once as each of them.**」→ 多目标 = 逐个执行（不是只取第一个）。
4. **`x y z`**：「Coordinates specified with tilde notation are **relative to the target**, not to the position of the command's execution.」→ `~` 以**各目标自身位置**为基准。
5. **`x2 y2 z2`**：「May use tilde notation to specify distances **relative to (x,y,z)**」→ detect 的坐标以第 4 条的解析结果为基准。
6. **`block`**：「Must be a valid block id (for example, `minecraft:stone`)」；`dataValue`「Must be a valid block data for that type of block or `-1` to match any block data.」；`state`（JE，1.11 起）「Must be a valid block state for that type of block or `*` to match any block state.」
7. **成功/失败判定（原文）**：「Fails if arguments are not specified correctly, if entity fails to resolve to one or more valid entities (named players must be online), if the checked block is not of the correct block id, data or state, or if the specified command fails. On success, **executes the specified command as if executed by the specified target(s)** with operator-level permission at the specified coordinate.」
8. **成功计数回传**：1.8（14w08a）「Commands run using `/execute` now pass their success value back to the command block running them.」（History，见 rev 1152284）
9. **版本边界**：1.13（17w45a）起 `/execute` 被整体重做（`as/at/run/if/unless`）→ 本 fork **不**实现 1.13+ 子命令（[AGENTS.md §9](../../AGENTS.md)）。

### 2.2 `/testfor`

1. **语法（JE）**：`testfor <player> [dataTag]`；权限 op（级别 2）。
2. **`player`**：「Must be a player name or target selector（**`@e` is permitted to target entities other than players**）」。
3. **`dataTag`**（可选，1.8 起）：必须是 NBT compound（如 `{XpLevel:3}`）。
4. **结果（原文）**：「Fails if the arguments are not specified correctly, or if player fails to resolve to one or more entities (named players must be online). On success, **produces a success count equal to the number of targets matching player**, which can be measured by a redstone comparator facing away from the command block.」

### 2.3 `/testforblock`

1. **语法（JE）**：`testforblock <x> <y> <z> <block> [dataValue|state] [dataTag]`；权限 op（级别 2）。
2. **`x y z`**：「May use tilde notation to specify a position relative to the command's execution.」
3. **`block`**：同 §2.1 第 6 条。**`dataValue`**：「Must be between **-1 and 15** (inclusive). If not specified, or if `-1`, dataValue matches any block data value.」
4. **`dataTag`**（可选，JE）：NBT compound；缺省匹配任意 data tag。
5. **结果（原文）**：「Fails if the arguments are not specified correctly, if the block at the specified position does not match the specified block id or data, or if the specified data tag has different values than are defined in the block's data tag … On success, **returns a success count of 1**.」

### 2.4 `block` 参数只接受**名字**，不接受数字 ID（本轮新增定案）

[Commands, rev 1152284](https://minecraft.wiki/w/Commands?oldid=1152284) 的 History 条目：**1.8（14w03a）「Numerical IDs no longer work in other parts of commands, though they still work for NBT data.」**；[setblock, rev 1135361](https://minecraft.wiki/w/Commands/setblock?oldid=1135361) 的 `block` 参数写作「Must be a block id (for example, `minecraft:stone` **or `stone`**)」。

→ 1.12.2 的方块参数 = **名字**（可带 `minecraft:` 前缀，也可不带），**数字 ID 必须被拒绝**（这与 C1 §2 的「1.12.2 用字符串 ID」一致；Cuberite 的 `BlockStringToType()` 两者都收，故**不能**直接用它）。

## 3. 本分支范围与设计决策

**做**：

1. `cmd_execute.lua`：解析 `<entity> <x> <y> <z> [detect …] <command…>`；命令体取**原始命令行**的剩余串（见 §3.4）。
2. `cmd_testfor.lua`：`/testfor <target>` → 命中数量作为成功计数（**这是命令方块计数能被观测的主要途径**）。
3. `cmd_testforblock.lua`：`/testforblock <x> <y> <z> <block> [<dataValue>]`。
4. 三者都必须走 C1 的选择器模块与 C2 的 origin 通路；**禁止**在 Lua 里自己再造一份选择器解析。
5. `Info.lua` 注册（`Commands` + `ConsoleCommands` 两张表）+ 权限节点 `core.execute` / `core.testfor` / `core.testforblock` + 帮助文案。
6. C++ 侧新增一条「**按给定原点执行命令**」的通路（`cPluginManager` 新导出方法 + 手写绑定，见 §3.3）。

**不做**：`/testforblocks`（体积比较，独立一支）、`/blockdata`、`/entitydata`、`/function`、1.13+ 子命令、`^` 局部坐标（1.13/17w50a 才引入）、选择器的 `tag`/`score_*`/`rx|ry`（属 C1 增量）。

### 3.1 三个身份的分工（本轮定案）

| 角色 | 取自 | 依据 |
|---|---|---|
| **权限主体** | **`/execute` 的发起者**（玩家；控制台/命令方块无主体 → 不查权限，但见 §3.2） | 页面首段「with **originating** permissions」+ C4 §5 的反提权要求（见下） |
| **命令的 sender / `@s` 目标** | **被选中的目标**（仅当目标是玩家） | §2.1 第 7 条「as if executed by the specified target(s)」 |
| **原点（世界 + 坐标）** | 目标所在世界 + 解析后的 `<x y z>` | §2.1 第 4/7 条 |

**为何权限取发起者而不是目标**：§2.1 第 7 条另写「with operator-level permission」，与首段的「originating permissions」互相矛盾（两处同页）。Cuberite 没有数字权限级别，只有按命令的权限节点；若按**目标**（尤其是 `@p` 选中的 op 玩家）的权限授权，则「发起者挑命令、目标出权限」等于让低权限者借用 op 的权限（`/execute <op> ~ ~ ~ stop`），是提权漏洞。取发起者的权限集是**取两者的交集下界**：既不会超出任何一方，也满足 C4 §5「必须用查权限的那条」。此取舍是**有意偏差**，记入 §5。

**非玩家目标**：C2 的 `cCommandOrigin` 只携带 `cPlayer *`（[CommandOrigin.h](../../src/CommandOrigin.h)），故非玩家目标（生物）**没有 sender、`@s` 也解析不到它** → 内层命令里含 `@s` 会按「无目标」失败。这是**已知偏差**（§5），修法需要给 `cCommandOrigin` 增加非玩家执行者（属 C2 追加，不在本支私补）。

### 3.2 命令方块的管理员命令拦截必须继续有效（本轮定案）

C3 §2.9 的 `stop/restart/kick/ban/ipban` 白名单拦截在**命令方块自身**的执行路径上（[CommandBlockEntity.cpp:440-447](../../src/BlockEntities/CommandBlockEntity.cpp#L440)）。命令方块以控制台身份进入 Core，内层命令若走「无权限主体」分支就会**绕过**该拦截 → `/execute @p ~ ~ ~ kick Admin` 成为提权。

判据用本轮实测确认的原点约定：`cServer` 给控制台/RCON 的兜底原点是 **default world + (0,0,0)**（[Server.cpp:482-486](../../src/Server.cpp#L482)），而命令方块的原点是**方块中心** `(x+0.5, y, z+0.5)`（[CommandBlockEntity.cpp:458-462](../../src/BlockEntities/CommandBlockEntity.cpp#L458)）→ 整数 x 使 `x+0.5` 永不为 0，故「**无玩家主体且原点不是 (0,0,0)**」⇔「某个方块发起的」，是**精确**判据（不是启发式；C1 的 `GetSelectorDeferral()` 已用同一约定区分控制台）。命中时按与 C++ 相同的那 5 个命令拒绝，并把该策略写进测试。

### 3.3 C++ 通路：`cPluginManager:ExecuteCommandFromOrigin`

签名（手写绑定，返回三值给 Lua）：

```
cPluginManager:ExecuteCommandFromOrigin(Origin, PermissionPlayer, Command) -> Result, Output, SuccessCount
```

- `Origin` 是 Lua 侧用 C1 的坐标解析结果构造的 `cCommandOrigin(World, Position, Sender)`（构造器已由 tolua 导出）。
- 语义：切分用与引擎同一把切分器 `StringSplitCommandLine()`；**目标是玩家**时走 in-game 路径（hook → 按 `PermissionPlayer` 查权限 → `handler->ExecuteCommand(Split, Sender, …, &Output, &SuccessCount)`）；**无 sender** 时走控制台路径（仍按 `PermissionPlayer` 查权限；命令必须在 `m_ConsoleCommands` 里）；两条路径都取原点世界的 `cWorld::cLock`（递归锁，调用方已持有时无影响），与 [Server.cpp:605-613](../../src/Server.cpp#L605) 的控制台路径保持**同一条 world → plugin 锁序**。
- `Result` 是 `cPluginManager::CommandResult`（Lua 侧用已导出的 `cPluginManager.crExecuted` 等常量比较；**不能**用真值判断——`crExecuted == 0`，而 Lua 里 0 为真）。

### 3.4 命令体取原文切片

C2 的切分器会剥掉引号、压缩不了空白（但会丢引号），所以命令体**必须**从 handler 收到的 `a_EntireCommand`（引擎切分用的**同一个字符串**，[PluginManager.cpp:1333](../../src/Bindings/PluginManager.cpp#L1333)、[Server.cpp:613](../../src/Server.cpp#L613) 传递）按**字节偏移**切片，不能用 `table.concat(Split, " ", N)` 重组（`/execute @p ~ ~ ~ tell "First Last" hi` 会被拼坏）。偏移由与 `StringSplitCommandLine()` 逐条对齐的纯 Lua 分词器给出（只以空格分隔、连续空格产生空 token、引号段内空格不断词、`\` 转义下一字符、未闭合引号取到行尾——[StringUtils.cpp:129](../../src/StringUtils.cpp#L129) + [CommandLineSplitTest.cpp](../../tests/StringUtils/CommandLineSplitTest.cpp) 的不变量），并配表驱动用例。

### 3.5 成功计数

- `/testfor`：成功计数 = 命中数（§2.2 第 4 条原文）；未命中 → 0 + 失败消息。
- `/testforblock`：成功 = 1（§2.3 第 5 条原文）；不匹配 → 0 + 失败消息。
- `/execute`：**各目标内层命令成功计数之和**；只要有一条内层命令被成功派发即算成功，全部失败（或目标为空、或 detect 全不通过）算失败。多目标时的确切定义 wiki 未写，**〔推测，置信度中〕**：依据是「the success value of an executed command is now passed back」＋「run once as each of them」，取和是唯一不丢信息的聚合；已在 §5 记录。

## 4. 测试与验收（本分支实际执行）

- **纯逻辑抽头（Lua 表驱动）**：`Server/Plugins/Core/command_execute.lua` 的 `SplitRawCommand()`（与 C++ 切分器逐条对齐的分词 + 字节偏移，含引号/转义/连续空格/空 token 用例）与 `ParseExecuteLine()` / `ParseTestforblockLine()` / `ParseBlockAndData()`（命令体起点、detect 参数个数、方块名字与 `-1`、拒绝数字 ID 与 `name:damage`）都是不碰游戏对象的纯函数 → `tests/test_execute_line.lua`（95 例，全绿）。
- **Core 行为（引擎打桩）**：`tests/test_execute_command.lua`（50 例，全绿）加载真实的 `command_coords.lua` / `command_execute.lua` / `cmd_execute.lua`，把选择器层与 `ExecuteCommandFromOrigin()` 换成记录式桩，逐条核对**无实机就验不了的那部分接线**：权限主体是发起者而非目标、玩家目标是内层 sender、`~` 跟随目标位置、命令方块拦截在任何内层调用之前生效、detect 逐目标放行、成功计数求和、各错误路径的文案。
- **fuzz**：`tests/FuzzCommands.lua` 的 `choices` 增列 `"~"`、`"~-1"`、`"detect"`、`"-1"`、`"stone"`，`maxLen` 提到 `6`（`/execute` 的最小可用形态）。**本机未能执行**：该场景由外部 `CuberitePluginChecker` + AutoAPI/ManualAPI 驱动（[Core 的 ci.yml](../../Server/Plugins/Core/.github/workflows/ci.yml)），本机既无 checker、`github.com` 也不可达 → 〔待核〕，复现命令见该 yml 的 `Run tests` 步骤。
- **六道门**（改了 `src/`，全适用）：
  1. 风格 `cd src && lua CheckBasicStyle.lua` → 0 violations；
  2. 编译 `cmake --build build` → 通过；
  3. `ctest -E "UrlClient-test|Google-test"` → 49/49 通过；
  4. 绑定依赖 `cd src/Bindings && lua CheckBindingsDependencies.lua` → 通过（新增方法未改变依赖关系）；
  5. 行为 → §6；
  6. APIDump 自检：本分支新增的两个导出符号（`cPluginManager:ExecuteCommandFromOrigin`、`cCommandOrigin` 的 2 参构造器）已写入 [Classes/Plugins.lua](../../Server/Plugins/APIDump/Classes/Plugins.lua)，自检产出的 `NewlyUndocumented.lua` **不含它们**；该文件仍列出 6 个**与本分支无关的既有**符号（`cEnderCrystal.HasRespawnBeam` 等，C2 交付说明已记录同一清单）→ 门 6 依然不是全绿，但与本支无关。
- Core 侧：`luac -p` 全部通过；四个新 Lua 文件与两个新测试文件 `luacheck` 干净（0 warning / 0 error）。`Info.lua` 无法做到干净（基线已有 210 条同型告警：行尾空格与「handler 名未定义」），本支新增 12 条同型告警、无新类型；新增的 6 行行尾空格与文件既有 74 行同款式。
- **行为证据**（本机无实机 oracle，见 [AGENTS.local.md §1](../../AGENTS.local.md)）：在 `build/Server` 起真实服务器，用控制台与一个**临时探针插件**（`build/` 下、不入库）跑 `ExecuteCommandFromOrigin()` 并记录返回值，见 §6。实机（真客户端 + `/execute @p ~ ~ ~ detect …`）与玩家发起路径**未验**，见 §5。

## 5. 已知偏差 / 风险

1. **权限语义**（§3.1）：vanilla 的「operator-level permission」在 Cuberite 无对应物；本支按**发起者**的权限节点授权（更保守，不引入提权），并且这是 `/execute` 唯一的权限闸门。vanilla 对级别 3/4 命令（`/stop`、`/op` 等）在 `/execute` 下的确切行为**〔待核〕**：需 1.12.2 实机或 1.12.2 时点文档；本轮 wiki 两处说法互相矛盾。
2. **玩家发起路径未实机验证**〔待核〕：本机无法构造 `cPlayer`（`cPlayer` 需要 `cClientHandle`，测试与服务器探针都拿不到在线玩家），所以「玩家发起 → 按发起者权限查 → 目标是玩家时以目标为 sender」这一段只有 §4 的打桩测试（断言传给引擎的三个参数正确）与 C++ 侧代码复核支撑。**核实方法**：在 1.12.2 客户端上，用一个「有 `core.execute`、无 `core.give`」的权限组执行 `/execute @p ~ ~ ~ give @s stone` → 期望拒绝且不给物品；再用同时拥有两者的组执行 → 期望目标收到物品（证明 sender 是目标而非发起者）。
3. **非玩家目标没有 sender / `@s`**（§3.1）：`/execute @e[type=zombie] ~ ~ ~ kill @s` 在内层解析不到目标（`cCommandOrigin` 只携带 `cPlayer`）。修法：给 `cCommandOrigin` 增加非玩家执行者实体（C2 追加），本支不私补。
4. **管理员命令拦截**（§3.2）：无玩家主体且原点不是 (0,0,0)（或没有位置）时，`stop/restart/kick/ban/ipban` 一律拒绝；控制台与 RCON 的原点是 (0,0,0)，不受影响。命令方块一侧与 C++ 现状一致；这是**有意的安全策略**，非 vanilla 行为（与 C3 §2.9 同类）。
5. **未实现的可选参数**（都在 §3「不做」内，遇到时给出明确的「不支持」失败而不是静默忽略）：`/testfor` 与 `/testforblock` 的 `dataTag`（NBT 匹配需要 SNBT 解析层）、`[state]` 方块状态形式（Cuberite 用 type+meta，无方块属性注册表）、`/execute` 的 `state` 形式；`block` 参数按 §2.4 只收名字，数字 ID 与 Cuberite 自有的 `name:damage` 写法都被明确拒绝。
6. **`@e` 仍只覆盖玩家与怪物**（C1 §3 的既有偏差）：`/testfor @e[type=item]` 找不到掉落物，`/execute` 的 `<entity>` 也只能选中玩家与怪物。
7. **跨世界目标不执行**（对 `/execute` 与 `/testfor` 都成立）：非位置型选择器（如 `@a`、`@e[type=cow]`）在其它世界的命中被**忽略**——选择上下文置 `LocalOnly`，使解析只覆盖原点世界。原因：其它世界由 C1 的 D2 机制排队到各自线程执行，那里既不能触碰发起者（权限主体，[C1 §7.3](vanilla-1.12.2-command-target-selectors.md)）也收不到内层命令的文本，更无法同步回传计数；不置 `LocalOnly` 会让「无玩家主体」分支绕过权限检查（提权）。vanilla 跨维度生效 → **已知偏差**，修法需要「发起者权限快照 + 计数回传」两个通道，列为后续任务。
8. **多目标时 `/execute` 成功计数取和**（§3.5）：vanilla 未写明，**〔推测，置信度中〕**。
9. **文案非 vanilla 原文**：三条命令的成功/失败消息都是本仓库自拟（wiki 不记载 1.12.2 的原文），例如 `Unknown command`、`You do not have permission to execute that command`、`The block detection failed`、`Found N matching targets`。响应文案本身不影响计数与副作用。
10. **管理员命令拦截依赖原点位置约定**（§3.2）：若日后有新的 origin 构造方给出 `(0,0,0)` 原点却期望「方块」语义，会静默落到「控制台」分支 → 拦截失效。缓解：约定写进注释与本文件，**没有位置的原点按「方块」处理**（fail-closed）；`cCommandOrigin` 若日后携带 sender 类型，应改为按类型判定（建议的后续任务）。

## 6. 实现与运行期证据（本分支）

**触点**：

- `src/CommandOrigin.h`：新增 2 参构造器 `cCommandOrigin(World, Position)`（无 sender；tolua 已导出，Lua 侧因此能构造非玩家目标的原点）。
- `src/Bindings/PluginManager.h` / `.cpp`：新增 `ExecuteCommandFromOrigin(Origin, PermissionPlayer, Command, Output, SuccessCount)`；`HandleCommand()` 拆成「原签名包装 + 带 origin/权限主体/输出/计数的实现」，玩家路径行为不变（仍传 `a_Output = nullptr`）。有玩家 sender 时走 in-game 表（hook → 查权限 → handler）；无 sender 时走控制台表（仍查发起者权限，且要求命令在 in-game 表里存在）。两条路径都在**原点世界的 `cWorld::cLock`** 内派发，与 `cServer::ExecuteConsoleCommand()` 同一条 world → plugin 锁序；cServer 自己的内建命令（`stop`/`restart` 等，`m_Plugin == nullptr`）按「未知命令」返回，不落到 `crError`。
- `src/Bindings/ManualBindings.cpp`：手写绑定 `cPluginManager:ExecuteCommandFromOrigin(Origin, PermissionPlayer, Command) -> Result, Output, SuccessCount`（`CheckParamSelf("cPluginManager")`；第 2 参可为 nil）。
- `Server/Plugins/Core/`：`command_execute.lua`（纯解析）、`cmd_execute.lua`、`cmd_testfor.lua`、`cmd_testforblock.lua`、`Info.lua`（`Commands` + `ConsoleCommands`，权限 `core.execute` / `core.testfor` / `core.testforblock`）、`tests/test_execute_line.lua`、`tests/test_execute_command.lua`、`tests/FuzzCommands.lua`。

**运行期观察**（控制台 + 临时探针插件；探针在 `build/` 下，未入库；世界数据来自本检出既有的 `build/Server/world`）：

| 命令（探针以 `Origin = (default world, (0,64,0))`、无权限主体调用） | result | 成功计数 | 输出文本 |
|---|---|---|---|
| `testfor @e` | 0（crExecuted） | **12** | `Found 12 matching targets` |
| `testfor @a`（无人在线） | 0 | 0 | `No target matched the selector "@a"` |
| `testforblock 0 64 0 air` | 0 | 0 | `The chunk of the block at 0, 64, 0 is not loaded` |
| `testforblock 73 58 -214 stone` | 0 | 0 | `The block at 73, 58, -214 is not "stone"` |
| `say probe-message` | 0 | 1 | （广播，无文本） |
| `nosuchcommand` | 1（crUnknownCommand） | 1 | — |
| `stop` | 1（crUnknownCommand） | 1 | — |
| `kick Nobody` | 0 | 1 | `Player "Nobody" not found` |
| `execute @e[type=cow] ~ ~ ~ say nested` | 0 | 1 | `Successfully executed the command on 1 target(s)` |
| `execute @e[type=cow] ~ ~ ~ detect ~ ~-1 ~ stone -1 say nested` | 0 | 0 | `The block detection failed` |
| `execute @e[type=cow] ~ ~ ~ testforblock ~ ~-1 ~ stone -1` | 0 | 0 | `The block at 73, 59, -214 is not "stone"` |

最后一行同时验证了三件事：内层命令确实以**目标（牛）的位置**为原点（`~ ~-1 ~` 解析到 `73, 59, -214`，而不是控制台的 `0, 0, 0`）、`/execute` 的命令体被原样交给引擎、以及内层命令的**失败**不会让 `/execute` 自己崩掉（`result = 0`，计数 0）。`testfor @e` 的 12 与 `testfor @e[type=cow]` 的 1 说明成功计数就是 1.12.2 定义里的「命中数量」，也就是比较器能从命令方块读到的值。

**交付时的仓库坐标**：父仓库分支 `feature/core-execute-commands`；Core 分支 `feature/core-execute-commands`（fork `rrtt217/Core`，commit `797047e`），父仓库 gitlink 指向同一提交。
