# S1 状态效果：amplifier 少算一级（Speed / Slowness）

分支：`feature/effects-amplifier-scaling` ｜ 级别：**P0** ｜ 前置：无 ｜ 编排见 [roadmap](vanilla-1.12.2-status-effects-roadmap.md)

## 1. 问题

`cEntityEffectSpeed` / `cEntityEffectSlowness` 用 **裸 amplifier** 做系数：

- [EntityEffect.cpp:240-283](../src/Entities/EntityEffect.cpp) —— 玩家 normal/sprint/fly 与怪物 walk speed 全部为 `± 0.2 * m_Intensity` / `± 0.15 * m_Intensity`
- [EntityEffect.cpp:286-330](../src/Entities/EntityEffect.cpp) —— 同上（Slowness）

而 `m_Intensity` 是 **amplifier**（0 = Level I，见 [Pawn.h:54-59](../src/Entities/Pawn.h) 的注释「a_EffectIntensity is the level of the effect (0 = Potion I …)」）。于是：

| 效果等级 | amplifier | 现在 | 应为 |
|---|---|---|---|
| Speed I | 0 | **+0.00（完全无效）** | +20% |
| Speed II | 1 | +20% | +40% |
| Slowness I | 0 | **−0.00（完全无效）** | −15% |

后果面很广：迅捷药水 I（最常见的形态）、缓慢药水、信标 Speed、**节肢杀手附魔的 Slowness IV**（[Entity.cpp:526](../src/Entities/Entity.cpp) 传 amplifier 4 → 现在只减 60%，应为 75%……见 §4.3）全部错位。

同仓库的正确写法可作对照：Haste 用 `GetIntensity() + 1`（[Player.cpp:2643](../src/Entities/Player.cpp)），Poison/Wither/Regen 用 `m_Intensity + 1`（[EntityEffect.cpp:450-480](../src/Entities/EntityEffect.cpp)）。所以这是**笔误级缺陷**，不是设计选择。

## 2. 行为规格（1.12.2）

来源：[Potion — Status effects given by potions](https://minecraft.wiki/w/Potion)

- Speed：Movement Speed **+20% per level**
- Slowness：Movement Speed **−15% per level**

即乘数 = `1 ± 0.2 × (amplifier + 1)` / `1 ± 0.15 × (amplifier + 1)`。

## 3. 现状机制与为何可直接按「加法」写

Cuberite 不用 vanilla 的属性系统，而是「相对速度」标量：

- 玩家基准 normal = 1.0、sprint = 1.3、fly = 1.0（[Player.cpp:124-126](../src/Entities/Player.cpp)），sprint 是 normal 的 1.3 倍 → 对 1.0 基准做 `+0.2 × level` 与对结果做 ×1.3 相互兼容（0.2 × 1.3 = 0.26，正是现有 sprint 系数）。
- 怪物用 `m_RelativeWalkSpeed`（[Monster.h:148-149](../src/Mobs/Monster.h)），同为相对倍率。

因此**保持加法、只把 amplifier 换成 level** 即可，不需要引入属性系统。

## 4. 改动

1. 新增纯规则头 `src/Entities/EffectSpeedRules.h`（风格照 [src/Mobs/EndermanBlockRules.h](../src/Mobs/EndermanBlockRules.h)）：
   - `EffectLevel(int a_Amplifier) -> int`（= amplifier + 1，带 `ASSERT` 不溢出）
   - `SpeedMaxSpeedDelta(a_Amplifier) -> double`（`+0.2 * level`）
   - `SlownessMaxSpeedDelta(a_Amplifier) -> double`（`-0.15 * level`）
   - `SprintDelta(a_Amplifier) -> double` / `FlyDelta(a_Amplifier) -> double`（= 对应 delta × 1.3；fly 是否乘 1.3 见 §6）
   具名常量替代魔法数（AGENTS §5 禁魔法数字）：`SPEED_SPEEDUP_PER_LEVEL = 0.2`、`SLOWNESS_SLOWDOWN_PER_LEVEL = 0.15`、`SPRINT_MULTIPLIER = 1.3`。
2. `cEntityEffectSpeed::OnActivate/OnDeactivate/OnMove`（[EntityEffect.cpp:240-283](../src/Entities/EntityEffect.cpp)）与 `cEntityEffectSlowness`（同文件）改为调用规则头函数；玩家分支保留现有「先复原基准再加成」的写法（现有 OnDeactivate 已按 1.0/1.3/1.0 复原，逻辑不变）。
3. 不改动 `OnMove` 里的蹲下分支（速度的蹲下处理与本分支无关，属 [待核] 项）。

## 5. 测试

新建目录 `tests/Entities/`（本仓库第一个实体类测试）：

- `tests/Entities/EffectSpeedRulesTest.cpp`：验证 §2 的乘数表（amplifier 0..4 → ±0.2/0.4/0.6/0.8/1.0、±0.15/0.30/0.45/0.60/0.75）、sprint = delta × 1.3、以及「Slowness IV（amplifier 3）仍能移动而非归零」。
- `tests/Entities/CMakeLists.txt`：照抄 [tests/Mobs/CMakeLists.txt](../tests/Mobs/CMakeLists.txt) 形式（`add_executable` → `target_link_libraries(... GeneratorTestingSupport)` → `add_test`）。
- 在 [tests/CMakeLists.txt](../tests/CMakeLists.txt) 的 `add_subdirectory` 列表按字母序插入 `add_subdirectory(Entities)`。
- 文件头注释按仓库惯例写清：验证哪条规格、不变量、为什么这样验（纯函数无需 world harness）。

## 6. 已知偏差 / [needs-check]

- **[needs-check]** 飞行速度与 sprint 是否共用 1.3 系数：允许来源只给出「+20% per level」，未拆分 sprint/fly 子速率；本分支沿用现有 1.3 结构，仅在规则头里把系数显式命名，便于日后单点修正。
- **[needs-check]** 怪物是否该用「乘法」而非「加法」（`+0.2 × level` 对 `m_RelativeWalkSpeed` 基准 1.0 等价于 +20%，但若 Monsters.ini 给了非 1.0 基准则不等价）。本分支保持加法并记为偏差。
- 蹲下速度（sneaking speed）在本仓库无对应通路 → 速度的蹲下分支仍缺，不在本分支。
