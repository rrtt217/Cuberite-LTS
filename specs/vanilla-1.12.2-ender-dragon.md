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

> **验证状态**：`dragonID+1 .. dragonID+8` 基准范围与**头部索引**均由**合法客户端实机验证**
> （维护者实测：本分支落地后末影龙可被近战伤害；正确瞄准头部时客户端回传 `partID = dragonID + 1`，
> 即 **头部 = 部件索引 0**，落在 `cEnderDragon::HEAD_PART_INDEX`）。同一轮测试中索引 1 对应**颈部**
> （一度被误判为头部），可佐证顺序以 `head(0), neck(1)` 开头；其余部件顺序仍未逐一确认，
> 但头部（唯一享受全额伤害的部件）已足以实现伤害例外。

## 3. 增量范围

### 3.1 部件命中（feature/mobs-ender-dragon-parts）

目标：修复 #4906 的 “player can't damage them”。

1. 龙构造时**原子预留** `dragonID .. dragonID+8` 共 9 个连续实体 ID，并把本体 ID 设为块首，
   使客户端派生的部件 ID 不会被其它服务端实体占用。
2. `cEnderDragon::IsPartID(id)` 判断某 ID 是否落在该龙的部件块内。
3. 左键攻击且按实体 ID 查不到实体时，在玩家附近搜索末影龙：若目标 ID 属于某条龙的部件块，
   则把这次命中转交该龙（`cEnderDragon::TakeDamageFromPart`）。
4. `TakeDamageFromPart` 对非头部部件施加 vanilla 的 `原伤/4 + min(1, 原伤)` 减免，头部（索引 0）全额伤害。

### 3.2 DragonPhase 状态机基础（feature/mobs-ender-dragon-phases，叠在 3.1 之上）

目标：让龙能飞、能悬停，并建立可扩展的阶段字段与客户端同步。

1. 新增 `cEnderDragon::eDragonPhase`（0–10，对应 vanilla `DragonPhase`）、`m_DragonPhase`、
   `GetDragonPhase()`、`SetDragonPhase()`（变化时广播 entity metadata）。
2. 构造时 `SetGravity(0); SetAirDrag(0);`；重写 `HandlePhysics` 做穿墙自由飞行（不在方块上停下）。
3. 重写 `Tick`：`Circling`（绕世界中心水平盘旋）与 `Hovering`（原地悬停、清零速度）。
4. `cMonster::Tick` 的寻路对末影龙禁用（与 Ghast 同样处理）。
5. 协议 1.9–1.12.2：`WriteMobMetadata` 写 `ENDER_DRAGON_DRAGON_PHASE`（1.10–1.12 用各自的 `Metadata` 常量；1.9 用其 writer 的裸索引 11）。
6. `Circling` 按 wiki 区分柱环内侧/外侧：竞技场仍有末影水晶时绕外侧（半径 48），水晶清空后绕内侧（半径 20）；
   每 20 tick 统计一次世界中心 ±64 格内的 `cEnderCrystal` 数量。
7. `Strafing`（phase 1）：水晶数量减少（检测到被摧毁）且**有目标**时进入，持续 60 tick，朝目标头部直线飞行，
   随后回到 Circling/Hovering。Vanilla 在进入 64 格内时喷龙火球——该投射物本分支未实现（见偏差 8）。
8. `Perching` 序列（phase 2 → 6 → 7 → 5 → 4）：每完成一圈按 wiki 的 `1/(3+水晶数)` 掷骰决定去出口传送门上方 (0, 70, 0)：
   - 到达后 `LandedSearching`(6) 搜索 25 tick（1.25s）：20 格内有玩家 → `LandedRoar`(7)；150 格内无玩家 → 起飞；
   - `LandedRoar`(7) 10 tick → `LandedBreath`(5) 60 tick（3s；伤害云未实现，只计时）；
   - 连续 4 次龙息或 150 格内无玩家 → `TakingOff`(4) 20 tick → `Circling`；
   - 落着期间累计受伤 > 50 立即起飞并清零（`m_PerchDamageTaken`）；免疫箭矢/投掷三叉戟（`dtRangedAttack`）。
9. `Dying`（phase 9）：致命伤由 `KilledBy` 拦截——保持 1 血、进入 `Dying` 并飞向出口传送门；到达后调用基类 `KilledBy` 真正死亡，
   若有玩家参与则掉落 12000 XP（对应 wiki 的 “takes a fatal blow → flies toward the exit portal before dying”）。
   到达传送门只结算一次。死亡后由龙的 `m_DragonDeathTime` 计时（对应 vanilla NBT `DragonDeathTime`，仅服务端、不作为 metadata 下发）：
   150 tick 掉落 12000 XP，200 tick（10 秒）时 `Destroy()`；`Tick` 在血量 `<= 0` 时只跑该计时，
   不调用 `cMonster::Tick`（它 1 秒就会删龙，会截断死亡动画）。
10. 阶段语义：`Hovering`(10) 是 `/summon` 龙的默认且**终点**态——原地悬停、无害、不进入 fight 逻辑（不统计水晶、不接触伤害、不切换）；
    fight 生成的龙在 `TickEnderDragonFight` 里被显式设为 `Circling`(0)，且所有转换（Strafing/Perching/Dying）只回落到 `Circling`，**从不进入 10**。
    删除了早期的“有目标→盘旋/否则悬停”占位切换。`DragonPhase` 随实体一起存/读档（NBT int `DragonPhase`）。
11. 受击与免疫：龙只受**玩家造成的伤害**与**爆炸伤害**（`dtExplosion`），其余一律在 `DoTakeDamage` 返回 false；
    不会被击退（覆写 `cEntity::CanBeKnockedBack()`），也不会被玩家近战暴击（覆写 `cEntity::CanBeCriticalHit()`）。
12. 朝向：`FaceSpeedDirection()` 用引擎标准的 `VectorToEuler()`（与 `cMonster::SetPitchAndYawFromDestination` 一致），
    而非 `cEntity::SetYawFromSpeed()`——后者用 `atan2(speed.x, speed.z)`，在 X 上与标准朝向镜像，是“倒着飞”的根因。
13. Boss 栏颜色为 **Pink**（[Bossbar](https://minecraft.wiki/w/Bossbar)：the ender dragon has a pink bossbar；紫色是凋灵）。
14. 末地水晶治疗（[Ender Crystal](https://minecraft.wiki/w/Ender_Crystal)）：水晶每 tick 找 32 格内最近的龙，
    每 10 tick `Heal(1)`，并用 `BeamTarget` metadata 画出白色光柱；水晶被摧毁时若正在治疗龙，则对龙造成 10 点 `dtExplosion` 伤害。

**故意不做的部分**（后续增量）：

- 8 个部件作为真实服务端实体（`/kill` 计 9、爆炸对部件的结算等）。
- Strafing 的龙火球、Perching 的咆哮/龙息伤害、Charge、死亡后的传送门/龙蛋/gateway、持久化 fight 状态、水晶治疗、重生流程。

### 3.3 接触（近战）伤害（feature/mobs-ender-dragon-phases）

来源：[Ender Dragon § Attacking](https://minecraft.wiki/w/Ender_Dragon) / 信息框。

1. 停用通用怪物近战（`cEnderDragon::Attack` 返回 false），改在 `Tick` 里做接触判定。
2. 每 tick 取与龙包围盒（16×8）相交的 `cPawn`；用“前方半宽、半高处”的近似头点（半径 3 格）区分头部/翅膀。
3. 伤害取 wiki 的 **Normal** 值：头部 10、翅膀 5（Cuberite 无难度系统，恒为 Normal）。
4. 命中走 `TakeDamage(dtMobAttack, ...)`，基础伤害逻辑会把目标抛起（玩家的 `KnockbackHeight = 8`）。
5. 受击后 0.5s（10 tick）内不施加接触伤害（复用 `cMonster::m_TicksSinceLastDamaged`）。
6. `Circling` 调用 `SetYawFromSpeed()`，让朝向跟随飞行方向（头部判定与客户端朝向依赖它）。

### 3.4 首次进末地自动生成龙（feature/mobs-ender-dragon-spawn，叠在 3.3 之上）

来源：[Ender Dragon § Spawning](https://minecraft.wiki/w/Ender_Dragon)（“spawns 20 game ticks (1 second) after an entity first arrives in the End”）。

1. `cWorld::Tick` 在 `dimEnd` 世界里调用 `TickEnderDragonFight`。
2. 世界里已有玩家且本场尚未开始时，倒计时 1 秒（20 tick）；到点后在 `(0.5, 最高方块 + 20, 0.5)` `SpawnMob(mtEnderDragon)`。
3. `m_HasSpawnedEnderDragon` 保证一场只生成一次；若世界里已有龙（例如从存档加载回来的）则不再生成第二条。
   fight 状态（`HasSpawned` / `DragonKilled`）持久化到 `world.ini` 的 `[EnderDragon]` 段，重启后不会重新生成。

## 4. 验证方式

- 静态：`src && lua CheckBasicStyle.lua`（通过，0 违规）。
- 编译 + `ctest`（排除两个联网用例；通过，26/26）。
- 实机对照：**已由合法客户端实测通过**——末影龙可被近战伤害，部件命中转交生效，
  `dragonID+1 .. dragonID+8` 范围得到验证；正确瞄准头部回传 `partID = dragonID + 1`，确定头部索引为 0（索引 1 为颈部）。
- **未做自动化单测**：现有测试框架只编译孤立源文件，无法链接实体引擎做有意义的 `cEnderDragon` 测试。
- 其余 7 个部件的顺序未逐一确认（不影响头部全额伤害；如需可加临时日志或抓包逐部件核对）。
- 状态机分支：`lua CheckBasicStyle.lua` 0 违规；`cmake --build build` exit 0；`ctest` 26/26。飞行/悬停观感需实机确认。

### 已知偏差

1. 部件不是服务端实体 → `/kill` 仍计 1、部件不参与爆炸/碰撞结算。
2. 非头部减免施加在**基础伤害**上；附魔（Sharpness 等）/暴击加成由 `cEntity::DoTakeDamage` 之后再加，未被减免
   （vanilla 减免的是最终值）。无附魔的普通攻击不受影响；修正需要给 `cEntity::DoTakeDamage` 增加一个伤害后处理钩子，属独立改动。
3. 1.8 客户端是否使用同一部件 ID 公式与头部索引未验证（本分支按 1.12.2）。
4. 尚无在运行时设置 `DragonPhase` 的入口（Core 的 `/summon` 不支持 NBT，也没有 `/data`）；只能通过 NBT 存读档保留，或由 fight 逻辑设置。
5. 盘旋参数（外侧半径 48 / 内侧 20、高度 80、速度 8 格/秒、每 tick 5% 修正）是便于测试的初值，未经 vanilla 实测校准；
   柱子环本身按生成器默认半径 43 生成，两者都是可配置量。
6. 接触伤害的头部/翅膀用**几何近似**（头点 = 前方半宽、半径 3 格），不是真实部件；抛起强度沿用基础击退，未按 vanilla 校准。
7. Cuberite 没有难度设置，接触伤害恒取 **Normal**（头 10 / 翅膀 5）；vanilla 会按难度取 6/10/15 与 3.5/5/7.5。
8. `Strafing` 只是朝目标飞行 3 秒后回绕，未实现 vanilla 的“进入 64 格即喷龙火球”；水晶被毁通过每 20 tick 的数量对比检测（最多 1 秒延迟）。
9. `Perching` 的落点高度（70）是近似；`LandedBreath` 的 3 秒只有计时，没有龙息伤害云（AreaEffectCloud 未实现）；咆哮没有独立的声音/粒子。
10. `Charging`（仅 BE）尚未实现（JE 本就没有该状态）。
11. fight 状态用 `world.ini` 的 `[EnderDragon]` 段（`HasSpawned`/`DragonKilled`）持久化，只是两个布尔；未实现 vanilla 的 `PreviouslyKilled`、`ExitPortalLocation`、`Gateways`、`respawn_crystals`/重召唤等完整状态。
12. 没有完整的 fight 控制器（水晶被毁的概率切换、传送门/龙蛋/gateway 记录等），生成位置取 (0,0) 最高方块 + 20。
13. 死亡序列只到“200 tick 后删龙 + 150 tick 一次性掉 12000 XP”；升天动画由客户端据死亡自行渲染（服务端只保证 200 tick 生命周期），
    出口传送门激活、龙蛋、End gateway 未实现；XP 未按 vanilla 每 5 tick 分批掉落。
15. 未改动全局 `cEntity::SetYawFromSpeed()`（投射物仍在用）；若投射物朝向也有镜像问题，属另一改动。
16. 水晶治疗用 32 格球形判定（vanilla 是 AABB 扩张），未区分方块阻挡（本就纯距离，wiki 也说明可穿方块）；摧毁治疗水晶的 10 点伤害可能与水晶自身爆炸伤害叠加。
17. `dtPlugin`/`dtAdmin` 对龙的伤害也被免疫（严格按“只受玩家与爆炸”）；插件若需强改血量应直接 `SetHealth`。
14. 致命伤时攻击者会被提前记入 `Killed` 统计（基类 `DoTakeDamage` 在 `KilledBy` 之后无条件调用），此时龙尚未真正死亡。
