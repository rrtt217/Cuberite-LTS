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
   - 需要先确认红石模拟与比较器读取的**相对顺序**，并把它固定为 vanilla 的顺序（命令方块先写计数、比较器后读）；这是本分支最关键的实现细节，须用测试钉死。
2. **锁序**：执行点已持 world 锁（`m_CSChunks` 可重入）→ 之后取插件 Lua 锁 = `world → plugin`，与 `TickMobs` 里的钩子同序；不会再出现 `plugin → world`。
3. **输出与计数回写**：`CommandBlockOutCb` 的 `Out()` / `CommandExecuted()` 在**同一线程**被调用，直接写方块实体；`SuccessCount` 取命令报告的**数量**。
4. **保留队列用于其它 origin**：console / RCON / 玩家命令仍走 `cServer`（它们本来就不在命令方块相位）。命令方块 origin 走同步路径；`cServer::TickCommands` 保留（向后兼容）。
5. **跨世界**：命令方块内的 `@e` 全局选择器可能触及别的世界。同步执行下，跨世界读写在**别的世界的锁**上进行——必须规定：跨世界部分要么仍按世界排队（C3 已实现的 `ApplyToTargets` 世界扇出），要么在持本世界锁时只做本世界部分。**本分支只保证 origin world 的同 tick 语义**，跨世界部分沿用 C3 的 `World:QueueTask` 扇出（其语义已在 C3 §5 记录）。
6. **`/execute` 与 `/function`**：1.12.2 的例外（计数转发）不在本分支；`/execute` 尚未实现（Core 无 `cmd_execute.lua`）。

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

- **〔待核〕** 红石模拟内"命令方块"与"比较器"的评估顺序，以及它与 vanilla 的对应关系（实现前先用本地红石测试或实机确认）。
- **〔待核〕** 同步执行后，慢命令（`/kill @e` 全图）会拖长 world tick；需要预算/分片（与 C3 §5 的异步队列风险相反）。
- **风险**：`/stop`、`/restart` 等由 `cRoot` 处理的命令在 world 线程同步执行的副作用（本分支可继续走队列）。
- **风险**：`CommandBlockOutCb` 目前是 `*new` 出来、指针存进 `m_PendingCommands`（[Server.h:212-215](../src/Server.h#L212-L215)）——同步路径下它的生命周期需要重新设计。
