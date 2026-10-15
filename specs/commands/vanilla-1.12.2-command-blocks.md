# C3 命令方块对齐（成功计数 / 比较器 / 链与循环）

分支：`feature/command-blocks-parity` ｜ 级别：P1 ｜ 前置：C2（执行上下文）｜ 编排见 [roadmap](vanilla-1.12.2-command-layer-roadmap.md)

上游追踪：[issue #4888](https://github.com/cuberite/cuberite/issues/4888) 的 `Pre 1.0 → Command Blocks #3239 #5131` 与 `Version 1.5 → Renamed Command Blocks now use their name instead of @ in the chat #3239`。

## 1. 现状与差距（逐条实测）

| # | 现状 | 位置 | 应有 |
|---|---|---|---|
| 1 | 执行后**成功计数恒 0**（无条件赋值） | [CommandBlockEntity.cpp:213](../../src/BlockEntities/CommandBlockEntity.cpp#L213) | 写入命令的真实成功计数；条件不满足时置 0（“真值”受 §5 决策项限制） |
| 2 | 成功计数被**写进存档**但永远是 0 | 写 [NBTChunkSerializer.cpp:592](../../src/WorldStorage/NBTChunkSerializer.cpp#L592)（`SuccessCount = GetResult()`）；读 [WSSAnvil.cpp:1085-1088](../../src/WorldStorage/WSSAnvil.cpp#L1085) | 读写对称且值真实 |
| 3 | **比较器读不到命令方块**：后置区块实体只认 `cBlockEntityWithItems`，其余直接 `return false` | [RedstoneComparatorHandler.h:48-56](../../src/Simulator/IncrementalRedstoneSimulator/RedstoneComparatorHandler.h#L48) | 命令方块后置比较器输出 = 成功计数（0–15） |
| 4 | `TrackOutput` **硬编码写 1**，读取时被忽略（留 TODO） | 写 [NBTChunkSerializer.cpp:594](../../src/WorldStorage/NBTChunkSerializer.cpp#L594)；读 [WSSAnvil.cpp:1097](../../src/WorldStorage/WSSAnvil.cpp#L1097) | 关掉时不保存/不显示上一次输出 |
| 5 | **链 / 循环方块完全不可用**：ID 已定义（210/211，[BlockType.h:229-230](../../src/BlockType.h#L229)），区块实体工厂**只建 137**，其余落 `default` → `ASSERT` + `nullptr` | [BlockEntity.cpp:90](../../src/BlockEntities/BlockEntity.cpp#L90)、[:112](../../src/BlockEntities/BlockEntity.cpp#L112)；`IsBlockEntityBlockType` 同样只列 137（[:138 起](../../src/BlockEntities/BlockEntity.cpp#L138)）；红石侧还额外 `ASSERT(GetBlockType() == E_BLOCK_COMMAND_BLOCK)`（[CommandBlockHandler.h:36](../../src/Simulator/IncrementalRedstoneSimulator/CommandBlockHandler.h#L36)） | 三型都能放、能配、能跑 |
| 6 | 无**朝向**、无**条件模式**（metadata），也无 `auto`/`powered`/`conditionMet`/`TrackOutput` 等 NBT 字段；**Java 1.12.2 本身没有 Delay 字段** | [CommandBlockEntity.h](../../src/BlockEntities/CommandBlockEntity.h) 全部成员只有 `m_ShouldExecute / m_Command / m_LastOutput / m_Result` | 朝向/条件按 §2.4 的 metadata 编解码；补齐 §2.7 的 NBT 字段 |
| 7 | 红石只认**上升沿** | [CommandBlockHandler.h:26-33](../../src/Simulator/IncrementalRedstoneSimulator/CommandBlockHandler.h#L26) | 循环方块在激活期间每 tick 执行 |
| 8 | 原点已由 C2 传入（方块位置 +0.5/+1/+0.5），但**输出前缀仍是控制台格式**（`[SUCCESS] …`），没有方块坐标 `[x, y, z]` | 原点传入 [CommandBlockEntity.cpp:204](../../src/BlockEntities/CommandBlockEntity.cpp#L204)；前缀 [CommandBlockEntity.cpp:172](../../src/BlockEntities/CommandBlockEntity.cpp#L172) | 输出前缀 `[x, y, z]` |
| 9 | 命令方块禁用管理员命令（`stop/restart/kick/ban/ipban` 白名单式拦截） | [CommandBlockEntity.cpp:184-191](../../src/BlockEntities/CommandBlockEntity.cpp#L184) | **Cuberite 自有安全策略**，非 vanilla 行为 → 保留，在文档标注为有意偏差 |
| 10 | 世界开关 `CommandBlocksEnabled`（缺省 **false**） | [World.cpp:534](../../src/World.cpp#L534)，判定在 [CommandBlockEntity.cpp:151](../../src/BlockEntities/CommandBlockEntity.cpp#L151) | 与 vanilla `enable-command-block` 默认 `false` 一致（[Command block](https://minecraft.wiki/w/Command_block)）——**不是缺陷** |
| 11 | **1.9+ 编辑界面拿不到数据**：右键能打开界面（客户端行为），但服务端从不下发 `Update Block Entity`(0x09, `Action 2`)，客户端的 “Done” 一直 disabled，`MC\|AutoCmd` 永不发出 | 使用路径 [CommandBlockEntity.cpp:32](../../src/BlockEntities/CommandBlockEntity.cpp#L32)（`UsedBy` 空实现）；放置路径 [Chunk.cpp:1412](../../src/Chunk.cpp#L1412)（`AddBlockEntity` 不入待发队列），对比会入队的 [Chunk.cpp:1393](../../src/Chunk.cpp#L1393) / [:1758](../../src/Chunk.cpp#L1758) | 玩家使用时下发一次 Action 2（MC-87209 + wiki.vg Protocol FAQ 直接要求）；放置后是否也需下发**〔待核〕**（见 §1.1、§2.11） |
| 12 | 区块实体 id 与 1.12.2 不符：写 `"Control"`、读只兼容 `"Control"`/`"minecraft:command_block"`，缺 1.11+ 的 `"command_block"` | 写 [NBTChunkSerializer.cpp:593](../../src/WorldStorage/NBTChunkSerializer.cpp#L593)；读 [WSSAnvil.cpp:1071](../../src/WorldStorage/WSSAnvil.cpp#L1071) | 读取兼容三种拼写；**写出改为 `minecraft:command_block`**（§2.13 实测定案） |
| 13 | 放置命令方块**朝向固定**（默认 meta=0，即朝下），不随玩家朝向变化；GUI 里改类型（`Mode`）被丢弃，只有条件位生效 | 放置 [ItemHandler.cpp:192](../../src/Items/ItemHandler.cpp#L192)（`cDefaultItemHandler`）；改类型 [ClientHandle.cpp](../../src/ClientHandle.cpp) 旧实现 | 按 §2.10 用 `Mode` 重建方块类型；按 §2.12 由玩家朝向决定放置 meta |

**本分支进度**：行 5 `358d365`；行 11（使用下发 Action 2）`26a8261`；行 4/6/12 的 NBT 读写与 `command_block` 读取兼容 `4c5bbbd`；行 3（比较器通道）`56605ed`；行 1–3 的成功计数（偏差 (b)：记 1/0；vanilla 1.12.2 是受影响数量，见 §2.2/§5）与条件执行门 `ca071c0`；行 6/7 的类型、朝向/条件、红石时序（纯规则 `6eacb33` + 实现 `568e247`）；行 6 的协议模式字段 `18a884d`；§4 的 NBT 往返测试 `1d51726`；行 13（放置朝向 + `Mode` 改类型）`51c9c7b`。**§3“做”清单已全部完成。** 运行期验证（隔离世界，跑后已删）：有效命令 `SuccessCount=1`、无效命令 `=0`；条件方块在“背后成功”时执行、在“背后失败”时被拦下。行 12 的**写出拼写**已由 §2.13 的实测存档定案为 `minecraft:command_block`（现仍写 `"Control"`），并按其实测新增三个偏差：`SuccessCount` 值语义、`LastOutput` 的 JSON 组件格式、`LastExecution` 的写出条件（见 §5）。另有一个**上游既有死锁**（world 锁 ⟷ 插件 Lua 锁，见 §5）导致命令方块场景仍可复现，尚未修；此前尝试的 `835b1d7` 无效已回滚（`8ef9b90`）。

### 1.1 编辑界面缺口（1.9+，实测）

**症状**：右键命令方块能打开编辑界面，但 “Done” 按钮是灰的（disabled），无法保存，因此服务端收不到 `MC|AutoCmd`、命令方块始终为空。这不是“命令方块未启用”（本机 `world/world.ini` 为 `CommandBlocksEnabled=1`、玩家为 Admin `*`），也不是保存包缺失。

**允许来源**：

- 官方行为：[MC-87209](https://mojira.dev/MC-87209)（Mojira，分类 Networking）——右键后界面先打开，在收到 block entity 更新包之前字段为默认值、**“Done” 按钮 disabled**；其建议原文即“让界面像箱子一样等更新包到达”。
- 包语义：[wiki.vg Protocol（1.12.2, protocol 340）](https://c4k3.github.io/wiki.vg/Protocol.html) clientbound `0x09 Update Block Entity`，`Action 2 = Set command block text (command and last execution status)`；serverbound Play 包表**没有**专用的“更新命令方块”包（1.13 才加）。
- 保存通道：[wiki.vg Plugin channels](https://c4k3.github.io/wiki.vg/Plugin_channels.html) `MC|AutoCmd`（client→server）；本仓库已实现（[Protocol_1_9.cpp:1253](../../src/Protocol/Protocol_1_9.cpp#L1253) → [ClientHandle.cpp:1042](../../src/ClientHandle.cpp#L1042)）。
- GUI 文案与 “Done/Cancel” 语义：[Minecraft Wiki: Command block](https://minecraft.wiki/w/Command_block)（现行页；1.12.2 文案待核）。

**抓包实测**（ProtoProxy，protocol 340；客户端与服务器同一段会话，`build/bin/Logs/`）：

1. 登录时目标区块 Chunk Data 报告 `0 block entities`；第一次右键后在 `[10.633]` 只收到 **Multi Block Change**：`<37,64,-99> block 2192 (type 137, meta 0)`（137 = 命令方块）——**没有** Action 2。
2. 随后两次右键使用同一命令方块（`[11.021]`、`[15.318]`），服务端**仍无**任何 `Update Block Entity`。
3. 同一抓包中，登录时下发的**旧**命令方块 `<151,38,-185>`（经区块下发路径 [ChunkSender.cpp:248](../../src/ChunkSender.cpp#L248)）**有** Action 2，NBT 含 `Command`/`SuccessCount`/`TrackOutput`/`CustomName`/`LastOutput`。
4. 全程 **0 条** `MC|AutoCmd`/`MC|AdvCmd` —— 与“Done 灰、客户端不发保存包”一致。

**结论**：缺的是 **server→client 的 Action 2 触发点**；MC-87209 直接要求的是**玩家使用时**这一次（vanilla 就是用它填界面并解锁 Done）。**放置后是否也需下发〔待核〕**（本抓包只证明放置时没有下发，未证明 vanilla 是否下发）；保存通道本身没问题。修复后 Done 才可点，`MC|AutoCmd` 才会到达现有处理器。

## 2. 行为规格（1.12.2）

来源（优先 1.12.2 时点版本）：[Command Block/BE, rev 1107062（2017-06-07）](https://minecraft.wiki/w/Command_Block/BE?oldid=1107062)（NBT 字段）、[Command Block, oldid=1150335（2017-09-24）](https://minecraft.wiki/w/Command_Block?oldid=1150335)（总述 / History / infobox）、[wiki.vg Protocol（protocol 340）](https://c4k3.github.io/wiki.vg/Protocol.html)、[wiki.vg Plugin channels](https://c4k3.github.io/wiki.vg/Plugin_channels.html)、[wiki.vg Protocol FAQ](https://c4k3.github.io/wiki.vg/Protocol_FAQ.html)。**注意**：现行 wiki 页描述的是 1.13+（`Impulse/Chain/Repeat` 术语、`Delay`、`Always Active` 文案）——**Java 1.12.2 没有 `Delay` 字段/属性**（那是 Bedrock 专属）；现行页“成功数通常 0/1”的说法也不适用于 1.12.2。

1. **三型语义**：脉冲（上升沿执行一次）、链（被指向它的方块触发时执行，且自身需充能）、循环（充能期间每 tick 执行）。（[Java Edition 1.9](https://minecraft.wiki/w/Java_Edition_1.9) Changes → Blocks → Command blocks）
2. **成功计数 `SuccessCount`**：= **该命令的成功数**；条件模式且背后方块未成功 → 置 0（现行[命令方块 §成功次数](https://zh.minecraft.wiki/w/%E5%91%BD%E4%BB%A4%E6%96%B9%E5%9D%97)：“If in Conditional mode, if the command block behind it didn't execute successfully, it sets its success count to 0. Otherwise, it sets it to the success count of the command.”）。
   **1.12.2 的取值 = 受影响数量（实机观察，本项目允许来源）**：在 vanilla Java 1.12.2 用命令方块执行 `/kill @e[type=zombie]`，比较器信号强度**等于被杀死的僵尸数**（杀得越多信号越强）。这与 1.12.2 时代修订 [oldid=1174241](https://minecraft.wiki/w/Command_Block?oldid=1174241)（2017-12-29）的措辞一致：“signal strength specific to the last executed command (**for example, the number of players affected by the command**)”；该页同时把 `SuccessCount` 与 `AffectedBlocks`/`AffectedEntities`/`AffectedItems`/`QueryResult` 列为**不同 stats**。
   **版本陷阱（务必记住）**：现行 [Command Block](https://minecraft.wiki/w/Command_Block) 页写 “**In Java Edition, success count is usually 0 or 1, except for `/function` and `/execute`**”——这是 **1.13+ 命令返回值重构之后**的 Java 行为，**不适用于 1.12.2**（现行 Wiki 默认描述最新版）。本项目 parity 只认 1.12.2 时代修订 / 版本页 / 实机观察。何时发生该变更**〔待核〕**。
   **本项目接受偏差 (b)**（维护者已确认，见 §5）：成功计 1、失败计 0。原因：Cuberite 的命令方块经 server tick 队列**异步**执行，`cServer::ExecuteConsoleCommand` 只能回传“命令是否被处理”，拿不到 1.12.2 的“受影响数量”（如 `/kill @e[type=zombie]` 杀掉几只）。注意这与“handled / succeeded”不是同一个问题：**值是数量，不是布尔**。
   **〔待核〕**：现行英页称 Java 版“leaving the command block GUI by clicking Done or pressing Return resets the success count to 0, regardless of whether changes were made”（该句自带 more-information-needed 标记），1.12.2 是否如此需查 1.12.2 时代修订或实机确认。另：中文现行页称条件制约的“背后是否成功”判定取**至少 1 游戏刻前**的状态，与我们读当前 `GetResult()` 的做法不一致，需核。
3. **比较器**：输出 = `SuccessCount` 的信号强度（0–15；>15 截断为 15**〔推测〕**）；方块 deactivated 后仍反映最后一次执行。（同上）
4. **朝向与条件（metadata，已实测确认）**：`facing = meta & 0x7`，取值 `0=down, 1=up, 2=north, 3=south, 4=west, 5=east`（活塞式编码）；`conditional = ((meta & 0x8) != 0)`。证据：1.12.2 Cleanroom 存档解析 + 三个 F3 点——`meta=1` ↔ `facing: up`、`meta=3` ↔ `facing: south`、`meta=5` ↔ `facing: east`，且均为 `conditional: false`；与活塞式编码完全一致。`0/2/4` 由同一编码推得（**〔推测〕**，但 1/3/5 已由实测锚定）。**wiki 的 `Command Block/DV` 子页已于 2024-03-24 删除**，这是目前唯一可用来源。
5. **时序（无 Delay）**：链自 1.12（17w16a）起**同一 tick 内按顺序执行**，本 tick 已执行过的方块不重复执行（循环只跑一次）。（[oldid=1174241](https://minecraft.wiki/w/Command_Block?oldid=1174241)）
6. **`TrackOutput`**（byte 0/1）：是否保存 `LastOutput`；GUI 用 “O/X” 按钮切换。（[Command Block/BE, rev 1107062](https://minecraft.wiki/w/Command_Block/BE?oldid=1107062)）
7. **NBT**（1.12.2 BE 字段表）：`Command`(string)、`SuccessCount`(int)、`LastOutput`(string)、`TrackOutput`(byte)、`powered`(byte)、`auto`(byte)、`conditionMet`(byte)、`UpdateLastExecution`(byte, 默认 true)、`LastExecution`(long)、`CustomName`(string)，外加 `CommandStats`(compound)。**`Conditional` 不在 1.12.2 的 BE 字段表内**（条件多半只由 metadata 承载）**〔待核〕**。来源同上。
8. **区块实体 id**：1.11（16w32a）起为 **`command_block`**（此前 `Control`）；1.13+ 为 `minecraft:command_block`。（[oldid=1150335](https://minecraft.wiki/w/Command_Block?oldid=1150335) History + infobox `entityid=command_block`）**本仓库现写 `"Control"`、读 `"Control"`/`"minecraft:command_block"`，缺 `"command_block"` → 读 1.12.2 存档会丢方块实体。**
9. **改名显示**：命名后的命令方块在聊天里以名字替代 `@`（上游 #3239）。
10. **协议 `MC|AutoCmd`**（client→server）：X/Y/Z(Int×3)、Command(String)、Track output(Bool)、Mode(String enum `SEQUENCE`/`AUTO`/`REDSTONE`)、Is conditional(Bool)、Automatic(Bool)（[wiki.vg Plugin channels, oldid 14089](https://c4k3.github.io/wiki.vg/Plugin_channel.html)）。**`Mode` 就是 GUI 里选的方块类型**，映射**已由 ProtoProxy 实测确认**：`SEQUENCE`=连锁、`AUTO`=循环、`REDSTONE`=脉冲（客户端改成连锁发出 `SEQUENCE`、改成循环发出 `AUTO`，`REDSTONE` 为脉冲默认）。所以改类型必须**重建方块**（id 137/210/211），不能只改 BE。
11. **编辑界面数据下发**：**使用时**服务端回 `Update Block Entity`(0x09, `Action 2`) 携带该方块 NBT，客户端正是用它打开/填充界面（[wiki.vg Protocol FAQ](https://c4k3.github.io/wiki.vg/Protocol_FAQ.html)）；否则 1.9+ 客户端 “Done” 保持 disabled（MC-87209，见 §1.1）。**放置时是否也发〔待核〕**。
12. **放置朝向**：命令方块自 1.9（15w34a）起有朝向（[Command Block](https://minecraft.wiki/w/Command_Block) History：“Added block facing for command blocks”）。**放置时玩家朝向↔facing 的映射 wiki 未记载**；本分支按**发射器/活塞的同一规则**实现（共用 `cDisplacementYawRotator`：方块朝向玩家；玩家在正上方时→`up`）。**〔待核〕**：需实机 F3 核对命令方块是否与发射器/活塞完全一致（含正上方/正下方两种边界）。

13. **实测 NBT（允许来源的实测观察）**：存档 `…/instances/cleanroom-0.6.13-alpha/minecraft/saves/新的世界/region/r.-1.0.mca`（区域文件内 11 个命令方块）。**来源性质**：客户端是 **Cleanroom 0.6.13-alpha + 26 个 mod** 的集成服务器（F3 截图可见），与纯 vanilla 可能有别，mod 敏感项另标〔待核〕。
    唯一执行过的方块 `(-71,66,314)`（F3：`facing: west`、`conditional: false`）：

    | 字段 | 实测值 |
    |---|---|
    | `id` | `minecraft:command_block` |
    | `Command` | `kill @e[type=zombie]` |
    | **`SuccessCount`** | **14**（= 被杀死的僵尸数；该方块比较器信号与数目一致，用户实测） |
    | `LastOutput` | **JSON 文本组件**：`{"extra":[{"translate":"commands.kill.successful","with":[{…实体 hover…}]}],"text":"[17:21:56] "}` |
    | `TrackOutput` / `UpdateLastExecution` / `LastExecution` | `1` / `1` / `13220` |
    | `auto` / `powered` / `conditionMet` | `0` / `1` / `1` |
    | `CustomName` | `@` |

    其余 10 个（`Command=""`、从未执行）：`SuccessCount=0`、`conditionMet=0`、`powered=0`、`auto=0`、`UpdateLastExecution=1`、`TrackOutput=1`、`CustomName="@"`、`LastOutput=""`，且 **`LastExecution` 字段不存在**。

    **据此确定 / 推进的项**：
    - **`SuccessCount` = 受影响数量**（1.12.2）。`kill @e[type=zombie]` 的**计划命中数**（我方 `ApplyToTargets` 的返回值）与实测 14 一致 → 计划数即同步可得的近似值。
    - **写出 id 应为 `minecraft:command_block`**（§2.8 的拼写取舍定案；我们现写 `"Control"`，那是 1.11 之前的拼写）。
    - **`LastOutput` 是 JSON 文本组件**（含 `[HH:MM:SS] ` 前缀与 `translate` 键），不是纯字符串；我们现写 `FormatChatPrefix(...) + text`（差距记入 §5）。`[时间]` 前缀是否 vanilla 原生**〔待核〕**（来源是 mod 环境）。
    - **`LastExecution` 的写出条件〔待核〕**：实测中 `UpdateLastExecution=1` 的从未执行方块**没有**该字段，执行过才有；我们目前无条件写出。

## 3. 本分支范围

**做**：
1. 成功计数：`Execute()` 回写 `m_Result`；命令是否被处理经新增的 `cCommandOutputCallback::CommandExecuted(bool)` 传回，再按偏差 (b) 记 1 / 0。**注意**：vanilla 1.12.2 的值是**受影响数量**（int，比较器读时截断到 15），不是布尔（§2.2/§5）。
2. 比较器扩展：[RedstoneComparatorHandler.h](../../src/Simulator/IncrementalRedstoneSimulator/RedstoneComparatorHandler.h) 的后置查询增加「非容器方块实体」通道（避免继续用 `dynamic_cast<cBlockEntityWithItems *>` 单一路径），命令方块返回 `GetResult()`。
3. `TrackOutput` 落实为字段并在读/写/GUI 三处生效。
4. 三型支持：区块实体工厂与 `IsBlockEntityBlockType` 覆盖 210/211；红石 handler 按类型分派（脉冲=上升沿一次、循环=充能期间每 tick、链=被指向它的方块触发）；朝向/条件按 §2.4 的 metadata 编解码（**已实测确认**）；`auto`（Always Active）按 §2.7 的 NBT/protocol 读写。**Java 1.12.2 无 Delay 字段**，不实现延迟。
5. NBT 读写补齐（§2.7 的字段）并加「写→读」往返测试。
6. 编辑界面数据下发（§1.1 / §2.11）：`UsedBy` 向使用者下发 `SendUpdateBlockEntity`；放置后是否需一并标记待发**〔待核〕**。
7. 区块实体 id 兼容：读取接受 `Control`/`command_block`/`minecraft:command_block`；**写出用 `minecraft:command_block`**（§2.8 + §2.13 实测定案）。
8. 放置朝向与 GUI 改类型（§2.10 / §2.12）：新增 `cItemCommandBlockHandler`（三个命令方块物品共用）按发射器/活塞的 `cDisplacementYawRotator` 规则设置放置 meta；`MC|AutoCmd` 的 `Mode` 用于重建方块类型（137/210/211）。

**不做**：命令方块矿车（#3499，另立分支）、1.13+ 的 `Always Active`/`Unconditional` 新语义命名、`/function`。

## 4. 测试与验收

- **纯规则头 + 独立测试**（AGENTS §4.2，仿 [tests/Mobs/EndermanBlockRulesTest.cpp](../../tests/Mobs/EndermanBlockRulesTest.cpp) + [src/Mobs/EndermanBlockRules.h](../../src/Mobs/EndermanBlockRules.h) 的做法）：新增 `src/BlockEntities/CommandBlockRules.h`（类型 × 红石状态 × 条件 × 前一方块成功计数 → 是否执行 / 新成功计数 / 链触发目标）与 `tests/BlockEntities/CommandBlockRulesTest.cpp`，并入既有那**一个** `set_target_properties(... FOLDER Tests/...)`。
- **metadata 编解码测试**：`facing = meta & 0x7` 的六向映射与 `conditional = meta & 0x8`，用已实测的三个锚点（`1↔up`、`3↔south`、`5↔east`）覆盖，其余三向按同一编码。
- **NBT 往返测试**：§2.7 的字段（`Command`/`SuccessCount`/`LastOutput`/`TrackOutput`/`powered`/`auto`/`conditionMet`/`UpdateLastExecution`/`LastExecution`/`CustomName`）写出后读回必须等价（含 `SuccessCount=15` 边界）。
- 六道门全适用（见 [roadmap §5](vanilla-1.12.2-command-layer-roadmap.md)）；若比较器扩展改了导出 API，跑门 4 + 门 6。
- 行为证据：无实机 oracle（[AGENTS.local.md §1](../../AGENTS.local.md)）→ 规格逐条对表 + 单测；比较器与链式触发的手工验证需实机，列入「已知偏差」。

## 5. 已知偏差 / 风险

- **已核实**（1.12.2 时点 wiki + 用户存档实测）：§2.4 metadata 编码、§2.7 NBT 字段、§2.8 区块实体 id、§2.11 “使用时下发”。原 spec 的 `Delay` 条目**已删除**——Java 1.12.2 没有该字段/属性（Bedrock 专属）。
- **仍标〔待核〕/〔推测〕**（实现到相关处前需再核或降级为近似）：§2.2 条件失败置 0 在 1.12.2 的适用性、§2.3 `SuccessCount>15` 截断、§2.4 的 `0/2/4` 朝向、§2.7 `Conditional` 是否也有 BE 字段、§2.11 放置时是否下发、§2.12 放置朝向是否与发射器/活塞完全一致。
- **成功计数已定 (b)**（维护者确认）：成功计 1、失败计 0，作为**有意偏差**记录。vanilla Java 1.12.2 写入的是命令的**受影响数量**（实机：`/kill @e[type=zombie]` 的比较器信号 = 被杀僵尸数；见 §2.2）；Cuberite 的命令方块经 server tick 队列**异步**执行，`cServer::ExecuteConsoleCommand` 只能回传“命令是否被处理”（新增的 `cCommandOutputCallback::CommandExecuted(bool)` 虚函数），拿不到该数量。落地：`src/Server.cpp`（`Finish()` 辅助）+ `CommandBlockEntity.cpp`（输出回调覆盖 + 条件模式执行门）；条件门的“背后方块成功”= `SuccessCount > 0`。
  **修复方向（不依赖严格 C）**：让命令结果区分「已处理」与「成功」，Core 的命令显式返回成功/失败，`CommandExecuted` 据此记 0/1。严格 C 只影响**时序 / 线程**（比较器同 tick 可见、从方块实体所属线程写入）与 `/execute`、`/function` 的计数转发，见下文 (C)。
- **由 §2.13 实测 NBT 新增的三个偏差（都**不依赖严格 C**，应先行）**：
  1. **BE id 写出**：现写 `"Control"`，实测 1.12.2 存档为 `minecraft:command_block` → 改 [NBTChunkSerializer.cpp:593](../../src/WorldStorage/NBTChunkSerializer.cpp#L593)。
  2. **`LastOutput` 格式**：实测是 JSON 文本组件（`[HH:MM:SS] ` + `translate` 键），我们写的是 `FormatChatPrefix(...) + text` 纯字符串 → 需按组件的序列化格式生成（且 `[时间]` 前缀是否原生〔待核〕）。
  3. **`SuccessCount`**：见偏差 (b)；选择器命令可用 `ApplyToTargets` 的**计划数**先顶上（`kill @e[type=zombie]` 实测 14 = 计划数）。需要把 `cCommandOutputCallback::CommandExecuted(bool)` 拓宽为携带计数。
- **§2.9 改名显示不在 §3“做”清单内**（roadmap 的一行提及过），本分支未实现：Cuberite 的命令方块输出只写进 `LastOutput`、不广播到聊天，所以上游 #3239 的“聊天里用名字替代 `@`”在现状下没有落点；`CustomName` 目前仍硬编码 `"@"`（只在 GUI 标题 / NBT 可见）。要落地需先决定是否广播命令方块输出。
- **死锁：world 锁 ⟷ 插件 Lua 锁（已用核心转储定位；尚未修）**。新 core（PID 1394400，commit `a064f38`）的两侧：
  - **world 线程**：`cWorld::Tick` → `TickMobs`（该函数**开头**就取 `cWorld::cLock`，[World.cpp:2147](../../src/World.cpp#L2147)）→ `cEnderman::Tick` → 环境伤害 → `CallHookTakeDamage` → `OnTakeDamage`（**要 Core 的 Lua 锁**）。core 里 `TickMobs` 帧的局部 `Lock`（`m_CS = world chunkmap`）确为 locked —— 整个 `TickMobs` 都在 world 锁内。
  - **命令线程**：`LuaCommandHandler::ExecuteCommand`（`give @p apple`，**持 Core 的 Lua 锁**）→ 目标选择器 `cWorld::ForEachPlayer`（**要 world 锁**）。

  即“world 锁 → 插件锁”与“插件锁 → world 锁”互等。**不是 C3 引入**：上游 master 至今同样是 `TickMobs` 持锁（`_X 2013_10_22 ... quick fix for #283`；#283 说明怪物 `Tick()` 直接访问 `cChunk`，必须在 chunkmap 锁内）。

  **上游 PR #3100（Merge）/ #3126（Revert）不能修这个死锁**：它们只是把 mob tick 从 `TickMobs` 挪进 `cChunk::Tick`，而 `cChunkMap::Tick` 同样**全程**持 `m_CSChunks`（上游 master 亦然），钩子仍在 world 锁内被同步调用。

  **候选修法（待维护者定方向）**：
  - **(A) 钩子侧：实体 tick 触发的插件钩子不在 world 锁内同步调用**（延后到释放锁后派发）。
    代价：实体钩子里有四个可否决 —— `OnSpawningEntity`（[Entity.cpp:130](../../src/Entities/Entity.cpp#L130)）、`OnTakeDamage`（[:434](../../src/Entities/Entity.cpp#L434)）、`OnEntityChangingWorld`（[:1673](../../src/Entities/Entity.cpp#L1673)）、`OnEntityTeleport`（[:2016](../../src/Entities/Entity.cpp#L2016)）—— 延后 = 动作已发生，插件失去否决与「改本次伤害」的能力；钩子看到的是事后状态；需要延迟队列 + 实体保活/UniqueID 复核；必须全量审计「持 world 锁时进插件 Lua」的路径（`TickMobs`、`cChunkMap::Tick`→`cChunk::Tick`、`CheckBlocks`、`ProcessProtocolIn` …），漏一条仍死锁；派发点必须既不持 world 锁也不持插件锁；破坏插件 API 的同步契约、需同步改 APIDump 文档。
    变体 **(A3)**：临时释放 world 锁、再同步调钩子（保留否决语义），但短暂违反 #283 的「mob tick 期间持 chunkmap」假设 → 需复核/保活，偏内存安全风险。
  - **(B) 命令侧：遍历世界不阻塞 world 锁**。玩家选择器（`@a/@p/@r`）可行且便宜：`cWorld::ForEachPlayer` 枚举的是 `m_Players`，但仍借 chunkmap 锁（[World.cpp:3261](../../src/World.cpp#L3261)），而 `m_Players` 的增删被要求「在 chunkmap 锁下」（[World.cpp:2280](../../src/World.cpp#L2280)）→ 给它独立锁 + 快照式枚举（快照后释放，再跑 Lua 回调）。代价：改写所有 `m_Players` 读写点、确立新锁序（world 线程不得持该锁进插件钩子）、快照语义。`@e` 贵：实体表在 `cChunkMap::m_Chunks`，需 world 侧维护不可变快照/RCU，且快照有延迟、实体可能已销毁（UniqueID 复核又要 world 锁 → 只能强引用保活）。**只改选择器是治标**：`World:GetBlock`/`SetBlock`/`ForEachEntity`/`GetEntitiesInBox` … 同样在持插件锁时取 world 锁，要根治等于重做 world 并发模型。
    注：任何「持插件锁时取 world 锁」都可能死锁，**缩短持锁时间无用**。
  - **(C) 命令侧：把「需要 world 锁的效果层」移出命令线程，作为世界任务执行**。机制现成：`cWorld::QueueTask`（[World.h:754](../../src/World.h#L754) / [World.cpp:3742](../../src/World.cpp#L3742)），由 `TickQueuedTasks()`（[World.cpp:2311](../../src/World.cpp#L2311)）在 `cWorld::Tick` 末尾执行（[World.cpp:1354](../../src/World.cpp#L1354)，晚于 `m_ChunkMap.Tick`(1350) / `TickMobs`(1351)），且任务在 `m_CSTasks` 之外运行（先拷出到期项再跑）→ 队列锁本身不构成新环。
    - **必须留在命令线程的同步壳**（都不需要 world 锁）：`bool`「是否被处理」（[ManualBindings.cpp:89](../../src/Bindings/ManualBindings.cpp#L89)）、输出 `a_Output->Out(s)`、权限/语法错误判定、命令方块 `SuccessCount`/`GetResult()` 与 `LastOutput`、插件 API 返回值 —— 它们都能由「命令名 + 参数 + 权限 + origin」决定。
    - **可以移出的是效果层**：选择器解析、改世界/实体/背包、传送、生成/杀死、掉落、计分板 —— 没有一个是「必须立刻返回」。
    - **锁序关键**：`TickQueuedTasks` 位于 world 锁**之外**，所以原样使用只是把 `plugin→world` 从 server 线程搬到 world 线程（选择器触及别的世界时仍可能跨世界反转）。正确做法是让任务执行点**处在 world 锁之内**（例如在 `TickQueuedTasks` 外再套 `cWorld::cLock`），则世界线程为 `world→plugin`（与钩子一致），而命令线程完全不取 world 锁 → `plugin→world` 这条边消失。
    - 其它代价：(1) 跨世界命令（`@e` 全局）没有唯一目标世界线程，需串行 fan-out 或限定 origin world；(2) 插件命令是任意 Lua（文件/网络/聊天/player 对象/按返回值分支），全量延后破坏插件契约；(3) 同 tick 可见性：vanilla 链式命令方块下一块能看到上一块的改动，延后至少一 tick；(4) 慢命令（`/kill @e` 全图）开销从 server tick 挪到 world tick，需预算/分片。
    - **加分**：C 顺带把命令方块从「server 队列异步执行」拉回「world tick 内执行」，更接近 vanilla 单主线程模型（对 C3 parity 有利）。
    - **阶段 1（Core-only，上游有同类判断）**：把 Core 选择器层的**枚举与动作**都移出命令线程。旁证（非 §2.2 白名单来源，仅作工程旁证；结论已逐条用代码核实）：2025-04-26 Cuberite 维护者 NiLSPACE 在 Discord 指出 `HOOK_TICK`（server tick 线程）配 `cRoot:ForEachPlayer`（要锁**每个**世界）会死锁，改成 `HOOK_WORLD_TICK`（world 线程）配 `cWorld:ForEachPlayer`（只锁本世界）即消失。代码核对：`cRoot::ForEachPlayer` 见 [Root.cpp:684](../../src/Root.cpp#L684)（逐世界调 `World.ForEachPlayer`），`cWorld::ForEachPlayer` 见 [World.cpp:3261](../../src/World.cpp#L3261)，`HOOK_TICK` 派发于 [Server.cpp:339](../../src/Server.cpp#L339)，`HOOK_WORLD_TICK` 派发于 [World.cpp:1309](../../src/World.cpp#L1309)（且在 world 锁之前）。我们的 Core 选择器（[target_selectors.lua:571](../../Server/Plugins/Core/target_selectors.lua#L571) 的 `cRoot:Get():ForEachPlayer`、[:473](../../Server/Plugins/Core/target_selectors.lua#L473) 的 `ForEachLoadedEntity`）正是同一形态。
      **关键约束**：`OwnedEntity = std::unique_ptr<cEntity>`（[ChunkDef.h:32](../../src/ChunkDef.h#L32)）——实体为独占所有权，**Lua 引用不能跨 tick 保活**，因此快照**不能**存实体引用，只能存**只读属性副本**；动作必须回到 world 线程按 UniqueID 重枚举后执行。
      实现落在 Core fork `rrtt217/Core` 的分支 `feature/world-tick-selector-snapshot`（已推送）：`b7b2728` 属性快照模块 + 单测；`05e3771` 选择器层改用快照（枚举、按名查找、动作排队）；`9a74acd` `/give` 不再按名字回查。父仓库 gitlink 指向 `9a74acd`。
      **已知残留**：`/enchant` 的响应依赖动作结果（无手持物品 / 不可附魔），改成排队动作就会把输出延后给调用者，因此仍按名字回查、仍取 world 锁；`ApplyToTargets` 现在返回**计划数**，目标在其世界任务执行前死亡时动作会少做（不漏报错、不多报成功）；过滤读到的是上一 world tick 的快照（最多约 50 ms 旧），这也是 VFC/VillagerTrade 已在用的取舍。
      验证：`lua tests/test_target_snapshot.lua`（31 passed）、`lua tests/test_selectors.lua`（153 passed），后者新增断言保证「解析选择器 / 解析命令目标**不枚举世界**」，以及 `ApplyToTargets` 的动作用 `World:QueueTask` 在目标世界重解析。
  - 注：此前那次「把 mob tick 移出 `ForEachEntity`」的修复对此**无效**（锁来自 `TickMobs` 的函数级 `cWorld::cLock`，与 `ForEachEntity` 无关），已回滚（`8ef9b90`）。
- 与 vanilla 的**有意**偏差：管理员命令拦截（现状 #9）保留。
- 循环方块「每 tick 执行 + 排队到控制台队列」的组合可能把 [Server.cpp:447](../../src/Server.cpp#L447) 的待执行队列打满（每 tick 一条）→ 需限制单 tick 处理条数并在交付说明里说明；这是 **C2** 队列改造的连带风险。
- **C2 已合并**（本地 `master`，`8af5d03`）：本分支从新 `master` 起，原先「与 C2 同文件相邻区域必须串行」的约束已解除；§1 行 1/8 的行号已随 C2 同步。
