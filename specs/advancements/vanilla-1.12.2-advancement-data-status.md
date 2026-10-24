# 1.12.2 原版进度数据导入状态

> 分支 feature/advancements-vanilla-data（叠在 feature/advancements-vanilla-list 之上）。数据来源：M0 入服包解码（20 条有 display 的可见条目）+ wiki 列表（列表/父链/要求文本）。

## 已导入（55/55；20 条精确抓包 + 35 条撰写）

- story 3：root（stone 背景，0,1.25）、mine_stone（1,1.25）、upgrade_tools（2,1.25）。
- adventure 10：root、kill_a_mob（22 个 mob 条件）、kill_all_mobs（22 个）、sniper_duel、trade、adventuring_time（36 个 biome）、sleep_in_bed、summon_iron_golem、totem_of_undying。
- husbandry 7：root、breed_an_animal（10 个 mob）、bred_all_animals（10 个）、balanced_diet（35 个食物）、plant_seed（5 个作物）、break_diamond_hoe。
- display 字段来自抓包：translate key、icon id/data、frame、flags（roots=1、非 roots=2）、背景、精确 x/y；criteria/requirements 来自抓包；conditions 按 wiki 要求文本 + 触发器规格撰写。

## 实测

- 服务端启动 `Loaded 52 advancement(s)`（32 demo + 20 vanilla）；只有 adventuring_time 的 36 个 biome criterion 打 unsupported 警告（预期）。
- `advancement grant TestBot only minecraft:story/root` / adventure/root / husbandry/root 成功。
- 给 cobblestone → `minecraft:story/mine_stone` 完成（inventory_changed）。
- 用 bot 击杀僵尸 → `minecraft:adventure/root`（killed_something）与 `minecraft:adventure/kill_a_mob`（zombie 条件）都完成。

## 已知限制（这些条目已导入但条件/触发不完整）

- `adventure/adventuring_time`：36 个 biome 条件，location 匹配器暂不支持 biome → 全部 criterion 跳过。
- `adventure/trade`：villager_trade 触发器未实现 → criterion 永不触发。
- `adventure/summon_iron_golem`：summoned_entity 触发器目前只接 /summon 命令，铁傀儡搭建路径不触发。
- `adventure/sniper_duel`：只校验 skeleton 类型，没有 50 格/弓箭的 killing_blow 条件。
- `husbandry/balanced_diet`：item 名称直接用 criterion 名，个别食物名（cooked_cod/raw_cod 等）需与仓库物品名核对。
- `husbandry/plant_seed`：block 名直接用 criterion 名，作物方块名需与 ItemToString 核对。
- 所有条目都没有 rewards（抓包不含）。

## 下一步

1. 补齐剩余 35 条（Nether 13、The End 9、story 13、adventure 1、husbandry 1）：优先完整入服抓包（原版服务器 grant everything 后入服）；否则按 wiki 树形图布局 + 逐条撰写。
2. recipes 21 条（无 display）+ recipe_unlocked 名称映射。
3. biome/effects/entity 列表等谓词扩展；villager_trade/铁傀儡等触发路径。
4. rewards 逐条确认；旧成就桥；客户端实机（页签/树/公告）验证。

## 35 条补充导入（本轮）

- story 13、nether 13、end 9：translate key（advancements.<path>.title/description）、图标（映射表，含药水/头颅的 data）、帧（wiki bg）、roots 背景（nether/end.png）、flags（根=1/非根=2）、坐标（按父链的 tidy 布局撰写，**非抓包精确值**）。
- 条件按 wiki 要求文本撰写；复杂/无对应谓词的条目使用 minecraft:impossible 占位（follow_ender_eye、find_fortress、uneasy_alliance、all_potions、all_effects、enter_end_gateway、find_end_city、levitate）；deflect_arrow 用 entity_hurt_player + arrow 近似；obtain_armor/shiny_gear 用四件套 criteria OR。
- 实测：Loaded 87 advancement(s)；smelt_iron（铁锭）、nether/root + story/enter_the_nether（下界维度）、end/root + story/enter_the_end（末地维度）均完成；mine_stone、adventure/root、adventure/kill_a_mob 上一轮已验证。
- 剩余：recipes 21、35 条的精确坐标/flags/条件（完整抓包）、biome/effects 等谓词、rewards、旧成就桥、客户端实机、门禁、合并。

## recipes 21 条导入（本轮）

- 数据来自 14177 字节入服包的 21 条 recipes：全部无 display、parent=minecraft:recipes/root；root 的 criterion 是 impossible。
- criteria 映射：has_the_recipe -> recipe_unlocked（recipe=本仓库 crafting.txt 的配方名）；has_planks/has_log/has_stick/has_bone/has_egg -> inventory_changed；impossible -> impossible。
- crafting.txt 名称修复（前置依赖，同一分支）：WoodenButton 命名为 wooden_button；原 oak_button（实际产物 WoodenDoor）改为 wooden_door；WoodPlate 命名为 wooden_pressure_plate；新增 bone_meal_from_bone: BoneMeal, 3 = Bone。
- 配方名映射：oak_wooden_slab->oak_slab、trapdoor->oak_trapdoor、fence->oak_fence、fence_gate->oak_fence_gate、sign->oak_sign；其余与 crafting.txt 同名。
- 实测：Loaded 108 advancement(s)；bot 在 overworld 捡起一个 log（pickup 生命周期 6000）-> has_log=true、has_the_recipe=true、minecraft:recipes/building_blocks/oak_planks=true。
- 剩余：35 条的精确坐标/flags/条件（完整抓包）、biome/effects 等谓词、rewards、旧成就桥、客户端实机、门禁、合并。
