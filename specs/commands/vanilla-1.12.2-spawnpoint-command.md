# 既有命令修复：`/spawnpoint` 的坐标范围与失败反馈

分支：`feature/spawnpoint-validation` ｜ 级别：P2（缺陷修复）｜ 前置：无

## 1. 缺陷（修复前，合并后 master c4c3175）

两处都在 `Server/Plugins/Core/cmd_spawnpoint.lua`：

1. **不校验坐标范围**：`/spawnpoint TestBot 1 300 1` 被接受并写入（实测回
   `Set the spawn point … to (1, 300, 1)`）；`y=256`、`y=-1` 同样被接受。
2. **解析失败时反馈为空**：`/spawnpoint TestBot a 64 b` 回一条**空消息**。
   根因是第 39 行的四变量赋值：
   ```lua
   local Reason
   X, Y, Z, Reason = ParseCommandCoords(Split[3], Split[4], Split[5], BasePosition)
   ```
   `ParseCommandCoords()` 失败时返回的是 **`nil, reason`**（两个值），所以 `Reason` 恒为 nil，
   `SendMessageFailure(…, nil)` 发出空文案。同类问题在 server-state 规格 §4.7 已被记录为既有缺口。

## 2. 行为规格（vanilla 1.12.2）

来源：[Commands/spawnpoint rev 1123064](https://minecraft.wiki/w/Commands/spawnpoint?oldid=1123064)
（2017-07-09，1.12.2 期；1.13 于 2018-07-18 发布）。

1. 语法：
   ```
   spawnpoint
   spawnpoint <player>
   spawnpoint <player> <x> <y> <z>
   ```
2. `<player>`：玩家名或目标选择器；省略时默认命令执行者；「**Not optional in command blocks.**」
3. `<x> <y> <z>`：可选，省略时默认执行者位置；
   原文「x and z must be integers within the range **-30,000,000 to 30,000,000 (inclusive)**.
   y must be an integer between **0 and 256 (inclusive)**. May use tilde notation to specify
   coordinates relative to the command's execution.」
4. Result：参数不合法，或 `<player>` 解析不到在线玩家 → 失败；成功则设置目标的出生点。
5. 1.13 之后：1.16.2（20w29a）加入 `<angle>` 参数、1.16（20w12a）允许任意维度执行——**均不引入**。

## 3. 实现映射

| 规格点 | 落点 |
|---|---|
| x/z ±30,000,000 含端点、y 0..256 含端点的校验 | 新纯函数 `GetSpawnPositionRangeReason(x, y, z)`（`cmd_spawnpoint.lua`） |
| 解析失败必须给出原因 | `ParseCommandCoords()` 改用三个局部变量接收，失败原因取**第二个**返回值 |
| 非法坐标不得写入 | 解析/校验失败时直接返回，不调用 `SetRespawnPosition()` |
| 文案 | `The x and z coordinates must be between -30,000,000 and 30,000,000`、`The y coordinate must be between 0 and 256`（与 `/setworldspawn` 一致） |

## 4. 已知偏差

1. **`[world]` 是 Core 扩展**（第 6 个参数）；原版不允许指定世界。
2. **小数坐标**：原版按方块位置取整；Core 把解析值直接交给 `Vector3i`（截断）而显示值用 `floor`，
   对 `1.5` 这类输入的具体取整方向未逐条核对〔待核〕。
3. **命令方块「player 不可省略」**：Core 在缺坐标时回 Usage、有坐标无 player 时按名字解析失败，
   行为上一致但文案不同。
4. **权限**：Core 用 `core.spawnpoint`，不是原版的数字等级 2。
5. **`/spawnpoint` 的选择器只接受 player kind**（与原版「player」类型一致）。

## 5. 验证

### 5.1 纯 Lua 测试

`lua tests/test_spawnpoint_command.lua` → **15 passed, 0 failed**：

- 有效坐标写入并把 `(1, 64, 1)` 回报；`y=0`、`y=256`、`x=30000000`/`z=-30000000` 接受；
  `~ ~ ~` 按 origin (10,20,30) 解析。
- `y=300`/`y=-1`/`y=257`、`x=30000001`、`z=-30000001` 一律拒绝且**不写入**，文案给出范围。
- 回归：`a 64 b` 的拒绝消息**非空**且包含 `not a valid number`。

### 5.2 静态

`luac -p` 通过；`luacheck` 与修复前同为 5 条既有告警、无新增。

### 5.3 实机（真实服务器 + bot）

| 用例 | 期望 |
|---|---|
| `/spawnpoint TestBot 1 300 1` | `The y coordinate must be between 0 and 256`（修复前接受） |
| `/spawnpoint TestBot a 64 b` | `'a' is not a valid number`（修复前空消息） |
| `/spawnpoint TestBot 1 64 1` | `Set the spawn point … to (1, 64, 1)` |
| `/spawnpoint TestBot 1 256 1` | 接受 |
| `/spawnpoint TestBot 30000000 64 0` | 接受；`30000001` 拒绝 |
| `/spawnpoint`（无参） | 设到执行者位置 |

### 5.4 验收门（按 AGENTS.md §4.1）

只改 `Server/Plugins/Core/`（Lua）+ 父仓库 `specs/` + gitlink：

| 门 | 结果 |
|---|---|
| 1 风格 | 不适用（无 `.cpp`/`.h`） |
| 2 编译 | 不适用（纯 Lua） |
| 3 ctest | 不适用（同上） |
| 4 绑定依赖 | 不适用 |
| 5 行为规格 | 本文件 + §5.1/§5.3 |
| 6 APIDump | 不适用（未新增/导出 C++ API） |
