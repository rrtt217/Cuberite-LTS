# 1.12.2 进度系统交付说明（评审候选）

> 状态：实现与门禁已完成；客户端视觉检查与合并等待维护者。
> 来源：Minecraft Wiki Advancement 2017-09-27 修订（oldid=1151757）、ProtoProxy 14177 字节原版入服包、本仓库既有数据与触发器规格。未引入 Mojang 数据文件。

## 1. 分支与提交

- feature/advancements-vanilla-list：2d4bf9b（55 条列表规格）、b01a4f3（入服包解码记录）。
- feature/advancements-vanilla-data（叠在 list 分支之上）：653334e（20 条抓包条目）、f8fd2d4（剩余 35 条）、0b61571（21 条 recipes + crafting.txt 名称修复）、0b4160d（biome location 谓词）、8fa714a（旧成就桥）、1f8f26d（player location 谓词 + 两个占位条件）、496599b（effects 谓词 + all_potions）。
- master 基线：c5db1e2；本分支栈可用 fast-forward 合并，尚未推送。

## 2. 交付范围

- 数据：Server/advancements/minecraft 下 76 个 JSON（55 主树 + 21 recipes；原 cuberite demo 数据已移除）；主树含 translate key、图标、帧、背景、flags、坐标；20 条坐标来自抓包，35 条按父链布局撰写。
- recipes：无 display、parent=minecraft:recipes/root；has_the_recipe -> recipe_unlocked，材料 -> inventory_changed；配方名映射见 0b61571（oak_slab/oak_trapdoor/oak_fence/oak_fence_gate/oak_sign 及 crafting.txt 修复）。
- 触发器：28 个 vanilla 触发器实现 25 个；villager_trade（无村民交易功能）与 levitation（无漂浮效果）缺底层功能；impossible 为数据专用。
- 谓词：inventory/entity/killing/durability/consume/placed/recipe/brewed/beacon/tame/breed/cure/changed_dimension/location(biome, dimension)/effects/player(location)/nether_travel。
- M6 旧成就桥：17 条 advancement 的 legacy_achievement 映射；首次授予广播 Achievement get!。

## 3. 验证

- 风格 0；构建通过；ctest 49/49（排除 UrlClient-test|Google-test）。
- 数据加载：Loaded 76 advancement(s)；仅 all_effects 的 minecraft:glowing 报 unsupported（底层效果缺失，预期）。
- 实机（自动 bot）：story/mine_stone、story/smelt_iron、story/enter_the_nether、story/enter_the_end、story/follow_ender_eye、adventure/root、adventure/kill_a_mob、adventure/kill_a_mob(ghast 条件)、nether/root、nether/uneasy_alliance、nether/return_to_sender、end/root、hubandry... 见各轮记录；recipes/building_blocks/oak_planks（log pickup -> recipe unlock）；nether/all_potions（11 效果）；旧成就桥 grant story/mine_diamond -> 聊天出现 [Diamonds!] 与 Achievement get!。
- adventuring_time：bot 的 FlowerForest 映射 forest -> criterion forest = true。

## 4. 已知偏差与底层依赖

- 35 条坐标是撰写布局，不是原版精确坐标；本机没有独立 vanilla 服务端，精确抓包需要外部环境。
- feature 谓词（find_fortress、find_end_city、enter_end_gateway）未实现；其中下界要塞生成器本仓库缺失、End gateway 行为需先行实现，属于本分支之外的底层缺口。
- all_effects 需要 glowing/levitation 效果，服务端 cEntityEffect::eType 只到 saturation；属状态效果子系统的独立缺口。
- villager_trade 触发器缺底层交易功能；levitation 缺底层效果。
- rewards：1.12.2 主树数据中没有 rewards；配方解锁由 recipe book 路径负责；自定义 function 未使用。
- 客户端视觉（5 个页签/树/公告/toast）尚未由人类客户端确认；M0 的客户端侧 toast 行为仍记录为客户端侧观察项。
- 旧成就桥只做 advancement -> 旧成就；反向桥未实现（按 M6 字段语义）。

## 5. 合并前置

- 维护者确认已知偏差可接受；
- 可选：人类客户端连 25565 后 console grant everything，按 L 核对 5 个页签与树；
- 维护者明确同意后 fast-forward 合并并推送（AGENTS §3）。
