# 纯 Lua 命令族（六）：`/function`

分支：`feature/core-function-command` ｜ 级别：P2 ｜ 前置：C4（`cPluginManager:ExecuteCommandFromOrigin`）｜
编排见 [roadmap §4.2](vanilla-1.12.2-command-layer-roadmap.md)

**为什么单独成支**：这是本批唯一要**读服务器文件**的命令，也是唯一把一组命令交给 C4 的
「带原点执行」通路去跑的命令；文件名解析、注释/空行规则、条件选择器与递归保护都只属于它。

## 1. 现状

| 命令 | Core 现状 | 所需 API | 判定 |
|---|---|---|---|
| `/function` | 无 | `cWorld:GetDataPath`（[World.h:703](../../src/World.h#L703)）、`cFile:IsFile` / `cFile:ReadWholeFile`（[ManualBindings.cpp:4690-4692](../../src/Bindings/ManualBindings.cpp#L4690)）、C4 的 `cRoot:Get():GetPluginManager():ExecuteCommandFromOrigin` | **API 齐，缺命令** |

## 2. 行为规格

来源：[Commands/function 的 1.12.2 期修订](https://minecraft.wiki/w/Commands/function?oldid=1126060)
（rev 1126060，2017-07-16）与
[Function (Java Edition) 的 1.12.2 期修订](https://minecraft.wiki/w/Function_(Java_Edition)?oldid=1153369)
（rev 1153369，2017-10-01）。两页都早于首个 1.13 快照（17w43a，2017-10-25）。

### 2.1 语法与条件

1. `function <function> [if|unless] [selector]`。
2. `if`：「Will only make the function run when an entity was found matching the selector」；
   `unless`：「when **no** entity was found」。Result 因此有三条失败：参数错误、`if` 未找到、
   `unless` 找到。
3. `if`/`unless` 与 `selector` 必须成对出现。
4. 函数的命令「run through the entity or command block that ran the /function command」——
   执行者与命令原点沿用调用者的。

### 2.2 函数文件

1. 位置：`[WORLD_NAME]/data/functions/<namespace>/<path>.mcfunction`（Function 页「Usage」；
   Commands/function 页的例子 `data/functions/custom/example/test.mcfunction` 一致）。
2. 名字：`namespace:path/to/file`；**省略 namespace 时默认 `minecraft:`**（Function 页原文）。
3. 内容：**一行一条命令，不带前置 `/`**；以 `#` 开头的行为注释；1.12.2 起注释只认 `#`
   （History：1.12 pre3「Comments can now only be preceded with #; using // is no longer allowed」）。
   命令可以长于命令方块的 32,500 字符限制。
4. 执行：一个函数的所有命令在**同一个 tick** 内跑完；函数里再调 `/function` 也在同一 tick。
5. 总量限制：函数内执行的总命令数受 `gamerule maxCommandChainLength` 限制（默认 **65,536**），
   超出的命令被忽略（Function 页）。
6. **相对坐标的基准**：Function 页明确「Position changes that happen within a function will not affect
   the relative coordinates used in other commands within that function (or within its child functions)
   until the next iteration」→ 一个函数执行期间，`~` 的基准是 `/function` 被调用时的原点，**不**随
   函数内传送而改变。本支把同一个 `cCommandOrigin` 传给每条命令，符合这一条。
7. 成功反馈：Function 页：「Executed [amount] command(s) from function '[function file directory]'」。
8. 1.12.2 之后：1.13 起文件移到数据包（`data/<ns>/functions/...`），并对语法做多项改动；
   **不引入**——本支只做 1.12.2 的 `data/functions/` 布局。

### 2.3 重新加载

Function 页：「If a function is modified or added, using the `/reload` command will reload the function
files from disk.」本支**不缓存**函数内容——每次调用都重新读盘，因此修改立即生效，`/reload` 无需
额外处理（与规格可观察行为一致，实现上更简单）。

### 2.4 权限与命令方块

1. 权限等级 2〔待核：infobox 不可抓；核法＝非 OP/OP 各一次〕。
2. 命令方块调用 `/function` 时，函数里的命令以命令方块的权限跑；C4 已定义命令方块不得执行
   `stop`/`restart`/`kick`/`ban`/`ipban`（[command_execute.lua:32](../../Server/Plugins/Core/command_execute.lua#L32)）。
   本支对每条函数命令复用同一判定（origin 不是玩家且位置不是 (0,0,0) 视为命令方块，见
   [cmd_execute.lua:115-121](../../Server/Plugins/Core/cmd_execute.lua#L115)），否则 `/function` 会成为
   绕过该规则的入口。

## 3. 实现映射（Core，纯 Lua）

| 规格点 | 落点 |
|---|---|
| 名字解析（默认 namespace、路径分段、拒绝 `..`/绝对路径）、`if\|unless` 参数、文件内容切行（`\r\n`、空行、`#` 注释） | 新 `command_function.lua`：`ParseFunctionLine()`、`NormalizeFunctionName()`、`ParseFunctionSource()`、`BuildFunctionPath()`（全部纯函数） |
| 读文件 | `cFile:IsFile()` / `cFile:ReadWholeFile()`；路径 = `World:GetDataPath() .. "/data/functions/" .. namespaced .. ".mcfunction"` |
| 条件 | `ResolveTargets(selector, MakeSelectorContext(Player, Origin), "entity")`，`#Targets > 0` |
| 执行 | 逐行 `cRoot:Get():GetPluginManager():ExecuteCommandFromOrigin(Origin, Player, Line)`；原点固定 |
| 命令方块禁用表 | 复用 `IsBlockForbiddenCommand()`（C4 导出） |
| 递归/总量保护 | 模块级深度计数（上限 32）+ 单次调用命令数上限 65,536 |
| 注册 | `Info.lua` 的 `/function` 与控制台 `function` |

## 4. 已知偏差与未核实项

1. **`maxCommandChainLength` 不是真实的 gamerule**：Cuberite 没有 gamerule 子系统（roadmap §4.1 B），
   本支用编译期常量 **65,536** 作为单次 `/function` 的命令上限，且按**每个 `/function` 调用**计数，
   而原版按整条命令链计数（命令方块里先 `/execute` 再 `/function` 会共享预算）〔待核：原版的确切
   计数边界〕。
2. **递归保护**：模块级嵌套深度上限 **32**，超出即失败并停止。原版没有独立的深度限制（由
   `maxCommandChainLength` 间接限制）〔推测：这是最接近的行为；置信度中〕。
3. **命令失败的传播**：函数内某条命令失败不会中止函数（原版行为），但本支**不把失败输出/错误
   汇总到 `/function` 的反馈里**；原版是否在聊天栏报告函数内错误未核实〔待核〕。
4. **成功文案**：`Executed N command(s) from function '<name>'` 取自 Function 页的英文原文，
   但大小写/单复数/函数名的写法（含不含 namespace）未与原版逐字比对〔待核〕。
5. **文件编码**：按字节读入、按 `\n` 切行并去掉 `\r`；UTF-8 BOM 会被当作第一条命令的一部分
   （Function 页建议「UTF-8 without BOM」）〔待核：原版对 BOM 的处理〕。
6. **`/reload`**：本支不缓存，所以行为上总是最新（见 §2.3）；Core 的 `/reload` 是重载插件，
   不会重读函数——两者不冲突，但语义与「重载函数文件」不同，记录在此。
7. **`gamerule gameLoopFunction`**：1.12.2 存在（Function 页），但 gamerule 子系统缺失，不属本支。
8. **命令方块检测**：沿用 C4 的启发式（origin 无玩家且位置非 (0,0,0)）——控制台在 (0,0,0)，
   因此控制台调用函数不受禁用表限制；如果控制台把原点设成别处，会被误判为命令方块〔推测：
   控制台 origin 恒为 (0,0,0)，置信度高〕。

## 5. 测试与验收

### 5.1 纯 Lua 测试

`lua tests/test_function_command.lua` → **44 passed, 0 failed**。覆盖：

- 名字解析：显式/默认 namespace、嵌套路径、拒绝 `..`、空段、绝对路径、尾斜杠、反斜杠、空 namespace、
  空格、`.` 段。
- 参数：缺名 Usage、`if`/`unless` 必须带 selector、selector 不能单独出现、多余参数拒绝。
- 文件内容：`#` 注释与空行跳过、CRLF 去 `\r`、只有注释/空文件为 0 条命令。
- 路径拼接。
- handler（stub 文件系统 + stub 命令执行器）：按文件顺序执行全部命令、反馈文案与 success count 0、
  文件不存在、`if` 无匹配不执行、`unless` 有匹配不执行、`unless` 无匹配执行、命令方块原点
  （非零位置且无玩家）拒绝 `stop`、非法名字在文件访问前被拒。

### 5.2 静态检查（按 [AGENTS.md §4.1](../../AGENTS.md) 选门）

| 门 | 结果 |
|---|---|
| 1 风格 | 不适用（无 `src/*.cpp|h` 改动） |
| 2 编译 | 不适用（纯 Lua） |
| 3 ctest | 不适用（同上） |
| 4 绑定依赖 | 不适用 |
| 5 行为规格 | 本文件 + §5.1/§5.3 |
| 6 APIDump | 不适用 |

- `luac -p`：新增/改动文件通过。
- `luacheck`：`command_function.lua`、`cmd_function.lua` **0 warning / 0 error**。
- `Info.lua`：新增 2 个 handler 名，按既有 W113 同型增长。

### 5.3 真实服务器探针（控制台，`build/Server`）

测试文件 `world/data/functions/test/hello.mcfunction`（3 条命令 + 1 条注释，不进仓库）：

```
# a comment
say hello from function
time set 6000
weather clear
```

控制台输出：

```
Executing console command: "function test:hello"
[Core] Time in world "world" was set to 6000
Executed 3 command(s) from function 'test:hello'
Executing console command: "function test:missing"
Function not found "test:missing"
Executing console command: "function test:hello if @e[type=pig]"
[Core] Time in world "world" was set to 6000
Executed 3 command(s) from function 'test:hello'
Executing console command: "function test:hello unless @e[type=pig]"
The selector matched an entity
Executing console command: "function test:../escape"
Invalid function name "test:../escape"
Executing console command: "function"
Usage: function <function> [if|unless] [selector]
```

注释被跳过（3 条命令而不是 4 条），`time set` 的输出证明函数内的命令真的执行了；
`if` 与 `unless` 对同一 selector 给出相反结果（探针世界里当时恰好有该类型的实体）。

### 5.4 未覆盖

- **相对坐标锚定**：§2.2 第 6 条（函数内传送不改变后续 `~` 的基准）没有在真实玩家/命令方块上验证；
  实现上每条命令拿到的是同一个 `cCommandOrigin`，handler 测试记录了「同一个 origin 传给每条命令」
  〔待核：真实客户端在函数里 `tp` 后再 `setblock ~ ~ ~`，确认基准不变〕。
- **嵌套函数与深度上限**：单元测试未构造 32 层嵌套；真实探针只跑了单层〔待核〕。
- **命令方块路径**：禁用表逻辑只由单元测试覆盖，未在真实命令方块上验证〔待核〕。
- **超大函数（65,536 命令上限）**：未构造。
- **玩家路径与权限闸门**：无客户端〔待核〕。
