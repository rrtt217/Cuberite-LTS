# C5 表现类命令合并：`/title`、`/particle`、`/playsound`、`/stopsound`

分支：`feature/core-presentation-commands` ｜ 级别：P2 ｜ 前置：C1（选择器，用于 `<targets>`）｜ 编排见 [roadmap](vanilla-1.12.2-command-layer-roadmap.md)

**合并理由**：四条命令形状完全相同——「解析参数 → 校验 → 调一个已有 C++ API 广播」，无服务端状态、无存档、无协议新增；单条实现不足 150 行，各立一支会让评审成本翻四倍而收益为零。它们共享同一批触点（`Info.lua` 注册表 + 选择器调用 + 目标解析样板）。

## 1. 现状

| 命令 | Core | 所需 C++ API | 状态 |
|---|---|---|---|
| `/title` | 无 `cmd_title.lua` | `cClientHandle::SendSetTitle`（[ClientHandle.h:222](../src/ClientHandle.h#L222)）、`SendTitleTimes`（[:232](../src/ClientHandle.h#L232)），已文档化（[APIDesc.lua:1857](../Server/Plugins/APIDump/APIDesc.lua#L1857)、[:1952](../Server/Plugins/APIDump/APIDesc.lua#L1952)） | **API 齐，缺命令** |
| `/particle` | 无 | `cWorld::BroadcastParticleEffect`（[World.h:182-183](../src/World.h#L182)，两个重载：带/不带 `std::array<int,2>` 附加数据） | **API 齐，缺命令** |
| `/playsound` `/stopsound` | 无 | 需定位 sound 下发函数（本文件**未**核实其导出形态，实现前先查 [APIDesc.lua](../Server/Plugins/APIDump/APIDesc.lua) 的 `cClientHandle` 段） | **API 待核** |

即：**这不是「要新建架构」的缺口，而是 Core 少写了几个 `cmd_*.lua`。**

## 2. 行为规格

### 2.1 `/title`（已核实）

来源：[/title](https://minecraft.wiki/w/Commands/title)

1. 1.12.2 语法：`/title <targets> title|subtitle|actionbar|times|clear|reset …`
   - `title` / `subtitle` / `actionbar` 的参数是**文本**（支持 JSON 文本组件；本 fork 先支持普通字符串 + 现有 `&` 颜色码，JSON 组件留待 `/tellraw` 分支统一处理）；
   - `times <fadeIn> <stay> <fadeOut>` 三个参数是**整数**——该页版本历史明确 `1.19.4 → 22w17a` 才把它们改成**时间类型**，「Before this snapshot, these integer arguments specifies a time in game ticks」→ **1.12.2 单位是 game tick**，且不接受 `1s` 一类写法；
   - `clear` 清除已保存的标题；`reset` 恢复默认值（两者语义不同，页面 Usage 小节有专门说明）。
2. 加入历史：1.8 → `14w20a`；`actionbar` 子命令：1.11 → `16w32b`。→ 1.12.2 四条子命令都在。
3. 权限等级 2（同页信息框）。
4. 目标为多玩家时对每个玩家生效；`@s`/`@e` 命中非玩家实体时忽略之。

> 现状可用性核对：`SendSetTitle` 收 `cCompositeChat`；`SendTitleTimes(fadeInTicks, displayTicks, fadeOutTicks)` 单位即 tick，与 §2.1 一致。

### 2.2 `/particle`、`/playsound`、`/stopsound`（**未核实**）

本文件**不**给出这两条的语法与参数表——本轮未取相应 wiki 页面，凭记忆写参数就是 [AGENTS.md §2.3](../AGENTS.md) 禁止的「凭模型记忆写」。开工前必须先取：

- [/particle](https://minecraft.wiki/w/Commands/particle)：需确认 1.12.2 的参数序（`<name> <pos> <offset> <speed> <count> [mode] [viewers]` 的确切形态）、粒子名集合、`<count>` 为 0 时的「仅在有观察者时显示」语义、`normal|forced` 模式与可视距离裁剪规则；
- [/playsound](https://minecraft.wiki/w/Commands/playsound) 与 [/stopsound](https://minecraft.wiki/w/Commands/stopsound)：需确认 1.12.2 的 `<sound>` 写法（`random\|record\|music\|weather\|block` 来源分类是 1.13 还是更早）、`<volume> <pitch> <minVolume>` 语义。

**验收前置**：把核实到的参数表回填本文件 §2.2 并附来源，才允许动代码。

## 3. 本分支范围

**做**：
1. `cmd_title.lua`：完整四条子命令 + 目标解析（C1）+ 越界校验（times 非负、非数字报错文案沿用 Core 风格）。
2. `cmd_particle.lua`：粒子名白名单校验（非法名返回失败而非静默）、位置（含 `~` 相对坐标，与 C4 共用坐标解析工具函数）、`count`/`mode` 透传给 `BroadcastParticleEffect`。
3. `cmd_playsound.lua` / `cmd_stopsound.lua`：待 §2.2 核实后实现。
4. 共享的小工具：坐标 token 解析（`~`/绝对值）与「选择器 → 玩家列表」两处样板抽到 Core 内的公共文件，供 C4 复用。

**不做**：`/tellraw`（需 JSON 文本组件层）、`/titleraw`（BE 独有）、`/bossbar`（1.13+）、粒子 ID 与方块/红石粒子的 `id` 参数细节（若核实属 1.13+）。

## 4. 测试与验收

- 纯逻辑抽头：粒子命令的参数解析（坐标 + 偏移 + 速度 + 数量 + 模式）写成纯函数并表驱动测试，含畸形输入不得崩。
- **fuzz**：`choices` 增列 `"title"`、`"clear"`、`"1"`；确认四条新命令被 `fuzzAllCommands` 覆盖（[FuzzCommands.lua:22](../Server/Plugins/Core/tests/FuzzCommands.lua#L27) 是「所有已注册命令」，注册即覆盖）。
- Core 侧三件（`luac -p` / `luacheck` / fuzz）；用到未文档化的 C++ 成员时同步 [APIDump](../Server/Plugins/APIDump/) 并跑门 6 自检。
- 行为证据：`/title @s title X` + `times` 三种取值、`/particle` 的 `count=0` 与 `forced` 差异需实机；本机无实机（[AGENTS.local.md §1](../AGENTS.local.md)）→ 记录为「未验」，不写成已验。

## 5. 已知偏差 / 风险

- `/title` 的 JSON 文本组件在本 fork 只有 `cCompositeChat` 一条路（`SendSetTitle(cCompositeChat)`），**任意** JSON（含 `clickEvent`/`hoverEvent`）无法直接透传 → 与 vanilla 有实质差距，须写入交付说明的「已知偏差」，并在 §3「不做」里指向未来的 `/tellraw` 分支。
- `/particle` 的可视距离裁剪语义（`normal` 模式按距离丢弃、`forced` 强制下发）取决于 `BroadcastParticleEffect` 的实现，**本文件未核实其行为**；若 C++ 侧不做裁剪，则与 vanilla 不一致 → 实现时定位并在交付说明标注。
- `/playsound`/`/stopsound` 在参数核实前**禁止**开工（AGENTS §2.3）；若核实后发现 C++ 无对应导出，则本支拆分为「`/title` + `/particle`」两支，声音两条另立并附前置依赖（AGENTS §3（1））。
