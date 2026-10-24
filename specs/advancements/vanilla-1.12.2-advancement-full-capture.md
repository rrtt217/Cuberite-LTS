# 1.12.2 原版完整 advancement 包抓取记录

> 来源：合法原版 1.12.2 专用服务器（人类启动，端口 25567）+ ProtoProxy（25566 -> 25567）+ mineflayer TestBot；人类在服务器 console 执行 advancement grant TestBot everything。
> 未读取任何 Mojang 数据文件；全部数据来自合法客户端/服务端之间的协议实测。

## 抓包

- 日志：build/bin/Logs/Log_1792829416_5.log:206327（S->C Advancements 0x4d, 119277 bytes, 34.933s），206352（Advancements = 119277 bytes）。
- 解码：build/tmp/decode_vfull.py；逐字节 119277/119277 消耗完。
- reset=false；mapping=**488**；removals=0；progress=488。

## 构成

- **55 条有 display 的主树条目**（story 16、nether 13、end 9、adventure 10、husbandry 7）。
- **433 条 recipes 条目**（无 display；parent=minecraft:recipes/root）。
- 客户端 grant 反馈计数是 455，而包内 mapping 是 488：455 的统计口径与 488 的差 33 条〔待核〕（可能是 recipe root/无 criteria 的 recipe 条目不参与 grant 计数）。

## 与现有数据的关键差异

- minecraft:adventure/root 的坐标是 **(0.0, 2.5)**，而 M0 抓包/当前数据是 (0.0, 1.0)；需要以本次完整包为准更新。
- minecraft:adventure/shoot_arrow 存在（当前数据按 wiki 列表已导入 adventure 10 条，需要核对 IDs）。
- minecraft:nether/all_effects 的 flags=**6**（show_toast + hidden），当前数据是 hidden=false。
- 主树 display 的 translate key/icon/frame/flags/background/x/y 全部以本次包为准；criteria 名称与 requirements 也以本次包为准。
- recipes 是 **433** 条（不是 400）；当前按 crafting.txt 生成的 364 条是近似数据，下一步用本次的 433 条替换。

## 下一步

1. 用 build/tmp/vanilla-full-advancements.json 更新 55 条主树的 display/criteria/requirements（20 条已有抓包 + 35 条撰写数据）。
2. 用 433 条 recipes 替换当前 21 条 + 364 条生成数据；has_the_recipe 的 recipe 名映射到本服 crafting.txt 名称。
3. 重新加载/测试；与本次包做逐条 diff。
