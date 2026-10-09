# S9 状态效果：Resistance（伤害减免）

分支：`feature/effect-resistance` ｜ 级别：P1 ｜ 前置：**S8**（同文件相邻，须串行） ｜ 后继：S10 ｜ 编排见 [roadmap](vanilla-1.12.2-status-effects-roadmap.md)

## 1. 问题

Resistance 是空转效果：类建了（[EntityEffect.cpp:211](../src/Entities/EntityEffect.cpp)）、信标 2 级可选（[BeaconEntity.cpp:77](../src/BlockEntities/BeaconEntity.cpp)）、附魔金苹果给 5 分钟（[ItemGoldenApple.h](../src/Items/ItemGoldenApple.h)），但伤害管线完全不查它。

**管线现状（本分支的落点依据，全部实测）**：

- `cEntity::DoTakeDamage`（[Entity.cpp:419](../src/Entities/Entity.cpp)）：插件钩子（:433）→ 护甲耐久损耗（:440-441）→ 攻击方加成（暴击 :466-473、锋利 :481-485、亡灵杀手 :486-506、节肢杀手 :507-533、火焰附加 :535-567、荆棘 :574-592）→ **`m_Health -= a_TDI.FinalDamage;`（:597）**。
- **附带发现（记入 roadmap 待办，不在本分支修）**：`GetArmorCoverAgainst`（[Entity.cpp:789](../src/Entities/Entity.cpp)）的返回值**只**用于 `ApplyArmorDamage`（:440-441），护甲与保护附民的**减伤并未接入** —— `GetEnchantmentCoverAgainst`（[Entity.cpp:721](../src/Entities/Entity.cpp)）全仓库无调用点。故本分支的 Resistance 是本仓库**第一个真正生效的伤害减免**。

## 2. 行为规格（1.12.2）

来源：[Damage](https://minecraft.wiki/w/Damage)（已核对原文）：

- 「**Resistance** - Reduces most damage (including fall) by **20% per level**.」
- 「**Resistance** reduces damage from all sources.」

推导规则（本分支按此实现）：乘数 `×(1 − 0.2 × level)`，`level = amplifier + 1`，结果夹紧到 `[0, 1]`（level ≥ 5 ⇒ 免伤，与「20%/级」线性外推一致）。

**不减免的来源**（属「most」的补集，允许来源未逐条列举 → `[needs-check]`，本分支取保守集合并写进注释）：
`dtInVoid`（虚空）、`dtStarving`（饿死）、`dtPlugin`、自杀指令类。

## 3. 改动

1. 新增纯规则头 `src/Entities/DamageReductionRules.h`：
   ```
   double ResistanceMultiplier(int a_ResistanceAmplifier, eDamageType a_DamageType);
   // 不可减免来源返回 1.0；否则 std::clamp(1 - 0.2 * (amp + 1), 0.0, 1.0)
   ```
   常量 `RESISTANCE_REDUCTION_PER_LEVEL = 0.2`、`RESISTANCE_EXEMPT_DAMAGE_TYPES`（表驱动）。
2. 插入点：`cEntity::DoTakeDamage` 中，**在攻击方加成之后、`m_Health -=`（[Entity.cpp:597](../src/Entities/Entity.cpp)）之前**，对 `a_TDI.FinalDamage` 应用乘数。
   该位置在护甲耐久之后（护甲磨损不受 Resistance 影响 ✓）且在统计累计（:594 `DamageTaken`）之后 —— 若希望统计记录减免后的值，需把累计语句后移，**在本分支实现时用注释固化选择**。
3. 与 S10（Absorption）的次序：**先 Resistance、后 Absorption**（减免是伤害计算，吸收是伤害分配）→ S10 会复用本分支新增的头文件与插入位置。

## 4. 测试

`tests/Entities/DamageReductionRulesTest.cpp`：
- 乘数表：I→0.8、II→0.6、III→0.4、IV→0.2、V→0.0（且不会为负）；
- 无效果（读不到效果）→ 恰为 1.0（回归锚：对无效果实体零行为变化）；
- 免除来源（`dtInVoid` / `dtStarving` / `dtPlugin`）→ 1.0，即使 amplifier 很高；
- 可减免来源含 `dtFalling`（规格明列 including fall）→ 应用乘数。

## 5. 已知偏差

- **[needs-check]** 免除来源集合的边界（本分支用保守集合）。
- **[needs-check]** 减免与「护甲/保护附魔」的先后顺序：因本仓库尚无护甲减伤（§1 附带发现），顺序暂无从体现；护甲减伤补齐时需回归本文件。
- **[needs-check]** Resistance 与 `m_InvulnerableTicks`（[Entity.cpp:427-431](../src/Entities/Entity.cpp)）无交互（后者是硬免疫，优先），已确认无冲突。
- 怪物侧 Resistance 同样生效（插在 `cEntity` 基类），vanilla 亦如此。
