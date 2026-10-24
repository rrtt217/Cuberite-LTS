# 1.12.2 原版进度列表（收集）

> **状态**：列表收集完成；显示细节/条件/奖励仍待补。来源：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 “List of advancements” 小节（五个页签的 Advancement 模板表格）。

## 1. 方法与统计

- 解析 wiki 表格的 Advancement 模板：title、图标名、描述、父进度、要求文本、id；父进度按同页签标题映射为 ID。
- 帧（task/goal/challenge）由模板的 bg 推断：oval → goal，fancy → challenge，缺省 → task（待与原版显示标志核对）。
- 共 **55** 条：Minecraft 16、Nether 13、The End 9、Adventure 10、Husbandry 7；帧统计：task 35、goal 8、challenge 12。
- **不含 recipes 页签**：1.12.2 的配方进度（minecraft:recipes/...）是自动生成的另一组；M0 的 ProtoProxy 入服包实测有 21 条 recipes 进度（见 observation 规格）。

## 2. 列表

### Minecraft

| ID | 标题 | 父进度 | 图标 | 帧 | 描述 | 要求（wiki 文本） |
|---|---|---|---|---|---|---|
| minecraft:story/root | Minecraft | — | Grass Block | task | The heart and story of the game | Get a crafting table in your inventory. |
| minecraft:story/mine_stone | Stone Age | minecraft:story/root | Wooden Pickaxe | task | Mine stone with your new pickaxe | Get cobblestone in your inventory. |
| minecraft:story/upgrade_tools | Getting an Upgrade | minecraft:story/mine_stone | Stone Pickaxe | task | Construct a better pickaxe | Get a stone pickaxe in your inventory. |
| minecraft:story/smelt_iron | Acquire Hardware | minecraft:story/upgrade_tools | Iron Ingot | task | Smelt an iron ingot | Get an iron ingot in your inventory. |
| minecraft:story/obtain_armor | Suit Up | minecraft:story/smelt_iron | Iron Chestplate | task | Protect yourself with a piece of iron armor | Get any type of iron armor in your inventory. |
| minecraft:story/lava_bucket | Hot Stuff | minecraft:story/smelt_iron | Lava Bucket | task | Fill a bucket with lava | Get a lava bucket in your inventory. |
| minecraft:story/iron_tools | Isn't It Iron Pick | minecraft:story/smelt_iron | Iron Pickaxe | task | Upgrade your pickaxe | Get an iron pickaxe in your inventory. |
| minecraft:story/deflect_arrow | Not Today, Thank You | minecraft:story/obtain_armor | Shield | task | Deflect an arrow with a shield | Deflect a projectile with a shield. |
| minecraft:story/form_obsidian | Ice Bucket Challenge | minecraft:story/lava_bucket | Obsidian | task | Form and mine a block of Obsidian | Get a block of obsidian in your inventory. |
| minecraft:story/mine_diamond | Diamonds! | minecraft:story/iron_tools | Diamond | task | Acquire diamonds | Get a diamond in your inventory. |
| minecraft:story/enter_the_nether | We Need to Go Deeper | minecraft:story/form_obsidian | Flint and Steel | task | Build, light and enter a Nether Portal | Enter the Nether dimension. |
| minecraft:story/shiny_gear | Cover Me With Diamonds | minecraft:story/mine_diamond | Diamond Chestplate | task | Diamond armor saves lives | Get any type of diamond armor in your inventory. |
| minecraft:story/enchant_item | Enchanter | minecraft:story/mine_diamond | Enchanted Book | task | Enchant an item at an Enchanting Table | — |
| minecraft:story/cure_zombie_villager | Zombie Doctor | minecraft:story/enter_the_nether | Golden Apple | goal | Weaken and then cure a zombie villager | Throw a splash potion of weakness at a zombie villager and give it a golden apple (by facing the zombie and pressing the use key with a golden apple in your hand). |
| minecraft:story/follow_ender_eye | Eye Spy | minecraft:story/enter_the_nether | Eye of Ender | task | Follow an Ender Eye | Enter a stronghold. |
| minecraft:story/enter_the_end | The End? | minecraft:story/follow_ender_eye | End Stone | task | Enter the End Portal | Enter the End dimension. |

### Nether

| ID | 标题 | 父进度 | 图标 | 帧 | 描述 | 要求（wiki 文本） |
|---|---|---|---|---|---|---|
| minecraft:nether/root | Nether | — | Red Nether Brick | task | Bring summer clothes | Enter the Nether dimension. |
| minecraft:nether/fast_travel | Subspace Bubble | minecraft:nether/root | Empty Map | challenge | Use the Nether to travel 7km in the Overworld | Use the Nether to travel between 2 points in the Overworld with a minimum horizontal distance of 7000 blocks between each other. |
| minecraft:nether/find_fortress | A Terrible Fortress | minecraft:nether/root | Nether Brick | task | Break your way into a Nether Fortress | Enter a Nether fortress. |
| minecraft:nether/return_to_sender | Return to Sender | minecraft:nether/root | Fire Charge | challenge | Destroy a Ghast with a fireball | Kill a ghast using a ghast fireball. |
| minecraft:nether/obtain_blaze_rod | Into Fire | minecraft:nether/find_fortress | Blaze Rod | task | Relieve a Blaze of its rod | Get a blaze rod in your inventory. |
| minecraft:nether/get_wither_skull | Spooky Scary Skeleton | minecraft:nether/find_fortress | Wither Skeleton Skull | task | Obtain a wither skeleton's skull | Get a wither skeleton skull in your inventory. |
| minecraft:nether/uneasy_alliance | Uneasy Alliance | minecraft:nether/return_to_sender | Ghast Tear | challenge | Rescue a Ghast from the Nether, bring it safely home to the Overworld... and then kill it. | Kill a Ghast in the Overworld. |
| minecraft:nether/brew_potion | Local Brewery | minecraft:nether/obtain_blaze_rod | Potion of Regeneration | task | Brew a potion | Pick up an item from a brewing stand potion slot. |
| minecraft:nether/summon_wither | Withering Heights | minecraft:nether/get_wither_skull | Nether Star | task | Summon the Wither | Be within a 100.9×100.9×103.5 cuboid centered on the Wither when it is spawned. |
| minecraft:nether/all_potions | A Furious Cocktail | minecraft:nether/brew_potion | Milk Bucket | challenge | Have every potion effect applied at the same time | Have all of these 11 potion effects applied to you at the same time. |
| minecraft:nether/create_beacon | Bring Home the Beacon | minecraft:nether/summon_wither | Beacon | task | Construct and place a Beacon | Be within a 20×20×14 cuboid centered on a beacon block when it realizes it has become powered. |
| minecraft:nether/all_effects | How Did We Get Here? | minecraft:nether/all_potions | Bucket | challenge | Have every effect applied at the same time | Have all of these 20 effects applied to you at the same time. |
| minecraft:nether/create_full_beacon | Beaconator | minecraft:nether/create_beacon | Beacon | goal | Bring a beacon to full power | Be within a 20×20×14 cuboid centered on a beacon block when it realizes it is being powered by a size 4 pyramid. |

### The End

| ID | 标题 | 父进度 | 图标 | 帧 | 描述 | 要求（wiki 文本） |
|---|---|---|---|---|---|---|
| minecraft:end/root | The End | — | End Stone | task | Or the beginning? | Enter the End dimension. |
| minecraft:end/kill_dragon | Free the End | minecraft:end/root | Dragon Head | task | Good luck | Kill the ender dragon. |
| minecraft:end/dragon_egg | The Next Generation | minecraft:end/kill_dragon | Dragon Egg | goal | Hold the Dragon Egg | Get the dragon egg in your inventory. |
| minecraft:end/enter_end_gateway | Remote Getaway | minecraft:end/kill_dragon | Ender Pearl | task | Escape the island | Throw an ender pearl through or walk into an End gateway. |
| minecraft:end/respawn_dragon | The End... Again... | minecraft:end/kill_dragon | End Crystal | goal | Respawn the ender dragon. | Summon an ender dragon using ender crystals. |
| minecraft:end/dragon_breath | You Need a Mint | minecraft:end/kill_dragon | Dragon's Breath | goal | Collect dragon's breath in a glass bottle. | Get a bottle of dragon's breath in your inventory. |
| minecraft:end/find_end_city | The City at the End of the Game | minecraft:end/enter_end_gateway | Purpur Block | task | Go on in, what could happen? | Enter an End city. |
| minecraft:end/elytra | Sky's the Limit | minecraft:end/find_end_city | Elytra | goal | Find an Elytra | Get a pair of elytra in your inventory. |
| minecraft:end/levitate | Great View From Up Here | minecraft:end/find_end_city | Shulker Shell | challenge | Levitate up 50 blocks from the attacks of a Shulker | Have the Levitation effect applied, and move a vertical distance of 50 blocks. |

### Adventure

| ID | 标题 | 父进度 | 图标 | 帧 | 描述 | 要求（wiki 文本） |
|---|---|---|---|---|---|---|
| minecraft:adventure/root | Adventure | — | Empty Map | task | Adventure, exploration, and combat | Kill any entity, or be killed by any entity. |
| minecraft:adventure/kill_a_mob | Monster Hunter | minecraft:adventure/root | Iron Sword | task | Kill any hostile monster | Kill one of these 22 mobs. Other mobs are ignored for this advancement. |
| minecraft:adventure/trade | What a Deal! | minecraft:adventure/root | Emerald | task | Successfully trade with a Villager | — |
| minecraft:adventure/sleep_in_bed | Sweet dreams | minecraft:adventure/root | White Bed | task | Change your respawn point | Lie down in a bed. The advancement will be granted as soon as the player is in the bed, even if the player does not actually successfully sleep. |
| minecraft:adventure/shoot_arrow | Take Aim | minecraft:adventure/kill_a_mob | Bow | task | Shoot something with a bow and arrow | Fire a bow so that the arrow strikes any entity. |
| minecraft:adventure/kill_all_mobs | Monsters Hunted | minecraft:adventure/kill_a_mob | Diamond Sword | challenge | Kill one of every hostile monster | Kill each of these 22 mobs. Other mobs may be killed, but are ignored for the advancement. |
| minecraft:adventure/totem_of_undying | Postmortal | minecraft:adventure/kill_a_mob | Totem of Undying | goal | Use a Totem of Undying to cheat death | — |
| minecraft:adventure/summon_iron_golem | Hired Help | minecraft:adventure/trade | Pumpkin | goal | Summon an Iron Golem to help defend a village | Summon an iron golem. |
| minecraft:adventure/adventuring_time | Adventuring Time | minecraft:adventure/sleep_in_bed | Diamond Boots | challenge | Discover every biome | Visit these 36 biomes. Other biomes may also be visited, but are ignored for the advancement. |
| minecraft:adventure/sniper_duel | Sniper duel | minecraft:adventure/shoot_arrow | Arrow | challenge | Kill a skeleton with an arrow from more than 50 meters | Use a launched arrow to kill a skeleton from 50 or more blocks away, horizontally. |

### Husbandry

| ID | 标题 | 父进度 | 图标 | 帧 | 描述 | 要求（wiki 文本） |
|---|---|---|---|---|---|---|
| minecraft:husbandry/root | Husbandry | — | Hay Bale | task | The world is full of friends and food | Eat anything that can be eaten. |
| minecraft:husbandry/breed_an_animal | The Parrots and the Bats | minecraft:husbandry/root | Wheat | task | Breed two animals together | Breed a pair of one of these 10 mobs. Other breedable mobs, if any, are ignored for the advancement. |
| minecraft:husbandry/tame_an_animal | Best Friends Forever | minecraft:husbandry/root | Lead | task | Tame an animal | Tame one of the 5 types of mob that are tamable. |
| minecraft:husbandry/plant_seed | A Seedy Place | minecraft:husbandry/root | Wheat | task | Plant a seed and watch it grow | Plant one of these 5 plants. Other plants are ignored for the advancement. |
| minecraft:husbandry/breed_all_animals | Two by Two | minecraft:husbandry/breed_an_animal | Golden Carrot | challenge | Breed all the animals! | Breed pairs of each of these 10 mobs. Other tamable mobs, if any, are ignored for the advancement. |
| minecraft:husbandry/balanced_diet | A Balanced Diet | minecraft:husbandry/plant_seed | Apple | challenge | Eat everything that is edible, even if it's not good for you | Eat each of these 35 foods. Other foods, if any, are ignored for the advancement. |
| minecraft:husbandry/break_diamond_hoe | Serious Dedication | minecraft:husbandry/plant_seed | Diamond Hoe | challenge | Completely use up a diamond hoe, and then reevaluate your life choices | Use up the final bit of durability on a diamond hoe, so that it breaks. |

## 3. 字段状态

| 字段 | 状态 | 说明 |
|---|---|---|
| ID / 标题 / 描述 / 父进度 | 已收集 | 来自 wiki 列表；父进度已映射为 ID |
| 图标 | 名称待映射 | 需要映射到物品/方块 ID；药水、附魔书、生物头颅等特殊图标需单独处理 |
| 帧 | 推断待核 | 由 bg 推断；需与原版显示标志核对（M0 抓包可核对可见子集） |
| 坐标 x/y | 未收集 | 列表没有坐标；需要从树形图/观察或自行设计布局 |
| 背景 / show_toast / announce / hidden | 未收集 | 需要抓包或逐条观察；根进度通常 announce=false |
| criteria 的 trigger + conditions | 未收集 | 列表只有要求文本；需按触发器规格逐条撰写（已完成 25 个 trigger 的机制） |
| rewards（XP/配方/函数） | 未收集 | wiki 列表的“奖励”列/模板 prize 只能提示有奖励，具体值需逐条确认 |
| recipes 页签 | 未收集 | 21 条 recipes 进度在 M0 抓包里可见；需要单独的列表 |

## 4. 与已有抓包的交叉核对

- M0 入服包（14177 bytes）的可见子集：3 条 story + 10 条 adventure + 7 条 husbandry + 21 条 recipes；非 recipe 的 20 条都在上面的 55 条列表里。
- 抓包中的 tab 根：minecraft:story/root、minecraft:adventure/root、minecraft:husbandry/root、minecraft:recipes/root；本列表只覆盖前三个（story/adventure/husbandry）及 Nether/The End。
- 抓包里的显示细节（icon 的 id/count/damage、flags、坐标）可用于核对可见子集；完整 55 条的坐标/flags 仍需后续观察。

## 5. 收集脚本（一次性）

解析脚本读取 wiki 2017 修订的 wikitext（build/tmp/wiki/adv2017.json），按 === <tab> === 分段、按嵌套模板深度切分 Advancement 参数，输出 build/tmp/vanilla-advancements.json。脚本为一次性工具，未入库；本文件是收集结果。

## 6. 后续步骤

1. 收集 recipes 页签的 21+ 条进度（从 M0 抓包 ID + wiki/观察）。
2. 为 55 条 + recipes 补坐标、背景、toast/announce/hidden 与帧标志（优先用 M0 抓包的可见子集，其余用观察或自行布局）。
3. 按已实现的 trigger 规格逐条撰写 criteria/conditions（物品/实体/位置/距离等谓词子集已就绪）。
4. 图标名 → 物品/方块 ID 映射（优先用仓库的 StringToItem/StringToBlock 名称表，特殊图标单独处理）。
5. 奖励（XP/配方/函数）逐条确认；1.12.2 的 recipe 奖励可作为 recipes 页签的解锁入口。
