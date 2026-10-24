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

## recipes 材料条件映射（本轮）

- 128 个材料 criteria 名称映射到本服 items.ini：has_<item>、has_at_least_9_<item>（count min 9）、has_lots_of_items（配方材料）、lapis->lapislazuli、chorus_fruit_popped->poppedchorusfruit。
- 物品条件用数字 item type（MatchItemPredicate 支持 int），items.ini 显式 data 时带 data（如 black_wool -> 35:15）。
- 船的 entered_water -> enter_block water；boat 配方名映射到本服 oak_boat。
- 实测：拾取 9 个铁锭 -> has_at_least_9_iron_ingot=true；minecraft:recipes/building_blocks/iron_block 完成（其 requirements 允许该条件单独完成）。
- 剩余：has_the_recipe 的配方名映射（部分 vanilla 配方不在本服 crafting.txt，条件永不触发）；材料 data/count 条件是按 items.ini 与 criteria 名推断，不是逐条原版 JSON 条件。

## recipes 配方名覆盖分析（本轮）

- 433 条 recipes 中有 **101 条**的 advancement 配方名不在本服 crafting.txt（例如染色 hardened clay、各色床、花朵染料、部分方块/工具配方）。
- 其中 slab/boat/fence/fence_gate/sign/trapdoor 通过 sname 映射到本服名称；其余这些 advancement 的 has_the_recipe 条件使用原版配方名，本服没有对应配方，**条件永不触发**。
- 这属于本服配方数据缺口（crafting.txt 的配方集比 vanilla 少/命名不同），不是 advancement 数据包本身的问题：advancement 的 ID/criteria/requirements 与原版包逐条一致。
- recipes 材料条件已在上轮映射到本服 item type/data；iron_block 由 9 个铁锭的条件完成已实测。

## recipes 同名/同配方改名映射（本轮）

- 18 条 recipe 的 has_the_recipe 按「同结果 + 材料 type/data 子集」映射到本服名称：noteblock->note_block、brick_block->bricks、melon_block->melon、stonebrick->stone_bricks、red_nether_brick->red_nether_bricks、iron_ingot_from_block->iron_ingot_from_iron_block、gold_ingot_from_block->gold_ingot_from_gold_block、gold_ingot_from_nuggets->gold_ingot、pillar_quartz_block->quartz_pillar、magenta_dye_from_lapis_red_pink->magenta_dye_from_blue_red_pink、pink_dye_from_red_bonemeal->pink_dye_from_red_white_dye，以及 fence/sign/trapdoor/fence_gate/slab/boat 的既有映射。
- 撤回 red_dye_from_poppy/tulip -> red_dye_from_rose_bush 的错误映射（花朵 data 不同、是不同配方）。
- 实测：拾取 planks + redstone -> 本服 note_block 配方解锁 -> has_the_recipe=true -> minecraft:recipes/redstone/noteblock 完成。
- 其余 83 条 recipes 的本服 crafting.txt 无对应配方（染色 hardened clay、各色床、花朵染料、部分方块），属本服配方数据缺口，独立于 advancement 数据。

## 客户端数据对偶验证（本轮）

- 用 bot 以 488 进度入服，经 ProtoProxy 抓 Cuberite 的 Advancements 包（119362 bytes）并解码：
  - mapping=488；definitions 与 vanilla 包逐条 diff = **0**（title/description/icon/frame/flags/background/x/y/parent/criteria/requirements）。
  - progress 488 条；发现 7 条材料 criterion 在加载时被当作 date=0（vanilla 是 null）→ 修复 cPlayer::LoadFromDisk 跳过 null；修复后入服包 119306 bytes（少 56 字节）。
  - 结论：客户端收到与 vanilla 相同的 488 条定义；进度语义一致。
- 已知服务器问题：advancement grant everything 经 MCP 控制台触发 deadlock 检测（Core/MCPServer Lua 状态），服务器中止；客户端核对可改用离线玩家数据注入（advancements=vanilla progress）或 RCON。

## grant everything deadlock 修复（本轮）

- 原因：GrantAdvancementTree 对每条 advancement 调 GrantAdvancement，每条都 ComputeVisible + SendAdvancementUpdate；488 条时主线程长时间占用，触发 deadlock 检测（world age）并中止服务器。
- 修复：拆分 GrantAdvancementInternal（只改进度/完成/公告）与公共 GrantAdvancement；Grant/RevokeAdvancementTree 批量执行 internal，最后只发一次 SendAdvancementUpdate。
- 实测：新玩家数据下 advancement grant TestBot everything 返回 Granted all advancements，耗时 917ms，服务器保持在线；无新 deadlock。
- 客户端核对：人类客户端连 25565 后可直接 console grant everything，然后按 L 看 5 页签。

## location feature 谓词（本轮）

- location 条件新增 feature；通过 GetNearestStructureTarget 找最近结构目标，用水平半径近似结构边界（Stronghold/Fortress 64、EndCity 128、EndGateway 16）。
- EndCityGen 增加按名称的 GetNearestStructureTarget；cWorld 暴露 Lua API GetNearestStructureTarget（int out 参数）。
- 数据：follow_ender_eye(Stronghold)、find_end_city(EndCity)、find_fortress(Fortress)、enter_end_gateway(EndGateway)。
- 已知：半径是近似；本服无下界要塞/末地传送门结构时对应条件永不触发；Lua 侧 out 参数调用方式仍待修（本轮未完成实机测试）。
- 客户端：rrtt217 已 grant everything（933ms，服务器在线）；等待人类按 L 核对 5 页签。

## location feature 实机验证（本轮）

- cWorld::GetNearestStructureTarget 改为返回 Vector3i（Lua 友好），APIDump 同步。
- Stronghold 目标 (144,36,-189)：bot 传送后 in_stronghold=true，minecraft:story/follow_ender_eye 完成。
- EndCity 目标 (83,0,-68)：bot 传送到末地后 in_city=true，minecraft:end/find_end_city 完成。
- 下界要塞/末地传送门结构在本服不存在，find_fortress/enter_end_gateway 的条件永不触发（底层结构缺口）。
- 半径仍是近似（原版是结构 bounding box）。

## inventory_changed 索引（本轮，维护者反馈修复）

- 原实现每次槽位变化扫描全部 inventory_changed 条目（488 数据下为数百条），导致背包触发器明显变慢。
- 改为启动时建索引：m_InventoryItemIndex（item type -> entries）、m_InventorySlotEntries（slots 条件）、m_InventoryUnconditional（无 items/slots）。
- cInventory::OnSlotChanged 把变更槽位的 item type 传给 TriggerInventoryChanged；只评估该 type 的条目 + slots/unconditional，并去重。
- 实测：revoke story/root 的 crafting_table criterion -> false；给 1 个 crafting table -> criterion/advancement 在一个 tick 内变为 true。

## location feature 真实包围盒（本轮，维护者反馈修复）

- 实现 cGridStructGen::IsInsideStructure：按位置所在 chunk 取结构（带缓存锁），调用结构的 IsInside；stronghold 用 cPrefabStructure 各 piece 的 hitbox，end city 用 cEndCity 各 room 的 prefab 尺寸。
- 调用链：cWorld::GetGenerator -> cChunkGeneratorThread -> cComposableGenerator -> 各 finisher -> cGridStructGen；manager 的 location.feature 改为 IsInsideStructure（删掉水平半径近似）。
- 实测：stronghold 目标 (144,36,-189)；把 bot 放到目标 +200 格（344,-189）不触发（正确）；放到目标点本身暂未触发 —— 待查 hitbox/坐标（本轮未完成）。

## 结构包围盒 hitbox 修复（本轮）

- 根因：cPlacedPiece::MoveToGroundBy 只更新 m_Coords.y，没更新 m_HitBox，stronghold 的 piece 被移到地面后 hitbox 仍停在移动前高度，导致 IsInside 判断失败。
- 修复：MoveToGroundBy 同步 m_HitBox 的 p1.y / p2.y。
- 新增 cWorld:IsInsideStructure Lua API（APIDump 同步），便于直接核对。
- 实测：w:IsInsideStructure("Stronghold",144,37,-189)=true，344 处=false；但 bot 传到目标点后 in_stronghold criterion 仍为 false（location trigger 侧待查，下一轮）。

## location feature 调试（本轮）

- Lua 侧 w:IsInsideStructure("Stronghold",144,37,-189) = true；同一位置在 manager 的 location trigger 内返回 false。
- 网格调试日志：GetStructuresForChunk 返回 4 个候选结构（origin -9,-297 / 83,-68 / 220,-205 / 275,-123），逐个 IsInside=0；但 Lua 对同一位置的调用却是 true。
- 结论：问题在结构对象的 pieces/hitbox 状态在两次调用间不一致（或候选结构集合不同）；下一轮直接记录 cPrefabStructure 的 piece hitbox 与 cPlacedPiece 坐标。
- 临时调试日志已移除；构建/风格恢复通过。

## location feature 调试（轮 27 追加）

- cPrefabStructure::IsInside 对 stronghold 的 133 个 piece 在位置 144,37,-189 命中 box 144 36 -189 .. 156 43 -176，in=1；Lua 路径与 manager 路径一致。
- 轮 26 的 false 来自当时 GetStructuresForChunk 返回的候选结构集合（4 个非 stronghold 结构），不是 hitbox 问题；本轮同一位置已拿到 stronghold 的 133-piece 结构。
- 下一步：teleport 后确认 in_stronghold criterion 是否授予（本轮未测）。
