# S11 状态效果：Health Boost（最大血量）

分支：`feature/effect-health-boost` ｜ 级别：P2 ｜ 前置：**S10**（同文件相邻） ｜ 编排见 [roadmap](vanilla-1.12.2-status-effects-roadmap.md)

## 1. 问题

Health Boost 空转：`cEntityEffectHealthBoost` 是空类（[EntityEffect.h:503-514](../src/Entities/EntityEffect.h)），从不触碰 `m_MaxHealth`。
它没有生存内的正常来源（`/effect`、创造栏），但信标/插件可用，且 `/effect 21` 是管理员常用手段 → 现在表现为「图标亮了、血量没变」。

## 2. 行为规格（1.12.2）

来源：[Health Boost](https://minecraft.wiki/w/Health_Boost)（已核对原文）：

- 「It adds **4 maximum health per level**.」
- 「Unlike Absorption, the added hearts are **empty at first**, but can be healed through the usual methods…」
- 「Health of any entity is **capped at 1024 HP** and extra health provided by Health Boost can't exceed that…」（即 `maxHealth ≤ 1024`）
- 效果结束时：额外上限消失，当前血量若超过新上限则被夹到新上限。

## 3. 改动

1. 新增纯规则头（或并入 S9/S10 的 `src/Entities/DamageReductionRules.h` 同一目录下的 `src/Entities/MaxHealthRules.h`）：
   ```
   float HealthBoostBonus(int a_Amplifier);                 // 4 * (amp + 1)
   float ClampMaxHealth(float a_MaxHealth);                 // ≤ HEALTH_CAP_1024
   ```
   具名常量 `HEALTH_BOOST_PER_LEVEL = 4.0f`、`MAX_HEALTH_CAP = 1024.0f`。
2. `cEntityEffectHealthBoost::OnActivate`：`a_Target.SetMaxHealth(ClampMaxHealth(BaseMaxHealth + bonus))`；
   `OnDeactivate`：恢复基线并 `SetHealth` 夹紧（`cEntity::SetHealth` 已有 `Clamp(a_Health, 0, m_MaxHealth)`，[Entity.cpp:922](../src/Entities/Entity.cpp)）。
3. **基线上限的来源**：`cEntity` 现有 `SetMaxHealth`（[Entity.cpp:1944](../src/Entities/Entity.cpp)）直接赋值、无「基线」概念，玩家基线在 [Player.cpp:147](../src/Entities/Player.cpp) `SetMaxHealth(MAX_HEALTH)`。
   → 需在 `cPawn` 记录「效果前上限」（效果系统里保存旧值，效果移除时回滚），**不要用「基线 + bonus」的重复累加写法**（多来源/重复施加会漂）。这是本分支最容易做错的一点，单测必须锁住。
4. HUD：1.12 客户端按效果列表自行绘制血量上限，服务端无需新包（`SendHealth` 只送血量/饥饿，[Player.cpp:2920](../src/Entities/Player.cpp)）。

## 4. 测试

`tests/Entities/MaxHealthRulesTest.cpp`：
- `4 × level`：amp 0/1/21 → 4/8/88；
- 上限夹紧：基线 20 + amp 250 → 结果 ≤ 1024 且不溢出（`float` 精度下限用 `ASSERT`）；
- **回滚不变量**（本分支核心）：同一效果连续 OnActivate/OnDeactivate N 次后，上限必须回到基线（防「+bonus 累加」回归）；
- 与 S10 的交互：Health Boost 提升上限时吸收值不受影响。

## 5. 已知偏差

- 新增的容器是**空血**（需自然恢复/再生/瞬间治疗填充）✓ 由规格直接得出，实现时 `SetHealth` 不得同时抬当前血量。
- 最大血量**不该单独落盘**（vanilla 由效果推导）：本仓库玩家 JSON 里也没有该字段（[Player.cpp:1977-2002](../src/Entities/Player.cpp)）。[S17](vanilla-1.12.2-status-effect-persistence-player.md) 落地后，重登时由 `effects` 里的 Health Boost 重新推导上限 → **必须走 §3.2 的「基线 + bonus 且可回滚」写法**，否则会出现每次重登 +4 的累加漂移（§4 的回滚不变量同样覆盖这条）。
- 非玩家实体（怪物）默认上限来自 `Monster.ini`？—— 本分支只对 `cPawn` 生效，不做 `cMonster` 上限的配置化。
