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
- **首次**生成回程网关（主岛网关诞生时即预生成）走 `cWorld::EnsureEnderDragonGatewayLink`：
  - 以 **1024** 格为默认距离，沿该网关方向生成候选距离序列（1024、1040、1008、1056、992… 逐 **16** 格，界于 **768–1280**，按离 1024 由近及远）；
  - 对每个候选先 `PrepareChunk` **加载目标区块**，在回调里扫描该区块最高非空气方块：**有高于 Y=15 的方块**即判定为陆地，在其上 **10** 格放置网关结构并双向记录链接；
    否则顺延到下一个候选；全部候选都无陆地时回退到 1024 处的默认高度。
  - 关键：`cWorld::GetBlock`/`SetBlock` 对**未加载区块**直接返回空气 / 空操作，必须先加载区块，否则会扫不到地形、结构也放不下。
- 传送前若链接尚未生成（目标区块还在加载），**不传送**（避免进入虚空）；链接就绪后再次进入即可传送。
- 链接表 `m_EnderDragonGatewayLinks` 持久化到 `[EnderDragon] GatewayLinks`（`first>second` 坐标对）。
- 传送后设置 `m_PortalCooldownData.m_ShouldPreventTeleportation`，同一实体不会立刻来回弹。
- 主岛网关 ↔ 外岛回程网关双向互传。
- **末影珍珠**：掷入折跃门方块时 `cThrownEnderPearlEntity::DetectPortal` 拦截——对**投掷者**激活该折跃门的传送（送到链接网关 +2 Y）并**移除珍珠**，
  不产生摔落伤害、末影螨等任何其它效果（对应 vanilla）。链接未就绪时珍珠继续飞行，不做错误传送。

## 外岛自然回归折跃门（end_gateway_return）

来源：[Minecraft Wiki — End Gateway](https://minecraft.wiki/w/End_Gateway)（By natural generation / History 1.11 16w39a）· [End Highlands](https://minecraft.wiki/w/End_Highlands) · 官方数据包 JSON（公开数据报告，见偏差）。

与上面「配对回程网关」不同，这是**独立于末影龙战斗**、随外岛地形自然生成的一类折跃门：

- **行为**（Java 1.11+，含 1.12.2）：随机自然生成于外岛（Java 版为 End Highlands 生物群系）；传送目的地是**黑曜石平台**（本实现 (100,48,0)）；通过它传送**不会**令黑曜石平台重新生成（这与末地传送门不同）。
- **放置规律**（数据包 `end_gateway_return` 的 placed_feature）：
  - `rarity_filter: chance = 700` → 每个区块 **1/700** 概率；
  - `in_square` → 在区块内随机取一列 (x, z)；
  - `heightmap: MOTION_BLOCKING` → y 取该列地表（本实现取最高非空气方块 Y **+1**）；
  - `offset y: uniform [3, 9]` → 再抬高 3–9 格；
  - `biome` → 仅限 End Highlands（本实现以「离 (0,0) 水平距离 > 1024」近似）。
- **结构**：与其它折跃门相同（12 基岩 + 1 折跃门方块）。
- **末影珍珠**：掷入自然回归折跃门方块时，同样把**投掷者**送到黑曜石平台并移除珍珠，无摔落伤害 / 末影螨。

## 偏差 / 后续

- 生成顺序 PRNG 与 vanilla 不同（仅影响“第几次击杀生成哪一个”）。
- 外岛网关的“找陆地”顺序为**离 1024 由近及远**，而 vanilla 是先由 1024 向 768 收缩找“无陆地”的边界、再由该处向 1280 扩张找“有陆地”；结果位置可能略不同，但都落在该方向的外岛上。
- 光柱（生成 200 tick 紫色、进入 40 tick、每 2400 tick）尚未实现。
- 自然回归折跃门的 placed_feature 数值来自**数据包格式**（1.13 起），1.12.2 为硬编码；**假设两者相同**（无法用 1.12.2 的允许来源独立核实，标为假设）。
- 每区块的“是否生成 / 选哪一列 / 抬高几格”由本实现的 `std::minstd_rand`（以世界种子 + 区块坐标混合播种，确定性、跨平台一致）驱动，与 vanilla 装饰器的 PRNG 不同；只保证分布与规则一致，不保证逐区块相同。
- Cuberite 的末地生成器没有原版生物群系，用「距离 > 1024」代替 End Highlands，且只在有陆地的列生成。
- 为保证 5×5 结构完全落在同一区块内（Cuberite 的 finisher 只能写当前区块），随机列限制在区块内 [2, 13]²；原版允许整块 [0, 15]²，故有效位置数约为原版的 56%。
