# vanilla 1.12.2 客户端：含翻译键（translate）文本组件的聊天消息是否被渲染成译文

> 只读观测记录。本文只记录「屏幕上出现了什么」，不含实现建议、不涉及任何客户端内部名称、
> 不引用任何反编译或映射内容。服务端侧一律用本仓库自己的词汇（SendMessageRaw / cCompositeChat /
> AddTextPart / AddShowAchievementPart / ctAboveActionBar）。

- 观测日期：2026-10-22（本机时间：A/B/C 三批 23:16–23:20，追加的 D 批 23:21）
- 观测者：观测子 Agent（只读角色）
- 被测服务端：本仓库 Cuberite 实例，从 build/Server 运行，端口 25565（观测期间未停、未重启）
- 被测客户端：人类已拉起、并已连上该服务器的 1.12.2 客户端，玩家名 rrtt217（观测期间未启停、未重连、未输入）
- 产物：本文件 + build/ 下的截图（build/ 被 gitignore）

---

## 0. 屏幕事实摘要（不含成因判断）

1. 服务端用 SendMessageRaw 发出的「根组件带 translate 键 + with 参数」的消息，**被渲染成句子**，
   键名本身没有出现在屏幕上：A1、A2、A7、B4、B5、C1、C2 全部如此。
2. 未知键（cuberite.missing.key）**不被翻译**：屏幕上直接是键原文，且它的 with 参数没有出现在屏幕上（A4）。
3. with 里再嵌一个 translate 时，**内层显示为键原文**，内层的 with 参数（A / B）没有出现在屏幕上（B3）；
   外层 chat.type.text 仍然被渲染成了句子形状（B3 行 = 「[B3] <Alice> chat.type.achievement」）。
4. 键名写在 text 字段里时**不会被翻译**，原样显示（A5、B2）——两条对照组都符合预期。
5. 通过 cCompositeChat:AddShowAchievementPart 发出的那一条（B1），屏幕上显示为键原文
   「chat.type.achievement」，**不是**成就句子；它与紧随其后的 AddTextPart 纯文本条（B2）在屏幕上
   **逐字相同**。
6. 颜色可以生效：给 translate 那一段指定 color=red 后，行尾参数「colored」呈红色（B4，另见 §4 存疑）。
7. 参数里的百分号不被当成格式符：参数「100%」原样渲染（B5）。
8. actionbar 通道（ctAboveActionBar）同样被渲染成句子（C1）；控制台 tellraw 通道同样（C2）。
9. **批次 D（追加）把「键不存在」与「嵌套失效」两种解释分开了**：
   - 1.12 时代的进度键 chat.type.advancement.task **被翻译**（D1：「Alice has made the advancement Stone Age」）；
   - 通用键 multiplayer.player.joined **被翻译**（D2：「Alice joined the game」）；
   - 同一个键**嵌套在 with 里会被递归翻译**（D3：「[D3] <Alice> <Bob> nested」，内外两层都译了）；
   - 而 chat.type.achievement **不带任何嵌套、直接发出时仍然显示为键原文**（D4）。
   ⇒ 屏幕证据指向：**这台 1.12.2 客户端的语言表里没有 chat.type.achievement 这一条**；
     B1 与 B3 内层显示为键原文与此一致，**不是**「嵌套 / 包装形态失效」。
10. **批次 E（追加）把「复合路径的 with 形态」也闭环了**：用与 B1 相同的形态（with 的第二个元素是
    **嵌套数组**，数组元素带 color=green 与 hoverEvent）分别配**已知键**与**该缺失键**：
    - E1（配 chat.type.advancement.task）→ 屏幕「[E1] Alice has made the advancement Stone Age」，
      其中 **Stone Age 呈绿色** ⇒ 该形态本身可用，连数组元素自带的颜色都生效；
    - E2（配 chat.type.achievement，其余完全相同）→ 屏幕「[E2] chat.type.achievement」。
    ⇒ B1 显示为键原文**不再**需要用「服务端组装出的形态可能坏了」来解释：同一形态 + 已知键可译，
      同一形态 + 该键不可译。

---

## 1. 观测条件

### 1.1 客户端侧读数（工具返回值逐项抄录）

mcp__mcmm__ping →

    {"result": "pong"}

mcp__mcmm__get_minecraft_status →

    {
      "connected": true,
      "port": 9876,
      "processAlive": false
    }

mcp__mcmm__get_player_info →

    {
      "name": "rrtt217",
      "health": 20,
      "pos": "20.7 63.0 -1.9",
      "rotation": "49.3 0.0",
      "dimension": "overworld",
      "gamemode": "survival",
      "food": 17
    }

mcp__mcmm__get_world_info →

    {
      "world_name": "MpServer",
      "difficulty": "normal",
      "gametype": "survival",
      "time": 0,
      "weather": "clear"
    }

**哪些字段不可信（本会话内有反例）：**

| 字段 | 读数 | 反例 | 结论 |
|---|---|---|---|
| get_player_info.gamemode | survival | 同一次观测中服务端 mcp__cuberite__list_players 报 rrtt217 的 gamemode 为 **creative** | 不可信 |
| get_world_info.gametype | survival | 同上（同一客户端、同一时刻） | 不可信 |
| get_minecraft_status.processAlive | false | 客户端显然在运行：本任务期间截图全部成功、画面在动（云的位置在不同截图间变化） | 不可信 |
| get_world_info.time | 0 | 截图显示昼间天光、天空明亮、太阳高挂（见 oracle-l10n-A.png 左上区域） | 不能当作世界时间读数〔推测：该字段未与服务器同步，见 §4〕 |
| get_world_info.world_name | MpServer | 无直接反例；按工作区环境记录该字段不宜当世界标识 | 本次仅记录，不作依据 |

### 1.2 服务端侧读数（对偶观测）

mcp__cuberite__list_players →

    {"count": 1, "players": [{"gamemode": "creative", "health": 20, "name": "rrtt217",
      "x": 20.699999988079071, "y": 63.0, "z": -1.8859692698797068}]}

mcp__cuberite__get_server_info →

    {"authenticating": false, "description": "Cuberite - in C++!", "hardcore": false,
     "loaded_chunks": 441.0, "max_players": 100.0, "online_players": 1.0, "uptime_seconds": 6255.0}

（uptime_seconds 6255 为 §1.1/§1.2 读数时刻的值；位置 (20.7, 63.0, −1.89) 在整轮观测结束时未变，
说明客户端没有重连、玩家没有移动。）

### 1.3 客户端类型与 loader

本任务只读工具里 **没有任何一个字段返回客户端版本或 loader 信息**：
mcp__mcmm__get_minecraft_status 只有 connected / port / processAlive 三个键（见 §1.1），
mcp__mcmm__ping 只返回 pong。

⇒ 客户端为「1.12.2 + Cleanroom Loader 0.6.13-alpha + 约 20 个 mod 的 Forge 系客户端」这一点**本次未由工具读到**，
来源是工作区环境记录 / 主 Agent 提供 ⇒ 标 **〔待核〕**，核法见 §4.1。

### 1.4 客户端语言环境（反推与其局限）

反推依据：A1 渲染为「[A1] <Alice> hi」、A7 渲染为「<Root> direct」，即 chat.type.text 被渲染成
「<第一个参数> 第二个参数」形状的**英文语序**句子。

A1 单独**不足以**定语言：chat.type.text 这一条在英文与中文语言环境下**形状相同**（都是「<玩家> 消息」），
而且 B1、B3（唯一涉及 chat.type.achievement 的两条）在屏幕上只剩键名。也就是说，只靠 A 批与 B 批，
语言环境是判不出来的。

**追加批次 D 之后有了真正的语言信号：** D1 屏幕上是「Alice has made the advancement Stone Age」、
D2 是「Alice joined the game」。这两句里的「has made the advancement」/「joined the game」**不在发出的 JSON 里**
（JSON 里只有键名和两个参数），只能来自客户端自己的语言表；而语言表给出的是英文措辞
⇒ **该客户端的语言表是英文**（不是中文，否则 D1/D2 会以中文措辞出现）。

⇒ 语言环境：**英文**；具体变体（en_us / en_gb 之类）**〔待核〕**，核法见 §4.1 第 2 条。

### 1.5 采集方法与时序控制（重要，影响证据解读）

1. **截图相对消息到达有一帧以上的滞后。** 控制实验（不是行为探针）：用 execute_lua 发一条纯文本
   「[ctl] lag-check marker」后**立刻**截图（oracle-l10n-lag0.png）→ 聊天区**空白**；等 0.8 s 再截
   （oracle-l10n-lag1.png）→ 该行正常出现在屏幕左下聊天区。所以「立刻截图」会拍到消息到达之前的帧。
2. 因此所有正式截图都在 execute_lua 之后**静置 0.7–0.9 s** 再截。
3. 第一次拍 A 批（同名文件，已被复拍覆盖）就是违反了第 1 条：当时 0 ms 延迟，拍到的是空白聊天区；
   随后用同样代码重发一次、静置 0.9 s，才得到本文引用的 oracle-l10n-A.png。
4. 聊天行约 10 s 后淡出：批次之间等待 ≥ 12 s，避免批次间叠行。B 批额外重发过一次（B2 文件）以确认可重现。
5. 截图分辨率 854×480，等于客户端窗口原生大小（非缩放）。逐字抄录按原生分辨率判读；字号很小，
   个别空格的存否不可分辨（见 §4）。
6. **滞后不是定值**：批次 E 第一次拍摄用了 850 ms 延迟，聊天区仍是空的；再等约 1.2 s 重拍才看到那两行。
   ⇒ 0.7–0.9 s 只是「通常够」，**不是保证**；判据是「多截一次确认」，不能只看一张空图就断言「屏幕上没东西」。
   本批最终采用静置 1.5 s 的那一张（oracle-l10n-E2.png）。

---

## 2. 探针与判定

判定口径（按派发要求）：translated = 渲染成句子、键原文**没有**出现在屏幕上；key-literal = 屏幕上出现键原文
（chat.type.text / cuberite.missing.key / chat.type.achievement）；other = 其它。

说明：本次实际共 **21 条**探针（A1–A7 共 7 条 + B1–B5 共 5 条 + C1、C2 共 2 条 + 追加的 D1–D5 共 5 条 +
追加的 E1、E2 共 2 条）；派发时说的 13 条与实际批次条数不符，这里按实际全部列出。
颜色一律指屏幕上判读到的字色；「白色」指默认聊天字色。

| 探针 | 发出的 JSON 原文 | 屏幕渲染原文（逐字） | 颜色 | 判定 | 截图（绝对路径） |
|---|---|---|---|---|---|
| A1 | {"text":"[A1] ","extra":[{"translate":"chat.type.text","with":[{"text":"Alice"},{"text":"hi"}]}]} | [A1] <Alice> hi | 白 | translated | /home/david/Cuberite-LTS/build/oracle-l10n-A.png |
| A2 | {"text":"[A2] ","extra":[{"translate":"chat.type.text","with":["Alice","hi"]}]} | [A2] <Alice> hi | 白 | translated | 同上（oracle-l10n-A.png） |
| A3 | {"text":"[A3] ","extra":[{"translate":"chat.type.text","with":[{"text":"Only"}]}]} | [A3] <Only> | 白 | translated（只代入 1 个参数，第 2 个位置为空，未报错、未显示键名） | 同上 |
| A4 | {"text":"[A4] ","extra":[{"translate":"cuberite.missing.key","with":[{"text":"X"}]}]} | [A4] cuberite.missing.key | 白 | key-literal（参数 X 未出现） | 同上 |
| A5 | {"text":"[A5] chat.type.text"} | [A5] chat.type.text | 白 | key-literal（对照组：键名写在 text 里就不翻译） | 同上 |
| A6 | {"text":"[A6] %s has just earned the achievement %s"} | [A6] %s has just earned the achievement %s | 白 | key-literal / 原文回显（对照组：%s 未被代入） | 同上 |
| A7 | {"translate":"chat.type.text","with":[{"text":"Root"},{"text":"direct"}]}（根级组件，无 [Ax] 标签） | <Root> direct | 白 | translated | 同上（该截图的最后一行） |
| B1 | 服务端侧 cCompositeChat + AddShowAchievementPart("Alice", "Taking Inventory")，经 SendMessage（未手写 JSON） | chat.type.achievement | 白 | key-literal（两个无标签行中的**第 1 行**；成就句子与两个参数均未出现在屏幕上） | /home/david/Cuberite-LTS/build/oracle-l10n-B.png（复拍：oracle-l10n-B2.png） |
| B2 | 服务端侧 cCompositeChat + AddTextPart("chat.type.achievement")，经 SendMessage | chat.type.achievement | 白 | key-literal（对照组；两个无标签行中的**第 2 行**，与 B1 逐字相同） | 同上 |
| B3 | {"text":"[B3] ","extra":[{"translate":"chat.type.text","with":[{"text":"Alice"},{"translate":"chat.type.achievement","with":[{"text":"A"},{"text":"B"}]}]}]} | [B3] <Alice> chat.type.achievement | 白 | key-literal（外层已译；内层键原文泄漏，内层参数 A、B 未出现） | 同上 |
| B4 | {"text":"[B4] ","extra":[{"translate":"chat.type.text","color":"red","with":[{"text":"Alice"},{"text":"colored"}]}]} | [B4] <Alice> colored | [B4] 与 <Alice> 判读为白；colored 为**红** | translated（颜色生效，作用范围见 §4） | 同上 |
| B5 | {"text":"[B5] ","extra":[{"translate":"chat.type.text","with":[{"text":"100%"},{"text":"pct"}]}]} | [B5] <100%> pct | 白 | translated（% 未被当格式符） | 同上 |
| D1 | {"text":"[D1] ","extra":[{"translate":"chat.type.advancement.task","with":[{"text":"Alice"},{"text":"Stone Age"}]}]} | [D1] Alice has made the advancement Stone Age | 白 | translated（1.12 时代的进度键） | /home/david/Cuberite-LTS/build/oracle-l10n-D.png |
| D2 | {"text":"[D2] ","extra":[{"translate":"multiplayer.player.joined","with":[{"text":"Alice"}]}]} | [D2] Alice joined the game | 白 | translated | 同上（oracle-l10n-D.png） |
| D3 | {"text":"[D3] ","extra":[{"translate":"chat.type.text","with":[{"text":"Alice"},{"translate":"chat.type.text","with":[{"text":"Bob"},{"text":"nested"}]}]}]} | [D3] <Alice> <Bob> nested | 白 | translated（内层 translate **被递归翻译**：内外两层都成句子） | 同上 |
| D4 | {"text":"[D4] ","extra":[{"translate":"chat.type.achievement","with":[{"text":"Alice"},{"text":"Taking Inventory"}]}]} | [D4] chat.type.achievement | 白 | key-literal（**无任何嵌套、直接发出**；两个参数未出现在屏幕上） | 同上 |
| D5 | {"text":"[D5] ","extra":[{"translate":"chat.type.text"}]} | [D5] <> | 白 | translated（键被识别；两个占位符都代入空串 ⇒ 屏幕为「<>」，未回退成键名） | 同上 |
| E1 | {"text":"[E1] ","extra":[{"translate":"chat.type.advancement.task","with":[{"text":"Alice"},[{"color":"green","translate":"Stone Age","hoverEvent":{"action":"show_achievement","value":"Stone Age"}}]]}]} | [E1] Alice has made the advancement Stone Age（其中 Stone Age 为**绿**） | [E1] 与 Alice 白；Stone Age 绿 | translated（与 B1 同一 with 形态：第二个参数是**嵌套数组**且带 hoverEvent，照样渲染） | /home/david/Cuberite-LTS/build/oracle-l10n-E2.png（另一张同内容：oracle-l10n-E.png） |
| E2 | {"text":"[E2] ","extra":[{"translate":"chat.type.achievement","with":[{"text":"Alice"},[{"color":"green","translate":"Taking Inventory","hoverEvent":{"action":"show_achievement","value":"Taking Inventory"}}]]}]} | [E2] chat.type.achievement | 白（嵌套数组里的「Taking Inventory」未出现，其绿色也未出现） | key-literal（与 E1 仅键名不同） | 同上 |
| C1 | {"text":"[C1] ","extra":[{"translate":"chat.type.text","with":[{"text":"Alice"},{"text":"actionbar"}]}]}，SendMessageRaw 第二参数 ctAboveActionBar | [C1] <Alice> actionbar | 白 | translated（**actionbar 位置**：屏幕中下方、快捷栏上方，非左下聊天区） | /home/david/Cuberite-LTS/build/oracle-l10n-C1.png |
| C2 | 控制台命令：tellraw rrtt217 {"translate":"chat.type.text","with":[{"text":"Alice"},{"text":"tellraw"}]} | <Alice> tellraw | 白 | translated（**左下聊天区**；无反编译、无标签，命令行本身没带回显） | /home/david/Cuberite-LTS/build/oracle-l10n-C2.png |

屏幕位置补充：A1–A7 共 7 行同屏，从聊天区顶端向下依次排列；B 批为「chat.type.achievement /
chat.type.achievement / [B3] … / [B4] … / [B5] …」5 行；D 批为「[D1] … / [D2] … / [D3] … / [D4] … / [D5] <>」
5 行；E 批 2 行（[E1] … / [E2] …）；C1 单独出现在屏幕中下部的 actionbar 位置；C2 单独出现在左下聊天区。

---

## 3. 每条判定的取证方式（工具调用 → 结论）

### 3.1 批次 A（A1–A7）

1. mcp__cuberite__execute_lua，code 为附录 A.1 的 Lua 原文（逐字），返回
   {"output": "batch A: 7 messages to 1 player(s)", "success": true}。
2. 静置 0.9 s 后 mcp__mcmm__screenshot_to_file，path = /home/david/Cuberite-LTS/build/oracle-l10n-A.png，
   返回 size 182901。
3. read_image 同一路径，逐字读出上表 7 行 → A1/A2/A3/A7 = translated；A4/A5/A6 = key-literal。
4. 时序控制见 §1.5（lag0/lag1 两次截图）；第一批的错误截图（0 ms 延迟、聊天区空白）说明「无变化」
   在当时并不成立，只是拍早了。

### 3.2 批次 B（B1–B5）

1. mcp__cuberite__execute_lua，code 为附录 A.2 的 Lua 原文（逐字），返回
   {"output": "batch B: 5 messages to 1 player(s)", "success": true}。
2. 静置 0.9 s 后 mcp__mcmm__screenshot_to_file → /home/david/Cuberite-LTS/build/oracle-l10n-B.png（size 186953）。
3. read_image 读出 5 行；其中前两行都只有「chat.type.achievement」这串字、都不带 [Bx] 标签。
   按发送顺序：先 SendMessage(m1 = AddShowAchievementPart) 后 SendMessage(m2 = AddTextPart)，
   聊天区自下而上为「新的在下」，故第 1 行 = B1、第 2 行 = B2。
4. **可重现性复拍**：把批次 B 的 Lua 原样再发一次（先等 12 s 让上一批淡出），
   mcp__mcmm__screenshot_to_file → /home/david/Cuberite-LTS/build/oracle-l10n-B2.png（size 188792），
   read_image 读到的 5 行与上一次**逐字相同**（画面只有云的位置不同）⇒ B1 显示为键原文不是一次性抖动。

### 3.3 批次 C1（actionbar）

1. mcp__cuberite__execute_lua，code 为附录 A.3（SendMessageRaw 第二参数 ctAboveActionBar），返回
   {"output": "batch C1 to 1", "success": true}。
2. 静置 0.7 s 后 mcp__mcmm__screenshot_to_file → /home/david/Cuberite-LTS/build/oracle-l10n-C1.png（size 193705）。
3. read_image：屏幕中下方（快捷栏上方、地平线下方）出现「[C1] <Alice> actionbar」→ translated。
   左下聊天区此时为空。

### 3.4 批次 C2（控制台 tellraw）

1. mcp__cuberite__run_console_command，command =
   tellraw rrtt217 {"translate":"chat.type.text","with":[{"text":"Alice"},{"text":"tellraw"}]}
   → 返回 {"output": "Successfully sent the message to rrtt217", "success": true}。
2. 静置 0.9 s 后 mcp__mcmm__screenshot_to_file → /home/david/Cuberite-LTS/build/oracle-l10n-C2.png（size 192445）。
3. read_image：左下聊天区一行「<Alice> tellraw」→ translated。此截图中 C1 的 actionbar 已淡出，
   说明 actionbar 存活时间 < 本次操作间隔（数秒级）。

### 3.5 观测期间客户端状态

整轮观测结束时重读：mcp__cuberite__list_players 仍报 1 名玩家 rrtt217、位置 (20.7, 63.0, −1.89)、
gamemode creative；mcp__mcmm__get_player_info 仍返回同一 pos/rotation。⇒ 观测期间客户端未掉线、未重连。

### 3.6 批次 D（追加；用来分离「键不存在」与「嵌套失效」两种解释）

1. mcp__cuberite__execute_lua，code 为附录 A.5（逐字），返回
   {"output": "batch D: 5 messages to 1", "success": true}。
2. 先用 0 ms 延迟拍了一张 → /home/david/Cuberite-LTS/build/oracle-l10n-D-imm.png；静置 0.9 s 后再拍
   → /home/david/Cuberite-LTS/build/oracle-l10n-D.png（size 186324）。**以后者为准**（帧滞后见 §1.5）。
3. read_image 读出 5 行（见 §2 表）。要点：
   - D1 的「has made the advancement」这段措辞**不在发出的 JSON 里**（JSON 只有键名与两个参数），
     只能来自客户端自己的语言表 ⇒ 该语言表在 1.12 时代的进度键上是可用的、且给的是英文措辞。
   - D3 把 chat.type.text 再嵌套进 with 的第二个参数，屏幕得到「[D3] <Alice> <Bob> nested」：
     外层与内层**都**被翻译 ⇒ **嵌套形态本身有效**。
   - D4 发出**同一个 chat.type.achievement 键、不带任何嵌套**，屏幕上仍是键原文 ⇒ 与 B1、B3 内层一致。
4. 三条合起来：B1 与 B3 内层显示为键原文，与「嵌套 / 包装形态失效」**不符**；
   与「这台 1.12.2 客户端的语言表里没有 chat.type.achievement 这一条」**相符**。

### 3.7 批次 E（追加；把「复合路径的 with 形态能不能渲染」也关掉）

1. 第一次调用 mcp__cuberite__execute_lua 时**失败**（Lua 语法错，未发出任何消息）：
   {"error":"Plugins/MCPServer/tools.lua:722: [string \"local sent = 0...\"]:3: ')' expected near '}'",
    "output":"","success":false}。原因是这批 JSON 里含 **}]]** 序列（嵌套数组 + with 数组的双重闭合），
   把 Lua 的长括号字符串提前截断。改把字符串定界符换成 [==[ … ]==]（**JSON 内容一字未改**）后成功。
   该次失败的截图已用正式截图覆盖。
2. 成功的调用返回 {"output": "batch E: 2 messages to 1", "success": true}。
3. **滞后异常**：静置 850 ms 拍的第一张（oracle-l10n-E.png）聊天区是空的；再等约 1.2 s 重拍
   （oracle-l10n-E-retry.png）才看到两行 ⇒ 属于 §1.5 第 6 条记的「滞后不是定值」，不是「消息没发出」。
4. **可重现复拍**：等上一批淡出后，用同一段 Lua（同样 [==[ ]==]）原样再发一次，静置 1.5 s 拍
   /home/david/Cuberite-LTS/build/oracle-l10n-E2.png，read_image 得到清晰两行：
   - [E1] Alice has made the advancement Stone Age（Stone Age 绿色）
   - [E2] chat.type.achievement
   两次拍摄的**文本内容一致**，后者对比度更好，故正文引用 E2 这一张。
5. 判读：E1 与 E2 的 with 形态**完全相同**（第二个参数都是嵌套数组、都带 color=green 与 hoverEvent），
   唯一差别是键名。E1 出句子、E2 出键原文 ⇒ **形态可用，卡的是键**。

---

## 4. 未核实 / 存疑

### 4.1 〔待核〕

1. **客户端版本与 loader**：get_minecraft_status 不返回这两个信息（§1.3）。
   核法：读 bridge 的 /api/status（curl http://127.0.0.1:9876/api/status，看其中的版本 / loader 字段），
   或由人类在启动器里确认实例名与 loader 版本。本次按派发约束（工具白名单只有截图与指定 MCP 工具）未执行。
2. **客户端界面语言的具体变体**：由 D1/D2 的英文措辞（§1.4）已可确定语言表是英文，但究竟是 en_us
   还是 en_gb 之类，本次未区分。核法：读客户端实例目录下 options.txt 里的 lang: 行
   （本次 read 只被允许读本 spec，未读）。
3. **【已闭环，不再是待核】B1 为什么是键原文**：
   批次 E 用与 B1 **相同**的 with 形态（第二个参数是嵌套数组，数组元素带 color=green 与 hoverEvent）
   做了只差键名的对照：E1 配已知键 chat.type.advancement.task ⇒ 屏幕出句子
   （「[E1] Alice has made the advancement Stone Age」，Stone Age 绿色）；E2 配 chat.type.achievement、
   其余完全相同 ⇒ 屏幕仍是键原文。再加上 D4（同键、无嵌套、直接发 translate）也是键原文
   ⇒ 「复合路径组装出的形态本身不能渲染」这一解释**在屏幕层已被排除**，与本观测相符的唯一解释是
   「该键在这台 1.12.2 客户端的语言表里不存在」。
   仅存一项事实性限制（不是待解释项，也不影响上面的判定）：本轮**没有抓线上字节**，
   因此「Cuberite 经 SendMessage 组装出的字节与 E2 手写 JSON 逐字节等价」未经抓包核对；
   核法：用 Tools/ProtoProxy（或等价抓包）比对 outbound 聊天包字节与 execute_lua 里发出的内容。
4. **B3 内层参数 A、B 为什么消失**、**A4 的未知键参数 X 为什么消失**、**D4 的参数为什么消失**：
   屏幕事实稳定（B3、A4、D4、E2 都只显示键名、参数不见），且与「未知 / 缺失键 ⇒ 显示键原文」一致；
   但「参数被丢弃」这一步的确切成因仍不可由客户端侧观测归属，核法：抓包（同第 3 条末段）。
5. **B4 红色作用范围的精确边界**：判读为「[B4] 与 <Alice> 白、colored 红」，但这是 854×480 原生分辨率下
   的目视判读，字号约 6 px 高。**校准**：E1 的绿色在同样分辨率下清晰可辨，说明「彩色字 vs 白字」在该分辨率下
   是可靠可分的 ⇒ 上面这条判读的可靠性提高；但「为什么红只落在第二个参数上」属于屏幕事实之外的成因，
   本文只记屏幕事实，不作断言。核法：放大截图局部，或在更大窗口 / 更高 GUI 缩放下重拍同一条。
6. **行尾空格**：A3 的「<Only>」之后是否有尾随空格、A4/A5/A6 行尾是否有空格，在原生分辨率下不可分辨。
7. **聊天区同时可见行数上限**：本批最多 7 行同屏（A 批）与 5 行（B 批），据此**不能**确定上限值。

### 4.2 〔推测〕

1. get_world_info.time 恒为 0 是「该字段未与服务器同步」，而非世界时间真是 0。
   依据：同一时刻的截图是明亮昼间光照、太阳高挂；服务端未另行读取世界时间作对照。置信度：中。
2. 「未知翻译键（含本客户端语言表里没有的键）→ 显示键原文并丢弃参数」这一形态，本次由 3 个独立样本支持：
   A4（cuberite.missing.key）、D4（chat.type.achievement，无嵌套）、B3 内层（同键，嵌套）；B1 与之屏幕一致。
   它在所有未知键上普遍成立〔推测，置信度较高，样本数 3〕。
3. actionbar 上也会走与聊天区相同的翻译渲染（C1 一条样本）。置信度：中高（仅 1 条样本）。

---

## 5. 产物清单（绝对路径）

| 文件 | 内容 |
|---|---|
| /home/david/Cuberite-LTS/specs/chat/vanilla-1.12.2-chat-text-translation-observation.md | 本文件（唯一写入仓库的文件） |
| /home/david/Cuberite-LTS/build/oracle-l10n-A.png | 批次 A 的 7 行（A1–A7） |
| /home/david/Cuberite-LTS/build/oracle-l10n-B.png | 批次 B 首次拍摄（B1–B5） |
| /home/david/Cuberite-LTS/build/oracle-l10n-B2.png | 批次 B 原样重发的复拍 |
| /home/david/Cuberite-LTS/build/oracle-l10n-C1.png | C1（actionbar） |
| /home/david/Cuberite-LTS/build/oracle-l10n-C2.png | C2（控制台 tellraw） |
| /home/david/Cuberite-LTS/build/oracle-l10n-D.png | 批次 D 的 5 行（D1–D5，本文引用的一版） |
| /home/david/Cuberite-LTS/build/oracle-l10n-D-imm.png | 批次 D 发完 0 ms 拍的对照（帧滞后，见 §1.5） |
| /home/david/Cuberite-LTS/build/oracle-l10n-E.png | 批次 E 首拍（850 ms，聊天区仍空——滞后，见 §3.7 第 3 条） |
| /home/david/Cuberite-LTS/build/oracle-l10n-E-retry.png | 批次 E 补拍（约 2 s，两行可见，对比度偏淡） |
| /home/david/Cuberite-LTS/build/oracle-l10n-E2.png | 批次 E 原样重发后静置 1.5 s 的清晰版（正文引用） |
| /home/david/Cuberite-LTS/build/oracle-l10n-lag0.png | 时序控制：发完立刻截（聊天区空） |
| /home/david/Cuberite-LTS/build/oracle-l10n-lag1.png | 时序控制：0.8 s 后截（marker 可见） |
| /home/david/Cuberite-LTS/build/oracle-l10n-t0.png | 时序控制：纯文本探针发完立刻截（该条不可见，此前 A 批可见） |
| /home/david/Cuberite-LTS/build/oracle-l10n-probe.png | 工具参数形态试拍（空聊天区） |

未使用的既有文件：build/oracle-l10n-baseline.png、build/oracle-l10n-smoke.png（主 Agent 此前拍的基线/烟测）。

---

## 附录 A：实际执行的输入原文

### A.1 批次 A（mcp__cuberite__execute_lua 的 code，逐字）

    local msgs = {
      [[{"text":"[A1] ","extra":[{"translate":"chat.type.text","with":[{"text":"Alice"},{"text":"hi"}]}]}]],
      [[{"text":"[A2] ","extra":[{"translate":"chat.type.text","with":["Alice","hi"]}]}]],
      [[{"text":"[A3] ","extra":[{"translate":"chat.type.text","with":[{"text":"Only"}]}]}]],
      [[{"text":"[A4] ","extra":[{"translate":"cuberite.missing.key","with":[{"text":"X"}]}]}]],
      [[{"text":"[A5] chat.type.text"}]],
      [[{"text":"[A6] %s has just earned the achievement %s"}]],
      [[{"translate":"chat.type.text","with":[{"text":"Root"},{"text":"direct"}]}]],
    }
    local sent = 0
    cRoot:Get():ForEachPlayer(function(p)
      for _, m in ipairs(msgs) do p:SendMessageRaw(m) end
      sent = sent + 1
    end)
    print("batch A: " .. #msgs .. " messages to " .. sent .. " player(s)")

### A.2 批次 B（mcp__cuberite__execute_lua 的 code，逐字）

    local sent = 0
    cRoot:Get():ForEachPlayer(function(p)
      local m1 = cCompositeChat()
      m1:AddShowAchievementPart("Alice", "Taking Inventory")
      p:SendMessage(m1)
      local m2 = cCompositeChat()
      m2:AddTextPart("chat.type.achievement")
      p:SendMessage(m2)
      p:SendMessageRaw([[{"text":"[B3] ","extra":[{"translate":"chat.type.text","with":[{"text":"Alice"},{"translate":"chat.type.achievement","with":[{"text":"A"},{"text":"B"}]}]}]}]])
      p:SendMessageRaw([[{"text":"[B4] ","extra":[{"translate":"chat.type.text","color":"red","with":[{"text":"Alice"},{"text":"colored"}]}]}]])
      p:SendMessageRaw([[{"text":"[B5] ","extra":[{"translate":"chat.type.text","with":[{"text":"100%"},{"text":"pct"}]}]}]])
      sent = sent + 1
    end)
    print("batch B: 5 messages to " .. sent .. " player(s)")

### A.3 批次 C1（mcp__cuberite__execute_lua 的 code，逐字）

    local sent = 0
    cRoot:Get():ForEachPlayer(function(p)
      p:SendMessageRaw([[{"text":"[C1] ","extra":[{"translate":"chat.type.text","with":[{"text":"Alice"},{"text":"actionbar"}]}]}]], ctAboveActionBar)
      sent = sent + 1
    end)
    print("batch C1 to " .. sent)

### A.4 批次 C2（mcp__cuberite__run_console_command 的 command，逐字）

    tellraw rrtt217 {"translate":"chat.type.text","with":[{"text":"Alice"},{"text":"tellraw"}]}

### A.5 批次 D（追加；mcp__cuberite__execute_lua 的 code，逐字）

    local sent = 0
    cRoot:Get():ForEachPlayer(function(p)
      p:SendMessageRaw([[{"text":"[D1] ","extra":[{"translate":"chat.type.advancement.task","with":[{"text":"Alice"},{"text":"Stone Age"}]}]}]])
      p:SendMessageRaw([[{"text":"[D2] ","extra":[{"translate":"multiplayer.player.joined","with":[{"text":"Alice"}]}]}]])
      p:SendMessageRaw([[{"text":"[D3] ","extra":[{"translate":"chat.type.text","with":[{"text":"Alice"},{"translate":"chat.type.text","with":[{"text":"Bob"},{"text":"nested"}]}]}]}]])
      p:SendMessageRaw([[{"text":"[D4] ","extra":[{"translate":"chat.type.achievement","with":[{"text":"Alice"},{"text":"Taking Inventory"}]}]}]])
      p:SendMessageRaw([[{"text":"[D5] ","extra":[{"translate":"chat.type.text"}]}]])
      sent = sent + 1
    end)
    print("batch D: 5 messages to " .. sent)

### A.6 批次 E（追加；mcp__cuberite__execute_lua 的 code，逐字）

注意：本批 JSON 含 **}]]** 序列，直接用 [[ … ]] 会把 Lua 字符串截断（见 §3.7 第 1 条），
所以实际用的是 [==[ … ]==]；**JSON 内容本身与派发原文一字不差**。

    local sent = 0
    cRoot:Get():ForEachPlayer(function(p)
      p:SendMessageRaw([==[{"text":"[E1] ","extra":[{"translate":"chat.type.advancement.task","with":[{"text":"Alice"},[{"color":"green","translate":"Stone Age","hoverEvent":{"action":"show_achievement","value":"Stone Age"}}]]}]}]==])
      p:SendMessageRaw([==[{"text":"[E2] ","extra":[{"translate":"chat.type.achievement","with":[{"text":"Alice"},[{"color":"green","translate":"Taking Inventory","hoverEvent":{"action":"show_achievement","value":"Taking Inventory"}}]]}]}]==])
      sent = sent + 1
    end)
    print("batch E: 2 messages to " .. sent)

### A.7 时序控制用的纯文本探针（不是行为探针，见 §1.5）

    cRoot:Get():ForEachPlayer(function(p) p:SendMessageRaw([[{"text":"[ctl] lag-check marker"}]]); end) print("ctl sent")

---

## 附录 B：本次使用的工具白名单与未使用/被禁工具

实际调用过：mcp__cuberite__execute_lua、mcp__cuberite__run_console_command、mcp__cuberite__list_players、
mcp__cuberite__get_server_info、mcp__mcmm__ping、mcp__mcmm__get_minecraft_status、mcp__mcmm__get_player_info、
mcp__mcmm__get_world_info、mcp__mcmm__screenshot_to_file、read_image、本地 shell 的 mkdir / ls / git status（只读自查）。

未调用：全部 mcp__mcc__*（bot，不是 vanilla 客户端）、launch_minecraft / install_* / serve / kill_minecraft /
debug_fields / enter_control_mode / exit_control_mode / execute_command / click* / type_text / press_key / hotkey /
paste_text / open_chat / use_item / place_block / look_delta / set_view_angle / pause_game / close_screen。

未对客户端做任何输入；未启停服务器；未启停/重连客户端。
