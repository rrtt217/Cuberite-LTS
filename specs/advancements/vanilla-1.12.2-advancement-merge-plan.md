# 1.12.2 进度系统：合并整理方案

> 分支：`feature/advancements-vanilla-data`（叠在 `feature/advancements-vanilla-list` 之上）
> 基线：`master` 的 `c5db1e2`；当前 55 个提交、437 文件、+14708/−1139。

## 1. 按原子单元归组

| 组 | 内容 | 涉及提交（新→旧） |
|---|---|---|
| A 数据 | 488 条 advancement + 数据修复（sniper_duel 距离、tame_an_animal 占位、recipe 奖励） | `af724bd`、`a16ac06`、`6771367`、`0b61571`、`f8fd2d4`、`653334e`、`fd1a9eb`、`0f484df`、`0b866f5`、`11f6512`、`01671d4` |
| B 触发器/谓词引擎 | AdvancementManager：触发器分发、实体/物品/位置/效果谓词、背包索引、join/update 包 | `29693ac`、`0bf31c1`、`709a8fb`、`43ba88f`、`ea8d9cd`、`ce28cf9`、`9eedbb4`、`931cc9f`、`d5074c5`、`a8b70cf`、`641e5bb`、`2c2e1de`、`e471ecd`、`496599b`、`1f8f26d`、`0b4160d`、`2e79fc6`、`8c85381`、`f2ba660` |
| C 结构判定 | `location.feature` 严格查询、piece 包围盒、end city 目标 | `f81ede9`、`67a1df0`、`0721720`、`c119980`、`427a607`、`b45a8f1`、`8926944`、`3e6d2cf`、`0aa6bd1`、`dd46650`、`59af4c7` |
| D 旧成就桥 | `Ach*` 映射 | `8fa714a`、`97b2da9` |
| E 服务器稳定性前置修复 | 怪物陈旧目标 ABRT、`SpawnMobFinalize` 触发时机 | `2d4ee49`、`641e5bb`（后半） |
| F Core 插件 | 移除 `/summon` 的非原版触发（submodule 指针） | Core `0da1082` / 父仓库 `2c2e1de` 内 gitlink |
| G 规格 | `specs/advancements/*` 共 31 个文件 | 分散在各提交 |

## 2. squash 建议

按上表合成 **6 个原子提交**（A–F；G 并入各自涉及的提交）：

1. `data: import the full 1.12.2 advancement set`（A + G 中数据相关规格）
2. `advancements: implement the trigger and predicate layer`（B + G 中引擎规格）
3. `worldgen: strict structure queries for location.feature`（C）
4. `advancements: bridge completions to legacy achievements`（D）
5. `mobs: fix stale target abort and spawn-time summon trigger`（E）
6. `core: drop the non-vanilla summon command trigger`（F）

## 3. 合并前门禁（须全部重跑）

- `cd src && lua CheckBasicStyle.lua`
- `cmake --build build -j 8`
- `cd build && ctest --output-on-failure -E 'UrlClient-test|Google-test'`
- 服务器启动日志：`Loaded 488 advancement(s)`，无 `unsupported entity predicate`
- Core submodule：`luac -p cmd_summon.lua`

## 4. 合并方式

由维护者决定 FF / merge commit；`git merge` 与推送只在维护者明确同意后执行（AGENTS.md §3）。
