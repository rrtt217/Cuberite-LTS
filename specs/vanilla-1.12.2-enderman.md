# Vanilla 1.12.2 — 末影人（Enderman）

来源（cleanroom 白名单）：

- [Minecraft Wiki（中文）— 末影人](https://zh.minecraft.wiki/w/%E6%9C%AB%E5%BD%B1%E4%BA%BA)（行为 / 激怒 / 瞬移 / 移动方块各节 + Java 版历史表。**本规格的传送与速度数值以中文 wiki 为主源**——其「瞬移」节含英文 wiki 缺失的凝视规则、尝试次数规则与天光概率公式）
- [Minecraft Wiki（中文）— 属性/速度](https://zh.minecraft.wiki/w/%E5%B1%9E%E6%80%A7/%E9%80%9F%E5%BA%A6)（末影人速度属性 0.3、有攻击目标时 +0.15 修饰符）
- [Minecraft Wiki — Enderman](https://minecraft.wiki/w/Enderman)（Behavior / Teleportation / Moving blocks 各节；含 [Java Edition 历史表](https://minecraft.wiki/w/Enderman#Java_Edition)，用于把「现行描述」裁剪回 1.12.2）
- [Minecraft Wiki — Endermite — Behavior](https://minecraft.wiki/w/Endermite#Behavior)
- [Minecraft Wiki — Mob spawning](https://minecraft.wiki/w/Mob_spawning)（怪物分类刷怪权重）
- 上游缺陷追踪：[cuberite#4986](https://github.com/cuberite/cuberite/issues/4986)（行为清单）、[#2552](https://github.com/cuberite/cuberite/issues/2552)（受击不传送）、[PR #4982](https://github.com/cuberite/cuberite/pull/4982)（恢复刷怪被关闭，理由「需要先有可用的末影人 AI」）
- wiki.vg 不涉及本特性（协议侧 metadata 已在仓库实现，仅复核）。

本文只写规格，不含实现细节（Cuberite 落点除外）。凡 1.12.2 适用性无法从允许来源直接确认的数值，标 **〔待核〕**；无来源只能合理的，标 **〔推测〕**。

---

## 0. 当前 Cuberite 实现状态（基线 `master` @ `2f054c3`）

已有（**不要重复实现**）：

| 项 | 位置 |
|---|---|
| `cEnderman`（0.6×2.9，音效组，40 血，SightDistance 64，AttackDamage 4，AttackRange 1，AttackRate 1） | `src/Mobs/Enderman.{h,cpp}`、`Server/monsters.ini` |
| 凝视激怒（64 格、视线 `losAirWater`、南瓜头、`CanMobsTarget`） | `Enderman.cpp: cPlayerLookCheck` |
| 中立怪受击转敌对（环境伤害与创造不激怒） | `src/Mobs/Monster.cpp: DoTakeDamage`、`PassiveAggressiveMonster.cpp` |
| 敌视末影螨（64 格，仅按距离） | `Enderman.cpp: Tick` + `AggressiveMonster.cpp: GetMonsterOfTypeInSight` |
| 水/雨伤害（每 tick 尝试 1 点，受 `m_InvulnerableTicks` 节流） | `Enderman.cpp: Tick` |
| 掉落 0–1 珍珠（+抢夺） | `Enderman.cpp: GetDrops` |
| metadata：carried block / screaming（1.8–1.16 协议全量） | `src/Protocol/Protocol_*.cpp` |
| NBT **写入** `carried`/`carriedData` | `src/WorldStorage/NBTChunkSerializer.cpp` |
| 刷怪落点检查（≥3 格空气、光照 ≤7、y<250） | `src/MobSpawner.cpp` |
| 传送落点搜索原型 `cPawn::FindTeleportDestination`（紫颂果在用） | `src/Entities/Pawn.cpp` |

缺失（本规格覆盖）：

1. **传送：完全没有**（`Enderman.cpp` 内 `TODO teleport to a safe location`）——本规格 §3.6。
2. **搬方块：完全没有**（`m_CarriedBlock` 无任何写入方；`LoadEndermanFromNBT` 也不读 `carried`）——§3.7。
3. **自然刷怪被禁用**（`World.cpp` 主世界/末地默认列表剔除 enderman；`MobSpawner.cpp` 下界候选列表无 enderman）——§2。
4. 已知偏差清单见 §6。

---

## 1. 基础数据

| | 值 | 来源/备注 |
|---|---|---|
| 生命 | 40 | wiki infobox ✅已实现 |
| 碰撞箱 | 0.6 宽 × 2.9 高 | ✅已实现 |
| 行为 | 中立 | ✅（`PASSIVE` personality） |
| 攻击力（Normal） | 7 | wiki infobox 现行 7（Easy 4.5 / Hard 10.5 为新版标度，1.12.2 的 Easy/Hard 数值〔待核〕）；当前实现 4.0，且 Cuberite 无难度缩放（上游 #1006） |
| 移动速度 | 属性 0.3；**有攻击目标时 +0.15（=0.45）** | 中文 wiki [属性/速度](https://zh.minecraft.wiki/w/%E5%B1%9E%E6%80%A7/%E9%80%9F%E5%BA%A6)（基础值表 + "attacking" 修饰符表）。Cuberite 无属性系统（#4921），经 `monsters.ini` WalkSpeed/RunSpeed 倍率映射，见 §3.8 |
| 掉落 | 0–1 珍珠，每级抢夺 +1 | ✅已实现 |
| 经验 | 5 | 当前 6–8，偏差（§6） |
| 死亡 | 掉落携带方块（丝触等效掉落物） | 依赖 §3.7 |
| 音效 | `entity.endermen.hurt/death/ambient`（✅）、`entity.endermen.stare`（缺，§6）、`entity.endermen.teleport`（缺，§3.6） | 1.10+ 协议直接发名字；1.8/1.9 客户端收到未知名字仅静默 |

---

## 2. 自然刷怪

- **主世界**：几乎全生物系稀刷（怪物类权重 10，组 1–4；1.12.2 无蘑菇岛限制之外的例外）。光照 ≤7（**1.12.2 时代**；「光照 0」是 1.18+ 变更）。可刷表面须有 ≥3 格空气（身高 2.9）。
- **下界**：1.10 起在 nether wastes 刷（权重/组数 1.12.2〔待核〕；wiki 现行为组 4），光照 ≤7。
- **末地**：唯一自然刷怪，组 4，光照 0（末地恒暗）。
- Cuberite 落点：`World.cpp: InitializeAndLoadMobSpawningValues` 默认列表加回 enderman；`MobSpawner.cpp` 下界候选列表补 `mtEnderman`；`CanSpawnHere` 的 mtEnderman 分支已符合「≥3 格空气 + 光照 ≤7」。
- **前置**：上游因「AI 不完整体验差」禁用（#3108、PR #4982）。先落 §3.6 传送，再放开刷怪（独立分支）。

---

## 3. 行为

### 3.1 激怒（Provoking）

末影人在被激怒前保持中立。激怒条件（wiki「Provoking」）：

1. 玩家或其他生物攻击它（1.5.2 起环境伤害不激怒；1.7.2 起创造模式玩家攻击不激怒）。✅已实现。
2. **64 格内**的玩家注视它的**眼睛/头部**，且视线不被任何固体方块（含透明固体）阻挡，且玩家未戴南瓜头。
   - 注视头部任意位置（含后脑）即激怒；距离较远时注视大腿上部也算 → 规格是**视线与头部 hitbox 相交**，而非固定角度锥。
   - 激怒范围像「探测范围」，潜行/隐身会缩小（Cuberite `CanMobsTarget` 为布尔开关，无距离缩放，§6）。
   - 现行 wiki 说需注视 **5 game ticks（1/4 s）**才触发——1.12.2 是否已有此延迟〔待核〕；1.12.2 先按**当 tick 立即触发**（现行 Cuberite 即如此）。
   - 视线检查中水不阻挡（水非固体）✅（`losAirWater`）。
3. 看到 **64 格内**的末影螨（wiki「Provoking」/「Attacking」）。
   - 1.12.2 只敌视**末影珍珠生成**的末影螨（14w17a 起，1.17 才改为全部）——Cuberite 无来源标记，现攻击所有末影螨（§6）。
   - 现行 wiki 要求「视线可见」（in line of sight）；当前实现只按距离（§6）。

激怒表现：张嘴 + 持续尖叫（scream metadata ✅已实现，客户端驱动）+ 原版 Java 版发抖（客户端行为）。

### 3.2 被注视时的反应（Staring）

**1.12.2 规则（已实现）**：**被注视而激怒**的末影人，在激怒发生的那一瞬间**立即尝试瞬移离开一次**（随机传送，单次尝试）；**此后即使继续被注视也正常冲锋追击**。

依据：中文 wiki [历史表](https://zh.minecraft.wiki/w/%E6%9C%AB%E5%BD%B1%E4%BA%BA) **1.14 / 19w07a** 条目「现在末影人被玩家注视激怒后**不再会立即瞬移**，而是会先盯住玩家一段时间」（MC-71256 修复）——反推 1.12.2 = **被注视激怒的瞬间立即传送**。实机观察（1.12.2 正版服务端）：锁定后即便持续被注视也会冲锋——因此**不是**「对视期间每 tick 传送」（那是本规格前一版的误读，曾导致追击时「走一步停一步/永远走不到脸上」，已修正）。

**注意（版本裁剪）**：中文 wiki「行为」节现描述的「4–16 格僵直不动、<4 格立即瞬移」是 **1.14+** 行为（正是 19w07a 引入的「先盯住」的现行细化），**1.12.2 不适用，不实现僵直**。

注视判定复用仓库既有 `cPlayerLookCheck`（64 格、南瓜头豁免、`CanMobsTarget`、`losAirWater` 视线、5° 锥近似 hitbox——锥近似为已知偏差 §6）。

### 3.3 攻击与追击（Attacking）

- 激怒后向目标跑动追击；近战攻击。✅已实现（基类 `MoveToPosition` + `Attack`）。**追击速度见 §3.8**——被激怒的末影人比绝大多数怪快近一倍，这是它压迫感的主要来源。
- **目标在 ≥16 格远时，每 1.5–2 s 向目标传送一次**（§3.6.2）。
- 追击期间不因淋水丢失目标（wiki 历史：1.8 / 14w06a「remain aggravated despite being in contact with water」；1.12.2 适用）。**当前实现在淋水时 `EventLosePlayer()`，与 1.8+ 相悖**——随 §3.6 一并修正。
- 白天（天空光照充足）且未在追击时：放弃追击并随机传送找暗处（1.0 Beta 1.9pr4 起，1.12.2 适用）；现行 wiki「每 20–30 s 尝试脱离、40 s 未复见则放弃」为新版措辞，1.12.2 的脱离计时〔待核〕。实现以「无目标 + 强光 → 随机传送」（§3.6.3）近似该场景，主动脱离目标不实现（§6）。
- 目标跨维度/太远/死亡时丢失目标（基类 `CheckEventLostPlayer` 近似）。

### 3.4 水与雨（Water）

- 接触**雨、水、喷溅水瓶（1.11+）**各造成 **1HP 伤害**；Cuberite 无溺水豁免 ✅。
- **雨中持续传送，直到找到干处或死亡**；水中无处可传送时会沉底（即传送失败则留在原地，正常物理下沉）。
- 伤害频率：wiki 只写「接触即受 1HP」。1.12.2 每 tick 尝试伤害一次、由受伤硬直（Cuberite `m_InvulnerableTicks = 10`）节流，与现实现一致；精确节拍〔待核〕。
- 淋水**不丢失目标**（§3.3）。

### 3.5 弹射物（Projectiles）

- 「弹射物打不中」是伤害传送的副作用（wiki 历史：1.0 Beta 1.9pr4「teleport away before impact」→ 现代实现为受击即传送）；1.12.2 中箭矢实际能造成伤害后触发传送（wiki 现行「immune/bounce」描述属较新版本与 MC-79556 边角，标〔待核〕）。**不需要专门的弹射物逻辑**——§3.6.1 覆盖。

### 3.6 传送（Teleportation）——本特性核心

来源：中文 wiki [末影人「瞬移」节](https://zh.minecraft.wiki/w/%E6%9C%AB%E5%BD%B1%E4%BA%BA)（主源）+ 英文 wiki「Teleportation」节；历史表用于版本裁剪。

**尝试次数总则（中文 wiki 原文）**：「被**喷溅或滞留型水瓶**伤害或**被弹射物即将命中**时，至多尝试 **64** 次瞬移，**其余情况下均只尝试 1 次**。」
（英文 wiki 现行「因伤害传送做 64 次尝试」与中文表述冲突，**以中文为准取 1**〔待核〕；Cuberite 无水瓶对末影人的伤害实现、无弹射物临近预测，故 T1 实际恒为 1 次——两特例留 §6。）

**触发器**：

| # | 触发 | 候选点选择 | 尝试次数 | 备注 |
|---|---|---|---|---|
| T1 | 受到伤害（任意来源，含环境/窒息，1.8+） | 以当前位置为中心的 **64×64×64 立方**（每轴 ±32）内随机 | **1**（总则；64 仅限水瓶/弹射物临近两特例） | 死亡不传送 |
| T2 | 追击**玩家**目标（与其他生物战斗时不闪，中文 wiki 括注） | 中心 = 向目标方向**水平 16 / 垂直 17** 处的点（目标比该范围更近时，中心取目标的 XZ / Y），在该点周围 **9×11×9**（水平 ±4、垂直 ±5）棱柱内随机 | 每 **1.5–2 s** 一次尝试，单次，**任何距离** | 几何数值为英文 wiki 描述（Needs-testing〔待核〕）。**距离门 en/zh 冲突**：英文 wiki 限定「目标 ≥16 格时用于拉近」（Needs-testing），中文 wiki 原文「被玩家激怒时，随机尝试瞬移到激怒它的玩家后方一段距离之内」**无距离限定**——**从中文，无门**：远处=拉近（中心吸附逻辑不变），近处=玩家四周 ±4/±5 随机落点（即「身后闪现」，上游 #2552 报告的 vanilla 行为）。前一版按英文加 ≥16 门，实测近战追击时末影人被寻路卡顿完全暴露（见 §6 引擎偏差），已修正 |
| T3 | 主世界、**白天露天**（脚下方块原始天光 =15）、**已无攻击目标 ≥600 game ticks（30 s）**、**眼睛位置内部光照等级 Li ≥13** | 同 T1 | 每 tick 掷概率 **P = (11·Li − 120) / (225·(20 − Li))**，命中则单次尝试（Li=15 → 1/25 ≈ 每 1.25 s 一次） | 公式与条件为中文 wiki 原文〔待核 1.12.2 适用性：1.0 Beta 1.9pr4 历史条目「白天玩家接近即瞬移离开」证明白天逃离机制在 1.12.2 已存在，600gt 前置为现行措辞〕；「内部光照等级」以**时间修正后天光**近似（露天条件下块光无关紧要〔推测〕），眼睛取脚上 **+2 方块〔推测〕**；时间修正光夜间自动降到 4，无需单独白天门 |
| T4 | 雨中/水中 | 同 T1 | 每 tick 一次尝试（单次），直到离开水；本 tick 若伤害传送已挪位则不重复跳 | wiki「continuously teleport until dry」 |
| T5 | **因注视被激怒的瞬间**（仅凝视挑衅那一次，见 §3.2） | 同 T1 | 单次 | 1.14/19w07a 起改为「先盯住再跳」；1.12.2 为挑衅瞬间立即跳，之后被注视照常冲锋 |

**限制（中文 wiki「瞬移」节，对所有触发器生效）**：

1. **在矿车/船中（骑乘状态）不会尝试瞬移**；
2. **悬空（不落地）状态不做随机类瞬移**（T3；「灵魂沙」例外为 1.16+ 生物群系内容，不适用）；
3. 不会瞬移到世界边界外〔Cuberite `FindTeleportDestination` 无边界检查，已知偏差 §6〕；
4. 不落在黑名单方块上（基岩、火、岩浆块等——现代版本用 1.20.5+ 方块标签表达，**1.12.2 等价清单〔待核〕**，不实现 §6）。

**落点校验**（T1/T3/T4 与 T2 共用；wiki「Both teleportation types then apply the following checks」）：

1. 从随机候选点所在列向下搜索，直到「阻挡移动的方块」（movement-blocking）；
2. 站立点上方需 ≥3 格非固体（末影人高 2.9）；
3. 液体阻止站立（1.13+ 的 waterlogged 细节不适用 1.12.2）。

Cuberite 落点：`cPawn::FindTeleportDestination(world, 3, tries, dest, minCorner, maxCorner)` 已经实现同构语义（其算法注释即引本页）；差异：以 `cBlockInfo::IsSolid` 近似 movement-blocking、单段向下搜索——作为 1.12.2 规格的可接受近似，记录于此。

**副作用**（wiki「Teleportation」+历史 15w49a）：

- 传送音效 `entity.endermen.teleport` **只在目的地**播放（1.9 修复，1.12.2 适用）。
- 传送粒子（portal）出现在**起点与目的地**〔推测：wiki 未记粒子位置；两端都放为 vanilla 观感〕。粒子数量〔推测〕。

### 3.7 搬方块（Moving blocks）

来源：wiki「Moving blocks」节（列表按历史表裁剪回 1.12.2）。

- 每 tick **1/20** 概率：在以其为中心 **4×3×4**（水平 4、垂直含自身 3）区域内随机取一格；若为「可携带列表」方块**且可直接看到**，则拿起（方块消失，进入 `carried`）。
- 每 tick **1/2000** 概率：把携带方块放到以其为中心 **2×2×2**（水平 2、与其同层）区域内「上方为空气 + 下方为完整方块」的位置；放置无音效（MC-167369，1.12.2 时代未修=无声 ✅）。
- `mobGriefing = false` 时禁止拿起与放置。
- **1.12.2 可携带列表** = 现行 wiki 列表减去 1.13+ 新增（nylium、crimson/warped roots & fungi、mud、muddy mangrove roots、moss、pale moss、rooted dirt、cactus flower、eyeblossom、azalea 系），**加上 netherrack**（1.10/16w20a 加入，1.16/20w07a 移除）：草方块、泥土、podzol（灰化土）、coarse dirt（粗土）〔待核〕、沙、红沙、砾石、黏土、蘑菇、小花、南瓜（含雕刻南瓜）、西瓜、TNT、仙人掌、菌丝（mycelium）、netherrack。逐块清单落地前用 1.12 时代资料逐块复核。
- 死亡掉落所拿方块物品的**丝触等效掉落物**（1.9/15w31a 起 ✅适用 1.12.2）。
- NBT：`carried`（short）/`carriedData`（short）读写（写已有，**读缺失**）；summon 支持等价 `carriedBlockState` 即读回 `carried`。
- 「拿方块不 despawn」为 1.16 修复（MC-124812），**1.12.2 不适用，不实现**。

### 3.8 追击速度（Speed attribute）

来源：中文 wiki [属性/速度](https://zh.minecraft.wiki/w/%E5%B1%9E%E6%80%A7/%E9%80%9F%E5%BA%A6)。

| 事实 | 值 |
|---|---|
| 末影人速度属性基础值 | **0.3**（与蜘蛛/洞穴蜘蛛/狼同档；僵尸 0.23、骷髅/苦力怕 0.25、玩家 0.1） |
| 「attacking」修饰符（**有攻击目标时** add_value） | **+0.15 → 0.45**（末影人专属，猪灵蛮兵 +0.05 为唯一同类；该修饰符 13w21a/1.4.2 加入，1.12.2 适用） |
| 理论最高持续速度公式 | `0.21168·a / ((1 − 0.91·f)·f³)` 格/tick（f=脚下摩擦系数）——页面明示**生物 AI 达不到理论值**，故只能取**属性比例**做映射 |

**Cuberite 落点与工程核算**：无属性系统（#4921），速度经 `Server/monsters.ini` 的 `WalkSpeed` / `RunSpeed` 倍率实现（加载链 `MonsterConfig.cpp:80-81`）。引擎物理（本仓库实测代码）：`MoveToWayPoint` 每移动 tick `AddSpeed(1.25×倍率)`，地面摩擦每 tick 保留 `0.7/1.05≈2/3`（`Entity.cpp: ApplyFriction`）→ **终端速度 = 3.75×倍率 b/s**（约 4-5 tick 达 80%）。

但**名义倍率 ≠ 交付速度**——两个引擎侧折扣：

1. **run 分支门控语义**：原分支按 `m_EMState==CHASING` 选 RunSpeed；vanilla 的「attacking」修饰符按**持有攻击目标**门控（本页 13w21a 表原文「有攻击目标时」）。AI 态在追击中会短暂回落 IDLE（LOS 4s 计时、目标重置路径等），期间追击移动按 WalkSpeed 结算——维护者实机确认「追击经常只吃到 WalkSpeed」。已改为 `(CHASING || ESCAPING || 有目标)`（`Monster.cpp: MoveToWayPoint`）。
2. **寻路占空比**：末影人（碰撞箱需 3 格净空 + A* 20 步预算）拿到移动权的 tick 占比实测远低于低箱怪（§6.10），速度被稀释约 5-7 倍。

**ini 落点（维护者实机校准，1.12.2 正版对照）**：

```ini
WalkSpeed=1.3     ; 0.30/0.23，属性比例（无目标漫步态：路径简单、占空比高，直接可用）
RunSpeed=2        ; 0.45/0.23≈1.96，属性比例；§6.10 寻路罚站修复后占空比≈满，无需补偿
```

**已回标定**：§6.10 寻路罚站修复后（`CALCULATING` tick 朝最终目标滑移、占空比≈满），`RunSpeed` 从带缺陷引擎的补偿值 10 回落到属性比例 ≈2；vanilla 准绳 = 0.45/0.23 ≈ 1.96。交付速度 = 引擎终端速度 3.75×倍率（走 ≈4.9 / 追 ≈7.5 b/s）。维护者实机复核：交付速度**仍偏慢**——归因 **Cuberite 速度表系统性失准**（`monsters.ini` 全局倍率），不属本分支工作、另案处理；本分支交付的是属性比例映射与罚站/滑移/跳跃行为。

已知粒度差：`RunSpeed` 在 ESCAPING 态同样生效（vanilla「attacking」修饰符只在有目标时）——末影人几乎不进 ESCAPING（无 `BurnsInDaylight` 旗标，逃离靠传送），实际无影响。

---

## 4. 数据值

- metadata 15 `carried block ID`（OptBlock/short<<4|meta 依协议版本）、16 `screaming`：✅全协议已实现。
- NBT `carried`/`carriedData`：写 ✅、**读缺**（`WSSAnvil.cpp: LoadEndermanFromNBT`）。

---

## 5. Cuberite 集成点清单

| 改动 | 位置 | 分支 |
|---|---|---|
| 传送几何/概率纯函数（追击中心点、天光 P 公式 + 具名常数） | `src/Mobs/EndermanTeleportRules.h`（新） | 本分支 |
| `cEnderman`：`DoTakeDamage`（T1）、追击传送（T2）、天光传送（T3）、水雨传送（T4）、注视即传（T5）、骑乘禁传、淋水保留目标、传送音/粒子 | `src/Mobs/Enderman.{h,cpp}` | 本分支 |
| 末影人 WalkSpeed/RunSpeed（§3.8 映射） | `Server/monsters.ini` | 本分支 |
| 传送几何与天光概率单元测试 | `tests/Mobs/`（新目录） + `tests/CMakeLists.txt` | 本分支 |
| 基类移动模型：`CALCULATING` 滑移（寻路计算/冷却期间不再定身，影响所有怪） | `src/Mobs/Monster.{h,cpp}` | `feature/mobs-pathfinding-glide` |
| 末影人 RunSpeed 回标定 10→2（罚站修复后，§3.8） | `Server/monsters.ini` | `feature/mobs-pathfinding-glide` |
| 可携带列表判定纯函数、拿起/放置 tick、`carried` NBT 读取、死亡掉落 | `src/Mobs/Enderman.*`、`WSSAnvil.cpp` | 后续 `feature/mobs-enderman-block-carrying` |
| 默认刷怪列表（主世界/末地）、下界候选列表 | `World.cpp`、`MobSpawner.cpp` | 后续 `feature/mobs-enderman-natural-spawning` |
| 攻击伤害 7、经验 5、stare 音效、凝视 hitbox 化、末影螨 LOS 与珍珠来源 | 分散 | 后续小修分支 |
| 弹射物临近预测（64 次尝试特例）、水瓶对末影人伤害 | `ArrowEntity`/药水效果侧 | 后续（§3.5/§3.6 特例） |

---

## 6. 偏差与待确认

**本分支修复**：

1. 淋水 `EventLosePlayer()` 与 1.8+「水中保留愤怒」相悖 → 删除（并消除每 tick 的 metadata 广播）。
2. **注视挑衅瞬间传送（T5）补上**：因注视被激怒的那一瞬间立即传送一次（19w07a 才改为「先盯住」）；**之后被注视照常冲锋**（实机观察）。注意本分支曾把这条误实现为「对视期间每 tick 传送」，导致追击时走一步停一步/永远逼近不了，已按 19w07a 措辞语义（「激怒**后**」）修正——若把它做回每 tick 版，症状会原样复现。
3. **追击速度**：此前 ini 未配置，末影人与僵尸同速；WalkSpeed=1.3 按属性比例（§3.8）。
4. **run 分支门控**（基类，全怪适用但默认无行为差）：RunSpeed 选择条件从「AI 态==CHASING/ESCAPING」改为「**或持有攻击目标**」，对齐 vanilla attacking 修饰符语义，消除追击中态闪烁泄漏 WalkSpeed（维护者实机指认）。
5. **RunSpeed=10（实机校准的占空比补偿值，待 §6.10 修复后回标至 ≈2）**——见 §3.8，vanilla 准绳值是 1.96 不是 10。
6. **尝试次数**：受击传送按中文 wiki 总则取 1 次（英文 wiki「伤害→64 次」表述与之冲突，从中文〔待核〕）。

**本分支不做、已记录**（保持现状）：

1. 凝视判定为 5° 锥近似（规格为头部 hitbox 相交）；5-tick 驻留〔待核〕。
2. 主动脱离追击计时不实现（新发现其 1.14+ 措辞已并入 T3 的 600gt 前置）。
3. 末影螨敌视：1.12.2 应只针对珍珠来源 + 要求视线；现为「所有末影螨 + 无视线」。
4. 攻击伤害 4（规格 Normal 7）与经验 6–8（规格 5）：数值修正牵涉难度问题（#1006），留小修分支。
5. `entity.endermen.stare` 未播。
6. 弹射物「弹开/穿过」边角（MC-79556）不实现；「弹射物临近 → 64 次尝试」特例需要弹道预测，另分支（§5 表末行）。
7. 水瓶对末影人伤害（1.11/16w35a 起 1HP）Cuberite 未实现 → 「水瓶 → 64 次尝试」特例暂挂空。
8. 传送黑名单落点（§3.6 限制 4）与「不越世界边界」不实现（`FindTeleportDestination` 通用缺陷，牵一发动全身，另分支）。
9. 「空闲随机瞬移」中文 wiki 未给频率/概率，无法实现，只实现了白天天光公式（T3）；夜晚随机跳〔数值缺口〕。
10. **引擎偏差（影响所有怪）——已由 `feature/mobs-pathfinding-glide` 修复**：原 `cMonster::Tick` 只在寻路状态 `PATH_FOUND` 时施加移动速度，`CALCULATING` tick 与 `PATH_NOT_FOUND` 后的 **20 tick 冷却**（`PathFinder.cpp` `m_NotFoundCooldown`）怪完全无速度 → 追击会周期性定身；末影人碰撞箱高 2.9 → A* 需 **3 格净空**（`Ceil(2.9)`），比高 2 的僵尸/骷髅容易路径失败/耗尽 20 节点步预算 → 定身远比常见怪显眼。修复 = 基类移动模型改为「`CALCULATING` 时朝最终目标滑移并跳台阶」：
	- `MoveToWayPoint` 尾部的速度施加段抽成 `cMonster::ApplySpeedToward`（距离归一化，对任意距离安全），跳跃块抽成 `cMonster::JumpToward`（与路点移动共用同一逻辑与常数，含跳跃冷却 tick 递减），`Tick` 在 `CALCULATING` 时以 `ApplySpeedToward` 滑向 `m_FinalDestination`；
	- 滑移中**遇 1 格台阶会跳**（vanilla 寻路导航对 1 格台阶本就跳跃——路点移动已按此实现，滑移沿用同一行为）：跳跃目标钳制到**前方 1 格**（水平单位向量 ×1、Y 取最终目标）——跳跃的水平速度 `3.2×dx` 假设目标在 1 格内，直接滑向远目标会把怪水平弹出；滑移跳为**落地门控**（`cMonster::HopToward`，仅 `IsOnGround` + 目标更高即跳）：间隔 = 跳跃自身滞空时间——**成功跳上台阶的滞空约为落回本格的一半**（维护者实机指认的 vanilla 物理），两种情形各自跟随实际滞空、无额外地面停顿，落地即跳（vanilla 跳跃几乎没有延时）；固定 tick 冷却曾把两种滞空压成同一间隔（已弃）。路点移动不用 `HopToward`——保留其自身 20 tick 冷却与水中跳跃旁路（master 原文，零变化）；
	- vanilla 的等效掩盖（近身对玩家持续闪现，本分支 T2）保留；随之 `RunSpeed` 从补偿值 10 回标定至属性比例 ≈2（§3.8）。

**〔待核〕数值汇总**：T2 的 9×11×9/16/17/1.5–2 s（英文 wiki Needs-testing）；T3 公式与 600gt 前置对 1.12.2 的适用性（公式为现行描述，1.0 历史条目仅证「白天会逃离」）；「内部光照等级」定义（以时间修正天光近似）；T3 眼睛取 +2 方块〔推测〕；受击尝试次数 1（en/zh 冲突从 zh）；速度锚点 0.23〔推测〕；水中伤害节拍；1.12.2 Easy/Hard 攻击力；1.12.2 下界刷怪权重。

---

## 7. 验收与测试计划

门（沿用 AGENTS §4）：`CheckBasicStyle` / 编译 / `ctest`（排除 UrlClient-test、Google-test）/ 未动绑定与导出 API（门 4、6 不适用）。

单元测试（`tests/Mobs/`）：

- `EndermanChaseTeleportCenter` 几何：目标 >16/≤16、|dy|>17/≤17 的四象限组合、斜向独立钳制、目标在正上方、目标重合退化情形——锁定 §3.6.2 的 16/17 常数与「取目标坐标」分支。
- `EndermanSunlightTeleportChance` 概率：Li<13 → 0；Li=13 → 23/1575；Li=15 → 1/25；单调增——锁定 §3.6.3 公式。
- （T1/T4 仅常数与调用；落点校验语义由 `FindTeleportDestination` 既有实现承载，改动风险低，无世界测试桩不强行覆盖。）
- 罚站滑移（§6.10，`feature/mobs-pathfinding-glide`）：改动在 `cMonster::Tick` / `MoveToWayPoint` 控制流内，需要实体引擎（`cChunk`/`cWorld`）才有意义，现有测试框架只编译孤立源文件，无法链接实体引擎，故不强行覆盖；验证以规格逐条核对 + 编译 + `ctest` 为准（同 cEnderDragon 先例）。滑移跳跃与路点跳跃共用 `JumpToward`（同一代码路径，无独立分支）。

行为核对（无 vanilla oracle，逐条对照本文）：

- 打一下末影人 → 出现在附近 ≤64 格、目的地有声音起点无；**单次尝试**，封闭空间可能跳不掉；
- 雨中/水中末影人高频跳动直至离开，期间被打仍锁定攻击者；伤害已挪位的 tick 不重复跳；
- 站在全天光下、失去目标 ≥30 s 的末影人以约 1.25 s 平均间隔换位置；追击目标时不触发；悬空/骑乘矿车时不触发；
- 追击玩家目标时每 1.5–2 s 闪一次（**任何距离**）：远处跳近、近处绕玩家 ±4 随机跳（含身后）；追末影螨时不闪；近战缠斗中它会持续冲锋、偶尔闪现，**不应出现 >1 s 的原地罚站循环**，被 1 格台阶挡住时**落地即跳**（滑移跳跃落地门控：间隔 = 滞空时间，成功跳上≈落回本格的一半，§6.10）；
- **首次与它对视（凝视挑衅）→ 它立即跳走一次**，随后**即便一直盯着它也会冲过来**；1.12.2 **无**站桩僵直、**无**对视期间持续跳（均为 1.14+ 行为/误读）；
- 追击跑速明显快于僵尸、能贴住急走的玩家（RunSpeed=2，属性比例 0.45/0.23；罚站修复后追击速度连续无 >1 s 停顿），脱战漫步略快（≈1.3 倍）；**打/凝视它之后立刻生效**，不需要等它「进入追击态」。

---

## 8. 分支拆分（按 AGENTS §3）

1. **本分支** `feature/mobs-enderman-teleportation`：§3.6 全部 + §6.1 修复 + 本规格文档。
2. `feature/mobs-enderman-block-carrying`：§3.7（含 NBT 读回、死亡掉落）。
3. `feature/mobs-enderman-natural-spawning`：§2（依赖 1）。
4. 小修分支：§5 表末行（伤害/经验数值、stare 音、凝视判定精化、末影螨细节）。
