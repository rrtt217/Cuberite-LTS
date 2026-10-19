# C6 记分板队伍（`/scoreboard teams`）与显示槽位

分支：`feature/core-scoreboard-teams` ｜ 级别：P2 ｜ 前置：C1（选择器层，已合并）｜ 状态：**已实现**
（Core 命令层 + 一处 `src/` 持久化前置修复；四槽位选项与 Teams 包等 C++ 缺口逐条列在 §5）

## 1. 行为规格（1.12.2，来源已核实）

来源：Minecraft Wiki “Scoreboard”，**2017-10-06 修订（rev 1156283）**——该修订晚于 1.12.2（2017-09-18）、
早于 1.13 的命令改写，是能直接引用的 1.12.2 期页面：
<https://minecraft.wiki/w/Scoreboard?oldid=1156283>（Teams 一节、Command reference → Teams commands 表、
NBT format → Teams / DisplaySlots、Display slots 一节）。

### 1.1 队伍模型

- 队伍由 **name**、**display name**、**prefix**、**suffix**、**friendly fire** 等选项与成员列表组成。
- name 用于内部引用：大小写敏感、**必须是单个词**、**最长 16 字符**、同记分板内唯一。
- display name 可含空格、**最长 32 字符**，省略时**默认为 name**。
- prefix / suffix 插在成员名字前后（1.12.2 没有通过命令修改它们的 option 键，只存在于 NBT）。
- 每个实体只能属于一个队伍。
- 默认值：friendlyfire = **true**；seeFriendlyInvisibles = **true**；nametagVisibility = **always**；
  deathMessageVisibility = **always**；collisionRule = **always**。

### 1.2 命令表

| 命令 | 说明 | 成功条件 |
|---|---|---|
| `scoreboard teams add <name> [display name...]` | 创建队伍 | name 未被占用且 ≤16；display name ≤32 |
| `scoreboard teams remove <name>` | 删除队伍 | name 存在 |
| `scoreboard teams empty <name>` | 清空成员 | name 存在**且至少有一个成员** |
| `scoreboard teams join <team> [entities...]` | 把实体加进队伍；**省略实体时由执行命令的实体加入** | team 存在；名字**不必在线或存在**，也可以已在队里；`*` 表示记分板跟踪的全部实体 |
| `scoreboard teams leave [entities...]` | 让实体退出各自队伍；省略实体时执行者退出 | **每个**实体都必须在某个队伍里才算成功；允许部分成功；wiki 注明 1.12.2 的 bug MC-106681（多实体时第一个必须成功） |
| `scoreboard teams list [teamname]` | 列出全部队伍（显示名 + 成员数），或某个队伍的成员 | 无参数时至少要有一个队伍；给 teamname 时它必须存在且有成员 |
| `scoreboard teams option <team> <key> <value>` | 改队伍选项 | team 存在且取值合法 |

### 1.3 `option` 的键（1.12.2）

| 键 | 取值 | 默认 |
|---|---|---|
| `color` | 16 个颜色名（`black`…`white`）或 `reset` | 无 |
| `friendlyfire` | `true` / `false` | true |
| `seeFriendlyInvisibles` | `true` / `false` | **true** |
| `nametagVisibility` | `never` / `hideForOtherTeams` / `hideForOwnTeam` / `always` | always |
| `deathMessageVisibility` | 同上四值 | always |
| `collisionRule` | `always` / `never` / `pushOwnTeam` / `pushOtherTeams` | always |

### 1.4 显示槽位

`/scoreboard objectives setdisplay <slot> [objective]` 的槽位在 1.12.2 是
`list`、`sidebar`、`belowName` 与 16 个 `sidebar.team.<color>`；**objective 可省略，省略即清除该槽位**。

### 1.5 `scoreboard.dat`（NBT）

`data.Teams[]` 的字段：`AllowFriendlyFire`（**TAG_Byte**）、`SeeFriendlyInvisibles`（**TAG_Byte**）、
`NameTagVisibility`、`DeathMessageVisibility`、`CollisionRule`、`DisplayName`、`Name`、`Prefix`、
`Suffix`、`TeamColor`（无颜色时不写）、`Players`（字符串列表）。
`data.DisplaySlots` 用 `slot_0`…`slot_18` 表示 19 个槽位，空槽位**不写该标签**。

## 2. 开工时的现状（在 HEAD 实测）

| # | 现状 | 结论 |
|---|---|---|
| 1 | Core `/scoreboard` 只有 `objectives` 与 `players`，没有 `teams` | 本支补上 |
| 2 | C++ 队伍 API 存在但**不完整**：`RegisterTeam`/`RemoveTeam`/`GetTeam`/`ForEachTeam`、`cTeam:AddPlayer`/`RemovePlayer`/`HasPlayer`/`Reset`/`GetNumPlayers`/前后缀/friendlyfire/seeFriendlyInvisibles（[Scoreboard.h:125-192](../../src/Scoreboard.h#L125)） | 够做命令层；**缺成员枚举**，`list <team>` 只能报人数 |
| 3 | `cTeam` **没有** color / nametagVisibility / deathMessageVisibility / collisionRule 字段 | 这 4 个 option 键无法存储（§5-1） |
| 4 | `src/Protocol/` 里**没有任何 Teams 包**（`grep -rl Team src/Protocol/` 零命中） | 队伍数据不下发客户端（§5-2） |
| 5 | 显示槽位只有 `dsList`/`dsSidebar`/`dsName` 三个（[Scoreboard.h:203-210](../../src/Scoreboard.h#L203)） | `sidebar.team.*` 无法实现（§5-5） |
| 6 | `Slots` 里把 `"count"` 映射到 `cScoreboard.dsCount`（`dsCount` 是枚举哨兵，不是槽位） | 本支移除：`setdisplay count <obj>` 原来会 ASSERT（Debug）或数组越界写（Release）；同一分支顺带消除了「未知目标 → Lua 错误」与「缺槽位 → 对 nil 取 `.lower()`」两条崩溃路径 |
| 7 | 队伍落盘：`ScoreboardSerializer` 存了 Name/DisplayName/Prefix/Suffix/Players 与两个布尔 | **两个布尔存的是 TAG_Byte，读的却要求 TAG_Int** → 每次重启都丢（§3.2 前置修复） |
| 8 | friendlyfire 有应用点（[Player.cpp:2906](../../src/Entities/Player.cpp#L2906)）；seeFriendlyInvisibles 只有在有 Teams 包时才由客户端渲染 | friendlyfire 存了就有行为；seeFriendlyInvisibles 目前只是状态（§5-2） |

## 3. 本支改动

### 3.1 Core（`rrtt217/Core`，纯 Lua）

- 新增 [command_scoreboard.lua](../../Server/Plugins/Core/command_scoreboard.lua)：纯函数——名字/显示名长度校验
  （16 / 32）、剩余 token 拼接、`true`/`false` 解析、option 键值解析（含“能识别但本机不支持”的四个键）、
  目标 token 收集；不碰任何游戏对象。
- [cmd_scoreboard.lua](../../Server/Plugins/Core/cmd_scoreboard.lua#L390) 新增 `HandleScoreboardTeamsCommand`
  与 console 包装 `HandleConsoleScoreboardTeams`：
  - `add`：先查重，再 `RegisterTeam(name, displayName, "", "")`，并按 1.12.2 默认值把
    `seeFriendlyInvisibles` 置 true（C++ 构造函数默认 false，是引擎侧既有偏差）；
  - `remove` / `empty`：先 `ClearOnlineTeamMembers()`（[cmd_scoreboard.lua:326](../../Server/Plugins/Core/cmd_scoreboard.lua#L326)）
    用 `cPlayer:SetTeam(nil)` 清掉同世界在线玩家的原始 `cTeam*` 指针，再 `Reset()`/`RemoveTeam()`
    （否则指针悬空，`Player:GetTeam()` 会把已释放对象交给 Lua）；
  - `join` / `leave`：目标 token 依次解析（[cmd_scoreboard.lua:351](../../Server/Plugins/Core/cmd_scoreboard.lua#L351)）：
    `@…` 走 C1 选择器层（仅来源世界）、普通名字按“可离线/可不存在”处理、`*` 明确拒绝（无 tracked-entity 列表）；
    在线玩家用 `SetTeam()`（同时维护旧队成员表与新队成员表），离线名字直接改 roster；
    省略目标时由执行者（玩家或命令方块 origin 的玩家）执行；`leave` 按 1.12.2 允许部分成功；
  - `list` / `list <team>`：先收集行、再发头，保证玩家侧“头在行前”；
  - `option`：只接受 `friendlyfire` 与 `seeFriendlyInvisibles`；其余四个键按名字拒绝并说明本机不支持；
    未知键回显 1.12.2 的六个键名。
- `Slots` 改为 `{Slot, Name}` 三槽表（[cmd_scoreboard.lua:22](../../Server/Plugins/Core/cmd_scoreboard.lua#L22)），
  移除 `count` 哨兵；`setdisplay` 对未知槽位、`sidebar.team.*`、未知目标分别给出明确失败
  （[cmd_scoreboard.lua:80](../../Server/Plugins/Core/cmd_scoreboard.lua#L80)）；成功文案用规范化槽名。
- [Info.lua](../../Server/Plugins/Core/Info.lua#L560) 注册 in-game 与 console 两套 `teams` 子命令
  （权限 `core.scoreboard.teams`，7 条参数组合）。
- 测试：[tests/test_scoreboard_teams.lua](../../Server/Plugins/Core/tests/test_scoreboard_teams.lua)（新，144 条断言）、
  [tests/FuzzCommands.lua](../../Server/Plugins/Core/tests/FuzzCommands.lua) 增列 teams 相关 token。

### 3.2 `src/`（前置修复，独立原子提交）

**为什么必须一起做**：`option` 写入的 friendlyfire / seeFriendlyInvisibles 要能跨重启保留，否则命令层的状态是假的。
[ScoreboardSerializer.cpp:283-297](../../src/WorldStorage/ScoreboardSerializer.cpp#L283) 写的时候是 `AddByte`
（[同文件 125-126](../../src/WorldStorage/ScoreboardSerializer.cpp#L125)，与 vanilla 的 TAG_Byte 一致），
读的时候却要求 `TAG_Int`——两个标签永远读不出来，重启后 friendlyfire 恒为 true、seeFriendlyInvisibles 恒为 false。
本支把两处判断改成 `TAG_Byte` + `GetByte()`。没有新增/导出 C++ API。

## 4. 验证

### 4.1 静态

- `cd src && lua CheckBasicStyle.lua` → **0 violations**。
- `luac -p`：Core 全部 `*.lua` 与 `tests/*.lua` 通过。
- `luacheck`（`build/Server/.luacheckrc`）：`command_scoreboard.lua` **0 warnings**；
  `cmd_scoreboard.lua` 28 条（基线 27，新增的 1 条是 `HandleConsoleScoreboardTeams` 的
  “unused global”，与文件里既有两个 console handler 同型）；`Info.lua` 263 条（基线 261，
  新增 2 条是新 handler 的 W113，与该文件既有 127+ 条同型）。

### 4.2 单元 / 集成（纯 Lua）

`lua tests/test_scoreboard_teams.lua` → **144 passed, 0 failed**。覆盖：

- 纯函数：16/32 长度边界、显示名默认与拼接、`true`/`false`（含大小写拒绝）、option 四个不支持键与未知键、
  目标 token 收集；
- handler：`add`（重复/超长/多词显示名）、`join`（在线玩家/离线名字/`@` 选择器/无目标=执行者/
  console 无执行者/`*` 拒绝/无匹配选择器/跨世界目标）、`leave`（在线/离线/不在队伍/部分成功/无目标=执行者）、
  `empty`、`remove`、`list`（空板/头部顺序/单队/空队/未知队/多余参数）、`option`（两个支持的键、
  四个不支持的键、未知键、非法布尔、缺值、多余参数）、未知子命令；
- `objectives setdisplay`：`count` 不再到达 `SetDisplay`、`sidebar.team.*` 明确拒绝、未知目标不再崩、
  缺槽位不再崩、`belowName` 映射到 `dsName`、正常路径写入正确的 `dsList`。

Core 既有 17 套 `tests/test_*.lua` 同时全绿（断言数与基线一致，无回归）。

### 4.3 构建 / ctest

- `cmake --build build` → 通过（`ScoreboardSerializer.cpp` 所在 unity 单元重编 + 链接）。
- `cd build && ctest --output-on-failure -E "UrlClient-test|Google-test"` → **49/49 passed**（30.07 s；
  排除项按 [AGENTS.local.md §3](../../AGENTS.local.md)）。

### 4.4 实机（真实服务器 + mineflayer bot；本支内实测）

服务端起自 `build/Server`（本仓库二进制），Core 是本支代码。

| 用例 | 结果 |
|---|---|
| console `scoreboard teams add red Red Team` | `Team "red" has been created` |
| console `scoreboard teams list` | `DisplayName -> Name : Members` + `Red Team -> red: 0 member(s)` |
| console `scoreboard teams join red TestBot` | `Team "red": added 1 member(s)`；随后 `execute_lua` 读到 `player TestBot team=red` |
| console `scoreboard teams option red friendlyfire false` | 成功，状态读到 `ff=false` |
| console `scoreboard teams option red color blue` | `Team option "color" is not supported by this server yet` |
| console `scoreboard teams option red bogus x` | `Unknown team option "bogus". Valid options are: color, friendlyfire, …` |
| console `scoreboard teams join blue TestBot` | `Team "blue" does not exist` |
| player `/scoreboard teams add pteam Player Team` / `join pteam`（省略目标） | 成功；`list` 显示 `Player Team -> pteam: 1 member(s)`，且头部先于各行 |
| console `scoreboard teams empty red2`（TestBot 在队内） | 成功；随后 `execute_lua` 读到 `team=nil`（指针被清） |
| console `scoreboard teams remove red3`（TestBot 在队内） | 成功；随后 `teamIsNil=true` 且进程存活（无悬空指针） |
| player `/scoreboard teams leave`（不在队伍） | `"TestBot" is not on a team` |
| console `scoreboard teams join red3 @a` | 选择器命中 1 个实体并加入 |
| console `scoreboard objectives setdisplay count t1` | `Unknown display slot "count". Valid slots are: list, sidebar, belowName`（修复前 Debug ASSERT / Release 越界） |
| console `scoreboard objectives setdisplay sidebar.team.red t1` | `The sidebar.team.* display slots are not supported by this server yet` |
| console `scoreboard objectives setdisplay list nope` | `Unknown objective "nope"`（修复前 Lua 错误） |
| **重启往返** | 停服后 `world/data/scoreboard.dat` 中 `AllowFriendlyFire`/`SeeFriendlyInvisibles` 为 **TAG_Byte**（值 1/1 与 0/0）；重启后 `pteam ff=true seeInv=true`、`red ff=false seeInv=false`，显示名与成员（TestBot）一并恢复 |

### 4.5 验收门（按 [AGENTS.md §4.1](../../AGENTS.md)）

| 门 | 结果 |
|---|---|
| 1 风格 | ✅ `CheckBasicStyle.lua` 0 violations |
| 2 编译 | ✅ `cmake --build build` 通过 |
| 3 ctest | ✅ 49/49（排除 `UrlClient-test` / `Google-test`） |
| 4 绑定依赖 | 不适用（未改 `src/Bindings/`） |
| 5 行为规格 | ✅ 本文件 + §4.2/§4.4 |
| 6 APIDump | 不适用（未新增/导出 C++ API） |

## 5. 已知偏差（本支**不做**的，逐条给理由）

1. **四个 option 键**（`color`、`nametagVisibility`、`deathMessageVisibility`、`collisionRule`）：
   `cTeam` 无对应字段。按 [AGENTS.md §3(1)](../../AGENTS.md)，它们属于独立的前置分支（字段 + NBT + 与 Teams
   包一起设计），本支不私加字段；命令按名字拒绝并给出明确原因，不假装成功。
2. **没有 Teams 包**：`src/Protocol/` 无该包 → 队伍不下发客户端。因此**名牌、侧边栏、Tab 列表里的前后缀/颜色、
   friendlyfire 之外的可见性规则**都不会有 1.12.2 的观感；`friendlyfire` 是唯一有服务端应用点的选项。
   后续分支需要：0x44 Teams 包（5 种 mode）+ `cClientHandle` 下发 + 队伍变更广播 + 入服 `SendTo`。
3. **`teams list <team>` 只报人数**：`cTeam` 只有 `HasPlayer`/`GetNumPlayers`，没有成员枚举 API；
   若要列成员名，需要给 `cTeam` 加一个只读枚举（小改动，但属 `src/` API 变更，留给 Teams 包那支一起做）。
4. **`*` 目标**：vanilla 展开为“记分板跟踪的全部实体”；该集合在 Lua 侧不可枚举（`cObjective` 没有分数键枚举）。
   本支明确拒绝，不近似成在线玩家。
5. **显示槽位只有 3/19**：`sidebar.team.<color>` 需要 `cScoreboard::eDisplaySlot` 扩到 19 个 + 协议槽位字节 + NBT
   `slot_3..18`，属 C++ 分支；本支只保留 list/sidebar/belowName，并对 `sidebar.team.*` 明确报错。
6. **`objectives setdisplay` 省略 objective 的“清除槽位”**：1.12.2 允许（§1.4），本支未改（`objectives` 的既有行为
   不属 C6 的改动面）；当前仍回用法串。
7. **队伍按世界隔离**：Cuberite 每个世界一个记分板（既有偏差，vanilla 全服一个）。命令在发送者的世界执行、console 用
   默认世界；跨世界的在线目标被明确拒绝，选择器只在来源世界解析。
8. **非玩家实体**：`cTeam` 按字符串存成员，本支用 `GetTargetName()`（自定义名或生物类型名）；vanilla 用实体键。
9. **文案**：原版 1.12.2 的命令反馈逐字文本无法用允许来源核实（wiki 只给成功条件），本支沿用 Core 既有句式
   （`Team "x" has been created` 一类）——**〔待核〕：用原版 1.12.2 客户端 + 服务端实测回包**。
10. **`true`/`false` 大小写**：只接受 wiki 写出的两种拼写；1.12.2 解析器是否忽略大小写未核实——**〔待核〕**。
11. **显示名拼接**：多词显示名按单空格连接（C2 的引号切分让 `"Red Team"` 也成为一个 token）；vanilla 的贪婪字符串
    对连续空格的保留方式未核实——**〔待核〕**。
12. **序列化器没有单元测试**：仓库没有 world fixture（`cScoreboard` 的 `SetDisplay` 会解引用 `cWorld`，null world
    不可用），本支以“停服→NBT 字节检查→重启读回”的实机往返覆盖（§4.4）。

## 6. 不在本支范围但已顺手记录的缺陷

- `/scoreboard players list <player>` 仍引用未定义的 `GlobalPlayer`（[cmd_scoreboard.lua:145](../../Server/Plugins/Core/cmd_scoreboard.lua#L145)），
  由后续分支处置；
- `/scoreboard objectives add` 的缺参路径仍可能对 nil 取 `.lower()`（本支未改，保持原状）。
