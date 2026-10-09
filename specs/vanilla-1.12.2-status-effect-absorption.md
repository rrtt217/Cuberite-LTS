# S10 状态效果：Absorption（吸收心）

分支：`feature/effect-absorption` ｜ 级别：P1 ｜ 前置：**S9**（复用其规则头与插入位置） ｜ 后继：S11 ｜ 编排见 [roadmap](vanilla-1.12.2-status-effects-roadmap.md)

## 1. 问题

仓库内**没有吸收值状态**：`grep` 全仓库 `Absorb|m_Absorbed|absorptionHearts` 只命中统计枚举（[CustomStatistics.h](../src/Registries/CustomStatistics.h) 一类），从未被写入。
`cEntityEffectAbsorption` 是空类（[EntityEffect.h:520-531](../src/Entities/EntityEffect.h)）。
后果：**金苹果**（[ItemGoldenApple.h](../src/Items/ItemGoldenApple.h) 普通 Absorption I 2 分钟 / 附魔 Absorption IV 2 分钟）与**不死图腾**（[Pawn.cpp:138](../src/Entities/Pawn.cpp) `effAbsorption, 100, 1`，即 Absorption II 5 秒）给的都是一个**只有图标的假保护**。

## 2. 行为规格（1.12.2）

来源：[Absorption](https://minecraft.wiki/w/Absorption)（已核对原文）：

- 「Absorption adds **4 additional health points per level** … displayed as yellow hearts above the normal health bar.」
- 「If the entity takes damage while under this effect, **the absorption health points are depleted first**, followed by…」
- 效果消失（含到期）即失去吸收心；死亡清空（本仓库 `cPawn::KilledBy` 已 `ClearEntityEffects()`，[Pawn.cpp:131](../src/Entities/Pawn.cpp)）。

图腾三件套**已核对一致**：[Pawn.cpp:138-140](../src/Entities/Pawn.cpp) 给 `effAbsorption, 100 tick, amp 1` / `effRegeneration, 900, amp 1` / `effFireResistance, 800, amp 0`，与 [Totem of Undying](https://minecraft.wiki/w/Totem_of_Undying) 信息框「Absorption II (0:05)、Regeneration II (0:45)、Fire Resistance I (0:40)」逐项吻合 → 本分支只需让吸收真的生效，**不要改动这三处数值**。

## 3. 改动

1. **状态**：`cPawn` 新增 `float m_Absorption;`（私有，`GetAbsorption()` 只读导出，`AddAbsorption`/`SetAbsorption` 内部）。
   语义：吸收点是**独立于 `m_Health` 的缓冲**，不受 `m_MaxHealth` 约束（[Entity.cpp:922](../src/Entities/Entity.cpp) 的 `SetHealth` 夹紧不作用于它）。
2. **施加**：`cEntityEffectAbsorption::OnActivate` 调 `SetAbsorption(4 × level)`（**刷新而非叠加**，与「per level」一致）；`OnDeactivate`（到期/移除/清场）调 `SetAbsorption(0)`。
3. **消耗**：在 S9 建好的插入位置（[Entity.cpp:597](../src/Entities/Entity.cpp) 之前）按「先扣吸收、余下扣血」分配 `a_TDI.FinalDamage`：
   ```
   absorbed = min(absorption, damageAfterResistance); 剩余进 m_Health
   ```
   注意：吸收**完全吸收**伤害（不是百分比），且**不减免**`dtInVoid`/`dtStarving`？—— 允许来源未列吸收的例外 → `[needs-check]`，本分支取「与 Resistance 同一免除表」并注释。
4. **协议/HUD**：**无需改动**。黄色吸收心由客户端依据效果列表自行绘制（效果同步链路见 [roadmap §1.3](vanilla-1.12.2-status-effects-roadmap.md)）；本仓库 `Update Health` 只有 Health/Food/Saturation 三字段，1.12 亦无吸收字段。
5. **统计**：`DamageTaken`（[Player.cpp:2930](../src/Entities/Player.cpp)）应记减免后仍落到血量的部分 —— 与 S9 同一处决策，用注释固化。

## 4. 测试

`tests/Entities/AbsorptionRulesTest.cpp`（新增纯函数 `src/Entities/AbsorptionRules.h`：`AbsorptionFromAmplifier`、`SplitDamage`）：
- `4 × level`：amp 0/1/3 → 4/8/16；
- 分配不变量：`absorbed + toHealth == incomingDamage`、`absorbed ≤ 现有吸收`、吸收为 0 时等价于无效果（回归锚）；
- 边界：伤害恰好等于吸收 → 血量不变、吸收归 0；伤害 0 → 不动吸收；
- 「吸收不会把血量顶过 `m_MaxHealth`」由 `SetHealth` 的既有夹紧保证，测试用规则函数验证不产生负吸收。

## 5. 已知偏差

- 吸收心的**视觉**依赖效果图标同步；玩家重登/换维度时若未做快照（[S13](vanilla-1.12.2-status-effect-sync-snapshot.md)）会「有心无形」→ 属 S13 范围。
- **[needs-check]** 苦力怕爆炸/魔法伤害等来源是否被吸收（本分支按「全来源可吸收，除 Resistance 免除表」实现）。
- `SetAbsorption` 若需暴露给插件（金苹果类插件会用），则 AGENTS 门 6 适用（`AllToLua.pkg` + APIDump）；**本分支默认不导出**，只在 C++ 内使用。
