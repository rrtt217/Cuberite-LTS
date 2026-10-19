# 既有命令修复：四处静默失败 / nil 解引用

分支：`feature/preexisting-feedback-fixes` ｜ 级别：P2（缺陷修复）｜ 前置：无

合并后全命令边缘测试（`.testrun/preexisting-report.md`）里剩下的四条：都是**输入非法时没有
可读反馈**或**对 nil 直接取字段**，与大功能无关，逐条修掉即可。

## 1. `/give` 的数量/数据值非法时无反馈

**修复前**：`/give @s stone 0`、`/give @s stone -1` 完全无反馈；`/give @s stone 1 -1` 同样。
根因是 `give.lua` 的 `GiveItemCommand()`：

```lua
if not PlayerName or not ItemName or Amount < 1 or DataValue < 0 then
    return false        -- 静默返回；选择器路径还忽略这个返回值
end
```

`/item stone 0` 走同一个函数，但它走的是名字路径、调用方在 false 时补发 Usage，所以
`/give`（选择器路径）什么都不显示而 `/item` 显示 Usage —— 两者不一致。

**修复**：数量 < 1 或数据值 < 0 时像既有的「Amount must be a number」一样发一条明确消息并
把命令视为已处理：

- `The number you have entered ( %d ) must be at least 1`（数量）
- `Data must be at least 0`（数据值）

**来源**：[Commands/give](https://minecraft.wiki/w/Commands/give)：数量是物品个数（必须为正），
Result 规定参数不合法即 unparseable/失败。原版的具体文案未逐字核对，本支沿用 Core 既有风格
（`The number you have entered ( %d ) is too big …` 的同款句式）〔待核：原版 1.12.2 的逐字文案〕。

## 2. `/time … [world]` 指定不存在的世界时静默

**修复前**：`/time set 100 x`、`/time add 100 x`、`/time day x`、`/time query daytime x`、
`/time query gametime x` 全部**无任何输出**（`time.lua` 的 `Common*Time` 在 `World == nil` 时
直接 `return true`）。

**修复**：新增 `GetTimeWorld(Split, index, Player)`，世界不存在时发
`There is no world "<name>"`（游戏内聊天 / 控制台日志），与既有的 `/weather clear x` 文案一致。
`[world]` 是 Core 扩展（原版 `/time` 没有世界参数），所以这里没有原版文案可对，参照同类命令。

## 3. `/r` 在回复目标无法解析时抛 Lua 错误

**修复前**：`/r hi` 在「有上一条私聊、但目标 UUID 解析不出名字」（离线模式，或 Mojang API 不认识）
时回 `Something went wrong while executing command "/r hi"`，日志里是
`cmd_tell.lua:42: FindAndDoWithPlayer: Expected a non-empty string for parameter #1`。

**修复**：`GetPlayerNameFromUUID()` 返回 nil/空串时改发
`Could not find the player you are replying to`，不再把空串传给 `FindAndDoWithPlayer()`。

**来源**：`/r` 是 Core 扩展（vanilla 1.12.2 没有 `/r`），行为以「不抛内部错误、给出可读原因」为准。

## 4. `/scoreboard players …` 对不存在的目标抛 Lua 错误

**修复前**：`/scoreboard players set|get|add|remove|reset|operation <玩家> <不存在的目标> …`
回 `Something went wrong while executing command …`，日志里是
`cmd_scoreboard.lua:148/165/174: attempt to index local 'TargetedObjective' (a nil value)`。

**修复**：新增 `GetObjectiveOrReport(Scoreboard, name, Player)`，目标不存在时发
`Unknown objective "<name>"` 并结束命令；`operation` 的目标与来源两个目标都检查。
正常路径不受影响（实测 objectives add → players set/get → objectives remove 全部照常）。

**来源**：[Commands/scoreboard](https://minecraft.wiki/w/Commands/scoreboard)：players 子命令
作用于一个已存在的目标，参数不合法/目标不存在属失败；原版文案未逐字核对〔待核〕。

## 5. 顺带的静态检查清理

`cmd_tell.lua`、`cmd_scoreboard.lua` 原先没有 luacheck 头，`SendMessage*` 被逐行报
「accessing undefined variable」；补上 `-- luacheck: globals SendMessage SendMessageFailure
SendMessageSuccess` 后：

- `cmd_tell.lua` 11 → **5** 条告警；
- `cmd_scoreboard.lua` 64 → **27** 条告警（剩余的是既有真实缺陷：
  `GlobalPlayer`、`Objetive` 拼写、`get_key_for_value` 未定义——不在本支范围）。

## 6. 验证

### 6.1 静态

- `luac -p`：Core 全部 Lua 通过。
- `luacheck`（`build/Server/.luacheckrc`）：四个改动文件均无新增类别；`give.lua` 52→52、
  `time.lua` 30→23、`cmd_tell.lua` 11→5、`cmd_scoreboard.lua` 64→27。
- Core 全部 `tests/test_*.lua`：**1307 条断言全绿**（本支不涉及这些纯函数模块，回归确认）。

### 6.2 实机（真实服务器 + bot，reload Core 后）

| 用例 | 修复后 |
|---|---|
| `/give @s stone 0` | `The number you have entered ( 0 ) must be at least 1` |
| `/give @s stone -1` | 同上（-1） |
| `/give @s stone 1 -1` | `Data must be at least 0` |
| `/give @s stone 1` | `Given [ stone ] x 1`（正常路径不受影响） |
| `/item stone 0` | 同一条数量消息（原先为 Usage，现与 /give 一致） |
| `/time set|add|day|query daytime|query gametime … x` | `There is no world "x"`（5/5） |
| `/time set 6000` | `Time was set to 6000`（正常路径） |
| `/r hi` | `Could not find the player you are replying to`（不再有 Lua 错误） |
| `/scoreboard players set|get|add|remove|reset|operation … nope` | `Unknown objective "nope"`（6/6） |
| `/scoreboard objectives add t1 dummy T1` → `players set … 5` → `players get` → `objectives remove` | 正常路径全部照常 |

### 6.3 验收门（按 AGENTS.md §4.1）

只改 `Server/Plugins/Core/`（Lua）+ 父仓库 `specs/` + gitlink：

| 门 | 结果 |
|---|---|
| 1 风格 | 不适用（无 `.cpp`/`.h`） |
| 2 编译 | 不适用（纯 Lua） |
| 3 ctest | 不适用（同上） |
| 4 绑定依赖 | 不适用 |
| 5 行为规格 | 本文件 + §6.1/§6.2 |
| 6 APIDump | 不适用（未新增/导出 C++ API） |
