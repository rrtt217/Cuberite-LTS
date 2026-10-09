# S17 玩家状态效果持久化（JSON 存档通道）

分支：`feature/effects-persistence-player` ｜ 级别：P1 ｜ 前置：**S5**（合并/时长语义）、建议接 **S13** ｜ 编排见 [roadmap](vanilla-1.12.2-status-effects-roadmap.md)

> **对既有文档的一条更正**：本仓库**有**玩家落盘，只是**格式与原版不同**——JSON 而非原版 NBT `level.dat`/`players/*.dat`。
> 先前的判断「玩家实体完全不落盘」是**错的**，本节以实测代码为准。

## 1. 实测事实（落盘通道存在，但**不含效果**）

| 事实 | 位置 |
|---|---|
| `cPlayer::SaveToDisk()` 用 `Json::Value` 组装后写文件 | [Player.cpp:1940](../src/Entities/Player.cpp) |
| 字段清单：`position/rotation/inventory/knownItems/knownRecipes/equippedItemSlot/enderchestinventory/health/xpTotal/xpCurrent/air/food/foodSaturation/foodTickTimer/foodExhaustion/isflying/lastknownname/SpawnX/Y/Z/SpawnForced/SpawnWorld/enchantmentSeed/world/gamemode` | [Player.cpp:1977-2002](../src/Entities/Player.cpp) —— **无任何效果字段** |
| 落盘路径 `players/<UUID 前 2 位>/<UUID>.json` | [Player.cpp:2521-2531](../src/Entities/Player.cpp) |
| 读取在构造函数里：`LoadFromDisk()` | [Player.cpp:146](../src/Entities/Player.cpp) / 函数体 [:1810](../src/Entities/Player.cpp) |
| 写盘触发点：析构（登出）[:162](../src/Entities/Player.cpp)、死亡 [:882](../src/Entities/Player.cpp)、在线周期存 [:3271-3275](../src/Entities/Player.cpp)（`PLAYER_INVENTORY_SAVE_INTERVAL`） | 同文件 |

结论：**登出/周期存盘都不会保存效果** → 喝一瓶迅捷药水后重登，效果消失（vanilla 会保留）。这条偏差**可以只加字段修好**，不需要新建落盘体系。

## 2. 规格

- 语义来源（vanilla 保留效果跨重登）：[Effect](https://minecraft.wiki/w/Effect) 的历史段明确 living entity NBT 里有 `ActiveEffects` 列表（1.20.5 前为该名，条目字段 `Id`/`Amplifier`/`Duration`/`Ambient`，1.20.5 起改小写下划线名）→ 「效果属于玩家存档数据」是 vanilla 行为。
- **本仓库必须沿用自有 JSON 格式**（不与原版 `players/*.dat` 互操作，这一点已是既成事实：[SaveToDisk](../src/Entities/Player.cpp) 通篇 JSON）。
  → 新增顶层键 **`"effects"`**，数组元素字段名沿用本文件既有风格（小写、无下划线优先）：
  ```json
  "effects": [ { "id": 1, "amplifier": 0, "duration": 173, "ambient": false } ]
  ```
  - `duration` = **剩余 tick**（`GetDuration() - GetTicks()`），不是原始时长；
  - `ambient` 先落盘但暂无消费者（消费者是 [S14](vanilla-1.12.2-status-effect-potion-metadata.md)）；无 `Ambient` 通路时可写 `false`，但**字段要占位**以免日后格式变更；
  - 不保存 `hideParticles`：1.12.2 是否需要该字段 `[needs-check]`，先不落。
- **离线不倒计时**：vanilla 的 `Duration` 是「剩余 tick」，实体未被 tick 时不递减；本仓库同理（读回后原样续用）✓ 与 `Duration` 语义一致。
- **死亡清空优先于存盘**：`cPawn::KilledBy` 先 `ClearEntityEffects()`（[Pawn.cpp:131](../src/Entities/Pawn.cpp)），死亡存盘（[Player.cpp:882](../src/Entities/Player.cpp)）因此自然写空数组 ✓ 无需特判。

## 3. 改动

1. 写侧：`SaveToDisk` 内组装 `root["effects"]`，遍历 `m_EntityEffects`（`std::map` 已按 id 有序 → 输出稳定，避免 diff 抖动）。
   **`effNoEffect` 不落盘**（`AddEntityEffect` 已拒收，[Pawn.cpp:196-198](../src/Entities/Pawn.cpp)，防御性再过滤一次）。
2. 读侧：`LoadFromDisk` **只做校验 + 缓存**到新的私有成员 `m_LoadedEffects`，**不得在此处调用 `AddEntityEffect`**：
   - 原因：`LoadFromDisk` 在构造函数中被调（[Player.cpp:146](../src/Entities/Player.cpp)），此时 `SetMaxHealth(MAX_HEALTH)` 尚未执行（同函数 :147）、实体尚未进入世界、`BroadcastEntityEffect` 需要 `m_World` 与 chunk 视图；在这里施加会让 Health Boost 被随后的 `SetMaxHealth` 抹掉（[S11](vanilla-1.12.2-status-effect-health-boost.md)）、Speed 类效果被 `OnMove` 覆盖、并且广播发给空气。
   - 施加时机：`cPlayer::OnAddToWorld`（[Player.cpp:2993-2996](../src/Entities/Player.cpp)）里、`Super::OnAddToWorld` 之后逐条 `AddEntityEffect`（**与 [S13](vanilla-1.12.2-status-effect-sync-snapshot.md) 的快照天然合并成一次实现**：施加即广播）。
3. **不要**顺手给 JSON 加 `Air`/`OnGround` 之类原版字段名（本仓库已有 `air`，格式自洽优先）。
4. 兼容性：老存档无 `effects` 键 → 视为空数组（`Root.get("effects", Json::arrayValue)`），不报错、不踢人（读盘失败会踢人，见 [ClientHandle.cpp:430-434](../src/ClientHandle.cpp)）→ **解析必须容错**。

## 4. 测试

- `tests/Entities/EffectSerializationTest.cpp`：round-trip 纯函数
  ```
  // src/Entities/EffectSerialization.h
  Json::Value EffectsToJson(const std::vector<EffectSnapshotEntry> &);
  std::vector<EffectSnapshotEntry> EffectsFromJson(const Json::Value &);
  ```
  断言：剩余时长往返一致；`duration <= 0` 的条目读回即丢；缺键 → 空向量；乱序输入 → 输出按 id 升序；`amplifier` 越界（>255）被拒且不崩。
- **手工验证**（无 oracle，[AGENTS.local.md §1](../AGENTS.local.md)）：`/effect 1 60` → 登出 → 检查 `players/…json` 出现 `effects` → 重登 → 图标仍在且时长 ≤ 存盘值。这一步必须在交付说明里附实录。

## 5. 已知偏差

- 存档格式与 vanilla 不兼容（既有事实，非本分支引入）：原版客户端/工具读不了本仓库玩家档，本仓库也读不了原版 `players/*.dat`。要在同一分支解决属另一个议题（roadmap §5）。
- `ShowParticles`/`hideParticles` 不落盘（`[needs-check]`）。
- 区域效果云/抛掷物上的效果由 chunk NBT 走 `CustomPotionEffects`（[WSSAnvil.cpp:1834](../src/WorldStorage/WSSAnvil.cpp)、[NBTChunkSerializer.cpp:707](../src/WorldStorage/NBTChunkSerializer.cpp)），本分支不碰。
