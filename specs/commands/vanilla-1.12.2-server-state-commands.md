# 纯 Lua 命令族（一）：`/save-off`、`/save-on`、`/setworldspawn`

分支：`feature/core-server-state-commands` ｜ 级别：P2 ｜ 前置：无（不消费选择器，纯 Core）｜
编排见 [roadmap §4.1](vanilla-1.12.2-command-layer-roadmap.md)（A 表「纯 Lua 可做」）

**为什么归为一支**：三条命令都只读写**服务端/世界的持久状态**（自动保存开关、世界出生点），
不涉及实体、方块批量操作、选择器或 JSON 文本，也不需要任何新 C++ 通路；单条规模 S（< 80 行 Lua），
共享同一批触点（[Info.lua](../../Server/Plugins/Core/Info.lua) 注册 + 控制台包装 + `SendMessage*` 反馈）。

## 1. 现状

| 命令 | Core 现状 | 所需 C++ API | 判定 |
|---|---|---|---|
| `/save-off` | 无；只有 `/save-all`（[cmd_saveall.lua](../../Server/Plugins/Core/cmd_saveall.lua)） | `cRoot:SetSavingEnabled`（[Root.h:143](../../src/Root.h#L143)，`// tolua_export`）、`cWorld:IsSavingEnabled`（[World.h:83](../../src/World.h#L83)）、`cRoot:ForEachWorld`（[Root.h:85](../../src/Root.h#L85)，ManualBindings 导出） | **API 齐，缺命令** |
| `/save-on` | 同上 | 同上 | **API 齐，缺命令** |
| `/setworldspawn` | 无；Core 自有 `/setspawn`（[cmd_setspawn.lua](../../Server/Plugins/Core/cmd_setspawn.lua)，只接受「把出生点设成自己脚下」，无参数形态） | `cWorld:SetSpawn`（[World.h:588](../../src/World.h#L588)，已导出；失败返回 false 表示写 INI 失败）、`cWorld:GetName`、`cCommandOrigin:GetWorld/GetPosition`（[CommandOrigin.h:57-60](../../src/CommandOrigin.h#L57)） | **API 齐，缺命令** |

三条都不是「要新通路」的缺口，纯粹是 Core 没有对应的 `cmd_*.lua`。

## 2. 行为规格

### 2.1 `/save-off`、`/save-on`

来源：[Commands/save 的 1.12.2 期修订](https://minecraft.wiki/w/Commands/save?oldid=1154843)
（rev 1154843，2017-10-02；1.13 于 2018-07-18 发布，该修订早于首个 1.13 快照 17w43a）。

1. 语法：`save-off`、`save-on`，**均无参数**（同页 `;Syntax` 只有命令本身）。
2. `save-off`：「Disables the server writing to the world files. All changes will temporarily be queued.」
   成功条件原文：「**Always succeeds unless already disabled.**」
   → 已经全部关闭时是**失败**（success count 0），不是幂等成功。
3. `save-on`：「Enables the server writing to the world files. This is the default behavior.」
   结果原文：「**Always succeeds unless already enabled.**」→ 已经全部开启时失败。
4. 服务器级开关：两页的描述都写 "the server"（不是 per-world）。Cuberite 的状态是 per-world 的
   （`cWorld::m_IsSavingEnabled`，[World.h:1004](../../src/World.h#L1004)），但
   `cRoot:SetSavingEnabled()` 会**遍历所有世界**统一设置（[Root.cpp:600](../../src/Root.cpp#L600)）
   → 用 `cRoot` 的入口即得到与规格相同的服务器级语义；「已处于目标状态」按**所有世界都已是
   目标状态**判定。
5. 权限等级：**4**〔待核：本页修订的 infobox 没有可抓取的权限字段；核法＝原版 1.12.2 专用服上
   用非 OP 玩家执行 `/save-off`，应回「You do not have permission」类失败〕。
6. 1.12.2 之后：`/save-off`、`/save-on` 在 1.13 被移除（现行页面把它们归入 Removed commands）
   → 按 [AGENTS.md §9](../../AGENTS.md) 只实现 1.12.2 形态，**不**为 1.13 做兼容。

### 2.2 `/setworldspawn`

来源：[Commands/setworldspawn 的 1.12.2 期修订](https://minecraft.wiki/w/Commands/setworldspawn?oldid=1123063)
（rev 1123063，2017-07-09）。

1. 语法：
   `setworldspawn` ｜ `setworldspawn <x> <y> <z>`（同页 `;Syntax`；**1.12.2 没有** 1.13 之后的角度参数）。
2. 参数：
   - `x`、`z`：「must be within the range −30,000,000 to 30,000,000 (inclusive)」；
   - `y`：「must be between 0 and 256 (inclusive)」——同页原文如此（256 是原文档的边界写法，
     实际可站立空间到 255）〔待核：原版 1.12.2 对 `y=256` 是接受并钳制还是报错；核法＝
     原版专用服执行 `/setworldspawn 0 256 0` 与 `0 255 0` 各一次〕；
   - 三个坐标都接受 `~` 相对记号，「relative to the command's execution」；
   - 「If not specified, defaults to the position of the command's execution. **Not optional in command blocks.**」
3. 作用对象：「Sets the world spawn」，即**执行者所在世界**的出生点，不影响其他世界。
4. y 的语义：非冒险模式下玩家总在 `(x, z)` 附近的最顶层方块上重生，y 实际上被忽略；
   冒险模式下按给定 y 精确重生（必要时向上找可站立空间）。——这是**重生逻辑**的规格，
   本条命令只负责把坐标写进世界；重生行为归 `cWorld::SetSpawn` 与既有的重生流程，本支不改。
5. 权限等级：**2**〔待核：同 §2.1 第 5 条；核法＝非 OP 玩家应被拒绝，OP 应成功〕。
6. 命令方块：无参数形态在命令方块里「not optional」——命令方块没有实体执行者，但**有位置**
   （[CommandOrigin.h](../../src/CommandOrigin.h) 的 `m_Position`）；本支按「用 origin 位置」实现，
   与「相对命令执行位置」一致〔待核：原版命令方块里 `/setworldspawn` 无参数的行为；
   核法＝命令方块里写入 `setworldspawn` 并激活，观察出生点〕。
7. 1.12.2 之后：1.13 起语法为 `setworldspawn [<pos>] [<angle>]`（角度参数），**不引入**。

## 3. 实现映射（Core，纯 Lua）

| 规格点 | 落点 |
|---|---|
| `/save-off`、`/save-on` 的开关与「已处于该状态即失败」 | 新 `cmd_save.lua`：`HasAnyWorldSaving(enabled)` 遍历世界判状态，再 `cRoot:Get():SetSavingEnabled()` |
| `/setworldspawn` 的坐标解析（绝对 / `~`、x/z ±3e7、y 0–256、缺省用 origin 位置） | 新 `cmd_setworldspawn.lua`，复用 C2 的 `ParseCommandCoords` / `GetOriginPosition`（[command_coords.lua:12,81](../../Server/Plugins/Core/command_coords.lua#L12)） |
| 世界选择 | `Origin:GetWorld()`（控制台/RCON 由 C++ 补成默认世界，[Server.cpp:482-486](../../src/Server.cpp#L482)）；玩家执行时即玩家所在世界 |
| 写入出生点 | `World:SetSpawn(floor(x), floor(y), floor(z))`；返回 false 时报失败（INI 写不进去） |
| 反馈 | 成功 `SendMessageSuccess`，失败 `SendMessageFailure`；控制台路径把同一文案作为返回值 |
| 命令注册 | `Info.lua` 的 `Commands` / `ConsoleCommands` 各三条（`/save-off`、`/save-on`、`/setworldspawn`） |

**保留 `/setspawn`**：它是 Core 的自有命令（把出生点设成执行者脚下，无参数），与 vanilla 的
`/setworldspawn` 并存不冲突；本支不改它的行为，也不把它做成别名（别名留给维护者决定，见 §4 第 4 条）。

## 4. 已知偏差与未核实项

1. **反馈文案**：vanilla 1.12.2 的原文（如 `/save-off` 的 "Automatic saving is now disabled"、
   `/setworldspawn` 的成功文案）本支按语义拟写，**未与原版逐字比对**〔待核：原版 1.12.2 专用服
   逐条执行并抄录输出；核法＝控制台与 OP 玩家各跑一次〕。C5 的先例把「成功反馈文本与原版不同形」
   记为偏差，本支同样处理。
2. **`y` 的边界 256**：规格页写 0–256，但方块坐标 y 到 255；本支接受 0–256 并原样交给
   `SetSpawn`（C++ 端按世界高度处理），超出则报错〔待核：见 §2.2 第 2 条〕。
3. **权限等级**：Core 的权限名是配置驱动的（`core.save-off` 等），不是 vanilla 的 0–4 数字等级；
   本支只按 Core 惯例给权限名，实际能否执行取决于 `config/permissions.ini`。等级 4 / 2 的来源见
   §2.1 第 5 条与 §2.2 第 5 条的〔待核〕。
4. **`/setspawn` 与 `/setworldspawn` 并存**：前者是 Core 历史命令，后者是 vanilla 名；是否让前者
   变成后者的别名（含权限名迁移）是**维护者决策**，本支不擅自合并或删除。
5. **`/save-all flush`**：1.12.2 的 `/save-all` 支持 `flush` 参数（同页），Core 现有 `/save-all`
   忽略多余参数、也不区分 flush；这是**既有缺口**，不属本支（本支只加 off/on），记入 roadmap。
6. **跨世界**：`/setworldspawn` 只作用于 origin 世界，这是 vanilla 语义（不涉及选择器，无跨世界候选）。
7. **顺带发现的既有缺口（本支不修，建议单独成支）**：`ParseCommandCoords()` 失败时返回
   `nil, reason` 两个值，而 `teleport.lua`、`cmd_spawnpoint.lua`、`cmd_summon.lua`、`console.lua`
   用四个局部变量接收并把**第四个**当 reason → 这四处命令的失败提示实际为 `nil`（静默无反馈）。
   本支自己的调用点已按真实返回形状搬移（见 `cmd_setworldspawn.lua` 的 `ParseSpawnPosition`），
   其余四处未动。

## 5. 测试与验收

### 5.1 纯 Lua 测试

`lua tests/test_server_state_handlers.lua`（Core 插件目录）→ **48 passed, 0 failed**。
覆盖的规格点：

- `/save-off`：有世界在保存时成功且调 `SetSavingEnabled(false)`；全部已关闭时不调用、返回失败文案、
  success count 0；部分世界关闭时仍然执行。
- `/save-on`：对称的四条。
- 两者拒绝多余参数且不改状态；控制台包装走同一实现并把文案作为返回值。
- `/setworldspawn`：写的是 origin 世界的出生点（另一世界不被触碰）；无参数用 origin 位置并向下取整；
  `~` / `~N` 按 origin 解析；y 0–256（256 接受、257 与负数拒绝）；x/z ±30,000,000（边界值接受、
  越界拒绝）；缺坐标、多坐标都拒绝且给出原因；`SetSpawn` 返回 false 时报失败；无 world 时不崩。

### 5.2 静态检查（按 [AGENTS.md §4.1](../../AGENTS.md) 选门）

| 门 | 结果 |
|---|---|
| 1 风格 | 不适用（无 `src/*.cpp|h` 改动） |
| 2 编译 | 不适用（纯 Lua；服务器二进制未改） |
| 3 ctest | 不适用（同上；纯 Lua 测试用 `lua` 直接跑） |
| 4 绑定依赖 | 不适用（无 `src/Bindings/` 改动） |
| 5 行为规格 | 本文件 + §5.1/§5.3 |
| 6 APIDump | 不适用（未新增/导出 C++ API） |

- `luac -p`：Core 目录全部 Lua 文件通过。
- `luacheck`（配置取自 `build/Server/.luacheckrc`）：`cmd_save.lua`、`cmd_setworldspawn.lua`
  **0 warning / 0 error**；`Info.lua` 从 232 条增至 238 条，**+6 全部是既有的 W113「handler 名未定义」**
  （新命令的 3 个 in-game + 3 个 console handler），无新类型；新加的表行未引入行尾空白。

### 5.3 真实服务器探针（控制台路径）

运行目录 `build/Server`（已有存档），stdin 逐条输入控制台命令，日志原文：

```
Executing console command: "save-on"
Automatic saving is already enabled
Executing console command: "save-off"
Automatic saving is now disabled
Executing console command: "save-off"
Automatic saving is already disabled
Executing console command: "save-on"
Automatic saving is now enabled
Executing console command: "setworldspawn 10 64 10"
Spawn set at {10, 64, 10}
Set the world spawn point to (10, 64, 10)
Executing console command: "setworldspawn"
Spawn set at {0, 0, 0}
Set the world spawn point to (0, 0, 0)
Executing console command: "setworldspawn 0 257 0"
The y coordinate must be between 0 and 256
Executing console command: "setworldspawn 30000001 0 0"
The x and z coordinates must be between -30,000,000 and 30,000,000
Executing console command: "setworldspawn 1 2"
No coordinate was given
Executing console command: "setworldspawn 1 2 3 4"
Usage: setworldspawn [<x> <y> <z>]
```

出生点持久化核对：`build/Server/world/world.ini` 的 `[SpawnPosition]` 在 `setworldspawn 10 64 10`
之后为 `X=10 Y=64 Z=10`（`Spawn set at {...}` 是 C++ 侧日志）。

### 5.4 未覆盖

- **玩家/命令方块路径**：本机无客户端，权限闸门与聊天栏反馈未实测；handler 测试用 fake player
  覆盖了调用面，权限名（`core.save-off` 等）由 `config/permissions.ini` 决定〔待核：真实客户端
  用非 OP 与 OP 各执行一次〕。
- **`/save-off` 的实际持久化效果**：只验证了开关状态与 API 调用；「关闭后世界不再写盘」依赖
  [World.cpp](../../src/World.cpp) 的既有实现，未构造磁盘观察〔待核：关闭后改方块、等自动保存周期、
  重启后确认方块仍在〕。
