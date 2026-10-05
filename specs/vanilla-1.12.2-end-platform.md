# Vanilla 1.12.2 — 末地到达平台（End Arrival Platform）

来源：[End Portal](https://minecraft.wiki/w/End_Portal) · [The End](https://minecraft.wiki/w/The_End) ·
[cuberite PR #5609](https://github.com/cuberite/cuberite/pull/5609)。

玩家穿过末地传送门进入末地时，落在主岛之外 **(100, 50, 0)** 处的 **5×5 黑曜石平台**上，面朝**西**（看向主岛）。

## 行为

1. **每次**穿过末地传送门都重新生成平台（vanilla 行为）：
   - 平台：以 (100, 48, 0) 为中心、x/z 各 ±2 的 5×5 黑曜石（`E_BLOCK_OBSIDIAN`）。
   - 平台上方 **3 格**清空：空气跳过；**末地石**（说明平台生成在地下）直接挖掉、不掉落；其它方块作为掉落物弹出。
2. **落点**：玩家 (100, 49, 0)，yaw 90 / pitch 0（面朝西）；非玩家实体 (100.5, 50, 0.5)。
3. **世界出生点**移动到平台 (100, 49, 0)，使进入与重生都落在平台上。
4. 平台横跨 z=0 的区块边界，因此先 `PrepareChunk(6, -1)` 与 `PrepareChunk(6, 0)` 再放置方块。

## 实现

- `src/Generating/EndPlatform.{h,cpp}`：`cEndPlatform::Generate(cWorld *)`，用 `cChunkCoordCallback` 等两个区块就绪后放置。
- `cEntity::DetectPortal()`（主世界→末地）：调用 `cEndPlatform::Generate()`，按玩家/实体设置落点，玩家额外设 yaw/pitch。

## 偏差

- 平台坐标固定为 vanilla 的 (100, 48, 0)；未实现“平台被刻意破坏后不再生成”的边界情况——本实现每次进入都重铺。
