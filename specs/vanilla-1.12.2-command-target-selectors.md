# C1 目标选择器层（Core 插件内共用模块）

分支：`feature/core-target-selectors`（父仓库）+ `<fork>/Core` 上的同名分支 ｜ 级别：P0 ｜ 前置：无 ｜ 编排见 [roadmap](vanilla-1.12.2-command-layer-roadmap.md)

## 1. 为什么必须在 fork 里做

`Server/Plugins/Core` 是 **submodule**（当前 gitlink `bcba1c6`，[.gitmodules](../.gitmodules) 仍指上游）：文件**不能**在父仓库直接提交。流程硬性要求（[AGENTS.md §6（一）](../AGENTS.md)）：fork `cuberite/Core` → `https://github.com/rrtt217/Core.git`（[AGENTS.local.md §5](../AGENTS.local.md)：**尚未建**）→ fork 上一功能一分支 → 同一父仓库分支内**成对**改 `.gitmodules` 的 `url` 与 gitlink。

**为什么落插件侧而不是 `src/`**：命令全部由插件实现（[roadmap §1.1](vanilla-1.12.2-command-layer-roadmap.md)），且插件注册接口无参数类型（[InfoReg.lua:141](../Server/Plugins/InfoReg.lua#L141)），选择器只能由 handler 自行解析。所需实体侧 API 已在（`cWorld:ForEachEntity` [World.h:269](../src/World.h#L269)、`IsPlayer/IsPawn` [Entity.h:161-164](../src/Entities/Entity.h#L161)、`cMonster.StringToMobType` [Monster.h:191](../src/Mobs/Monster.h#L191)），无需改 C++。
唯一需要 C++ 配合的是「执行上下文」（`@s`、`@p` 的原点、命令方块原点）→ 属 **C2**，本分支的模块**签名预留该参数并允许为 `nil`**，`nil` 时按 vanilla 语义退化（见 §3.4）。

## 2. 现状与差距（逐条实测）

| 现状 | 位置 | 1.12.2 应有 |
|---|---|---|
| 无任何选择器概念；`grep -rn "Selector" src` 零命中 | — | 选择器是命令参数的通用类型 |
| 唯一实现是硬编码 `@a` 字符串比较，且只覆盖在线玩家 | [cmd_effect.lua:20,28,50,57](../Server/Plugins/Core/cmd_effect.lua#L50) | `@p @a @r @e @s` + `[...]`，`@e` 含非玩家实体 |
| 目标解析只有「玩家名前缀匹配」一条路 | [Root.cpp:698](../src/Root.cpp#L698) + [RateCompareString](../src/StringUtils.cpp#L277)（大小写不敏感、取最长公共前缀） | 名字只是**可选**过滤条件之一 |
| 多目标只能 `ForEachPlayer` | [cmd_effect.lua:51](../Server/Plugins/Core/cmd_effect.lua#L51) | 跨世界 `@e` / `@a`，并支持 `limit` 截断 |
| 命令参数按空格裸切，含空格的 `[...]` 必被切断 | [PluginManager.cpp:1258](../src/Bindings/PluginManager.cpp#L1258) → [StringSplit](../src/StringUtils.cpp#L51) | `name="First Last"` 必须可用 → 依赖 **C2** |
| 无排序/距离/体积/分数/标签等任何筛选原语 | — | 见 §3.3 |

## 3. 行为规格

### 3.1 选择器变量

| 变量 | 语义 | 本仓库可行性 |
|---|---|---|
| `@p` | 距执行原点最近的**单个**玩家 | 可行（`ForEachPlayer` + 距离） |
| `@r` | 随机玩家 | 可行（`FastRandom`） |
| `@a` | 所有玩家 | 可行（现成） |
| `@e` | 所有实体 | 可行：`ForEachWorld` + `cWorld:ForEachEntity` + `IsPawn` 判定；**注意**与 vanilla「所有实体（含掉落物/矿车/盔甲架）」不同 → 记入已知偏差 §6 |
| `@s` | 执行命令的实体 | 需 **C2**；无执行者时为**空集**（非「全体玩家」） |

来源：[Target selectors](https://minecraft.wiki/w/Target_selectors)（该页称 Java 侧「six in Java Edition」，除上表五个外还列有 `@n`；**`@n` 是否属 1.12.2 Java 待核实**，本分支**不**实现）。

### 3.2 语法

`@<var>[<key>=<value>,<key>=<value>…]`，方括号内**逗号分隔、无空格**；值可加双引号以容纳空格；`!` 前缀表示取反（如 `type=!player`）。不带 `[]` 时等价于无参选择器。

### 3.3 参数集（**必须开工前补齐核实**）

- **本轮已核实存在于 1.13 之前**：`type=`、`name=`、`c=`（数量上限）——证据是 1.13 前形态的 `/execute` 页面示例直接用了 `@e[type=zombie]`、`@a[name=name_of_player]`、`@e[c=10]`：[Commands/execute (old)](https://minecraft.wiki/w/Commands/execute_\(old\))。
- **本轮未能核实**（**推测**，动笔实现前必须逐条找到 1.12.2 专用来源）：`r=`/`rm=`（最小/最大距离）、`x/y/z` + `dx/dy/dz`（体积）、`l=`/`lm=`（等级）、`rx/rxm/ry/rym`（视角）、`scores=`、`tag=`、`team=`。现行 wiki 页只给 1.13+ 的 `distance=`/`limit=`/`sort=`，而 `minecraft.wiki/w/Target_selectors/history` **实测不存在（HTTP 404）** → 需改用「Java Edition 1.12.2 版本页 + 补丁说明」或**合法客户端实测观察**（后者本机不具备，见 [AGENTS.local.md §1](../AGENTS.local.md)）。
- **禁止**把 1.13+ 的 `distance=..10` / `sort=nearest` 语法当作 1.12.2 实现——那是版本污染（[AGENTS.md §9](../AGENTS.md)）。

### 3.4 退化与错误语义

- 参数非法（未知 key、非数字值、括号不配对）→ **整条命令不可解析**，返回用法提示，绝不静默忽略。
- 选择器解析出**空集** → 命令返回失败（`/effect @e[type=cow]` 无目标时应报「找不到目标」，与现有文案风格一致：[cmd_effect.lua:54](../Server/Plugins/Core/cmd_effect.lua#L54)）。
- 单目标位（如 `/kill`、`/effect`）收到多目标选择器 → 1.12.2 的行为是**取其一**还是**报错**：**待核实**，本分支先按「报错并提示」实现，注释标注待核实。
- 无执行者（控制台/命令方块）时：`@s` 空集；`@p` 以命令方块位置/世界为原点（依赖 C2），控制台命令则以世界出生点为原点（**推测**，待核实）。

## 4. 本分支范围

**做**：
1. Core 内新增 `target_selectors.lua`：`ParseSelector(token)` + `Resolve(selector, context)`，返回实体列表与失败原因。
2. 支持变量 `@p @a @r @e @s`；参数先落 `type=` / `name=` / `c=` / `r=`（后者待核实后启用），其余按 §3.3 核实结果增量加。
3. `type=` 值映射：走 `cMonster.StringToMobType`（[Monster.cpp:1140](../src/Mobs/Monster.cpp#L1140)，导出见 [APIDesc.lua:9409](../Server/Plugins/APIDump/APIDesc.lua#L9409)）；**注意**其内部是「小写 + 二分查找」（[Monster.cpp:29-30](../src/Mobs/Monster.cpp#L29)），而 1.12.2 选择器里写的是 `Creeper` 一类首字母大写名 → 解析层必须大小写不敏感，并核实 1.12.2 接受的确切拼写集合。
4. 把 `@a` 硬编码从 [cmd_effect.lua](../Server/Plugins/Core/cmd_effect.lua) 摘掉，改调本模块（这是 S16 §3.3 的兑现）。
5. 供 C4/C5/C6 复用的导出函数 + `Info.lua` 帮助文案里把 `<player>` 改为 `<目标>` 的位置一并更新。

**不做**：`/execute`（C4）、`scores=`/`tag=` 依赖的计分板/标签子系统（C6 与后续分支）、`@n`、1.13+ 语法、SNBT。

## 5. Cleanroom 裁决（本轮检索结论，写进 spec 以免重复调研）

Brigadier 是 Mojang 自研、MIT 许可、**1.13/17w45a** 才进入 vanilla 的命令框架（[minecraft.wiki/Brigadier](https://minecraft.wiki/w/Brigadier)，页面明记作者 Mojang、MIT、2018-09-26 开源）。据此：

1. **不得**引入 brigadier 或任何语言 port（C++/C/Go/C#/TS/Rust/Python 均有 port，GitHub 检索可得）——它们都是 Mojang 代码的派生物，违反 [AGENTS.md §2.1/§2.2](../AGENTS.md)；且 1.13+ 语义与 1.12.2 基线冲突（§9）。
2. **Lua 侧不存在 brigadier port**（2026-08 GitHub repo + code 检索）：语言为 Lua 的唯一相关仓库是 CC:Tweaked 的 `migeyel/cbb`（MIT，自研 DSL，无选择器）；`NirlekaPlay/asymptote-engine` 有一份**独立实现**的 MC 风格选择器解析器（**GPL-3.0**，Roblox/Luau）→ 按 §2.2 属「独立实现的行为观察」，**只能对照行为，不得引用其代码**，且其参数名是新版风格。
3. 结论：按 wiki 规格自研本模块。这是最小、可验证、不越界的路径。

## 6. 测试与验收

- **插件侧**：`luac -p` + `luacheck` 干净；`tests/FuzzCommands.lua` 的 `choices` 增加 `"@a"`、`"@p"`、`"@e[type=...]"`、`"@a["`（畸形输入不得崩，参考现有 fuzz 场景 [FuzzCommands.lua:22-36](../Server/Plugins/Core/tests/FuzzCommands.lua#L27)）；新增手工用例：`@a` vs `@a[c=1]`、`type=!player`、`name="带空格"`。
- **纯函数可测性**：`ParseSelector` 必须是**不碰游戏对象的纯函数**（输入字符串 → 结构体/失败原因），便于用独立 Lua 解释器跑表驱动用例（`lua tests/test_selectors.lua`，不入库运行产物）。
- **父仓库侧**：本分支不改 `src/` → AGENTS 门 1–3 不适用；但须自查 `.gitmodules` 的 `url` 与 `git submodule status` 的 gitlink 一致性（AGENTS §6（一）3）。
- **行为证据**：规格逐条对表；无实机 oracle（[AGENTS.local.md §1](../AGENTS.local.md)）→ 以规格 + 单测为准，实机对照后续补。

## 7. 已知偏差 / 风险

- `@e` 只覆盖 pawn（玩家 + 怪物），不含掉落物/矿车/物品展示框 → 与 vanilla 有实质差距；若要全覆盖需 C++ 侧暴露实体类型判定，另立分支。
- 跨世界 `@a`/`@e` 需在 `cRoot:ForEachWorld` 里遍历（Core 已如此用：[cmd_players.lua:8-14](../Server/Plugins/Core/cmd_players.lua#L8)）→ 与 vanilla 的「全局」语义一致，但 `@p` 的距离**跨世界如何比较**未定（**推测**：只在执行者所在世界内取最近）→ 待核实。
- 参数集若按 §3.3 核实后与现行 wiki 差异较大，本 spec 的 §3.3 必须回改，**不得**以「已实现」倒逼规格。
