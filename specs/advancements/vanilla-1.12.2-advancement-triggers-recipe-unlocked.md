# 1.12.2 进度触发器：recipe_unlocked spec

> **状态**：实现中（feature/advancements-triggers-recipe-unlocked，叠在 feature/advancements-triggers-used-totem 之上）。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:recipe_unlocked` 小节。

## 1. 原版行为（文档）

玩家解锁一个配方后触发。条件：

- `recipe`：配方 ID 字符串（如 `minecraft:boat`）。

## 2. 实现

- trigger 白名单加入 `minecraft:recipe_unlocked`；加载期只接受 `recipe`，其他键 warning 并跳过（fail-closed）。
- 调用点：`cPlayer::AddKnownRecipe(UInt32)` 插入成功并发送 UnlockRecipe 包之后 → `TriggerRecipeUnlocked(*this, a_RecipeId)`。
- 配方名：用 `cRoot::Get()->GetCraftingRecipes()->GetRecipeNameMap()` 反查 ID → 名字（`crafting.txt` 里的名字，如 `oak_boat`）；条件里的 `minecraft:` 前缀会被去掉后比较。
- demo：已移除（原 cuberite 测试数据由 1.12.2 原版数据取代，见 vanilla-1.12.2-advancement-delivery.md）。
- 注意：`AddKnownRecipe` 的既有实现直接解引用 `m_ClientHandle`，因此只在有客户端的路径（拾取/合成/奖励）触发；玩家读档阶段不会经过这里。

## 3. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 18 advancement(s)；无 unsupported 条件 warning |
| 触发授予 | revoke 再 grant `cuberite:m5/place_stone`（其 rewards 加 oak_boat）→ `HasAdvancement("cuberite:m5/recipe_unlocked")` = true（实测 true） |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 4. 已知偏差 / 后续

- 配方条件只支持单个 `recipe` 字符串；`recipe` 的命名空间前缀会被去掉（本仓库 `crafting.txt` 用裸名字）。
- 如果配方已经解锁，`AddKnownRecipe` 会提前返回，不会触发；与原版一致（只在首次解锁时触发）。
- 配方名映射依赖 `crafting.txt` 的 `m_RecipeName`；匿名配方（名字为空）无法通过 `recipe` 条件匹配。
