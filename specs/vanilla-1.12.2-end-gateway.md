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

## 传送

- 实体每 tick 在 `cEntity::DetectPortal()` 检查脚下方块；若为 `E_BLOCK_END_GATEWAY`，则传送到其**链接**的折跃门 +2 Y。
- **首次**生成回程网关（主岛网关诞生时即预生成）走 `cWorld::EnsureEnderDragonGatewayLink`：沿该网关方向距中心 **1024** 格处，
  先 `PrepareChunk` **加载目标区块**，再在回调里扫描该列最高非空气方块、在其上 **10** 格放置同样的网关结构；链接双向存储。
  关键：`cWorld::GetBlock`/`SetBlock` 对**未加载区块**直接返回空气 / 空操作，必须先加载区块，否则会扫不到地形、结构也放不下。
- 传送前若链接尚未生成（目标区块还在加载），**不传送**（避免进入虚空）；链接就绪后再次进入即可传送。
- 链接表 `m_EnderDragonGatewayLinks` 持久化到 `[EnderDragon] GatewayLinks`（`first>second` 坐标对）。
- 传送后设置 `m_PortalCooldownData.m_ShouldPreventTeleportation`，同一实体不会立刻来回弹。
- 主岛网关 ↔ 外岛回程网关双向互传。

## 偏差 / 后续

- 生成顺序 PRNG 与 vanilla 不同（仅影响“第几次击杀生成哪一个”）。
- 外岛网关位置用**简化算法**（固定距离 1024 + 扫描列高 + 10），未实现 vanilla 逐 16 格收缩/扩张选块的完整逻辑；目标是落在该方向的外岛上。
- 光柱（生成 200 tick 紫色、进入 40 tick、每 2400 tick）尚未实现。
- 自然生成于外岛的 `end_gateway_return`（回到末地平台）尚未实现。
