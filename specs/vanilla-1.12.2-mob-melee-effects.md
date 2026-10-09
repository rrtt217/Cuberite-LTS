# S15 怪物近战施加效果的时机

分支：`feature/mob-melee-effects` ｜ 级别：P1 ｜ 前置：无 ｜ 提交：**单个提交**（洞穴蜘蛛那半被难度系统缺口挡住，见 §2） ｜ 编排见 [roadmap](vanilla-1.12.2-status-effects-roadmap.md)

## 1. 缺陷：凋灵骷髅无论是否命中都上 Wither

[WitherSkeleton.cpp:20-29](../src/Mobs/WitherSkeleton.cpp)：

```
bool cWitherSkeleton::Attack(std::chrono::milliseconds a_Dt)
{
    if (GetTarget() == nullptr) { return false; }
    GetTarget()->AddEntityEffect(cEntityEffect::effWither, 200, 0);   // 先无条件上效果
    return Super::Attack(a_Dt);                                        // 再决定这一刀是否真的打出去
}
```

对照同类的正确写法 —— 洞穴蜘蛛是 `if (!Super::Attack(a_Dt)) { return false; }` 之后再上毒（[CaveSpider.cpp:35-45](../src/Mobs/CaveSpider.cpp)）。
后果：凋灵骷髅**挥空 / 目标不在受击窗口内**时，玩家照样中 Wither I 10 秒（数值本身正确，只是时机错）。

**修复**：改成「先 `Super::Attack(a_Dt)`，返回 true 才施加效果」，与洞穴蜘蛛同构。效果数值不动：`200 tick / amp 0` = Wither I 10 秒，与 [Wither Skeleton](https://minecraft.wiki/w/Wither_Skeleton)（「Wither effect for 10 seconds… 1 HP every 2 seconds」）一致，且 [EntityEffect.cpp:484-495](../src/Entities/EntityEffect.cpp) 的 `40 / (amp + 1)` tick 结算 = 2 秒一次 ✓。

## 2. 缺陷二（**本分支不做**）：洞穴蜘蛛不分难度 —— 被前置系统阻塞

[CaveSpider.cpp:44-45](../src/Mobs/CaveSpider.cpp) 恒 `effPoison, 7 * 20, amp 0`，同处留有 `// TODO: Easy = no poison, Medium = 7 seconds, Hard = 15 seconds`。
规格（已核对）：[Cave Spider](https://minecraft.wiki/w/Cave_Spider) —— 简单 = 无毒、普通 = 7 秒、困难 = 15 秒。

**阻塞原因（实测）**：本检出**没有难度系统**，没有任何可读的难度状态可依赖：

| 证据 | 事实 |
|---|---|
| [EnderDragon.cpp:35](../src/Mobs/EnderDragon.cpp) | 注释直书「Cuberite has no difficulty setting」 |
| [Protocol_1_8.cpp:834](../src/Protocol/Protocol_1_8.cpp)、[:1289](../src/Protocol/Protocol_1_8.cpp)、[Protocol_1_9.cpp:2408](../src/Protocol/Protocol_1_9.cpp) | `// TODO: Difficulty (set to Normal)`，登录流程恒写 Normal |
| [WSSAnvil.cpp:96](../src/WorldStorage/WSSAnvil.cpp) | 存档把 `Difficulty` **硬编码为 2** |
| [BlockInfested.h:25](../src/Blocks/BlockInfested.h) | 同类待办「Add when difficulty is implemented」 |

按 [AGENTS.md §3](../AGENTS.md)「前置依赖规模 ≥ 本功能本身 → 拆成独立分支先行合并」：难度系统要动设置项、存档、协议与各怪取值，远大于本分支 → **洞穴蜘蛛的 TODO 保持原样**，缺口登记在 [roadmap §5](vanilla-1.12.2-status-effects-roadmap.md)。

## 3. 测试

命中判定抽成纯函数（沿用本仓库「规则头 + 纯函数测试」的做法，见 [tests/Mobs/EndermanBlockRulesTest.cpp](../tests/Mobs/EndermanBlockRulesTest.cpp)）：

```
// src/Mobs/MeleeEffectRules.h
/** 近战附加效果的唯一判据：只有真正命中（cMonster::Attack 返回 true）才施加。 */
bool ShouldApplyMeleeEffect(bool a_AttackLanded);
```

`tests/Mobs/MeleeEffectRulesTest.cpp`（`tests/Mobs/` 目录已存在，照 [tests/Mobs/CMakeLists.txt](../tests/Mobs/CMakeLists.txt) 追加一个 `add_executable` + `add_test`，并**并入已有的那一个** `set_target_properties`）：
- `a_AttackLanded == false` → 不施加（本缺陷的回归锁）；
- `a_AttackLanded == true` → 施加；
- 头文件注释写明契约：`cMonster::Attack` 的返回值是唯一可信的命中判据。

凋灵骷髅那处改动本身是会话级行为，本机无 vanilla oracle（[AGENTS.local.md §1](../AGENTS.local.md)）→ 证据为「代码路径审阅 + 注释指向本 spec」，交付说明中标注「未做自动化验证」。

## 4. 已知偏差

- 未处理「盾牌格挡 / 攻击冷却」对命中判定的影响：本仓库无 `m_DamageCooldown` / `GetDamageCooldown`（见 [roadmap §5](vanilla-1.12.2-status-effects-roadmap.md)）→ 修复后仍可能与 vanilla 在「格挡时是否上效果」上有差异，`[needs-check]`。
- 凋灵骷髅自身应对 Wither 免疫（属 [S6](vanilla-1.12.2-status-effect-immunity.md)，不在本分支）。
- 尸壳（Hunger）、远古守卫者（Mining Fatigue III）等 1.10+ 怪物本仓库未实现 → 属生物缺口。
