# C4 `/execute`、`/testfor`、`/testforblock`（1.12.2 形态）

分支：`feature/core-execute-commands` ｜ 级别：P1 ｜ 前置：C1（选择器）、C2（执行上下文）、C3（成功计数）｜ 编排见 [roadmap](vanilla-1.12.2-command-layer-roadmap.md)

三条命令合并为一支：它们共享**同一套依赖**（选择器解析、原点覆盖、成功计数回传），且**同一批触点**（Core 新增 3 个 `cmd_*.lua` + `Info.lua` 注册 + C++ 侧一条「带原点执行命令」的通路）。单独立项每条都不足 200 行。

## 1. 现状

- Core **完全没有**这三条命令：`Server/Plugins/Core/` 无 `cmd_execute.lua` / `cmd_testfor*.lua`（目录清单实测），[Info.lua](../Server/Plugins/Core/Info.lua) 亦无对应条目。
- 上游 [issue #4888](https://github.com/cuberite/cuberite/issues/4888) **未**跟踪这三条 → 属本 fork 自行发现的缺口（AGENTS §1：缺口由维护者逐条下达，本文仅为候选规格）。
- 已有可复用的「以他者身份执行」通路：Core `/do` 用 `cPluginManager:ExecuteCommand(a_Player, newSplit)`（[do.lua:13](../Server/Plugins/Core/do.lua#L13)，C++ 侧 [PluginManager.h:355](../src/Bindings/PluginManager.h#L355)）→ 能换身份，**不能换原点**。这正是 C2 的 origin 通路要解决的。
- 成功计数通路已存在但恒 0（[CommandBlockEntity.cpp:204](../src/BlockEntities/CommandBlockEntity.cpp#L204)）→ 依赖 C3 才能兑现「`/execute` 的成功计数回传给命令方块」。

## 2. 行为规格（已核实的部分）

来源：[Commands/execute (old)](https://minecraft.wiki/w/Commands/execute_\(old\))（专讲 1.13 之前形态的页面，已核）：

1. **语法**：
   - `execute <entity> <x> <y> <z> <command …>`
   - `execute <entity> <x> <y> <z> detect <x2> <y2> <z2> <block> <dataValue|-1> <command …>`
2. **语义**：以 `<entity>` 为代表执行者、以 `<x y z>` 为原点执行命令；权限仍取执行者（页面首段：「on behalf of one or more other entities, with originating permissions」）。
3. **`detect`**：先做一次 `/testforblock` 风格的单方块检查，通过才执行后面的命令。
4. **相对坐标**：`~` 表示「相对于给定坐标」——页面示例 `execute @p ~ ~ ~ detect ~ ~-1 ~ sand -1 kill @p` 已证明 1.13 前支持 `~`。
5. **`<dataValue|-1>`**：`-1` 表示任意数据值。
6. **成功计数回传命令方块**：版本历史 `1.8 → 14w08a`「The success value of an executed command is now passed back to the command block it was run from」。
7. **版本边界**：`1.13 → 17w45a` 起 `/execute` 被整体重做（`as/at/run/if/unless`）→ 本 fork **不**得实现 1.13+ 子命令（[AGENTS.md §9](../AGENTS.md)）。

**未核实（实现前必须补来源，标为推测）**：
- `^`（insertion/局部坐标）在 1.12.2 是否可用 → **推测：不可用**（现行页面把 caret 记为 1.13+ 引入，但未逐条核对）。本分支只实现 `~` 与绝对坐标。
- `/testfor` 与 `/testforblock` 的 1.12.2 精确语法与错误文案 → 需 [Commands/testfor](https://minecraft.wiki/w/Commands/testfor)、[Commands/testforblock](https://minecraft.wiki/w/Commands/testforblock) 的历史修订（本轮未取）。
- 方块参数的写法（数字 ID / `minecraft:sand` / 名称 + 数据值）在 1.12.2 的确切集合。
- 多目标时**成功计数的定义**（选中数？每个目标各 1？）→ 与 C3 §2.2 一同核实。

## 3. 本分支范围

**做**：
1. `cmd_execute.lua`：解析 `<entity> <x> <y> <z> [detect …] <command…>`；命令体取「从第 6 个 token 起的原始剩余串」（照抄 [do.lua:9](../Server/Plugins/Core/do.lua#L9) 的 `table.concat(Split, " ", 3)` 手法，起点改为 6 或 detect 后的位置）。
2. `cmd_testfor.lua`：`/testfor <target>` → 输出命中数量作为成功计数（**这是命令方块计数能被观测的主要途径**）。
3. `cmd_testforblock.lua`：`/testforblock <x> <y> <z> <block> [<dataValue>]`。
4. 三者都必须走 C1 的选择器模块与 C2 的 origin 通路；**禁止**在 Lua 里自己再造一份选择器解析。
5. `Info.lua` 注册 + 权限节点（沿用 `core.*` 命名，如 `core.execute`）+ 帮助文案。

**不做**：`/testforblocks`（体积比较，独立一支）、`/blockdata`、`/entitydata`、1.13+ 子命令、`/function`。

## 4. 测试与验收

- **可判定纯逻辑抽头**：把「execute 行的参数切分」抽成不碰游戏对象的纯函数（命令体起点、detect 参数个数、坐标 token 的 `~` 展开），配 Lua 表驱动用例。
- **fuzz**：[tests/FuzzCommands.lua](../Server/Plugins/Core/tests/FuzzCommands.lua) 的 `choices` 增列 `"~"`、`"detect"`、`"-1"`、`"@p"`；`maxLen` 需 ≥ 6 才能命中 `/execute` 的最小可用形态 → 若因此显著拉长 fuzz，改为新增独立场景文件。
- 改 `src/`（若为 origin 执行新增 C++ 入口）→ 六道门全适用；只改 Core → `luac -p` / `luacheck` / fuzz 三件。
- 行为证据：`/execute @p ~ ~ ~ detect ~ ~-1 ~ sand -1 kill @p` 与 `/testfor @e[type=...]` 的手工记录（实机不具备时明确写「未验」）。

## 5. 已知偏差 / 风险

- **依赖链最深**的一支：C1 的选择器语义、C2 的原点、C3 的成功计数任一未合并，本支就只能做到「能跑但计数不对」→ 交付说明必须显式列出被哪一环挡住（AGENTS §3 的「可绕过相关缺口」处理方式：留显式 TODO，不在本支私补）。
- `/execute` 以他者身份执行会**绕过权限检查**：C++ 的 `ExecuteCommand` 与 `ForceExecuteCommand` 语义不同（[PluginManager.h:355 vs 358](../src/Bindings/PluginManager.h#L355)，前者查权限、后者不查）。必须用**查权限**的那条，否则 `/execute` 成为提权漏洞；此条须写成测试。
- 命令体是「原始剩余串」，与 C2 的引号切分改动交互微妙（切分后 `Split` 已丢引号）→ 必须用 `a_EntireCommand` 原文切片，不能用重切分结果。
