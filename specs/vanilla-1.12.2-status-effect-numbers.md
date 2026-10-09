# S4 状态效果数值对齐（瞬间治疗量、饥饿耗尽速率）

分支：`feature/effects-numbers` ｜ 级别：**P0** ｜ 前置：无 ｜ 编排见 [roadmap](vanilla-1.12.2-status-effects-roadmap.md)

本分支只做**两处已被允许来源证实的数值错误**，同文件（[EntityEffect.cpp](../src/Entities/EntityEffect.cpp)）、同一提交、无接口变化。

## 1. 瞬间治疗量：`6 × 2ⁿ` → `4 × 2ⁿ`

现状（[EntityEffect.cpp:326-337](../src/Entities/EntityEffect.cpp)）：治疗与伤害共用 `6 * (1 << m_Intensity)`。

来源 [Potion](https://minecraft.wiki/w/Potion)：
- Potion of Healing：「Restores **4** health」；Healing II：「Restores **8** health (doubles with every additional level)」
- Potion of Harming：「Deals **6** health」/ II 为 12

→ **治疗应为 4 × 2ⁿ，伤害维持 6 × 2ⁿ**。现在共用常量导致治疗恒偏 1.5×。

改动：把 `cEntityEffectInstantHealth::OnActivate` 与 `cEntityEffectInstantDamage::OnActivate` 各自使用具名常量
`INSTANT_HEALTH_BASE_RECOVERY = 4`、`INSTANT_DAMAGE_BASE_DAMAGE = 6`（放 `src/Entities/EffectNumbers.h`），并保留「不死族反向」分支（[EntityEffect.cpp:326-337](../src/Entities/EntityEffect.cpp)）不动。

## 2. 饥饿效果的耗尽速率：`0.025` → `0.005` 每 tick 每级

现状（[EntityEffect.cpp:393-401](../src/Entities/EntityEffect.cpp)）：
```
// 0.5 per second = 0.025 per tick
a_Player.AddExhaustion(0.025 * (GetIntensity() + 1));
```
来源 [Hunger (effect)](https://minecraft.wiki/w/Hunger_(effect))：「Hunger increases food exhaustion by **0.005 × level per game tick**」，同页交叉佐证：「rotten flesh / raw chicken / husk 造成的饥饿在 30 秒内累计 3.0 耗尽」= 0.005/tick × level 1 × 600 tick ✓。

→ 现状是 vanilla 的 **5 倍**饥饿消耗。改动：常量 `HUNGER_EXHAUSTION_PER_TICK_PER_LEVEL = 0.005`，并更新那句过时注释。

## 3. 无需改动（已核对为正确）

| 项 | 代码 | 来源核对 |
|---|---|---|
| 再生间隔 `50/(amp+1)` | [EntityEffect.cpp:366-384](../src/Entities/EntityEffect.cpp) | Potion：Regen「1 HP every 50 ticks（II：25）」✓ |
| 中毒间隔 `25/(amp+1)`、不可致死 | [EntityEffect.cpp:445-475](../src/Entities/EntityEffect.cpp) | Potion：Poison「1 damage per 25 ticks, unable to kill」✓ |
| 凋零间隔 `40/(amp+1)`、可致死 | [EntityEffect.cpp:484-495](../src/Entities/EntityEffect.cpp) | [Wither Skeleton](https://minecraft.wiki/w/Wither_Skeleton)：1♥/2 s，10 s 共 5♥ ✓ |
| 瞬间伤害 6/12 | 同上 | Potion：Harming 6 / 12 ✓ |
| 药水基础时长（3600/1800/900、II 减半、Extended ×8/3、Splash ×3/4） | [EntityEffect.cpp:68-120](../src/Entities/EntityEffect.cpp) | 与 Potion 表一致（3:00 / 1:30 / 0:45）✓ |

## 4. 测试

- `src/Entities/EffectNumbers.h`：纯常量 + `InstantHealthAmount(a_Amplifier)` / `InstantDamageAmount(a_Amplifier)`（位运算翻倍，含 amplifier 上限保护：`1 << n` 在 n ≥ 24 会溢出，vanilla 以 Byte 传 amplifier ≤ 255 → 需要 `std::clamp` 或饱和取值，方案在 spec 评审时定并写进注释）。
- `tests/Entities/EffectNumbersTest.cpp`：
  - 治疗序列 4 / 8 / 16 / 32，伤害序列 6 / 12 / 24 / 48；
  - 治疗量 ≤ 玩家最大血量时不产生过热（配合 `cPlayer::Heal` 的既有夹紧）；
  - `HUNGER_EXHAUSTION_PER_TICK_PER_LEVEL × 600 == 3.0`（允许来源给出的 30 秒累计值，作为回归不变量）。
- 目录/CMake 做法同 [S1 §5](vanilla-1.12.2-status-effect-amplifier.md)。

## 5. 已知偏差 / 待核

- 瞬间治疗的「不死族反伤」与「不死族治疗」不受`dtPotionOfHarming` 护甲减免（[Entity.cpp:683-706](../src/Entities/Entity.cpp) 已排除）✓ 无需改。
- **[needs-check]** 河豚的 Poison/Nausea 等级、`/effect` 时长 0 的语义、龙息云的伤害模型：三者都需要 1.12.2 oracle 或更精确来源，本分支不动（见 roadmap §5）。
