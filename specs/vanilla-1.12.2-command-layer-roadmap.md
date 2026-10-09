# Vanilla 1.12.2 命令层（Command Layer）修复编排

本文件是**编排索引**，不含实现。它把 2026-08 的命令层审计结果拆成 **6 个可独立编译、可独立评审、可独立回滚** 的分支，每个分支对应一份 spec。
按 [AGENTS.md](../AGENTS.md) §3：**不做自动续跑**，本文件只是「列候选 + 写规格」，未获下达不开工。

- 审计范围：命令分发与切分（`cPluginManager` / `InfoReg.lua`）、目标选择器、命令方块、`/execute` 家族、表现类命令、记分板队伍。
- 判定口径：**缺失**=无代码；**空转**=有字段/入口但无逻辑读取；**偏差**=有逻辑但语法/数值/时机与 1.12.2 不符。
- 编号用 **C 系列**，与状态效果的 S 系列（`vanilla-1.12.2-status-effects-roadmap.md`，写作时**尚未提交**）不冲突；**S16**（Core `/effect`）的选择器部分由本批 C1 接管，见 C1 §5。
- **不确定标注约定**：本系列用 **推测**（无 1.12.2 专用来源支撑）与 **待核实**（须补来源后才准开工）两种标记，语义等同 S 系列使用的 `[needs-check]`；带标记的条目一律不得当作实现依据。

## 1. 关键事实（写作时全部实测）

1. **不存在 C++ 命令解析框架**：`src/` 内无 `CommandParser.*` / `CommandSystem.*`（`git ls-files | grep -i command` 只有 [CommandOutput.cpp](../src/CommandOutput.cpp)、[CommandBlockEntity.cpp](../src/BlockEntities/CommandBlockEntity.cpp) 等）；`grep -rn "Selector" src` 零命中。命令注册/分发全在 [PluginManager.cpp:1256-1258](../src/Bindings/PluginManager.cpp#L1256)：`StringSplit(a_Command, " ")` → 按首 token 查表 → 把原始 token 数组交给插件（[ManualBindings.cpp:67](../src/Bindings/ManualBindings.cpp#L67)）。
2. **切分器不支持引号**：命令路径用 [StringSplit](../src/StringUtils.cpp#L51)（纯分隔符切分）；仓库已有 [StringSplitWithQuotes](../src/StringUtils.cpp#L72) 但只导出给 Lua（[ManualBindings.cpp:4532](../src/Bindings/ManualBindings.cpp#L4532)），命令路径未用。
3. **玩家命令的输出被丢弃**：[PluginManager.cpp:1301](../src/Bindings/PluginManager.cpp#L1301) 调 handler 时 `a_Output` 传 `nullptr`；只有控制台路径（[ManualBindings.cpp:29-45](../src/Bindings/ManualBindings.cpp#L29) 的 `LuaCommandHandler`）会把输出回传。
4. **插件侧无参数类型系统**：注册仅有 `Permission` / `HelpString` / `Subcommands`（[Info.lua](../Server/Plugins/Core/Info.lua) + [InfoReg.lua:141](../Server/Plugins/InfoReg.lua#L141)）→ 目标选择器只能在 handler 内自行解析。
5. **唯一存在的选择器是 Core `/effect` 的硬编码 `@a`**：[cmd_effect.lua:20,28,50,57](../Server/Plugins/Core/cmd_effect.lua#L50)，仅覆盖在线玩家，无 `[...]` 参数。全仓库（含 submodule）再无 `@p/@r/@e/@s`。
6. **命令方块只有「能跑」**：成功计数恒 0（[CommandBlockEntity.cpp:204](../src/BlockEntities/CommandBlockEntity.cpp#L204)）、比较器无输出（[CommandBlockHandler.h](../src/Simulator/IncrementalRedstoneSimulator/CommandBlockHandler.h) 只 `Activate()`）、`TrackOutput` 写死 1（[NBTChunkSerializer.cpp:593](../src/WorldStorage/NBTChunkSerializer.cpp#L594)）、循环/链方块（ID 210/211，[BlockType.h:229-230](../src/BlockType.h#L229)）**无区块实体工厂**（[BlockEntity.cpp:90](../src/BlockEntities/BlockEntity.cpp#L90) 只建 137）→ 放置走到 `default` 分支的 `ASSERT`。
7. **选择器/`/execute` 所需 API 已在**：`cWorld:ForEachEntity`（[World.h:269](../src/World.h#L269)）、`ForEachEntityInBox`（[:277](../src/World.h#L277)）、`IsPlayer/IsPawn`（[Entity.h:161-164](../src/Entities/Entity.h#L161)）、`cMonster.StringToMobType`（[Monster.h:191](../src/Mobs/Monster.h#L191)）、`SendSetTitle`/`SendTitleTimes`（[ClientHandle.h:222,232](../src/ClientHandle.h#L222)）、`BroadcastParticleEffect`（[World.h:182](../src/World.h#L182)）。→ **多数缺口是插件侧 + 少量 src 通路，不需要新架构。**
8. **Cleanroom 结论（重要）**：Brigadier 是 Mojang 自研、MIT 许可、**1.13/17w45a 才进入 vanilla** 的命令框架（[minecraft.wiki/Brigadier](https://minecraft.wiki/w/Brigadier)）。本仓库基线 1.12.2，且 [AGENTS.md](../AGENTS.md) §2 禁止引入 Mojang 代码及其派生物 → **不得**引入 brigadier 或任何语言 port（详见 [C1 §5](vanilla-1.12.2-command-target-selectors.md)）。
9. **聊天层不会吞 `@`**：内部颜色分隔符是 `§`（[ChatColor.cpp:5-6](../src/ChatColor.cpp#L5)），文本内代码前缀是 `&`（[CompositeChat.cpp:107](../src/CompositeChat.cpp#L107)），style 串里的 `@` 被忽略 → 回显 `@a` 安全。

## 2. 分支清单与顺序

| # | 分支 | spec | 级别 | 前置 | 改动面 | 一句话 |
|---|---|---|---|---|---|---|
| C1 | `feature/core-target-selectors` | [command-target-selectors](vanilla-1.12.2-command-target-selectors.md) | P0 | — | Core（submodule）| 选择器层 `@p @a @r @e @s` + `[...]`，命令共用 |
| C2 | `feature/command-context-and-splitting` | [command-context-and-splitting](vanilla-1.12.2-command-context-and-splitting.md) | P0 | — | `src/` | 执行上下文（世界/位置/执行者）+ 引号切分 + 玩家命令输出回传 |
| C3 | `feature/command-blocks-parity` | [command-blocks](vanilla-1.12.2-command-blocks.md) | P1 | C2 | `src/` | 成功计数/比较器/`TrackOutput`/链与循环方块/改名显示 |
| C4 | `feature/core-execute-commands` | [execute-and-testfor](vanilla-1.12.2-execute-and-testfor.md) | P1 | C1, C2, C3 | Core + `src/` | `/execute`、`/testfor`、`/testforblock`（1.12.2 形态） |
| C5 | `feature/core-presentation-commands` | [presentation-commands](vanilla-1.12.2-presentation-commands.md) | P2 | C1 | Core | `/title` `/particle` `/playsound` `/stopsound`（合并的小命令组） |
| C6 | `feature/core-scoreboard-teams` | [scoreboard-teams](vanilla-1.12.2-scoreboard-teams.md) | P2 | C1 | Core | `/scoreboard teams` 子命令 + setdisplay 槽位 |

**串行约束**：C2 → C3（同改 `CommandBlockEntity` / `PluginManager` 的上下文通路）；C1 → C4/C5/C6（都消费选择器）。
C1 与 C2 可并行（分属 submodule 与 `src/`，无同文件冲突）。**每个分支都必须自带测试。**

**建议起点**：C1 先落「仅玩家 + `name=`/`type=`/`r=`」最小子集即可解锁 C4/C5；其余参数增量补。

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
| `/summon` 的选择器化与 NBT 参数 | [cmd_summon.lua:179](../Server/Plugins/Core/cmd_summon.lua#L179) 只收怪物名 | 与 C1 正交；需 1.12.2 实体名对照表 |
| 命令方块矿车 #3499 | `E_ITEM_MINECART_WITH_COMMAND_BLOCK = 422` 已在 [BlockType.h:469](../src/BlockType.h#L469)，无对应实体类 | 需新实体 + GUI + 轨道交互，规模 ≥ C3 |
| Tab 补全到参数级 | [PluginManager.cpp:1625](../src/Bindings/PluginManager.cpp#L1625) 仅补命令名 | 需先核实 vanilla 1.12.2 的参数补全行为（**推测**：仅命令名与玩家名），再决定 |

## 5. 统一验收门（AGENTS §4.1 按改动面选）

- 改 `src/`（C2/C3/C4）：六道门全适用——`cd src && lua CheckBasicStyle.lua`；`cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=DEBUG -DSELF_TEST=Yes -DBUILD_TOOLS=Yes && cmake --build build`；`cd build && ctest --output-on-failure -E "UrlClient-test|Google-test"`；行为逐条核；改导出 API 时同步 APIDump 三处并跑自检。
- 只改 Core（C1/C5/C6）：风格/编译/ctest 记「不适用」，但**内容正确性是硬门**——Core 侧要求 `luacheck` + `luac -p` 干净，且 [tests/FuzzCommands.lua](../Server/Plugins/Core/tests/FuzzCommands.lua) 仍通过；新增用例；用到未文档化成员时同步本仓库 `APIDump`。
- C1/C5/C6/C4 的插件侧改动一律走 [AGENTS.md §6（一）](../AGENTS.md) 的 fork 流程（`rrtt217/Core` **尚未创建**，见 [AGENTS.local.md §5](../AGENTS.local.md)）。
