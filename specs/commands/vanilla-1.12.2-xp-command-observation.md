# vanilla 1.12.2 `/xp` 命令行为观测记录

> 本文件是**观测记录**，不是实现规格。实现规格见
> [vanilla-1.12.2-xp-command.md](vanilla-1.12.2-xp-command.md)；本文件只为该规格 §7 的
> 〔待核〕条目提供**真实 1.12.2 客户端**的实证，并记录本轮观测**没能**取得的项。
>
> 铁律：本文每个数值都指向下面某一次具体工具调用 + 某张截图；没有观测到的内容不补齐。

## 0. 观测环境（固定变量与事实）

| 项 | 值 | 取证方式 |
|---|---|---|
| 观测时刻 | 2026-10-22 19:33 – 19:45 CST | `date`（bash） |
| 客户端 | Minecraft Java **1.12.2 + Forge** | `curl -s http://127.0.0.1:9876/api/status` → `{"ok":true,"type":"minecraft-mod","version":"0.4.15","loader":"forge","pid":619950,"port":9876}`；`mcp__mcmm__ping` → `{"result":"pong"}` |
| 实例路径 | `~/.local/share/FjordLauncher/instances/1.12.2/minecraft` | 任务书给定；客户端内未二次核对 |
| 渲染 | 截图 854×480；**GUI 缩放 2**（快捷栏实测宽 359 px ≈ 182×2；经验条实测宽 **362 px**（x 246–607）≈ 182×2） | `oracle-xp-c2.png` 逐像素测量（bash + PIL） |
| 世界 / 玩家 | `MpServer`（单人 integrated server，**非专用服**）；玩家 `rrtt217` | `mcp__mcmm__get_world_info` → `{"world_name":"MpServer","difficulty":"normal","gametype":"survival","time":0,"weather":"clear"}`；`mcp__mcmm__get_player_info` → `{"name":"rrtt217",...,"gamemode":"survival","food":20}` |
| 难度 / 天气 | `normal` / `clear` | 同上 |
| 客户端语言 | **简体中文（zh_CN）** —— 全部聊天文案为中文，直接影响第 2 条（单复数）的可观测性 | 全部截图 |
| 作弊 | **已开启**（所有 `/xp`、`/gamemode` 均被执行） | 见表一第 6 条 |

### 0.1 本轮用到的工具调用名（取证可复核）

只读类：`mcp__mcmm__ping`、`mcp__mcmm__get_minecraft_status`、`mcp__mcmm__get_player_info`、
`mcp__mcmm__get_world_info`、`mcp__mcmm__screenshot_to_file`、`read_image`、`bash`（仅 `curl`、
`date`、`python3`+PIL 像素测量）。

输入类（**本轮由维护者临时授权**，与仓库 `AGENTS.local.md` 记录的"输入类必崩"相反，本轮**未崩**）：
`mcp__mcmm__enter_control_mode`、`mcp__mcmm__execute_command`、`mcp__mcmm__exit_control_mode`。

> 说明：`mcp__mcmm__execute_command` 的返回值除 `"sent": true` 外还带一个指向**原版协议封包类**的
> `method` 字段。按 cleanroom 要求该字段**未抄录**进本文件，也不作为任何断言的依据。

## 1. 表一：六条待核的答案

| # | 待核项 | 观测到的行为 | 证据（工具调用 + 截图绝对路径） |
|---|---|---|---|
| 1 | `/xp <负数> @s` 接受还是拒绝？报错原文 | **拒绝**。红色错误行：`无法给玩家为负数的经验值`（12 个字形单元，逐字核对）。总经验与等级均不变。**但等级形态的负数被接受**：`/xp -1L @s` 成功，文案 `从rrtt217身上移除了1级经验`，HUD 等级 4→3 —— 负点数与负等级是**两条不同路径**，不能互相外推。 | `mcp__mcmm__execute_command("/xp -5 @s")` → `/home/david/Cuberite-LTS/build/oracle-xp-a2.png`（放大件 `build/dbg-z2-a2-neg.png`）；`"/xp -1L @s"` → `/home/david/Cuberite-LTS/build/oracle-xp-e2-negative-level.png`（等级对比 `build/dbg-e-hud-stack.png`） |
| 2 | 反馈文案逐字；点/级两形态；单数/复数 | 点数形态 `已给予rrtt217 <n>经验`；等级形态 `已给予rrtt217 <n>级经验`。姓名与数字之间**有一个空格**（间隙 10 px，明显大于字间 2 px）；数字与"经验/级经验"之间**无空格**。已观测 <n> = 0、1、30、2147483647（点数）与 0、1、3、2147483647（等级）。**该客户端语言为 zh_CN，不存在单复数语法**：n=1 与 n=30 的文案除数字本身外字形序列完全一致，无任何量词或词形变化 ⇒ 英文 en_us 下的单复数形态本次**没能观测**，见 §5 第 1 条。 | `oracle-xp-a2.png`（n=0）、`oracle-xp-a5.png`（n=1、1 级）、`oracle-xp-a6.png`（n=3 级）、`oracle-xp-b4.png`（2147483647 两形态）、`oracle-xp-c2.png`（30）、`oracle-xp-c4.png`（0 级）；逐字放大件 `build/dbg-zoom-a6-line1.png`、`build/dbg-a6-l4.png`、`build/dbg-zoom-c4-chat.png` |
| 3 | `L` 形态加等级后，等级内进度是否保留 | **保留，不清零**。对照实验（同一画面、逐像素比较经验条带）：`/xp 0` → 无等级数字、条无填充；`/xp 30` → 等级 **3**、填充 **x 246–327（82 px / 362 px ≈ 22.7%）**；`/xp 1L` → 等级 **4**、填充**逐像素不变**（同 x 246–327）；`/xp 0L` → 等级仍 4、填充不变（且**命令成功**，文案 `已给予rrtt217 0级经验`）。次要路径同样保留：`/xp -1L` 后等级 3、填充仍为同一区间。**注意**：观测只能证明"条形填充比例在加/减等级后保持不变"，无法区分内侧实现是"保留等级内原始点数"还是"保留填充比例"——两者在 HUD 上都会表现为比例不变（见 §5 第 4 条）。 | `/xp 0`→`build/oracle-xp-c1.png`；`/xp 30`→`build/oracle-xp-c2.png`；`/xp 1L`→`build/oracle-xp-c3.png`；`/xp 0L`→`build/oracle-xp-c4.png`；叠图 `build/dbg-c-stack.png`；c2↔c3 条带差异：最大通道差 103 但仅 14 px 超阈，且全部落在等级数字所占的 y 414–423 行（条带 y 420–438 逐像素相同） |
| 4 | 溢出：`/xp 2147483647`、极大 `L` 值 | **两个都被接受，都不报错**。① `/xp 2147483647 @s` → 成功文案，HUD 等级显示 **21863**，经验条仍有填充（x 246–377）；② `/xp 2147483647L @s` → 成功文案，HUD 等级变为 **0**、经验条**清空**（等级被夹回 0，无可见回绕到正数）。③ 追加一例：`/xp 2147483648 @s`（超 int 上限）→ **解析阶段即拒绝**，红色 `'2147483648'不是一个有效的数字`，等级不变（仍 3）。 | ①→`build/oracle-xp-b3.png`（等级数字放大 `build/dbg-digit-b3.png`）；②→`build/oracle-xp-b4.png`（`build/dbg-digit-b4.png` 中无任何等级数字）；③→`build/oracle-xp-e3-outofrange.png`（`build/dbg-e3-digit.png` 等级仍为 3） |
| 5 | 目标解析失败的文案 | ① 名字查无此人：`/xp 5 NoSuchPlayer` → 红色 `无法找到玩家‘NoSuchPlayer’`（**弯引号**，字形宽 2 px）。② 选择器命中非玩家实体：`/xp 5 @e[type=!player]` → **每个匹配到的实体各输出一行**红色 `无法找到玩家‘<该实体的 UUID>’`；首次执行聊天框内**可见 7 行**，重跑**可见 8 行**（行数随附近非玩家实体数变化；聊天框高度有限、更早的行会滚出，故这是**下限**），**不是**一行总括。③ 选择器**空集**形态本次**未取到**：我用 `distance=..1` 过滤，1.12.2 不认该参数，得到红色 `无效的选择器参数:'distance=..1'`（**直角单引号**，字形宽 1 px，与 ① 的弯引号字形不同）。 | ①→`build/oracle-xp-a6.png` 第 6 行（放大 `build/dbg-a6-l6.png`）；②→`build/oracle-xp-b2.png`、重跑 `build/oracle-xp-d2-selector-rerun.png`（放大 `build/dbg-d2-chat.png`）；③→`build/oracle-xp-e1-empty-selector.png`（放大 `build/dbg-e1-chat.png`） |
| 6 | 权限被拒的逐字文案 / 该世界是否开作弊 | **本世界作弊已开启**：本轮全部 `/xp` 命令都被执行并返回成功或业务错误文案（不存在的玩家、非法数字、非法选择器参数），`/gamemode` 也被执行 ⇒ 玩家权限等级 ≥ 2。**因此"权限不足"的逐字文案本次无法观测**，见 §5 第 2 条。 | 表二全部条目；`mcp__mcmm__execute_command` 每次都返回 `"sent": true`，且随后截图出现对应输出行 |

## 2. 表二：逐字抄录（命令 → 客户端聊天原文）

命令均通过 `mcp__mcmm__execute_command` 的 `command` 参数执行（含前导 `/`）；
"截图"为该命令**执行后立即**用 `mcp__mcmm__screenshot_to_file` 落盘的绝对路径。
颜色栏为客户端渲染颜色（白 = 成功/信息，红 = 错误）。

| 序 | 命令 | 客户端聊天原文（逐字，未翻译未改写） | 颜色 | 截图 |
|---|---|---|---|---|
| 1 | `/xp 0 @s` | `已给予rrtt217 0经验` | 白 | `/home/david/Cuberite-LTS/build/oracle-xp-a1.png`（该帧 HUD 尚不可读，见 §5 第 3 条） |
| 2 | `/xp -5 @s` | `无法给玩家为负数的经验值` | 红 | `/home/david/Cuberite-LTS/build/oracle-xp-a2.png` |
| 3 | `/xp 1 @s` | `已给予rrtt217 1经验` | 白 | `/home/david/Cuberite-LTS/build/oracle-xp-a3.png` |
| 4 | `/xp 1L @s` | `已给予rrtt217 1级经验` | 白 | `/home/david/Cuberite-LTS/build/oracle-xp-a4.png` |
| 5 | `/xp 3L @s` | `已给予rrtt217 3级经验` | 白 | `/home/david/Cuberite-LTS/build/oracle-xp-a5.png`、`build/oracle-xp-a6.png` |
| 6 | `/xp 5 NoSuchPlayer` | `无法找到玩家‘NoSuchPlayer’` | 红 | `/home/david/Cuberite-LTS/build/oracle-xp-a6.png` |
| 7 | `/gamemode survival` | `你的游戏模式已被更新为生存模式` | 白 | `/home/david/Cuberite-LTS/build/oracle-xp-gm-survival.png` |
| 8 | `/xp abc` | `'abc'不是一个有效的数字` | 红 | `/home/david/Cuberite-LTS/build/oracle-xp-b1.png` |
| 9 | `/xp 5 @e[type=!player]` | 每个匹配实体一行：`无法找到玩家‘<UUID>’`；本帧可见 7 行，重跑帧可见 8 行（均为下限，更早的行滚出聊天框） | 红 | `/home/david/Cuberite-LTS/build/oracle-xp-b2.png`；重跑 `build/oracle-xp-d2-selector-rerun.png` |
| 10 | `/xp 2147483647 @s` | `已给予rrtt217 2147483647经验` | 白 | `/home/david/Cuberite-LTS/build/oracle-xp-b3.png` |
| 11 | `/xp 2147483647L @s` | `已给予rrtt217 2147483647级经验` | 白 | `/home/david/Cuberite-LTS/build/oracle-xp-b4.png` |
| 12 | `/xp 0 @s` | `已给予rrtt217 0经验` | 白 | `/home/david/Cuberite-LTS/build/oracle-xp-c1.png` |
| 13 | `/xp 30 @s` | `已给予rrtt217 30经验` | 白 | `/home/david/Cuberite-LTS/build/oracle-xp-c2.png` |
| 14 | `/xp 1L @s` | `已给予rrtt217 1级经验` | 白 | `/home/david/Cuberite-LTS/build/oracle-xp-c3.png` |
| 15 | `/xp 0L @s` | `已给予rrtt217 0级经验` | 白 | `/home/david/Cuberite-LTS/build/oracle-xp-c4.png` |
| 16 | `/xp 5 @e[type=zombie,distance=..1]` | `无效的选择器参数:'distance=..1'` | 红 | `/home/david/Cuberite-LTS/build/oracle-xp-e1-empty-selector.png` |
| 17 | `/xp -1L @s` | `从rrtt217身上移除了1级经验` | 白 | `/home/david/Cuberite-LTS/build/oracle-xp-e2-negative-level.png` |
| 18 | `/xp 2147483648 @s` | `'2147483648'不是一个有效的数字` | 红 | `/home/david/Cuberite-LTS/build/oracle-xp-e3-outofrange.png` |

原文块（保持原样，供逐字对照）：

> 已给予rrtt217 0经验
> 无法给玩家为负数的经验值
> 已给予rrtt217 1经验
> 已给予rrtt217 1级经验
> 已给予rrtt217 3级经验
> 无法找到玩家‘NoSuchPlayer’
> 你的游戏模式已被更新为生存模式
> 'abc'不是一个有效的数字
> 无法找到玩家‘<UUID>’（每实体一行）
> 已给予rrtt217 2147483647经验
> 已给予rrtt217 2147483647级经验
> 已给予rrtt217 30经验
> 已给予rrtt217 0级经验
> 无效的选择器参数:'distance=..1'
> 从rrtt217身上移除了1级经验
> '2147483648'不是一个有效的数字

### 2.1 引号字形的判读（两处不同）

用字形游程（bash + PIL，统计每行的墨迹列区间）测得：

| 消息 | 引号字形宽度 | 结论 |
|---|---|---|
| `无法找到玩家‘NoSuchPlayer’`（含 `<UUID>` 变体） | **2 px** | 不是 ASCII 直角单引号；按字形抄录为弯单引号 `‘ ’` |
| `'abc'…` / `'2147483648'…` / `无效的选择器参数:'distance=..1'` | **1 px** | 按字形抄录为 ASCII 直角单引号 `'` |

两处引号**不是同一个字符**这一点由字形宽度差异直接支持；其**确切 Unicode 码位**属〔推测〕
（依据：字形宽 1 px 与 2 px 的差异，置信度高；核法见 §5 第 5 条）。

## 3. 表三：HUD 读数

读数方法：截图逐像素测量（bash + PIL）。经验条**满宽 362 px**（x 246–607，GUI 缩放 2）；
"填充"= 从条左端开始连续的高亮区间；百分比 = 填充像素 / 362。
GUI 几何来自 `build/oracle-xp-c2.png` 的实测，同一会话内未变。

| 时刻（对应命令） | 等级数字（屏幕中央绿色） | 经验条填充 | 百分比（估算） | 截图 |
|---|---|---|---|---|
| A 组执行期间（`/xp 0` … `/xp 5 NoSuchPlayer`） | **无任何等级数字** | **无经验条**（生命/饥饿条同样不渲染） | —— | `build/oracle-xp-a1.png`…`a6.png` |
| `/gamemode survival` 之后（A 组累计效果） | **4** | x 248–295（48 px） | ≈ 13% | `build/oracle-xp-gm-survival.png` |
| `/xp abc`（被拒） | 4 | x 246–295（50 px） | ≈ 14% | `build/oracle-xp-b1.png` |
| `/xp 5 @e[type=!player]`（无成功目标） | 4（HUD 区与上一帧逐像素相同） | 同上一帧 | ≈ 14% | `build/oracle-xp-b2.png` |
| `/xp 2147483647 @s` | **21863** | x 246–377（132 px） | ≈ 36% | `build/oracle-xp-b3.png` |
| `/xp 2147483647L @s` | **无等级数字（= 0）** | 无填充 | 0% | `build/oracle-xp-b4.png` |
| `/xp 0 @s` | 无等级数字（= 0） | 无填充 | 0% | `build/oracle-xp-c1.png` |
| `/xp 30 @s` | **3** | x 246–327（82 px） | ≈ 22.7% | `build/oracle-xp-c2.png` |
| `/xp 1L @s` | **4** | x 246–327（与上一帧逐像素相同） | ≈ 22.7% | `build/oracle-xp-c3.png` |
| `/xp 0L @s` | **4** | 与上一帧逐像素相同 | ≈ 22.7% | `build/oracle-xp-c4.png` |
| `/xp 5 @e[type=zombie,distance=..1]`（被拒） | 4 | 与上一帧相同 | ≈ 22.7% | `build/oracle-xp-e1-empty-selector.png` |
| `/xp -1L @s` | **3** | x 246–327（与 C 组同级帧相同） | ≈ 22.7% | `build/oracle-xp-e2-negative-level.png` |
| `/xp 2147483648 @s`（被拒） | **3**（不变） | 与上一帧相同 | ≈ 22.7% | `build/oracle-xp-e3-outofrange.png` |

补充对照（`c5`–`c8`，用于交叉验证第 3 条）：`/xp 0` → 无填充；`/xp 13L` → 等级 13；
`/xp 7` → 等级 13 且条带出现新的填充区间；`/xp 1L` → 等级 14 且**填充区间与上一帧逐像素相同**。
这一组的背景亮度已随日出改变（PNG 体积从约 190 KB 涨到约 320 KB），**未从中换算百分比**，
只取"逐像素相同"这一相对结论。

## 4. 附加观测（超出原 6 条，但对实现直接有用）

1. **负等级被接受**：`/xp -1L @s` 成功，文案 `从rrtt217身上移除了1级经验`（与"给予"是不同文案），
   等级 4→3，条内进度不变。⇒ 规格里"负数一律拒绝"的写法只对**点数形态**成立。
2. **`0L` 被接受**：`/xp 0L @s` 返回成功文案 `已给予rrtt217 0级经验`，等级与进度均不变
   （不是"无效数量"类错误）。
3. **超 int 上限的值在解析阶段被拒**：`/xp 2147483648 @s` → `'2147483648'不是一个有效的数字`，
   与 `/xp abc` 的文案同一条（同一 key），不是"数字过大"式的单独文案。
4. **1.12.2 不认选择器的 `distance=` 参数**：`@e[type=zombie,distance=..1]` → `无效的选择器参数:'distance=..1'`。
   ⇒ 规格/测试里不要用 `distance`（那是更高版本才有的参数）。
5. **mod 工具的 gamemode 字段不能当玩家实际模式用**：全程 `mcp__mcmm__get_player_info` 报
   `"gamemode":"survival"`、`mcp__mcmm__get_world_info` 报 `"gametype":"survival"`，但本轮开始时
   客户端 HUD **没有生命/饥饿/经验条**（创造模式 HUD），且 `/gamemode survival` 返回
   `你的游戏模式已被更新为生存模式`，证明切换前**不是**生存模式。两条独立证据（HUD 缺失 + 切换成功文案）
   一致，故判定"开始时玩家处于创造模式"。
6. **死亡画面仍然绘制 HUD 并随网络包刷新**：B/C 组的所有 HUD 读数都是在死亡画面（`重生`/`标题画面`）
   叠加下读的（红色叠层把绿色经验条染成金色，不影响逐像素比较）。`/xp` 在死亡状态下**照样能执行**，
   等级数字会随命令变化（b3→b4 由 21863 变为无数字）。

## 5. 没能观测到的 / 受限项

1. **【待核】en_us 下的单复数形态**。本次客户端语言是简体中文，中文没有单复数语法，
   因此"1 点 vs 7 点、1 级 vs 3 级是否用不同文案"在 zh_CN 里**结构上不可观测**。
   —— 怎么核：把客户端语言切到 English (US) 后重复 `/xp 1`、`/xp 7`、`/xp 1L`、`/xp 3L`，
   逐字抄录聊天行；或对同一命令用英文客户端与中文客户端各跑一次对照。
2. **【待核】权限被拒的逐字文案**。本世界作弊已开启，没法触发"权限不足"。
   —— 怎么核：在 **cheats 关闭**的单人世界（或把玩家权限等级降到 0 的专用服）执行 `/xp 1 @s`，
   抄录红色错误行。
3. **【待核】选择器命中 0 个实体时的文案**。本次用的 `distance=..1` 在 1.12.2 非法（见 §4 第 4 条），
   没拿到"空集"样例；只拿到"命中 N 个非玩家实体 ⇒ N 行 `无法找到玩家‘<UUID>’`"。
   —— 怎么核：用 1.12.2 合法参数构造空集，例如 `/xp 5 @e[type=zombie,r=1]`
   （玩家周围 1 格内无僵尸时），抄录输出行数。
4. **【待核】"加等级保留进度"的内部表示**。HUD 只能证明"条形填充比例在加/减等级后逐像素不变"；
   无法区分内侧是保留"等级内原始点数"还是"填充比例"——若保留原始点数而等级容量变化，
   比例本应随之改变，但实测比例分毫未变，故更倾向于"比例被原样保留"〔推测，置信度中高〕。
   —— 怎么核：在同一等级下用不同点数测填充百分比，反推该等级的容量（例如 `/xp 0` → `/xp 13L` →
   `/xp 7`，读百分比并与 `/xp 0` → `/xp 7` 对比），再对同一等级做 `L` 加级后比对；
   本次尝试过一次但因画面亮度突变（日出）导致阈值失效，未完成。
5. **【待核】引号的确切 Unicode 码位**。字形宽度 1 px / 2 px 的差异已由像素游程证实（§2.1），
   但把"1 px = ASCII `'`、2 px = `‘ `"落到具体码位属〔推测〕。
   —— 怎么核：把同一条错误消息用 `/tellraw` 复现并导出为文本，或对照客户端语言资源中该 key 的原文
   （属于资源文件，引入仓库前需按 AGENTS.md §2.1 评估）。
6. **【待核】多目标成功时的输出粒度**。本世界只有一名玩家，无法观测"一条命令命中多个**玩家**时
   是每人一行还是合并一行"。已观测到的是**非玩家目标**每实体一行错误。
   —— 怎么核：专用服拉两名玩家，执行 `/xp 5 @a`，看聊天行数。
7. **受限：A 组执行期间 HUD 完全不可读**（当时玩家处于创造模式，客户端不渲染生命/饥饿/经验条），
   所以 A 组只有聊天文案，没有逐条 HUD 读数；A 组的累计 HUD 效果是在
   `/gamemode survival` 之后一次性读到的（等级 4、条 13%）。

## 6. 本轮对客户端状态造成的改动（如实记录）

为了让 HUD 可读并取得第 3、4 条的实证，本轮通过 `mcp__mcmm__execute_command` **改动**了世界状态：

- `/gamemode survival` —— 把玩家从**创造**切到**生存**（这是本轮唯一改动固定变量的操作；
  切到生存后玩家在数十秒内死亡，死亡原因本轮**未观测**，只看到死亡画面）。
- 若干 `/xp` 命令。观测结束时玩家**等级 14**（最后四条是 §3 末尾那组交叉验证：`/xp 0` → `/xp 13L` → `/xp 7` → `/xp 1L`）。
- 未使用 `mcp__mcmm__set_gamemode` / `click*` / `type_text` / `open_chat` 等其它输入类工具；
  观测结束时已调用 `mcp__mcmm__exit_control_mode` 把控制权交还。

## 7. 交付复核（实现侧，2026-10-22）

复核人：实现侧 dev agent（非观测方）。抽检了三张关键截图并逐项比对正文，结论：**表一/表二/表三与截图一致**，
未发现抄录错位。以下两处需要更正/补充：

1. **更正 §6 的「死亡原因本轮未观测」**：死因在本文档自己的截图里就能读到——`build/oracle-xp-c3.png`
   与 `build/oracle-xp-b2.png` 的死亡画面副行是 `rrtt217被爬行者炸死了`。故应记为本轮**可读**事实
   （爬行者爆炸致死），不是受限项。`§5` 的受限清单不受影响。
2. **抽检记录**（供日后复核同样三步）：
   - `build/oracle-xp-a2.png`：聊天三行 `已给予rrtt217 0经验` ×2 + 红色 `无法给玩家为负数的经验值`，
     且该帧**无生命/饥饿/经验条**（与 §5 第 7 条的创造模式判断一致）。
   - `build/oracle-xp-c3.png`：绿色等级数字 **4**，经验条明显非空（含填充段），与表三「等级 4 / ≈22.7%」一致。
   - `build/oracle-xp-b2.png`：约 9 行红色 `无法找到玩家‘<UUID>’`，与「每实体一行」的结论一致；
     引号为弯引号，与 §2.1 的判读一致。

以下实现侧落差**不在本文件范围内**，只做登记（规格与代码的改动待维护者下达）：
`/xp` 负点数被本实现接受（vanilla 拒绝）、负等级缺"移除"专属文案、极大 `L` 值本实现失败而 vanilla
夹回 0 并报成功、`@e` 命中非玩家时本实现只报一行（vanilla 每实体一行）。

## 8. en_us 补充取证（2026-10-22 20:06，实现侧复核时补做）

同一个 `minecraft-mod-mcp` mod，换成 `~/.minecraft` 这个**英文（en_us）**实例（Forge 14.23.5.2864 /
Minecraft 1.12.2，`mods/` 里只有本 mod，日志 `identified 5 mods`）⇒ §5 第 1 条「en_us 下单复数不可观测」
的缺口现在补上了。

**取证方式**：`mcp__mcmm__enter_control_mode` → 逐条 `mcp__mcmm__execute_command`（间隔约 0.35 s）→
`mcp__mcmm__screenshot_to_file` 后 `read_image` 读字。截图（绝对路径）：
`/home/david/Cuberite-LTS/build/drive-xp-visible.png`（点数形态，等级 1）、
`/home/david/Cuberite-LTS/build/drive-xp-forms.png`（负点数 / 1 级 / 3 级 / -1 级，四行同屏，等级 4）。

| 命令 | 客户端聊天原文（en_us，逐字） | 颜色 |
|---|---|---|
| `/xp 5 @s` | `Given 5 experience to rrtt217` | 白 |
| `/xp -5 @s` | `Cannot give player negative experience points` | 红 |
| `/xp 1L @s` | `Given 1 levels to rrtt217` | 白 |
| `/xp 3L @s` | `Given 3 levels to rrtt217` | 白 |
| `/xp -1L @s` | `Taken 1 levels from rrtt217` | 白 |

**结论（修正 §1 表一第 2 条与 §5 第 1 条）**：

1. 等级形态**没有单数形式**：1 级也写 `1 levels`（恒用复数 `levels`），与「1 用 level」的猜测相反。
2. 负等级是**独立句式** `Taken <n> levels from <player>`（数量取绝对值），不是 `Given -1 levels …`。
3. 负点数的英文文案是 `Cannot give player negative experience points`，与 zh_CN 的
   `无法给玩家为负数的经验值` 是同一 key 的两种语言渲染。
4. 点数形态 `Given <n> experience to <player>` 与本仓库 `cmd_xp.lua` 现有模板**一致**；
   等级形态（恒复数、负数用 `Taken … from …`）与本实现**不一致**。

**为什么这里能拿到、而 §1 拿不到**：vanilla 发的是可翻译文本组件（key + 参数），客户端按自己的语言表渲染；
上一台客户端是 zh_CN，所以只观测到中文。**推论（对实现有意义）**：Cuberite 侧发的是**固定英文字符串**，
因此在 zh_CN 客户端上永远显示英文——这是比措辞更根本的已知偏差，与 §7 末尾的落差清单同批处理。

**仍未核**：多**玩家**目标的输出粒度（本实例仍只有一名玩家）；选择器空集文案（`distance=` 在 1.12.2 非法，
需改用 `r=` 构造）；权限拒绝文案（本世界作弊已开）。
