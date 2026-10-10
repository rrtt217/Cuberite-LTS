# Vanilla 1.12.2 — 光照系统（三维度：Overworld / Nether / End）

来源（cleanroom 白名单）：

- [Minecraft Wiki — Light（2018-11-24 修订版，1.14 光照引擎重写之前）](https://minecraft.wiki/index.php?title=Light&oldid=1287694)（**本规格的主源**：1.12.2 时点的 block light 传播、天光散布、滤光方块表、发光方块表、Nether/End 天光行为、月光 4）
- [Minecraft Wiki — Light（现行版）](https://minecraft.wiki/w/Light)（16 级定义、`max(sky light, block light)`、internal light level 概念、Java 版天光滤光 -1 清单；用于交叉核对 1.14 重写**未改动**的部分。1.14/1.18 改动——方向性不透明度、hostile 刷怪 block light 须为 0——**不适用**于 1.12.2，见 §6）
- [Minecraft Wiki — Daylight Detector（2018-05-31 修订版）](https://minecraft.wiki/index.php?title=Daylight_Detector&oldid=1217335)（阳光传感器输出表、反转模式 15−光照、天光感知）
- [wiki.vg — Protocol（1.12.2 期，2017-07-29 修订 1287）](https://wikivg.booky.dev/index.php?title=Protocol&oldid=1287)（Chunk Data：**只为 Overworld 区块柱发送 skylight**）
- 上游实现：`src/LightingThread.{h,cpp}`、`src/BlockInfo.cpp`、`src/Chunk.cpp`、`src/World.cpp`、`src/MobSpawner.cpp`、`src/Protocol/ChunkDataSerializer.cpp`、`src/WorldStorage/NBTChunkSerializer.cpp`

本文只写规格与 Cuberite 对照，不含新实现。凡 1.12.2 适用性无法从允许来源直接确认的数值，标 **〔待核〕**；无来源只能合理的，标 **〔推测〕**。

---

## 0. 当前 Cuberite 实现状态（基线 `master` @ `3faae00`）

光照**引擎**（`src/LightingThread.{h,cpp}`）：

| 项 | 位置 / 行为 |
|---|---|
| 整块重算：对目标区块读入 3×3 区块数据，flood fill 算出中间区块的光照 | `LightingThread.cpp: LightChunk`、`ReadChunks` |
| 天光种子：按高度图自上而下走透光且非天光散布列，列内填 15，柱底作种子 | `LightingThread.cpp: PrepareSkyLight` |
| 方块光种子：遍历所有发光方块作种子并预置自身光值 | `LightingThread.cpp: PrepareBlockLight` |
| 传播：向 6 邻域扩散，衰减量取**目的方块**的 falloff | `LightingThread.cpp: CalcLightStep`、`PropagateLight` |
| 结果压缩成 nibble 并写回区块（标记 light valid） | `LightingThread.cpp: CompressLight`、`Chunk.cpp: SetLight` |
| 按需触发：区块无光时排队重算；方块变更改变光值/衰减/透明性时使光照失效 | `ChunkMap.cpp: PrepareChunk`、`Chunk.cpp: SetMeta`（1354–1359） |
| 协议发送：block light 恒发；skylight 仅主世界发送 | `ChunkDataSerializer.cpp:303/361/422/588` |

光照**消费者**：

| 项 | 位置 |
|---|---|
| 刷怪光照判定（含天光随时间变暗、End 天光强制 0） | `src/MobSpawner.cpp:90-102` |
| 时间变暗 `m_SkyDarkness`（黄昏/夜晚/黎明，0–8） | `src/World.cpp: UpdateSkyDarkness` |
| 苗木/植物生长用 `max(blockLight, 天光−变暗)` | `src/Blocks/BlockSapling.h`、`BlockPlant.h` |
| 末影水晶/床/雪/冰等按维度判定（非光照） | `src/Blocks/*` |

---

## 1. 光照等级与传播（1.12.2 引擎）

**〔来源：[Light 2018-11-24 修订](https://minecraft.wiki/index.php?title=Light&oldid=1287694) §Spread / §Light-filtering blocks；现行版 §Light level（16 级定义未变）〕**

1. **16 级**：整数 0（最小）– 15（最大）。
2. **方块光**：来自发光方块（§2），flood fill 传播；每沿一轴走 1 米（曼哈顿/taxicab 距离）衰减 1——火把 14 放地上，四邻 13，对角 12（14−1−1）。
3. **天光**：垂直暴露于天空的方块天光 = 15；之后按 flood fill 扩散。
   - 天光 **15** 直下穿过非滤光方块**不衰减**；水平/向上传播减 1。
   - 天光 **<15** 时与方块光相同：向 6 邻域传播逐格衰减到 0。
   - 穿过滤光方块（§1.1）时不适用上述两条，按该方块的衰减量衰减。
4. **不透明方块**阻断传播；玻璃、铁栏杆等透明方块对光照**无额外影响**（照常 −1）。
5. **光照与时间无关**：天光数组**不随夜晚减小**——夜晚变暗是消费端（internal light，§4）的事。

### 1.1 衰减与滤光（Java 版，1.12.2）

基础衰减恒为 −1（每个方块距离），**额外**衰减取决于穿过的方块：

| 方块 | 额外衰减 | 说明 |
|---|---|---|
| 空气、玻璃、铁栏杆等普通透明方块 | 0 | 照常 −1 |
| **水、冰** | −2（合计 −3） | 「partially transparent = 减光至少 3 级（正常传播 1 + 额外 2），如水或冰」；且在柱顶散布天光 |
| **树叶、蜘蛛网** | 0（不额外衰减方块光） | 但**散布天光**：柱内光照中断，叶下光照按光源式衰减 |
| **耕地、台阶、楼梯** | Java 版视同不透明（完全阻挡） | 1.12.2 光照引擎不感知方块形状 |
| 其他所有不透明方块 | 完全阻挡 | |

**天光散布（leaves/cobweb/water/ice 通用，Java）**：柱顶最高障碍物上方的空气柱内天光保持满值；放置叶/网/水/冰会切断光照柱，其**最低上方空气块**如同普通光源向下照——叶下光照按光源式逐格衰减（如用不透明 1×1 烟囱 + 顶部叶控制此效应）。

### 1.2 Cuberite 对照

| vanilla 规则 | Cuberite 落点 | 一致性 |
|---|---|---|
| 16 级、flood fill、taxicab 衰减 | `CalcLight`/`CalcLightStep`/`PropagateLight` | ✅ |
| 衰减取所穿方块的额外量（含水/冰 −3） | `GetSpreadLightFalloff`：水/冰 = 3，叶/网/普通透明 = 1，默认 15 | ✅（falloff 1 = 「照常 −1 无额外」，falloff 15 = 阻断） |
| 天光散布方块：叶、网、水、冰 | `IsSkylightDispersant`：叶/网显式 + `falloff > 1`（即水/冰） | ✅ |
| 耕地/台阶/楼梯完全阻挡 | 不在 falloff-1 列表 → 默认 15 | ✅ |
| 天光 15 直下不衰减、水平/向上 −1 | `PrepareSkyLight`（列内 15 + 柱底种子，种子之后按普通传播） | ✅（近似：列内填充按高度图而非完整直下链，见 §5 偏差 5） |
| 光照与时间无关 | `m_SkyLight` 数组不变，消费端 `GetTimeAlteredLight` | ✅ |

---

## 2. 发光方块（1.12.2 光值表）

**〔来源：[Light 2018-11-24 修订](https://minecraft.wiki/index.php?title=Light&oldid=1287694) §Sources of light / Blocks。1.13+ 方块（海泡菜、灯笼、潮涌核心等）不适用〕**

| 光值 | 方块 | Cuberite `GetLightValue` |
|---|---|---|
| 15 | 信标、末地折跃门、末地传送门、火、萤石、南瓜灯、岩浆、红石灯（亮）、海晶灯 | 信标/末地传送门/火/萤石/南瓜灯/岩浆/红石灯/海晶灯 ✅；**末地折跃门缺**（§5 偏差 1） |
| 14 | 末地烛、火把 | ✅ |
| 13 | 燃烧的熔炉 | ✅ |
| 11 | 下界传送门 | ✅ |
| 9 | 发光的红石矿石（被触碰时） | ✅ `E_BLOCK_REDSTONE_ORE_GLOWING` |
| 7 | 末影箱、红石火把（亮） | ✅ |
| 3 | 岩浆块 | ✅ |
| 1 | 酿造台、棕色蘑菇、龙蛋、末地传送门框架 | ✅ |

**1.12.2 不发光（Java 版）**：红石比较器（两个时代的 infobox 均 `light = No`，仅基岩版通电时 7）、红石中继器（infobox `light = No`；两个时代的 Light 光源表均未列入）。Cuberite 给两者光值 **9**——§5 偏差 2。

## 3. 三维度差异

**〔来源：[Light 2018-11-24 修订](https://minecraft.wiki/index.php?title=Light&oldid=1287694) §Brightness（Nether「sky lighting doesn't play a role since there is no source of sky light」、End「sky lighting wouldn't play a role even if there were a source」）；现行版 §Internal light level；[wiki.vg 1.12.2 修订 1287](https://wikivg.booky.dev/index.php?title=Protocol&oldid=1287) Chunk Data〕**

| 维度 | vanilla 1.12.2 行为 | Cuberite 对照 |
|---|---|---|
| **Overworld** | 有天光；`max(天光, 方块光)` 用于刷怪/植物判定；天光数组发给客户端 | ✅（`MobSpawner.cpp`、`ChunkDataSerializer.cpp:303/588`） |
| **Nether** | **无天光源**，天光不参与任何光照判定（internal sky light = 0，光照 = 方块光）；协议**不发** skylight | 消费端基本等效：协议不发 ✅；但**引擎与存档仍生成/保存天光 15**（§5 偏差 3）；个别下界怪的刷怪光照门槛仍查询天光（如末影人 `MobSpawner.cpp:186-201`），受偏差 3 影响的细节归各 mob spec 处理 |
| **End** | 无天光（即使有天光源也不参与亮度）；协议**不发** skylight | 协议不发 ✅；刷怪侧天光强制 0 ✅（fork `3faae00`）；引擎/存档仍生成天光 15（§5 偏差 3） |

---

## 4. 时间与天气变暗（internal light level）

**〔来源：[Light 2018-11-24 修订](https://minecraft.wiki/index.php?title=Light&oldid=1287694) §Other light sources（月光 4、雨天 12、雷暴 10）；[现行版 §Internal light level](https://minecraft.wiki/w/Light#Internal_light_level)（internal sky light = 15 − darkening，正午 15 / 午夜 4 的图表；1.12.2 的变暗概念自 alpha 即存在——「a "darkness" value is subtracted from the sky to create the effects of different times of day」）；[Daylight Detector 2018-05-31 修订](https://minecraft.wiki/index.php?title=Daylight_Detector&oldid=1217335)〕**

1. 天光数组本身不随时间变（§1.5）；游戏在**消费端**计算「内部天光」= `天光 − 变暗量`（darkening），变暗量最大 **11**（午夜晴天：15 − 11 = **4**，即月光 4）。
2. 雷暴正午：内部天光 10（变暗 5）；雨天正午：12（变暗 3）。
3. 黄昏/黎明渐变期间变暗量平滑过渡（现行版 internal sky light 图表：internal 7 晴天 Time↓ 对应 22,653–22,812、Time↑ 对应 13,188–13,347——即 Cuberite 所用 13187/22812 边界）。
4. 消费端用 `max(内部天光, 方块光)` 判定刷怪、植物生长、阳光传感器输出。
5. **其他维度无内部天光**（internal sky light = 0）：光照判定 = 方块光（§3）。

### 4.1 Cuberite 对照

`cWorld::UpdateSkyDarkness`（`World.cpp:2346-2371`）+ `cChunk::GetTimeAlteredLight`（`Chunk.cpp:1987-1992`）：

| vanilla 规则 | Cuberite | 一致性 |
|---|---|---|
| 变暗量最大 11（午夜内部天光 4） | `m_SkyDarkness` 最大 **8**（夜晚 13187–22812 恒 8 → 内部天光 7） | ❌ 偏差 6 |
| 黄昏（12000→13187）变暗 0→11 渐增 | 分支 `(TIME_NIGHT_START − TempTime) / 148`：12000 时 = 8，**递减**到 13187 时 = 0，夜始跳回 8 | ❌ 偏差 6（黄昏变暗方向反了，疑似上游笔误） |
| 黎明（22812→23999）变暗 11→0 渐减 | `(TIME_SUNRISE − TempTime) / 148`：22812 时 = 8 递减到 23999 时 = 0 | ✅（方向对，幅度见偏差 6） |
| 雨天正午内部天光 12、雷暴 10 | **无天气变暗**（`m_SkyDarkness` 只由时间驱动） | ❌ 偏差 7 |
| 13187/22812 = internal 7 的昼夜边界 | 同值边界 | ✅ |
| 末地无内部天光 | `MobSpawner.cpp:96` 强制 SkyLight = 0 | ✅ |

---

## 5. 偏差清单（Cuberite vs vanilla 1.12.2）

| # | 偏差 | 位置 | vanilla 行为 | 影响 |
|---|---|---|---|---|
| 1 | **末地折跃门无光值**（发光 15） | `BlockInfo.cpp: GetLightValue`（switch 内无 `E_BLOCK_END_GATEWAY`） | 末地折跃门发光 15 | 末地折跃门周围无光照；`specs/vanilla-1.12.2-end-gateway.md` 相关 |
| 2 | **比较器/中继器发光 9** | `BlockInfo.cpp:416/434`（`ACTIVE_COMPARATOR`/`REDSTONE_REPEATER_ON` = 9） | Java 版两者 `light = No`（仅基岩版比较器通电时 7） | 通电比较器/中继器向周围散光，vanilla 不会 |
| 3 | **Nether/End 生成并保存天光 15** | `LightingThread.cpp: PrepareSkyLight`（维度无关）+ `NBTChunkSerializer.cpp:1280-1286`（有 section 即写 SkyLight） | 下界/末地无天光，区块 SkyLight 全 0 | 存档格式与 vanilla 不一致；天光消费者（如僵尸猪人门槛）受影响 |
| 4 | **阳光传感器不输出红石信号** | `Blocks/BlockDaylightSensor.h`（仅右键切换正常/反转） | 按天光/时间输出 0–15 红石信号；反转模式输出 `15 − 天光`；方块光不激活 | 红石系统缺口（Cuberite 红石整体缺失）的相关项 |
| 5 | **天光直下链按高度图近似** | `PrepareSkyLight`（以高度图 + 散布判定确定「最后阳光柱底」，非逐方块直下传播） | 天光 15 逐方块直下不衰减 | 对直下链中的**非散布透明块**（如玻璃塔）结果一致；散布块（叶/网/水/冰）经种子传播已覆盖；已知近似 |
| 6 | **夜晚变暗量 8 而非 11；黄昏变暗方向反** | `World.cpp: UpdateSkyDarkness` | 变暗 0→11（午夜内部天光 4）；黄昏渐增 | 夜晚天光消费者（植物生长、末影人 15 天光判定等）与 vanilla 偏差；刷怪门槛 ≤7 恰好兼容 |
| 7 | **无天气变暗** | `World.cpp: UpdateSkyDarkness`（无天气输入） | 雨天正午 12、雷暴 10 | 雷暴日无刷怪增益；天色由客户端自行渲染 |
| 8 | **整块重算而非增量更新** | `LightingThread`（3×3 flood fill、按需重排） | vanilla 对方块变更做增量光照更新〔推测：未从允许来源直接核实〕 | 性能与精度折衷；单方块光照传播路径在 3×3 范围内结果一致 |

## 6. 不适用项（1.14+/1.18+ 改动，1.12.2 不具备）

- **方向性不透明度**（台阶/楼梯/耕地等形状感知）：18w46a 加入，1.14+。1.12.2 按方块整体判定（§1.1）。
- **hostile 刷怪 block light 须为 0**：1.18+。1.12.2 为光照 ≤7（max(内部天光, 方块光) ≤7）。
- **作物按 client light 判定**：18w06a 加入（1.13+）。1.12.2 用 internal light。

## 7. 验证方式（无实机 oracle，见 `AGENTS.local.md`）

1. **规格 + 单元测试**（若后续实现修复偏差 1/2/6）：
   - 光值表测试：遍历 `cBlockInfo::GetLightValue` 断言 §2 表（含末地折跃门 15、比较器/中继器 0）。
   - 传播测试：构造单区块场景（火把、树叶、水柱、玻璃塔），断言 flood fill 后的目标光值（taxicab 衰减、叶下光源式衰减、水 −3、玻璃塔直下 15）。
   - 变暗测试：`UpdateSkyDarkness` 各时段断言（正午 0、午夜 11、黄昏单调递增、黎明单调递减）。
2. **实机对照**（后续补）：合法 vanilla 1.12.2 服务端 + F3 调试屏「Client Light」对照同坐标光值；ProtoProxy 抓 Chunk Data 包核对 skylight 仅主世界发送。
3. **协议侧**：[wiki.vg 1.12.2 修订 1287](https://wikivg.booky.dev/index.php?title=Protocol&oldid=1287) Chunk Data 节已核对，Cuberite `ChunkDataSerializer` 一致。

