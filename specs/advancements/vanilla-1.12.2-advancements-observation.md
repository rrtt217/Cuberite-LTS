# 1.12.2 进度同步实测记录（ProtoProxy）

> 本文件是观察记录，不是行为规格；规格在 [progress protocol spec](vanilla-1.12.2-advancements-protocol.md) 与 roadmap 分支里。
> 数据来源：本机 ProtoProxy（协议 340）对着一台原版 1.12.2 服务器（远端端口 25567）的抓包。
> 记录日期：2026-10-23。

## 1. 证据文件

| 抓包 | 内容 | 关键行 |
|---|---|---|
| build/bin/Logs/Log_1792730526_5.log | 一次完整进服 + 打开三个页签；入服全量包 14177 字节 | 111724（入服包）、184396/184406（页签往返） |
| build/bin/Logs/Log_1792720840_5.log | 一次完整游戏会话；9 个"获得进度"更新包 + 1 条公告 | 200741、211096、221841、248315、263065、263449、525564、588786、590394（更新包）；588602（公告） |
| build/bin/Logs/Log_1792127802_5.log | 新玩家视角的入服包 1374 字节（只有 recipe 进度） | 34461 |

## 2. 入服包（Log_1792730526_5.log:111724）

- 时间 0.849s，包 0x4D，未压缩载荷 14177 字节；逐字节解码后正好消费完。
- reset = 1，mapping = 41，removals = 0，progress = 41（每个 mapping 条目都带进度）。
- 41 条的构成：
  - minecraft:story 3 条（root、mine_stone、upgrade_tools）；
  - minecraft:adventure 10 条（root + 9 个子/孙节点）；
  - minecraft:husbandry 7 条（root + 3 个子 + 3 个孙节点）；
  - minecraft:recipes 21 条（无 display，recipe/root + 20 个配方）。
  - **没有 nether、没有 end**：这两个页签的 root 在这个玩家身上还没被授予，服务端连定义都不发。
- 对照新玩家（Log_1792127802_5.log:34461，1374 字节）：mapping 只有 7 条 recipe/transportation 船配方 + recipe/root，没有任何 display 树。
  ⇒ 入服包不是"全树"，而是"当前可见集合"。

### 2.1 显示字段（从同一包解出）

- 三个 root 的 background 分别是：
  - minecraft:textures/gui/advancements/backgrounds/stone.png（story）
  - minecraft:textures/gui/advancements/backgrounds/adventure.png（adventure）
  - minecraft:textures/gui/advancements/backgrounds/husbandry.png（husbandry）
  - nether / end 本次没抓到（root 未授予）；按命名规律应是 nether.png / end.png〔推测，置信度高〕。
- root 的 flags = 1（只有 background，**没有 show_toast**、没有 hidden）；非 root 的 display 节点 flags = 2（show_toast，无 background、无 hidden）。本包内没有 hidden 节点。
- frame 值分布：0（task）12 个、1（challenge）6 个、2（goal）2 个，与协议的 0/1/2 次幂一致。
- title/description 全部是 translate 组件，例如：
  - {"translate":"advancements.story.root.title"}
  - {"translate":"advancements.adventure.kill_a_mob.title"}
  - {"translate":"advancements.husbandry.breed_an_animal.title"}
  说明原版 1.12.2 的进度标题/描述确实走客户端语言表；本仓库若要本地化，应使用同一族键（键是否存在，M1 实机再核）。
- icon 是普通 Slot（id/count/damage，无 NBT），例如 story/root 的 icon 是 id 2（草方块）、adventure/root 是 id 395、husbandry/root 是 id 170。

## 3. 更新包（Log_1792720840_5.log）

9 个包都在 reset = 0、removals = 0 下发送；每个包都是"新可见的声明 + 这些声明的当前进度"：

| 时间 | 载荷 | mapping（新声明） | progress（变化/新可见） |
|---|---|---|---|
| 12.888 | 254 | recipes/food/cake + recipes/root | cake: has_egg=1；root: impossible=0 |
| 18.242 | 202 | recipes/building_blocks/oak_planks | has_log=1 |
| 24.838 | 1000 | recipes/decorations/crafting_table + story/root + story/mine_stone + story/upgrade_tools | story/root: crafting_table=1；mine_stone/upgrade_tools 无成就 |
| 41.090 | 1970 | 10 个 recipe（slab/fence/button/pressure_plate/trapdoor/door/stick/sign/stairs/fence_gate） | 每个 recipe 的 has_planks=1 |
| 49.638 | 196 | recipes/tools/wooden_pickaxe | has_the_recipe=1 |
| 49.841 | 940 | 5 个 recipe（sword/axe/hoe/ladder/shovel） | 每个 has_stick=1 |
| 120.788 | 3986 | husbandry/root + 6 个节点 | root: consumed_item=1；balanced_diet: enchanted_golden_apple=1；其余无成就 |
| 143.489 | 5460 | adventure/root + 9 个节点 | root: killed_something=1；kill_a_mob: skeleton=1；kill_all_mobs: skeleton=1；adventuring_time: forest_hills/plains=1；其余无成就 |
| 144.037 | 201 | recipes/misc/bone_meal_from_bone | has_bone=1 |

要点：

1. **声明与进度同包**：新可见的进度（例如 husbandry 整棵树）把定义和当前进度一起发，客户端不需要额外请求。
2. **所有 criteria 都写**：progress 里每个条件都出现，achieved=false 的条件也写（带 achieved=0、无 date），不是只写已达成的。这一点修正了 M0 的简化实现（M0 只写已达成条件；1.12.2 客户端对缺省条件的处理〔待核〕）。
3. **进度先于可见性存在**：adventure 整棵树在 143.489 才发，但 adventuring_time 的 forest_hills/plains 进度是更早获得的；服务端在树可见时把累积进度一并发出。
4. **可见性规则（从本会话推导）**：
   - 一个页签的 root 被授予（root 的 criteria 达成）时，服务端发 root + 该树下 **深度 ≤ 2** 的所有节点（story 3 条、husbandry 7 条、adventure 10 条都符合）；
   - 更深的节点在父节点完成后再补发（本会话没有走到深度 3 的样本，规则〔待核〕）；
   - recipe 节点在玩家获得对应进度/配方时按需发送（无 display，客户端只用它做配方书/toast）。
5. **root 不发公告**：story root（24.838）、husbandry root（120.788）的更新包都没有伴随聊天消息；adventure root 与 kill_a_mob 同一时刻更新，只有 kill_a_mob 有公告。结合 root 的 flags=1（无 show_toast），root 的 show_toast/announce_to_chat 都是 false。

## 4. 公告包（Log_1792720840_5.log:588602）

时间 143.488，0x0F Chat Message，position=1（系统消息）。JSON 原文（格式化后）：

    {
      "translate": "chat.type.advancement.task",
      "with": [
        {
          "insertion": "rrtt217",
          "clickEvent": { "action": "suggest_command", "value": "/msg rrtt217 " },
          "hoverEvent": {
            "action": "show_entity",
            "value": { "text": "{name:\"rrtt217\",id:\"078996bf-0cdf-3003-94b8-8de2270212b7\"}" }
          },
          "text": "rrtt217"
        },
        {
          "color": "green",
          "extra": [
            {
              "hoverEvent": {
                "action": "show_text",
                "value": {
                  "extra": [
                    { "color": "green", "translate": "advancements.adventure.kill_a_mob.title" },
                    { "text": "\n" },
                    { "translate": "advancements.adventure.kill_a_mob.description" }
                  ],
                  "text": ""
                }
              },
              "translate": "advancements.adventure.kill_a_mob.title"
            },
            { "text": "]" }
          ],
          "text": "["
        }
      ]
    }

- 键按 frame 分：本次是 task → chat.type.advancement.task；goal/challenge 用同族键〔待核：本会话没有 goal/challenge 的公告样本〕。
- 第二个参数不是纯文本：外层是绿色、text 为 "["、extra 里是 title 组件（hover 显示 title + 换行 + description）与 "]"。
- 玩家名参数带 insertion / clickEvent / hoverEvent（show_entity，value 是嵌套 JSON 字符串）。
- 本仓库现有 cCompositeChat 的 AddClientTranslatedPart 只能把参数当字符串（[CompositeChat.cpp:297](../../src/CompositeChat.cpp)），无法表达这种嵌套；M1 需要专用 part（参考 AddShowAchievementPart，[CompositeChat.cpp:338-340](../../src/CompositeChat.cpp)）。

## 5. 页签交互（Log_1792730526_5.log）

- 6.006s：客户端 C->S Advancement Tab (0x19)，tab = minecraft:husbandry/root；6.026s 服务端回 S->C Select Advancement Tab (0x37)，tab 相同（Log_1792730526_5.log:184396/184406）。
- story、adventure 页签同样：0x19 → 0x37 原样回显。
- 说明服务端会记住/回显页签选择；M0 目前只丢弃，不影响显示，但 M1 可补上回显。

## 6. 对 M1 的直接影响

1. **入服包可以是全树**：原版发的是"可见集合"（root + 深度 ≤ 2 + recipe 进度），但客户端本身会按"父节点完成才显示"过滤（2017 页原文），所以发全树是原版可见集合的超集；是否会造成页签/深层节点差异，需要一次"三节点（root→child→grandchild，child 未完成）"的实机对照〔待核〕。
2. **更新包格式**：reset=0；mapping 只放新可见的声明；progress 放变化/新可见进度的**全部**条件（含 achieved=false）；removals 本会话恒为 0。
3. **公告**：用 chat.type.advancement.<frame> + 玩家名组件 + 绿色 bracket 组件；需要新增 CompositeChat part。
4. **root 的 show_toast/announce_to_chat 都是 false**；非 root 默认 true。
5. **数据**：title/description 用 advancements.<tab>.<name>.title/.description；background 用上面三个字符串（nether/end 待抓）；icon 是普通物品。

## 7. 未覆盖 /〔待核〕

- nether / end 的 root、background 字符串、公告样本（需要玩家进入下界/末地）。
- depth ≥ 3 的补发规则（需要玩家在 story 里继续做到 acquire_hardware）。
- goal / challenge 的公告键。
- hidden 进度在包里的表现（本会话没有 hidden 节点被发）。
- 客户端对"全树"里未授予 root / 未完成父节点的深层节点是否完全隐藏（M1 三节点实验）。
