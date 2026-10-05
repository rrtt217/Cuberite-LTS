# Vanilla 1.12.2 通用区域效果云（Area Effect Cloud）

本文件描述 `cAreaEffectCloud` 实体的行为规格、协议与已知偏差。分支：`feature/entities-area-effect-cloud`。

## 1. 来源

- [Minecraft Wiki — Area Effect Cloud](https://minecraft.wiki/w/Area_Effect_Cloud)（重定向到 Lingering Potion#Creating area effect clouds）
- [中文 Minecraft Wiki — 区域效果云](https://zh.minecraft.wiki/w/%E5%8C%BA%E5%9F%9F%E6%95%88%E6%9E%9C%E4%BA%91)（行为细节更全）
- [中文 Minecraft Wiki — 区域效果云/ED](https://zh.minecraft.wiki/w/%E5%8C%BA%E5%9F%9F%E6%95%88%E6%9E%9C%E4%BA%91/ED)（实体 NBT 字段）
- [wiki.vg — Entities](https://c4k3.github.io/wiki.vg/Entities.html)（Spawn Object 类型 ID）

## 2. 行为规格

- 区域效果云是一个不可见的实体，玩家只能通过粒子看到它；包围盒为 `2.0 × Radius`（XZ）与 `0.5`（Y）。
- 实体数据：`Age` / `Duration` / `Radius` / `RadiusPerTick` / `RadiusOnUse` / `DurationOnUse` / `ReapplicationDelay` / `WaitTime` / `Color` / `Particle` / 状态效果列表。
- `Age` 每 tick +1；当 `Age > WaitTime + Duration`（或 `Duration <= 0` 且非 -1）时消散；`Duration == -1` 表示永不自然消散。
- 每 tick `Radius += RadiusPerTick`；半径 > `WaitTime` 之前不施加效果、粒子只出现在中心。
- 对云内每个生物施加效果，同一生物受 `ReapplicationDelay` 限制；每次成功施加后 `Radius += RadiusOnUse`、`Duration += DurationOnUse`。
- 方块不阻挡云的扩散与效果；云内任意位置效果相同。
- 非即时效果时长取普通药水的 1/4，即时效果（瞬间治疗/伤害/饱和）效力取 1/2；由调用方在 `AddEffect()` 时给出最终值（见偏差）。

### 生成来源

- 滞留药水：半径 3，30 秒内缩到 0，每次生效后半径 -0.5、剩余时长 -5 秒。
- 带效果的苦力怕爆炸：同上，效果取自苦力怕。
- 末影龙火球（JE）：命中方块爆炸，0.5 秒后生成半径 3、30 秒内扩到 5 的云，效果为瞬间伤害 II、紫色。

## 3. 协议

- Spawn Object 的类型 ID = **3**（`GetProtocolEntityType`）。
- 1.9–1.12.2 元数据：
  - 1.9：半径 5、颜色 6、单点效果 7、粒子 ID 8、粒子参数 9/10。
  - 1.10–1.12.2：以上各 +1（`Metadata*/AREA_EFFECT_CLOUD_*`）。
- 粒子以名称存储，经 `GetProtocolParticleID()` 转协议 ID（当前沿用 1.8 粒子表，默认 `mobspell` = 15）。

## 4. NBT

- 保存/读取 `Age`、`Duration`、`DurationOnUse`、`ReapplicationDelay`、`WaitTime`、`Radius`、`RadiusOnUse`、`RadiusPerTick`、`Color`、`Particle`、`CustomPotionEffects`（`Id`/`Duration`/`Amplifier`）。
- 实体名 `AreaEffectCloud` / `minecraft:area_effect_cloud`。

## 5. 验证

- `cd src && lua CheckBasicStyle.lua`：0 违规。
- `cmake --build build`：exit 0。
- `ctest -E "UrlClient-test|Google-test"`：26/26。
- 无自动化单测（测试框架不链接实体引擎）。

## 6. 已知偏差

1. 未导出 Lua 绑定/APIDump：插件暂时无法创建区域效果云；龙/滞留药水的 C++ 集成可 `World:AddEntity`（`cWorld::AddEntity`）。
2. 效果时长/效力缩放（1/4、1/2）不在实体内部做，由调用方在 `AddEffect()` 时给出最终值。
3. 粒子仍用 1.8 粒子表（`mobspell` 等），未按 1.9+ 重新编号；自定义粒子参数固定为 0。
4. 未实现“用玻璃瓶右键云获得龙息并让半径 -0.5”。
5. 尚未接入任何生成来源（滞留药水、苦力怕、龙火球）——本增量只做通用实体本身。
