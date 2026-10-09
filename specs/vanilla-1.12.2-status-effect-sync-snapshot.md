# S13 状态效果快照同步（登录 / 重生 / 换世界 / 开始被看见）

分支：`feature/effects-sync-snapshot` ｜ 级别：P1 ｜ 前置：无 ｜ 后继：S14 ｜ 编排见 [roadmap](vanilla-1.12.2-status-effects-roadmap.md)

## 1. 问题：只发「增量」，从不发「存量」

全仓库只有一处 `SendEntityEffect` 调用（[Broadcaster.cpp:270-277](../src/Broadcaster.cpp) ← [Pawn.cpp:210](../src/Entities/Pawn.cpp)），即**只在效果被施加的那一刻**发一次包；到期/移除走 `BroadcastRemoveEntityEffect`（[Broadcaster.cpp:504-511](../src/Broadcaster.cpp)）。
`cPawn::GetEntityEffects()`（[Pawn.cpp:583-600](../src/Entities/Pawn.cpp)）只被 Lua 使用 → **服务端从未把「当前身上的全部效果」整体告诉任何客户端**。

由此产生的四类可见错位（服务端有、客户端无）：

| 场景 | 代码事实 |
|---|---|
| 死亡重生 | 客户端清空效果表，服务端在 [Player.cpp:981](../src/Entities/Player.cpp) 只发 `SendRespawn(dim, true)`，不补发任何效果 |
| 换世界/换维度 | [Player.cpp:3133](../src/Entities/Player.cpp) `SendRespawn(DestinationDimension, false)`，同样不补发 |
| 玩家进入他人视野 | `cPlayer::SpawnOn`（[Player.cpp:3145](../src/Entities/Player.cpp)）/`cPlayer::OnAddToWorld`（[Player.cpp:2993](../src/Entities/Player.cpp)）只发 spawn + metadata |
| 实体开始被追踪 | 走 chunk 发送线程的 spawn 循环（[ChunkSender.cpp:252-277](../src/ChunkSender.cpp)）与跨 chunk 移动回调（[Chunk.cpp:802-805](../src/Chunk.cpp) `cMover::Added`），二者都只发 spawn |

注：**登录路径当下无事可做**——玩家 JSON 存档里没有效果字段（[roadmap §1.5](vanilla-1.12.2-status-effects-roadmap.md)），登录时身上本就空；但一旦 [S17](vanilla-1.12.2-status-effect-persistence-player.md) 让效果可跨重登，这条路径立刻变成必需，故列入本分支的挂钩点清单（不额外加代码，与重生/换世界共用同一个 `SendEntityEffectsTo`）。

## 2. 行为规格（1.12.2）

规格依据（协议文档）：1.8 `0x1D Update Entity Effect` 与 1.9+ `0x1E`/`0x30` 都是**单效果**包，**协议里不存在批量效果包** → vanilla 必然是在「客户端开始得知该实体存在」之后**逐个补发**。
来源：[wikivg 镜像 Protocol rev 965 / rev 1064](https://wikivg.booky.dev/index.php?title=Protocol&oldid=1064)（1.8 的 Entity Effect 与 Remove Entity Effect 字段，已逐字段核对）。
**不存在**「Update Health 附带效果列表」的路径（同一修订里 `0x06` 只有 Health / Food / Food Saturation 三字段）→ 也不能指望血条包补。

## 3. 改动

1. 新增 `cPawn::SendEntityEffectsTo(cClientHandle & a_Client)`：遍历 `m_EntityEffects`，逐条发 `SendEntityEffect`，**顺序必须确定**（`std::map` 已按 id 有序 ✓，避免同一实体在不同客户端出现不同顺序）。
   `cPlayer` / `cMonster` 无需覆写；`cEntity::SpawnOn` 的非 pawn 分支不受影响。
2. 调用点（四处，全部为增量式改动）：
   - `cPlayer::SpawnOn`（[Player.cpp:3145](../src/Entities/Player.cpp)）：当 `a_Client` 是本玩家的客户端时补发自己（他视角度由 metadata 决定，不发别人身上的效果）；
   - `cPlayer::OnAddToWorld`（[Player.cpp:2993](../src/Entities/Player.cpp)）：换世界后补发；
   - 重生路径（[Player.cpp:981](../src/Entities/Player.cpp) 附近，`SendRespawn` 之后）；
   - **他人实体**：在 `cEntity::SpawnOn` 的 pawn 分支或 chunk 发送循环里，对 `IsPawn()` 的实体补发。落点二选一（实现时定，用注释固化）：
     a) `cMonster::SpawnOn` / `cPawn` 新增虚 `SpawnOn` 覆写 —— 改动集中在实体层；
     b) [ChunkSender.cpp:252-277](../src/ChunkSender.cpp) 循环内补发 —— 改动集中在网络层，但会绕过 `OnAddToWorld` 路径，需同时覆盖 [Chunk.cpp:802-805](../src/Chunk.cpp)。
     **推荐 a)**：与 `SendEntityEquipment` 的既有做法一致（例：[WitherSkeleton.cpp:58-62](../src/Mobs/WitherSkeleton.cpp) 就是在 `SpawnOn` 里补装备包）。
3. **广播过滤已就绪**：`ForClientsWithEntity`（[Broadcaster.cpp:83-102](../src/Broadcaster.cpp)）按 chunk 视图过滤，本分支不动它。
4. 时长语义：补发时用 `Duration - Ticks`（剩余值），**不是原始时长** —— 否则客户端会看到效果「续命」。

## 4. 测试

- **纯逻辑单测**（新增 `src/Entities/EffectSnapshotRules.h`）：
  ```
  struct EffectSnapshotEntry { int EffectID; int Amplifier; int RemainingTicks; };
  std::vector<EffectSnapshotEntry> BuildEffectSnapshot(...);   // 输入 (id, amp, duration, ticks) 序列
  ```
  `tests/Entities/EffectSnapshotRulesTest.cpp` 断言：
  - 剩余时长 = `Duration - Ticks` 且**不为负**（负值条目被丢弃）；
  - 输出按 effect id 升序（确定性）；
  - 空 map → 空向量（不发包）；
  - 0 剩余时长的条目被丢弃（回归 S5 里 `DistanceModifier = 0` 那类退化输入）。
- **包级验证**：`Tools/ProtoProxy` 抓取「进入他人视野 / 重生 / 换世界」三条路径的包序，确认 spawn 之后紧跟 N 条 `0x1D`（无 vanilla oracle → 只验包序与字段自洽，标为手工验证）。

## 5. 已知偏差

- 快照仍**不含 ambient/hideParticles**（协议 writer 恒写 0）→ 归 [S14](vanilla-1.12.2-status-effect-potion-metadata.md)。
- 玩家效果跨登录仍会丢（不落盘），本分支只解决「在线期间」的可见性。
- 大量效果同时补发（理论上 23 条上限）无分包风险：每效果一包，vanilla 同构。
