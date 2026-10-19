# 既有命令修复：`/whitelist off` 反向开关

分支：`feature/whitelist-off-fix` ｜ 级别：P2（缺陷修复）｜ 前置：无

## 1. 缺陷（修复前）

`Server/Plugins/Core/whitelist.lua` 的游戏内处理器把开关写反了：

```lua
function HandleWhitelistOffCommand(a_Split, a_Player)
    g_IsWhitelistEnabled = true      -- ← 应为 false
    SaveConfig()
    a_Player:SendMessageSuccess("Whitelist is disabled.")
    return true
end
```

而控制台处理器 `HandleConsoleWhitelistOff` 调用的是 `WhitelistDisable()`（= `false` + `SaveConfig`），
是正确的。两边走不同实现，产生了这次分叉。

**实测（合并后 master 76cad07）**：游戏内 `/whitelist on` → 控制台 `whitelist list` = ENABLED；
随后游戏内 `/whitelist off` → 回 `Whitelist is disabled.`，但 `whitelist list` 仍是 **ENABLED**；
名单为空时新连接全部被拒（`Note: Whitelist is empty. No player can connect …`）。
即管理员收到"已关闭"的假反馈，服务器实际处于锁服状态。

## 2. 行为规格（vanilla 1.12.2）

来源：[Commands/whitelist](https://minecraft.wiki/w/Commands/whitelist)（minecraft.wiki，2026-10-19 抓取）。

1. 语法（Java）：`whitelist add <targets>` ｜ `whitelist list` ｜ `whitelist off` ｜
   `whitelist on` ｜ `whitelist reload` ｜ `whitelist remove <targets>`。
2. 语义原文：
   - `whitelist off`：「**Disables** the server's use of a whitelist.」
   - `whitelist on`：「**Enables** the server's use of a whitelist.」
   - `whitelist list`：「Displays all player profiles in the whitelist.」
3. Result：`/whitelist on` 在**已经启用**时失败；`/whitelist off` 在**已经禁用**时失败。
4. Output：`on`/`off`/`reload` 成功计数 1；`list` 的计数 = 名单人数。
5. 权限等级 3（Java），仅服务端命令；Java 原版中 **OP 始终可以连接**（即使不在名单里）。
6. 1.13 之后：命令保留（1.19 起 Bedrock 改名 `/allowlist`），Java 侧语法不变。

## 3. 实现映射

| 规格点 | 落点 |
|---|---|
| `off` 必须真的禁用 | `HandleWhitelistOffCommand` 改为调用 `WhitelistDisable()` |
| `on` 与 `off` 共用同一实现，避免再次分叉 | `HandleWhitelistOnCommand` 改为调用 `WhitelistEnable()` |
| 成功文案 | 保留 Core 既有文案（`Whitelist is disabled.` / `Whitelist is enabled.`） |

`WhitelistEnable()` / `WhitelistDisable()` 是 Core 既有的公开 API（同样供控制台与 WebAdmin 使用），
行为为「设置标志 + `SaveConfig()` 写库」，因此游戏内与控制台、以及重启后的持久状态从此一致。

## 4. 已知偏差（本支不改）

1. **「已启用/已禁用时失败」未实现**：两个处理器总是返回成功（原版此时失败，成功计数 0）。
   与本缺陷无关，属于文案/计数层面的对齐，留给后续分支〔待核：是否需要按原版失败〕。
2. **`/whitelist list` 的格式**是 Core 自己的（`Whitelist is enabled/disabled` + 逗号分隔名单），
   不是原版的 `There are N whitelisted players: …`。
3. **`/whitelist reload` 未注册**：Info.lua 的 `/whitelist` 只有 add/list/off/on/remove。
4. **OP 绕过**：Java 原版允许 OP 在启用白名单时连接；Core 的登录门是否放行 OP 未核实〔待核〕。
5. 权限节点：Core 用 `core.whitelist`，不是原版的数字等级 3。

## 5. 验证

### 5.1 静态

- `luac -p whitelist.lua`；`luacheck`（`build/Server/.luacheckrc`）该文件 0 warning。
- Core 全部 `tests/test_*.lua` 仍然全绿（本支不涉及这些纯函数模块，仅回归确认）。

### 5.2 实机（真实服务器 + bot）

1. 游戏内 `/whitelist on` → 控制台 `whitelist list` 显示 **ENABLED**；
2. 游戏内 `/whitelist off` → 控制台 `whitelist list` 显示 **DISABLED**（修复前为 ENABLED）；
3. `reload_plugin Core`（重新初始化 → 从 DB `LoadConfig`）后 `whitelist list` 仍为 **DISABLED**，
   证明 `SaveConfig()` 的持久化与修复前一样有效；
4. 结束后保持 DISABLED，避免锁服。

### 5.3 验收门（按 AGENTS.md §4.1）

只改 `Server/Plugins/Core/` 的 Lua 与父仓库 `specs/` + gitlink：

| 门 | 结果 |
|---|---|
| 1 风格 | 不适用（无 `.cpp`/`.h`） |
| 2 编译 | 不适用（纯 Lua） |
| 3 ctest | 不适用（同上） |
| 4 绑定依赖 | 不适用 |
| 5 行为规格 | 本文件 + §5.1/§5.2 |
| 6 APIDump | 不适用（未新增/导出 C++ API） |
