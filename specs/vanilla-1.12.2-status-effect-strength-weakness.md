# S8 状态效果：Strength / Weakness（近战伤害）

分支：`feature/effect-strength-weakness` ｜ 级别：P1 ｜ 前置：无 ｜ 后继：S9（同文件相邻，须串行） ｜ 编排见 [roadmap](vanilla-1.12.2-status-effects-roadmap.md)

## 1. 问题

Strength 与 Weakness 是**空转效果**：类建了（[EntityEffect.cpp:215/217](../src/Entities/EntityEffect.cpp)）、图标会亮、信标能选（[BeaconEntity.cpp:77](../src/BlockEntities/BeaconEntity.cpp)），但**没有任何一处伤害计算读取它们**。
近战入口是虚函数 `cEntity::GetRawDamageAgainst`（[Entity.cpp:634-666](../src/Entities/Entity.cpp)、声明 [Entity.h:322](../src/Entities/Entity.h)），被 [Entity.cpp:290](../src/Entities/Entity.cpp) 与 [EnderDragon.cpp:708](../src/Mobs/EnderDragon.cpp) 调用——里面只有锋利/节肢杀手/亡灵杀手。
Weakness 在工厂处即留有 `// TODO: Implement me!`（[EntityEffect.cpp:427-436](../src/Entities/EntityEffect.cpp)）。

## 2. 行为规格（1.12.2）

来源：[Potion](https://minecraft.wiki/w/Potion) 状态效果表（已核对原文）：

- Strength：**Melee damage +3 per level**
- Weakness：**Melee damage −4 per level**

即 `±(3 或 4) × (amplifier + 1)` 的**加法**修正（不是百分比）。
玩家侧还需一条 1.9+ 规则：攻击伤害随攻击冷却进度缩放；**本仓库没有攻击冷却系统**（全仓库无 `m_DamageCooldown` / `GetDamageCooldown`），故本分支按「满冷却」处理，并把冷却缺口记入偏差。

## 3. 改动

1. 新增纯规则头 `src/Entities/MeleeDamageRules.h`：
   ```
   int MeleeDamageEffectBonus(int a_StrengthAmplifier, int a_WeaknessAmplifier);
   // = 3*(strAmp+1) - 4*(weakAmp+1)，两者皆无时返回 0
   ```
   常量具名：`STRENGTH_DAMAGE_PER_LEVEL = 3`、`WEAKNESS_DAMAGE_PENALTY_PER_LEVEL = 4`。
2. `cPawn` 增加一个**内部**取值口（不导出 Lua）：
   `double cPawn::GetMeleeDamageBonus(void) const`，读 `GetEntityEffect(effStrength)/GetEntityEffect(effWeakness)`（[Pawn.cpp:597-601](../src/Entities/Pawn.cpp)）并调用规则头。
3. `cEntity::GetRawDamageAgainst` 在现有附魔修正之后、类型相性加成之后追加该修正（顺序：与 vanilla 一致的「基础武器伤害 → 附魔 → 效果」；**具体插入点在本分支实现时用注释固化**）。
4. 删除 [EntityEffect.cpp:427-436](../src/Entities/EntityEffect.cpp) 的 TODO 注释，替换为指向本 spec 的说明（Weakness 无需自己的 `OnTick`——它是被动修正）。
5. **不实现** Strength 对挖掘速度的影响（1.9+ Haste 才管攻速，见 [Potion](https://minecraft.wiki/w/Potion)：Haste =「Mining **and attack** speed +20% per level」），攻速缺口的归属在 roadmap §5。

## 4. 测试

`tests/Entities/MeleeDamageRulesTest.cpp`：
- 单独存在：Strength I/II/III → +3/+6/+9；Weakness I → −4、II → −8；
- 叠加：Strength II + Weakness I → +2；Strength I + Weakness II → −5；
- 不变量：两者皆无 → 恰为 0（保证 S8 对无效果实体**零行为变化**，是本分支的回归锚）；
- 下限：Weakness 高 Amplifier 时返回值允许为负 → 由调用点钳到最小 0（在规则头里 `std::max` 到 0，并断言「裸手永不因 Weakness 变成治疗」）。

## 5. 已知偏差

- **[needs-check]** 修正相对于护甲/难度缩放的先后顺序：允许来源只给「+3 per level」，未给管线顺序。实现时用注释标注并留在 `[needs-check]`，待 ProtoProxy/实机对齐。
- 攻击冷却未实现（roadmap §5）→ 1.9+ 玩家「连点降低伤害」的整体机制缺失，本分支不受影响。
- 凋零boss（[EnderDragon.cpp:708](../src/Mobs/EnderDragon.cpp)）调用 `GetRawDamageAgainst` 的路径同样会吃到修正——龙没有效果即返回 0，行为不变。
