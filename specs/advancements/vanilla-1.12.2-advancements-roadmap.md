# 1.12.2 进度（Advancements）系统 roadmap

> **状态**：规划（未实现）。本文件是一次规划任务的交付物；它列出的条目**不自动构成新任务**（[AGENTS.md §1](../../AGENTS.md)），每个分支开工仍需维护者下达。
> **基线**：master 317733a（写作时的 HEAD）。
> **缺口来源**：上游 [issue #3770 "Add support for advancements"](https://github.com/cuberite/cuberite/issues/3770)（2017-06-14 开，仍 OPEN）。该 issue 里 1.12 客户端打开进度界面会被踢下线；PR [#3845](https://github.com/cuberite/cuberite/pull/3845) 把这里热修成当前这句 INFO 文案（§2）。
> **核对结论**：缺口在当前 HEAD 仍然存在（§2）。
> **落盘位置说明**：本文件覆盖协议、服务端模型、触发器、命令与多版本显示，不是单条规格；因此新开 specs/advancements/。若维护者希望换位置，移动时按 [specs/README.md](../README.md) 的引用面规则整批改。

## 1. 结论摘要

1. **1.12 / 1.12.1 / 1.12.2：可以做到"服务端只实现进度，客户端正常显示进度"。** 进度界面完全由服务端下发的 Advancements 包驱动：树结构、标题/描述、图标、边框、背景、toast、隐藏、逐条件进度、移除都在包里。客户端不需要服务端下发任何文件；服务端只要把树和进度描述对，1.12.x 客户端就会画出与 vanilla 相同形状的界面。
2. **1.8–1.11.2：不能。** 这些客户端没有进度界面；成就界面是客户端内置的一棵固定树（到 1.11.2 为止共 34 个成就），服务端唯一能影响的通道是 Statistics 包里的 achievement.* 统计项。服务端可以驱动这 34 个内置成就的一部分（与 1.12 进度重叠的 13 个），但**无法让自定义进度/新成就出现在界面上**。
3. **1.13 / 1.14.4：协议上有同形的包，但按 [AGENTS.md §9](../../AGENTS.md) 冻结。** 不扩展、不做 parity；实现时给它们留正确的包 ID 但不下发（或另开任务决定），不要把 1.12.2 的显示调优算到它们头上。
4. **这是一次规划，不是实现。** 分支拆分与降险顺序见 §11；第一个分支先做"协议 + 两节点树 + 实机截图"，把客户端行为的不确定性提前打掉。

## 2. 缺口在当前 HEAD 仍然存在（证据）

| 缺口 | HEAD 证据 |
|---|---|
| 没有 clientbound 进度包 | ePacketType 里只有 pktStatistics（[Protocol.h:117](../../src/Protocol/Protocol.h)），没有 pktAdvancements；grep -rn "pktAdvancement" src/ 零命中 |
| 只有 serverbound 页签热修 | [Protocol_1_12.cpp:1092-1096](../../src/Protocol/Protocol_1_12.cpp) 把页签包整个丢掉，只回一句 "The new advancements are not implemented."；1.13/1.14 复用它（[Protocol_1_13.cpp:616](../../src/Protocol/Protocol_1_13.cpp)、[Protocol_1_14.cpp:968](../../src/Protocol/Protocol_1_14.cpp)） |
| 1.12 包 ID 表没有进度项 | [Protocol_1_12.cpp:1023](../../src/Protocol/Protocol_1_12.cpp)（1.12）、[Protocol_1_12.cpp:1217](../../src/Protocol/Protocol_1_12.cpp)（1.12.1；1.12.2 继承） |
| 1.13/1.14 表同样没有 | [Protocol_1_13.cpp:392](../../src/Protocol/Protocol_1_13.cpp)、[Protocol_1_14.cpp:453](../../src/Protocol/Protocol_1_14.cpp) |
| 包名映射表没有 | [Packetizer.cpp:119](../../src/Protocol/Packetizer.cpp) 附近只有 pktStatistics 等条目 |
| 服务端只有 1.8 成就统计 | 34 个 CustomStatistic::Ach*（[CustomStatistics.h:8-41](../../src/Registries/CustomStatistics.h)）；cPlayer::AwardAchievement()（[Player.cpp:1379-1405](../../src/Entities/Player.cpp)）；1.8 统计名 achievement.*（[Protocol_1_8.cpp:4052+](../../src/Protocol/Protocol_1_8.cpp)） |
| 持久化只有统计 | 玩家 JSON 的读写（[Player.cpp:1810](../../src/Entities/Player.cpp)、[Player.cpp:1940](../../src/Entities/Player.cpp)）；统计另存（[Player.cpp:2028](../../src/Entities/Player.cpp)）；成就名落盘为 cuberite:achievement.*（[NamespaceSerializer.cpp:101-134](../../src/WorldStorage/NamespaceSerializer.cpp)） |
| 没有数据文件 | Server/ 下没有 advancements 目录；数据文件按 CWD 平铺读取（如 crafting.txt，[CraftingRecipes.cpp:389](../../src/CraftingRecipes.cpp)） |
| 没有 /advancement | Core 的 Commands 表（[Info.lua:12](../../Server/Plugins/Core/Info.lua)）没有它；grep -in advancement Server/Plugins/Core/*.lua 零命中；[command-layer roadmap:120](../commands/vanilla-1.12.2-command-layer-roadmap.md) 已把它列为"需新 C++" |
| 没有 Lua 进度 API | cPlayer 没有导出进度读写；只有 GetStatistics() 的只读文档（APIDump） |
| 入服时没有进度同步 | [ClientHandle.cpp:465-489](../../src/ClientHandle.cpp) 的登录序列：SendLogin → 权限 → 配方 → 玩家列表 → SendStatistics；没有进度 |

注：1.13/1.14 的 HandlePacketAdvancementTab 是继承来的，所以这两版客户端点页签时也会收到同一句 INFO 文案——这本身是当前的一个可见瑕疵，实现进度时应一并删掉（§6.4）。

## 3. 多版本客户端到底能显示什么

### 3.1 版本矩阵

| 客户端 | 进度界面 | 服务端→客户端的通道 | 服务端能定义树/标题/图标吗 | 服务端能驱动"成就"吗 | 本仓库现状 |
|---|---|---|---|---|---|
| 1.8.x–1.11.2 | 无 | Statistics（1.8 0x37；1.9–1.12 0x07） | 不能 | 只能内置 34 个成就，且通道是统计（§3.3） | 统计名与 34 个 Ach* 已有 |
| 1.12（335） | 有 | Advancements 0x4C + Select Advancement Tab 0x36 | 能 | 成就已移除，只有进度 | 无 |
| 1.12.1（338） | 有 | Advancements 0x4D + 0x37 | 能 | 同上 | 无 |
| 1.12.2（340） | 有 | Advancements 0x4D + 0x37 | 能 | 同上 | 无 |
| 1.13 / 1.13.1（393 / 401） | 有 | Advancements 0x4D + 0x37（wiki 修订页） | 能（协议同形） | 同上 | 冻结，无 |
| 1.14.x（477–498） | 有 | Advancements 0x51 + Select Advancement Tab 0x3A | 能（协议同形） | 同上 | 冻结，无 |

版本号与"接受 1.8–1.14.4"的范围见 [ProtocolRecognizer.h:10](../../src/Protocol/ProtocolRecognizer.h) 的 MCS_CLIENT_VERSIONS。

### 3.2 1.12–1.12.2：进度包是唯一数据源

1.12.2 的 Advancements 包（0x4D，clientbound play）一次带四段（[wiki.vg 1.12.2 协议页，oldid 2772350](https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2772350)）：

| 段 | 字段 | 类型 | 说明 |
|---|---|---|---|
| 1 | Reset/Clear | Boolean | 是否先清空客户端已有的进度 |
| 2 | Mapping size | VarInt | 后接若干 Identifier + Advancement |
| 2 | Advancement | Bool has parent → Identifier parent；Bool has display → Display；VarInt 条件数 → [条件名 String]；VarInt 需求组数 → [[条件名 String]] | 声明树与条件 |
| 2 | Display | title (Chat)、description (Chat)、icon (Slot)、frame (VarInt)、flags (Int)、background (Identifier，可选)、x (Float)、y (Float) | 显示数据 |
| 3 | Removals | VarInt + [Identifier] | 移除已声明的进度 |
| 4 | Progress | VarInt + [Identifier + Progress] | 进度 |
| 4 | Progress | VarInt 条件数 → [条件名 String, achieved Bool, (date Long)] | date 仅在 achieved 为真时存在，单位毫秒（Date.getTime()） |

要点：

- **frame 的线上枚举是 0=task、1=challenge、2=goal**（wiki 页原文），与 JSON 里的字符串不是同一顺序，序列化时别按 JSON 顺序写。
- **flags**：0x1 有 background、0x2 show_toast、0x4 hidden。announce_to_chat **不在包里**——聊天公告是服务端自己发的（§8）。
- **title/description 是 Chat 组件**：可以是字面文本，也可以是 translate + with。客户端对未知 translate 键回退为键原文（本仓库 chat 规格已实测，见 [R14/R15](../chat/vanilla-1.12.2-client-translatable-messages.md)）。
- **icon 是 Slot**：1.12.2 的物品序列化与背包槽位同一套（仓库侧用 WriteItem，[Protocol_1_8.h:240](../../src/Protocol/Protocol_1_8.h)）。
- **background 是资源位置字符串**：客户端只会画它自己带的纹理；根节点没给 background 时画"缺失纹理"（[Advancement definition](https://minecraft.wiki/w/Advancement_definition) 原文："The missing texture displays on the tab's background if the root advancement does not have a background."）。5 个 vanilla 页签背景的准确字符串本次未核实〔待核〕（§13）；自定义背景需要资源包。
- **hidden** 会隐藏该节点及其子节点，直到该节点完成（2017 年 JSON 格式页原文）。
- **页签可见性**：[Advancement definition](https://minecraft.wiki/w/Advancement_definition) 写的是"根节点所在树里有任意一个进度被授予后，根才作为页签出现"；1.12.2 是否严格如此〔待核〕（§13）。这与 2017 年页的描述"tabs are themselves advancements"一致。
- **客户端只认服务端给的树**：它不会自己生成 vanilla 树。若服务端不发包，按 L 只会看到空界面（当前还会被 INFO 文案打扰）。

因此：**对 1.12.x，服务端"只实现进度"是成立的**——只要把树与进度描述对，客户端就能画出来，不需要客户端 mod 或资源包（背景纹理除外，见上）。

### 3.3 1.8–1.11.2：只有内置成就，通道是统计

- 1.8 与 1.11 的协议页里**没有任何成就/进度包**（1.8 页有 Statistics 0x37；1.11 页有 Statistics 0x07，没有 Advancements）。客户端成就界面是内置的固定树；1.8 页的 Client Status 还有 action 2 "Open inventory achievement"（[1.8 协议页，oldid 2771712](https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2771712)），说明这个动作是客户端发起的。
- 服务端能发的是统计项：1.8–1.12 的 Statistics 包是"字符串名 + 数值"（[Protocol_1_8.cpp:1501-1520](../../src/Protocol/Protocol_1_8.cpp)），成就对应的名字形如 achievement.openInventory（[Protocol_1_8.cpp:4052+](../../src/Protocol/Protocol_1_8.cpp)；1.8 协议页的统计表把名字列表外链到一个 gist）。
- 上游 PR [#4789](https://github.com/cuberite/cuberite/pull/4789) 的描述把这件事说得更直接："Statistics and Achievements are tied in 1.8 -> 1.11"，"1.12 removed achievements from lookup"。
- 数量边界：到 1.11.2 一共 34 个成就；1.12 把其中 **13 个**改造成进度（[Achievement (Java Edition)](https://minecraft.wiki/w/Achievement_(Java_Edition))）。也就是说：
  - 服务端只实现进度时，老客户端最多能"对上"这 13 个；另外 21 个成就没有 1.12 进度对应物，要么保留旧成就逻辑，要么在 1.8–1.11 上直接消失。
  - 自定义进度/新页签在 1.8–1.11 上**没有任何通道**能显示。
- 1.9 的 /achievement 命令历史有一条"16w03a：/achievement no longer affects statistics"（[Commands/achievement](https://minecraft.wiki/w/Commands/achievement)）。1.9–1.11 上服务端到底怎么把成就状态送到客户端，本次**没有核实**〔待核〕；但"没有任意进度通道"这个结论不受影响。1.8 的链路是完整的（协议无包 + 命令参数是 achievement.<id> 统计名 + 本仓库既有实现）。
- "服务端主动 push 统计时客户端会不会弹成就 toast"〔待核〕：本机没有 1.8–1.11 客户端；现有实现只证明了"客户端 action 2 → 服务端 award → 发统计"这条路径（[Protocol_1_8.cpp:2441-2442](../../src/Protocol/Protocol_1_8.cpp)）。验证方法见 §12.4。

### 3.4 1.13 / 1.14.4：协议同形，但冻结

- 1.13 的 Advancements 仍是 0x4D、Select Advancement Tab 仍是 0x37（[1.13 协议页，oldid 2772385](https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2772385)）；1.14 变成 0x51 / 0x3A（[1.14 协议页，oldid 2772458](https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2772458)）。包结构与 1.12.2 相同（同一页的 Advancement/Display/Progress 表）。
- 按 [AGENTS.md §9](../../AGENTS.md)，1.13/1.14 处于冻结状态：**不扩展、不做 parity、不主动修**。建议实现时：
  - 给 pktAdvancements / pktSelectAdvancementTab 在 1.13/1.14 的 ID 表里补上正确值（因为包类型是全局枚举，不补会让 GetPacketID 回退到 1.12.1 的 0x4D，1.14 会发错包）；
  - 但**只对 1.12–1.12.2 下发**；1.13/1.14 保持 no-op（与今天一样是空界面，不新增行为）。
  - 是否让 1.13/1.14 也显示，是独立任务，需维护者下达。

### 3.5 直接回答

| 问题 | 答案 |
|---|---|
| 服务端只实现进度，1.12.2 客户端能正常显示进度吗？ | 能。树、标题/描述、图标、边框、背景、toast、隐藏、进度全在 Advancements 包里。 |
| 1.12.1 / 1.12 呢？ | 能，包结构相同，只是 ID 不同（0x4D / 0x4C）。 |
| 1.8–1.11 能看到同样的进度吗？ | 不能。没有通道；最多用统计驱动客户端内置的 34 个成就，且只有 13 个与 1.12 进度重叠。 |
| 1.13/1.14 呢？ | 协议上有，但按 §9 冻结；默认 no-op，另开任务才做。 |
| 需要客户端装 mod 或资源包吗？ | 不需要（5 个 vanilla 背景纹理除外；自定义背景需要资源包）。 |

## 4. 目标与非目标

**目标（1.12.2 客户端）**：

- 服务端权威的进度树：加载、校验、声明、进度、持久化。
- 逐条件进度与需求组语义（AND-of-OR）。
- toast（客户端）、聊天公告（服务端）、hidden / announce_to_chat / show_toast。
- /advancement grant|revoke|test（1.12.2 语法）。
- 奖励：经验、配方解锁、函数；战利品表暂缺（§8.3）。
- 1.8–1.11 的成就桥（可选阶段，§10）。

**非目标**：

- 1.13/1.14 parity（§9 冻结）。
- 1.8–1.11 的历史版本行为（AGENTS §9：只做封包格式翻译）。
- 完整 NBT 条件、完整战利品表、任意 datapack zip 加载。
- 客户端资源/语言文件（用客户端已有的；不引入 Mojang 资源）。

## 5. 数据模型与持久化

### 5.1 C++ 侧对象（建议）

- cAdvancementManager（挂在 cRoot，tolua_export 给 Lua）：持有 std::unordered_map<AString, cAdvancement>、父子索引、按触发器名索引的条件表；负责加载、校验、触发器分发、授予/移除。
- cAdvancement：ID、Parent、可选 Display、Criteria（名字 → {trigger, conditions}）、Requirements（空 = 全部条件）、Rewards。
- Display：title/description（原样 JSON 文本组件字符串）、icon（cItem）、frame 枚举、show_toast、hidden、background、x/y。
- cAdvancementProgress（每玩家每进度）：std::map<AString, Int64> AchievedCriteria（条件名 → 毫秒时间戳）。
- cPlayer 持有 std::unordered_map<AString, cAdvancementProgress> m_Advancements。

### 5.2 数据文件与加载

- 位置：Server/advancements/&lt;namespace&gt;/&lt;path&gt;.json（CWD 相对，和 crafting.txt 同一套读取方式，[CraftingRecipes.cpp:389](../../src/CraftingRecipes.cpp)）。
- 格式：采用 wiki 记录的 JSON 形状（display / parent / criteria / requirements / rewards），但**只支持子集**：
  - 支持的 trigger 与条件键见 §7；不认识的 trigger/条件在加载时报 warning 并让该条件永不满足（fail-closed，避免"看起来能拿其实拿不到"）。
  - display.icon 的 item/data 映射到 cItem；background 原样保留字符串。
  - 数据必须**自己按 wiki 写**，不得从 Mojang 的 data generator 导出文件直接引入（[AGENTS.md §2.2](../../AGENTS.md) 的数据报告边界）。
- 校验：重复 ID、悬空 parent、parent 环、requirements 引用了不存在的条件、根节点缺 display/background（warning）。
- 加载时机：与 cCraftingRecipes 一样在 cRoot 启动期加载；/reload 是否重载留给实现分支决定（先不做热重载）。

### 5.3 持久化

- 存进玩家 JSON（[Player.cpp:1940](../../src/Entities/Player.cpp) 的 SaveToDisk），形状建议：

    "advancements": {
      "minecraft:story/root": { "crafted_table": 1500000000000 }
    }

- 读回在 LoadFromDisk（[Player.cpp:1810](../../src/Entities/Player.cpp)）；未知/已删除的进度在加载时丢弃并 warning。
- **世界归属**：Cuberite 的玩家 JSON 是全局的，统计已注明 "shared between dimensions / worlds"（[Player.cpp:1824](../../src/Entities/Player.cpp)）。vanilla 的进度随世界存档保存（[Java Edition level format](https://minecraft.wiki/w/Java_Edition_level_format) 把玩家数据与 Advancement storage format 都列在世界存档下）；1.12.2 的具体存放位置〔推测：player.dat，依据是 1.12 已把 enteredNetherPosition 加进玩家数据供 nether_travel 使用，见 [Player.dat format](https://minecraft.wiki/w/Player.dat_format)〕。本实现建议先沿用"全局一份"（与统计一致），并在交付说明里标为已知偏差；若要 parity，再按世界名分键。

## 6. 协议层

### 6.1 新包类型与 ID

在 ePacketType 里加 pktAdvancements、pktSelectAdvancementTab（[Protocol.h:117](../../src/Protocol/Protocol.h) 附近），在 PacketTypeToStr（[Packetizer.cpp:119](../../src/Protocol/Packetizer.cpp)）补名字，然后：

| 协议类 | Advancements | Select Advancement Tab | 来源 |
|---|---|---|---|
| cProtocol_1_12 | 0x4C | 0x36 | [1.12 协议页，oldid 2772313](https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2772313) |
| cProtocol_1_12_1（1.12.2 继承） | 0x4D | 0x37 | [1.12.1 协议页，oldid 2772340](https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2772340)、[1.12.2 协议页，oldid 2772350](https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2772350) |
| cProtocol_1_13 | 0x4D | 0x37 | [1.13 协议页，oldid 2772385](https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2772385) |
| cProtocol_1_14 | 0x51 | 0x3A | [1.14 协议页，oldid 2772458](https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2772458) |

注意继承链（[Protocol_1_12.h:26-82](../../src/Protocol/Protocol_1_12.h)、[Protocol_1_13.h:28](../../src/Protocol/Protocol_1_13.h)、[Protocol_1_14.h:22](../../src/Protocol/Protocol_1_14.h)）：1.12.1 → 1.12 → 1.11.1 → …；1.12.2 继承 1.12.1；1.13 继承 1.12.2；1.14 继承 1.13.2。把 1.12.1 的表项写对，1.12.2 自动正确；1.14 必须显式覆盖，否则会回退到 1.12.1 的 0x4D。

### 6.2 发送方法

- cProtocol 加纯虚 SendAdvancements(...) 与 SendSelectAdvancementTab(...)；cProtocol_1_8_0 给 no-op（覆盖 1.8–1.11.2 全部客户端）；cProtocol_1_12 起实现真实序列化。
- cClientHandle 加对应转发（模式同 SendStatistics，[ClientHandle.cpp:3201](../../src/ClientHandle.cpp)）。
- 序列化细节按 §3.2 的表；物品走现有 WriteItem（[Protocol_1_8.h:240](../../src/Protocol/Protocol_1_8.h)），Chat 走原样 JSON 字符串（Pkt.WriteString）。
- 为可测试性，把 payload 序列化抽成不依赖 cClientHandle 的函数（见 §12.1）。

### 6.3 join 同步

- 在 [ClientHandle.cpp:465-489](../../src/ClientHandle.cpp) 的登录序列里，SendStatistics 之后发一次 SendAdvancements(reset=true, 全树, 全进度)。
- 只对 GetProtocolVersion() >= 335（1.12）发；1.8–1.11 走 no-op。
- 重进、换世界、重生：先按"每次进服全量"处理；跨世界是否重发留给实现分支验证（客户端在同一个连接里不会清空，重发 reset=true 是幂等的）。

### 6.4 页签包与热修

- 删除 [Protocol_1_12.cpp:1092-1096](../../src/Protocol/Protocol_1_12.cpp) 的 INFO 文案；页签包可以只记录（服务端不需要它来画界面）。
- 是否回复 Select Advancement Tab：1.12.2 的 wiki 说它在"客户端切页签或别的页签里有进度完成"时由服务端发；不回复不影响界面〔待核〕。建议第一版不回，等实机确认。

### 6.5 变更下发

- 条件达成/整个进度完成时，发 reset=false 的单条 Advancements：带该进度的声明（display，可选）+ 进度。带不带 display 更稳〔待核〕；建议带，客户端重复声明应当是幂等的。
- 移除（/advancement revoke everything）用第 3 段 removals。
- 多玩家同世界：进度是每玩家状态，不发广播包。

## 7. 触发层

### 7.1 1.12.2 的 28 个触发器

以 [2017-09-27 的 Advancement 页，oldid 1151757](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 "List of triggers" 为准（28 个）：

bred_animals、brewed_potion、changed_dimension、construct_beacon、consume_item、cured_zombie_villager、effects_changed、enchanted_item、enter_block、entity_hurt_player、entity_killed_player、impossible、inventory_changed、item_durability_changed、levitation、location、nether_travel、placed_block、player_hurt_entity、player_killed_entity、recipe_unlocked、slept_in_bed、summoned_entity、tame_animal、tick、used_ender_eye、used_totem、villager_trade。

### 7.2 分簇与调用点

| 簇 | trigger | 主要调用点（HEAD） |
|---|---|---|
| A 物品/背包 | inventory_changed | cInventory::OnSlotChanged（[Inventory.cpp:835](../../src/Inventory.cpp)；监听器注册在 [Inventory.cpp:28-31](../../src/Inventory.cpp)） |
| A | consume_item | cPlayer::FinishEating（[Player.cpp:527](../../src/Entities/Player.cpp)）；药水/牛奶等其它入口需在该分支补 |
| A | enchanted_item | cEnchantingWindow（src/UI/EnchantingWindow.cpp） |
| A | item_durability_changed | cInventory::DamageItem（[Inventory.cpp:514](../../src/Inventory.cpp)） |
| A | recipe_unlocked | cPlayer::AddKnownRecipe（[Player.cpp:2742](../../src/Entities/Player.cpp)） |
| A | placed_block | 方块放置路径（cPlayer::PlaceBlock；实现分支用 grep 定位） |
| A | brewed_potion | cBrewingstandEntity 完成酿造（src/BlockEntities/BrewingstandEntity.cpp） |
| A | villager_trade | cVillager（src/Mobs/Villager.cpp）+ 交易窗口 |
| A | tame_animal | cWolf / cHorse（src/Mobs/Wolf.cpp、src/Mobs/Horse.cpp） |
| A | bred_animals | cMonster 繁殖（[Monster.cpp:1469](../../src/Mobs/Monster.cpp) 附近已有 AchBreedCow） |
| A | summoned_entity | 生成蛋/召唤路径（实现分支定位） |
| A | construct_beacon | cBeaconEntity 激活（src/BlockEntities/BeaconEntity.cpp） |
| B 战斗 | player_hurt_entity / entity_hurt_player / player_killed_entity / entity_killed_player | cPlayer::Killed（[Player.cpp:896](../../src/Entities/Player.cpp)）、cPlayer::KilledBy（[Player.cpp:855](../../src/Entities/Player.cpp)）、伤害入口 |
| C 世界/状态 | changed_dimension | cEntity::MoveToWorld（[Entity.cpp:1670](../../src/Entities/Entity.cpp)） |
| C | nether_travel | 同上 + 玩家记录的"进入下界前坐标"（vanilla 用玩家数据里的 enteredNetherPosition，见 [Player.dat format](https://minecraft.wiki/w/Player.dat_format)；需新增持久字段） |
| C | location | 玩家 tick / 位置检查（实现分支定位，注意开销） |
| C | enter_block | 玩家进入方块（传送门等）路径 |
| C | slept_in_bed | cPlayer::SetIsInBed（[Player.cpp:496](../../src/Entities/Player.cpp)） |
| C | effects_changed | cEntity::AddEntityEffect / 移除效果路径 |
| C | levitation | 玩家 tick + 漂浮效果 |
| C | tick | cPlayer::Tick |
| C | impossible | 无自动触发，只能由命令/插件授予 |

cured_zombie_villager（治疗僵尸村民）在当前实现里没有对应逻辑，先不实现（该进度的条件永远不满足；或该进度不加载）〔待核：是否已有僵尸村民治疗代码〕。

### 7.3 条件子集

第一版支持：

- 物品：items（id、data、count、durability）、item；
- 实体：type（实体类型）、player（可选）；
- 方块：block（id、data）；
- 位置：dimension、position（range）、biome（若 API 可用）；
- 效果：effects；
- 距离/伤害等简单数值条件按需。

不支持：nbt、完整的 predicate / entity_properties 嵌套、战利品表条件。加载时对不支持的键 warning + fail-closed。

### 7.4 授予语义

- 条件达成：写入 AchievedCriteria（时间戳取 std::chrono::system_clock 毫秒）。
- 完成判定：requirements 为空 → 所有条件都达成；否则每个需求组至少一个条件达成（AND-of-OR）。
- 只在"首次从未完成变完成"时发公告/奖励/toast；重复授予幂等。
- 授予整个进度 = 把它所有条件都标记达成（用于 /advancement grant ... only）。
- 移除 = 清条件/清进度；revoke everything 用 removals 段或全量重发。

## 8. 公告、toast 与奖励

### 8.1 聊天公告（服务端）

- 键：chat.type.advancement.task / .goal / .challenge（1.12.2 客户端可译；task 已由 [chat 规格 R15](../chat/vanilla-1.12.2-client-translatable-messages.md) 实测，goal/challenge 同族〔待核〕）。
- 结构：{"translate":"chat.type.advancement.task","with":[{"text":玩家名},{"color":"green", …title 组件…, "hoverEvent":{"action":"show_text","value":…description…}}]}。
- 现有 cCompositeChat 的 AddClientTranslatedPart 把每个参数当字符串写（[CompositeChat.cpp:297](../../src/CompositeChat.cpp)），**不能**嵌组件；需要新增一个专用 part（参考 AddShowAchievementPart 的写法，[CompositeChat.cpp:338-340](../../src/CompositeChat.cpp)），或加一个"translate + 任意 JSON 数组参数"的通用入口。
- 广播范围：玩家所在世界（cWorld::BroadcastChat，[World.h:166](../../src/World.h)）；announce_to_chat=false 时不发。

### 8.2 toast

- 客户端行为，服务端只负责 show_toast 标志与进度从 false→true 的转换；不发额外包。
- 帧类型影响客户端 toast 的样式/音效（客户端行为）。

### 8.3 奖励

- 经验：cPlayer::DeltaExperience。
- 配方：cPlayer::AddKnownRecipe（[Player.cpp:2742](../../src/Entities/Player.cpp)）+ 发送解锁包。
- 函数：1.12.2 的 rewards.function 指 .mcfunction；仓库已有 /function（Core，[cmd_function.lua](../../Server/Plugins/Core/cmd_function.lua)）。C++ 侧可以通过 cPluginManager::ExecuteCommandFromOrigin（[Bindings/PluginManager.cpp:1377](../../src/Bindings/PluginManager.cpp)）以玩家为 origin 执行 function &lt;name&gt;。
- 战利品表：仓库没有通用战利品表子系统（只有结构箱子的 src/Generating/EndCityLootTable.*），rewards.loot 第一版不支持，加载时 warning。

## 9. /advancement 命令与 Lua API

### 9.1 语法（1.12.2）

来自 [Commands/advancement（2017-09 修订，oldid 1143335）](https://minecraft.wiki/w/Commands/advancement?oldid=1143335)：

    advancement <grant|revoke> <player> <only|until|from|through> <advancement> [criterion]
    advancement <grant|revoke> <player> everything
    advancement test <player> <advancement> [criterion]

- player 是名字或目标选择器（可多选）；advancement 是合法标识符；criterion 可选。
- 权限：op（infobox op，即等级 2）。
- until/from/through/everything 的遍历语义见页面原文（父链/子链）。
- 成功/失败的反馈文案 1.12.2 页面没给全〔待核〕；命令分支要用允许来源（wiki 的 1.12.2 命令页/实机）补齐，不许照 1.13+ 页面写。

### 9.2 实现分工

- C++：cAdvancementManager 的查询/遍历（父、子、全部 ID）、cPlayer 的授予/移除/查询。
- Lua（Core）：cmd_advancement.lua + Info.lua 注册 + tests/test_advancement.lua；选择器解析复用 Core 已有的 ResolveTargets / MakeSelectorContext（模式见 [cmd_xp.lua](../../Server/Plugins/Core/cmd_xp.lua)）。
- 导出给 Lua 的最小 API（需要同步 APIDump）：
  - cPlayer:HasAdvancement(ID) / HasAdvancementCriterion(ID, Criterion)
  - cPlayer:GrantAdvancement(ID) / RevokeAdvancement(ID)（+ criterion 重载）
  - cPlayer:GetAdvancementProgress(ID) → 条件名到时间戳的表
  - cRoot:GetAdvancementManager()；cAdvancementManager:GetAllIDs() / GetParent(ID) / GetChildren(ID) / IsValid(ID)
- 门 6：改了导出 C++ API 就必须同步 Server/Plugins/APIDump/ 并跑 APIDump 自检（AGENTS §4 门 6）。

## 10. 1.8–1.11 成就桥（可选工作流）

- 数据侧给进度加一个**非 vanilla 字段**（例如 "legacy_achievement": "AchDiamonds"），只在我们自己的数据文件里用。
- 完成该进度时，若玩家协议版本 < 335，调用现有 AwardAchievement（[Player.cpp:1379](../../src/Entities/Player.cpp)）→ 发统计 → 客户端内置成就界面可能更新。
- 13 个重叠的成就（按标题，id 留给数据分支）：Getting an Upgrade、Monster Hunter、Sniper Duel、Acquire Hardware、DIAMONDS!、We Need to Go Deeper、Return to Sender、Into Fire、Local Brewery、The End?、Adventuring Time、The Beginning?、Beaconator。
- 其余 21 个旧成就：要么保留现有触发代码（AwardAchievement 的旧调用点），要么放弃；这是维护者的产品决定，不在本 roadmap 里替选。
- 公告：1.8–1.11 上应继续用 chat.type.achievement（1.12.2 上不存在这个键，见 [chat 规格 R14](../chat/vanilla-1.12.2-client-translatable-messages.md)），所以公告要按协议版本分叉。

## 11. 分支拆分与里程碑

按"一功能一分支"（AGENTS §3），建议：

| 里程碑 | 分支 | 内容 | 规模〔推测〕 | 验收 |
|---|---|---|---|---|
| M0 协议降险 | feature/advancements-protocol | 新包类型/ID、SendAdvancements、join 同步、删除 INFO、硬编码 2 节点测试树 | S–M | 1.12.2 客户端按 L 能看到 2 节点树 + 授予时 toast；ProtoProxy 抓到 0x4D 包 |
| M1 核心模型 | feature/advancements-core | cAdvancementManager、JSON 加载/校验、进度模型、持久化、授予/移除、公告、奖励（经验/配方/函数）、单元测试 | M | 单元测试全绿；重启后进度还在；公告键可译 |
| M2 命令 | feature/core-advancement-command | Core /advancement + Lua API 导出 + APIDump | M | Core 测试；门 6；实机命令回显 |
| M3 物品簇 | feature/advancements-triggers-items | inventory_changed + 物品条件 + story 页签 | M–L | story 全树可在 1.12.2 上完成；条件匹配单测 |
| M4 战斗/世界簇 | feature/advancements-triggers-world | 战斗 4 个 + 世界/状态簇 + nether_travel 持久字段 | L | 对应进度实机完成 |
| M5 其余页签 | feature/advancements-tree-full | nether/end/adventure/husbandry 数据 + 剩余触发器（酿造/交易/繁殖/驯服/信标/治疗） | L | 全部节点校验；抽样实机 |
| M6 成就桥（可选） | feature/advancements-legacy-bridge | 13 个映射 + 版本分叉公告 | M | 有 1.8–1.11 客户端时实机；没有就只做单元 + bot 验证 |

注：数据量按 [2017-09-27 列表页](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 id 统计约 55 个节点（story 16 / nether 13 / adventure 10 / end 9 / husbandry 7；逐条以该页为准，数量本身〔待核〕）。每个节点的 title/description/icon/frame/parent/criteria 都要从 wiki 抄成我们的数据，不能导入 Mojang 文件。

## 12. 测试与证据计划

### 12.1 单元测试（C++）

- 新增 tests/Advancements/AdvancementsTest.cpp（IMPLEMENT_TEST_MAIN + TEST_*，FOLDER Tests/...，命名 <Feature>Test，按 AGENTS §4.2）。
- 覆盖：加载校验（重复/悬空/环/未知 trigger）、requirements 语义（AND-of-OR）、条件匹配（物品/实体/方块/位置）、进度 JSON 往返、授予幂等、公告 JSON 形状。
- 协议序列化：把 payload 写进 cByteBuffer 的纯函数 + 手工期望字节（1.12.2 一段小树），避免构造 cClientHandle。

### 12.2 Core 测试

- tests/test_advancement.lua：参数解析、only/until/from/through/everything 遍历、criterion、目标选择器多选、失败文案（按 1.12.2 规格）。

### 12.3 实机协议

- Tools/ProtoProxy 抓 1.12.2 客户端的登录与授予，逐字节核对包 ID、字段顺序、date、flags。
- mineflayer bot（1.12.2 协议）跑 /advancement 与公告文本（bot 能看聊天，不能看 UI）。

### 12.4 客户端 UI（需要人类连服务器）

- 1.12.2 客户端（本机 oracle 实例）连 build/Server：进服 → 按 L → 截图（空树/有树）→ /advancement grant @s only <id> → 截图（toast + 聊天公告）→ 重进 → 截图（进度保留）。
- 1.8–1.11 客户端：本机没有；成就桥的弹窗行为只能用真实客户端验证〔待核〕。验证步骤写进分支的交付说明。
- 本文写作时未跑观察：mcp__mcmm__get_minecraft_status 返回 {"connected": false, "port": null, "processAlive": false}（写作时探测），oracle 客户端未连接；因此 §3 的客户端行为条目均按文档来源书写，未实测的标〔待核〕。

### 12.5 验收门

- 动 src/：门 1 风格、门 2 构建、门 3 ctest（本机排除 UrlClient-test|Google-test）、门 5 行为规格、门 6 APIDump（导出 API 时）。
- 动 Core submodule：父仓库与 fork 成对提交（AGENTS §6），Core 测试 + luacheck。
- 纯文档分支（本分支）：只跑 [specs/README.md](../README.md) 的链接校验脚本。

## 13. 风险与〔待核〕清单

| # | 事项 | 状态 | 怎么核 |
|---|---|---|---|
| 1 | 1.12.2 客户端对"只带进度、不带 display"的更新是否接受 | 〔待核〕 | M0 实机：先发全树，再发 progress-only，看界面是否更新 |
| 2 | 页签可见性是否要求树内至少一个进度已授予 | 〔待核〕 | M0 实机：空进度时按 L 看是否有页签 |
| 3 | 5 个 vanilla 背景的资源位置字符串 | 〔待核〕/〔推测〕 | M0 实机逐个试；错误只表现为缺失纹理；字符串不能从 Mojang 文件抄，优先找 wiki 记载 |
| 4 | title/description 的 advancements.* translate 键是否存在 | 〔待核〕 | M0：发 {"translate":"advancements.story.root.title"}，看是否渲染成键原文（chat 规格已证明未知键回退为键原文） |
| 5 | goal/challenge 的公告键是否同 task 可译 | 〔待核〕 | M1：三种 frame 各发一条公告 |
| 6 | 1.8–1.11 服务端 push 统计是否触发成就弹窗 | 〔待核〕 | 需要 1.8–1.11 真客户端；本机没有 |
| 7 | 1.9–1.11 成就同步通道（/achievement 不再影响统计之后） | 〔待核〕 | 同上；先按 1.8 链路实现并标偏差 |
| 8 | 1.12.2 客户端跨世界/重生的进度缓存行为 | 〔待核〕 | M0：换世界再回来截图 |
| 9 | 1.12.2 日期字段的时区/单位（wiki 说 Date.getTime() 毫秒） | 已按 wiki 写 | 抓包只能对文档，因为没有独立 vanilla 1.12.2 专用服 |
| 10 | hidden 对子节点的确切语义 | 文档有，1.12.2 未实测 | M1 实机 |
| 11 | 无 NBT / 无战利品表 | 已知限制 | 数据加载 warning；不做 |
| 12 | 线程：触发器来自世界线程，发包走协议锁 | 设计约束 | 代码评审 + 压测 |
| 13 | 玩家 JSON 全局 vs vanilla 每世界 | 已知偏差 | 交付说明标注 |

## 14. 来源

- 上游 issue [#3770](https://github.com/cuberite/cuberite/issues/3770)（进度支持）；PR [#3845](https://github.com/cuberite/cuberite/pull/3845)（当前 INFO 热修）；PR [#4789](https://github.com/cuberite/cuberite/pull/4789)（统计/成就关系）。
- wiki.vg 协议页（经 Minecraft Wiki 迁移，按修订号引用）：
  - 1.8：<https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2771712>
  - 1.11：<https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2772275>
  - 1.12：<https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2772313>
  - 1.12.1：<https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2772340>
  - 1.12.2：<https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2772350>
  - 1.13：<https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2772385>
  - 1.14：<https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2772458>
- Minecraft Wiki 页面：
  - [Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757)：界面、5 页签、JSON 格式、28 个触发器、奖励、hidden/toast/announce、历史（1.12 取代成就）。
  - [Advancement definition](https://minecraft.wiki/w/Advancement_definition)：根节点/页签/背景缺失纹理的行为（当前版本页，1.12.2 未逐条实测）。
  - [Commands/advancement（2017-09 修订）](https://minecraft.wiki/w/Commands/advancement?oldid=1143335)：语法与遍历语义。
  - [Achievement (Java Edition)](https://minecraft.wiki/w/Achievement_(Java_Edition))：34 个成就、13 个被改造成进度。
  - [Commands/achievement](https://minecraft.wiki/w/Commands/achievement)：achievement give achievement.<id> 与 1.9 的历史。
- 仓库内规格：[chat 规格](../chat/vanilla-1.12.2-client-translatable-messages.md)（R14/R15：chat.type.achievement 在 1.12.2 不存在；chat.type.advancement.task 可译）、[command-layer roadmap:120](../commands/vanilla-1.12.2-command-layer-roadmap.md)（/advancement 列为需新 C++）。
- 链接校验：在仓库根运行 [specs/README.md](../README.md) 的校验脚本（断链数必须 0）。
