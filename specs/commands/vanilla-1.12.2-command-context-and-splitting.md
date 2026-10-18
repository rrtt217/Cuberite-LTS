# C2 命令执行上下文与参数切分（`src/`）

分支：`feature/command-context-and-splitting` ｜ 级别：P0 ｜ 前置：无 ｜ 被依赖：C3、C4、C5 ｜ 编排见 [roadmap](vanilla-1.12.2-command-layer-roadmap.md)

三个**各自都太小**、但共享同一批触点（`cPluginManager` / `cServer` 命令入口）的改动合并为一个分支：
**(a)** 参数切分可容纳带空格的参数；**(b)** 命令执行上下文（原点世界/位置 + 执行者）；**(c)** 把输出通路契约写成文档并保持不变。

## 1. 现状与差距（逐条实测）

| # | 现状 | 位置 | 1.12.2 应有 |
|---|---|---|---|
| a1 | 命令按裸空格切分，无引号、无反斜杠转义 | [PluginManager.cpp:1258](../../src/Bindings/PluginManager.cpp#L1258)、[Server.cpp:474](../../src/Server.cpp#L474)、[PluginManager.cpp:359](../../src/Bindings/PluginManager.cpp#L359) | `/tell "First Last" hi` 一类带引号的**命令参数**必须可解析。注意：**1.12.2 的选择器本身不含空格/引号**（见 [C1 §3.2](vanilla-1.12.2-command-target-selectors.md)），故 `@a[name="First Last"]` **不是** 1.12.2 需求 |
| a2 | 仓库里**已有**带引号切分器，但两处语义不兼容，**不能直接替换** | [StringUtils.cpp:72-100](../../src/StringUtils.cpp#L72)：① 空 token 被丢弃（而 `StringSplit` 保留）→ Core 依赖 `Split[2] == ""` 的判定会失效（[cmd_clear.lua:21](../../Server/Plugins/Core/cmd_clear.lua#L21)、[cmd_kill.lua:18](../../Server/Plugins/Core/cmd_kill.lua#L18)、[cmd_tell.lua:49](../../Server/Plugins/Core/cmd_tell.lua#L49)）；② 只认 **token 首字符**为引号，`say foo"bar baz"` 这类 token 中部的引号段处理不了 | 需要**新写**切分函数：保留空 token 语义 + 支持任意位置引号段 + `\"` 转义 |
| b1 | handler 只拿得到 `cPlayer *`，控制台/命令方块为 `nullptr`，**没有原点世界/位置** | [PluginManager.h:198](../../src/Bindings/PluginManager.h#L198)、[ManualBindings.cpp:29-45](../../src/Bindings/ManualBindings.cpp#L29) | `@p`/`@e[x=..]`/`/execute` 都需原点；命令方块的原点是其自身坐标 |
| b2 | 命令方块以「无上下文控制台命令」执行 | [CommandBlockEntity.cpp:195](../../src/BlockEntities/CommandBlockEntity.cpp#L195) | 命令方块执行的相对坐标、`@s` 语义都应以方块为原点 |
| c1 | 玩家命令的 handler 返回值被丢弃（`a_Output = nullptr`） | [PluginManager.cpp:1301](../../src/Bindings/PluginManager.cpp#L1301) | **这是有意设计**：Core 的 handler 用 `SendMessage(Player, …)` 直接下发，返回值只作控制台输出用（[core_functions.lua:75-88](../../Server/Plugins/Core/core_functions.lua#L75)）。本分支**只加注释固化契约**，不改行为 |

## 2. 规格

### 2.1 切分（a）

新增 `StringSplitCommandLine(const AString & a_Command)`（放 [StringUtils.cpp](../../src/StringUtils.cpp)，与 `StringSplit` 并列）：

1. 以空白分隔；**连续空白产生空 token 的行为必须与 `StringSplit` 逐字节一致**（否则会破坏 Core 的 `Split[2] == ""` 判定——迁移时须逐处复核那三处调用）。
2. 双引号段内的空格不分隔，引号本身剥离；单引号同义（与既有 `StringSplitWithQuotes` 行为对齐，见 [StringUtils.cpp:90](../../src/StringUtils.cpp#L90)）。
3. 未闭合引号：取到行尾，**不报错**（现状即无校验）。
4. 三个入口全部切换：玩家命令、控制台命令、未知命令提示路径。
5. **不得**复用 `StringSplitWithQuotes`：它丢空 token（a2①）且只认首字符引号（a2②）。

> 规格来源说明：vanilla 1.12.2 的命令行引号语义**本轮仍未找到允许来源**（wiki 的 Commands 页描述的是 1.13+ brigadier 的分词；选择器里的引号是 17w45a 才加入的，见 [C1 §3.2](vanilla-1.12.2-command-target-selectors.md)）。故本条按「与既有 `StringSplitWithQuotes` 对齐 + 保留空 token」定为**工程约定**，标注为**〔推测〕**，开工前需以允许来源（合法客户端实测 / 1.12.2 专用文档）核实。

### 2.2 执行上下文（b）

引入 `cCommandOrigin`（新文件 `src/CommandOrigin.h`，具名类型遵循 [AGENTS.md §5](../../AGENTS.md)：坐标用 `Vector3d`，禁止三个裸 `double`）：

| 成员 | 玩家命令 | 控制台 | 命令方块 | RCON |
|---|---|---|---|---|
| 世界 | 执行者所在世界 | **默认世界**（〔推测〕、〔待核〕） | 方块所在世界 | 默认世界 |
| 位置 | 执行者位置 | `0,0,0`（〔推测〕） | 方块位置 +0.5/+1/0.5（〔推测〕、〔待核〕） | `0,0,0` |
| 执行者 | `cPlayer *` | `nullptr` | `nullptr`（vanilla：命令方块无 `@s` 目标） | `nullptr` |

接入点（**必须全部覆盖**，否则 `@p` 在部分入口静默失效）：

1. [PluginManager.h:198](../../src/Bindings/PluginManager.h#L198) `cCommandHandler::ExecuteCommand` 增参（或改传 `const cCommandOrigin &`）；
2. [PluginManager.h:355/358](../../src/Bindings/PluginManager.h#L355) `ExecuteCommand` / `ForceExecuteCommand`（tolua 导出）→ 同步 [AllToLua.pkg](../../src/Bindings/AllToLua.pkg) + 手写绑定 + [APIDump](../../Server/Plugins/APIDump/APIDesc.lua)（AGENTS §5「三处同步」，并跑 `CheckBindingsDependencies.lua`，门 4）；
3. [ManualBindings.cpp:29-45](../../src/Bindings/ManualBindings.cpp#L29) `LuaCommandHandler` 把 origin 作为**第 4 个 Lua 参数**追加（保持既有 3 参兼容，Core 无需同步改）；
4. [Server.cpp:447](../../src/Server.cpp#L447) `QueueExecuteConsoleCommand` 与 [Root.cpp:497](../../src/Root.cpp#L497) 队列元素携带 origin；调用方 [CommandBlockEntity.cpp:195](../../src/BlockEntities/CommandBlockEntity.cpp#L195)、[RCONServer.cpp:298](../../src/RCONServer.cpp#L298) 各自提供；
5. `cWorld::SetCommandBlockCommand`（[World.cpp:3609](../../src/World.cpp#L3609)）等「按方块取命令方块」的路径与 C3 一同复核。

**兼容性硬要求**：`InfoReg.lua` 的 handler 包装（[InfoReg.lua:127](../../Server/Plugins/InfoReg.lua#L127)）与全部 Core handler 签名只吃前 2–3 参，Lua 忽略多余实参 → 追加第 4 参不破坏现有插件。此项须写成测试（见 §3）。

### 2.3 命令名的斜杠形态（派发容错）

来源：[Function (Java Edition) 的 1.12.2 期修订](https://minecraft.wiki/w/Function_(Java_Edition)?oldid=1153369)
（rev 1153369，2017-10-01）写明函数文件「一行一条命令，**不带**前置 `/`」；
[Commands/execute 的 1.12.2 期修订](https://minecraft.wiki/w/Commands/execute?oldid=1130584)
（rev 1130584，2017-07-26）的 `<command …>` 同样是命令字符串，原版是否接受前置 `/`〔待核：
原版客户端执行 `/execute @s ~ ~ ~ /say hi` 的结果〕。

1. **引擎的两张表键约定不同**（C2 引入，实测）：
   - in-game 表 `m_Commands` 的键带前导 `/`——Core 的 [Info.lua](../../Server/Plugins/Core/Info.lua)
     写 `["/say"]`，[InfoReg.lua:142](../../Server/Plugins/InfoReg.lua#L142) 原样
     `cPluginManager:BindCommand()`；
   - 控制台表 `m_ConsoleCommands` 的键不带 `/`——同文件的 `ConsoleCommands` 写 `["say"]`，
     [InfoReg.lua:203](../../Server/Plugins/InfoReg.lua#L203) `BindConsoleCommand()`。
2. **规格**（本支实现）：派发查表**同时接受两种形态**，精确命中优先：
   - in-game 表：先查 `Split[0]`；未命中且 `Split[0]` 不以 `/` 开头 → 再查 `"/" + Split[0]`；
   - 控制台表：先查 `Split[0]`；未命中且以 `/` 开头 → 再查去掉 `/` 的形式；
   - 覆盖四个查表点：`HandleCommand()`（玩家 sender）、`ExecuteCommandFromOrigin()` 的
     无 sender 权限查表、无 sender 的错误归类查表、`ExecuteConsoleCommand()`。
     （**只修前两处不够**：`/execute @e[type=pig] ~ ~ ~ say hi` 这类「非玩家目标 + 玩家权限主体」
     会先走无 sender 权限查表，miss 之后仍会 `crUnknownCommand`。）
3. **不改变的东西**：`a_Split` 与原始命令串原样保留（`/execute` 依赖原串做字节切分）；
   权限仍取命中条目的 `m_Permission`；success count 与输出回调不变；
   `IsCommandRegistered()` / `GetCommandPermission()` / `TabCompleteCommand()` 仍是**精确名**语义。
4. **`HOOK_EXECUTE_COMMAND` 的可见变化**：`HandleCommand()` 对所有**命中**的命令调用该 hook
   （不限于带 `/` 的输入）。修复后，`/execute`、`/function` 产生的无 `/` 内层命令（此前直接
   `crUnknownCommand`、根本到不了 hook）会经过 hook——插件因此可以拦截这些命令，这与
   「命令按该玩家执行」一致，记录在案。
5. **有意的宽松**：控制台输入 `/say hi` 现在也能执行（此前 Unknown command）；原版控制台不接受
   前置 `/`〔待核〕。这是对称容错的代价，记为偏差。

## 3. 测试与验收

- **新测试**（AGENTS §4.2 风格，一个可执行 + 一个 `add_test`）：`tests/StringUtils/CommandLineSplitTest.cpp` —— 表驱动覆盖：普通切分、连续空格（与 `StringSplit` 逐元素比对）、token 中部引号（如 `say foo"bar baz"`）、未闭合引号、`\\"`。目标名与 `add_test` 名用 `<Feature>Test`，并入既有那**一个** `set_target_properties(... FOLDER Tests/...)` 调用。
- **切分入口回归**：`/msg "Player Name" hi`、`/tell PlayerName hi`、`/effect @a[...]`（依赖 C1）手测记录。
- **origin 通路**：新增 `tests/` 用例或在 `DumpInfo` 类插件里断言四个入口的 origin 非空且世界正确（禁止把断言写进 tick 线程；参考 [AGENTS.md §9](../../AGENTS.md) 线程模型）。
- **六道门全适用**（改了 `src/`）：见 [roadmap §5](vanilla-1.12.2-command-layer-roadmap.md)；因改导出 API，门 4（`CheckBindingsDependencies.lua`）与门 6（APIDump 自检：`NewlyUndocumented.lua` 无输出）必须跑。
- **派发两种形态（§2.3，本支补测）**：`cPluginManager` 的两张表没有可独立构造的单测环境（需要插件/世界），故用**真实服务器 + mineflayer bot**（独立端口 25599、Core = `feature/core-function-command` 的副本、bot `FixBot` 经 `op` 获得 Admin）实测：
  - `/execute @s ~ ~ ~ say inner-no-slash` → 聊天 `[FixBot] inner-no-slash`（玩家 sender + 无 `/`）；
  - `/execute @s ~ ~ ~ /say inner-with-slash` → `[FixBot] inner-with-slash`（带 `/` 仍可）；
  - `/function test:hello`（文件行**不带** `/`）→ `[FixBot] hello from function`、`Time was set to 6000`、`Changing to clear weather`，并报 `Executed 3 command(s)`；
  - `/function test:anchor`（`tp 0 120 0` 后 `summon pig ~ ~ ~`）→ bot 升到 y≈119（300 ms 后采样），猪生成在**原点** `[X:0 Y:100 Z:0]`；`/testfor @e[type=pig,x=0,y=100,z=0,r=1]` 命中、以 y=120 查询不命中 → origin 锚定成立；
  - `/execute @e[type=pig] ~ ~ ~ /say oink-slash`（**非玩家目标 + 玩家权限主体 + 带 `/`**）→ `[SERVER] oink-slash`，覆盖无 sender 权限查表与错误归类两处；
  - 控制台 `/say console-slash` → bot 聊天 `[SERVER] console-slash`（§2.3 第 5 条的宽松行为）；
  - 回归：`/tps`、`/time set 6000`、`/say direct-say` 正常。
- **本支实际门结果**：门 1 `CheckBasicStyle.lua` 0 violations；门 2 `cmake --build build` 通过；门 3 `ctest -E "UrlClient-test|Google-test"` **49/49**；门 4 `CheckBindingsDependencies.lua` 无输出（通过）；门 5 见上（本文件 + [C4 §3.3](vanilla-1.12.2-execute-and-testfor.md)）；门 6 APIDump 自检仍只列 C2/C4 已记录的**同 6 个既有**未文档化符号（`cEnderCrystal.HasRespawnBeam` 等），本支未新增导出符号（两个查找助手是 private 成员），`DuplicateDocs.txt` 未生成。

## 4. 已知偏差 / 风险

- **最高风险**是 (a) 的空 token 语义变更：Core 三处 `Split[x] == ""` 判定依赖它（a1 表）。缓解：新函数保持语义 + 专门单测 + 迁移复核清单写进 PR 描述。
- 追加 handler 参数会让**第三方插件**的 C++ 命令处理器（若有）需重编译；Lua 插件不受影响（见 §2.2 兼容性要求）。
- 表中标 **〔推测〕/〔待核〕** 的默认原点（控制台/命令方块的世界与位置、相对坐标偏移）必须在实现前用允许来源核实，或降级为「实现时留 TODO + 在交付说明列已知偏差」。
- 本分支**不**实现选择器（C1）、**不**改命令方块行为（C3）；若为传 origin 而顺手改命令方块成功计数，属混做，禁止。
