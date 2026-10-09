# C6 记分板队伍与显示槽位（`/scoreboard teams`）

分支：`feature/core-scoreboard-teams` ｜ 级别：P2 ｜ 前置：C1（`team=` / `scores=` 选择器参数依赖本支落地才有意义）｜ 编排见 [roadmap](vanilla-1.12.2-command-layer-roadmap.md)

## 1. 现状与差距（逐条实测）

| # | 现状 | 位置 | 应有 |
|---|---|---|---|
| 1 | Core `/scoreboard` 只实现 `objectives` 与 `players`；**没有 `teams` 子命令** | [cmd_scoreboard.lua:42](../Server/Plugins/Core/cmd_scoreboard.lua#L42) 的用法串自陈只支持 `add, remove, list, setdisplay, modify`；全文 `grep -n teams` 零命中 | `/scoreboard teams add\|remove\|join\|leave\|empty\|list\|option` |
| 2 | C++ 侧队伍 API **完整存在**，纯属「没人调」 | [Scoreboard.h:125](../src/Scoreboard.h#L125) `cTeam`（`AddPlayer`/`RemovePlayer`/`HasPlayer`/`Reset`/前后缀/`SetFriendlyFire`/`SetCanSeeFriendlyInvisible`）、`RegisterTeam` [:231](../src/Scoreboard.h#L231)、`RemoveTeam` [:234](../src/Scoreboard.h#L234)、`GetTeam` [:237](../src/Scoreboard.h#L237)、`ForEachTeam` [:269](../src/Scoreboard.h#L269)、`QueryPlayerTeam` [:257](../src/Scoreboard.h#L257) | 命令侧封装即可 |
| 3 | 玩家 ↔ 队伍已有通路 | [Player.h:243-254](../src/Entities/Player.h#L243)（`GetTeam`/`SetTeam`/`UpdateTeam`） | 加入/退出即时反映到名牌 |
| 4 | 显示槽位只有三个：`dsList` / `dsSidebar` / `dsName` | [Scoreboard.h:203-210](../src/Scoreboard.h#L203) | 与 1.12.2 的 `list` / `sidebar` / `belowname` 对应（**推测**：1.12.2 正是这三个；现行 wiki 还有 `sidebar.team.<color>` 属 1.8–1.12 的彩色侧边栏？→ 待核实） |
| 5 | 聊天前后缀取自 rank 而非队伍 | [ClientHandle.cpp:1560-1562](../src/ClientHandle.cpp#L1560) 用 `cPlayer::GetPrefix/GetSuffix`（实现见 [Player.cpp:1674/1683](../src/Entities/Player.cpp#L1674)） | 队伍前后缀应作用于名牌与聊天（1.12.2 行为，待核实具体叠加顺序） |
| 6 | 记分板已落盘 | [ScoreboardSerializer.cpp:20](../src/WorldStorage/ScoreboardSerializer.cpp#L20)（每世界 `scoreboard.dat`） | 队伍是否随存？→ **待核实**（该序列化器的字段集合本轮未读） |

**结论**：C6 的主体是「Core 缺命令」而非「C++ 缺能力」，因此与 C1/C5 同属插件侧分支；只有 §5（前后缀应用）与 §4（槽位）可能触及 `src/`。

## 2. 行为规格（**待核实**，本轮未取来源页）

1.12.2 的 `/scoreboard teams option` 键集合、各键取值、颜色码格式、`collisionRule` 是否已在 1.12.2 存在、`empty` 与 `leave` 的差别、`join` 省略目标时的默认行为——**本文件一律不给具体清单**（[AGENTS.md §2.3](../AGENTS.md) 禁止凭记忆写）。开工前先取：

- [/scoreboard](https://minecraft.wiki/w/Commands/scoreboard)（现行页对 1.12.2 有偏差，需配版本历史或 1.12.2 时期修订）
- [Scoreboard](https://minecraft.wiki/w/Scoreboard) 主条目（显示槽位与队伍渲染）

核实结果**必须回填本节**并附来源链接，再动代码。

## 3. 本分支范围

**做**：
1. `cmd_scoreboard.lua` 增 `teams` 分支：`add` / `remove` / `join` / `leave` / `empty` / `list` / `option`，全部走 `cScoreboard:RegisterTeam/RemoveTeam/GetTeam/ForEachTeam` + `cTeam:AddPlayer/RemovePlayer`。
2. `option` 的键按 §2 核实结果实现；C++ 只有 `SetFriendlyFire` / `SetCanSeeFriendlyInvisible` / 前后缀 / 显示名（[Scoreboard.h:155-167](../src/Scoreboard.h#L155)）→ 若 1.12.2 需要更多（如队伍颜色），**属 C++ 字段缺口**，按 AGENTS §3（1） 作为独立原子提交的前置依赖列出并单列规格，不在本支私加字段。
3. 目标解析统一走 C1 模块（`/scoreboard teams join <team> <targets>` 的 `<targets>` 是选择器位）。
4. 越界与文案沿用现有 `cmd_scoreboard.lua` 的多行响应风格（`MultiLineResponse`）。

**不做**：`objectives modify` 之外的既有行为改动、`/scoreboard objectives criteria` 的高级 NBT 选项、R 显式分数显示、彩色侧边栏槽位（若核实属 1.13+）。

## 4. 测试与验收

- **纯逻辑抽头**：`teams` 参数解析（子命令 + 队伍名 + 目标串）写成纯函数 + Lua 表驱动用例；队伍名含空格的引号行为依赖 **C2**。
- **状态用例**：同一世界内 `add → join → list → empty → leave → remove` 全链路，断言 `cScoreboard:ForEachTeam` 与 `cPlayer:GetTeam` 的一致性（可放 Core 的 Lua 测试或 `tests/` 的 C++ 用例，二者择一并说明理由）。
- **持久化**：若 §2 核实出「队伍应随 `scoreboard.dat` 落盘」，补一条写→读往返用例；当前 [ScoreboardSerializer](../src/WorldStorage/ScoreboardSerializer.cpp) 字段集合**未核**，先列入检查项。
- Core 侧三件（`luac -p` / `luacheck` / [FuzzCommands.lua](../Server/Plugins/Core/tests/FuzzCommands.lua)）；若为队伍颜色改 C++ 导出 → 六道门全适用 + 门 4 + 门 6。

## 5. 已知偏差 / 风险

- **本支最容易被低估的一条**：队伍不只是命令，还牵动**名牌渲染、 friendly fire、隐形可见性、聊天着色**。C++ 有字段不等于有应用点；实现前须逐条定位应用点（`SetTeam`/`UpdateTeam` 的调用方、`cTeam::AllowsFriendlyFire` 的读取方），找不到应用点的要在交付说明里列为「命令可用但行为未接」的偏差。
- 与 rank/前缀体系（Core 的 `web_playerranks.lua`）叠加时谁先谁后未定 → 待 §2 核实 + 维护者定夺。
- 与 **C1** 的 `team=` 参数互锁：本支未合并前，C1 不应声明支持 `team=`（在 C1 的参数表里标注「依赖 C6」）。
