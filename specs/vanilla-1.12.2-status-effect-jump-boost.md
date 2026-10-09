# S12 状态效果：Jump Boost（跳跃高度与摔落减免）

分支：`feature/effect-jump-boost` ｜ 级别：P2 ｜ 前置：无 ｜ 编排见 [roadmap](vanilla-1.12.2-status-effects-roadmap.md)

## 1. 问题与关键认知

Jump Boost 空转：`cEntityEffectJumpBoost` 无 `OnTick`/`OnActivate`（[EntityEffect.h](../src/Entities/EntityEffect.h)），信标 2 级可选（[BeaconEntity.cpp:77](../src/BlockEntities/BeaconEntity.cpp)）。

**服务端职责边界（本分支的规格基础）**：本仓库的移动是**客户端权威**（客户端上报位置，服务端只做合法性与摔落结算，见 [Pawn.cpp:393-440](../src/Entities/Pawn.cpp) 的 `FallDamageAbsorbed` 结算）。
因此「跳得更高」这件事**由客户端依据已同步的效果自行实现**，服务端需要做的是：
1. 把效果同步到客户端（链路已存在：[Broadcaster.cpp:270-277](../src/Broadcaster.cpp)）；
2. **在服务端结算摔落伤害时按等级减免**（这是唯一的服务端可算部分，也是可测部分）。

## 2. 行为规格（1.12.2）

来源：[Potion](https://minecraft.wiki/w/Potion)（已核对原文）：

- 跳跃提升 I：「Increases the jump height to **1.83 blocks** and reduces fall damage by **one block**.」
- 跳跃提升 II：「Further increases jump height to **2.51 blocks** and reduces fall damage by **two blocks**.」

来源：[Damage](https://minecraft.wiki/w/Damage)（已核对原文）：「**Jump Boost** - In addition to increasing jump height, it also reduces fall damage by **1 HP per level**.」

→ 摔落减免量 = `1 × level` 点（level = amplifier + 1）；「每级 1 格高度」与「1 HP」两条来源一致（1 格 ≈ 1 HP 的常规换算），取 **1 HP/级** 为唯一真源。

## 3. 改动

1. 新增纯规则头 `src/Entities/FallDamageRules.h`（或并入 S9/S10 的规则头集合）：
   ```
   float JumpBoostFallRelief(int a_Amplifier);      // = (amp + 1) * 1.0f
   ```
2. 落点在 `cPawn` 的摔落结算：[Pawn.cpp:426-440](../src/Entities/Pawn.cpp)（`FallDamageAbsorbed` 判定之后、`TakeDamage(dtFalling, ...)` 之前）把伤害减去减免量，钳到 ≥ 0。
   注意减免要**在摔落高度换算之后**做（vanilla 是「少算几格」而非「伤害打折」）；`dtFalling` 已被排除在护甲减免之外（[Entity.cpp:695](../src/Entities/Entity.cpp)）✓。
3. **不做**服务端跳跃高度模拟、不改 `m_NormalMaxSpeed`（`OnMove` 已按速度处理，速度不是跳跃高度）。
4. 若日后引入服务端位移校验（反飞/反跳），Jump Boost 需要在此处开白名单 → 在代码注释里指向本 spec。

## 4. 测试

`tests/Entities/FallDamageRulesTest.cpp`：
- 减免表：amp 0/1/4 → 1/2/5；
- 不变量：减免后伤害不为负；无 Jump Boost 时减免恰为 0（回归锚）；
- 与既有 `FallDamageAbsorbed`（水/梯子/鞘翅/史莱姆，[Pawn.cpp:393-414](../src/Entities/Pawn.cpp)）的组合：已被完全吸收的摔落不因减免路径产生负伤害。

## 5. 已知偏差

- 服务端**不校验**「玩家是否真能跳到那个高度」，故本分支后玩家可能借 Jump Boost 上到平时上不去的地方 —— 与 vanilla 表现一致（效果本就允许），但本仓库缺少反作弊校验这一更大议题仍在 roadmap §5。
- **[needs-check]** 1.83 / 2.51 格这类高度数值未被服务端使用，仅作规格记录。
