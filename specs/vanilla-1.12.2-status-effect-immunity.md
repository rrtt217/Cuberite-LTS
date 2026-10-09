# S6 状态效果免疫表（Immunities）

分支：`feature/effects-immunity` ｜ 级别：P1 ｜ 前置：**S2**（复用其 `IsFireproof` 归一化结果） ｜ 编排见 [roadmap](vanilla-1.12.2-status-effects-roadmap.md)

## 1. 问题

免疫判定散落且不全。现状（全部在 `OnTick` 里就地写死）：

| 免疫 | 实现处 | 状态 |
|---|---|---|
| 不死族免疫 Regeneration | [EntityEffect.cpp:366-384](../src/Entities/EntityEffect.cpp) | ✅ |
| 不死族免疫 Poison | [EntityEffect.cpp:445-475](../src/Entities/EntityEffect.cpp) | ✅ |
| 蜘蛛/洞穴蜘蛛免疫 Poison | 同上 | ✅ |
| **凋灵骷髅免疫 Wither** | 无 | ❌ |
| **凋灵 / 末影龙免疫所有效果** | 无 | ❌ |
| 施加前拒绝（而非 tick 时静默不结算） | 无：效果仍被写入、仍广播图标 | ❌ |

后果：被凋灵骷髅互殴、或龙息云笼罩凋灵时，会持续掉血；玩家用喷溅凋零药水能伤到凋灵首领。
另一类不一致：免疫的效果**依然会加进 map 并广播图标**（[Pawn.cpp:209-212](../src/Entities/Pawn.cpp)），vanilla 是**施加即被拒绝**。

## 2. 行为规格（1.12.2）

来源：[Status effect](https://minecraft.wiki/w/Status_effect?action=render)（Immunity 段）：

- 不死族生物免疫 Regeneration 与 Poison；
- 凋灵骷髅在上述之外还免疫 Wither；
- 蜘蛛与洞穴蜘蛛免疫 Poison；
- 凋灵、末影龙免疫**所有**效果；
- 创造/旁观：**不属免疫**——效果照样上、图标照样显示，只是伤害被游戏模式吸收（本仓库的伤害吸收路径需实测确认，标 `[needs-check]`）。

## 3. 改动

1. 新增 `src/Entities/EffectImmunityRules.h`（纯逻辑，可测）：
   ```
   bool IsEffectApplicable(cEntityEffect::eType a_Type, eMonsterType a_MobType, bool a_IsUndead, bool a_IsBoss);
   ```
   表驱动：`{效果 → 免疫条件}`，把现有分散判定迁移进来，保证 S6 后**判定真源唯一**。
2. `cPawn::AddEntityEffect` 在插件钩子之后、写入 map 之前调用它 → **免疫即整体拒绝**（不广播、不进 map）。
   - 顺序约定：**插件钩子优先**（[`CallHookEntityAddEffect`](../src/Entities/Pawn.cpp)，见 [OnEntityAddEffect 钩子文档](../Server/Plugins/APIDump/Hooks/OnEntityAddEffect.lua)），保持插件仍可强推。
3. `cEntityEffectPoison/Regeneration/Wither::OnTick` 内原有的免疫分支改为**兜底 ASSERT/直接结算**（施加口已拦截），避免双重判定漂移；保留 `IsUndead()` 查询接口不变。
4. 凋灵/末影龙的「免疫一切」用 `cMonster` 的类型判定，不新增虚函数（避免为两个怪动类层次）。

## 4. 测试

`tests/Entities/EffectImmunityRulesTest.cpp`（用 `eMonsterType` 枚举，不构造实体）：
- 不死族 × {Regen, Poison} → 拒；不死族 × {Wither, Weakness, Speed} → 允许；
- 凋灵骷髅 × Wither → 拒；僵尸 × Wither → 允许；
- 蜘蛛/洞穴蜘蛛 × Poison → 拒；洞穴蜘蛛 × Slowness → 允许（节肢杀手路径回归）；
- 凋灵/末影龙 × 全部 23 种 → 拒；
- 玩家（非怪）× 全部 → 允许（免疫表不得误伤玩家）。

## 5. 已知偏差

- **[needs-check]** 创造/旁观模式下效果是否应被服务端拒绝：本 spec 判定「不应拒绝」（vanilla 会显示图标），故本分支不做游戏模式豁免；需实机确认伤害吸收路径后另议。
- **[needs-check]** 「凋灵免疫所有效果」在 1.12.2 的完整清单（现代 wiki 的免疫段含 1.14+ 新增条目，已按 1.12.2 存在的效果裁剪）。
- 免疫生效后，原本依赖「图标已上」的插件行为可能变化 → 记入交付说明（属修复，非回归）。
