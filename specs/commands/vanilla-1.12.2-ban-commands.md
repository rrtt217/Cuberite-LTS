# 纯 Lua 命令族（二）：`/ban-ip`、`/banlist`、`/pardon`、`/pardon-ip`

分支：`feature/core-ban-commands` ｜ 级别：P2 ｜ 前置：无（不消费选择器，纯 Core）｜
编排见 [roadmap §4.2](vanilla-1.12.2-command-layer-roadmap.md)

**为什么归为一支**：四条命令共用同一份封禁数据库与同一套「地址/玩家名 → 封禁条目」解析
（[banlist.lua](../../Server/Plugins/Core/banlist.lua) 的 `AddIPToBanlist` / `RemoveIPFromBanlist` /
`ListBannedIPs` / `ListBannedPlayers` 等局部助手），且都不需要新 C++ 通路。Core 已有 `/ban`、`/unban`
与**仅控制台**的 `banip` / `unbanip` / `banlist`，缺的正是 1.12.2 的另外四个名字与玩家侧入口。

## 1. 现状

| 命令 | Core 现状 | 所需 C++ API | 判定 |
|---|---|---|---|
| `/ban-ip` | 无；控制台有非 vanilla 名 `banip`（[banlist.lua:324](../../Server/Plugins/Core/banlist.lua#L324) `HandleConsoleBanIP`，只收 IP，不收在线玩家名） | 无（`cRoot:ForEachPlayer`、`cClientHandle:GetIPString/Kick` 均已导出） | **缺命令 + 缺名字解析** |
| `/banlist` | 仅控制台（[banlist.lua:359](../../Server/Plugins/Core/banlist.lua#L359)），可选参数、非 vanilla 文案 | 无 | **玩家侧缺失，文案/语法待对齐** |
| `/pardon` | 无；`/unban` 等价但非 vanilla 名（[banlist.lua:274](../../Server/Plugins/Core/banlist.lua#L274)） | 无 | **缺命令** |
| `/pardon-ip` | 无；控制台 `unbanip` 等价但非 vanilla 名 | 无 | **缺命令** |

`banlist.lua` 的数据库助手都是 `local`，所以新 handler 必须写在同一文件里（或先把助手导出——
后者会扩大改动面，本支不采纳）。

## 2. 行为规格

来源：[Commands/ban 的 1.12.2 期修订](https://minecraft.wiki/w/Commands/ban?oldid=1122910)
（rev 1122910，2017-07-08）与 [Commands/pardon 的 1.12.2 期修订](https://minecraft.wiki/w/Commands/pardon?oldid=1159196)
（rev 1159196，2017-10-13）。两页都早于首个 1.13 快照（17w43a，2017-10-25）。
两页的信息框都标 `mp`：这些命令**只在多人服务器**存在（单人存档里不存在），与专用服务器的实现一致。

### 2.1 `/ban-ip <address|name> [reason …]`

1. `address|name`：「An IP address or the name of a player who is currently online」。
2. 结果原文：「**Fails if address|name is neither a valid IP address nor the name of an online player.**
   On success, the IP address (of the online player, or the address specified) is added to the server
   blacklist, which prevents anyone from connecting to the server from that address.」
   → 玩家名解析**只认在线玩家**（离线玩家名无法反查 IP）。
3. `reason`（可选）：「Text to be displayed with the ban, to other operators and in the server logs.」
   省略时的默认值本支取 "Banned by an operator."〔待核：原版 1.12.2 的默认理由原文；
   核法＝原版专用服 `/ban-ip <ip>` 后读 `banned-ips.json`〕。
4. 成功后应把该 IP 上的在线玩家踢出（「prevents anyone from connecting」的即时效果）；
   踢出理由与封禁理由相同〔推测：依据是 vanilla 的封禁命令都会即时踢人，置信度高〕。
5. 权限等级 **3**〔待核：同 §2.5 第 1 条〕。

### 2.2 `/banlist <ips|players>`

1. 语法（同页）：`banlist ips` ｜ `banlist players`，**参数必填**。
2. 结果：「Always succeeds. Displays banned IP addresses or banned players.」
   → 空列表也是成功。
3. 输出文案（列表为空/非空、单复数、分隔符）本支按语义拟写（见 §4 第 1 条），
   **未与原版逐字比对**〔待核〕。
4. 权限等级 **3**〔待核〕。

### 2.3 `/pardon <name>`

1. 语法：`pardon <name>`，恰好一个参数。
2. 结果：「Fails if the arguments are not specified correctly. On success, **if the name … is on the
   banlist, it is removed**」→ 名字不在列表里**也算成功**（不报错）。
3. 权限等级 **3**〔待核〕。

### 2.4 `/pardon-ip <address>`

1. 语法：`pardon-ip <address>`，`address`「Must be a valid IP address.」
   → 地址格式非法是参数错误（失败），不是「不在列表」。
2. 结果：同 §2.3 的「在列表里才删除，否则也算成功」。
3. 权限等级 **3**〔待核〕。

### 2.5 两条共用事实

1. **权限等级**：1.12.2 的 ban 系命令是等级 3〔待核：本页修订的 infobox 没有可抓取的权限字段；
   核法＝原版 1.12.2 专用服上用等级 2 与等级 3 的玩家各执行一次〕。Core 用配置驱动的权限名，
   本支按既有惯例给 `core.ban-ip` / `core.banlist` / `core.pardon` / `core.pardon-ip`。
2. **IP 合法性**：1.12.2 用 Java 标准库的 `InetAddress` 解析（IPv4 与 IPv6 文本形式都收）。
   Cuberite 的 Lua 侧没有 IP 解析 API，本支自写校验：IPv4 严格四段十进制；IPv6 按
   「至多一个 `::`、1–4 位十六进制组、可带 IPv4 结尾」的文本规则。
   与 Java 的**完全一致**未核实〔待核：原版对 `01.2.3.4`、`::ffff:192.0.2.1`、带 zone id
   (`fe80::1%eth0`) 的接受度；核法＝原版专用服逐条 `/pardon-ip`〕。
3. **名字大小写**：玩家名在封禁库里按原样存储；`/ban-ip <name>` 的在线查找按大小写不敏感
   （vanilla 玩家名解析大小写不敏感）〔推测：依据是游戏内玩家名查找的通行行为，置信度高〕。
4. 1.12.2 之后：1.13 把这些命令的反馈改成可点击的组件（`/banlist` 输出可点击的解除链接），
   **不引入**——本支只做 1.12.2 的纯文本反馈。

## 3. 实现映射（Core，纯 Lua）

| 规格点 | 落点 |
|---|---|
| IP 校验、`ips\|players` 解析、列表文案 | 新 `command_ban.lua`：`IsValidIPAddress()`、`ParseBanlistKind()`、`FormatBanlist()`、`ParseBanIPArguments()`、`ParsePardonArguments()`、`ParsePardonIPArguments()` |
| `/ban-ip`：IP 直用 / 在线名反查、加库、踢人 | `banlist.lua` 新 handler：`GetOnlinePlayerIP()` + 复用 `AddIPToBanlist()`，踢人逻辑从既有 `HandleConsoleBanIP` 提取为 `BanIPAddress()` |
| `/banlist`、`/pardon`、`/pardon-ip` | `banlist.lua` 新 handler，直接调既有 `ListBannedPlayers/IPs`、`RemovePlayerFromBanlist`、`RemoveIPFromBanlist` |
| 命令注册 | `Info.lua`：`/ban-ip`、`/banlist`、`/pardon`、`/pardon-ip` 四条 in-game + 对应控制台名；控制台 `banlist` 改指新 handler（旧的无参/小写行为由新语法取代） |

控制台**保留**既有 `banip` / `unban` / `unbanip` 名字（向后兼容），新增 `ban-ip` / `pardon` / `pardon-ip`。

## 4. 已知偏差与未核实项

1. **反馈文案全部是拟写**：vanilla 1.12.2 的 `/ban-ip` 成功/失败文案、`/banlist` 的空列表与非空列表
   文案、`/pardon` 系文案，本支按语义拟写并与 Core 既有风格一致，**未与原版逐字比对**〔待核：
   原版 1.12.2 专用服逐条执行并抄录；核法＝控制台 + OP 玩家各一次〕。C5 已把「成功反馈文本与
   原版不同形」记为可接受偏差，本支同样处理。
2. **IPv6 校验是近似**：见 §2.5 第 2 条；本支不保证与 Java `InetAddress` 的接受集合逐字相同。
3. **`/ban-ip` 用玩家名时的 `BannedBy`**：记录执行者名字（控制台记 `<console>`），与既有
   `/ban` 一致；vanilla 的 `banned-ips.json` 里 source 字段格式未核实。
4. **控制台 `banlist` 行为变化**：旧实现无参数时列玩家、参数只认小写 `ips`；新实现要求
   `ips|players` 且输出 vanilla 形文案。既有脚本若依赖旧行为会受影响——这是本支唯一的有意行为变更，
   已在此记录；若要保留旧行为，需要维护者决定是否回退。
5. **`/ban` 与 `/unban` 不动**：它们是 Core 历史命令（非 vanilla 名），本支不把它们改成别名；
   是否合并留给维护者决策（与 server-state 支的 `/setspawn` 同型问题）。
6. **封禁的登录拦截**不在本支：`banlist.lua` 的 `IsIPBanned` / `IsPlayerBanned` 已被登录流程使用，
   本支只保证新命令写入同一张表，不重测登录路径〔待核：被 `/ban-ip` 封掉后真实客户端重连应被拒〕。

## 5. 测试与验收

### 5.1 纯 Lua 测试

`lua tests/test_ban_commands.lua` → **58 passed, 0 failed**。覆盖：

- IP 校验：13 个合法地址（IPv4 边界 0/255、`::`、`::1`、完整 8 组、内嵌 IPv4 的
  `::ffff:192.0.2.1` 与 `1:2:3:4:5:6:192.0.2.1`、7 组 + `::`）与 18 个非法形式
  （越界 octet、前导零、组数不足/过多、两个 `::`、`:::`、zone id、非十六进制、5 位组、
  IPv4 尾越界、8 组 + `::`）。
- `/banlist` 参数：`ips` / `players` 接受、`IPs` 拒绝（Brigadier 字面量大小写敏感）、缺参与未知参数报 Usage。
- `/banlist` 文案：两类空列表、两条条目、单条条目。
- `/ban-ip` 参数：缺参 Usage、默认理由 `Banned by an operator.`、多 token 理由按空格拼接。
- `/pardon` 参数：缺参/多参拒绝、正常解析。
- `/pardon-ip` 参数：缺参、非法地址（文案点名坏值）、多参拒绝、IPv6 接受。

### 5.2 静态检查（按 [AGENTS.md §4.1](../../AGENTS.md) 选门）

| 门 | 结果 |
|---|---|
| 1 风格 | 不适用（无 `src/*.cpp|h` 改动） |
| 2 编译 | 不适用（纯 Lua） |
| 3 ctest | 不适用（同上；纯 Lua 测试用 `lua` 直接跑） |
| 4 绑定依赖 | 不适用 |
| 5 行为规格 | 本文件 + §5.1/§5.3 |
| 6 APIDump | 不适用 |

- `luac -p`：Core 目录全部 Lua 文件通过。
- `luacheck`：`command_ban.lua` **0 warning / 0 error**；`banlist.lua` 从 23 条降到 **7 条**——
  新增的 `-- luacheck: ignore 131` 消掉了 16 条既有的「handler 名未定义/未使用」警告，
  剩余 7 条是**既有**类型（`HandleBanCommand` 里未使用的局部 `Reason`、引擎全局
  `KickPlayer`/`NewSQLiteDB`），本支新增代码零警告。
- `Info.lua`：新增 7 个 handler 名，警告数按既有 W113 同型增长（未新增类型）。

### 5.3 真实服务器探针（控制台路径，`build/Server`）

```
Executing console command: "banlist players"
There are no banned players
Executing console command: "banlist ips"
There are no banned IP addresses
Executing console command: "banlist foo"
Usage: /banlist <ips|players>
Executing console command: "ban-ip 203.0.113.7 test reason"
[Core] Banned IP 203.0.113.7, banned by <console>
Banned IP address 203.0.113.7
Executing console command: "banlist ips"
There are 1 banned IP addresses: 203.0.113.7
Executing console command: "ban-ip NoSuchPlayer"
"NoSuchPlayer" is not a valid IP address or an online player
Executing console command: "pardon-ip not-an-ip"
Invalid IP address "not-an-ip"
Executing console command: "pardon-ip 203.0.113.7"
[Core] Unbanned IP 203.0.113.7
Unbanned IP address 203.0.113.7
Executing console command: "banlist ips"
There are no banned IP addresses
Executing console command: "pardon NoSuchPlayer"
[Core] Unbanned NoSuchPlayer
Unbanned player NoSuchPlayer
```

`pardon NoSuchPlayer` 对不在列表里的名字仍成功，符合 §2.3「在列表里才删除，否则也算成功」。
探针数据全部在 `build/Server/banlist.sqlite`（运行期产物，不入库），最后已用 `pardon-ip` 清掉
临时条目。

### 5.4 未覆盖

- **在线玩家名反查**：本机无客户端，`/ban-ip <name>` 的「在线玩家 → IP」路径未实测；
  handler 级代码走 `cRoot:ForEachPlayer` + `cClientHandle:GetIPString`（与既有 `banip` 相同），
  但未在真实连接上验证〔待核：真实客户端连入后 `/ban-ip <该玩家名>`，应封其 IP 并踢出〕。
- **玩家侧反馈与权限闸门**：同 server-state 支，无客户端；权限名 `core.ban-ip` 等由配置决定〔待核〕。
- **登录拦截**：见 §4 第 6 条。
- **IPv6 封禁的端到端**：校验器接受 IPv6 文本，但未用真实 IPv6 客户端验证 `GetIPString()` 的
  输出能被同一校验器接受〔待核：IPv6 客户端连入后 `/ban-ip <其地址>` 并重连〕。
