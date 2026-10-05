# Vanilla 1.12.2 — End Gateway（末地折跃门）

来源：[End Gateway](https://minecraft.wiki/w/End_Gateway) · [End Gateway/Structure](https://minecraft.wiki/w/End_Gateway/Structure)。

每次击败末影龙，在主岛外环生成 **1** 个末地折跃门，最多 **20** 个。

## 生成

- **位置**：Y=75、距 (0,0) 96 格的 20 个固定点（wiki 表格，例如 (96,75,0)、(91,75,29)…）。
- **顺序**：由世界种子决定（vanilla 用其 PRNG；本实现用 `std::default_random_engine(worldSeed)` 洗牌 20 个索引，**与 vanilla 的排列不保证一致**——偏差）。
- **结构**（[End Gateway/Structure](https://minecraft.wiki/w/End_Gateway/Structure) 蓝图，以折跃门方块为中心）：
  - Y−2：单格基岩；
  - Y−1：十字形 5 格基岩；
  - Y  ：`E_BLOCK_END_GATEWAY`；
  - Y+1：十字形 5 格基岩；
  - Y+2：单格基岩。
  合计 12 基岩 + 1 折跃门方块。
- 触发：末影龙 200 tick 死亡动画结束时（`cEnderDragon::TickDeath`）调 `cWorld::SpawnEnderDragonGateway()`。
- 已生成的网关位置持久化在 `[EnderDragon] Gateways`。

## 偏差 / 后续

- 生成顺序 PRNG 与 vanilla 不同（仅影响“第几次击杀生成哪一个”）。
- **传送逻辑**（进入方块传送到外岛、首次激活生成回程网关、光柱）为后续增量。
