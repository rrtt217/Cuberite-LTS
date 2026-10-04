# Vanilla 1.12.2 末影龙行为规格（The End / Ender Dragon）

> 本文件是 `feature/mobs-ender-dragon-parts` 的**功能交付规格**（行为规格 + 来源 + 验证方式），
> 依 AGENTS.md §2.4。它不是仓库级的 parity ledger；缺口编号仍由维护者逐条下达。
> 基准：vanilla Java Edition **1.12.2**；本文件只引用 AGENTS.md §2.2 白名单来源。
> Minecraft Wiki 当前描述最新版，凡 1.12.2 之后才有的内容均已显式标注。

## 0. 关联 issue

- [cuberite/cuberite#4906](https://github.com/cuberite/cuberite/issues/4906) — Ender Dragon Fight/Behaviour（本分支针对其中 “player can't damage them” 一项）
- [cuberite/cuberite#4907](https://github.com/cuberite/cuberite/issues/4907) — The End MEGA issue

## 1. 战斗状态机

### 1.1 `DragonPhase`（NBT 整数，15w31a / 1.9 引入）

来源：[Minecraft Wiki — Ender Dragon § Entity data](https://minecraft.wiki/w/Ender_Dragon)

| phase | 含义 |
|---|---|
| 0 | circling（盘旋/守御） |
| 1 | strafing（准备喷火球） |
| 2 | flying to portal to land（飞向出口传送门准备落地） |
| 3 | landing on portal（正在落地） |
| 4 | taking off from portal（起飞） |
| 5 | landed，执行龙息攻击 |
| 6 | landed，寻找可攻击的玩家 |
| 7 | landed，龙息前的咆哮 |
| 8 | charging player（冲向玩家） |
| 9 | flying to portal to die（致命一击后飞向传送门） |
| 10 | hovering（原地扇翅徘徊；`/summon` 默认态） |

> **版本注**：0–9 在 1.12.2 已存在。phase 10 是否存在于 1.12.2 未确认（当前 Wiki 描述最新版；
> 26.1 起战斗数据已改由数据包 `ender_dragon_fight` 管理，与 1.12.2 无关）。

### 1.2 行为组与迁移触发

来源同上（§ Behavior / § Attacking）。

- **Guarding / 盘旋（phase 0）**：初始态。仍有水晶时绕黑曜石柱环外侧飞，水晶清空后改内侧。
  每摧毁一颗水晶 → 龙受伤，且切换状态几率增大。
  - **Targeting**：与当前目标距离 `<10` 或 `>150` 格时重选目标；受伤时把目标设在自身后方。
- **Strafing（phase 1）**：某颗末影水晶被摧毁时切入。进入 64 格内即发射龙火球；结束后回盘旋。
- **Perching / 落地**：盘旋到路径末端时，以 `1 / (3 + 存活水晶数)` 几率前往出口传送门落地
  （约 7.7%–33%）。落地点为 (X=0,Z=0) 最高方块（直至 Y=101）；(0,0) 无方块时 1.12.2 降到 Y=0。
  - 落地 **1.25s** 后，若玩家在出口传送门 20 格内 → 咆哮并施放 **3 秒龙息**（类似滞留型伤害药水；和平难度不触发）。
  - **Charge**：落地 5 秒后玩家仍不在传送门附近 → 冲向 150 格内最近玩家（仅 BE；JE 无视线要求）。
  - **Take-off**：连续 4 次龙息、或 150 格内找不到玩家 → 起飞回盘旋；和平难度总是起飞。
  - **Escape**：落地累计受伤 `>= 50`（25% 最大生命）→ 起飞并清零累计。对应 NBT `sitting_damage_recieved`。
  - 落地态免疫箭矢/三叉戟（会被弹开并着火）。
- **Death**：致命一击后除非 150 格内找不到出口传送门或身处方块内，否则先飞向传送门再死（phase 9）。

### 1.3 数值与结算

| 项 | 值 | 来源 |
|---|---|---|
| 生命 | 200 | 信息框 |
| 包围盒 | 宽 16 × 高 8 | 信息框 |
| 伤害减免 | 除头部外 `原伤/4 + min(1, 原伤)`（等效约 800 生命） | Behavior |
| 受伤来源 | 仅爆炸与玩家（含命令）；免疫火/摔/溺水/中毒/闪电/虚空/状态效果（玩家投掷的瞬间伤害除外） | Behavior |
| 翅膀/头部攻击 | 翅膀 5（简单 3.5/困难 7.5），头部 10 | 信息框 |
| 水晶治疗 | 32 格长方体内最近水晶每 0.5s 回 1 HP | End Crystal § Healing |
| 摧毁供能水晶 | 龙受 10 伤害 | End Crystal § Explosions |
| 死亡 | `DragonDeathTime` 150 起每 5 tick 掉经验（首掉 155），>=200 移除；首杀 12000 XP，重召 500 XP | Entity data / Death and drops |
| 重召 | 出口传送门四边各一颗水晶 → 604 tick（30.2s）流程 | Re-summoning |

## 2. 部件与实体 ID

来源：[Minecraft Wiki — Ender Dragon](https://minecraft.wiki/w/Ender_Dragon)（§ Behavior）、
[Mojira MC-146503](https://mojira.dev/MC-146503)、[MC-274526](https://mojira.dev/MC-274526)、
[MC-158205](https://mojira.dev/MC-158205)。

### 2.1 结构

- 一条龙在服务端由 **9 个实体**组成：1 个主实体 + **8 个非 mob 部件实体**
  （尾 ×3、身体、头、脖子、左翼、右翼），因为部件是实体，`/kill` 会报告 “Killed 9 entities”。
- 8 个部件是 F3+B 下的 8 个绿色可受伤子碰撞盒；头部部件是唯一不触发 ~75% 减免的部位。
- 命名牌无法作用于龙：玩家只能与部件（非活体）交互。

### 2.2 客户端如何知道部件 ID

- 服务端**只为龙本体发 Spawn 包**（1.12.2 Spawn Mob，实体类型 63），**不发送部件生成包**。
- 客户端收到本体生成后**自行构造 8 个部件实体**，其实体 ID 由**龙本体的实体 ID 派生**：
  `part[k] = dragonID + 1 + k`（k = 0..7），即连续块 `dragonID+1 .. dragonID+8`。
- 玩家左键点击部件时，客户端发送 **Use Entity（1.12.2 服务端 Play 0x0A，Type=1=ATTACK）**，
  携带的是**客户端派生的部件实体 ID**；服务端必须把该 ID 映射回龙本体。
- **历史 bug**：19w08a（1.14）起客户端误把第一个部件赋成 `dragonID`（整体 off-by-one），
  造成客户端/服务端命中错位（MC-274526 → MC-158205）。**1.12.2 早于该 bug，故使用 `dragonID+1+k`。**

> **未确认项（本分支最大的风险）**：`dragonID+1+k` 的基准公式与 8 个部件的**顺序**由 Mojira 的
> off-by-one 报告反推，未获得 1.12.2 合法客户端/抓包的直接佐证。落地实现把该假设集中在
> `cEnderDragon::PART_COUNT` 与 `cEnderDragon::IsPartID()` 一处，便于后续用 Tools/ProtoProxy
> 或合法客户端实测校正。**头部部件在 8 个 ID 中的位置同样未确认**，因此本分支暂不实现“仅头部全额伤害”。

## 3. 本分支增量范围（最小改动）

目标：修复 #4906 的 “player can't damage them”。

1. 龙构造时**原子预留** `dragonID .. dragonID+8` 共 9 个连续实体 ID，并把本体 ID 设为块首，
   使客户端派生的部件 ID 不会被其它服务端实体占用。
2. `cEnderDragon::IsPartID(id)` 判断某 ID 是否落在该龙的部件块内。
3. 左键攻击且按实体 ID 查不到实体时，在玩家附近搜索末影龙：若目标 ID 属于某条龙的部件块，
   则把这次命中转交该龙（`cEnderDragon::TakeDamageFromPart`）。
4. `TakeDamageFromPart` 施加 vanilla 的 `原伤/4 + min(1, 原伤)` 减免。

**故意不做的部分**（后续增量）：

- 头部部件全额伤害（部件顺序未确认）。
- 8 个部件作为真实服务端实体（`/kill` 计 9、爆炸对部件的结算等）。
- 真实飞行 / 状态机 `DragonPhase` / 水晶治疗 / 重生流程。

## 4. 验证方式

- 单元测试：预留的 ID 块与龙自身的 ID 连续、且不被后续实体占用；`IsPartID` 边界。
- 静态：`src && lua CheckBasicStyle.lua`。
- 编译 + `ctest`（排除两个联网用例）。
- 实机对照：本环境无 vanilla 服务端/客户端作 oracle，部件 ID 公式与头部顺序留待 ProtoProxy 校正。

### 已知偏差

1. 所有部件（含头部）都按非头部公式减免 → 打头也拿不到全额伤害。
2. 部件不是服务端实体 → `/kill` 仍计 1、部件不参与爆炸/碰撞结算。
3. 1.8 客户端是否使用同一部件 ID 公式未验证（本分支按 1.12.2）。
