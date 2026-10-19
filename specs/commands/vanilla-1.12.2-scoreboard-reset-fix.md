# `/scoreboard players reset`（不带目标）与 Core 的 `Objetive` 拼写缺陷

分支：`feature/core-scoreboard-reset-fix` ｜ 级别：P2（缺陷修复）｜ 前置：无

## 1. 1.12.2 行为（来源）

`scoreboard players reset <entity> [objective]`：删除该实体在指定目标中的分数；**省略目标时删除该实体在全部目标中的分数**。语义是**移除记录**（不是把分数设为 0），且"总是成功，即使实体不在记分板上"。

来源：Minecraft Wiki “Scoreboard”，2017-10-06 修订（rev 1156283），*Players commands* 表 ——
<https://minecraft.wiki/w/Scoreboard?oldid=1156283>

## 2. 缺陷（修复前，实测）

[cmd_scoreboard.lua](../../Server/Plugins/Core/cmd_scoreboard.lua) 的 `ResetAllObjectives` 闭包形参名是 `Objective`，函数体却写成 `Objetive`：

```lua
local ResetAllObjectives = function(Objective)
	Objetive:ResetScore(TargetedPlayer)  -- 未定义全局，取到 nil
end
```

于是 `/scoreboard players reset <player>`（不带目标名）对**每个**目标抛 Lua 错误：

```
LUA: Plugins/Core/cmd_scoreboard.lua:130: attempt to index global 'Objetive' (a nil value)
```

**关键点**：这条错误发生在 `cScoreboard:ForEachObjective()` 的 C++ 回调里，被 `cLuaState::Call()` **吞掉、只写日志**；
控制器继续往下走，命令照常回**成功文案** `Player's Score(s) has successfully been reseted`，而分数一条也没清
（出错求值的正是 `Objetive:ResetScore(...)`，`ResetScore` 从未被调用）。只在记分板**至少有一个目标**时触发
（无目标时闭包不会被调用）。

- **修复前实测**（2026-10-19，真实服务器 + 控制台）：`objectives add t9 dummy T9` → `players set TestBot t9 5` →
  `players reset TestBot` 回 `Player's Score(s) has successfully been reseted`；日志出现上面那行
  （teams 分支上文件行号平移后是 `:150`）；停服后 `scoreboard.dat` 的 `PlayerScores` 仍是
  `('TestBot', 't9', 5)` → 分数**原封不动**。

## 3. 修复

函数体读形参（一行）。

**遗留（不在本支范围）**：同一文件的 `ListPlayerObjective` 引用了未定义的 `GlobalPlayer`（[cmd_scoreboard.lua:125](../../Server/Plugins/Core/cmd_scoreboard.lua#L125)）；`/scoreboard players list <player>` 会以**同样方式**在日志里报错（错误同样被 `ForEachObjective` 回调吞掉，玩家只看到一行标题）。按「一功能一分支」另支处置，本支不改。

## 4. 验证

### 4.1 静态

- `luac -p`：Core 全部 `*.lua` 与 `tests/*.lua` 通过。
- `luacheck --config build/Server/.luacheckrc`：`cmd_scoreboard.lua` **26 warnings / 0 errors**；本支不新增
  （修复前 27 条，多出的那条正是 `Objetive` 的 “accessing undefined variable”）。

### 4.2 单元（无 oracle 环境下的行为门）

`lua tests/test_scoreboard_reset.lua`（Core 插件目录）→ **18 passed, 0 failed**。覆盖：

- 记分板有两个目标时，`reset <player>` 对每个目标各调用一次 `ResetScore(<player>)`，返回 `true`、回成功文案、无失败文案；
- 指定目标名时只清该目标，另一个目标不受影响；
- 未知目标仍回 `Unknown objective "nope"`（前一支的防崩路径不回归）；
- 缺目标时回用法串。

**回归守卫实测**：把该行改回 `Objetive` 后同一测试 **6 failed**，首条失败即
`reset without an objective must not raise a Lua error (tests/../cmd_scoreboard.lua:130: attempt to index a nil value (global 'Objetive'))`。

**为什么这里用 `pcall`，而实机看不到失败**：引擎里这条错误被 C++ 回调吞掉（§2）；测试的 `ForEachObjective` 桩是纯 Lua，
错误会回到 `pcall`。因此该测试守的是「每个目标都真的被 `ResetScore`」，而不只是错误文本。

### 4.3 实机

真实服务器（`build/Server` 的本仓库二进制）控制台实测：

| 用例 | 修复后 |
|---|---|
| `scoreboard objectives add t1 dummy T1` / `add t2 dummy T2` | 各自 `Objective … has been created` |
| `scoreboard players set TestBot t1 5` / `set TestBot t2 7` | `Score … successfully assigned to TestBot` |
| `scoreboard players reset TestBot`（不带目标） | `Player's Score(s) has successfully been reseted`；该次运行（21:17:58）的日志段**没有** `attempt to index … 'Objetive'`（21:20:42 的那条来自修复前的对照运行，见 §2） |
| 停服后 `world/data/scoreboard.dat`（gzip NBT 解析） | `PlayerScores` 是**空列表**，`Objectives` 仍有 `t1`/`t2` → 分数是**被移除**，不是置 0 |

- **〔待核〕**：玩家侧（而非控制台）的回复文案未逐字抓取；控制台与玩家走同一个 handler，只是接收者不同，
  核法是用原版 1.12.2 客户端执行该命令读回包。

## 5. 验收门（按 [AGENTS.md §4.1](../../AGENTS.md)）

只改 `Server/Plugins/Core/`（Lua + 测试）与父仓库 `specs/`、gitlink：

| 门 | 结果 |
|---|---|
| 1 风格 | 不适用（无 `.cpp`/`.h`） |
| 2 编译 | 不适用（纯 Lua） |
| 3 ctest | 不适用（同上） |
| 4 绑定依赖 | 不适用 |
| 5 行为规格 | 本文件 + §4 |
| 6 APIDump | 不适用（未新增/导出 C++ API） |
