# 1.12.2 原版入服进度包解码记录（列表补充）

> 来源：ProtoProxy 日志 build/bin/Logs/Log_1792730526_5.log 的 `Advancements = 14177 bytes` 十六进制载荷；2026-10-23 的 vanilla 1.12.2 会话。解码脚本 build/tmp/decode_join.py（一次性，未入库）。

## 1. 包结构核对

- 逐字节解码**正好吞掉 14177 字节**：reset=true、mapping=41、removals=0、progress=41。
- 每条：Identifier；has_parent → Identifier；has_display → Display；criteria 名称数组；requirements 组；progress 每条件 achieved/date。
- 与 M0 协议规格的字段顺序一致（parent 在 has_parent 之后、display 之前）。

## 2. 可见子集构成

- 41 条 = **20 条有 display** + **21 条 recipes（无 display）**。
- 有 display 的 20 条：story 3（root/mine_stone/upgrade_tools）、adventure 10（root + 9 个子节点）、husbandry 7（root + 6 个子节点）。
- recipes 的 21 条：parent 全部是 `minecraft:recipes/root`；root 无 display、criterion `impossible`。

## 3. Display 的关键事实

- title / description 是 **translate key JSON**，例如 `{"translate":"advancements.adventure.kill_a_mob.title"}`。
- icon 是 Slot（id/count/damage，无 NBT）；frame 线上值：0=task、1=challenge、2=goal。
- flags：根进度 = 1（有 background、无 toast），非根 = 2（show_toast）。
- 根背景：story=stone、husbandry=husbandry、adventure=adventure。
- 坐标示例：story/root (0.0,1.25)、story/mine_stone (1.0,1.25)、story/upgrade_tools (2.0,1.25)；adventure/root、husbandry/root 各 (0.0,1.0)。
- 可见子集的帧推断核对：husbandry/bred_all_animals 与 balanced_diet = 1（challenge），adventure/summon_iron_golem、totem_of_undying = 2（goal），其余任务/根 = 0；与 wiki 的 bg 推断一致。

## 4. recipes 条目

- 20 个可见配方进度（另加 root）：wooden_button、wooden_sword、oak_planks、wooden_axe、cake、oak_wooden_slab、fence、wooden_hoe、crafting_table、bone_meal_from_bone、wooden_pickaxe、trapdoor、ladder、oak_stairs、wooden_shovel、fence_gate、wooden_pressure_plate、wooden_door、stick、sign。
- 每个配方的 criteria = `has_the_recipe` + 一个材料条件（has_planks/has_log/has_stick/has_bone/has_egg），requirements = 两者同组（AND）。
- 包内**不含 conditions 和 rewards**；条件需要按 `recipe_unlocked`/`inventory_changed` 撰写。
- 注意：本仓库 `Server/crafting.txt` 的配方名与 vanilla 的 recipe id 不完全一一对应（例如 oak_wooden_slab → oak_slab、trapdoor → oak_trapdoor、wooden_button/wooden_door/fence/fence_gate/sign/pressure_plate/bone_meal_from_bone 没有同名条目），接入 recipes 数据时需要一张映射表或补齐 crafting.txt 命名；否则 `recipe_unlocked` 的 recipe 条件无法命中。

## 5. 用途与缺口

- 20 条 + recipes 的 display/criteria/requirements/进度已拿到，可直接用于数据与列表核对。
- 其余 35 条主树（Nether 13、The End 9、Adventure 1、Husbandry 1、Minecraft 13）只有 wiki 列表（无坐标/flags/背景）；需要完整入服抓包（在原版服务器上 `/advancement grant @p everything` 后入服）或按树形图自行布局。
- 所有条目的 criteria conditions/rewards 都不在包里；需按 wiki 要求文本 + 触发器规格逐条撰写。
