# C1 目标选择器层（Core 插件内共用模块）

分支：`feature/core-target-selectors`（父仓库）+ [`rrtt217/Core`](https://github.com/rrtt217/Core) 上的同名分支 ｜ 级别：P0 ｜ 前置：无 ｜ 编排见 [roadmap](vanilla-1.12.2-command-layer-roadmap.md)

> **交付状态（2026-10-12）**：本分支已实现并推送。Core 侧提交 `8913d93212cf3fb4d7236437c1b06a83fa08e3f1`
> （fork 分支 `feature/core-target-selectors`），父仓库同一分支更新 `.gitmodules` 的 `url`
> 与 gitlink。下文 §3 的参数集已按允许来源核实完毕，§4 记录**实际落地**的范围。

## 1. 为什么必须在 fork 里做

`Server/Plugins/Core` 是 **submodule**：文件**不能**在父仓库直接提交。流程硬性要求（[AGENTS.md §6（一）](../../AGENTS.md)）：
fork `cuberite/Core` → `https://github.com/rrtt217/Core.git` → fork 上一功能一分支 →
同一父仓库分支内**成对**改 `.gitmodules` 的 `url` 与 gitlink。三者均已完成。

**为什么落插件侧而不是 `src/`**：命令全部由插件实现（[roadmap §1.1](vanilla-1.12.2-command-layer-roadmap.md)），
且插件注册接口无参数类型（[InfoReg.lua:141](../../Server/Plugins/InfoReg.lua#L141)），选择器只能由 handler 自行解析。
所需实体侧 API 已在（`cWorld:ForEachEntity` [World.h:269](../../src/World.h#L269)、
`IsPlayer/IsPawn` [Entity.h:161-164](../../src/Entities/Entity.h#L161)、
`cMonster.StringToMobType` [Monster.h:191](../../src/Mobs/Monster.h#L191)、
`cEntity:GetBoundingBox` [Entity.h:213](../../src/Entities/Entity.h#L213)），无需改 C++。
唯一需要 C++ 配合的是「执行上下文」（`@s`、`@p` 的原点、命令方块原点）→ 属 **C2**，
本模块的 `ResolveSelector` **接受可选的 context 参数且允许为 `nil`**，`nil` 时按退化语义处理（见 §3.5）。

## 2. 现状与差距（开工前复核，基线 `bcba1c6`）

| 开工前现状 | 位置 | 1.12.2 应有 |
|---|---|---|
| 无任何选择器概念；`grep -rn "Selector" src` 零命中 | — | 选择器是命令参数的通用类型 |
| 唯一实现是硬编码 `@a` 字符串比较，且只覆盖在线玩家 | 基线 `cmd_effect.lua` 的四处 `Split[2] ~= "@a"` / `== "@a"` | `@p @a @r @e @s` + `[...]`，`@e` 含非玩家实体 |
| 目标解析只有「玩家名前缀匹配」一条路 | [Root.cpp:698](../../src/Root.cpp#L698) + [RateCompareString](../../src/StringUtils.cpp#L277)（大小写不敏感、取最长公共前缀） | 名字只是**可选**过滤条件之一 |
| 多目标只能 `ForEachPlayer` | 基线 `cmd_effect.lua` | 跨世界 `@e` / `@a`，并支持 `c=` 截断 |
| 无排序/距离/体积/等级/队伍等任何筛选原语 | — | 见 §3.3 |

> 关于「命令参数按空格裸切」：**1.12.2 的选择器本身不允许空格或引号**（§3.2），
> 所以带空格的 `[...]` 不是 1.12.2 的选择器需求；引号切分是独立议题，归 C2
> （[command-context-and-splitting §1](vanilla-1.12.2-command-context-and-splitting.md)）。

## 3. 行为规格（已按允许来源核实）

**来源**（全部为 1.12.2 时点或记录 1.13 改动的允许来源）：

- **E1** = [Commands 的 2017-09-29 修订（rev 1152284）](https://minecraft.wiki/w/Commands?oldid=1152284)——
  1.12.2 发布于 2017-09-18，1.13 首个快照 17w43a 为 2017-10-25，故该修订落在 1.12.2 之后、1.13 之前。
  独立的交叉核对版本：[rev 1204417（2018-04-29）](https://minecraft.wiki/w/Commands?oldid=1204417)。
- **E2** = [Java Edition 17w45a](https://minecraft.wiki/w/Java_Edition_17w45a)——1.13 命令框架重做，
  用于**反推** 1.12.2 与 1.13+ 的分界（如「`x`/`z` 不再做中心修正」「参数可以加引号」）。
- **E3** = [Target selectors](https://minecraft.wiki/w/Target_selectors)（现行页）——其 History 段给出各参数引入版本。
- **E4** = [Commands/effect 的首个修订（rev 1122926，2017-07-08）](https://minecraft.wiki/w/Commands/effect?oldid=1122926)——
  1.12.2 时点的 `/effect` 目标语义（见 §3.4）。

### 3.1 选择器变量

| 变量 | 语义 | 本分支 |
|---|---|---|
| `@p` | 距执行原点最近的**玩家**（有 `type=` 时改为最近的匹配实体） | 已实现（同一世界内比距离） |
| `@r` | 随机玩家（有 `type=` 时为随机实体） | 已实现 |
| `@a` | 所有玩家 | 已实现 |
| `@e` | 所有实体 | **只覆盖 pawn**（玩家 + 怪物），见 §7 |
| `@s` | 执行命令的实体；无执行者时为**空集** | 已实现；context 为 `nil` 时为空集 |

**`@n` 不属于 1.12.2**：E3 的 History 记 `@n` 为 1.21 / 24w21a 引入；E1 明记 1.12.2「There are five variables」。
故本分支**不**接受 `@n`。

### 3.2 语法

`@<var>[<key>=<value>,<key>=<value>…]`。**1.12.2 不允许空格、不允许引号**：
E1 原文「Spaces are not allowed around the brackets, equal signs, or commas」；
引号与「`name` 支持带空格的引号值」都是 **17w45a（1.13）** 才加入的（E2）。
`!` 前缀表示取反（如 `type=!player`）。**同一参数不可重复**：重复是 17w45a 才允许的（E2 记 17w45a
「Multiple of the same argument in target selectors is now possible」，并修复 MC-119142）。

### 3.3 参数集（**已核实**）

| 参数 | 1.12.2 语义 | 本分支 |
|---|---|---|
| `type=` | 实体 ID（如 `creeper`、`minecart`、`player`），可取反；`@p`/`@r` 带 `type=` 时从玩家集切到实体集 | 已实现；**与 `/summon` 共用同一查找**（`entity_names.lua`：生物交给引擎的 `cMonster:StringToMobType`，非生物 id 由同一张小表识别），见下 |
| `name=` | 名字**精确**匹配，可取反；非玩家实体匹配自定义名 | 已实现（大小写敏感〔推测〕） |
| `c=` | 按距离排序后的数量上限；`@p`/`@r` 默认 1；**负值反转顺序**（`@p[c=-1]` = 最远） | 已实现 |
| `r=` / `rm=` | 距原点的最大 / 最小半径；**负值表示「仅执行维度、不限距离」，`r=` 与 `rm=` 一致** | 已实现（边界闭区间〔待核〕） |
| `x=` / `y=` / `z=` | 设定原点（供 `r`/`rm`/`dx`…与排序用）；**整数**；`x`/`z` 做中心修正（`x=0` ≡ `x=0.5`），`y` 不修正 | 已实现（中心修正按 E2 反推） |
| `dx=` / `dy=` / `dz=` | 自原点的有符号增量构成体积；未给的默认 0；选命中体积的实体 | 已实现（实体包围盒与体积求交） |
| `l=` / `lm=` | 最大 / 最小经验等级（含端点），过滤到玩家 | 已实现 |
| `m=` | 游戏模式，可取反；取值 `0/s/survival`、`1/c/creative`、`2/a/adventure`、`3/sp/spectator` | 已实现四值；`-1`〔待核〕 |
| `team=` | 队伍名匹配；`team=` = 无队伍，`team=!` = 任意队伍，可取反 | 已实现 |
| `tag=` | 计分板标签；`tag=` = 无标签，`tag=!` = 有标签；1.12.2 每选择器**只允许一个** `tag=` | **未实现**（Cuberite 无实体标签子系统） |
| `score_<obj>=` / `score_<obj>_min=` | 1.12.2 的计分板参数形态（**不是** `scores=`） | **未实现**（归 C6） |
| `rx=`/`rxm=`/`ry=`/`rym=` | 视角（俯仰 / 偏航）范围 | **未实现**（见 §7） |

**`scores=` 在 1.12.2 不存在**：1.12.2 用的是动态键名 `score_<objective>=<MAX>`（含端点）与
`score_<objective>_min=<MIN>`（含端点），无数值区间、无 `..`、无 `!`；`scores={foo=1..5}` 是 17w45a 的形态（E2）。
开工前的 spec 草稿把 `scores=` 列为「待核参数」本身就是版本污染，现更正。

**`type=` 值的大小写有来源冲突**〔待核〕：E1/E4（1.12.2 时点）写「Entity IDs are case-sensitive」，
而 [Java Edition 1.11](https://minecraft.wiki/w/Java_Edition_1.11) 写实体 ID「now are no longer case sensitive」。
两说直接冲突且未找到 1.12.2 专用裁决。本分支按**大小写不敏感**实现（同时接受 `Creeper` 与 `creeper`）。
裁决方法：合法 1.12.2 服务端实测 `/kill @e[type=Creeper]`。

**`type=` 与 `/summon` 共用同一个查找**（2026-10 对齐）：`entity_names.lua` 的 `GetEntityInfo()` 是唯一入口——
生物名**不在 Lua 里复制**，直接问引擎的 `cMonster::StringToMobType`（它内部就是 `g_MobTypeNames`：每个生物类型的
Cuberite 名、vanilla 名、1.12.2 NBT id 三种拼写，大小写不敏感），因此两条命令对生物名的接受集合按构造一致；
非生物 id（`tnt`、`boat`、`minecart`、`arrow` 等 21 个）引擎没有「名字→构造器」映射，只有这一张表，两边共用。
匹配时：`player` 由本层解析；生物比 `GetMobType()`；非生物比 `GetEntityType()`（今天就写成按实体类型匹配，
等到 `@e` 覆盖非 pawn 实体时无需再改，见 §5 的 `@e` 偏差）；不认识的 id 一律不匹配。
副作用（刻意）：`/summon` 不再接受它自己那张表独有的 1.10 驼峰别名（`PrimedTnt`、`MinecartChest`、`ThrownEgg` …）
——它们从来不是 1.12.2 的 id；生物侧的旧拼写仍由引擎表接受（`PigZombie`、`LavaSlime` …）。

**作用域规则（跨世界距离无定义）**：距离只在**单个世界内**良定义，因此凡是带
**位置 / 距离语义**的选择器，候选集一律限定在**原点世界**，绝不跨世界比较坐标：

| 触发条件 | 候选集 |
|---|---|
| 出现 `x=` / `y=` / `z=`（设定原点，而原点自带维度）、`dx=`/`dy=`/`dz=`、`r=`/`rm=`、`c=`，或变量为 `@p` | **仅原点世界** |
| 以上都没有（`@a` / `@e` / `@r` 的纯集合形态） | 全部世界（vanilla 的全局语义，不涉及距离） |

依据：E1 的 `player` 参数定义为「player name or target selector」，而位置参数自带维度；
`x/y/z` 单独给出时**只移动原点、不缩小候选集**，但仍**钉住维度**（见 §7 的〔待核〕）。
原实现把异世界实体当作「无穷远」参与 `c=` 排序、且 `dx/dy/dz` 的包围盒不查世界，属错误，已修正。

**负值半径的来源**：`r<0` 见 1.12.2 时代 wiki 原文（[Commands?oldid=1152284](https://minecraft.wiki/w/Commands?oldid=1152284)：
"Using a value under 0 restricts the targets to the same dimension the command was run from without
applying a restriction on distance from the location of execution"）；
`rm<0` 与之行为一致，来自维护者在真实 1.12.2 上的**实测观察（2026-10-13）**——
原实现把 `rm<0` 当正数下界（与数值平方比较，`rm=-1` 会排除 1 格内的实体），与实测相反，已修正
（Core `feature/selector-negative-radius`）。

**控制台 / RCON**：没有执行者世界，`GetSelectorOrigin` 回退到**默认世界**的出生点
（出生点存取器 [World.h:584](../../src/World.h#L584)），故控制台同样不存在多世界问题。

### 3.4 退化与错误语义

- 参数非法（未知 key、非整数数值、括号不配对、含空格、重复 key）→ **整条命令不可解析**，返回失败原因，绝不静默忽略。
- 合法但**本分支未实现**的 1.12.2 参数（`tag`、`score_*`、`rx/rxm/ry/rym`）→ 解析通过、**解析失败**，
  原因明示「not supported yet」。这样既不静默忽略，也不误报「语法错误」。
- 选择器解析出**空集** → 返回空列表，由调用方决定文案；`/effect` 报
  「No target matched the selector "…"」。
- 多目标：**1.12.2 的 `/effect` 接受多目标**，且**目标可以是玩家以外的实体**。
  证据：`Commands/effect` 的**首个修订**（[oldid=1122926, 2017-07-08](https://minecraft.wiki/w/Commands/effect?oldid=1122926)，
  即 1.12.2 时点）原文——命令「manages status effects on players and other entities」，
  `player` 参数「Must be a player name or a target selector (**`@e` is permitted to target entities other than players**)」，
  并给出例句 `effect @e[type=zombie] clear`；该页现行版本仍保持此表述。
  故本分支让 `/effect` 对**全部解析出的 pawn** 生效（Cuberite 的 `cPawn:AddEntityEffect`
  对玩家与怪物同样可用，[Pawn.h:73](../../src/Entities/Pawn.h#L73)），而不是过滤到玩家或报「多目标错误」。
  原 spec 草稿假设 `/effect` 是单目标位、只作用于玩家，属误判，现更正。

> **顺带发现的独立缺口（不在本分支）**：同一 E4 显示 1.12.2 的 `/effect` 还有第 6 个可选参数
> `[hideParticles]`，Core 的 `/effect` 只有 `[seconds] [amplifier]`；且「`seconds` 为 0 时改为取消该效果」
> 也未实现。两者属状态效果子系统，记入交付说明的已知偏差，不在 C1 范围。
- 无执行者（控制台/RCON）时：`@s` 空集；`@p` 以**默认世界的世界出生点**为原点（〔推测〕，
  待 C2 提供真正的命令原点后替换）；命令方块原点同属 C2。

### 3.5 模块接口（实际签名）

- `ParseSelector(Token)` → 选择器结构（`Variable` / `Arguments` / `Lookup` / `Raw`），失败返回 `nil, Reason`。
  **纯函数**，不触碰游戏对象。
- `ResolveSelector(Selector, Context)` → 实体列表（可能为空），失败返回 `nil, Reason`。
  `Context = { World = cWorld, Position = Vector3d, Executor = cEntity }`，**允许 `nil`**。
- `ResolveSelectorToken(Token, Context)` → 上面两步的合并入口。
- `ResolveTargets(Token, Context, Kind)` → 命令参数的统一入口：`"@…"` 走选择器，否则按**玩家名**
  交给 `FindAndDoWithPlayer`；`Kind` ∈ `"player"` / `"pawn"`（默认）/ `"entity"` 收窄实体种类。
  返回非空列表，或 `nil, Reason`。
- `GetTargetName(Entity)` → 目标的可读名（玩家名 / 自定义名 / 怪物类型名）。`cEntity` 无 `GetName()`，
  所以涉及 `@e` 的文案必须走它。
- `MakeSelectorContext(Player)` → 由执行命令的玩家构造 context；控制台返回 `nil`。

命名说明：草稿写的是 `Resolve(selector, context)`；落地改为 `ResolveSelector`，避免在 Core 的
单全局环境里占用 `Resolve` 这种过泛的名字。

## 4. 本分支范围（实际落地）

**做**：
1. Core 内新增 `target_selectors.lua`：`ParseSelector` / `ResolveSelector`（+ `ResolveSelectorToken` /
   `ResolveTargets` / `GetTargetName` / `MakeSelectorContext`）。
2. 变量 `@p @a @r @e @s`；参数 `type` / `name` / `c` / `r` / `rm` / `x` / `y` / `z` / `dx` / `dy` / `dz` / `l` / `lm` / `m` / `team`。
3. `type=` 走 `cMonster.StringToMobType`（[Monster.cpp:1140](../../src/Mobs/Monster.cpp#L1140)，
   导出见 [APIDesc.lua:9409](../../Server/Plugins/APIDump/APIDesc.lua#L9409)），大小写不敏感 + 下划线归一。
   **注意**：tolua++ 把该 static 绑成 usertable 形式（[Bindings.cpp:42768](../../src/Bindings/Bindings.cpp#L42768)
   的 `tolua_isusertable(tolua_S,1,"cMonster")`），必须**冒号**调用（`cMonster:StringToMobType(...)`）；
   `cMonster:MobTypeToString` 同理。点号调用会在运行时抛
   `argument #1 is 'string'; 'cMonster' expected`。
4. 把 `@a` 硬编码从 `cmd_effect.lua` 摘掉，改调本模块（这是 S16 §3.3 的兑现），并把其余
   **1.12.2 文档为「player name or target selector」的 Core 命令**一并接入（见 §4.1）。
5. 导出函数供 C4/C5/C6 复用；`Info.lua` 里 `/effect` 的 `HelpString` 由「a player」改为
   「players or other entities (player name or target selector)」，用法行由 `<player>` 改为 `<targets>`。

### 4.1 已接入的命令（逐命令来源）

1.12.2 的文档**没有**参数类型系统（`targets` / `player` 是 1.13 才引入的），能力按参数逐条写在
子页的散文里，标准措辞是「Must be a player name or a target selector」；总则见
[Commands rev 1152284 §Target selectors](https://minecraft.wiki/w/Commands?oldid=1152284)。
下表全部取自 **pre-17w43a**（即 1.13 首个快照前）的子页修订：

| Core 命令 | 目标参数 | 实体种类 | 多目标 | 证据（oldid，均 ≤ 2017-10-25） |
|---|---|---|---|---|
| `/effect` | `<targets>` | 玩家 + 非玩家实体 | 是 | [Commands/effect 1122926](https://minecraft.wiki/w/Commands/effect?oldid=1122926) |
| `/clear` | `[player]` | 玩家 | 是 | [Commands/clear 1122915](https://minecraft.wiki/w/Commands/clear?oldid=1122915) |
| `/kill` | `[player\|entity]` | **任意实体** | 是 | [Commands/kill 1141304](https://minecraft.wiki/w/Commands/kill?oldid=1141304) |
| `/gamemode`（+ `/gm`） | `[player]` | 玩家 | 是 | [Commands/gamemode 1160109](https://minecraft.wiki/w/Commands/gamemode?oldid=1160109) |
| `/give`、`/unsafegive` | `<player>` | 玩家 | 是 | [Commands/give 1148096](https://minecraft.wiki/w/Commands/give?oldid=1148096) |
| `/enchant` | `<player>` | 玩家 | 是 | [Commands/enchant 1143465](https://minecraft.wiki/w/Commands/enchant?oldid=1143465) |
| `/spawnpoint` | `[player]` | 玩家 | 是 | [Commands/spawnpoint 1123064](https://minecraft.wiki/w/Commands/spawnpoint?oldid=1123064) |
| `/spawn`（Cuberite 自有，非 vanilla） | `<player>` | 玩家 | 是 | 与 `/spawnpoint` 同类，为一致性接入（**无 vanilla 来源**，属本仓库扩展） |

**刻意未接入**（均有 1.12.2 期证据说明其**不接受**选择器）：

| 命令 | 为何不接入 |
|---|---|
| `/kick` | 子页只收玩家名；总则明记「No multiplayer commands permit target selectors in arguments」（[Commands/kick 1123019](https://minecraft.wiki/w/Commands/kick?oldid=1123019)） |
| `/op` | 1.12.2 只收名字；**17w45a 才加入选择器支持**（[Java Edition 17w45a](https://minecraft.wiki/w/Java_Edition_17w45a)）——顺手接入即版本污染 |
| `/deop` | 子页自相矛盾（选择器「will not evaluate」），实际等同名字 |
| `/ban` `/ban-ip` `/banlist` `/pardon` `/pardon-ip` `/whitelist` | 名字 / IP，且可对离线者生效 |
| `/rank` `/unrank` `/do` `/sudo` | Cuberite 自有，无 vanilla 语义；本次不改 |
| `/tp` `/teleport` | vanilla 形态是 `<targets> <destination>`；Core 现实现是「传送到某个目的地」，属另一套语法，改造归 C4 |
| `/tell` `/msg` `/w` | vanilla 允许选择器，但**仅限 operator**（[Commands/msg 1123078](https://minecraft.wiki/w/Commands/msg?oldid=1123078)：'or a target selector, but only if the user is an operator'）。Core 的 `core.tell` 默认人手一个、且**无** operator 概念，故本轮**不做**——需要维护者先决定门禁（建议新增一个权限节点，或复用既有 admin 节点） |

**顺带发现（不在本分支）**：vanilla **1.12.2 没有 `/gm` 别名**（2017 与现行文档均无）→ Core 的
`/gm` 是自有的；`/scoreboard players` 的名字参数在 1.12.2 **不接受**选择器（用 `*` 通配），
这点约束 C6。

**不做**（记录为后续增量）：`tag`、`score_*`、`rx/rxm/ry/rym`、`@n`、1.13+ 语法、SNBT；
`/execute`（C4）、计分板/标签子系统（C6）。

## 4.2 线程安全（本分支新增约定）

**线程模型**（源码级核实）：

- 每个世界有独立 tick 线程：`cWorld` 持有嵌套的 `cWorld::cTickThread : public cIsThread`
  （[World.h:934-948](../../src/World.h#L934)）。
- **玩家命令在执行者所在世界的 tick 线程上处理**：`cWorld::Tick` → `ProcessProtocolIn()`
  （[World.cpp:1338](../../src/World.cpp#L1338)）→ `cHandleChat` → `cServer::Command`
  （[Server.cpp:431](../../src/Server.cpp#L431)）→ `cWorld::DoWithPlayerByUUID`，后者
  `cLock Lock(*this)`（[World.cpp:3334-3345](../../src/World.cpp#L3334)）→ handler。
  **即 handler 全程持有执行者世界的 chunkmap 锁**。
- 控制台 / 未入世界的客户端走**服务端 tick 线程**（`cServer::Tick` → `TickClients` → `ServerTick`），
  该路径**不持有世界锁**。
- 枚举期间引擎是加锁的：`cWorld::ForEachPlayer` 持 `cLock`（[World.cpp:3261-3273](../../src/World.cpp#L3261)），
  `cChunkMap::ForEachEntity` 持 `cCSLock(m_CSChunks)`（[ChunkMap.cpp:981-992](../../src/ChunkMap.cpp#L981)）；
  锁是 `std::recursive_mutex`（[CriticalSection.h:45](../../src/OSSupport/CriticalSection.h#L45)）。
- `cEntity::Destroy()` **不**当场移除实体：只 `SetIsTicking(false)` 并把真正的移除排进 `QueueTask`
  （[Entity.cpp:260-280](../../src/Entities/Entity.cpp#L260)）；因此**在枚举回调里执行动作（含击杀）是安全的**，
  死亡动画/掉落由 `cMonster::KilledBy` + 1 秒计时后的 `Destroy()` 负责（[Monster.cpp:334-339](../../src/Mobs/Monster.cpp#L334)）。

**硬性约定**：**动作必须在目标所属世界的枚举回调内部执行**。

- `ResolveSelector` / `ResolveTargets` 只做**只读**选择；
- 变更必须走 `ApplyToTargets(Targets, Action)`：它按世界分组、以 `GetUniqueID()` 匹配，
  在 `World:ForEachEntity` 的回调内跑 `Action`（此时该世界锁被持有），返回实际生效数；
- **绝不在枚举返回之后**再改动实体——那是无锁访问另一个世界的实体。
- 不得把实体对象引用跨世界/跨线程传递（引擎在 `cWorld:QueueTask` 文档里明确要求存 EntityID，
  [World.lua:2590-2605](../../Server/Plugins/APIDump/Classes/World.lua#L2590)）；`ApplyToTargets` 只存 ID。
- `/give`、`/enchant` 天然满足该约定：它们把名字交回 `FindAndDoWithPlayer`/`EnchantItem`，
  后者在持锁回调内完成变更。

**为什么不用 `World:QueueTask` 做「异步动作」**：C3 要求命令方块回写**真实成功计数**、
C4 要求 `/execute` 的成功值与 `/testfor` 的命中数**同步回传**（见
[command-blocks §2.1-2.2](vanilla-1.12.2-command-blocks.md)、
[execute-and-testfor §2.6/§3.2](vanilla-1.12.2-execute-and-testfor.md)）。
异步化会让 `Execute()` 在工作发生前就得决定计数，直接破坏这两支；且排队任务在
`cWorld::TickQueuedTasks()`（[World.cpp:1354](../../src/World.cpp#L1354)）于同 tick 稍后统一执行，
命令序列不再是执行顺序。故本分支**保持同步**。

**残留（已知、记入 §7）**：跨世界 `@a`/`@e`/`@r`（纯集合形态）在动作阶段会依次取多个世界的锁
（执行者的世界锁已被持有 → A→B 嵌套）。这是既有名字路径（`FindAndDoWithPlayer`）本来就有的形态，
不是本分支引入；单独评估，不在此分支处理。

## 5. Cleanroom 裁决

**§2.1 许可证兼容例外的影响（本轮回改）**：旧版裁决把「引入 brigadier 或任何语言 port」判为违反
AGENTS.md §2.1/§2.2。这条已被现行 [AGENTS.md §2.1](../../AGENTS.md) 的**许可证兼容例外**推翻——
该条例外明确点名 brigadier 为 MIT、属「允许参照、借鉴乃至改写入库」的来源，且 §2.3 也补了
「许可证兼容例外允许借鉴的除外」。所以**不能再以 cleanroom 为由禁止参照 brigadier**。

**本分支仍选择自研，理由改为两条独立于 cleanroom 的依据**：

1. **版本语义冲突**：Brigadier 是 **1.13/17w45a** 才进入 vanilla 的命令框架
   （[minecraft.wiki/Brigadier](https://minecraft.wiki/w/Brigadier)），其选择器语法就是 §3.3 里
   那些 1.13+ 形态（引号、`scores={}`、`limit=`、`distance=`）。本仓库基线是 1.12.2
   （[AGENTS.md §9](../../AGENTS.md) 禁止顺手引入 1.13+ 行为），直接采用会把版本污染带进来。
2. **无 Lua 侧可用 port**：本轮检索中，语言为 Lua 且与选择器相关者只有 CC:Tweaked 的
   `migeyel/cbb`（MIT，自研 DSL，**无选择器**）；`NirlekaPlay/asymptote-engine` 有一份**独立实现**
   的 MC 风格选择器解析器，但其为 **GPL-3.0**——copyleft 不适用 §2.1 例外（例外第 3 条），
   只能作为「独立实现的行为观察」对照（§2.2），**不得引用其代码**，且其参数名是新版风格。
3. 结论：**按 wiki 规格自研本模块**——这是最小、可验证、不越界的路径。

## 6. 测试与验收

- **插件侧静态检查（已跑）**：`luac -p` 全部 Core `.lua` 通过；
  本分支新增/改动的三个文件 `target_selectors.lua`、`tests/test_selectors.lua`、`cmd_effect.lua`
  经 `luacheck` **0 warning / 0 error**（三者带 `-- luacheck:` 内联声明；
  Core 仓库与上游均无 `.luacheckrc`，未改动的旧文件仍有既有的 undefined-global 噪声，不在本分支范围）。
- **纯函数可测**：`lua tests/test_selectors.lua` —— 123 条断言全绿，覆盖解析器（含 1.12.2 的
  无引号/无空格/不可重复）与解析逻辑（伪造实体世界驱动 `ResolveSelector`），不依赖 Cuberite 进程。
- **Fuzz**：`tests/FuzzCommands.lua` 的 `choices` 增加 `@a`、`@p`、`@s`、`@e[type=cow]`、
  `@e[type=!player]`、`@a[c=1]` 与畸形 `@a[`（不得崩）。
  〔待核：本机无 CuberitePluginChecker，未实际跑该 fuzz；核法——按 `.github/workflows/ci.yml` 的步骤装
  Checker 后以 `-s tests/FuzzCommands.lua` 跑一遍〕。
- **父仓库侧**：本分支不改 `src/` → AGENTS 门 1–3 不适用；已自查
  `.gitmodules` 的 `url` 与 `git submodule status` 的 gitlink 一致（AGENTS §6（一）3）。
- **行为证据**：§3 逐条对表；无实机 oracle（[AGENTS.local.md §1](../../AGENTS.local.md)）→
  以规格 + 单元测试为准，`type=` 大小写等 §3.3 的〔待核〕项留待实机对照。

## 7. 已知偏差 / 风险

- **`@e` 只覆盖 pawn**（玩家 + 怪物），不含掉落物/矿车/物品展示框 → 与 vanilla 有实质差距。
  原因**不是** C++ 缺类型判定（`IsPickup`/`IsMinecart` 等已在 [Entity.h:161-171](../../src/Entities/Entity.h#L161) 导出），
  而是尚无**已核实的 1.12.2 实体 ID ↔ Cuberite 实体种类映射**；补齐映射前只放 pawn，另立分支。
  对 `/effect` 而言 pawn 恰好覆盖全部能接收效果的实体（E4 的 `@e[type=zombie]` 用例即怪物），
  故本分支的实际影响面较小；差距主要体现在 `/kill @e`、`/execute @e` 一类通用消费者上。
- **未实现的合法参数**：`tag`、`score_*`、`rx/rxm/ry/rym` → 使用时报「not supported yet」，不静默。
- **`type=` 值大小写**：来源冲突，按大小写不敏感实现〔待核〕（§3.3）。
- **`r`/`rm` 边界**：E1 措辞为「less than / more than」，现行页按闭区间；本实现取闭区间〔待核〕。
- **`dx/dy/dz` 判定**：E1 只说「within the volume」，现行页的「包围盒至少部分相交」是 1.13+ 措辞；
  本实现按包围盒相交〔待核〕。
- **`x/y/z` 单独给出不缩小候选集，但钉住维度**：E1 只隐含「不缩小候选集」；明确的
  「does not restrict」句与「position 携带维度」都是 1.13+ 措辞〔待核〕。本实现按 §3.3 的作用域规则
  取「只移原点 + 限定原点世界」——这是把「跨世界距离无定义」贯彻到底的最小自洽选择。
- **跨世界 `@p` / `c=`**：距离只在世界内良定义，故 `@p` 与任何含 `c=` 的选择器都只在**原点世界**内
  排序/取最近（§3.3）〔推测〕；vanilla 跨维度的 `@a[c=…]` 排序本轮未找到 1.12.2 专用来源〔待核〕。
- **控制台原点**：无 context 时以默认世界出生点为原点〔推测〕，待 C2 的真实命令原点替换；
  因此控制台含空间参数的选择器等价于在默认世界内选择。
- **跨世界锁序（既有）**：纯集合形态的 `@a`/`@e`/`@r` 在执行动作时会依次取多个世界的锁
  （执行者世界锁已持有 → A→B 嵌套），与既有 `FindAndDoWithPlayer` 名字路径同形；本分支未引入也未消除，
  见 §4.2 末尾，单独评估。
- **`m=` 的 `-1`**：E1 列出的取值未含 `-1` 的明确语义，本实现不接受〔待核〕。
- **`m=` 的未知取值**（含 `-1`）当前**不报解析错**，只是匹配不到任何实体；vanilla 会在参数解析期拒绝非法游戏模式。
  这是值级偏差（不会误匹配全体），记为后续增量；`type=` 的未知实体 ID 同理〔待核〕。
- **依赖库**：`lib/` 只读，本分支未动。**Core 许可证**：fork 自 `cuberite/Core`，未引入任何第三方代码，
  故 §2.1 例外的「保留版权声明」要求不适用。
