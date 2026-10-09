# S16 Core `/effect` 命令对齐（submodule 特殊流程）

分支：`feature/core-effect-command`（父仓库）+ `<fork>/Core` 上的同名分支 ｜ 级别：P2 ｜ 前置：**S1–S4 之后**，且选择器那半依赖 **C1**（[command-layer-roadmap](vanilla-1.12.2-command-layer-roadmap.md)）（否则命令越正确越容易暴露空转效果） ｜ 编排见 [roadmap](vanilla-1.12.2-status-effects-roadmap.md)

## 1. 为什么必须在 fork 里做

`Server/Plugins/Core` 是 **submodule**（[AGENTS.md §6（一）](../AGENTS.md)）：文件**不能**在父仓库直接提交。流程硬性要求：

1. 在贡献者账号下 fork `cuberite/Core` → `https://github.com/rrtt217/Core.git`（[AGENTS.local.md §5](../AGENTS.local.md) 记录当前**尚未建 fork**，需先建）；
2. 在 fork 上按同样的一功能一分支提交；
3. **同一父仓库分支内成对**改 `.gitmodules` 的 `url` + gitlink 指针（否则 clone/CI 取不到该提交）；
4. 交付说明写明父仓库分支、fork 分支/提交号，以及为什么不能只在上游等。

命令实现文件：[Server/Plugins/Core/cmd_effect.lua](../Server/Plugins/Core/cmd_effect.lua)（注册见同插件 [main.lua](../Server/Plugins/Core/main.lua)）。

## 2. 现状与差距（逐条实测）

| 现状 | 位置 | 1.12.2 应有 |
|---|---|---|
| 只能作用于玩家（`@a` 或单个玩家名） | [cmd_effect.lua:50-53](../Server/Plugins/Core/cmd_effect.lua) | 目标可含其他实体（`@p`/`@r`/`@e`） |
| `clear` 一律 `ClearEntityEffects()` | 同文件 :25-31 | `/effect <t> clear [<effect>]` 可只清一种 |
| 无 `hideParticles`（第 6 参数） | 全文件 | 1.12.2 语法含 `[hideParticles]` |
| 默认时长 30 秒、amplifier 默认 0 | :4-10 | ✓ 与 vanilla 默认一致 |
| 上限：ID 1–23、amplifier ≤ 255、秒数 ≤ 1 000 000 | :36-49 | ID/amplifier 上限 ✓；秒数上限需按 1.12.2 核（现值来自较新版本的说明） |
| 时长 0 → 立即过期 | :16-18 乘以 20 后交给 `AddEntityEffect` | vanilla 1.12.2 中 0 秒即「立刻结束」，与本实现一致（待实机确认，见 §4） |

语法来源：[Commands/effect](https://minecraft.wiki/w/Commands/effect)（**注意**：该页给的是 1.13+ 的 `give/clear` 子命令形态；1.12.2 为 `/effect <player> <effect> [seconds] [amplifier] [hideParticles]` 与 `/effect <player> clear` → 具体到 1.12.2 的页面（`Commands/effect/1.12` 一类）需在动手时核实并在此处补链接，本分支**先不改可参数目以外的行为**）。

## 3. 本分支范围（刻意收窄）

**做**：
1. `clear` 支持可选的第 4 参数（只清一种效果）→ 用 `OtherPlayer:RemoveEntityEffect(id)`（导出见 [APIDesc.lua](../Server/Plugins/APIDump/APIDesc.lua)）；
2. 接受第 6 参数 `hideParticles`（true/false），并把标记传到 C++（若 C++ 侧尚无通路 → **属于 S14 的签名改动**，此时本分支只解析并忽略该参数，注释指向 S14，**不得**在 Core 里私存状态）；
3. **目标选择器不在本分支实现**：选择器层由命令层的 **C1**（[command-target-selectors](vanilla-1.12.2-command-target-selectors.md)，Core fork 内新增 `target_selectors.lua`）提供。本分支只负责把 [cmd_effect.lua](../Server/Plugins/Core/cmd_effect.lua) 里硬编码的 `@a` 字符串比较摘掉、改调 C1 的解析结果（C1 §5 已把这一步记为「S16 §3.3 的兑现」）。**C1 未合并时**：本分支只做 §3.1/`clear` 与 §3.2/`hideParticles`，选择器保持现状并留 TODO 指向 C1——**不得**在 `cmd_effect.lua` 里私下一套选择器实现。
4. 越界与非法输入的文案保持现状风格。

**不做**：`infinite` 时长（1.21+ 语法）、`give` 子命令（1.13+）、效果名（`speed` 而非 `1`）—— 1.12.2 用数字 ID。

## 4. 测试与验收

- **插件侧**：Core 自带 fuzz 测试 [Server/Plugins/Core/tests/FuzzCommands.lua](../Server/Plugins/Core/tests/FuzzCommands.lua) → fork 上需保证它对改动后的命令仍通过；新增用例覆盖「`clear` 带 ID」「`@e` 目标」「第 6 参数非法值」。
- `luacheck` / `luac -p` 必须干净（Core 的 CI 见 fork 上的 [.github/workflows/ci.yml](../Server/Plugins/Core/.github/workflows/ci.yml)）。
- **父仓库侧**：AGENTS 门 1–3（本分支不改 `src/`，若为传参而改 `src/` 则六道门全适用），外加 `git submodule status` 与 `.gitmodules` 的 `url` 一致性自查。
- 行为证据：手工在 `build/Server` 里跑 `/effect` 全参数矩阵并记录（不提交运行期产物）。

## 5. 已知偏差 / 风险

- 需要新建 GitHub fork 与 `gh` 操作（[AGENTS.local.md §3](../AGENTS.local.md)：GitHub 走 `gh`，`web_fetch` 到 github.com 不通）。
- 若上游 `cuberite/Core` 后续合并同类改动，须把 `url` 与 gitlink 切回上游（AGENTS §6（一）4）。
- `@e` 会影响非玩家 pawn；S6（免疫表）未合并时，`/effect @e wither` 会给凋灵骷髅上 Wither（vanilla 会免疫）→ **建议在 S6 之后做**，或在本分支交付说明里显式标注。
