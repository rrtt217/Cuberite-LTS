# vanilla 1.12.2：服务端消息里哪些「翻译键」会被客户端翻译

> 任务：核实「Cuberite 发出的、含翻译字符串（文本组件里的 `translate` 键）的消息，哪些会被原版 1.12.2 客户端
> 翻译成本地语言」。
> 本文是**实现方交付物**：前半是客户端侧行为规格（白名单来源 + 实测），后半是本仓库的出站盘点与差距。
> 原始观测记录（19 + 2 条探针的逐条判定、工具调用、截图）在
> [vanilla-1.12.2-chat-text-translation-observation.md](vanilla-1.12.2-chat-text-translation-observation.md)，
> 本文只引用它的结论与截图路径，不重复其表格。

---

## 0. 结论摘要

1. **1.12.2 客户端只翻译 `translate` 文本组件**；键名写在 `text` 字段里永远只是字面文本（A5、B2），
   `%s` 写在 `text` 里也不会被代入（A6）。
2. `translate` 组件被渲染成本地语言句子，**服务端与客户端语言无关**：同一份 JSON，不同语言的客户端各看各的译文。
   各条渲染规则见 §2。
3. **Cuberite 今天实际产出、且能被翻译的聊天消息是 0 条**（§4）：
   - 唯一的 C++ 内建翻译键产出（`AddShowAchievementPart` → `chat.type.achievement`）在树内**没有调用点**，
     而且这个键在 1.12.2 客户端上**不存在**（渲染成键原文，B1/D4/E2）——1.12 起进度（advancement）取代了成就。
   - `AddClientTranslatedPart` 在 C++ 侧存在且序列化正确，但**没有 Lua 绑定、没有 API 文档**（运行时 `nil`），
     插件无法构造 `translate` 组件。
   - 玩家聊天、加入/离开、死亡、命令反馈**全部是硬编码英文纯文本**，vanilla 对应位置用的是 `translate` 键。
4. **能透传翻译键的三条现成通道**（无需改 `src/`）：`/tellraw`、`/title`（Core 的 JSON 组件层）与插件 API
   `cPlayer:SendMessageRaw(json)`。其中 `/tellraw` 与 actionbar 通道已实测被翻译（C2、C1）。
5. 因此「把命令反馈改成 vanilla 的 `commands.*` 翻译键」这条后续工作的前置缺口是
   **`cCompositeChat` 的 `translate` 能力没有导出到 Lua**（§5 的 G1），而不是客户端不支持。

---

## 1. 观测条件

| 项 | 值 | 出处 |
|---|---|---|
| 客户端 | Forge Mod Loader **14.23.5.2864 for Minecraft 1.12.2**，实例 `~/.minecraft` | `~/.minecraft/logs/latest.log` 启动行（23:10:41） |
| 客户端语言 | `lang:en_us` | `~/.minecraft/options.txt` 的 `lang:` 行 |
| 客户端是否原版 | **否**：`~/.minecraft/mods` 装有 mod（含观测桥）；翻译属客户端核心行为，但严格说本证据来自**已装 mod 的 Forge 客户端** | 启动日志 `Searching /home/david/.minecraft/mods for mods` |
| 服务端 | 本仓库 Cuberite，从 `build/Server` 运行，端口 25565，`Authenticate=0` | `build/Server/settings.ini`；`mcp__cuberite__get_server_info` |
| 服务端构建提交 | `a14048e8315ec6310d547cfdfdbe905732e82194`（HEAD `0eb6dc3` 运行二进制不含；两者之间 `src/CompositeChat.cpp` / `src/ClientHandle.cpp` / `src/Protocol/Protocol_1_8.cpp` 无差异，仅 `src/Bindings/ManualBindings.cpp` 与 Core gitlink 有差异） | `cRoot:Get():GetBuildCommitID()`（`execute_lua`）；`git diff --stat a14048e..HEAD` |
| 聊天前缀开关 | `UseChatPrefixes=1`（世界配置，运行期文件） | `build/Server/world/world.ini` |
| 玩家 | `rrtt217`，creative，位置整轮未变 | `mcp__cuberite__list_players` |

**证据强度**：客户端侧证据来自「服务端发一条 → 客户端截图 → 逐字判读」。截图在 `build/`（gitignore），
路径见 §8。观测期间客户端未掉线、未重连、未接受任何输入。

---

## 2. 客户端侧行为规格（每条都有实测探针）

来源：Minecraft Wiki 文本组件格式（`translate` / `with` / 槽位语义）见 §6；行为一律以 1.12.2 实机为准。

| # | 断言 | 实测 | 探针 |
|---|---|---|---|
| R1 | `{"translate":<键>,"with":[…]}` 渲染成本地语言句子，**键名不上屏** | ✔ | A1、A7 |
| R2 | `with` 的元素可以是组件对象，**也可以是裸 JSON 字符串** | ✔ 两种形态渲染完全相同 | A1（对象）/ A2（字符串） |
| R3 | 未知键 / 客户端语言表里没有的键 → **显示键原文**，`with` 参数**不出现** | ✔ | A4、D4、B1、B3 内层、E2 |
| R4 | `with` 少于槽位数 → 缺的槽位渲染为空，不报错 | ✔ `<Only>` | A3 |
| R5 | `with` 为 0 个而键有 2 个槽位 → 渲染成 `<>`（键被识别，槽位全空） | ✔ | D5 |
| R6 | 键名写在 `text` 字段里 → **只是字面文本**，不翻译 | ✔（对照组） | A5、B2 |
| R7 | `%s` / `%d` 写在 `text` 里 → 不代入，原样显示 | ✔（对照组） | A6 |
| R8 | `with` 里再嵌一个 `translate` → **递归解析**，内外两层都译 | ✔ | D3 |
| R9 | 样式（如 `"color":"red"`）作用在 `translate` 段上生效 | ✔ | B4；另 E1 证明 `with` 内层组件的颜色也生效（绿色） |
| R10 | 参数里的 `%` 不被当格式符，原样渲染 | ✔ `<100%> pct` | B5 |
| R11 | 根级 `translate`（无外层包装）同样生效 | ✔ | A7 |
| R12 | Cuberite 自己的包装形态 `{"text":<前缀>,"extra":[…组件…]}` **不影响**翻译 | ✔ | A1–A5、C2、E1/E2 全部是这个形态 |
| R13 | actionbar 通道（`SendMessageRaw(json, ctAboveActionBar)`）同样翻译 | ✔（屏幕中下方） | C1 |
| R14 | 1.12.2 客户端语言表**没有** `chat.type.achievement` | ✔ 渲染为键原文 | B1、D4、E2 |
| R15 | 1.12 时代的进度键 `chat.type.advancement.task` **有**，且可译 | ✔ `Alice has made the advancement Stone Age` | D1、E1 |
| R16 | `multiplayer.player.joined` **有**，且可译 | ✔ `Alice joined the game` | D2 |
| R17 | `chat.type.text` **有**，且可译（`<参数1> 参数2`） | ✔ | A1、A2、A3、A7、B4、B5、C2 |

**R14 / R15 / R16 的意义**：这不是「Cuberite 的 JSON 形态不对」。E1 与 E2 只差键名、其余字节形态
（`with` 第二个元素是**嵌套数组**、带 `hoverEvent`）完全相同，E1 译出、E2 不译 ⇒ 形态可用，是键不存在。
1.12 起成就被进度取代（§6），这正是 `chat.type.achievement` 消失的原因。

---

## 3. Cuberite 出站盘点

### 3.1 带翻译键的产出路径（`translate` 组件）

| # | 路径 | 产出形态 | 从 Lua 可达 | 树内调用点 |
|---|---|---|---|---|
| T1 | `cCompositeChat::AddShowAchievementPart`（[CompositeChat.cpp:91](../../src/CompositeChat.cpp)，序列化 [CompositeChat.cpp:338-362](../../src/CompositeChat.cpp)） | 根 `{"text":<前缀>}`，`extra` 里 `{"translate":"chat.type.achievement","with":[{"text":<玩家>},[{"color":"green","translate":<成就>,"hoverEvent":{…}}]]}` | **是**（手写绑定 [ManualBindings.cpp:4644](../../src/Bindings/ManualBindings.cpp)，文档 [APIDesc.lua:2275](../../Server/Plugins/APIDump/APIDesc.lua)） | **无**。唯一调用点被注释（[Player.cpp:1398](../../src/Entities/Player.cpp)），实际发的是 `AddTextPart("Achievement get!")`（[:1399](../../src/Entities/Player.cpp)） |
| T2 | `cCompositeChat::AddClientTranslatedPart`（[CompositeChat.cpp:55](../../src/CompositeChat.cpp)，序列化 [CompositeChat.cpp:297-310](../../src/CompositeChat.cpp)，声明 [CompositeChat.h:121](../../src/CompositeChat.h)） | `{"translate":<键>,"with":[<裸字符串>,…]}`（R2 证明裸字符串客户端接受） | **否**。运行时 `type(cCompositeChat().AddClientTranslatedPart) == "nil"`（`execute_lua` 实测）；APIDump 无此条目 | 无 |
| T3 | `/tellraw <player> <json>`（[cmd_tellraw.lua:38](../../Server/Plugins/Core/cmd_tellraw.lua)） | 玩家给的 JSON 原样进 Chat 包；只拒 `score`/`selector` 与含 `${}` 的文本 | 是（命令） | 有 |
| T4 | `/title … title\|subtitle\|actionbar <json>`（Core 的 JSON 组件层） | 原样进 Title 包或 Chat 包 | 是（命令） | 有（未探测，见 §7） |
| T5 | `cPlayer:SendMessageRaw(json[, eChatType])`（[Player.cpp:1261](../../src/Entities/Player.cpp) → [ClientHandle.cpp:2521](../../src/ClientHandle.cpp) → [Protocol_1_8.cpp:334](../../src/Protocol/Protocol_1_8.cpp)） | 任意 JSON | 是（已文档化） | 插件可用 |

### 3.2 硬编码文本的产出路径（vanilla 用 `translate`，这里用英文纯文本）

| # | 消息 | Cuberite 现状 | vanilla 对应 |
|---|---|---|---|
| P1 | 玩家聊天 | `<` + 前缀 + 名字 + 后缀 + `> ` + 正文，全是 `AddTextPart`（[ClientHandle.cpp:1603-1615](../../src/ClientHandle.cpp)） | R17 的 `chat.type.text` |
| P2 | 加入 | `"{} has joined the game"`（[ClientHandle.cpp:458](../../src/ClientHandle.cpp)） | R16 的 `multiplayer.player.joined` |
| P3 | 离开 | `"{} has left the game"`（[Player.cpp:3103](../../src/Entities/Player.cpp)） | `multiplayer.player.left`（本键未单独实测，见 §7） |
| P4 | 死亡 | `X was killed by Y` 等一整套英文伤害文案（[Entity.cpp:2437-2503](../../src/Entities/Entity.cpp)） | `death.attack.*` / `death.fell.*` 等（未实测） |
| P5 | 成就 | `"Achievement get!"`（[Player.cpp:1399](../../src/Entities/Player.cpp)） | 1.12.2 是进度播报（`chat.type.advancement.*`，R15 证明该族键可用） |
| P6 | 命令反馈 | Core 的英文句子（如 [cmd_tellraw.lua:48](../../Server/Plugins/Core/cmd_tellraw.lua)） | `commands.*`（`/stopsound` 已抓包确认为 `commands.stopsound.success.individualSound`，见 [presentation 规格](../commands/vanilla-1.12.2-presentation-commands.md) §2.4） |
| P7 | 私聊前缀 | `[MSG: <对方>]`（[ClientHandle.cpp:184-194](../../src/ClientHandle.cpp)） | `commands.message.display.incoming/outgoing`（未实测） |

---

## 4. 结论：今天哪些 Cuberite 消息能被原版客户端翻译

| 来源 | 能否被翻译 | 依据 |
|---|---|---|
| 成就播报（T1） | **不能**：① 树内没有调用点（发的是 `"Achievement get!"` 纯文本）② 即使调用，`chat.type.achievement` 在 1.12.2 上不存在（R14） | B1、D4、E2 |
| 插件自建 `translate` 组件（T2） | **不能**：能力存在但没导出到 Lua | 运行时 `nil` |
| `/tellraw`（T3） | **能**，只要键在客户端语言表里 | C2（`chat.type.text` 译出） |
| `/title`（T4） | 未实测（§7） | — |
| 插件 `SendMessageRaw`（T5） | **能**，同 T3；actionbar 变体同样能 | A1–A7、C1、D1–D5、E1（全部经此 API） |
| P1–P7 的内建消息 | **都不能**：发的是纯文本，不是 `translate` 组件 | A5/A6/B2 对照（纯文本永不翻译） |

一句话：**服务端能「让客户端翻译」的前提是发 `translate` 组件；Cuberite 今天只有玩家自己经 `/tellraw`、
`/title` 或插件脚本手写 JSON 时才做得到，服务器自身产生的所有消息都不是翻译组件。**

---

## 5. 差距清单（本次不实现，等维护者下达）

- **G1（前置）**：`cCompositeChat:AddClientTranslatedPart` 未导出到 Lua、未写 APIDump。要让 Core 用
  `commands.*` 键写反馈，这是必须先补的一环；序列化侧不用改（R2 已证明裸字符串 `with` 客户端接受）。
- **G2**：`AddShowAchievementPart` 用的 `chat.type.achievement` 是 1.12 之前的键，1.12.2 上不可译（R14）；
  若恢复该功能，应改用 `chat.type.advancement.*` 一族（R15 证明可用）。其 `hoverEvent` 行动名
  `show_achievement` 是否还是 1.12.2 的合法值**未核实**（§7）。
- **G3**：P1–P7 全部硬编码英文 ⇒ 非英语客户端看不到任何本地化。vanilla 在这些位置都发 `translate`。
- **G4**：`/tellraw`、`/title`、`SendMessageRaw` 已经能透传翻译键，**不需要改**（本条是「不用做」的结论）。

---

## 6. 来源

| 用途 | 来源 |
|---|---|
| `translate` / `with` / 槽位语义、未知键回退为键原文、「翻译跟随客户端语言」 | [Text component format（Java Edition 1.21.5 之前的 JSON 形态）](https://minecraft.wiki/w/Text_component_format/Before_Java_Edition_1.21.5)：*"Displays a translated piece of text from the currently selected language… If no corresponding translation can be found, the identifier itself is used as the translated text."*；`with` — *"A list of text components to be inserted into slots in the translation text"*，槽位形如 `%s` / `%3$s` |
| 1.12 起成就被进度取代（解释 R14） | [Advancement](https://minecraft.wiki/w/Advancement) 的历史表：*"1.12 \| 17w13a \| Added advancements, replacing achievements."* |
| 1.12.2 实机行为（R1–R17 的落地判据） | 本目录的观测记录 [vanilla-1.12.2-chat-text-translation-observation.md](vanilla-1.12.2-chat-text-translation-observation.md) + §8 的截图 |
| `/stopsound` 的 `commands.*` 抓包 | [presentation 规格](../commands/vanilla-1.12.2-presentation-commands.md) §2.4 |

**版本差异提醒**：上面那份 Wiki 页面描述的是「1.21.5 之前的 JSON 形态」，其中 `fallback` 字段是
比 1.12.2 晚得多的新增项——**不要据此认为 1.12.2 支持 `fallback`**。本文所有断言都只采信 1.12.2 实机结果。

---

## 7. 未核实 / 推测

- 〔待核〕**`/title` 的 title/subtitle 通道是否同样翻译**：走的是 Title 包而非 Chat 包，本次未探测。
  核法：`/title @s title {"translate":"chat.type.text","with":[{"text":"A"},{"text":"B"}]}` 后截图。
- 〔待核〕**P3/P4/P7 对应的 vanilla 键名**（`multiplayer.player.left`、`death.*`、`commands.message.*`）：
  本次只实测了 `multiplayer.player.joined`；其余键名按 vanilla 命名习惯推导，未逐条上机。
  核法：各发一条 `translate` 组件看是否译出（与 D2 同法）。
- 〔待核〕**`show_achievement` 悬停动作在 1.12.2 是否合法**：E1/E2 带着它仍正常渲染文本，但没有验证悬停行为。
- 〔待核〕**颜色作用范围的精确边界**：B4 判读为「`[B4]` 与 `<Alice>` 白、`colored` 红」，是 854×480 原生分辨率下的目视判读；
  E1「`Stone Age` 呈绿色、其余白」在同分辨率下清晰可辨，据此可靠性提高，但仍只是屏幕事实。
  核法：放大截图局部，或在更大窗口 / 更高 GUI 缩放下重拍同一条。
- 〔待核〕**纯净原版客户端**：本次客户端是装了 mod 的 Forge 1.12.2（§1）。翻译属客户端核心行为，
  但没有在无 mod 客户端上重复。核法：同一批探针在无 mod 客户端重跑。
- 〔推测〕**「未知键 ⇒ 参数被丢弃」在 1.12.2 普遍成立**（样本 5：A4、D4、B1、B3 内层、E2）。置信度较高：
  与 Wiki「回退为键原文」一致——没有模板就没有槽位可填。依据与置信度如上。
- 〔推测〕**R3 对「客户端语言表里存在、但本条 %s 数量不匹配」的键不适用**：本次只测了参数偏少（R4/R5），
  没测槽位多于参数以外的格式异常。置信度中。

---

## 8. 证据索引

截图全部在 `build/`（gitignore，不入库）：

| 文件 | 内容 |
|---|---|
| `build/oracle-l10n-A.png` | A1–A7：已知键（对象/字符串参数）、缺参、未知键、两条对照、根级 |
| `build/oracle-l10n-B.png`、`-B2.png` | B1–B5：`AddShowAchievementPart` 真实产出、纯文本对照、嵌套、样式、百分号；B2 为原样重发复拍 |
| `build/oracle-l10n-C1.png` | actionbar 通道翻译 |
| `build/oracle-l10n-C2.png` | 控制台 `/tellraw` 通道翻译 |
| `build/oracle-l10n-D.png` | D1–D5：进度键、`multiplayer.player.joined`、嵌套递归、同键无嵌套、零参数 |
| `build/oracle-l10n-E.png`、`-E2.png`、`-E-retry.png` | E1/E2：Cuberite 成就消息的**确切形态**换键名对照 |

（完整截图清单与时序对照图见观测记录 §5。）

复现方式（探针原文逐字记录在观测记录的附录 A）：`mcp__cuberite__execute_lua` 发消息 →
静置 → `screenshot_to_file` → `read_image` 判读。

**两个会骗人的坑（本轮都踩过）**：
1. **帧滞后**：发完立刻截图拿到的是消息到达**之前**的帧（观测记录 §1.5 的 lag0/lag1 对照）。0.7–0.9 s 通常够，
   但 E 批静置 850 ms 仍是空聊天区、约 1.2 s 才可见 —— **看到空聊天区必须多拍一次再下结论**，不能记成「没渲染」。
2. **Lua 长括号被提前截断**：探针 JSON 里若出现 `]]`（例如 `with` 的嵌套数组紧贴结尾），`[[ … ]]` 会提前闭合，
   整段 Lua 语法报错、一条都不发。含嵌套数组时改用 `[==[ … ]==]`。
