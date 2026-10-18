# C5 表现类命令合并：`/title`、`/particle`、`/playsound`、`/stopsound`

分支：`feature/core-presentation-commands` ｜ 级别：P2 ｜ 前置：C1（选择器，用于 `<targets>`）｜ 编排见 [roadmap](vanilla-1.12.2-command-layer-roadmap.md)

**合并理由**：四条命令形状完全相同——「解析参数 → 校验 → 调一个已有 C++ API 广播」，无服务端状态、无存档、无协议新增；单条实现不足 150 行，各立一支会让评审成本翻四倍而收益为零。它们共享同一批触点（`Info.lua` 注册表 + 选择器调用 + 目标解析样板）。

## 1. 现状

| 命令 | Core | 所需 C++ API | 状态 |
|---|---|---|---|
| `/title` | 无 `cmd_title.lua` | `cClientHandle::SendSetTitle`（[ClientHandle.h:222](../../src/ClientHandle.h#L222)）、`SendTitleTimes`（[:232](../../src/ClientHandle.h#L232)）、`SendHideTitle`（[:193](../../src/ClientHandle.h#L193)，即协议 action 4）、`SendResetTitle`（[:216](../../src/ClientHandle.h#L216)，action 5），均已文档化（[APIDesc.lua:1857](../../Server/Plugins/APIDump/APIDesc.lua#L1857)、[:1952](../../Server/Plugins/APIDump/APIDesc.lua#L1952)、[:1786](../../Server/Plugins/APIDump/APIDesc.lua#L1786)、[:1820](../../Server/Plugins/APIDump/APIDesc.lua#L1820)）；`actionbar` 走 `cPlayer:SendAboveActionBarMessage`（[Player.h:298](../../src/Entities/Player.h#L298)，[APIDesc.lua:11045](../../Server/Plugins/APIDump/APIDesc.lua#L11045)） | **API 齐，缺命令** |
| `/particle` | 无 | `cWorld::BroadcastParticleEffect`（[World.h:182-183](../../src/World.h#L182)，两个重载：带/不带 `std::array<int,2>` 附加数据），已文档化 | **API 齐，缺命令**；`mode`/`<viewers>` 无对应能力，见 §2.2 |
| `/playsound` | 无 | `cClientHandle::SendSoundEffect`（[ClientHandle.h:224-225](../../src/ClientHandle.h#L224)）**已导出**（按目标逐个下发），`cWorld::BroadcastSoundEffect`（[World.h:194](../../src/World.h#L194)）用于广播 | **API 齐，缺命令**；`<source>` 无对应能力，见 §2.3 |
| `/stopsound` | 无 | **不需要新包**：1.12.2 用 **`MC|StopSound` 插件消息**实现（原版实测，§2.4），而 `cClientHandle:SendPluginMessage`（[ClientHandle.h:212](../../src/ClientHandle.h#L212)，[APIDesc.lua:1805](../../Server/Plugins/APIDump/APIDesc.lua#L1805)）已导出且有文档 | **API 齐，缺命令**；实测后并入本支（§2.4、§3） |
| `/tellraw` | 无 | `cPlayer::SendMessageRaw`（[Player.h:296](../../src/Entities/Player.h#L296)，把 JSON 组件原样写进 Chat 包）+ `cJson:Parse`（[LuaJson.cpp:272](../../src/Bindings/LuaJson.cpp#L272)），两者均已文档化（[APIDesc.lua:11117](../../Server/Plugins/APIDump/APIDesc.lua#L11117)、[:8422](../../Server/Plugins/APIDump/APIDesc.lua#L8422)） | **API 齐，缺命令**；2026-10 维护者决定随本支一并落地（见 §2.5） |

即：**这五条命令都不是「要新建架构」的缺口，而是 Core 少写了几个 `cmd_*.lua`（各自带 1–2 条记录在案的偏差）。**`/stopsound` 一度被判为「缺 C++ 通路」，2026-10 的原版 ProtoProxy 实测推翻了这一判断——见 §2.4。

## 2. 行为规格

### 2.1 `/title`（已核实）

来源：[/title 的 1.12.2 期修订](https://minecraft.wiki/w/Commands/title?oldid=1218226)（rev 1218226, 2018-06-01）。

1. 1.12.2 语法（页面「The command has six variations」）：
   `title <player> clear` ｜ `reset` ｜ `title <raw json title>` ｜ `subtitle <raw json title>` ｜ `actionbar <raw json title>` ｜ `times <fadeIn> <stay> <fadeOut>`
   - `title` / `subtitle` / `actionbar` 的参数是**文本**：vanilla 要求 raw JSON text，本 fork **两种都收**——以 `{` 或 `[` 开头按 JSON 组件校验（见 §2.5），否则按普通字符串渲染 `&` 颜色码（本 fork 的便利扩展，vanilla 会拒绝）；
   - `times <fadeIn> <stay> <fadeOut>` 三个参数是**整数**——该页版本历史明确 `1.19.4 → 22w17a` 才把它们改成**时间类型**，「Before this snapshot, these integer arguments specifies a time in game ticks」→ **1.12.2 单位是 game tick**，且不接受 `1s` 一类写法；
   - `clear` 清除屏幕上的标题（当前没有标题时无事发生，标题文本**保留**，见协议 action 4）；`reset` 把副标题清空、把三个时间恢复默认值（协议 action 5）——两者语义不同。
2. `times` 的取值：页面明确「All values must be between −2,147,483,648 and 2,147,483,647 (inclusive) … **but values below 0 will be treated as 0**」。默认值（未指定或 `reset` 后）为 **10 / 70 / 20**。
   → **本文件 §3 原文「越界校验（times 非负）」与来源冲突**：负数是**合法输入**、按 0 处理，报错是偏差。实现按来源（钳到 0），并记入本节。
3. 加入历史：1.8 → `14w20a`；`actionbar` 子命令：1.11 → `16w32b`。→ 1.12.2 六条形态都在。
4. 权限等级 2（同页信息框）。
5. 目标为多玩家时对每个玩家生效；`@e` 命中非玩家实体时忽略之（命令收的是 player 参数）。
6. 协议核对：Title = **0x48**（1.12.2；wiki.vg 1.12.2 期修订 rev 1320 与 `cProtocol_1_12_1::GetPacketID` 的 `pktTitle = 0x48` 一致），action 枚举 `0 set title / 1 set subtitle / 2 set action bar / 3 times / 4 hide / 5 reset`；同页注明「"Hide" makes the title disappear, but if you run times again the same title will appear. "Reset" erases the text.」→ `clear` 对应 action 4、`reset` 对应 action 5。

> 现状可用性核对：`SendSetTitle(cCompositeChat)` 走 `CreateJsonString()`（[Protocol_1_8.cpp:1370](../../src/Protocol/Protocol_1_8.cpp#L1370)），发出去的是 JSON 组件，符合协议要求；`SendSetRawTitle` 是**原样字符串**（不转 JSON），只能用于调用方自己给 JSON 的场合，本支不用它。`SendHideTitle()` = action 4、`SendResetTitle()` = action 5（[Protocol_1_11.cpp:372](../../src/Protocol/Protocol_1_11.cpp#L372)、[:384](../../src/Protocol/Protocol_1_11.cpp#L384)），与上面第 6 条一致。`SendTitleTimes(fadeInTicks, displayTicks, fadeOutTicks)` 单位即 tick。`actionbar` 用 `cPlayer:SendAboveActionBarMessage`（Chat Message 包的 position 2；同页注明与 Title action 2 等价，差别只是 MC-119145 的格式渲染 bug）。

### 2.2 `/particle`（已核实）

来源：[/particle 的 1.12.2 期修订](https://minecraft.wiki/w/Commands/particle?oldid=1186198)（rev 1186198, 2018-03-04；该修订的 Java 语法行标 `{{upcoming|until=1.13}}`，即 1.12.2 形态）。1.12.2 之后的变化（1.13 `17w45a` 移除 `<params>`、`17w47a` 改粒子名、`18w03a` 加 `<name> <pos>` 简写、1.20.5 参数改 SNBT）见同页 History。

1. 语法：
   `particle <name> <x> <y> <z> <xd> <yd> <zd> <speed> [count] [mode] [player] [params ...]`
   - `<name>`：粒子名。1.12.2 用**不带命名空间的旧名**（页面例：`explode`、`hugeexplosion`）；`dust`/`block`/`item` 那套带配置的名字是 1.13+；
   - `<x> <y> <z>`：粒子位置，接受 `~` 相对坐标，各分量须在 −30,000,000 … 30,000,000 内；
   - `<xd> <yd> <zd>`：散布体积，以 `<x y z>` 为中心、约除以 8（`1 1 1` ≈ 8×8×8）；
   - `<speed>`：粒子速度，**至少 0**；
   - `[count]`：粒子数，**至少 0**；页面明确「Must be at least 0 (**which produces one particle**)」→ **0 是合法值且语义为「只生成一个粒子」**（此时 `xd yd zd × speed` 作为粒子参数传递，见同页 `reddust`/`mobSpell` 的颜色说明）；
   - `[mode]`：`normal`（默认）或 `force`。本页写 `force` 可被 **256** 格外看见；同页 History 的 `15w32c` 条目写「普通 32 格、force 512 格」——**两处数字互相矛盾**〔待核：以实机为准，本支不依赖该距离，见第 3 条〕；
   - `[player]`：改由指定玩家观看，而不是可视范围内的所有人；
   - `[params ...]`：`blockdust` / `iconcrack` / `blockcrack` / `fallingdust` 的附加参数（本支不做，见 §3）。
2. 协议包（已核对，两份来源一致）：Particle = **0x22**，字段 `Particle ID / Long Distance(bool) / X Y Z / Offset X Y Z / Particle Data / Count / Data[]`；`Long Distance` 为 true 时「distance increases from 256 to 65536」（[wiki.vg Protocol 的 1.12.2 期修订](https://wikivg.booky.dev/w/Protocol?oldid=1320)，rev 1320 §Particle；`cProtocol_1_12_1::GetPacketID` 未覆盖该包，继承 1.9 的 0x22，与之一致）。
3. 本仓库现状核对（**决定本支能做到哪一步**）：
   - 粒子名→ID 表在 `cProtocol_1_8_0::GetProtocolParticleID()`（[Protocol_1_8.cpp:3980-4046](../../src/Protocol/Protocol_1_8.cpp#L3980)），**49 项、ID 0–48**，查找前 `StrToLower()` → 大小写不敏感；
   - **未知粒子名走 `LOGWARNING` + `ASSERT(!"Unknown particle")` + `return 0`**（[:4038-4043](../../src/Protocol/Protocol_1_8.cpp#L4038)）→ 在开启断言的构建里**会崩服**，且会静默变成 `explode`（ID 0）。故 Lua 侧白名单校验是**安全前置**，不只是 parity 打磨；
   - `cWorld:BroadcastParticleEffect()` 下发给「该位置的区块已加载的全部客户端」（[Broadcaster.cpp:400-420](../../src/Broadcaster.cpp#L400)，`ForClientsWithChunkAtPos`）：**没有距离裁剪、没有按玩家下发**。`cClientHandle::SendParticleEffect` 未导出到 Lua（[ClientHandle.h:198-199](../../src/ClientHandle.h#L198) 无 `tolua_export`）；
   - 协议层把 `Long Distance` 写死 `false`（[Protocol_1_8.cpp:969](../../src/Protocol/Protocol_1_8.cpp#L969)、[:992](../../src/Protocol/Protocol_1_8.cpp#L992)）→ **`mode` 无法透传**。
   → 故：`mode` 两个取值都接受（校验合法性），但**距离语义与 vanilla 不同**（本仓库按区块加载下发）；`<viewers>` **拒绝**并给出明确原因（无法按玩家下发，静默广播会波及无关玩家）。两条都记入 §5。

### 2.3 `/playsound`（已核实）

来源：[/playsound 的 1.12.2 期修订](https://minecraft.wiki/w/Commands/playsound?oldid=1218113)（rev 1218113, 2018-06-01；1.13 发布于 2018-07-18，故该修订即 1.12.2 形态）。当前页 History 补足了两条关键事实：「`16w02a`（1.9）`playsound` **now requires a** `source` parameter」、「1.20.5 `24w09a` Can now be used without specifying the player (assuming `@s`) and without specifying the mixer (assuming `master`)」→ **1.12.2 的 `source` 是必填**。

1. 语法：`playsound <sound> <source> <player> [x] [y] [z] [volume] [pitch] [minimumVolume]`
   - `<sound>`：`sounds.json` 里的 sound event 名（例 `mob.pig.say`）。资源包可加自己的事件；
   - `<source>`：**必填**，取值 `master`/`music`/`record`/`weather`/`block`/`hostile`/`neutral`/`player`/`ambient`/`voice`（`ui` 是 1.21.6 加的，1.12.2 没有）；
   - `<player>`：玩家名或目标选择器（1.8 注释：即使 `@e` 也不能命中非玩家实体）；
   - `[x] [y] [z]`：声音位置，接受 `~`；
   - `[volume]`：**至少 0.0**；描述「可听见的距离」——1.0 时听域半径 16 格，>1.0 不会更响而是把听域乘以 volume；
   - `[pitch]`：**0.0–2.0（含）**，<0.5 等价于 0.5；默认 1.0；
   - `[minimumVolume]`：**0.0–1.0（含）**；目标在听域外时，声音改在离目标不到 4 格处播放，音量取该值。
2. 协议包（已核对）：Named Sound Effect = **0x19**，字段 `Sound Name / Sound Category(VarInt enum) / X Y Z(×8 定点) / Volume(Float) / Pitch(Float)`；分类枚举 `master 0 … voice 9`（wiki.vg 1.12.2 期修订 rev 1320；`cProtocol_1_12_1` 未覆盖，继承 1.9 的 0x19，与之一致）。
3. 本仓库现状核对：
   - `cClientHandle::SendSoundEffect(name, Vector3d, volume, pitch)` **已导出**（[ClientHandle.h:225](../../src/ClientHandle.h#L225)）→ 可以做到「按目标玩家逐个下发」，与 vanilla 的 `<player>` 语义一致；
   - 但协议层把 **Sound Category 写死 0（master）**（[Protocol_1_9.cpp:578](../../src/Protocol/Protocol_1_9.cpp#L578)、[Protocol_1_10.cpp:308](../../src/Protocol/Protocol_1_10.cpp#L308)，注释原文「may want to be changed to a parameter later」）→ **`<source>` 校验得了、透传不了**；
   - 本仓库**没有** `sounds.json` 或任何服务端声音注册表（`find . -name sounds.json` 与 `grep -rn "SoundRegistry" src/` 均零命中）→ `<sound>` 只能做**格式**校验，无法像 vanilla 那样校验「是不是已注册的 sound event」。
   → 故：`source` 按十项白名单校验后**忽略**（记录偏差），`<sound>` 只校验格式，`minimumVolume` 按页面描述用「听域（volume×16 格）外则改在目标自身位置以该音量播放」近似。三条都记入 §5。

### 2.4 `/stopsound`（机制已由原版实测确定；本支已并入）

来源：[/stopsound 的 1.12.2 期修订](https://minecraft.wiki/w/Commands/stopsound?oldid=1217729)（rev 1217729, 2018-06-01）给出语法；**下发机制**来自合法客户端 + 合法 vanilla 服务端的 ProtoProxy 实测（[AGENTS §2.2](../../AGENTS.md) 允许的来源）。

1. 语法：`stopsound <player> [source] [sound]`
   - `<player>`：玩家名或目标选择器；
   - `[source]`：可选的十项分类（同 §2.3）；
   - `[sound]`：可选；不给则停止该范围内的全部声音。
2. **下发机制（实测）**：1.12.2 **没有** Stop Sound 包（protocol 340 的 clientbound 列表里没有该包，它到 1.13 才出现）。原版改用 **`MC|StopSound` 插件消息**（clientbound Plugin Message，包 ID **0x18**），载荷是**固定两个 VarInt 长度前缀的字符串**：先 source，后 sound；命令没给的那一段写成**空串**（不是省略）：

   | 命令（玩家原样输入） | 服务端发出的载荷 |
   |---|---|
   | `/stopsound Harry00025` | `00 00`（2 字节） |
   | `/stopsound Harry00025 weather` | `07 "weather" 00`（9 字节） |
   | `/stopsound Harry00025 weather minecraft:weather.rain` | `07 "weather" 16 "minecraft:weather.rain"`（31 字节） |

   整包示例（第三种形态）：
   ```
   2E 00 18 0C "MC|StopSound" 07 "weather" 16 "minecraft:weather.rain"
   ```
   - 三种形态各至少抓包一次（`build/bin/Logs/Log_1792127802_5.log` 的 54.240 / 73.983 / 77.137，`Log_1792294770_5.log` 的 9.794 / 13.643），同一形态多次执行字节**完全相同**；
   - 载荷永远是两个字符串这一点也解释了 ProtoProxy 的 "1 payload bytes left over, the proxy's layout is out of date"——它的旧布局只解一个字符串；
   - 服务端把 sound 名**原样**转发（没有改写命名空间）。
3. **成功反馈（实测）**：服务端随后发 Chat Message（0x0F，position 1）：
   `{"translate":"commands.stopsound.success.individualSound","with":["minecraft:weather.rain","weather","Harry00025"]}`
   —— 参数顺序是 `[sound, source, player]`。缺参数时 vanilla 发的是
   `{"color":"red","translate":"commands.generic.usage","with":[{"translate":"commands.stopsound.usage"}]}`
   （即原版用翻译键、红色系统消息；Core 用的是自己的英文文本，见 §5 第 13 条）。
4. 本仓库现状核对：`cClientHandle::SendPluginMessage(Channel, Message)`（[ClientHandle.h:212](../../src/ClientHandle.h#L212)）**已导出**（手写绑定取两个字符串，二进制字节按 `lua_strlen` 原样传出，[ManualBindings.cpp:2403](../../src/Bindings/ManualBindings.cpp#L2403)）且有 APIDump 文档 → **`/stopsound` 无需任何 `src/` 改动**，纯 Lua 即可。
5. 三种形态**均已实测**（见上表），实现按「固定两串、缺者为空串」编码，`BuildStopSoundPayload()` 对三种形态逐字节一致。

### 2.5 `/tellraw` 与 JSON 文本组件层（已核实；本支并入）

来源：[/tellraw 的 1.12.2 期修订](https://minecraft.wiki/w/Commands/tellraw?oldid=1136502)（rev 1136502, 2018-06-01）+ [Text component format](https://minecraft.wiki/w/Text_component_format)（组件类型与「谁负责解析」）。

1. 语法：`tellraw <player> <raw json message>`
   - `<player>`：玩家名或目标选择器（player 类）；
   - `<raw json message>`：**必须**是合法 raw JSON text（页面原文「Must be valid raw JSON text」，例 `{"text":"Hi there!","bold":true}`）。
2. 组件的顶层形态：对象，或数组（数组的每一项是组件）。
3. **服务端组件**：`Text component format` 原文——「Other types, such as `score`, `selector`, and `nbt`, require additional work from the *server* before they can be displayed by the client. Those are known as "server-side" or "advanced" text components.」并且明确「If the client receives unresolved server-side text components, `score` and `nbt` text components display as empty text, but **`selector` text components display the selector itself**」→ **不能原样透传**：`score` 会显示为空、`selector` 会把 `@a` 当字面文本显示出来。本支**拒绝**这两类组件（理由精确、不静默），展开它们（选择器解析 + 记分板取值）留待后续分支。
4. **参数必须取自原始命令行**：引擎的 `StringSplitCommandLine()` 把 `"` 当引号并**剥掉**，所以 `{"text":"Hi there"}` 到达 handler 时是 `{text:Hi there}`（引号没了、不再是合法 JSON）。因此文本参数要从**原始命令行**按 token 偏移切出——复用 C4 为 `/execute` 建的 `SplitRawCommand()` + span 机制。
5. 本仓库现状核对：
   - `cJson:Parse()`（[LuaJson.cpp:272](../../src/Bindings/LuaJson.cpp#L272)）返回 table 或 `nil + 错误文本`；JSON `null` 返回裸 `nil`（[:132](../../src/Bindings/LuaJson.cpp#L132)）→ 本支把「无错误文本但不是 table」一律判为「不是组件」，与来源第 2 条一致；
   - `cPlayer:SendMessageRaw(json, [type])`（[Player.cpp:1261](../../src/Entities/Player.cpp#L1261)）→ `cClientHandle::SendChatRaw()`：**JSON 组件原样写进 Chat 包**；第 2 个参数可用 `ctAboveActionBar`，这正是 `/title ... actionbar` 的 JSON 形态要走的路；
   - **`${` 会被整条丢弃**：`cProtocol_1_8_0::SendChatRaw()` 有 log4shell 加固（[Protocol_1_8.cpp:334-340](../../src/Protocol/Protocol_1_8.cpp#L334)），含 `${` 的消息直接 `return`，客户端什么也收不到。本支**拒绝**含 `${` 的聊天类文本（报错而不是谎报成功）；注意**报错文本本身也不能含该序列**，否则同样被丢——`MakeChatSafe()` 负责这一点，见 §5 第 11 条；
   - `/title` 的 `title`/`subtitle` 走 **Title 包**（`SendSetRawTitle`/`SendSetRawSubTitle`），**不经过** `SendChatRaw()`，所以 `${` 在这两个位置不受该加固影响。

## 3. 本分支范围

**做**：
1. `cmd_title.lua`：六条形态（`title`/`subtitle`/`actionbar`/`times`/`clear`/`reset`）+ 目标解析（C1，player 类）+ 参数校验（`times` 三个整数，负数按 0 钳——见 §2.1 第 2 条；非数字报错文案沿用 Core 风格）。
2. `cmd_particle.lua`：粒子名白名单校验（**未知名返回失败**，理由见 §2.2 第 3 条：C++ 侧会 `ASSERT` 崩服并静默退回 `explode`）、位置/散布体积/速度（含 `~` 相对坐标，复用 C4 的 `ParseCommandCoords`）、`count`/`speed` 透传给 `BroadcastParticleEffect`；`mode` 校验后忽略、`<viewers>` 明确拒绝（两条偏差见 §5）。
3. `cmd_playsound.lua`：`<sound> <source> <player> [x] [y] [z] [volume] [pitch] [minimumVolume]`，按目标玩家逐个 `SendSoundEffect`；`<source>` 校验后忽略（偏差见 §5）。
4. `cmd_tellraw.lua`（2026-10 维护者决定并入本支）：`tellraw <player> <raw json message>`，JSON 组件经 `cJson:Parse()` 校验后用 `SendMessageRaw()` 原样下发；**服务端组件（`selector`/`score`）与含 `${` 的消息明确拒绝**（理由见 §2.5 第 3/5 条）。
5. `command_text.lua`（新）：JSON 文本组件层——`CutRawRemainder()`（从原始命令行切出参数，复用 C4 的 `SplitRawCommand()`）、`CheckTextComponent()`、`ParseTextArgument()`、`CheckChatDeliverable()`；`/title` 的三个文本形态与 `/tellraw` 共用它。
6. 共享的小工具：坐标 token 解析与「选择器 → 玩家列表」两处样板**已由 C1/C4 落在** `command_coords.lua`（`ParseCommandCoords`）与 `target_selectors.lua`（`ResolveTargets(..., "player")`），本支直接复用，不再新造。
7. `cmd_stopsound.lua`（2026-10 实测后并入本支）：`stopsound <player> [source] [sound]`，按玩家发 `MC|StopSound` 插件消息；载荷由纯函数 `BuildStopSoundPayload()` 生成，**字节对原版抓包逐字节核对**（§2.4、§4）。
8. 纯解析层抽成 `command_presentation.lua`（五个 `Parse*Line()` + 两张白名单 + 载荷编码器），照 C4 的 `command_execute.lua` 模式，便于用 stock Lua 表驱动测试。

**不做**：
- `/stopsound` 的 0 串 / 1 串载荷形态：按 §2.4 第 5 条实现为「省略对应字符串」，但仍标注〔待核〕（等原版抓包复核）。
- `/particle` 的 `[params ...]`（`blockdust`/`iconcrack`/`blockcrack`/`fallingdust` 的附加值）：`BroadcastParticleEffect` 的 `std::array<int,2>` 重载只覆盖 `iconcrack`（2 值）与 `blockcrack`/`blockdust`/`fallingdust`（1 值），且 1.12.2 的写法是「id+(data×4096)」的数字编码；本轮不做数值 ID 解析，遇 `params` 明确报「不支持」。
- `/particle` 的 1.13+ 形态（`dust`/`block`/`item` 带配置的粒子名、`/particle <name> <pos>` 简写、`<count>` 缺省即单粒子）。
- `/tellraw` 的**服务端组件展开**（`{"selector":"@a"}`、`{"score":...}`）：需要把选择器层与记分板接进组件树，本轮只做「拒绝而非静默错渲」；`/titleraw`（BE 独有）、`/bossbar`（1.13+）。
- 任何 `src/` 改动：本支**只改 Core**（与 [roadmap §5](vanilla-1.12.2-command-layer-roadmap.md) 对 C5 的改动面约定一致）。上面每条偏差所需的 C++ 前置（`mode` 的 long-distance 位、`<source>` 的 sound category、按玩家下发粒子、Stop Sound 包）都留给后续分支。

## 4. 测试与验收

- **胶水层（handler 级）**：`tests/test_presentation_handlers.lua`（`lua tests/test_presentation_handlers.lua`）用一套**只实现「允许调用」的方法**的假宇宙（假玩家 / 假 client handle / 假世界）直接驱动五个 handler，断言每个命令**在哪个接收者上调了哪个方法、参数是什么**，并检查**每条被拒绝的输入都真的给玩家回了话、且回话文本本身是可送达的**。**实测 68 条断言全绿**。这一层是解析测试看不见的：`/title @s actionbar` 曾因把 `cPlayer:SendAboveActionBarMessage` 写成 `Client:SendAboveActionBarMessage` 而在实机崩（`attempt to call method (a nil value)`），本套件就是它的回归守卫（故意改回错误接收者时该套件立刻报错）。
- **纯逻辑抽头**：`command_presentation.lua` 的解析函数写成纯函数（唯一的例外是 `ParseTellrawLine()`，它按需调用 `cJson:Parse()`），表驱动测试 `tests/test_presentation_commands.lua`（`lua tests/test_presentation_commands.lua`）覆盖：缺参/多参、`~` 坐标、非数字、负数、越界、未知粒子名、未知 sound source、`<viewers>`/`[params]` 拒绝路径、原始命令行切分（含「引擎剥引号」的对照断言）、JSON 组件校验（合法对象/数组、畸形 JSON、标量/null、嵌套 `selector`）、含 `${` 的拒绝、以及畸形输入不得崩。**实测 271 条断言全绿**（测试自带一个最小 JSON 解析器顶替 `cJson`，见该文件头注释），且与 `cProtocol_1_8_0::GetProtocolParticleID()` 的 49 个名字逐一对表（测试里独立抄了一份，两侧互查）；`MC|StopSound` 的载荷用**原版抓包的字节**做断言，三种形态（`00 00` / `07 "weather" 00` / `07 "weather" 16 …`）逐一比对，另含 >127 长度的 VarInt 用例。
- **fuzz**：`choices` 增列 `"title"`、`"clear"`、`"particle"`、`"explode"`、`"master"`、`"mob.pig.say"`、`"tellraw"`、`"{\"text\":\"hi\"}"`、`"{\"text\":"`、`"stopsound"`、`"weather"`、`"minecraft:weather.rain"`；`fuzzAllCommands` 覆盖「所有已注册命令」，注册即覆盖。**本机未能执行**：该场景要外部 `CuberitePluginChecker` 驱动（[Core 的 ci.yml](../../Server/Plugins/Core/.github/workflows/ci.yml) 的 `Run tests` 步骤），本机无 checker、`github.com` 也不可达 → 〔待核〕，复现命令同 C4。
- Core 侧 `luac -p` / `luacheck`：`luac -p` 覆盖 Core 全部 Lua 文件；九个新增/改动文件（`command_presentation.lua`、`command_text.lua`、五个 `cmd_*.lua`、两个测试）**0 warning / 0 error**（用服务器生成的 `build/Server/.luacheckrc`、在 `build/Server` 下跑——这是 `.luacheckrc` 的来源与正确 CWD）。`Info.lua` 的改动新增 8 条 W113（6 个新 handler 名 + `HandleTellraw*`），与该文件既有的 127 条 **同型**，无新类型、无行尾空格。C1–C4 既有的五个纯 Lua 套件重跑无回归（35 / 153 / 95 / 50 / 44 条断言），本支两套共 339 条断言。
- 本支只用**已文档化**的 C++ 成员（`SendSetTitle`/`SendSetSubTitle`/`SendSetRawTitle`/`SendSetRawSubTitle`/`SendTitleTimes`/`SendHideTitle`/`SendResetTitle`/`SendAboveActionBarMessage`/`SendMessageRaw`/`SendPluginMessage`/`cJson:Parse`/`BroadcastParticleEffect`/`SendSoundEffect`/`GetClientHandle` 在 [APIDesc.lua](../../Server/Plugins/APIDump/APIDesc.lua) 均已在册）→ 无新增未文档化符号，门 6 不适用（详见 §6）。
- 行为证据：见 §6 的运行期观察（控制台命令表 + 真实服务器）；客户端可见效果仍为「未验」（本机无实机）。

## 5. 已知偏差 / 风险

1. **JSON 组件只支持客户端自己能渲染的那部分**：`title`/`subtitle` 走 `SendSetRawTitle`/`SendSetRawSubTitle`、`actionbar` 与 `/tellraw` 走 `SendMessageRaw`，因此 `{"text":...}`/`{"translate":...}`/`clickEvent`/`hoverEvent`/`extra` 等**都能透传**；但 `selector` 与 `score` 是**服务端组件**（§2.5 第 3 条有来源引文），原样发出去客户端会显示成选择器字面文本或空白，故本支**拒绝**它们（展开留待后续分支）。另外本 fork 保留「普通字符串 + `&` 颜色码」作为便利扩展：**以 `{`/`[` 开头**的参数按 JSON 校验（不合法就报错，不静默当文本），其余按普通文本渲染。
2. **`/particle` 的 `mode` 无效果**：`Long Distance` 位在协议层写死 `false`（[Protocol_1_8.cpp:969](../../src/Protocol/Protocol_1_8.cpp#L969)），且 `BroadcastParticleEffect` 按「区块已加载」下发、**不做 32/256/512 格裁剪**（[Broadcaster.cpp:400](../../src/Broadcaster.cpp#L400)）。故 `normal` 与 `force` 在本仓库产出**完全相同**，且都既不等于 vanilla 的 `normal`（32 格内）也不等于 `force`。
3. **`/particle` 的 `<viewers>` 不支持**：`cClientHandle::SendParticleEffect` 未导出（[ClientHandle.h:198-199](../../src/ClientHandle.h#L198)），无法只发给指定玩家；静默广播会波及无关玩家，故**明确报错**而不是忽略（与 C1 对未实现选择器参数的处理一致）。
4. **`/particle` 的 `[params ...]` 不支持**：见 §3「不做」。
5. **`/playsound` 的 `<source>` 无效果**：协议层把 Sound Category 写死 0（master）（[Protocol_1_9.cpp:578](../../src/Protocol/Protocol_1_9.cpp#L578)、[Protocol_1_10.cpp:308](../../src/Protocol/Protocol_1_10.cpp#L308)）。命令校验十项白名单后**无法透传**——玩家的「音乐与声音」分类音量设置因此不生效。
6. **`/playsound` 的 `<sound>` 不做注册表校验**：本仓库没有 `sounds.json`/声音注册表，只能校验「非空、无空格」，未知 sound event 会照发（客户端静默忽略）。vanilla 会**解析失败**。
7. **`/playsound` 的 `minimumVolume` 是近似**：按页面描述实现为「目标到声源距离 > volume×16 格时，改在**目标自身位置**以 `minimumVolume` 播放」。vanilla 是「离目标不到 4 格处」，具体落点算法**无来源**〔推测：改成目标自身位置可保证听域内、且听感等价；置信度中〕。
8. **`<volume>`/`<pitch>`/`<minimumVolume>` 的越界处理是〔推测〕**：页面写「Must be at least 0.0」「Must be between 0.0 and 2.0」，本支按**校验失败**处理（而不是钳值），因为这些措辞与同页 `count`/`radius` 那类「must be at least」一致。若实机证明是钳值，改起来只动 `command_presentation.lua` 一处。
9. **`/stopsound` 只对 1.12.2 及更早的客户端有效**：`MC|StopSound` 是 1.12.2 期的通道名；1.13 起原版改用真正的 Stop Sound 包（0x4C），本仓库的 1.13/1.14 协议实现处于冻结状态（[AGENTS §9](../../AGENTS.md)）且不发该包 → 1.13+ 客户端会忽略这条插件消息（命令本身仍报成功）。1.8 客户端没有 `/stopsound`（1.9.3 才加入），同样忽略。
  另：原版反馈文本的翻译键见 §2.4 第 3 条（与本支英文文本的差异属第 13 条）。
10. **权限/目标语义**：`/title`、`/particle` 的目标是 player 类；`/particle` 本身**没有目标参数**（它作用于「可视范围内的人」），所以 C5 的 `/particle` 不需要（也无法）用选择器限流——这与同批其它命令不同，实现时不要照抄目标解析样板。
11. **含 `${` 的聊天类文本被拒绝，且反馈本身必须可送达**：`SendChatRaw()` 的 log4shell 加固会**整条丢弃**含 `${` 的消息（§2.5 第 5 条）。受影响的是 `/tellraw` 与 `/title ... actionbar`（两者都走 Chat 包）；本支对它们**报错**而不是静默无输出后谎报成功。`/title ... title|subtitle` 走 Title 包，**不受**该加固影响、也不做此检查。vanilla 1.12.2 会正常送达，这是本 fork 的安全加固带来的偏差。
    **实测发现的两个坑（均为「静默」）**：① 反馈本身也走 `cPlayer:SendMessageFailure()` → `SendChat()` → `SendChatRaw()`（[Player.cpp:1207](../../src/Entities/Player.cpp#L1207)），所以**报错文本里如果原样写出 `${`，这条报错也会被同一个加固丢掉**——`/title @s actionbar a${b}` 因此完全没有回执。现改为**用文字描述**该序列（"a dollar sign followed by an opening brace"），并在 `MakeChatSafe()` 里把可能引用玩家输入的反馈中的 `${` 改写成 `$ {`，五个命令的 17 处 `SendMessageFailure` 全部经过它。② 解析器的报错会**引用玩家原样输入**（如 `Unknown particle "${x}"`），同样会被吞——`MakeChatSafe()` 一并覆盖。两条都有回归断言（见 §4 的 handler 套件）。
12. **`/tellraw` 不支持普通字符串**：vanilla 的 `/tellraw` 只收 JSON，本支保持这一点（不做 `/title` 那样的文本便利扩展）；`{"text":"..."}` 必须用 JSON 写。
13. **成功反馈文本与原版不同形**：五个命令都沿用 Core 的 `Successfully ...` 风格英文文本；原版用**翻译键 + 红色系统消息**（`/stopsound` 的已由抓包确认为 `commands.stopsound.success.individualSound`，参数序 `[sound, source, player]`，见 §2.4 第 3 条）。要完全一致需要 `cCompositeChat` 支持 `translate` 组件——本仓库没有该能力〔待核：其余四条命令的原版反馈文本也未抓包核对〕。

## 6. 实现与运行期证据（本分支）

**触点**（全部在 Core，无 `src/` 改动）：

- `Server/Plugins/Core/command_presentation.lua`（新）：解析层——`ParseTitleLine()` / `ParseParticleLine()` / `ParsePlaysoundLine()` / `ParseTellrawLine()` + `IsKnownParticleName()` / `IsKnownSoundSource()` / `GetAudibleRadius()`，两张白名单分别镜像 `cProtocol_1_8_0::GetProtocolParticleID()`（49 项）与 1.12.2 的十个 sound category。
- `Server/Plugins/Core/command_text.lua`（新）：JSON 文本组件层——`CutRawRemainder()` / `CheckTextComponent()` / `ParseTextArgument()` / `CheckChatDeliverable()`。
- `Server/Plugins/Core/cmd_title.lua` / `cmd_particle.lua` / `cmd_playsound.lua` / `cmd_tellraw.lua` / `cmd_stopsound.lua`（新）：薄胶水层，负责选择器解析（C1 的 `ResolveTargets`）、坐标解析（C4 的 `ParseCommandCoords`）与 C++ 调用。
- `Server/Plugins/Core/Info.lua`：`Commands` 增 `/particle`（`core.particle`）、`/playsound`（`core.playsound`）、`/stopsound`（`core.stopsound`）、`/title`（`core.title`）、`/tellraw`（`core.tellraw`）；`ConsoleCommands` 增同名五条。
- `Server/Plugins/Core/tests/test_presentation_commands.lua`（新，解析层 270 条断言，含测试自带的最小 JSON 解析器）与 `tests/test_presentation_handlers.lua`（新，胶水层 68 条断言，假宇宙 + 接收者/反馈断言）；`tests/FuzzCommands.lua` 的 `choices` 增列 `"title"`、`"clear"`、`"particle"`、`"explode"`、`"master"`、`"mob.pig.say"`、`"tellraw"`、`"{\"text\":\"hi\"}"`、`"{\"text\":"`、`"stopsound"`、`"weather"`、`"minecraft:weather.rain"`。

**运行期观察**（真实服务器 `build/Server/Cuberite` + 临时探针插件；探针放在 `build/` 之外的临时运行目录，未入库。探针以 `cCommandOrigin(默认世界, (0, 64, 0))`、无权限主体调用 `cPluginManager:ExecuteCommandFromOrigin()`，即控制台命令表）：

| 命令 | result | 成功计数 | 输出文本 |
|---|---|---|---|
| `particle explode ~ ~ ~ 0 0 0 1` | 0（crExecuted） | **1** | `Successfully created the particle "explode"` |
| `particle explode ~ ~1 ~ 0 0 0 0 0` | 0 | 1 | `Successfully created the particle "explode"`（`count=0` = 单粒子形态） |
| `particle explode ~ ~ ~ 0 0 0 1 1 force` | 0 | 1 | `Successfully created the particle "explode"`（`force` 被接受；下发语义与 `normal` 相同，见 §5 第 2 条） |
| `particle mobSpell ~ ~ ~ 0.5 1 0.5 1 0` | 0 | 1 | `Successfully created the particle "mobSpell"`（1.12.2 的 camelCase 写法） |
| `particle dust ~ ~ ~ 0 0 0 1` | 0 | 0 | `Unknown particle "dust"`（1.13+ 名称） |
| `particle explode ^ ~ ~ 0 0 0 1` | 0 | 0 | `Invalid x coordinate "^" ('^' is not a valid number)`（局部坐标是 1.13+） |
| `particle explode ~ ~ ~ 0 0 0 -1` | 0 | 0 | `The speed must be at least 0` |
| `particle explode ~ ~ ~ 0 0 0 1 1 normal @a` | 0 | 0 | `Selecting the viewers of a particle is not supported yet` |
| `particle blockcrack ~ ~ ~ 0 0 0 1 1 normal @a 1` | 0 | 0 | `Particle parameters are not supported yet`（`[params]` 在 `<viewers>` 之后，故带 12 个 token 时才报这条） |
| `particle explode ~ ~ ~ 0 0 0` | 0 | 0 | `Usage: particle <name> <x> <y> <z> <xd> <yd> <zd> <speed> [count] [normal\|force]` |
| `title @a title Hello there` | 0 | 0 | `No player was reached`（无人在线） |
| `title Nobody title Hello` | 0 | 0 | `Player "Nobody" not found` |
| `title @a times -5 70 20` | 0 | 0 | `No player was reached`（负数被钳，未报错——与 §2.1 第 2 条一致） |
| `title @a times 1 2 3 4` | 0 | 0 | `Unexpected argument "4"` |
| `title @a bogus x` | 0 | 0 | `Unknown title action "bogus"` |
| `title @a clear` | 0 | 0 | `No player was reached` |
| `title` | 0 | 0 | `Usage: title <player> title\|subtitle\|actionbar <text> OR <player> times <fadeIn> <stay> <fadeOut> OR <player> clear\|reset`（控制台路径的 `Split[1]` 无前导 `/`） |
| `playsound mob.pig.say master @a` | 0 | 0 | `No target was able to hear the sound`（无人在线） |
| `playsound mob.pig.say ui @a` | 0 | 0 | `Unknown sound source "ui"`（1.21.6 新增分类） |
| `playsound mob.pig.say master Nobody` | 0 | 0 | `Player "Nobody" not found` |
| `playsound mob.pig.say master @a ~ ~` | 0 | 0 | `Usage: playsound <sound> <source> <player> [x] [y] [z] [volume] [pitch] [minimumVolume]` |
| `playsound mob.pig.say master @a ~ ~ ~ 1 3` | 0 | 0 | `The pitch must be at most 2` |
| `stopsound @a` | 1（crUnknownCommand） | 1 | （未注册——本支不含 `/stopsound`，见 §2.4） |

另外两项探针检查：

- **标题文本 → JSON 组件**（`cCompositeChat` 的转换，即 `SendSetTitle` 实际下发的内容）：
  `cCompositeChat("&cHello &lworld"):CreateJsonString(false)`
  → `{"extra":[{"color":"red","text":"Hello "},{"bold":true,"color":"red","text":"world"}],"text":""}`。
- **注册表**：`ForEachCommand` / `ForEachConsoleCommand` 均报 `/particle`、`/playsound`、`/title`、`/tellraw`（in-game）与 `particle`、`playsound`、`title`、`tellraw`（console）为已注册。

**JSON 文本层（`/tellraw` 与 `/title` 的 JSON 形态）**：同一台服务器、同一探针；命令行**按玩家输入原样**构造（探针自己调 `StringSplitCommandLine()` 得到 token 表，并把原始行当第三参传入，与引擎的调用形态一致）：

| 命令 | result | 成功计数 | 输出文本 |
|---|---|---|---|
| `tellraw @a {"text":"Hi there","color":"red"}` | 0 | 0 | `No player was reached`（**校验已通过**：含引号与空格的组件完整到达 parser） |
| `tellraw @a ["a",{"text":"b"}]` | 0 | 0 | `No player was reached`（数组形态同样通过） |
| `tellraw @a {oops}` | 0 | 0 | `Invalid JSON text component: Parsing Json failed: * Line 1, Column 2` |
| `tellraw @a {"text": oops}` | 0 | 0 | `Invalid JSON text component: Parsing Json failed: * Line 1, Column 10`（**列号证明空格保住了**——若引号被引擎剥掉会得到 `{text: oops}`） |
| `tellraw @a 42` | 0 | 0 | `Invalid JSON text component: it must be a JSON object or array` |
| `tellraw @a {"selector":"@a"}` | 0 | 0 | `The "selector" and "score" text components are not supported yet` |
| `tellraw @a {"text":"${jndi}"}` | 0 | 0 | `A message containing "${" cannot be sent` |
| `tellraw @a` | 0 | 0 | `No message was given` |
| `title @a title {"text":"Chapter I","bold":true}` | 0 | 0 | `No player was reached`（JSON 形态通过校验） |
| `title @a subtitle {oops}` | 0 | 0 | `Invalid JSON text component: Parsing Json failed: * Line 1, Column 2` |
| `title @a actionbar {"text":"${x}"}` | 0 | 0 | `A message containing "${" cannot be sent` |
| `title @a actionbar plain ${x}` | 0 | 0 | `A message containing "${" cannot be sent`（普通文本形态同样检查） |
| `title @a title Hello there` | 0 | 0 | `No player was reached`（普通文本形态未回归） |

**`/stopsound`（`MC|StopSound`）**：载荷在**插件自己的 Lua 5.1 状态**里验证（探针 `dofile` Core 的 `command_presentation.lua` 后直接调用 `BuildStopSoundPayload()`，即命令实际交给 `SendPluginMessage()` 的字节）：

| 形态 | 本仓库产出的载荷（hex） | 原版抓包参考 |
|---|---|---|
| `weather` + `minecraft:weather.rain` | `07 77656174686572` `16 6D696E6563726166743A776561746865722E7261696E` | **逐字节相同**（`Log_1792127802_5.log` 的 `07 "weather" 16 "minecraft:weather.rain"`） |
| 只给分类 `weather` | `07 77656174686572` | 无抓包，〔待核〕（§2.4 第 5 条） |
| 什么都不给 | 空 | 无抓包，〔待核〕（同上） |

命令级（同一台服务器、控制台命令表）：

| 命令 | result | 成功计数 | 输出文本 |
|---|---|---|---|
| `stopsound @a weather minecraft:weather.rain` | 0 | 0 | `No player was reached`（**解析通过**） |
| `stopsound @a weather` | 0 | 0 | `No player was reached` |
| `stopsound @a` | 0 | 0 | `No player was reached` |
| `stopsound @a ui` | 0 | 0 | `Unknown sound source "ui"` |
| `stopsound @a master one two` | 0 | 0 | `Unexpected argument "two"` |
| `stopsound` | 0 | 0 | `Usage: stopsound <player> [source] [sound]` |

注册表：`ForEachCommand` / `ForEachConsoleCommand` 均报 `/stopsound` 与 `stopsound` 为已注册。

**客户端实测（维护者，2026-10-18，原版 1.12.2 客户端连本仓库服务器）**：C5 §2.1/§2.2/§2.3/§2.5/§2.4 的可见行为逐条通过——标题 / 副标题 / actionbar / `times` / `clear` 与 `reset` 的差异、plain 文本的 `&` 颜色码与 JSON 组件两种形态、`clear` 后文本保留、`/tellraw` 的组件透传与含空格 JSON、`/particle` 的粒子与 `count=0`、`/playsound` 的按玩家下发、`/stopsound` 真正停掉声音，以及 `mode=force` 不报错。**过程中抓到并已修掉两个只有实机才暴露的 bug**（见 §5 第 13 条与 `cmd_title.lua` 的 actionbar 接收者），修完后重测全部通过。

**仍未验**：

- **1.13+ 客户端**收到该通道的行为（按设计：原版 1.13 起改用真正的 Stop Sound 包，本仓库不受理）→ §5 第 9 条；
- 成功反馈**文本与原版不同形**（原版用翻译键 + 红色系统消息，已由抓包确认；本支用英文文本）→ §5 第 13 条。
