# 严格 C：把命令执行移回 world tick（命令方块 parity）

> 目标分支：`feature/command-execution-world-thread`（从 `master` 切，独立于 C3）。
> 本文只覆盖"**命令在哪个线程、tick 的哪一点执行**"；`LastOutput` 的组件格式与聊天/死亡消息
> 属于 `specs/vanilla-1.12.2-chat-components.md`（另行）。
> 相关：`specs/vanilla-1.12.2-command-blocks.md`（C3 现状、§2.13 实测 NBT、§5 偏差 (b)）。

## 1. 现状与缺口（Cuberite）

命令方块执行路径：`cCommandBlockEntity::Execute()`（world 线程）→
`cServer::QueueExecuteConsoleCommand(..., *new CommandBlockOutCb(this), Origin)`（[Server.cpp:447](../src/Server.cpp#L447)）
→ 命令进入 `m_PendingCommands` → **`cServer::TickCommands()`（server tick 线程，[Server.cpp:771](../src/Server.cpp#L771)）**
→ `ExecuteConsoleCommand` → `cPluginManager::ExecuteConsoleCommand` → Core 的 `LuaCommandHandler`。

由此产生三处与 vanilla 的偏离：

| # | 偏离 | 证据 |
|---|---|---|
| 1 | **线程错位**：命令在 server tick 线程执行，却写 world 线程持有的方块实体（`CommandBlockOutCb::Out()` → `SetLastOutput`，[CommandBlockEntity.cpp:350-357](../src/BlockEntities/CommandBlockEntity.cpp#L350-L357)） | 代码 |
| 2 | **比较器不可能同 tick 看到结果**：比较器在 `m_ChunkMap.Tick`（[World.cpp:1350](../src/World.cpp#L1350)，world 线程、红石模拟内）读 `GetResult()`；而 `CommandExecuted` 由 server 线程在**不确定的时刻**同步调用，C3 之后选择器的动作还要等到 `TickQueuedTasks`（[:1354](../src/World.cpp#L1354)） | 代码 + 死锁 core 栈 |
| 3 | **成功计数拿不到"受影响数量"**：vanilla 1.12.2 写的是数量（实测 `kill @e[type=zombie]` → `SuccessCount=14`，见 C3 §2.13），我们只有"是否被处理"的布尔（偏差 (b)） | 实测 NBT |

## 2. vanilla 1.12.2 行为规格（来源）

1. **同 tick 链式执行**：一处被激活的命令方块在同一游戏刻内按链条顺序执行；"Chained command blocks execute in the same game tick in the order they are chained."（英文 [Command Block § Chained execution](https://minecraft.wiki/w/Command_Block)）
2. **比较器读最后一次执行的计数**：信号强度反映"最后一次执行"，方块取消激活后仍保留（同上 § Success count）。
3. **成功计数 = 命令的受影响数量**（1.12.2；现行页的"Java 0/1"是 1.13+ 行为，**不可用于 1.12.2**）：`kill @e[type=zombie]` 实测 `SuccessCount=14`，且比较器信号与数目一致（C3 §2.2/§2.13）。
4. **条件模式**：背后的命令方块未成功 → 本方块成功计数置 0；否则写入命令的成功计数（同上）。
5. **命令方块不调度到下一 tick**：1.12 起"Chains of command blocks no longer schedule their commands to be run in the *next* tick, they now execute all in the same tick they are read."（[oldid=1174241](https://minecraft.wiki/w/Command_Block?oldid=1174241) History, 1.12）

## 3. 设计

**核心**：命令方块的命令改为在**它所属世界的线程**、**world 锁已持有**的位置执行，使
world→plugin 成为唯一锁序，并让比较器在同一 tick 的红石评估中读到新计数。

1. **执行点**：命令方块的红石相位（`IncrementalRedstoneSimulator` 的 `CommandBlockHandler`）已经是在 world 线程、world 锁内被调用（`m_ChunkMap.Tick` → `cChunk::Tick` → 红石模拟）。命令在此**同步执行**，而不是 `QueueExecuteConsoleCommand`。
   - **顺序已用代码查清**（原先的〔待核〕解除）：

     | `cChunk::Tick`（[Chunk.cpp:695](../src/Chunk.cpp#L695)）内的顺序 | 行 |
     |---|---|
     | `TickBlocks()` | [:697](../src/Chunk.cpp#L697) |
     | 方块实体 tick：`KeyPair.second->Tick(...)` ← **命令方块在这里 `Execute()`** | [:700](../src/Chunk.cpp#L700) |
     | 实体 tick | [:708](../src/Chunk.cpp#L708) |
     | 红石模拟 `SimulatorManager->SimulateChunk(...)` ← **比较器在这里读 `GetResult()`** | [:757](../src/Chunk.cpp#L757) |

     所以"方块实体阶段同步执行 → 同一 tick 的红石阶段读新计数"就是 vanilla 的形状，无需额外调序。
   - **新发现的危险点（必须在实现中处理）**：[:700](../src/Chunk.cpp#L700) 正在**遍历 `m_BlockEntities`**；若 `Execute()` 在该循环内同步执行 Lua，命令可能增删方块实体（`/setblock`、破坏方块）→ **迭代器失效**。
     因此 `cCommandBlockEntity::Tick` 只应**登记**待执行的方块（按坐标），由 `cChunk::Tick` 在方块实体循环**之后**、红石模拟**之前**逐个用坐标重新取方块实体并执行：既避开迭代器失效，又保住"比较器同 tick 可见"。
   - 重复型方块由自身 `Tick` 触发（`CommandBlockHandler::Update` 只 `Activate()`，[:38-42](../src/Simulator/IncrementalRedstoneSimulator/CommandBlockHandler.h#L38-L42)），同样走这条"登记 → 循环后执行"的路径。
2. **锁序**：执行点已持 world 锁（`m_CSChunks` 可重入）→ 之后取插件 Lua 锁 = `world → plugin`，与 `TickMobs` 里的钩子同序；不会再出现 `plugin → world`。
3. **输出与计数回写**：`CommandBlockOutCb` 的 `Out()` / `CommandExecuted()` 在**同一线程**被调用，直接写方块实体；`SuccessCount` 取命令报告的**数量**。
4. **保留队列用于其它 origin**：console / RCON / 玩家命令仍走 `cServer`（它们本来就不在命令方块相位）。命令方块 origin 走同步路径；`cServer::TickCommands` 保留（向后兼容）。
5. **跨世界**：命令方块内的 `@e` 全局选择器可能触及别的世界。同步执行下，跨世界读写在**别的世界的锁**上进行——必须规定：跨世界部分要么仍按世界排队（C3 已实现的 `ApplyToTargets` 世界扇出），要么在持本世界锁时只做本世界部分。**本分支只保证 origin world 的同 tick 语义**，跨世界部分沿用 C3 的 `World:QueueTask` 扇出（其语义已在 C3 §5 记录）。
6. **`/execute` 与 `/function`**：1.12.2 的例外（计数转发）不在本分支；`/execute` 尚未实现（Core 无 `cmd_execute.lua`）。

### 3.2 加锁序不变量（玩家命令对齐）

**不变量 A：任何插件钩子都持有 origin 世界的 chunkmap 锁。**

| 来源 | 取锁点 |
|---|---|
| 命令方块 | `cChunk::Tick` → `RunQueuedCommandBlocks`，在 `m_ChunkMap.Tick` 的锁内 |
| 控制台 / RCON | `cServer::ExecuteConsoleCommand` 在 plugin 分发前取 origin 世界锁（引擎提交 `1ac6920`） |
| 玩家命令 | `cWorld::Tick` 把 `ProcessProtocolIn` 循环包进 `cWorld::cLock`（本提交，[World.cpp:1336](../src/World.cpp#L1336)） |

**不变量 B：不得在持有客户端锁时获取世界锁。**
本改动**首次**确立 `world → client` 序，因此 B 必须遵守。核实方法（可复核）：把
`cClientHandle` 的全部 `cCSLock` 行号
（140 / 257 / 286 / 374 / 557 / 585 / 639 / 689 / 758 / 2124 / 2136 / 2231 / 2244 / 2377 / 2395 /
2596 / 2623 / 3250 / 3425 / 3441 / 3510 / 3538）与 `ClientHandle.cpp` 里全部世界调用行号
（458 / 485 / 672 / 709 / 1169 / 1306 / 1345 / 1348 / 1377 / 1419 / 1430 / 1451 / 1510 / 1617 /
1705 / 1720 / 1797 / 1812 / 1853 / 2070 / 2097 / 2283 / 2298 / 2552 / 2582 / 3047）取交集 →
**为空**；并逐段读过较复杂的几处（`UnloadOutOfRangeChunks`、`StreamChunk`、`cClientHandle::Tick`、
`RemoveFromWorld`、`SendBlockChange` / `SendBlockChanges`）：`World->...` 调用一律在锁作用域之外，
`SendBlockChange` 甚至显式 `Lock.Unlock()` 之后才发。

**不变量 C：钩子与 `cWorld:QueueTask` 任务不得锁另一个世界**（C1 规格 §233 既有）。
`TickQueuedTasks`（[World.cpp:1354](../src/World.cpp#L1354)）在锁外运行，任务是 `plugin → world` 序；
D2 的排队任务因此只碰自己那个世界，回写再投回源世界（坐标规格 §7.2）。

**本改动不解决**：插件在自己的钩子里去锁**另一个**世界——那由 C 约束（Core 已由 D2 满足）。
**代价**：origin 世界的 chunkmap 锁现在罩住整条收包链（含 Lua 钩子），慢插件会放大它；
命令方块路径本来就是这个形态。**收益**：三种来源的钩子获得同样的原子性——钩子执行期间本世界不会被并发改写。

## 4. 前向兼容性契约（必须保持不变）

1. **插件 API**：`cPluginManager:ExecuteCommand` / `ExecuteConsoleCommand`、`cCommandOutputCallback` 的既有虚函数（`Out`、`Finished`、`CommandExecuted(bool)`）**不得改变语义**；新增能力只能**追加**（例如新增携带数量的重载/新虚函数，并让旧虚函数的默认实现转发到它）。
2. **Lua 命令返回约定**：`function(Split, Player, Command, Origin)` 返回 `(handled, message)` 保持不变；要报告数量必须走**新增的第三个返回值**或新 API，不得改变 `handled` 的含义（否则 Core 对"参数错/无匹配"返回 true 的既有行为会让这些命令变成 unknown command）。
3. **console / RCON / 玩家命令路径**：行为、输出、时序不变；`Settings.ini` 无新增必填项。
4. **命令方块的既有行为**：C3 已实现的类型/朝向/条件/`TrackOutput`/`auto`/NBT 字段语义不变；本分支只改"何时/在哪个线程执行"和"成功计数的值"。
5. **无头/工具构建**：不改协议；`Tools/` 与测试目标照旧可构建。

## 5. 测试与验收

- **单元**：执行点与计数回写用纯逻辑测试（把"命令方块相位 → 写计数"抽成可测函数，仿 `src/BlockEntities/CommandBlockRules.h` 的做法），覆盖：条件失败置 0、成功写数量、同 tick 顺序（命令方块先、比较器后）。
- **行为（实机）**：与 C3 §2.13 的存档/NBT 对照——命令方块执行 `kill @e[type=zombie]` 后，比较器信号 = 被杀数量、`SuccessCount` = 数量。
- **门**：AGENTS §4 全部门（改 `src/` → 1/2/3/5 恒适用；改 `src/Bindings/` 才触发门 4；导出 C++ API 才触发门 6）。

## 6. 待核 / 风险

- ~~红石模拟内"命令方块"与"比较器"的评估顺序~~ → **已解除**，见 §3.1 的顺序表（方块实体阶段 → 红石阶段，命令先、比较器后，同 tick）。
- **〔待核〕** 同步执行后，慢命令（`/kill @e` 全图）会拖长 world tick；需要预算/分片（与 C3 §5 的异步队列风险相反）。
- **风险**：`/stop`、`/restart` 等由 `cRoot` 处理的命令在 world 线程同步执行的副作用（本分支可继续走队列）。
- **风险**：`CommandBlockOutCb` 目前是 `*new` 出来、指针存进 `m_PendingCommands`（[Server.h:212-215](../src/Server.h#L212-L215)）——同步路径下它的生命周期需要重新设计。

## 7. 进度

- **计数通道（已完成，行为不变）**：`06b59d9` 给 `cCommandOutputCallback` 增加 `CommandExecuted(int)`（默认转发到旧 `bool` 重载 → 只覆写 bool 的既有子类仍会被调用）；`cCommandHandler::ExecuteCommand` 与 `cPluginManager::ExecuteConsoleCommand` 增加**可选**计数出参；Lua 命令处理器把第三返回值当计数读，用 `cOptionalParam<int>` 保证只返回 `(handled, message)` 的既有命令不受影响。引擎自身仍报 1，所以**此提交不改变可观察行为**。
- **命令上报计数（已完成）**：`rrtt217/Core` 分支 `feature/command-success-count`，单个提交 `38308ec`（**直接基于 C1 的 `8913d93`**）。选择器命令（kill/clear/effect/spawn/spawnpoint/gamemode/give）第三返回值为"`ApplyToTargets` 实际作用到的目标数"，单目标路径为 1，失败路径为 0。父仓库 [Server/Plugins/InfoReg.lua](../Server/Plugins/InfoReg.lua) 的多返回值天然透传（`MultiCommandHandler` 全部 `return Handler(...)`），无需改动。`/kill` 的值有实机依据（§2.3）；其余命令的精确值**〔待核〕**。
- **C3 的 world-tick 快照工作被本分支取代**（维护者判断）：那套（`target_snapshot.lua` + 枚举读快照 + 动作走 `World:QueueTask`）的前提是"命令必须在命令线程上避免取 world 锁"；严格 C 之后命令方块命令在 **world 线程、持 chunkmap 锁**上执行，于是实时枚举 + 立即动作重新变得安全，而且只有它才能给出**实际**受影响数量。因此 `feature/command-success-count` 不含快照：`target_snapshot.lua` 与其测试随 C3 分支保留、但在本分支的 Core 指针里不存在。
  **残留（已知）**：console / RCON 命令仍在 server tick 线程执行，而它们会经 Core 的实时枚举取 world 锁——即 `plugin → world` 这条边在这条路径上仍然存在（world 线程持 world 锁等插件锁的钩子仍在）。命令方块路径已消除；console/RCON 若要同样消除，需要引擎在这条路径上也先持目标世界锁（跨世界命令需另行定义锁序），本分支未做。
- **执行点（已完成）**：命令方块不再排进 `cServer` 队列。`cCommandBlockEntity::Tick` 只按**相对坐标**向区块登记（`cChunk::QueueCommandBlockExecution`），`cChunk::Tick` 在方块实体循环**之后**、红石模拟**之前**用坐标重取方块实体并 `Execute()`；`Execute()` 在本线程同步调用 `cServer::ExecuteConsoleCommand`（该函数由 private 改为 public，注释说明命令方块从 world 线程调用它）。`cCommandBlockOutCb` 改为栈上对象，方块实体在本线程被写。console / RCON 仍走 `QueueExecuteConsoleCommand`，行为不变。
  **安全处理（命令可能摧毁自身方块）**：输出回调不再持 `cCommandBlockEntity *`，改为持 `(cWorld *, Vector3i)`，每次 `Out()`/`CommandExecuted()` 都重新查找方块实体；朝向链触发改为 `static TriggerFacingChainBlockAt(cWorld *, Vector3i)`，命令返回后不再触碰 `this`。
  **待做（配套）**：Core 在"已持 world 锁的 world 线程"上应**直接动作**（而不是 `World:QueueTask` 排队），才能把 `SuccessCount` 从"计划数"变成"实际数"——见 §3.3 与 C3 规格 §5。
- **已修 Bug（GUI 选项回弹）**：`cChunk::DoWithBlockEntityAt()` 会把访问到的方块实体排队，在本 tick 末**重发给该区块所有客户端**（[Chunk.cpp:1792](../src/Chunk.cpp#L1792)）。执行路径若用它重取方块实体，Always Active 的循环方块就会**每 tick 重发一次**；1.12.2 客户端在命令方块 GUI 打开时按收到的数据重置控件，于是选项立刻回弹、**"确定"点不了**。现在执行/回写路径改用**无副作用**的取用：`cChunk::RunQueuedCommandBlocks()` 用 `cChunk::GetBlockEntityRel()`，输出回调 / 条件检测 / 朝向链触发用 `DoWithCommandBlockAt()`（= `cWorld::DoWithChunkAt()` + `cChunk::GetBlockEntity()`）。GUI 的两条路径（`UsedBy` 打开时、`MC|AutoCmd` 保存时）仍会广播方块实体，所以客户端能看到保存后的状态。
  **〔待核〕**：vanilla 1.12.2 是否在"每次执行后"向**其他**玩家推送该方块实体的 `LastOutput`（若推送，客户端对它的处理方式与我们的 0x09 重发是否等价）。当前实现只在打开与保存时推送。
  **成功次数已是 int（已修）**：`m_Result` 改为 `int`（vanilla 的 `SuccessCount` 是 int，NBT 侧 `CommandBlockNBT::m_SuccessCount` 本就是 `Int32`[CommandBlockNBT.h:21](../src/WorldStorage/CommandBlockNBT.h#L21)），内部使用者（比较器信号、NBT 读/写、1.8 的 Update Block Entity、条件检测、输出回调）统一走新增的 **`SetResultCount(int)` / `GetResultCount()`**；这两个方法放在 `// tolua_end` 之后，**不进 Lua API**。**已导出**的 `GetResult()`/`SetResult()` 保持原签名（字节宽度）作为兼容包装，所以 **`src/Bindings/Bindings.cpp` 未被重生成、门 6 不受影响**。
