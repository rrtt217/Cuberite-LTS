# S18 怪物状态效果持久化（chunk NBT `ActiveEffects`）

分支：`feature/effects-persistence-mob` ｜ 级别：P2 ｜ 前置：无（与 S17 平行，二者互不依赖） ｜ 编排见 [roadmap](vanilla-1.12.2-status-effects-roadmap.md)

## 1. 实测事实

**怪物效果同样不持久化**：区块实体 NBT 既不写也不读效果。

| 事实 | 位置 |
|---|---|
| 实体写入分发（`etMonster → AddMonsterEntity`） | [NBTChunkSerializer.cpp:170](../src/WorldStorage/NBTChunkSerializer.cpp)；玩家被显式跳过（`case cEntity::etPlayer: return;  // Players aren't saved into the world`，[:178](../src/WorldStorage/NBTChunkSerializer.cpp)） |
| 通用实体字段写入 | [NBTChunkSerializer.cpp:657](../src/WorldStorage/NBTChunkSerializer.cpp)（`AddBasicEntity`）——**无效果** |
| 怪物专用写入 | [NBTChunkSerializer.cpp:805](../src/WorldStorage/NBTChunkSerializer.cpp)（`AddMonsterEntity`）——**无效果** |
| 实体基础字段读取（`Pos/Motion/Rotation/Health|HealF`） | [WSSAnvil.cpp:3763-3816](../src/WorldStorage/WSSAnvil.cpp)——**无效果** |
| 怪物基础字段读取 | `LoadMonsterBaseFromNBT`，[WSSAnvil.cpp:3822](../src/WorldStorage/WSSAnvil.cpp)——**无效果** |
| 全仓库检索 | `ActiveEffects` **零命中**；仅 `CustomPotionEffects` 两处，属区域效果云（[WSSAnvil.cpp:1834](../src/WorldStorage/WSSAnvil.cpp) / [NBTChunkSerializer.cpp:707](../src/WorldStorage/NBTChunkSerializer.cpp)） |

后果：给僵尸上中毒后卸载/重载区块（或重启服务器），僵尸变成无效果状态；凋灵骷髅/苦力怕爆炸等场景在存档往返后丢失持续伤害。属**可观测的 vanilla 偏差**。

## 2. 规格与字段名

来源：[Effect](https://minecraft.wiki/w/Effect) 历史段（已核对原文）：1.20.5 之前 living entity NBT 的该列表名为 **`ActiveEffects`**，条目字段为 **`Id` / `Amplifier` / `Duration` / `Ambient`**（1.20.5 起重命名为 `active_effects` / `id` / `amplifier` / `duration` / `ambient`，且 `id` 变字符串）。
→ **本仓库基线 1.8–1.12.2，必须用旧名**：

```
ActiveEffects: LIST of COMPOUND { Id: BYTE/SHORT, Amplifier: BYTE, Duration: SHORT/INT, Ambient: BYTE }
```

- `Duration` = 剩余 tick（与玩家侧 [S17](vanilla-1.12.2-status-effect-persistence-player.md) 同语义）；
- `ShowParticles` / `ShowIcon`：**是否属 1.12.2 字段未在允许来源逐条确认** → 本分支**不写**（缺字段 vanilla 视为默认值，安全），并在注释标 `[needs-check]`；
- `Ambient` 写 `0`（本仓库无 ambient 通路，见 [S14](vanilla-1.12.2-status-effect-potion-metadata.md)），字段照写以保持与 vanilla 结构同构；
- `Id` 用数字 ID（1–23），与 [EntityEffect.h:11-37](../src/Entities/EntityEffect.h) 的 `eType` 取值一致。

## 3. 改动

1. 写侧：在 `AddMonsterEntity`（[NBTChunkSerializer.cpp:805](../src/WorldStorage/NBTChunkSerializer.cpp)）内，`a_Monster->GetEntityEffects()` 非空时写 `ActiveEffects` 列表。
   **不要**写进 `AddBasicEntity`：非活体实体（矿车/物品框等）没有效果，写了会污染无关 NBT。
2. 读侧：在 `LoadMonsterBaseFromNBT`（[WSSAnvil.cpp:3822](../src/WorldStorage/WSSAnvil.cpp)）内解析列表并 `a_Monster.AddEntityEffect(...)`。
   - 顺序：必须在 `LoadEntityBaseFromNBT` 之后调用（各 loader 现有写法即如此，例 [WSSAnvil.cpp:2418-2423](../src/WorldStorage/WSSAnvil.cpp)），确保 `AddEntityEffect` 期间血量/位置已就绪；
   - 施加时的插件钩子：`AddEntityEffect` 会走 `CallHookEntityAddEffect`（[Pawn.cpp:189-193](../src/Entities/Pawn.cpp)）→ 读档被插件否决是可接受的（语义与在线施加一致），但**不要**因此让整个区块加载失败：忽略被拒的条目即可；
   - `effNoEffect`/`Id` 越界（0 或 >23）→ 跳过该条并 `LOGWARNING`，不中断。
3. 广播：读档时实体尚未 spawn，`BroadcastEntityEffect` 无接收者（`ForClientsWithView` 走 chunk 客户端表，[Broadcaster.cpp:83-102](../src/Broadcaster.cpp)）→ 无需抑制；客户端可见性由 [S13](vanilla-1.12.2-status-effect-sync-snapshot.md) 的快照补齐（**未做 S13 前，重载区块后服务端有毒、客户端无图标**，记为已知偏差）。

## 4. 测试

- 复用 [S17](vanilla-1.12.2-status-effect-persistence-player.md) 的 `EffectSnapshotEntry` 概念，但走 NBT：`tests/Entities/EffectNbtTest.cpp` 对纯函数
  ```
  // src/Entities/EffectNbtRules.h
  void WriteActiveEffects(cFastNBTWriter & a_Writer, const std::vector<EffectSnapshotEntry> &);
  std::vector<EffectSnapshotEntry> ReadActiveEffects(const cParsedNBT &, int a_TagIdx);
  ```
  做写→解析→读回往返：条数、`Id`、`Amplifier`、剩余 `Duration` 一致；畸形条目（缺 `Id`、`Id`=0、`Duration`≤0）被跳过且不抛。
- **手工验证**：`/effect` 给怪物上毒 → 走远卸载区块（或重启）→ 回来确认继续掉血（交付说明附实录）。

## 5. 已知偏差

- 玩家侧走 JSON、怪物侧走 NBT，两条通道格式不同（各自沿用所在子系统的既有格式，属最小改动；统一格式不在本分支）。
- 未做 S13 时，重载后的效果无客户端快照 → 图标缺失（同一偏差在 [S13 §1](vanilla-1.12.2-status-effect-sync-snapshot.md) 已列）。
- `ShowParticles`/`ShowIcon` 缺失对 vanilla 客户端的实际影响未实测（`[needs-check]`）。
