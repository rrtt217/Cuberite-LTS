# C3 命令方块对齐（成功计数 / 比较器 / 链与循环）

分支：`feature/command-blocks-parity` ｜ 级别：P1 ｜ 前置：C2（执行上下文）｜ 编排见 [roadmap](vanilla-1.12.2-command-layer-roadmap.md)

上游追踪：[issue #4888](https://github.com/cuberite/cuberite/issues/4888) 的 `Pre 1.0 → Command Blocks #3239 #5131` 与 `Version 1.5 → Renamed Command Blocks now use their name instead of @ in the chat #3239`。

## 1. 现状与差距（逐条实测）

| # | 现状 | 位置 | 应有 |
|---|---|---|---|
| 1 | 执行后**成功计数恒 0**（无条件赋值） | [CommandBlockEntity.cpp:204](../src/BlockEntities/CommandBlockEntity.cpp#L204) | 写入命令的真实成功计数；条件不满足时置 0 |
| 2 | 成功计数被**写进存档**但永远是 0 | 写 [NBTChunkSerializer.cpp:592](../src/WorldStorage/NBTChunkSerializer.cpp#L592)（`SuccessCount = GetResult()`）；读 [WSSAnvil.cpp:1085-1088](../src/WorldStorage/WSSAnvil.cpp#L1085) | 读写对称且值真实 |
| 3 | **比较器读不到命令方块**：后置区块实体只认 `cBlockEntityWithItems`，其余直接 `return false` | [RedstoneComparatorHandler.h:48-56](../src/Simulator/IncrementalRedstoneSimulator/RedstoneComparatorHandler.h#L48) | 命令方块后置比较器输出 = 成功计数（0–15） |
| 4 | `TrackOutput` **硬编码写 1**，读取时被忽略（留 TODO） | 写 [NBTChunkSerializer.cpp:593](../src/WorldStorage/NBTChunkSerializer.cpp#L594)；读 [WSSAnvil.cpp:1097](../src/WorldStorage/WSSAnvil.cpp#L1097) | 关掉时不保存/不显示上一次输出 |
| 5 | **链 / 循环方块完全不可用**：ID 已定义（210/211，[BlockType.h:229-230](../src/BlockType.h#L229)），区块实体工厂**只建 137**，其余落 `default` → `ASSERT` + `nullptr` | [BlockEntity.cpp:90](../src/BlockEntities/BlockEntity.cpp#L90)、[:112](../src/BlockEntities/BlockEntity.cpp#L112)；`IsBlockEntityBlockType` 同样只列 137（[:138 起](../src/BlockEntities/BlockEntity.cpp#L138)）；红石侧还额外 `ASSERT(GetBlockType() == E_BLOCK_COMMAND_BLOCK)`（[CommandBlockHandler.h:36](../src/Simulator/IncrementalRedstoneSimulator/CommandBlockHandler.h#L36)） | 三型都能放、能配、能跑 |
| 6 | 无**朝向**、无**条件模式**、无**延迟**字段 | [CommandBlockEntity.h](../src/BlockEntities/CommandBlockEntity.h) 全部成员只有 `m_ShouldExecute / m_Command / m_LastOutput / m_Result` | 朝向决定链目标与「后方」判定；条件/无条件；延迟 tick |
| 7 | 红石只认**上升沿** | [CommandBlockHandler.h:26-33](../src/Simulator/IncrementalRedstoneSimulator/CommandBlockHandler.h#L26) | 循环方块在激活期间每 tick 执行 |
| 8 | 以「无上下文控制台命令」执行，故输出前缀、原点都与控制台一致 | [CommandBlockEntity.cpp:195](../src/BlockEntities/CommandBlockEntity.cpp#L195) | 以方块位置为原点（依赖 C2）；输出前缀 `[x, y, z]` |
| 9 | 命令方块禁用管理员命令（`stop/restart/kick/ban/ipban` 白名单式拦截） | [CommandBlockEntity.cpp:184-191](../src/BlockEntities/CommandBlockEntity.cpp#L184) | **Cuberite 自有安全策略**，非 vanilla 行为 → 保留，在文档标注为有意偏差 |
| 10 | 世界开关 `CommandBlocksEnabled`（缺省 **false**） | [World.cpp:533](../src/World.cpp#L533)，判定在 [CommandBlockEntity.cpp:151](../src/BlockEntities/CommandBlockEntity.cpp#L151) | 与 vanilla `enable-command-block` 默认 `false` 一致（[Command block](https://minecraft.wiki/w/Command_block)）——**不是缺陷** |

## 2. 行为规格（1.12.2）

来源：[Command block](https://minecraft.wiki/w/Command_block)。**注意**：该页描述的是现行版本（1.13 红石大修后的 UI/术语，如 Impulse/Chain/Repeat 按钮、Always Active）。1.12.2 的 GUI 文案与 NBT 字段名必须另找 1.12.2 时期来源核对（版本历史小节 + 合法客户端实测）；下列带 **†** 的条目为**推测**，实现前须核实。

1. **三型语义**：脉冲（激活执行一次）、链（被前一方块**触发**时执行）、循环（激活期间每 tick 执行）。
2. **成功计数**：执行后置为该命令的成功计数；条件模式且后方方块未成功 → 置 0 且不执行。†（条件模式的 1.12.2 GUI 文案：`Conditional/Unconditional` + `Impulse/Stay loaded`）
3. **比较器**：后置比较器输出等于成功计数（0–15）。
4. **朝向**：决定链目标方块与「后方」方块；由方块元数据编码。†（1.12.2 元数据位定义需对照 [Block states](https://minecraft.wiki/w/Block_states) 的 1.12.2 版本）
5. **延迟**：脉冲/链的 `Delay`（tick）；循环方块首 tick 行为。†
6. **`TrackOutput`**：0 时不保存上一次输出（GUI 显示 `-`/空）。
7. **NBT**：`Command` / `SuccessCount` / `LastOutput` / `TrackOutput` / `Conditional` / `Auto` / `Delay` / `LastExecution` / `CustomName`。†（本仓库现只读写前三个 + 硬编码 `TrackOutput`）
8. **区块实体 id**：现读写 `"Control"`（+ 读时兼容 `minecraft:command_block`，[WSSAnvil.cpp:1071](../src/WorldStorage/WSSAnvil.cpp#L1071)），与仓库其它方块实体的「旧名 + 现代名」惯例一致（对比 [:921](../src/WorldStorage/WSSAnvil.cpp#L921) 的 Beacon）。1.12.2 存档的确切拼写**待用真实存档核实**。
9. **改名显示**：命名后的命令方块在聊天里以名字替代 `@`（上游 #3239）。
10. **协议**：客户端修改命令方块走 [ClientHandle.cpp:1042](../src/ClientHandle.cpp#L1042) / [:1072](../src/ClientHandle.cpp#L1072)，当前**只取命令字符串**，丢弃模式/条件/延迟/朝向字段 → 支持三型必须扩展该包的解析。

## 3. 本分支范围

**做**：
1. 成功计数真值：`Execute()` 用命令输出/结果回写 `m_Result`（依赖 C2 的 origin/输出通路）；条件不满足置 0。
2. 比较器扩展：[RedstoneComparatorHandler.h](../src/Simulator/IncrementalRedstoneSimulator/RedstoneComparatorHandler.h) 的后置查询增加「非容器方块实体」通道（避免继续用 `dynamic_cast<cBlockEntityWithItems *>` 单一路径），命令方块返回 `GetResult()`。
3. `TrackOutput` 落实为字段并在读/写/GUI 三处生效。
4. 三型支持：区块实体工厂与 `IsBlockEntityBlockType` 覆盖 210/211；红石 handler 去 `ASSERT`、按类型分派（脉冲=上升沿一次、循环=激活期每 tick、链=被触发一次）；朝向 + 条件 + 延迟字段与协议包解析。
5. NBT 读写补齐并加「写→读」往返测试。

**不做**：命令方块矿车（#3499，另立分支）、1.13+ 的 `Always Active`/`Unconditional` 新语义命名、`/function`。

## 4. 测试与验收

- **纯规则头 + 独立测试**（AGENTS §4.2，仿 [tests/Mobs/EndermanBlockRulesTest.cpp](../tests/Mobs/EndermanBlockRulesTest.cpp) + [src/Mobs/EndermanBlockRules.h](../src/Mobs/EndermanBlockRules.h) 的做法）：新增 `src/BlockEntities/CommandBlockRules.h`（类型 × 红石状态 × 条件 × 前一方块成功计数 → 是否执行 / 新成功计数 / 链触发目标）与 `tests/BlockEntities/CommandBlockRulesTest.cpp`，并入既有那**一个** `set_target_properties(... FOLDER Tests/...)`。
- **NBT 往返测试**：`Command`/`SuccessCount`/`LastOutput`/`TrackOutput` 写出后读回必须等价（含 `SuccessCount=15` 边界）。
- 六道门全适用（见 [roadmap §5](vanilla-1.12.2-command-layer-roadmap.md)）；若比较器扩展改了导出 API，跑门 4 + 门 6。
- 行为证据：无实机 oracle（[AGENTS.local.md §1](../AGENTS.local.md)）→ 规格逐条对表 + 单测；比较器与链式触发的手工验证需实机，列入「已知偏差」。

## 5. 已知偏差 / 风险

- §2 中带 † 的条目在核实前**不得**当作实现依据；若核实后与规格冲突，以规格为准并回改本文件。
- 与 vanilla 的**有意**偏差：管理员命令拦截（现状 #9）保留。
- 循环方块「每 tick 执行 + 排队到控制台队列」的组合可能把 [Server.cpp:447](../src/Server.cpp#L447) 的待执行队列打满（每 tick 一条）→ 需限制单 tick 处理条数并在交付说明里说明；这是 **C2** 队列改造的连带风险。
- 改动集中在 `CommandBlockEntity.*` 与红石模拟器，与 **C2** 同文件相邻区域 → **必须串行**（C2 先）。
