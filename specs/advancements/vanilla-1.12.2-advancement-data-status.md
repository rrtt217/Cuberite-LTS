# 1.12.2 原版进度数据导入状态

> 分支 feature/advancements-vanilla-data（叠在 feature/advancements-vanilla-list 之上）。数据来源：M0 入服包解码（20 条有 display 的可见条目）+ wiki 列表（列表/父链/要求文本）。

## 已导入（55/55；20 条精确抓包 + 35 条撰写）

- story 3：root（stone 背景，0,1.25）、mine_stone（1,1.25）、upgrade_tools（2,1.25）。
- adventure 10：root、kill_a_mob（22 个 mob 条件）、kill_all_mobs（22 个）、sniper_duel、trade、adventuring_time（36 个 biome）、sleep_in_bed、summon_iron_golem、totem_of_undying。
- husbandry 7：root、breed_an_animal（10 个 mob）、bred_all_animals（10 个）、balanced_diet（35 个食物）、plant_seed（5 个作物）、break_diamond_hoe。
- display 字段来自抓包：translate key、icon id/data、frame、flags（roots=1、非 roots=2）、背景、精确 x/y；criteria/requirements 来自抓包；conditions 按 wiki 要求文本 + 触发器规格撰写。

## 实测

- 服务端启动 `Loaded 52 advancement(s)`（20 vanilla）；只有 adventuring_time 的 36 个 biome criterion 打 unsupported 警告（预期）。
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
- 条件按 wiki 要求文本撰写；复杂/无对应谓词的条目曾使用 minecraft:impossible 占位；deflect_arrow 用 entity_hurt_player + arrow 近似；obtain_armor/shiny_gear 用四件套 criteria OR。
- 实测：Loaded 87 advancement(s)；smelt_iron（铁锭）、nether/root + story/enter_the_nether（下界维度）、end/root + story/enter_the_end（末地维度）均完成；mine_stone、adventure/root、adventure/kill_a_mob 上一轮已验证。
- 剩余：recipes 21、35 条的精确坐标/flags/条件（完整抓包）、biome/effects 等谓词、rewards、旧成就桥、客户端实机、门禁、合并。

## recipes 21 条导入（本轮）

- 数据来自 14177 字节入服包的 21 条 recipes：全部无 display、parent=minecraft:recipes/root；root 的 criterion 是 impossible。
- criteria 映射：has_the_recipe -> recipe_unlocked（recipe=本仓库 crafting.txt 的配方名）；has_planks/has_log/has_stick/has_bone/has_egg -> inventory_changed；impossible -> impossible。
- crafting.txt 名称修复（前置依赖，同一分支）：WoodenButton 命名为 wooden_button；原 oak_button（实际产物 WoodenDoor）改为 wooden_door；WoodPlate 命名为 wooden_pressure_plate；新增 bone_meal_from_bone: BoneMeal, 3 = Bone。
- 配方名映射：oak_wooden_slab->oak_slab、trapdoor->oak_trapdoor、fence->oak_fence、fence_gate->oak_fence_gate、sign->oak_sign；其余与 crafting.txt 同名。
- 实测：Loaded 76 advancement(s)；bot 在 overworld 捡起一个 log（pickup 生命周期 6000）-> has_log=true、has_the_recipe=true、minecraft:recipes/building_blocks/oak_planks=true。
- 剩余：35 条的精确坐标/flags/条件（完整抓包）、biome/effects 等谓词、rewards、旧成就桥、客户端实机、门禁、合并。

## biome location predicate（本轮）

- 实现：location 条件新增 biome；cAdvancementManager 增加 BiomeToAdvancementID（把服务端 EMCSBiome 名称映射到 1.12.2 advancement biome id，变种地图到基础 biome）；TriggerLocation 用 World->GetBiomeAt 匹配；NormalizeBiomeName 去掉 minecraft: 前缀。
- 覆盖 adventuring_time 的 36 个 biome criteria（不再打 unsupported 警告）。
- 实测：bot 的 biome=132（FlowerForest），映射为 forest，HasAdvancementCriterion(minecraft:adventure/adventuring_time, forest)=true。
- 剩余：location 的 feature/light/block 等谓词、effects 谓词、villager_trade、rewards、35 条精确抓包、旧成就桥、客户端实机、门禁、合并。

## 旧成就桥（M6，本轮）

- cPlayer::AwardLegacyAchievement(CustomStatistic)：不做旧成就树的前置检查；首次授予时广播 "Achievement get!" 并 SendStatistics。
- OnAdvancementCompleted：advancement 的 legacy_achievement 非空时，名字映射为 CustomStatistic 并调用 AwardLegacyAchievement；已映射 17 条（story/root→AchBuildWorkBench、upgrade_tools→AchBuildBetterPickaxe、smelt_iron→AchAcquireIron、mine_diamond→AchDiamonds、enter_the_nether→AchPortal、enter_the_end→AchTheEnd、enchant_item→AchEnchantments、adventure/kill_a_mob→AchKillEnemy、sniper_duel→AchSnipeSkeleton、adventuring_time→AchExploreAllBiomes、nether/obtain_blaze_rod→AchBlazeRod、return_to_sender→AchGhast、brew_potion→AchPotion、summon_wither→AchSpawnWither、create_full_beacon→AchFullBeacon、husbandry/breed_an_animal→AchBreedCow、end/kill_dragon→AchTheEnd2）。
- 实测：grant minecraft:story/mine_diamond → 聊天里同时出现 "has made the advancement [Diamonds!]" 与 "[INFO] Achievement get!"（旧成就广播由桥触发）。
- 旧成就 → advancement 的反向桥未实现（按 M6 字段语义只做 advancement → 旧成就；老代码路径仍走 AwardAchievement 的前置检查）。
- 剩余：35 条精确抓包坐标/flags、其余谓词、rewards、客户端实机、门禁、合并。

## 占位条件修复（本轮）

- story/follow_ender_eye：改为 minecraft:used_ender_eye；实测 bot 把末影之眼放进 hotbar 后 use → HasAdvancement(minecraft:story/follow_ender_eye)=true。
- nether/uneasy_alliance：改为 player_killed_entity + player.location.dimension=overworld；代码在实体谓词里新增 location.dimension，并在 TriggerPlayerKilledEntity 里匹配 killer 的 player 谓词；实测 overworld 击杀 ghast → HasAdvancement(nether/uneasy_alliance)=true（同时 return_to_sender 也 true）。
- 仍为占位：find_fortress（feature 谓词）、all_potions/all_effects（effects 谓词）、enter_end_gateway/find_end_city（结构/维度条件）、levitate（服务端无漂浮效果）。deflect_arrow 与 sniper_duel 仍是近似条件。
- 剩余：35 条精确抓包坐标/flags、feature/effects 谓词、rewards、客户端实机、门禁、合并。

## effects 谓词（本轮）

- effects_changed 现在支持 conditions.effects：效果 ID -> 空 predicate 或 amplifier/duration 范围；EffectNameToType 把 1.12.2 效果名映射到服务端 cEntityEffect::eType；MatchEffectsConditions 要求玩家同时拥有全部列出的效果。
- nether/all_potions 改为 effects_changed + 11 个 potion 效果（fire_resistance、invisibility、jump_boost、night_vision、poison、regeneration、slowness、speed、strength、water_breathing、weakness）。
- 实测：console 依次 effect TestBot <id> 30 0 施加 11 个效果 -> HasAdvancement(minecraft:nether/all_potions)=true。
- nether/all_effects 用 20 个效果的完整条件；服务端 eType 只有 1..23，缺 glowing/levitation -> 加载期跳过该 criterion，advancement 不可完成（属状态效果子系统的独立缺口）。
- 剩余：feature 谓词（find_fortress/enter_end_gateway/find_end_city）、glowing/levitation 效果、rewards、35 条精确坐标/flags、客户端实机、门禁、合并。

## 全量原版数据整合（本轮）

- 用 119277 字节的原版包（488 条：55 主树 + 433 recipes）替换/更新全部数据；逐条 diff（display 字段、criteria 名称、requirements）= **0**。
- 主树：display 的 translate key/icon/frame/flags/背景/x/y/parent 全部按包；criteria 名称与 requirements 按包；已撰写的 conditions 与 legacy_achievement 保留。
- recipes：433 条（含 root），criteria/requirements 按包；has_the_recipe 的 recipe 名映射到 crafting.txt（含 oak_wooden_slab -> oak_slab、trapdoor -> oak_trapdoor、fence -> oak_fence、fence_gate -> oak_fence_gate、sign -> oak_sign）；材料 criteria -> inventory_changed。
- 修复：cProtocol 输出缓冲区 64 KiB -> 2 MiB（原版 488 条入服包 119277 字节，超缓冲区导致 SendAdvancements 崩溃）。
- 加载 488；bot 入服不再崩溃；story/smelt_iron、recipes/oak_planks、adventure/root 命令正常；ctest 49/49。
- 已知：follow_ender_eye 的 location.feature=Stronghold 未实现；all_effects 的 glowing/levitation 未实现；find_fortress/enter_end_gateway/find_end_city/levitate 仍为 impossible 占位；recipes 的部分材料 item 名与本服 items.ini 未逐一核对。
