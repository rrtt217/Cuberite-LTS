# Vanilla 1.12.2 状态效果（Status Effect）修复编排

本文件是**编排索引**，不含实现。它把 2026-10-10 的状态效果全量审计结果拆成 **16 个可独立编译、可独立评审、可独立回滚** 的分支，每个分支对应一份 spec。
按 [AGENTS.md](../AGENTS.md) §3：**不做自动续跑**，本文件只是「列候选 + 写规格」，未获下达不开工。

- 审计范围：`cEntityEffect` 及其 23 种子效果、施加源（药水/食物/信标/怪物/附魔/命令）、协议下发与元数据。
- 判定口径：**已实现**=服务端有可观测逻辑；**空转**=有类、有图标、无任何逻辑读取；**偏差**=有逻辑但数值/时机/对象与 1.12.2 不符。

## 1. 关键事实（写作时全部实测）

1. 枚举完整：`cEntityEffect::eType` 覆盖 1.12.2 全部 23 种（[EntityEffect.h:11-37](../src/Entities/EntityEffect.h)），工厂为每种建类（[EntityEffect.cpp:190-221](../src/Entities/EntityEffect.cpp)）→ `/effect` 任意 ID 不崩。
2. 效果只存在于 `cPawn`（玩家+怪物），存储 [Pawn.h:101-102](../src/Entities/Pawn.h)，生命周期 [Pawn.cpp:186-259](../src/Entities/Pawn.cpp)。
3. 发包点唯一：`BroadcastEntityEffect`（[Broadcaster.cpp:270-277](../src/Broadcaster.cpp)）→ `cProtocol_1_8_0::SendEntityEffect`（[Protocol_1_8.cpp:506-516](../src/Protocol/Protocol_1_8.cpp)）；`GetEntityEffects()` 仅被 Lua 使用 → **无任何初始/快照同步**。
4. 包体编码**正确**：VarInt ID + Byte 效果 + Byte amplifier + VarInt 时长(tick) + Boolean 隐藏粒子，1.8 的 `0x1D`/`0x1E`（[Protocol_1_8.cpp:1908/1935](../src/Protocol/Protocol_1_8.cpp)）。已用 wiki.vg 迁移镜像的历史修订核对（1.8 时代即如此），**不是缺陷**。
5. **玩家实体确实落盘，但格式不是原版 NBT 而是 JSON**：`cPlayer::SaveToDisk`（[Player.cpp:1940](../src/Entities/Player.cpp)）写字段表（[:1977-2002](../src/Entities/Player.cpp)）到 `players/<UUID前2位>/<UUID>.json`（[:2521-2531](../src/Entities/Player.cpp)），读在构造函数的 `LoadFromDisk`（[:146](../src/Entities/Player.cpp) / [:1810](../src/Entities/Player.cpp)）。**该字段表里没有效果**；怪物侧同样不存效果（`ActiveEffects` 在全仓库零命中，实体写入 [NBTChunkSerializer.cpp:657/805](../src/WorldStorage/NBTChunkSerializer.cpp)、读取 [WSSAnvil.cpp:3763-3816](../src/WorldStorage/WSSAnvil.cpp) 均无效果）。→ 「重登/重载丢效果」是**可补的字段缺口**，见 S17/S18。
6. **测试基建**：现无任何效果相关测试；本仓库风格是「把可判定的纯逻辑抽成规则头 + 独立可执行测试」（见 [tests/Mobs/EndermanBlockRulesTest.cpp](../tests/Mobs/EndermanBlockRulesTest.cpp) 与 [src/Mobs/EndermanBlockRules.h](../src/Mobs/EndermanBlockRules.h)）→ 本批多数 spec 沿用「抽规则头 + 建 `tests/Entities/`」的做法。

## 2. 分支清单与顺序

| # | 分支 | spec | 级别 | 前置 | 一句话 |
|---|---|---|---|---|---|
| S1 | `feature/effects-amplifier-scaling` | [status-effect-amplifier](vanilla-1.12.2-status-effect-amplifier.md) | P0 | — | Speed/Slowness 少算一级：Level I 加成恰为 0 |
| S2 | `feature/effects-fire-resistance` | [status-effect-fire-resistance](vanilla-1.12.2-status-effect-fire-resistance.md) | P0 | — | `cPlayer::IsFireproof` 遮蔽基类实现，玩家防火无效 |
| S3 | `feature/effects-saturation` | [status-effect-saturation](vanilla-1.12.2-status-effect-saturation.md) | P0 | — | Saturation 漏调 `Super::OnTick` → 永不过期 |
| S4 | `feature/effects-numbers` | [status-effect-numbers](vanilla-1.12.2-status-effect-numbers.md) | P0 | — | 瞬间治疗 6→4×2ⁿ、饥饿耗尽 0.025→0.005 |
| S5 | `feature/effects-application-semantics` | [status-effect-application](vanilla-1.12.2-status-effect-application.md) | P1 | — | 重复施加合并规则 + `DistanceModifier` 误用 |
| S6 | `feature/effects-immunity` | [status-effect-immunity](vanilla-1.12.2-status-effect-immunity.md) | P1 | S2 | 免疫表集中化（凋灵骷髅/凋灵/龙） |
| S7 | `feature/beacon-power-duration` | [status-effect-beacon-powers](vanilla-1.12.2-status-effect-beacon-powers.md) | P1 | — | 信标时长应为 `9+2×level` 秒，现写死 9 秒 |
| S8 | `feature/effect-strength-weakness` | [status-effect-strength-weakness](vanilla-1.12.2-status-effect-strength-weakness.md) | P1 | — | 近战伤害 ±3/−4 每级（现为空转） |
| S9 | `feature/effect-resistance` | [status-effect-resistance](vanilla-1.12.2-status-effect-resistance.md) | P1 | S8 | 减伤 20%/级（现为空转） |
| S10 | `feature/effect-absorption` | [status-effect-absorption](vanilla-1.12.2-status-effect-absorption.md) | P1 | S9 | 吸收心（金苹果/图腾的吸收全废） |
| S11 | `feature/effect-health-boost` | [status-effect-health-boost](vanilla-1.12.2-status-effect-health-boost.md) | P2 | S10 | 最大血量 +2♥/级 |
| S12 | `feature/effect-jump-boost` | [status-effect-jump-boost](vanilla-1.12.2-status-effect-jump-boost.md) | P2 | — | 跳跃高度 +½ 块/级 + 摔落减免 |
| S13 | `feature/effects-sync-snapshot` | [status-effect-sync-snapshot](vanilla-1.12.2-status-effect-sync-snapshot.md) | P1 | — | 登录/重生/换世界/被开始看见时的效果快照 |
| S14 | `feature/effects-potion-metadata` | [status-effect-potion-metadata](vanilla-1.12.2-status-effect-potion-metadata.md) | P2 | S13 | Living 元数据的药水颜色/ambient 从不写 |
| S15 | `feature/mob-melee-effects` | [mob-melee-effects](vanilla-1.12.2-mob-melee-effects.md) | P1 | — | 凋灵骷髅「命中才上 Wither」（洞穴蜘蛛难度那半被难度系统缺口挡住，见其 §2） |
| S16 | `feature/core-effect-command` | [core-effect-command](vanilla-1.12.2-core-effect-command.md) | P2 | S1–S4；选择器那半依赖 **C1** | Core `/effect` 对齐（**submodule 特殊流程**） |
| S17 | `feature/effects-persistence-player` | [persistence-player](vanilla-1.12.2-status-effect-persistence-player.md) | P1 | S5，宜接 S13 | 玩家 JSON 存档加 `effects` 字段（重登保留效果） |
| S18 | `feature/effects-persistence-mob` | [persistence-mob](vanilla-1.12.2-status-effect-persistence-mob.md) | P2 | — | 怪物 chunk NBT 写读 `ActiveEffects`（重载区块保留效果） |

**跨系列交接**：命令层的 **C1**（[vanilla-1.12.2-command-target-selectors.md](vanilla-1.12.2-command-target-selectors.md)）接管 `/effect` 的目标选择器，本批 S16 只负责把 `cmd_effect.lua` 接上它，**不得**自建第二套选择器。
**串行约束**（同文件相邻区域，避免冲突）：S8 → S9 → S10 → S11；S2 → S6；S13 → S14；S5 → S17（S17 落「剩余时长」语义，依赖 S5 的时长/效力拆分）。
其余互不依赖，可任意顺序、可并行评审。**每个分支都必须自带测试**。

## 3. 空转效果一览（本批消灭的对象）

| 效果 | 现状 | 由谁解决 |
|---|---|---|
| Strength 5 / Weakness 18 | 无任何伤害读取 | S8 |
| Resistance 11 | `TakeDamage` 不查 | S9 |
| Absorption 22 | 无吸收心字段 | S10 |
| Health Boost 21 | 不接 `m_MaxHealth` | S11 |
| Jump Boost 8 | 不接跳跃/摔落 | S12 |
| Weakness 18 | 显式 TODO（[EntityEffect.cpp:427-436](../src/Entities/EntityEffect.cpp)） | S8 |

纯客户端效果（Nausea 9 / Blindness 15 / Night Vision 16）服务端本就无需逻辑，判定为**已实现**，仅需 S13/S14 保证可见性。

## 4. 统一验收门（AGENTS §4，改 `src/` 即六道全适用）

1. `cd src && lua CheckBasicStyle.lua` 0 违规
2. `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=DEBUG -DSELF_TEST=Yes -DBUILD_TOOLS=Yes && cmake --build build`
3. `cd build && ctest --output-on-failure -E "UrlClient-test|Google-test"`（新测试必须真被 `ctest` 跑到；总数以 `ctest -N` 为准）
4. 改 `src/Bindings/` 时：`cd src/Bindings && lua CheckBindingsDependencies.lua`
5. 逐条核对各 spec「行为规格」，记录证据（本机无 vanilla oracle → 以规格+单测为准，实机对照后续补）
6. 改导出 C++ 成员时：同步 `src/Bindings/AllToLua.pkg` + 手写绑定 + `Server/Plugins/APIDump/`，并跑 APIDump 自检三件套
7. 分支收口前 squash 成**一个提交**（S15 允许两个原子提交，理由写在该 spec）

## 5. 不排期项（越界或需前置）

| 项 | 原因 |
|---|---|
| 玩家存档格式与原版不兼容（JSON vs NBT） | 本仓库玩家档是 JSON（§1.5），要改成原版 `players/*.dat` 是存档体系改造，规模远超效果子系统；S17 只在既有 JSON 上补字段，不改格式 |
| 1.9+ 药水 `CustomPotionEffects` 模型 | 现状以 1.8 damage-bit 为唯一真源并在协议层翻名（[Protocol_1_9.cpp:1385-1480 / 1892-1950](../src/Protocol/Protocol_1_9.cpp)），重构成 NBT 模型是大改 |
| 难度系统（设置项 / 存档 / 协议 / 各怪取值） | 实测本检出**无难度状态**：[EnderDragon.cpp:35](../src/Mobs/EnderDragon.cpp) 注释「Cuberite has no difficulty setting」、[Protocol_1_8.cpp:834](../src/Protocol/Protocol_1_8.cpp) 登录包恒写 Normal、[WSSAnvil.cpp:96](../src/WorldStorage/WSSAnvil.cpp) 存档硬编码 Difficulty=2 → 挡住洞穴蜘蛛中毒难度（[S15 §2](vanilla-1.12.2-mob-melee-effects.md)）、怪物血量倍率与多种掉落 |
| 护甲 / 保护附魔的**减伤**未接入 | 实测：`GetArmorCoverAgainst` 的返回值只用于护甲耐久（[Entity.cpp:440-441](../src/Entities/Entity.cpp)），`GetEnchantmentCoverAgainst` 全仓库无调用点 → 属伤害系统缺口，S9 已在其空位上实现 Resistance，补齐护甲减伤时需回归 S9/S10 的插入次序 |
| 攻击冷却驱动的伤害倍率 | 全仓库无 `m_DamageCooldown`/`GetDamageCooldown`，属战斗系统缺口 |
| 河豚 Poison/Nausea 等级、龙息云伤害模型、`/effect` 无限时长语义 | 需 1.12.2 oracle 或更精确的允许来源，见各 spec `[needs-check]` |
| 玩家 NBT 呼吸附魔夜视（[Entity.cpp:1872](../src/Entities/Entity.cpp)）是否应存在 | 允许来源未直接佐证 1.12.2 呼吸附魔是否给夜视；S5 只修「`DistanceModifier=0` 使时长归零」这一确定性错误 |

## 6. 本次审计撤回的一条初判

初查时我认为 1.8 的 `SendEntityEffect` 用 VarInt 时长 + bool 是「协议格式错误（应为 Short）」。经 [wikivg 镜像 rev 965（2014-12）](https://wikivg.booky.dev/index.php?title=Protocol&oldid=965) 与 rev 1064（2015-10）核对，1.8 的 `0x1D` 就是 VarInt 时长 + Boolean 隐藏粒子 → **撤回该结论，无需改动**。教训已写进 S13/S14：协议类 spec 必须引用具体 oldid。
