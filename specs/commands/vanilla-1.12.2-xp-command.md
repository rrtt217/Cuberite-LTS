# 纯 Lua 命令族（三）：`/xp`

分支：`feature/core-xp-command` ｜ 级别：P2 ｜ 前置：C1（`@a` 等选择器）｜
编排见 [roadmap §4.2](vanilla-1.12.2-command-layer-roadmap.md)

**为什么单独成支**：`/xp` 只有经验数值一种语义，但它的「加等级」不是「加经验点」——
需要把等级变化换算成总经验值（Cuberite 只存总经验，等级由 `CalcLevelFromXp` 派生），
这套换算值得单独评审；与其它命令没有共享参数。

## 1. 现状

| 命令 | Core 现状 | 所需 C++ API | 判定 |
|---|---|---|---|
| `/xp` | 无 `cmd_xp.lua` | `cPlayer:DeltaExperience`（[Player.cpp:279](../../src/Entities/Player.cpp#L279)）、`SetCurrentExperience`（[:259](../../src/Entities/Player.cpp#L259)）、`GetCurrentXp`、`GetXpLevel`、`GetXpPercentage`、`CalcLevelFromXp`（[:184](../../src/Entities/Player.cpp#L184)），全部已导出（[Bindings.cpp:38056](../../src/Bindings/Bindings.cpp#L38056) 起） | **API 齐，缺命令**；等级→总经验的换算函数 `XpForLevel()` **未导出**，需在 Lua 里按公式实现（见 §2.2） |

## 2. 行为规格

来源：[Commands/xp 的 1.12.2 期修订](https://minecraft.wiki/w/Commands/xp?oldid=1124282)
（rev 1124282，2017-07-11）；等级→总经验公式取自
[Experience 的 1.12.2 期修订](https://minecraft.wiki/w/Experience?oldid=1160829)（rev 1160829，2017-10-18）。
两个修订都早于首个 1.13 快照（17w43a，2017-10-25）。

### 2.1 语法与参数

1. 语法（同页）：
   - `xp <amount> [player]`
   - `xp <amount>L [player]`
   → 1.12.2 **没有** 1.13 的 `add`/`set`/`query` 子命令形态；`L` 是 1.12.2 的「加等级」写法。
2. `amount`（经验点）：「Must be between 0 and 2,147,483,647 (inclusive)」。
   **与本页 Result 一节的「Total experience and levels will not be reduced below 0」存在张力**：
   若点数额不允许为负，这句话只对等级有意义。两种读法都记在 §4 第 1 条；本支**接受负点数**
   （`DeltaExperience` 自身会把总经验夹到 0），按〔待核〕处理。
3. `amountL`（等级）：`-2,147,483,648 … 2,147,483,647`，负值扣等级。
4. `player`（可选）：「Specifies the target of the command. If not specified, defaults to the command's
   executor. **Not optional in command blocks.**」；1.12.2 的 `player` 参数接受玩家名与目标选择器
   （`@a` 等），**只作用于在线玩家**——非玩家实体被忽略。
5. Result：「Fails if arguments are not specified correctly, or if player fails to resolve to one or more
   online players. On success, adds experience or adds/removes levels. **Total experience and levels will
   not be reduced below 0.**」
6. 权限等级 **2**（同页 infobox 的 `oplevel=2`）——1.12.2 期修订为 [Commands/xp?oldid=1124282]，
   其 infobox 未含该字段，等级 2 取自现行页面的 `oplevel`〔待核：原版 1.12.2 用等级 1 玩家验证〕。

### 2.2 等级与总经验的换算（已核实）

Cuberite 的 `cPlayer` 只存 `m_CurrentXp`（总经验），`GetXpLevel()` 由
`CalcLevelFromXp()` 反推（[Player.cpp:237](../../src/Entities/Player.cpp#L237)），
`GetXpPercentage()` 给出当前等级内的进度（0–1，[:246](../../src/Entities/Player.cpp#L246)）。
`cPlayer` **没有**「直接加等级」的方法，`XpForLevel()` 也未导出。

1. 达到某等级所需的总经验（Experience 页，1.12.2 期修订）：
   - `0 ≤ level ≤ 16`：`level² + 6 × level`
   - `17 ≤ level ≤ 31`：`2.5 × level² − 40.5 × level + 360`
   - `level ≥ 32`：`4.5 × level² − 162.5 × level + 2220`
   该页同时给出 1–65 级的查表值（1→7、16→352、17→394、31→1507、32→1628、65→10670 等），
   本支的测试逐条对照其中若干项。
2. 加等级时**保留等级内进度**：现行页面（1.13+ 形态）对 `add ... levels` 写明
   「The amount of experience points in the current experience bar is scaled proportionally, keeping the
   progress of experience bar unchanged」；1.12.2 是否同样保留进度未在 1.12.2 期修订中明写
   〔推测：依据是 1.12.2 的等级与进度分字段存储、加等级只改等级字段，置信度高〕。
   → 目标总经验 = `XpForLevel(目标等级) + round(进度 × (XpForLevel(目标等级+1) − XpForLevel(目标等级)))`。
3. 等级被扣到 0 以下时：等级夹到 0、进度清零、总经验 0（Result 一节的「not reduced below 0」）。

### 2.3 数值边界

1. 点数加法的溢出：`DeltaExperience` 在 `delta > INT_MAX − 当前总经验` 时返回 -1
   （[Player.cpp:281](../../src/Entities/Player.cpp#L281)）并拒绝修改 → 本支把 -1 当作失败。
2. 等级换算出的总经验超过 `INT_MAX` 时 `SetCurrentExperience` 返回 false
   （[Player.cpp:261](../../src/Entities/Player.cpp#L261)）→ 失败；1.12.2 对同样情形的行为
   〔待核：原版用极大的 `L` 值观察是报错还是回绕〕。
3. `amount` 的语法：1.12.2 的整数参数由 Brigadier 的整数读取器解析——接受可选 `+`/`-`，
   不接受小数或其它字符〔推测：`Integer.parseInt` 语义，置信度高〕。

## 3. 实现映射（Core，纯 Lua）

| 规格点 | 落点 |
|---|---|
| `amount[L]` 解析、int32 边界、负数 | 新 `command_xp.lua`：`ParseXpAmount()` |
| 等级→总经验公式与「保留进度」换算 | 新 `command_xp.lua`：`GetXpForLevel()`、`GetTotalXpForLevelChange()`（纯函数，测试直接对照 Experience 页的表） |
| 目标解析（名字或选择器，只取玩家） | `cmd_xp.lua`：`ResolveTargets(token, MakeSelectorContext(Player, Origin), "player")`；缺省用执行者 |
| 加点数 / 加等级 | `cPlayer:DeltaExperience(delta)` / `SetCurrentExperience(GetTotalXpForLevelChange(...))` |
| 反馈 | 每个受影响玩家一行 `SendMessageSuccess`；控制台把各行拼成一条输出 |
| 命令注册 | `Info.lua` 的 `/xp` 与控制台 `xp` |

## 4. 已知偏差与未核实项

1. **负数点数的取舍**：规格页 Arguments 写点数 0…2,147,483,647，Result 写「总经验不会被减到 0 以下」；
   本支接受负点数并依赖 `DeltaExperience` 夹到 0〔待核：原版 1.12.2 `/xp -5` 是否成功；
   核法＝原版专用服执行 `/xp -5` 后看经验条/等级〕。若原版拒绝，本支需把解析下限改成 0。
2. **反馈文案**：`Given N experience to X` / `Given N levels to X` 是拟写，未与原版逐字比对
   〔待核：原版专用服逐条执行并抄录〕。多目标时本支每个玩家一行，与原版是否逐行相同也未核实。
3. **1.12.2 的等级进度保留**：见 §2.2 第 2 条〔推测〕。
4. **溢出**：见 §2.3 第 2 条〔待核〕。
5. **「无匹配」的判定**：选择器命中 0 个实体时本支返回失败 `No player was found`〔待核：
   原版对「选择器命中 0 个」与「名字查无此人」是否用同一句文案；核法＝各执行一次〕。
6. **跨世界**：目标解析沿用 C1 的跨世界候选与延迟机制；本支的反馈文本只反映**原点世界当拍**
   命中的玩家（其余世界一拍后生效，见 [C1 §7](vanilla-1.12.2-command-target-selectors.md)）。
7. **`/xp` 的参数别名**：1.13+ 的 `/experience` 与 `add/set/query` 形态**不引入**。

## 5. 测试与验收

### 5.1 纯 Lua 测试

`lua tests/test_xp_command.lua` → **49 passed, 0 failed**。覆盖：

- `ParseXpAmount`：点数、`+` 前缀、负点数、`L` 形态、INT32 两端、越界/小数/裸 `L`/双重 `L`/非数字/空/缺参。
- `GetXpForLevel`：对照 Experience 页 1.12.2 期修订的查表值 0/1/2/16/17/18/31/32/65。
- `GetTotalXpForLevelChange`：从 0 级加 3 级、零变化保持总经验、加/减 1 级保留等级内进度、
  扣到 0 级以下夹到 0。
- handler（fake 玩家 + fake 选择器层）：点数走 `DeltaExperience` 而**不**走 `SetCurrentExperience`；
  `L` 反之；控制台逐条文案（含单数 `level`）；无目标与「命令方块无 player 参数」都失败且不碰玩家；
  非法 amount 被拒；游戏内缺省目标是执行者且反馈走 `SendMessageSuccess`。

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
- `luacheck`：`command_xp.lua`、`cmd_xp.lua` **0 warning / 0 error**。
- `Info.lua`：新增 2 个 handler 名，按既有 W113 同型增长（未新增类型）。

### 5.3 真实服务器探针（控制台路径，`build/Server`）

```
Executing console command: "xp 5"
No player was found
Executing console command: "xp abc"
Invalid amount "abc"
Executing console command: "xp 7 NoSuchPlayer"
Player "NoSuchPlayer" not found
Executing console command: "xp 3L NoSuchPlayer"
Player "NoSuchPlayer" not found
```

控制台没有执行者，`xp 5` 必须失败（「Not optional in command blocks」同理），与 §2.1 第 4 条一致；
名字查不到时文案来自 C1 的选择器层。

### 5.4 未覆盖

- **成功路径**：本机无客户端，`/xp 5 <玩家>` 与 `/xp 3L <玩家>` 的实机效果（经验条/等级变化、
  逐行聊天反馈）未实测〔待核：真实 1.12.2 客户端连入后执行两组命令，对照经验条与等级〕。
  等级换算的正确性由 §5.1 对照 Experience 页查表值保证，但「引擎的 `CalcLevelFromXp` 与本文件的
  `GetXpForLevel` 互为逆函数」只在数学上成立，未在真实玩家上对拍〔待核：用 `/xp 30L` 后读等级〕。
- **权限闸门**：无客户端〔待核〕。
- **跨世界目标**：C1 的延迟机制未在 `/xp` 上单独实测。
