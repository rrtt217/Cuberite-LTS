# Vanilla 1.12.2 命令层（Command Layer）修复编排

本文件是**编排索引**，不含实现。它把 2026-08 的命令层审计结果拆成 **6 个可独立编译、可独立评审、可独立回滚** 的分支，每个分支对应一份 spec。
按 [AGENTS.md](../../AGENTS.md) §3：**不做自动续跑**，本文件只是「列候选 + 写规格」，未获下达不开工。

- 审计范围：命令分发与切分（`cPluginManager` / `InfoReg.lua`）、目标选择器、命令方块、`/execute` 家族、表现类命令、记分板队伍。
- 判定口径：**缺失**=无代码；**空转**=有字段/入口但无逻辑读取；**偏差**=有逻辑但语法/数值/时机与 1.12.2 不符。
- 编号用 **C 系列**，与状态效果的 S 系列（`vanilla-1.12.2-status-effects-roadmap.md`，写作时**尚未提交**）不冲突；**S16**（Core `/effect`）的选择器部分由本批 C1 接管，见 C1 §5。
- **不确定标注约定**：统一用 [AGENTS.md §2.4](../../AGENTS.md) 的两级标记——**〔待核〕**（原则上可核实、但本次没有核实，须写明怎么核）与 **〔推测〕**（无法用允许来源佐证，须写明依据与置信度）；带标记的条目一律不得当作实现依据。

## 1. 关键事实（写作时全部实测）

1. **不存在 C++ 命令解析框架**：`src/` 内无 `CommandParser.*` / `CommandSystem.*`（`git ls-files | grep -i command` 只有 [CommandOutput.cpp](../../src/CommandOutput.cpp)、[CommandBlockEntity.cpp](../../src/BlockEntities/CommandBlockEntity.cpp) 等）；`grep -rn "Selector" src` 零命中。命令注册/分发全在 [PluginManager.cpp:1256-1258](../../src/Bindings/PluginManager.cpp#L1256)：`StringSplit(a_Command, " ")` → 按首 token 查表 → 把原始 token 数组交给插件（[ManualBindings.cpp:67](../../src/Bindings/ManualBindings.cpp#L67)）。
2. **切分器不支持引号**：命令路径用 [StringSplit](../../src/StringUtils.cpp#L51)（纯分隔符切分）；仓库已有 [StringSplitWithQuotes](../../src/StringUtils.cpp#L72) 但只导出给 Lua（[ManualBindings.cpp:4532](../../src/Bindings/ManualBindings.cpp#L4532)），命令路径未用。
3. **玩家命令的输出被丢弃**：[PluginManager.cpp:1301](../../src/Bindings/PluginManager.cpp#L1301) 调 handler 时 `a_Output` 传 `nullptr`；只有控制台路径（[ManualBindings.cpp:29-45](../../src/Bindings/ManualBindings.cpp#L29) 的 `LuaCommandHandler`）会把输出回传。
4. **插件侧无参数类型系统**：注册仅有 `Permission` / `HelpString` / `Subcommands`（[Info.lua](../../Server/Plugins/Core/Info.lua) + [InfoReg.lua:141](../../Server/Plugins/InfoReg.lua#L141)）→ 目标选择器只能在 handler 内自行解析。
5. **唯一存在的选择器是 Core `/effect` 的硬编码 `@a`**：[cmd_effect.lua:20,28,50,57](../../Server/Plugins/Core/cmd_effect.lua#L50)，仅覆盖在线玩家，无 `[...]` 参数。全仓库（含 submodule）再无 `@p/@r/@e/@s`。
6. **命令方块只有「能跑」**：成功计数恒 0（[CommandBlockEntity.cpp:204](../../src/BlockEntities/CommandBlockEntity.cpp#L204)）、比较器无输出（[CommandBlockHandler.h](../../src/Simulator/IncrementalRedstoneSimulator/CommandBlockHandler.h) 只 `Activate()`）、`TrackOutput` 写死 1（[NBTChunkSerializer.cpp:593](../../src/WorldStorage/NBTChunkSerializer.cpp#L594)）、循环/链方块（ID 210/211，[BlockType.h:229-230](../../src/BlockType.h#L229)）**无区块实体工厂**（[BlockEntity.cpp:90](../../src/BlockEntities/BlockEntity.cpp#L90) 只建 137）→ 放置走到 `default` 分支的 `ASSERT`。
7. **选择器/`/execute` 所需 API 已在**：`cWorld:ForEachEntity`（[World.h:269](../../src/World.h#L269)）、`ForEachEntityInBox`（[:277](../../src/World.h#L277)）、`IsPlayer/IsPawn`（[Entity.h:161-164](../../src/Entities/Entity.h#L161)）、`cMonster.StringToMobType`（[Monster.h:191](../../src/Mobs/Monster.h#L191)）、`SendSetTitle`/`SendTitleTimes`（[ClientHandle.h:222,232](../../src/ClientHandle.h#L222)）、`BroadcastParticleEffect`（[World.h:182](../../src/World.h#L182)）。→ **多数缺口是插件侧 + 少量 src 通路，不需要新架构。**
8. **Cleanroom 裁决（已按现行规范回改）**：Brigadier 是 Mojang 自研、MIT 许可、**1.13/17w45a 才进入 vanilla** 的命令框架（[minecraft.wiki/Brigadier](https://minecraft.wiki/w/Brigadier)）。按现行 [AGENTS.md §2.1](../../AGENTS.md) 的**许可证兼容例外**，brigadier 属「允许参照、借鉴乃至改写入库」的 MIT 来源，**不再**以 cleanroom 为由禁止；本批仍自研，理由是两条独立依据——(a) 1.13+ 语义与 1.12.2 基线冲突（[AGENTS.md §9](../../AGENTS.md)），(b) Lua 侧不存在许可证可用的选择器 port（详见 [C1 §5](vanilla-1.12.2-command-target-selectors.md)）。
9. **聊天层不会吞 `@`**：内部颜色分隔符是 `§`（[ChatColor.cpp:5-6](../../src/ChatColor.cpp#L5)），文本内代码前缀是 `&`（[CompositeChat.cpp:107](../../src/CompositeChat.cpp#L107)），style 串里的 `@` 被忽略 → 回显 `@a` 安全。

## 2. 分支清单与顺序

| # | 分支 | spec | 级别 | 前置 | 改动面 | 一句话 |
|---|---|---|---|---|---|---|
| C1 | `feature/core-target-selectors` | [command-target-selectors](vanilla-1.12.2-command-target-selectors.md) | P0 | — | Core（submodule，`rrtt217/Core`）| 选择器层 `@p @a @r @e @s` + 已核实的 1.12.2 参数集，命令共用 |

**C1 状态**：已实现并推送（Core 侧 `8913d93212cf3fb4d7236437c1b06a83fa08e3f1`，父仓库同步 `.gitmodules` 的 `url` 与 gitlink），待维护者评审。
实现范围与〔待核〕项见 [C1 §4/§7](vanilla-1.12.2-command-target-selectors.md)。
| C2 | `feature/command-context-and-splitting` | [command-context-and-splitting](vanilla-1.12.2-command-context-and-splitting.md) | P0 | — | `src/` | 执行上下文（世界/位置/执行者）+ 引号切分 + 玩家命令输出回传 |

**C2 状态**：已实现，经维护者同意后按 C1 的线性方式（`--ff-only`）合并进本地 `master`（`8af5d03`，**尚未推送**）。
验收：风格 / 编译 / ctest（47/47）/ `CheckBindingsDependencies` 全绿；门 6 因 6 个**与本分支无关的既有**未文档化符号（`cEnderCrystal.HasRespawnBeam` 等，基线 `master` 同样触发）未全绿，见交付说明。
| C3 | `feature/command-blocks-parity` | [command-blocks](vanilla-1.12.2-command-blocks.md) | P1 | C2 | `src/` | 成功计数/比较器/`TrackOutput`/链与循环方块/改名显示/编辑界面下发 |
| C4 | `feature/core-execute-commands` | [execute-and-testfor](vanilla-1.12.2-execute-and-testfor.md) | P1 | C1, C2, C3 | Core + `src/` | `/execute`、`/testfor`、`/testforblock`（1.12.2 形态） |

**C4 状态**：已按 C1 的线性方式（`--ff-only`）合入本地 `master`（父仓库 `738d8f5` 及其收尾提交；Core 侧 `feature/core-execute-commands` / `797047e` 同样已合入 `rrtt217/Core` 的 `master`），两个 `master` 均已推送，父仓库 gitlink 指向 `797047e`。
`src/` 侧新增一条「带原点执行命令」的通路（`cPluginManager:ExecuteCommandFromOrigin`，权限主体与 sender 分离），Core 侧新增 `cmd_execute.lua` / `cmd_testfor.lua` / `cmd_testforblock.lua` / `command_execute.lua` + `Info.lua` 注册。
验收：风格 0 violations / 编译通过 / ctest 49/49 / `CheckBindingsDependencies` 通过 / 行为（真实服务器 + 探针插件，见 [C4 §6](vanilla-1.12.2-execute-and-testfor.md)）；门 6 本支两个新符号已入 APIDump，`NewlyUndocumented.lua` 仍只剩 C2 记录过的同 6 个既有符号。
**待实机核实**（本机无客户端）：玩家发起路径的权限闸门与「目标即 sender」；另有跨世界目标不执行等 10 条偏差，逐条见 [C4 §5](vanilla-1.12.2-execute-and-testfor.md)。
| C5 | `feature/core-presentation-commands` | [presentation-commands](vanilla-1.12.2-presentation-commands.md) | P2 | C1 | Core | `/title` `/particle` `/playsound` `/stopsound` `/tellraw`（合并的小命令组） |
| C6 | `feature/core-scoreboard-teams` | [scoreboard-teams](vanilla-1.12.2-scoreboard-teams.md) | P2 | C1 | Core | `/scoreboard teams` 子命令 + setdisplay 槽位 |

**C5 状态**：已按 C1 的线性方式（`--ff-only`）合入 `master`（父仓库 `002a8e3`，其 gitlink 指向 Core 的 `d9e7369`），两个 `master` 均已推送；Core 侧该支为五个原子提交 `0cb92ff`（`/title`、`/particle`、`/playsound`）、`fc4bc3d`（`/tellraw` + JSON 文本层）、`56b959d`（`/stopsound`）、`2718356`（修 actionbar 接收者 + handler 级测试）、`d9e7369`（反馈文本不能被 `${` 加固吞掉）。
Core 侧新增 `command_presentation.lua`（解析 + 两张白名单 + `MC|StopSound` 载荷编码器）、`command_text.lua`（JSON 文本组件层）、`cmd_title.lua` / `cmd_particle.lua` / `cmd_playsound.lua` / `cmd_stopsound.lua` / `cmd_tellraw.lua`、`Info.lua` 注册（in-game + console 共十条）与 `tests/test_presentation_commands.lua`；**无 `src/` 改动**。
`/tellraw` 由维护者在 2026-10 决定并入本支（理由：它与 `/title` 的文本参数缺的是**同一个** JSON 文本组件层，`cJson:Parse` 与 `cPlayer:SendMessageRaw` 都已导出且有文档，单列一支只是把同一件事评审两遍）。它同时消掉了前一提交记录的第 1 条偏差，代价是新增两条记录在案的偏差（服务端组件 `selector`/`score` 拒绝、含 `${` 的聊天类文本拒绝）。
验收：`luac -p` Core 全部 Lua 文件通过；九个新增/改动文件 `luacheck` **0 warning / 0 error**（`Info.lua` 的改动只新增 8 条 W113「handler 名未定义」，与该文件既有的 127 条同型，无新类型）；C1–C4 的五个纯 Lua 测试套件重跑无回归（35/153/95/50/44）；两套纯 Lua 测试 **270 + 68 条断言全绿**（前者含 `MC|StopSound` 载荷与原版抓包的逐字节比对，后者是 handler↔C++ 调用层的回归守卫）；行为（真实服务器 + 临时探针插件，控制台命令表，逐条见 [C5 §6](vanilla-1.12.2-presentation-commands.md)）覆盖成功路径的可达部分与全部拒绝路径；门 1/2/3/4/6 因改动面只在 Core 记「不适用」（未新增任何 C++ 符号）。
**范围变化（两次）**：①`/tellraw` 由维护者决定并入本支（与 `/title` 的文本参数共用同一个 JSON 文本组件层）；②`/stopsound` 起初因「1.12.2 的下发机制〔待核〕、且本仓库没有任何 Stop Sound 包类型」被判为需新 C++ 通路，**维护者的 ProtoProxy 原版抓包推翻了这一判断**：1.12.2 用 `MC|StopSound` **插件消息**（0x18，载荷 = 分类 + 声音事件两个可选 VarInt 字符串），而 `cClientHandle:SendPluginMessage` 本就已导出 → 纯 Lua 实现，本支并入。抓包字节与实现见 [C5 §2.4/§6](vanilla-1.12.2-presentation-commands.md)。
**实机验收**：维护者于 2026-10-18 用原版 1.12.2 客户端逐条实测**通过**（标题/副标题/actionbar/`times`/`clear`/`reset`、plain 与 JSON 两种文本形态、`/tellraw` 的组件透传、`/particle` 的粒子与 `count=0`、`/playsound` 的按玩家下发、`/stopsound` 真正停声）。实测过程中抓到并修掉两个只有实机能暴露的 bug（actionbar 接收者用错类、拒绝提示被 `${` 加固吞掉），修完重测全绿。
后续补抓（`Log_1792294770_5.log`）把 `MC|StopSound` 的另外两种形态也定死了（缺的那一段是**空串**而非省略），据此做的收尾修正也已按同一方式合入 `master`（Core `2eedbdb`；父仓库 `a9e8965`，gitlink 指向前者），两个 `master` 均已推送；三种形态的实测字节见 [C5 §2.4](vanilla-1.12.2-presentation-commands.md)。
`/summon` 与选择器 `type=` 的实体名查找也已统一到 `entity_names.lua`（见 [C1 §3.3](vanilla-1.12.2-command-target-selectors.md)）：生物名不再在 Lua 复制，直接问引擎的 `cMonster::StringToMobType`，非生物 21 个 id 只有一张表。
仍余〔待核〕：fuzz 场景本机仍无法执行（需外部 `CuberitePluginChecker`）。另有 `mode`/`<source>`/`<viewers>`/`[params]`/服务端组件/1.13+ 客户端不认该通道/成功反馈文本与原版不同形 等**已记录偏差**，逐条见 [C5 §5](vanilla-1.12.2-presentation-commands.md)。

**串行约束**：C2 → C3（同改 `CommandBlockEntity` / `PluginManager` 的上下文通路）；C1 → C4/C5/C6（都消费选择器）。
C1 与 C2 可并行（分属 submodule 与 `src/`，无同文件冲突）。**每个分支都必须自带测试。**

**建议起点**（已由 C1 完成）：「仅玩家 + `name=`/`type=`/`r=`」最小子集即可解锁 C4/C5；C1 实际落地了
§3.3 中除 `tag`/`score_*`/`rx/rxm/ry/rym` 外的全部参数，其余增量补。

## 3. 上游追踪 issue 对照（缺口来源，非本仓库台账）

上游 [issue #4888](https://github.com/cuberite/cuberite/issues/4888)（`gh issue view 4888` 实测）中与本批直接相关的条目：

- `Pre 1.0` → **Command Blocks #3239 #5131** → 本批 C3
- `Version 1.5` → **Renamed Command Blocks now use their name instead of @ in the chat #3239** → C3
- `Version 1.7` → **Minecart with command block #3499** → 本批**不**覆盖，见 §4

## 4. 已识别但本轮**不写 spec** 的候选（需维护者下达才展开）

| 候选 | 证据 | 为何先不写 |
|---|---|---|
| `/testforblocks`、`/blockdata`、`/entitydata`、`/xp`、`/gamerule` | 1.12.2 均存在（可从 [Commands 导航模板](https://minecraft.wiki/w/Commands/title) 的 JE-only / Removed 列表反查），Core 无对应 `cmd_*.lua` | 各自独立且小，宜并入 C4 或各自成支；`/entitydata` 需 SNBT 解析层 |
| `/tellraw` | Core 无 | 需 JSON 文本组件解析，规模自成一支 |
| `/summon` 的选择器化与 NBT 参数 | [cmd_summon.lua](../../Server/Plugins/Core/cmd_summon.lua) 收实体名 + 可选 `[x] [y] [z]`，**无选择器、无 NBT**（多余 token 被静默忽略）；实体名查找已与选择器的 `type=` 统一到 `entity_names.lua`（生物交给引擎表，非生物 21 个 id 一张表） | 与 C1 正交；NBT 部分仍需 C++ 通路，名字表部分已随对齐落地 |
| 命令方块矿车 #3499 | `E_ITEM_MINECART_WITH_COMMAND_BLOCK = 422` 已在 [BlockType.h:469](../../src/BlockType.h#L469)，无对应实体类 | 需新实体 + GUI + 轨道交互，规模 ≥ C3 |
| Tab 补全到参数级 | [PluginManager.cpp:1625](../../src/Bindings/PluginManager.cpp#L1625) 仅补命令名 | 需先核实 vanilla 1.12.2 的参数补全行为（〔推测〕：仅命令名与玩家名，置信度低），再决定 |

### 4.1 命令层缺口全量清单（C5 期间的只读审计，**清单不是任务**）

口径：1.12.2 命令集 = [Commands, rev 1152284](https://minecraft.wiki/w/Commands?oldid=1152284)（2017-09-29，晚于 1.12.2、早于首个 1.13 快照）JE 列 **64 条**，Core 覆盖 31 条（含部分实现），缺 **33 条** → **19 条纯 Lua 可做、14 条需新 C++**。
来源：minecraft.wiki 的 1.12.2 期修订（每条 URL 带 `oldid`）+ [wiki.vg 镜像](https://wikivg.booky.dev/w/Protocol)（protocol 340 期修订 rev 1323）+ 本仓库代码。
导出判定：`src/Bindings/Bindings.cpp` 生成物 **与** `ManualBindings_*.cpp` 的手写注册 **与** [APIDump](../../Server/Plugins/APIDump) 文档三者对照（只看 `tolua_export` 会漏掉 `tolua_begin/end` 区间与手写绑定）。
本次核验：（a）下表引用的 14 个 `oldid` 已逐条实抓，页面存在且提及该命令；（b）`cWorld` / `cBlockArea` / `cPlayer` / `cEntity` / `cScoreboard` 的导出判定抽查通过（含"手写绑定里注册"的 `cWorld:GetHeight`、`cBlockArea:Read` 等）；（c）"Lua 拿不到 NBT"〔`grep NBT src/Bindings/ManualBindings*.cpp` 零命中〕与"`cTeam` 无 color/nametag/deathMessage/collision 字段"〔[Scoreboard.h:125-192](../../src/Scoreboard.h#L125) 只有 Name/DisplayName/Prefix/Suffix/FriendlyFire/SeeFriendlyInvisible〕已实测。
未核验：各行的**具体语法细节**（如 `<state>` 的确切词法、`/function` 的磁盘布局、`/spreadplayers` 的落点算法）与全部"规模"估计 → 见下表的〔待核〕/〔推测〕；开工前须按 AGENTS §2.2/§2.3 自行复核。

**A. 纯 Lua 可做（所需 API 均已导出且有 APIDump 文档）**

| 命令 | 1.12.2 语法与来源（`oldid`） | Core 现状 | 关键已导出 API | 规模〔推测〕 | 阻碍 |
|---|---|---|---|---|---|
| `/fill` | `fill <x1 y1 z1> <x2 y2 z2> <block> [dataValue] [oldBlockHandling] [dataTag]`，模式 destroy/hollow/keep/outline/replace，上限 32768（[1218202](https://minecraft.wiki/w/Commands/fill?oldid=1218202)） | 无 | `cBlockArea:Read/Write/Fill`、`Get/SetRelBlockType` | M | `dataTag`（无 NBT 通路）、`<state>`（方块状态注册表未导出） |
| `/clone` | `clone <x1 y1 z1> <x2 y2 z2> <x y z> [maskMode] [cloneMode] [TileName] [dataValue]`，filtered/masked/replace + normal/force/move（[1218186](https://minecraft.wiki/w/Commands/clone?oldid=1218186)） | 无 | `cBlockArea:Read/Write/CopyTo/Merge` + `cWorld:DigBlock` | M | force/move **在 1.12.2 就有**（14w10a/14w20a），不是 1.13 新增 |
| `/setblock` | `setblock <x> <y> <z> <block> [dataValue] [oldBlockHandling] [dataTag]`（[1218219](https://minecraft.wiki/w/Commands/setblock?oldid=1218219)） | 无 | `cWorld:SetBlock/SetBlockMeta/GetBlockTypeMeta/DigBlock/DropBlockAsPickups` | S–M | 同 `/fill` 的 dataTag/state |
| `/testforblocks` | `testforblocks <x1 y1 z1> <x2 y2 z2> <x y z> [all\|masked]`，成功数=匹配方块数（[1123086](https://minecraft.wiki/w/Commands/testforblocks?oldid=1123086)） | 无 | `cBlockArea:Read` + `GetRelBlockType(Meta)` | S | — |
| `/xp` | `xp <amount> [player]` / `xp <amount>L [player]`（[1124282](https://minecraft.wiki/w/Commands/xp?oldid=1124282)） | 无 | `cPlayer:SetCurrentExperience/DeltaExperience/GetCurrentXp/CalcLevelFromXp` | S | 负数与 ±2147483647 截断 |
| `/teleport` | `teleport <target entity> <x> <y> <z> [<y-rot> <x-rot>]`（[1139121](https://minecraft.wiki/w/Commands/teleport?oldid=1139121)） | `/tp` 只支持 玩家→玩家 / 玩家→xyz | `cEntity:TeleportToCoords/SetYaw/SetPitch/MoveToWorld` + C1 | S | 1.13 的 `facing` 与"目的地实体"形态不得引入 |
| `/spreadplayers` | `spreadplayers <x> <z> <spreadDistance> <maxRange> <respectTeams> <player…>`（[1123067](https://minecraft.wiki/w/Commands/spreadplayers?oldid=1123067)） | 无 | `cEntity:TeleportToCoords`、`cWorld:GetHeight`、`cBlockInfo:IsSolid` | M | 落点算法允许来源无文档 → 必然偏差〔待核〕 |
| `/save-off`、`/save-on` | 同名，op 4（[1134732](https://minecraft.wiki/w/Commands/save?oldid=1134732)） | 只有 `/save-all` | `cWorld:SetSavingEnabled` / `cRoot:SetSavingEnabled` | S | — |
| `/setworldspawn` | `setworldspawn [<x> <y> <z>]`（[1123063](https://minecraft.wiki/w/Commands/setworldspawn?oldid=1123063)） | Core 的 `/setspawn` 同功能但非 vanilla 名 | `cWorld:SetSpawn` | S | 只剩改名/别名决策 |
| `/ban-ip` | `ban-ip <address>\|<name> [reason …]`（[1122910](https://minecraft.wiki/w/Commands/ban?oldid=1122910)） | 仅控制台有 | 无需 C++（`banlist.lua` 里的 IP 助手是 `local`，要么导出要么复用控制台 handler） | S | — |
| `/banlist` | `banlist ips\|players`（同上修订） | 仅控制台 | 无需 C++ | S | 玩家侧文案差异 |
| `/pardon`、`/pardon-ip` | `pardon <name>` / `pardon-ip <address>`（[1123035](https://minecraft.wiki/w/Commands/pardon?oldid=1123035)） | `/unban` = pardon；`/pardon-ip` 缺 | 无需 C++ | S | — |
| `/tellraw` | `tellraw <player> <raw json message>`（[1136502](https://minecraft.wiki/w/Commands/tellraw?oldid=1136502)） | 无 | `cPlayer:SendMessageRaw`（原样透传 JSON） | M | 成本在 Lua 侧 JSON 文本组件编码器；含 `${` 的消息会被 C++ 静默丢弃 |
| `/function` | `function <function> [if\|unless] [selector]`（[1126060](https://minecraft.wiki/w/Commands/function?oldid=1126060)） | 无 | C4 的 `cPluginManager:ExecuteCommandFromOrigin` + `cFile` | M | 1.12.2 的磁盘布局〔待核〕 |
| `/scoreboard teams …` | teams list/add/remove/empty/join/leave/option（option 键 color/friendlyfire/seeFriendlyInvisibles/nametagVisibility/deathMessageVisibility/collisionRule）([1133760](https://minecraft.wiki/w/Scoreboard?oldid=1133760)) | `/scoreboard` 只有 objectives+players | `cScoreboard:RegisterTeam/RemoveTeam/GetTeam/ForEachTeam`、`cTeam` | M | = **C6**；其中 color / nametagVisibility / deathMessageVisibility / collisionRule 四个键 `cTeam` 无字段 → 需 C++ |

**B. 需新 C++（本清单明确排除）**

| 命令 | 具体缺的 C++ 件 |
|---|---|
| `/stopsound` | **1.12.2 的 clientbound 包列表里没有 Stop Sound 包**（wiki.vg protocol 340 期 rev 1323；该包首次出现于 1.13 期），`src/` 也没有任何停止声音的包类型或 Lua API → 先核实原版 1.12.2 的下发机制（ProtoProxy 实测），再新增通路。见 [C5 §2.4](vanilla-1.12.2-presentation-commands.md) 与 §5 |
| `/gamerule` | 无 gamerule 子系统（`grep -rn Gamerule src/` 零命中；1.12.2 有 24 条规则） |
| `/defaultgamemode` | 无"默认游戏模式"概念（`grep DefaultGameMode src/` 零命中），世界 gamemode 对 Lua 只读 |
| `/setidletimeout` | 无 idle 判定/踢出（`grep IdleTimeout src/` 零命中） |
| `/worldborder` | 无边界状态（`grep WorldBorder src/` 零命中），1.12.2 有 clientbound World Border 0x38 |
| `/advancement` | 无进度系统；clientbound 0x4D/0x37 未实现（只有 serverbound 页签 handler） |
| `/recipe` | `pktUnlockRecipe` 与 `cClientHandle::SendUnlockRecipe` 存在但**未导出**；`cPlayer:GetKnownRecipes/AddKnownRecipe`、`cCraftingRecipes` 也未导出 → 小导出 + Lua |
| `/locate` | 无结构定位 API；结构集合本身不全（无海底神殿/林地府邸） |
| `/debug` | 无 profiler |
| `/blockdata`、`/entitydata` | **Lua 完全拿不到 NBT**（`ManualBindings*.cpp` 零 NBT 入口）；SNBT 解析只是较小的一半（`give.lua` 已有受限版本） |
| `/stats` | 无 stat→objective 联动，C++ 只有 3 处硬编码统计更新 |
| `/trigger` | criteria 枚举无 `trigger`，也无 enable/locked 状态；Core 亦无 `players enable` |
| `/publish` | 专用服务器上不存在 → **按设计不适用**，不是缺口 |

同批被"无 NBT"卡住的参数：`/summon … [dataTag]`、`/scoreboard players set|add|remove … [dataTag]`、以及 `/setblock`、`/fill`、`/clone`、`/testforblock` 的 `<state>`（`src/Registries/BlockStates.h` 无任何 tolua 标记）。

**C. 对 §4 上表的修正**：`/entitydata` 的硬阻碍是"Lua 拿不到 NBT"而非"缺 SNBT 解析层"；`/summon` 行应补上 1.12.2 确有 `[dataTag]`；/fill、/clone、/setblock、/testforblocks、/xp、/teleport、/save-off·on、/setworldspawn、/ban-ip、/banlist、/pardon·ip、/tellraw、/function 这些**纯 Lua 候选**上一版未收录；`/particle`、`/playsound`、`/title` 已由 C5 落地，`/stopsound` 由 C5 判定为需先核实。

## 5. 统一验收门（AGENTS §4.1 按改动面选）

- 改 `src/`（C2/C3/C4）：六道门全适用——`cd src && lua CheckBasicStyle.lua`；`cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=DEBUG -DSELF_TEST=Yes -DBUILD_TOOLS=Yes && cmake --build build`；`cd build && ctest --output-on-failure -E "UrlClient-test|Google-test"`；行为逐条核；改导出 API 时同步 APIDump 三处并跑自检。
- 只改 Core（C1/C5/C6）：风格/编译/ctest 记「不适用」，但**内容正确性是硬门**——Core 侧要求 `luacheck` + `luac -p` 干净，且 [tests/FuzzCommands.lua](../../Server/Plugins/Core/tests/FuzzCommands.lua) 仍通过；新增用例；用到未文档化成员时同步本仓库 `APIDump`（C1 只用了已文档化成员，无需同步）。
- C1/C5/C6/C4 的插件侧改动一律走 [AGENTS.md §6（一）](../../AGENTS.md) 的 fork 流程（`rrtt217/Core` **已创建**，C1 已推送 `8913d93212cf3fb4d7236437c1b06a83fa08e3f1`；见 [AGENTS.local.md §5](../../AGENTS.local.md)）。
