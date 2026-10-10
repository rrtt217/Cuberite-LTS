# Vanilla 1.12.2 — 生物自然生成（Mob spawning）

来源（cleanroom 白名单，全部为 1.12 世代快照；minecraft.wiki 现行页描述的是 1.18+/1.21 行为，只用于类别归属与「何时变更」的判定）：

- **主源** [Minecraft Wiki — Spawn（2017-06 快照，对应 1.12 世代）](https://web.archive.org/web/20170601/https://minecraft.gamepedia.com/Spawn) —— 本文 §2 调度、§3 上限、§4 群体、§5 通用落点、§11 难度、§13 清除 的**唯一机械表述**。取 `Spawn cycle` / `Mob cap` / `Pack spawning` / `Spawn conditions` / `Notes` / `Despawning` 六节。
- [Minecraft Wiki — Spawn（2018-10 快照）](https://web.archive.org/web/20181003010302/https://minecraft.gamepedia.com/Spawn) —— 与上一快照交叉比对，用于确认 1.12→1.13 之间该节未被改写。
- [Minecraft Wiki — Monster Spawner（2017-06 快照）](https://web.archive.org/web/20170601/https://minecraft.gamepedia.com/Monster_Spawner) —— §10 刷怪箱机制与 NBT 语义。
- 单生物页（各取 `Spawning` 节；时间戳由 Wayback CDX 确认为 200 状态）：
  [Slime 20170513082145](https://web.archive.org/web/20170513082145/https://minecraft.gamepedia.com/Slime)、
  [Bat 20170513104434](https://web.archive.org/web/20170513104434/https://minecraft.gamepedia.com/Bat)、
  [Squid 20170516012340](https://web.archive.org/web/20170516012340/https://minecraft.gamepedia.com/Squid)、
  [Blaze 20170513095606](https://web.archive.org/web/20170513095606/https://minecraft.gamepedia.com/Blaze)、
  [Wither_Skeleton 20170513084701](https://web.archive.org/web/20170513084701/https://minecraft.gamepedia.com/Wither_Skeleton)、
  [Ghast 20170515141720](https://web.archive.org/web/20170515141720/https://minecraft.gamepedia.com/Ghast)（以上六页已逐字核对）。
  下列各页**从缓存读取、快照号待复核**（数值标 **〔快照待复核〕**，落地前须按给出的时间戳重取一次原文）：
  [Zombie 20170515142433](https://web.archive.org/web/20170515142433/https://minecraft.gamepedia.com/Zombie)、
  [Skeleton 20170425141452](https://web.archive.org/web/20170425141452/https://minecraft.gamepedia.com/Skeleton)、
  [Creeper 20170101030048](https://web.archive.org/web/20170101030048/https://minecraft.gamepedia.com/Creeper)、
  [Spider 20170515153914](https://web.archive.org/web/20170515153914/https://minecraft.gamepedia.com/Spider)、
  [Enderman 20170119174520](https://web.archive.org/web/20170119174520/https://minecraft.gamepedia.com/Enderman)、
  [Witch 20170424054951](https://web.archive.org/web/20170424054951/https://minecraft.gamepedia.com/Witch)、
  [Guardian 20170513081724](https://web.archive.org/web/20170513081724/https://minecraft.gamepedia.com/Guardian)、
  [Ocelot 20170101075941](https://web.archive.org/web/20170101075941/https://minecraft.gamepedia.com/Ocelot)、
  [Wolf 20170513084706](https://web.archive.org/web/20170513084706/https://minecraft.gamepedia.com/Wolf)、
  [Horse 20170209153143](https://web.archive.org/web/20170209153143/https://minecraft.gamepedia.com/Horse)、
  [Rabbit 20170301160500](https://web.archive.org/web/20170301160500/https://minecraft.gamepedia.com/Rabbit)、
  [Mooshroom 20170101075752](https://web.archive.org/web/20170101075752/https://minecraft.gamepedia.com/Mooshroom)、
  [Silverfish 20170515142156](https://web.archive.org/web/20170515142156/https://minecraft.gamepedia.com/Silverfish)、
  [Magma_Cube 20170520224538](https://web.archive.org/web/20170520224538/https://minecraft.gamepedia.com/Magma_Cube)、
  [Zombie_Pigman 20170125211808](https://web.archive.org/web/20170125211808/https://minecraft.gamepedia.com/Zombie_Pigman)、
  [Difficulty 20170518215231](https://web.archive.org/web/20170518215231/https://minecraft.gamepedia.com/Difficulty)。
- 类别归属与「1.18 光照规则改写」的变更时点：[Minecraft Wiki — Mob spawning（现行页）](https://minecraft.wiki/w/Mob_spawning)，其 History 表条目 `1.18-exp1 / 21w43a：Block light level now must be 0 for many hostile mobs to spawn` 即 1.12.2 ≤7 表述仍然有效的依据。
- 上游缺陷追踪：[#4888](https://github.com/cuberite/cuberite/issues/4888)（缺口清单入口）。**本规格不引用任何反编译产物、MCP/Yarn mappings 或 Spigot/Paper 派生代码。**

本文只写规格，不含实现细节（Cuberite 落点除外）。凡 1.12.2 适用性无法从允许来源直接确认者标 **〔待核〕**；只有合理性而无来源者标 **〔推测〕**。

> **草稿状态**：§14 列出与并行规格产出的待对账接缝。行号基线 `master` @ `85bfed1`。

---

## 0. 术语对齐

vanilla 的四个生成类别与 Cuberite 的 `cMonster::eFamily`（`src/Mobs/Monster.h:28-36`）**不是同一套划分**，这是多数偏差的根因：

| vanilla 类别 | 上限常量 | Cuberite 落点 | 归属差异 |
|---|---|---|---|
| hostile / monster | **70** | `mfHostile` | 守卫者、末影人在 vanilla 属怪物帽；Cuberite 把守卫者归 `mfWater`（`Monster.cpp:1204`，自带 TODO 注释） |
| friendly / creature | **10** | `mfPassive` | Cuberite 把铁傀儡、村民计入 passive 统计（`Monster.cpp:1209,1243`），vanilla 属 Misc，不计入任何帽 |
| ambient（蝙蝠） | **15** | `mfAmbient` | 1.12.2 有独立的 ambient 帽 |
| water（鱿鱼） | **5** | `mfWater` | 1.12.2 只有单一 water 类；1.14+ 才分裂为 water-creature / water-ambient |
| misc | **无上限** | — | **Cuberite 无 misc 概念** |

**S0.1** 生成类别共 4 个，另有一类不计入任何上限的 misc（铁傀儡、雪傀儡、村民、流浪商人）。
来源：主源 `Mob cap` 节 + 现行 [Mob spawning](https://minecraft.wiki/w/Mob_spawning) 类别表。**〔待核〕** 1.12.2 的 misc 边界仅由现行页给出，主源未列。

---

## 1. 生效前提

**S1.1** 游戏规则 `doMobSpawning` 为真时才进行自然生成；为假时**只停止生成，不清除已存在的怪物**。
来源：主源 `Natural generation` 节（「These spawns … are not affected by /gamerule doMobSpawning」反证生成期放置不受该规则约束）；现行页 `Mob spawning` 节。
Cuberite：**无游戏规则系统**（全仓无 `doMobSpawning` 符号，仅 `Chunk.cpp:867`、`Mobs/Silverfish.cpp:27` 两处 TODO）。→ GAP-11。

**S1.2** 和平难度下敌对生物不自然生成；和平下仍生成列表：卫道士、唤魔者、潜影贝、末影龙。蜘蛛、洞穴蜘蛛、僵尸猪人、末影人**在和平下也不生成**。
来源：[Difficulty 20170518215231](https://web.archive.org/web/20170518215231/https://minecraft.gamepedia.com/Difficulty) `Peaceful` 节（原文含「no spiders, cave spiders, zombie pigmen or endermen … can spawn naturally」）。

**S1.3** 难度从任意档切到和平的瞬间，所有「和平下不允许生成」的怪物**立即从世界中移除**。
来源：同 S1.2。
Cuberite：**无难度系统**（无 `GetDifficulty`/`eDifficulty` 符号；进服包硬编码 `2`，`Protocol_1_8.cpp:834`；存档写 `Difficulty=2`，`WSSAnvil.cpp:96`）。→ GAP-10，阻塞 S1.2/S1.3/S5.4。

---

## 2. 生成调度（spawn cycle）

**S2.1** 每类别独立调度：**敌对每 1 tick 一轮**；**友好与水生每 400 tick 一轮**。
来源：主源 `Spawn cycle` 节原文「Hostile mobs have a spawning cycle once every game tick. Friendly and water mobs have only one spawning cycle every 400 ticks」。
Cuberite：`cMonster::GetSpawnDelay`（`src/Mobs/Monster.cpp:1263-1274`）返回 hostile/passive/ambient **40**、water **400**；调用点 `src/World.cpp:2166`。敌对慢 **20 倍**、友好快 **10 倍**。→ **GAP-01**

**S2.2** 每一轮对**每个可生成区块**做**一次**群体生成尝试。
来源：主源 `Pack spawning` 节原文「For each spawning cycle, one attempt is made to spawn a pack of mobs in each eligible chunk」。
Cuberite 的「每区块一次 `cChunk::SpawnMobs`」形状与此**一致**（`src/ChunkMap.cpp:1325-1336` → `src/Chunk.cpp:621`）。差异只在区块集合（§3）与周期（S2.1）。

**S2.3** 上限检查在**每轮开始时做一次**；该类别超限则**整轮跳过**。
来源：主源 `Mob cap` 节。
Cuberite 形状一致（`World.cpp:2167`）。

**S2.4** 冷却在**本轮无产出时也必须推进**，且**本轮新增的生物不计入本轮上限统计**（先统计后生成）。
来源：主源 `Mob cap` 节（「checked once at the beginning of each spawning cycle」）。
Cuberite 形状一致（census 在 `World.cpp:2151`，生成在 `:2176-2181`）。

---

## 3. 可生成区块集合与生成上限

**S3.1** 可生成区 = 以每名玩家为中心的 **15×15 区块**（240×240 格）；玩家坐标非 8 的整数倍时按所在区块的中心划分。
来源：主源 `Spawn cycle` 节 + `Notes` 节。

**S3.2** 上限计算时把该区域**每方向各扩一区块**，得 **17×17 = 289 区块**。
来源：主源 `Mob cap` 节原文「the spawning area is expanded by one chunk in every direction (so the default gives 17×17 chunks)」。
`Notes` 节记载历史上曾用 17×17 作生成区，后缩为 15×15 但上限仍按 17×17 计——**两者不协调是 vanilla 的有意保留，修正时不得「顺手对齐」**。

**S3.3** 上限公式：`mobCap = constant × chunks / 289`，常量 hostile **70** / passive **10** / ambient **15** / water **5**。
来源：主源 `Mob cap` 节。
Cuberite：`MobCensus.cpp:33-47` 用 **79 / 11 / 16 / 5**，分母硬编码 **319**，注释自承 `MG TODO : code the correct count`。→ **GAP-02**

**S3.4** 多人下被多名玩家共享的区块**只计一次**；玩家越分散，总区块越多，上限越高。
来源：主源 `Mob cap` 节。
Cuberite 用 `std::set<cChunk *>` 天然去重（`MobCensus.h:48`），此点**一致**；但集合定义不同（S3.5）。

**S3.5** 计入上限统计的怪物范围 = 与 S3.2 相同的 17×17 区块集合。
来源：主源 `Mob cap` 节末句。

**S3.6** 视图距离 ≤9 时生成量显著低于视图距离 ≥10；vanilla 在视图 10 时 15×15 区域不被截断。
来源：主源 `Notes` 节。
Cuberite：可生成集合 = **所有「有效且被至少一个 client 加载」的区块**（`ChunkMap.cpp:1314` → `Chunk.cpp:558` 无条件登记），随视图距离（默认 10，`ClientHandle.h:57`）线性膨胀 → 上限 ≈ `79×441/319` ≈ **109**，高出 vanilla 56%。→ **GAP-03**

---

## 4. 群体生成（pack spawning）

**S4.1** 区块内随机取一格为**群体中心**；中心方块须「非不透明」**或**「不满格」**或**「被充能」，否则**整轮尝试取消**。
来源：主源 `Pack spawning` 节。
Cuberite：`CheckPackCenter` 要求中心**恰好是 `E_BLOCK_AIR`**（水生为 `IsBlockWater`）（`MobSpawner.cpp:31-43`），调用点 `Chunk.cpp:626-630`。→ **GAP-04**

**S4.2** 最多 **12 次**尝试，最多成功数 = 群体大小：**狼 8**、**幽灵 1**、**兔 3**、**其余 4**。
来源：主源 `Pack spawning` 节；幽灵 pack=1 另由 [Ghast 快照](https://web.archive.org/web/20170515141720/https://minecraft.gamepedia.com/Ghast)「in a single chunk only one ghast can spawn」交叉确认。
Cuberite：`Chunk.cpp:635-636` 12 次 / 4 只，`MobSpawner.cpp:528-539` 覆写 wolf 8、ghast 1、**凋灵骷髅 5**（无来源依据，见 §6）。→ **GAP-05**

**S4.3** 群体在 **41×1×41**（以中心为中心的 41×41 单格高方块）内生成，落点**强烈向中心偏置**：约 85% 在中心 5 格内，99% 在 10 格内。
来源：主源 `Pack spawning` 节（含该两个百分比）。
Cuberite：水平 ±20、垂直 0 正确（`Chunk.cpp:638-639`），但用**均匀分布**（`GetThreeRandomNumbers`，`:641`）→ 远端权重过高。→ **GAP-06**

**S4.4** 同群体**全部同种**；种类在**首次落点检查处**按可生成集合选定。
来源：主源 `Pack spawning` 节。
Cuberite 形状一致（`MobSpawner.cpp:523-542` 仅 `m_NewPack` 时选种）；**选种是等概率**，vanilla 按权重（平原与草原的马权重差 5 倍，见 §7）。→ **GAP-07**

**S4.5** 中心 Y 在「该列最高方块之上」到世界底部之间取随机。
来源：现行 [Mob spawning](https://minecraft.wiki/w/Mob_spawning) `Pack spawning` 节。
**〔待核〕** 主源仅说「a random location in the chunk」，未给 Y 分布。
Cuberite：Y 在 `[1, 254]` 全域均匀（`Chunk.cpp:608-615`）；若 vanilla 确实从地表起算，则地表生成概率被压到 1/254 量级。

---

## 5. 通用落点条件（非鱿鱼、非守卫者）

**S5.1** 脚下方块须有**实心顶面**（不透明，或倒置台阶/楼梯等），且**不得是基岩或屏障**。
来源：主源 `Spawn conditions` 节。
Cuberite：`!cBlockInfo::IsTransparent(BlockBelow)` + 基岩排除（`MobSpawner.cpp:82-85`、各 case）——方向正确，`IsTransparent` 与「实心顶面」不完全同义。

**S5.2** 站立方块与其上方方块**不得阻挡移动**、非液体、未充能，且**不得是铁轨/充能铁轨/探测铁轨/激活铁轨**。生物可生成在火把、梯子、告示牌等方块内。
来源：主源 `Spawn conditions` 节 + `Notes` 节（「can spawn inside signs … cannot spawn on any type of rails or on pressure plates」）与 `Pack spawning` 配图说明（「mobs can spawn inside torch and ladder blocks」）。
自 1.9/15w46a 起禁用清单另含**按钮、压力板、拉杆、红石火把、中继器、比较器、红石线**；玻璃**不构成合法地板**（原文图注「they can't spawn on top of glass because it's not opaque」）。
Cuberite：多数 case 要求站立与上方**恰为 `E_BLOCK_AIR`**（`MobSpawner.cpp` 各 case）→ 比 vanilla **更严**，火把/梯子/下半砖/地毯/告示牌位置一律不生成。→ **GAP-08**

**S5.3** 生成生物的**碰撞箱不得与实体方块、其它生物或液体相交**。
来源：主源 `Spawn conditions` 节。
Cuberite：**完全无此检查**——`SpawnMobFinalize`（`World.cpp:3921-3945`）→ `cEntity::Initialize`（`Entities/Entity.cpp:128-149`）→ `AddEntity`，无 `DoesCollide`。→ **GAP-09**

**S5.4** 站立点 24 格球形范围内**不得有玩家，也不得有玩家出生点**。
来源：主源 `Spawn conditions` 节。
Cuberite：只查玩家（`MobSpawner.cpp:506-519`）。此外三处多余/错误语义：① 走 `DoWithNearestPlayer` 默认 `a_CheckLineOfSight = true`（`World.h:252`、`World.cpp:3379-3385`）→ **被方块遮挡的玩家不阻断生成**，vanilla 无视线判定；② 观察者被排除（`World.cpp:3364`）；③ 距离取方块**整数角点**而非怪实际落点中心（`MobSpawner.cpp:505` vs `Chunk.cpp:683-685`），差最多约 0.7 格。→ **GAP-12**

**S5.5** 相邻区块边界处的尝试必须使用**邻接区块**的方块与群系数据。
来源：S5.1/S5.2 的检查语义必然要求（主源 `Pack spawning` 明示 41×41 区域跨区块）。
Cuberite **实现级 bug**：`Chunk.cpp:671` 调 `GetRelNeighborChunkAdjustCoords(Try)` 后 `Try` 已被改写为邻接区块局部坐标（`Chunk.cpp:1895+`，签名按引用），但 `:678` 传入的区块实参仍是 `this`，`GetBiomeAt` 同样取在 `this` 上；`cChunk::GetBlock` 不做跨界平移（`ChunkData.cpp:22-31` 的 `ASSERT(IsValidRelPos)`）→ Debug 断言、Release 回绕到本区块内错误坐标。正确写法见同仓 `MobSpawnerEntity.cpp:163-170`。→ **BUG-01**（非规格偏差，属实现错误）

---

## 6. 分生物附加条件

「光照」一律指 vanilla 的**光照等级**（方块光照与经时间修正的天光取较大者）；Cuberite 在 `MobSpawner.cpp:95` 用 `GetTimeAlteredLight` 做天光修正，方向正确，但多数 case 分别校验 `BlockLight <= N && SkyLight <= N`（等价于 max ≤ N，可接受）。

| # | 生物 | vanilla 1.12.2 | 来源 | Cuberite 现状 |
|---|---|---|---|---|
| S6.1 | 僵尸 / 骷髅 / 爬行者 / 蜘蛛 | 光照 ≤7，且**光照越强失败率越高**；阳光直射额外提高失败率**最多 50%**；蜘蛛需 3×3×2 空间，上方可透明不可实心 | 主源 `Spawn conditions` 节原文「the light level of the spawning block must be 7 or darker (with exception during thunderstorms), and more light increases the chance that the spawn will fail」「sunlight falling on the spawning block further increases the chance that the spawn will fail, by up to 50%」；[Spider 快照](https://web.archive.org/web/20170515153914/https://minecraft.gamepedia.com/Spider)〔快照待复核〕 | 三者共用同一 case 块（`MobSpawner.cpp:171-184`），光照 ≤7 **正确**（`SkyLight <= 7 && BlockLight <= 7` 等价于 vanilla 的 max ≤7）。偏差在块内末项 `Random.RandBool()`：把 vanilla「随光照与日照递增的失败率」**简化为恒 50% 固定失败率**——光照 0 的完全黑暗中也被砍一半，光照 7 处又不足。属**同一意图、错误实现**，非凭空发明。蜘蛛有 2×2 近似（应 3×3×2） |
| S6.2 | 洞穴蜘蛛 | **不属自然生成集合**，仅由地牢/矿井刷怪箱产出 | 主源 `Pack spawning` 可生成清单不含之 | `Types` 默认串含 `cavespider`，但 `GetAllowedMobTypes` 从不插入（`MobSpawner.cpp:376-495`）→ 永不产出，行为正确、配置误导 |
| S6.3 | 末影人 | 光照 ≤7，需 3 格净空，包 4；主世界与下界权重 10（敌对中最稀有） | [Enderman 快照](https://web.archive.org/web/20170119174520/https://minecraft.gamepedia.com/Enderman)〔快照待复核〕；权重待核 | `MobSpawner.cpp:186-206` ✓（3 格净空、y<250）；等概率无法表达「最稀有」 |
| S6.4 | 女巫 | **可自然生成**，光照 ≤7，包最多 3 | 主源可生成清单含 witches；[Witch 快照](https://web.archive.org/web/20170424054951/https://minecraft.gamepedia.com/Witch)〔快照待复核〕 | **双重缺失**：群系表无、`CanSpawnHere` 亦无 `mtWitch` case → `default: return false`（`MobSpawner.cpp:363-367`）。→ **GAP-13** |
| S6.5 | 史莱姆（区块） | 史莱姆区块（约 1/10）、y **< 40**、**无视光照**、**90% 失败率** | [Slime 快照](https://web.archive.org/web/20170513082145/https://minecraft.gamepedia.com/Slime) + 主源 `Spawn conditions` | `MobSpawner.cpp:229-259` 有 `IsSlimeChunk()` 与 y≤40，**无 90% 失败率** → 史莱姆过量 10 倍。→ **GAP-14** |
| S6.6 | 史莱姆（沼泽） | 沼泽、y **51–69**、光照 ≤7、月相参与、50% 失败 | 同上 | y 取 50–70（差一）；月相式 `moonThreshold = \|phase−4\|/4` **〔待核〕**（Slime 页表述为「月之亮面比例 > 随机数」，等价性未证） |
| S6.7 | 岩浆怪 | 下界任意光照，稀有；需 2 格透明空间 + 下方不透明 | [Magma_Cube 快照](https://web.archive.org/web/20170520224538/https://minecraft.gamepedia.com/Magma_Cube)〔快照待复核〕 | 走 `mtSlime` 分支，会误染史莱姆区块/沼泽条件（`MobSpawner.cpp:246-257`） |
| S6.8 | 幽灵 | **95% 失败率**（= 1/20 于其它敌对）、无视光照、包 1、需下方实心 + 5×4×5 净空 | 主源 `Spawn conditions`；[Ghast 快照](https://web.archive.org/web/20170515141720/https://minecraft.gamepedia.com/Ghast) | `MobSpawner.cpp:208-216` 用 **1%**（`RandBool(0.01)`）→ 稀 5 倍；无 5×4×5 净空检查。→ **GAP-15** |
| S6.9 | 烈焰人 | **仅下界要塞**、光照 **≤11** | [Blaze 快照](https://web.archive.org/web/20170513095606/https://minecraft.gamepedia.com/Blaze) | 无光照检查、无要塞限制（本检出**不生成下界要塞**）→ 全下界任意亮度可刷 |
| S6.10 | 凋灵骷髅 | **仅下界要塞**、光照 ≤7、包 5、**由普通骷髅 80% 替换而来** | [Wither_Skeleton 快照](https://web.archive.org/web/20170513084701/https://minecraft.gamepedia.com/Wither_Skeleton) | 光照 ≤7 ✓；`RandBool(0.6)`（`MobSpawner.cpp:330`）**无据**，正解是「下界骷髅 80% 转凋灵骷髅」 |
| S6.11 | 僵尸猪人 | 下界任意光照、包 4、5% 婴儿 | [Zombie_Pigman 快照](https://web.archive.org/web/20170125211808/https://minecraft.gamepedia.com/Zombie_Pigman)〔快照待复核〕 | ✓（无光照检查）；无婴儿变体 |
| S6.12 | 守卫者 | **仅海洋纪念碑内外**；站立与下方皆水、上方无碰撞；天空可见时 **95% 失败**；多在 y 39–63 | [Guardian 快照](https://web.archive.org/web/20170513081724/https://minecraft.gamepedia.com/Guardian)〔快照待复核〕 | 仅按群系（海洋/河流/冻洋）+ y 45–62（`MobSpawner.cpp:218-227`），无纪念碑、无 95% 失败；类别归 `mfWater`（应属怪物帽） |
| S6.13 | 鱿鱼 | 水中、y **46 – 海平面(62)**、任意群系 | [Squid 快照](https://web.archive.org/web/20170516012340/https://minecraft.gamepedia.com/Squid) | y **45**–62（`MobSpawner.cpp:314-321`）差一 |
| S6.14 | 蝙蝠 | 站立点 **y ≤62**；光照 **≤4**；10/20–11/3 放宽为 **≤7**；站立方块与上方方块按通用规则「不阻挡移动」 | 主源 `Spawn conditions` 原文「the spawning block must be at level 62 or below」「If the real-time day is between October 20 and November 3, then the light level must be 7 or darker. Otherwise the light level must be 4 or darker.」。〔源冲突〕Bat 页作「光照 3 或以下 / 放宽到 6」，取主源 | `MobSpawner.cpp:106-116`：y `<= 63`（**差一**）；光照 ≤4 ✓；**无 Halloween 例外**；且要求 `!IsTransparent(BlockAbove)` 即**上方须为不透明方块**——与主源「上方方块须不阻挡移动」**方向相反**（空气 `IsTransparent` 为真 → 蝙蝠在正常空腔下反而刷不出） |
| S6.15 | 家畜（鸡/牛/猪/羊/兔/马） | **生成方块光照 ≥9**（合并光照，来源未指明专取天光）且下方为草方块；兔包 2–3；马群 2–6（10% 为驴，20% 为幼崽）；哞菇仅蘑菇岛、菌丝上、群 4–8 | 主源 `Spawn conditions` 原文「the light level of the spawning block must be 9 or brighter」「the block directly below the spawning block must be grass」；[Rabbit](https://web.archive.org/web/20170301160500/https://minecraft.gamepedia.com/Rabbit)/[Horse](https://web.archive.org/web/20170209153143/https://minecraft.gamepedia.com/Horse)/[Mooshroom](https://web.archive.org/web/20170101075752/https://minecraft.gamepedia.com/Mooshroom)〔快照待复核〕 | 用 `SkyLight >= 9`（**只看天光**，`MobSpawner.cpp:155-184`）→ 火把照亮的洞穴与密林地表不生成；兔包 4（应 2–3）；马无驴/幼崽；哞菇包 4（应 4–8） |
| S6.16 | 狼 | 森林/Taiga 系群系；**群体上限〔源冲突〕：主源 `Pack spawning` 作 8，Wolf 页 1.12 时代快照作「packs of up to 4」** | 主源 `Pack spawning` 节 vs [Wolf 快照](https://web.archive.org/web/20170513084706/https://minecraft.gamepedia.com/Wolf)〔快照待复核〕 | 包 8 取主源（`MobSpawner.cpp:534`）；但要求站立方块 `== E_BLOCK_GRASS`（`:335-352`）→ 语义为「生成在草方块内部」，与 S5.1/S5.2 冲突 |
| S6.17 | 豹猫 | 丛林、站立点 **高于 63 层**（即 y ≥ 64）、下方草或叶、**1/3 失败** | 主源 `Spawn conditions` 原文「the spawning block must be above level 63」「there is a 1/3 chance the spawn will fail」 | `y >= 62`（**差二**）；`RandBool(2.0/3.0)` = 1/3 失败 ✓；`E_BLOCK_LEAVES`/`E_BLOCK_NEW_LEAVES` 覆盖「叶」✓ |
| S6.18 | 蠹虫 | **不自然生成**；刷怪箱产出时**光照 ≤11**，**但站在石头及其变体上时无视光照**，且**距玩家 5 格内不生成** | 主源；[Silverfish 快照](https://web.archive.org/web/20170515142156/https://minecraft.gamepedia.com/Silverfish)〔快照待复核〕 | `MobSpawner.cpp:141-153` 光照 ≤11 但**同时校验天光**（要塞顶部天光 15 → 永刷不出），且无石头例外、无 5 格规则。→ **GAP-16** |

---

## 7. 群系与结构对可生成种类的限制

**S7.1** 主世界「一般地点」可生成：羊、猪、鸡、牛、兔、蜘蛛、僵尸、骷髅、爬行者、末影人、史莱姆、**女巫**、鱿鱼。
来源：主源 `Pack spawning` 节（2016-11 与 2017-06 两快照文本一致）。

**S7.2** 例外：森林/Taiga 系加狼；平原与草原加马（**草原马权重为平原的 1/5**）；丛林加豹猫并提高鸡的概率；沙漠与积雪群系（冰原除外）只有兔；海滩/石岸/河流/海洋/恶地无动物；蘑菇岛仅哞菇。
来源：主源 `Pack spawning` 节。
Cuberite：手写 `switch`（`MobSpawner.cpp:376-495`）**无权重**、**完全不看维度**（`GetAllowedMobTypes` 恒返回主世界∪下界候选）→ 下界刷猪、末地可刷鱿鱼；沙漠「有兔无羊猪牛鸡」实现为「鸡牛猪羊缺失」但兔条件另有。→ **GAP-07** 的组成部分

**S7.3** 结构生成点覆盖群系候选：**女巫小屋只刷女巫**、**海洋纪念碑只刷守卫者**、**下界要塞只刷骷髅/凋灵骷髅/烈焰人（及岩浆怪）**，要塞外只刷幽灵（僵尸猪人/岩浆怪/末影人不限）。
来源：主源 `Pack spawning` 节 Nether 段 + 各结构页〔快照待复核〕。
Cuberite：**三类结构生成点全缺**；本检出 Nether 生成器不产要塞（`src/Generating/` 内无 fortress 实现）。→ GAP-17

**S7.4** 末地只有末影人。
来源：主源 `Pack spawning` 节原文「In the End, only endermen will spawn」。
Cuberite 靠 `Types=enderman` 配置达成（`World.cpp:1228`），**非代码约束**；`GetAllowedMobTypes` 在末地仍返回含鱿鱼在内的全集。

---

## 8. 区块生成期的生物放置

**S8.1** 约 **1/10** 的新生成区块带动物，**同种最多 4 只**，生成在该列**最高可用方块**上；该方块须**不透明**且**其上两格透明**；**不要求草方块**、**不要求光照**。
来源：主源 `Natural generation → Animals` 节。随机性由世界种子导出。

**S8.2** 该放置**不受** `doMobSpawning` 影响；沙漠/海滩/海洋不放置（兔、鱿鱼除外）；蘑菇岛仅哞菇。
来源：主源 `Natural generation` 节。

**S8.3** **实现级 bug**：Cuberite `cFinishGenPassiveMobs::TrySpawnAnimals`（`Generating/FinishGen.cpp:1655-1683`）的通过条件写作
`(AnimalToSpawn != mtSquid) && (BlockAtHead != AIR) && (BlockAtFeet != AIR) && (!IsTransparent(BlockUnderFeet))`——
三个「不满足」用 **AND** 连接，等价于「三者同时不满足才拒绝」，故头卡在土里、脚悬空亦可通过。应为 **OR**。同时：包大小固定「1+3」不随机；要求草方块（`:1673-1678`）违反 S8.1「不要求草方块」；噪声 `(IntNoise2DInt/7) % Width` 可能为负 → 取模得负下标。→ **BUG-02**

---

## 9. 刷怪箱（Monster Spawner）

**S9.1** 激活条件：16 格内有玩家；**每 tick** 判定。
来源：[Monster Spawner 快照](https://web.archive.org/web/20170601/https://minecraft.gamepedia.com/Monster_Spawner)。

**S9.2** 生成体积：**8×8×3**，以方块的 x/z 为中心（**不是以方格为心**）、以方块的 **y（底面）** 为中心 → 合法水平位移 `posX = spawnerX + (rand − rand) × spawnRange`，**`SpawnRange` 默认 4**；垂直为 `spawnerY + randint(−1, +1)`（腿所在层 = 同层 / 上层 / 下层）。
来源：同上 `Mechanics` 节与 Block entity data 节。

**S9.3** 每次尝试 **4** 只；成功后冷却 **200–799** tick；**一只都没刷出时不冷却，下一 tick 继续重试直到成功**；只有至少刷出一只才进入冷却。
**S9.4** 以刷怪箱方块为中心的盒内已有 **≥6** 只同种生物时，只播粒子不生成，并照常进入等待。盒尺寸取 NBT 精确式 `spawnrange*2+1 × spawnrange*2+1 × 8` → 默认 **9×9×8**（水平 ±4、垂直 y−4 … y+3）；同页散文将其圆整表述为 9×9×9，两者不矛盾，**实现按精确式**。计数以**碰撞箱重叠**为准而非实体坐标；1.8/14w26c 起该盒由 17×9×17 缩至上述值，1.12.2 已是新值。
来源：[Monster Spawner 快照](https://web.archive.org/web/20171007134352/https://minecraft.gamepedia.com/Monster_Spawner) `MaxNearbyEntities` 条与 `History` 表。
**S9.5** 除「敌对生物可**无脚下方块**（悬空）生成」外，**其余生成条件一律照常**（含光照）。
来源：同 S9.1（原文「Except for spawning on a solid block, all of the usual requirements for spawning must be met」）。

**S9.6** NBT：1.9+ 权威为 `SpawnData` 复合标签（可含任意实体标签：装备、血量、`PersistenceRequired`…）；`SpawnPotentials` 为加权候选列表；`EntityId` 已弃用，且在存在 `SpawnPotentials` 时**下次尝试后被覆写**。

Cuberite 现状与差距：

| 项 | 现状 | 对 S9 |
|---|---|---|
| 生成范围 | `m_SpawnRange = 8`（`MobSpawnerEntity.h:86`，注释自称「Diameter」）；位移 `(rand−rand)×8` → **±8**（`.cpp:157-161`） | S9.2 **超 2 倍** → **GAP-18** |
| 附近上限盒 | `cBoundingBox({x,y−4,z}, m_SpawnRange, 8)` → **x/z ±8**（`.cpp:248`） | S9.4 应为 **±4** |
| 激活判定频率 | 每 **100 tick**（`.cpp:97-101`）；非激活时 `m_SpawnDelay` **冻结不递减**（`.cpp:103-106`） | S9.1 应每 tick；冻结导致玩家回流瞬间不刷 |
| 生成校验 | 传 `a_DisableSolidBelowCheck = true`（`.cpp:170`），与 S9.5 一致 ✓ | |
| NBT | 只读写扁平 `EntityId`（`WSSAnvil.cpp:1363-1377`、`NBTChunkSerializer.cpp:555-568`），**无 `SpawnData`/`SpawnPotentials`** | S9.6 → **GAP-19**：读 vanilla 存档全成猪，写出的存档 vanilla 覆写为默认猪 |
| 冷却语义 | 仅在刷出时 `ResetTimer`（`.cpp:192-195`） ✓ | S9.3 ✓ |

---

## 10. 结构预置的刷怪箱

**S10.1** 自然生成位置与类型（逐项列于来源，非推导）：地牢 **僵尸 50% / 骷髅 25% / 蜘蛛 25%**；**废弃矿井 洞穴蜘蛛**（被蜘蛛网密集包围）；林地府邸密室蜘蛛；**要塞末地门厅蠹虫**（每要塞 1 个）；下界要塞阳台烈焰人（通常 2 个）。
来源：[Monster Spawner 20171007134352](https://web.archive.org/web/20171007134352/https://minecraft.gamepedia.com/Monster_Spawner) `Generation` 节，原文「It may be a zombie spawner (50% chance), skeleton spawner (25%), or spider spawner (25%)」「Abandoned mineshafts — Cave spider spawner heavily surrounded by cobwebs」。

Cuberite 现状（均为**实现级问题**，非规格偏差）：

- **BUG-03**：主世界默认 `Finishers` 串（`Generating/ComposableGenerator.cpp:229-256`）**不含 `DungeonRooms`**（该 finisher 已实现，`:467-473`）→ 出厂配置下**地牢与地牢刷怪笼根本不生成**，而 S10.1 表明地牢是 vanilla 刷怪笼的主要来源。
- 矿井预置刷怪笼并固定为洞穴蜘蛛（`MineShafts.cpp:854-888`，概率 12/250，`:523-531`）——**与 S10.1 一致，属正确实现**（本文早期版本据「矿井无刷怪笼」的错误印象把它列为偏差，已撤回）。
- 地牢刷怪笼类型分档 `MobType <= 25 / <= 50 / else`（`DungeonRoomsFinisher.cpp:64-77`）得 26% / 25% / 50%，与 S10.1 的 25/25/50 **实质一致**（±1 取整，僵尸由 50→49/50 边界差异），**不算偏差**；同行注释「25/25/50」亦无误。
- 林地府邸在默认 `Finishers` 中不存在（`SinglePieceStructures` 仅 `JungleTemple|WitchHut|DesertPyramid|DesertWell`），故府邸密室蜘蛛刷怪笼无从生成——归入 GAP-17 的「结构缺失」，非刷怪笼逻辑问题。

---

## 11. 生成期放置与运行期入口（非自然生成，列作边界）

以下入口**不得**套用 §5/§6 的落点条件：刷怪蛋、`/summon`、发射器、下界门、虫蚀方块、繁殖产仔、史莱姆分裂、闪电转化（猪→僵尸猪人、村民→女巫）、末影珍珠→末影螨。
Cuberite 现状与此一致（`World::SpawnMob*` 不做任何校验）——**此为正确行为，勿在补齐 §5 时一并加检查**。

**S11.1** `SpawnMobFinalize` 应保留「插件可否决生成」的语义。Cuberite 有两道钩子（`OnSpawningMonster`、`OnSpawningEntity`）✓。

---

## 12. 清除与持久（despawn）

**S12.1** 32 格内无玩家累计超过 **30 秒**后，每 tick 有 **1/800** 概率消失（≈2.47%/秒，30 秒后半衰期 27.73 秒）。
**S12.2** **128** 格球形范围内无玩家 → **立即**消失。
**S12.3** 以下情形**不会**消失：named（name tag）；手持或穿戴任何玩家给予/拾取的物品；矿车中的怪物；被像素上「持久化」标记的结构生成怪（如僵尸村民、女巫小屋的女巫）。
**S12.4** 鸡骑士的鸡**遵循僵尸的清除规则**（会消失）。
**S12.5** 多人：无玩家在线时**不进行任何清除**。
**S12.6** 切到和平 → 全部敌对立即消失（= S1.3）。
来源：主源 `Despawning` 节全文；史莱姆页「despawn over time if no player is within 32 blocks and instantly if no player is within 128 blocks」独立交叉印证。

Cuberite **全部缺失**：唯一下线路径是「hostile + 所在区块无 client + 最近目标非玩家 → 立即 `Destroy()`」（`World.cpp:2206-2220`），判据是**视图距离**而非欧氏距离；被动/环境/水生从不消失；无任何 persistence 豁免；`WasLastTargetAPlayer` 在 `SetTarget(nullptr)` 后**不复位**（`Monster.cpp:1302`）→ 一旦锁定过玩家就永久免疫。已实现的只有末影螨 2 分钟寿命（`Mobs/Endermite.cpp:15`）。

**可低成本落地**：`cMobProximityCounter` 已在 census 阶段采集每个怪到最近玩家的**平方距离**（`Chunk.cpp:577`、`MobProximityCounter.cpp:13-37`），其唯一出口 `cMobCensus::GetProximityCounter()`（`MobCensus.cpp:71-74`）**全仓零调用者**——S12.1/S12.2 所需数据已就绪，只差消费。→ **GAP-20**

---

## 13. 缺口 → 分支映射

| 缺口 | 内容 | 建议分支 | 依赖 |
|---|---|---|---|
| BUG-01 | 跨区块尝试用错区块（`Chunk.cpp:678` → `Chunk`） | `feature/mobs-spawn-neighbor-chunk-fix` | 无 |
| BUG-02 | 生成期放置 AND→OR（`FinishGen.cpp:1655-1683`） | `feature/mobs-chungen-passive-mobs-fix` | 无 |
| BUG-03 | 默认 `Finishers` 缺 `DungeonRooms` | `feature/gen-default-dungeons` | 无 |
| GAP-01/02/03 | 调度周期、上限常量与分母、15×15/17×17 区块集 | `feature/mobs-spawn-scheduler` | 本文 |
| GAP-20 | 清除与持久（S12.1–S12.6） | `feature/mobs-despawn-persistence` | 无（数据已就绪） |
| GAP-18/19 + S9.1/S9.4 | 刷怪箱参数、激活频率、`SpawnData` | `feature/mobs-mob-spawner-block` | 无 |
| GAP-04/06/07/08/09/12 | pack 中心、偏置、权重、S5.2/S5.3/S5.4 | `feature/mobs-spawn-placement-rules` | GAP-01 |
| GAP-07 + §7 | 群系 × 类别加权生成表 | `feature/mobs-biome-spawn-table` | GAP-01 |
| §6 逐条 | 各生物附加条件 | `feature/mobs-spawn-conditions` | 上一条 |
| GAP-10/11 | 难度系统、`doMobSpawning` | `feature/world-difficulty`、`feature/world-gamerules` | 规模 ≥ 本文任一分支，**应拆独立分支先行**（AGENTS.md §3） |
| GAP-17 | 下界要塞 / 海洋纪念碑 / 沼泽小屋及其生成点 | `feature/gen-nether-fortress` 等 | 同上 |

---

## 14. 对账结论与剩余未决项

**与并行规格产出（另一路独立抓取）对账完毕。** 该路给出 30+ 个 1.12 世代快照 URL 与逐条结论，本文已吸收其成立部分、撤回其被原文推翻部分：

| 议题 | 该路结论 | 本文处置 |
|---|---|---|
| 废弃矿井有无刷怪笼 | **有**（洞穴蜘蛛） | **采纳并撤回本文原 S10.2**——[Monster Spawner 快照](https://web.archive.org/web/20171007134352/https://minecraft.gamepedia.com/Monster_Spawner) `Generation` 节原文明列「Abandoned mineshafts — Cave spider spawner」。Cuberite 的矿井刷怪笼属**正确实现**，BUG-03 的连带指控一并撤回 |
| per-player mob cap | 1.18 实验快照 3 引入（非 1.14） | **采纳**，见 §15 护栏 |
| 守卫者「Y 45–62」 | 无 Java 来源支持 | **采纳**——本文 S6.12 的 vanilla 侧只写「纪念碑 + 水 + 露天 95% 失败 + 观测 Y 39–63」，「45–62」仅是 Cuberite 侧的代码事实 |
| 友好生物光照 ≥9 的口径 | 指**生成方块光照**，非专指天光 | **采纳**，见 S6.15 |
| 「每次尝试在玩家周围 4–8 区块环带取区块」 | 任何白名单来源均查不到 | **采纳**——本文未写入该说法，S2.2/S3.1 只保留主源可证的「每合格区块 1 次 / 15×15 区域」 |
| `8×8×3` vs `9×3×9` | 视为冲突 | **判定不冲突**，见 §15 末注——同一句原文的两种表述 |
| 地牢 25/25/50 | 主源未列 | **已升级为有源**，见 S10.1 |
| 蝙蝠光照 ≤3 vs ≤4 | 取 ≤3/≤6 | **保留本文取 ≤4/≤7**（主源 `Spawn conditions` 逐字给出，优先于 Bat 页），冲突已在 S6.14 标注，属需实测项 |
| 女巫包 ≤3 vs 主源「其余 4」 | 冲突 | **保留主源的 4**（S4.2 口径），S6.4 的 ≤3 作为单生物页佐证并列 |
| 沼泽史莱姆 Y 51–69 vs 50–70 | 冲突 | **保留 51–69**（主源逐字「on level 51 through 69 inclusive」） |

**剩余未决项（不得当已定规格验收）**：

1. **S4.5 pack 中心 Y 取法**：主源只说「a random location in the chunk」，无 1.12 世代来源给出 Y 分布。Cuberite 的 `[1,254]` 全域均匀（`Chunk.cpp:608-615` + `:587-601`，`Height-2` 后 `a_Y++`）是否为 vanilla 行为，**未知**。
2. **§7 权重表**：1.12.2 的 biome × category 加权绝对值在所有允许来源中均无完整表述；可证的只有「稀树草原马 = 平原的 1/5」这一条相对关系。GAP-07 的验收只能写成相对权重约束。
3. **光照合并方式**：主源只说「light level of the spawning block」，未述 blockLight/skyLight 如何合并 → Cuberite 分别判 `<= N` 等价于 max ≤ N，**可接受但未经证实**。
4. **S6.14 蝙蝠光照 / S6.16 狼包大小 / S6.4 女巫包大小 / S6.6 沼泽史莱姆 Y**：四处**源冲突**，均需实测或对上游 issue 佐证才能定值。本机无 vanilla oracle（`AGENTS.local.md` §1）→ 一律先按主源取值并在代码注释里标冲突。
5. **S6.6 沼泽史莱姆月相式**：主源给「chance of failure based on the phase of the moon」，Slime 页给两步随机判定；Cuberite 的 `|phase−4|/4` 等价性未证明。
6. **创造/旁观玩家是否计入 cap 与生成范围**：无来源。Cuberite 现状是把观察者排除在 24 格检查外（S5.4），维持现状并标注。
7. **所有标〔快照待复核〕的单生物页数值**：须按文中给出的时间戳重取原文核对后方可作为验收依据。
---

## 15. 版本差异护栏（禁止从现行 wiki 误移植）

补齐 §2/§3 时最可能的失败模式是「照现行页面实现」——那些条目在 1.12.2 上并不成立，一律**不得**实现：

| 现代表述 | 引入版本 | 1.12.2 正解 | 依据 |
|---|---|---|---|
| **per-player mob cap**（每玩家独立上限） | **1.18 实验快照 3** | 单一全局上限，按 17×17 区块数缩放 | 现行 [Mob spawning](https://minecraft.wiki/w/Mob_spawning) History；主源 `Mob cap` 节无此概念 |
| **blockLight 必须为 0** | 1.18（21w43a） | 光照 ≤7 + 随光照递增的随机失败 + 日照最多额外 50% | 同上 History 条目；主源 `Spawn conditions` |
| 每类别每轮 **≤3 次** pack 尝试 | 现版 | **每合格区块 1 次**（S2.2） | 主源 `Pack spawning` |
| **water-ambient / axolotl / underground-water-creature** 类别 | 1.13（鱼）、1.17 | 仅 4 类（§0） | 主源 `Mob cap` |
| 高光照怪物清除阈值降到 **10 秒** | 现版 | 恒 30 秒（S12.1） | 现行页 History |
| 鱿鱼 Y **50–63**、2–4 只、限河海 | 现版 | Y **46–62**、4 只、任意群系 | [Squid 快照](https://web.archive.org/web/20170516012340/https://minecraft.gamepedia.com/Squid) |
| 友好生物「下方方块」按生物分表 | 现版 | 一律草方块（哞菇菌丝、豹猫草或叶） | 主源 `Spawn conditions` |
| `doMobSpawning` 管辖围城/幻翼/巡逻 | 现版 `spawn_mobs` | **不管辖围城** | 主源 |
| 下方方块光照 ≥14 不可生成 | 现版 | 无此规则 | 主源 `Spawn conditions` 无该项 |

另注：`8×8×3` 与「9×9 水平范围」是 S9.2 同一事实的两种表述——来源原文为「within an 8×8×3 block volume … **centered on the center of the block**, effectively meaning mobs can spawn in a 9×9 area」，两者不构成冲突。

---

## 16. 群体内变体与附带产出（后续分支用）

**S16.1** 变体概率：沙漠生物群系中露天僵尸 **80% 替换为尸壳**；非尸壳僵尸 **5% 僵尸村民**；僵尸各变体 **5% 婴儿**；婴儿僵尸 **5% 鸡骑士**；蜘蛛 **1% 骑乘者（蜘蛛骑士）**；下界骷髅 **80% 替换为凋灵骷髅**；僵尸猪人 **5% 婴儿**，婴儿猪人 **5% 猪人骑士**；豹猫生成时 **1/7 附带 2 只幼崽**。
来源：[Zombie 20170515142433](https://web.archive.org/web/20170515142433/https://minecraft.gamepedia.com/Zombie)、[Skeleton 20170425141452](https://web.archive.org/web/20170425141452/https://minecraft.gamepedia.com/Skeleton)、[Zombie_Pigman 20170125211808](https://web.archive.org/web/20170125211808/https://minecraft.gamepedia.com/Zombie_Pigman)、[Ocelot 20170101075941](https://web.archive.org/web/20170101075941/https://minecraft.gamepedia.com/Ocelot)〔快照待复核〕。
**「群体中随机带幼崽」的统一规则属 1.15/19w37a → 1.12.2 无**，只按上表逐生物处理。
Cuberite：**全部未实现**（无婴儿/尸壳/鸡骑士路径；`mtZombieVillager` 在 `FamilyFromType` 有映射但 `ChooseMobType` 不可达）。凋灵骷髅以 `RandBool(0.6)` 近似 S6.10 的 80%（见 §6）。

**S16.2** 主世界下界门方块每次方块刻有小概率在其下方门框生成僵尸猪人：Easy **1/2000**、Normal **2/2000**、Hard **3/2000**，**光照与玩家距离均不阻止**。
来源：[Zombie_Pigman 快照](https://web.archive.org/web/20170125211808/https://minecraft.gamepedia.com/Zombie_Pigman)〔快照待复核〕。
Cuberite：`Blocks/BlockPortal.h:45` 调 `SpawnMob`，**无该概率表、无难度依赖**。

**S16.3** 史莱姆区块判定：以 `seed + x²·0x4c1906 + x·0x5ac0db + z²·0x4307a7 + z·0x5f24f ^ 0x3ad8025f` 派生种子后 `nextInt(10) == 0`，x/z 按 32 位整型回绕。
来源：[Slime 20170513082145](https://web.archive.org/web/20170513082145/https://minecraft.gamepedia.com/Slime)。
Cuberite：`IsSlimeChunk()` 已存在（`MobSpawner.cpp` 内调用），**其算法与本式是否等价未核** → 列为 S6.5 的前置核查项。

---

## 17. 主源多重快照

同一「Spawn」页取三份独立快照交叉比对，正文相关小节文本一致，可降低单点快照被编辑过的风险：
[2017-02-09](https://web.archive.org/web/20170209190207/https://minecraft.gamepedia.com/Spawn)、
[2017-06-01](https://web.archive.org/web/20170601/https://minecraft.gamepedia.com/Spawn)、
[2017-12-05](https://web.archive.org/web/20171205210747/http://minecraft.gamepedia.com/Spawn)、
[2018-10-03](https://web.archive.org/web/20181003010302/https://minecraft.gamepedia.com/Spawn)。
