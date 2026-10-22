# 记分板队伍下发（Teams 包）与队伍字段

分支：`feature/protocol-scoreboard-team-packet` ｜ 级别：P2 ｜ 前置：**C6 `feature/core-scoreboard-teams`（尚未合并，本支叠在其上）**
｜ Core 配套分支：`feature/core-scoreboard-team-options` ｜ 状态：已实现（待评审）

## 0. 为什么需要这条分支

C6 的命令层只是**记账**：客户端根本不知道队伍存在，于是前后缀、队伍颜色、名牌可见性、
隐身友军渲染、按队伍颜色的侧边栏槽位全都不生效；六个 option 里只有 `friendlyfire`
有服务端应用点（[Player.cpp:2906](../../src/Entities/Player.cpp#L2906)）。
Teams 包是服务端把队伍告诉客户端的**唯一通道**，六种 option 里有五种靠它送达客户端
（只有 `deathMessageVisibility` 不在包里，它是服务端聊天过滤）。

## 1. 协议规格（来源已核实）

来源：**wiki.vg 镜像** <https://wikivg.booky.dev/> 的 Protocol 页，按版本取修订
（整页 HTML 超过 `web_fetch` 的 100k 截断，故用镜像的 MediaWiki API 按节取 wikitext）：

| 版本 | 修订 | 时间 | Teams 节号 | 包 ID |
|---|---|---|---|---|
| 1.8（47） | [rev 994](https://wikivg.booky.dev/index.php?title=Protocol&oldid=994) | 2015-01-27 | 75 | 0x3E |
| 1.9（107） | [rev 1157](https://wikivg.booky.dev/index.php?title=Protocol&oldid=1157) | 2016-03-11 | — | 0x41 |
| 1.10（210） | [rev 1215](https://wikivg.booky.dev/index.php?title=Protocol&oldid=1215) | 2016-06-29 | — | 0x41 |
| 1.11（315/316） | [rev 1244](https://wikivg.booky.dev/index.php?title=Protocol&oldid=1244) | 2016-11-27 | — | 0x41 |
| 1.12（335） | [rev 1271](https://wikivg.booky.dev/index.php?title=Protocol&oldid=1271) | 2017-06-12 | — | 0x43 |
| 1.12.1（338） | [rev 1298](https://wikivg.booky.dev/index.php?title=Protocol&oldid=1298) | 2017-08-08 | — | 0x44 |
| **1.12.2（340）** | [rev 1306](https://wikivg.booky.dev/index.php?title=Protocol&oldid=1306) | 2017-09-18（注释 “1.12.2”） | 82 | **0x44** |

### 1.1 包定义（1.12.2）

```
0x44  Play  Clientbound  Teams
```

| 字段 | 类型 | 说明 |
|---|---|---|
| Team Name | String(16) | 队伍内部名，与记分板共用 |
| Mode | Byte | 0 create / 1 remove / 2 update info / 3 add players / 4 remove players |
| **Mode 0** | | Display Name String(32)；Prefix String(16)；Suffix String(16)；Friendly Flags Byte；Name Tag Visibility String Enum(32)；Collision Rule String Enum(32)；Color Byte；Entity Count VarInt；Entities Array<String(40)> |
| **Mode 1** | | 无字段 |
| **Mode 2** | | 同 Mode 0，但没有 Entity Count / Entities |
| **Mode 3 / 4** | | Entity Count VarInt；Entities Array<String(40)> |

- **Friendly Flags**：位掩码，`0x01` = 允许友伤，`0x02` = 能看见同队隐身成员。
- **Name Tag Visibility**：`always` / `hideForOtherTeams` / `hideForOwnTeam` / `never`。
- **Collision Rule**：`always` / `pushOtherTeams` / `pushOwnTeam` / `never`。
- **Color**：`0–15` 为 Chat 颜色索引，**`-1` = reset/无颜色**。
- **Entities**：玩家用用户名，其他实体用 **UUID 字符串**（≤40）。
- **Display Name / Prefix / Suffix 是普通字符串，不是 JSON 文本组件**（1.12.2 期文档如此；
  1.13 期的 rev 1334 仍是 String），所以这条包**不需要**文本组件层。

### 1.2 各版本差异

| 版本 | Teams 包 ID | 与 1.12.2 的差异 |
|---|---|---|
| 1.8 | 0x3E | **没有 Collision Rule 字段**；其余字段顺序相同（Display Name / Prefix / Suffix / Friendly Fire / Name Tag Visibility / Color / Player Count / Players） |
| 1.9 | 0x41 | 增加 Collision Rule；Friendly Flags 为 0x01/0x02 位掩码 |
| 1.10 | 0x41 | 同 1.9 |
| 1.11 | 0x41 | 同 1.9 |
| 1.12 | 0x43 | 同 1.9 |
| 1.12.1 | 0x44 | 同 1.9 |
| 1.12.2 | 0x44 | 基准 |
| 1.13 / 1.14 | — | **冻结**：本支不下发（见 §6-1） |

- 1.8 的 “Friendly Fire” 字段在 wiki.vg 里写作 “0 for off, 1 for on, 3 for seeing friendly
  invisibles”；本支 1.8 路径按同一位置的位掩码写 0x01/0x02 —— **〔待核〕**：1.8 客户端对
  0x02 位的实际解释未实测，核法是用 1.8 客户端连接并观察隐身队友渲染。

### 1.3 选项语义（Minecraft Wiki “Scoreboard” rev 1156283）

| option | 包里携带 | 生效侧 | 说明 |
|---|---|---|---|
| `color` | Color（Byte，-1 = reset） | 客户端渲染 | 名字颜色（聊天/头顶/Tab/侧栏）+ 发光轮廓色 |
| `friendlyfire` | Friendly Flags 0x01 | **服务端**（+ 客户端） | 允许队友互相伤害 |
| `seeFriendlyInvisibles` | Friendly Flags 0x02 | 客户端渲染 | 同队隐身成员显示为半透明 |
| `nametagVisibility` | Name Tag Visibility（字符串） | 客户端渲染 | 名牌可见性 |
| `collisionRule` | Collision Rule（字符串） | 服务端碰撞（+ 客户端预测） | 推挤规则 |
| `deathMessageVisibility` | **不携带** | 服务端聊天过滤 | 本支只存储/持久化，行为见 §6-4 |

前后缀按 wiki 描述会加到“聊天、玩家列表、侧边栏、头顶名牌”里的名字前后；其中哪些由
服务端提前拼好、哪些由客户端渲染未核实 —— **〔待核〕**（§6-10）。

### 1.4 NBT（`scoreboard.dat`，1.12.2）

`data.Teams[]` 复合标签：`AllowFriendlyFire`（byte）、`SeeFriendlyInvisibles`（byte）、
`NameTagVisibility`（string）、`DeathMessageVisibility`（string）、`CollisionRule`（string）、
`DisplayName` / `Name` / `Prefix` / `Suffix`（string）、`TeamColor`（string，**无颜色时不写**）、
`Players`（list<string>）。

## 2. 开工时现状（在 HEAD 实测）

| # | 现状 | 结论 |
|---|---|---|
| 1 | `src/Protocol/` 里没有任何 Team 包（`grep -rl Team src/Protocol/` 零命中） | 本支新增 |
| 2 | `cTeam` 只有 Name / DisplayName / Prefix / Suffix / 两个 bool / 成员名集合（[Scoreboard.h:125-192](../../src/Scoreboard.h#L125)） | 本支补 Color / NameTagVisibility / CollisionRule / DeathMessageVisibility |
| 3 | `cTeam` 不持有世界指针，改字段不发任何包；`SetDisplayName` 只有 `TODO: Update clients` | 本支接上广播 |
| 4 | `cScoreboard::SendTo()` 只发 objective + display slot（[Scoreboard.cpp:567](../../src/Scoreboard.cpp#L567)） | 本支补队伍全量下发 + 切世界清理 |
| 5 | Core 的 `option` 只接受 `friendlyfire` / `seeFriendlyInvisibles`（C6） | 本支开放 `color` / `nametagVisibility` / `collisionRule` |
| 6 | 序列化器只存 AllowFriendlyFire / SeeFriendlyInvisibles / 名字 / 前后缀 / Players | 本支补四个键 |

## 3. 本支范围

**做**：

1. `cTeam` 增四个字段（Color / NameTagVisibility / CollisionRule / DeathMessageVisibility）
   + getter/setter；C++ 侧成员枚举 `GetPlayers()`（只给 C++ 用，不入 Lua）；
2. `cTeam` 持有 `cWorld*`，所有变更广播：create=0、remove=1、update=2、join=3、leave=4、reset=4；
3. `cScoreboard::RegisterTeam` / `RemoveTeam` 广播，`SendTo` 全量下发，新增
   `SendTeamRemovals()`；
4. `cProtocol::SendTeam(const cTeam &, Byte a_Mode, const AStringVector & a_Players)`：
   1.8 布局（0x3E，无 Collision Rule）+ 1.9 布局（1.9–1.12.2，ID 0x41/0x43/0x44）+
   1.13 的冻结 no-op；
5. `cClientHandle::SendTeam`、`cWorld::BroadcastTeam`；
6. `cPlayer::OnRemoveFromWorld` 里清掉旧世界队伍（客户端记分板是全局的，服务端是每世界一个）；
7. 序列化器补 `NameTagVisibility` / `CollisionRule` / `DeathMessageVisibility` / `TeamColor`；
8. Core：开放三个 option 键并做取值校验；`deathMessageVisibility` 继续拒绝；
9. Core 测试更新；APIDump 同步。

**不做**（逐条见 §6）：`deathMessageVisibility` 的聊天过滤、`sidebar.team.<color>` 槽位、
`teams list <team>` 的成员名、`*` 目标、collisionRule 的服务端碰撞、1.13/1.14 下发、
聊天/Tab 前后缀的服务端拼接归属（〔待核〕）。

## 4. 实现要点

- [Scoreboard.h](../../src/Scoreboard.h) / [Scoreboard.cpp](../../src/Scoreboard.cpp)：
  字段与默认值（`always` / `always` / `always`、颜色空串 = 无颜色）、广播、
  `cScoreboard::SendTo`（另取 `m_CSTeams` 锁）、`SendTeamRemovals`。
- [Protocol.h](../../src/Protocol/Protocol.h)：`pktTeam` + 纯虚 `SendTeam`。
- [Protocol_1_8.cpp](../../src/Protocol/Protocol_1_8.cpp)：1.8 布局与 0x3E；
  [Protocol_1_9.cpp](../../src/Protocol/Protocol_1_9.cpp)：1.9 布局与 0x41；
  [Protocol_1_12.cpp](../../src/Protocol/Protocol_1_12.cpp)：0x43 / 0x44；
  [Protocol_1_13.h](../../src/Protocol/Protocol_1_13.h)：冻结 no-op（否则 `GetPacketID` 的
  default 分支会 ASSERT 并返回 0）。
- [ClientHandle.cpp](../../src/ClientHandle.cpp)：`SendTeam` 转发；
  [Broadcaster.cpp](../../src/Broadcaster.cpp)：`cWorld::BroadcastTeam`。
- [Player.cpp](../../src/Entities/Player.cpp)：`OnRemoveFromWorld` 里对旧世界调
  `SendTeamRemovals`（仅世界切换、非断线）。
- [ScoreboardSerializer.cpp](../../src/WorldStorage/ScoreboardSerializer.cpp)：四个新键。
- Core：[command_scoreboard.lua](../../Server/Plugins/Core/command_scoreboard.lua) 的取值表与校验、
  [cmd_scoreboard.lua](../../Server/Plugins/Core/cmd_scoreboard.lua) 的 option 分发、
  [tests/test_scoreboard_teams.lua](../../Server/Plugins/Core/tests/test_scoreboard_teams.lua)。
- APIDump：[APIDesc.lua](../../Server/Plugins/APIDump/APIDesc.lua) 的 `cTeam` 新方法。

## 5. 验证

### 5.1 静态

- `cd src && lua CheckBasicStyle.lua` → **0 violations**。
- `luac -p`：Core 全部 `*.lua` 与 `tests/*.lua` 通过。
- `luacheck`（`build/Server/.luacheckrc`）：`command_scoreboard.lua` **0 warnings**；
  `cmd_scoreboard.lua` 28 条、`Info.lua` 263 条，与本支之前持平（无新增类别）。

### 5.2 构建 / ctest

- `cmake --build build` → 通过（82 步，含 tolua 绑定重新生成）。
- `cd build && ctest --output-on-failure -E "UrlClient-test|Google-test"` → **49/49 passed**（29.08 s）。

### 5.3 Core 单元/集成

`lua tests/test_scoreboard_teams.lua` → **170 passed, 0 failed**（C6 的 144 条 + 本支新增 26 条）：
color 名称 / `reset`、两个规则键的合法值与拒绝、`deathMessageVisibility` 仍被拒绝且不改状态。

### 5.4 实机（真实服务器 + mineflayer；在原始包层解码）

服务端起自 `build/Server`；用一个独立 mineflayer 客户端（`TestBot`，有权限）在聊天里驱动命令，
并在 `bot._client.on('packet')` 层记录 `teams` 原始包。下表的值即服务端实际写出的字节。

| 动作 | 客户端收到的原始包 |
|---|---|
| `teams add persist Persist Team` | `mode 0`：name=Persist Team，prefix/suffix 空，friendlyFire=1，nameTagVisibility=always，collisionRule=always，**color=-1**，players=[]（随后 C6 的 seeFriendlyInvisibles=true 触发一条 `mode 2`：friendlyFire=3） |
| `option persist color dark_red` | `mode 2`：**color=4**（dark_red 的颜色索引） |
| `option persist nametagVisibility hideForOtherTeams` | `mode 2`：nameTagVisibility=hideForOtherTeams |
| `option persist collisionRule pushOtherTeams` | `mode 2`：collisionRule=pushOtherTeams |
| `option persist friendlyfire false` | `mode 2`：friendlyFire=2（0x02 = 只剩“看见隐身队友”） |
| `option persist seeFriendlyInvisibles false` | `mode 2`：friendlyFire=0 |
| `teams join persist`（自己） | `mode 3`：players=["TestBot"] |
| `teams leave`（自己） | `mode 4`：players=["TestBot"] |
| `teams remove persist` | `mode 1` |
| `/portal world`（切世界） | 旧世界队伍先由 `SendTeamRemovals` 清空（客户端 `bot.teams` 变为 `{}`），进入新世界后再收该世界的队伍 |

**重启往返**（停服 → 检查 NBT → 起服）：

- 停服后 `world_nether/data/scoreboard.dat` 的 `persist`：`TeamColor=dark_red`、
  `NameTagVisibility=hideForOtherTeams`、`CollisionRule=pushOtherTeams`、`DeathMessageVisibility=always`、
  `AllowFriendlyFire=0`、`SeeFriendlyInvisibles=0`；`world/data/scoreboard.dat` 的 `red`：
  `TeamColor=blue`、`never`、`pushOwnTeam`、`0`、`1`。
- 起服后新客户端连接即收到 **`mode 0`**：`persist` 的 color=4、hideForOtherTeams、pushOtherTeams、
  friendlyFire=0；回到默认世界后收到 `red` 的 color=9（blue）、friendlyFire=2、never、pushOwnTeam
  —— 全部从 NBT 恢复。

> **探针库的版本差异（不是服务端问题）**：mineflayer 的 `bot.teams[x].color` 在 1.12.2 上恒为
> `"reset"`，因为它的 team 插件按 1.13+ 的字段名 `formatting` 取值，而 minecraft-data 的 1.12.2
> `packet_teams` schema 里该字段叫 `color`。上表的颜色值取自原始包，所以不受影响。

## 6. 已知偏差

1. **1.13 / 1.14 不下发队伍**：两版协议类冻结（[AGENTS.md §9](../../AGENTS.md)），
   `SendTeam` 覆盖为空操作，避免把 1.12 布局按错误的包号发出去（`GetPacketID` 的
   default 分支会 ASSERT 并返回 0）。代价是这两版客户端仍看不到队伍（与今天相同）。
2. **每世界记分板 vs 客户端全局**：本支在切世界时清掉旧世界队伍，但**旧世界的
   objective/分数仍会留在客户端**——那是既有偏差，不在本支范围。
3. **非玩家实体**：包要求其他实体用 UUID 字符串，而 `cTeam` 用名字存、Core 目前把
   `@e` 命中的非玩家实体按显示名入 roster（C6 §5-8），所以非玩家成员的队伍数据可能
   不被客户端识别。
4. **`deathMessageVisibility` 只存储/持久化**：它的生效侧是服务端聊天过滤（不在包里），
   本支实现字段与 NBT，Core 仍拒绝该 option 键；聊天过滤留给后续分支。
5. **`sidebar.team.<color>` 槽位未实现（已确认是 1.12.2 的真实缺口）**：Minecraft Wiki 的
   “Java Edition 14w07a” 页面写明 *Different teams can now have different objectives displayed in
   the sidebar*，例子就是 `scoreboard objectives set display sidebar.team.yellow objectiveName`
   —— 该特性从 **1.8（14w07a）** 就存在，1.12.2 当然也有；wiki.vg 340 期把 DisplayScoreboard 的
   Position 写成只有 0–2 是**文档滞后**。本支没做是因为它需要把 `cScoreboard::eDisplaySlot` 扩到
   19 个 + NBT `slot_3..18` + Core 的槽位表，与 Teams 包本身无关，适合单独一支。
6. `teams list <team>` 仍只报人数（Lua 没有成员枚举；C++ 的 `GetPlayers()` 未导出）。
7. `*` 目标仍被拒绝（无 tracked-entity 列表）。
8. `collisionRule` 只下发，服务端推挤逻辑未实现。
9. **1.8 客户端路径未实机验证**：0x3E + 无 Collision Rule 的布局按 rev 994 实现，
   但本机只有 1.12.2 客户端协议库可用 —— **〔待核〕**：用 1.8 客户端连接并观察前后缀/颜色。
10. **聊天/Tab 前后缀的归属未核实** —— **〔待核〕**：wiki 只描述结果，未说明是服务端
    提前拼好还是客户端渲染；核法是用原版抓包或实机对比。

## 7. 验收门（按 [AGENTS.md §4.1](../../AGENTS.md)）

| 门 | 结果 |
|---|---|
| 1 风格 | ✅ `CheckBasicStyle.lua` 0 violations |
| 2 编译 | ✅ `cmake --build build` 通过（82 步，含 tolua 绑定重新生成） |
| 3 ctest | ✅ 49/49（排除 `UrlClient-test` / `Google-test`） |
| 4 绑定依赖 | 不适用（未改 `src/Bindings/` 手写部分与 `.pkg`；生成物随构建更新） |
| 5 行为规格 | ✅ 本文件 + §5 |
| 6 APIDump | 本支新增的 8 个 `cTeam` 方法**不在** `NewlyUndocumented.lua` 里（文档完整）；自检仍报 **6 个既有**未文档化符号（`cEnderCrystal.HasRespawnBeam`、`cEnderCrystal.SetRespawnBeam`、`cEntity.IsAreaEffectCloud`、`cEntity.etAreaEffectCloud`、`cProjectileEntity.pkDragonFireball`、`cProjectileEntity.pkEnderEye`），与 C2 记录、基线 `master` 相同 → 既有偏差，非本支引入 |
