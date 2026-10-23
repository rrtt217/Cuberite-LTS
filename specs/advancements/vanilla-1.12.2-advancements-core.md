# 1.12.2 进度核心（M1）spec

> **状态**：实现中（feature/advancements-core，叠在 feature/advancements-protocol 之上）。
> **范围**：JSON 定义加载、玩家进度与持久化、授予/移除、公告、奖励、Lua API。**不含**触发器（M3/M4）与 /advancement 命令（M2）。
> **观察依据**：[同步实测记录](vanilla-1.12.2-advancements-observation.md)（更新包、公告、root flags、可见集合、translate 键、背景字符串）。

## 1. 数据文件与标识

- 目录：Server/advancements/&lt;namespace&gt;/&lt;path&gt;.json（CWD 相对；build/Server 由 CMake 建符号链接）。
- 标识 = 相对路径去掉 .json，并把第一个 / 换成 :。例如 Server/advancements/cuberite/m1/root.json → cuberite:m1/root。
- 格式（本仓库自己的子集，字段名贴近 wiki 的 JSON 形状）：

    {
      "parent": "cuberite:m1/root",
      "display": {
        "title": {"text": "..."} 或 {"translate": "advancements...."},
        "description": {...},
        "icon": {"item": 264, "data": 0} 或 {"item": "minecraft:diamond"},
        "frame": "task" | "goal" | "challenge",
        "background": "minecraft:textures/gui/advancements/backgrounds/stone.png",
        "show_toast": true,
        "announce_to_chat": true,
        "hidden": false,
        "x": 0, "y": 0
      },
      "criteria": {"name": {"trigger": "minecraft:..."}},
      "requirements": [["a"], ["b"]],
      "rewards": {"experience": 10, "recipes": ["..."], "function": "..."},
      "legacy_achievement": "AchDiamonds"
    }

- 校验：未知 parent / parent 环 / requirements 引用未知 criterion 的条目会被丢弃并 warning；重复 ID 保留第一个。
- 不支持的字段：rewards.loot（warning，忽略）。

## 2. 进度模型与持久化

- 玩家进度：advancement ID → criterion 名 → 达成时间（毫秒）。只存已达成条件。
- 生成 packet 进度时，管理器的 BuildProgress() 会把这个 advancement 的**全部** criteria 列出：已达成的带 bool=true + date，未达成的带 bool=false。
- 持久化：玩家 JSON 的 "advancements" 对象：

    "advancements": {
      "cuberite:m1/child": {"child_criterion": 1792731492783}
    }

  写于 SaveToDisk()，读于 LoadFromDisk()；未知 ID 保留在数据里（管理器忽略）。

## 3. 发送行为

- 入服：1.12–1.12.2 玩家在 SendStatistics 之后收到一次 reset=true 的更新，内容是**当前加载的全部 advancement** + 每个 advancement 的全部 criteria 进度（管理器 BuildJoinUpdate）。这是原版"可见集合"的超集；客户端按父节点完成情况过滤显示（见观察记录 §6）。
- 授予/移除：reset=false，只带该 advancement 的 progress（全部 criteria）；客户端已知定义，不需要重复声明。
- root：数据里显式 show_toast=false / announce_to_chat=false；非 root 默认 true。

## 4. 授予 / 移除语义

- GrantAdvancement(ID)：把该 advancement 的所有 criteria 标记为达成（同一时间戳）；只有从未完成变完成时才公告/发奖励；返回是否发生变化。
- GrantAdvancementCriterion(ID, Criterion)：只标记一个 criterion；若因此首次完成，同样公告/发奖励。
- RevokeAdvancement / RevokeAdvancementCriterion：删除进度并下发 progress（全部 criteria 未达成）。
- HasAdvancement(ID)：按 requirements 的 AND-of-OR 语义判断是否完成；HasAdvancementCriterion 只看单个 criterion。

## 5. 公告与奖励

- 公告（announce_to_chat=true 且有 display）：按观察记录 §4 的 JSON 形状，用 cAdvancementManager::BuildAnnouncementJson() 拼，向玩家所在世界的所有玩家 SendMessageRaw(json, ctSystem)。键为 chat.type.advancement.task / .goal / .challenge。
- 奖励：experience → DeltaExperience；recipes → 按 cCraftingRecipes 的 name→ID 表 AddKnownRecipe；function → ExecuteCommandFromOrigin("function <name>")，origin 为玩家。loot 暂不支持。

## 6. Lua API（M1 导出）

cPlayer 上（自动绑定）：
- GrantAdvancement(ID) / GrantAdvancementCriterion(ID, Criterion)
- RevokeAdvancement(ID) / RevokeAdvancementCriterion(ID, Criterion)
- HasAdvancement(ID) / HasAdvancementCriterion(ID, Criterion)

M2 的 /advancement 还需要遍历（父/子/全部 ID），留给 M2 导出（避免 M1 再引一次绑定与 APIDump）。

## 7. 验证

| 项 | 结果 |
|---|---|
| 风格 | CheckBasicStyle.lua → 0 violations |
| 构建 | 通过 |
| ctest | 49/49（排除 UrlClient-test / Google-test） |
| 数据加载 | 服务器启动日志 "Loaded 4 advancement(s) from \"advancements\"" |
| 授予 API | execute_lua：GrantAdvancementCriterion(root) → true，GrantAdvancement(child) → true，HasAdvancement(child) → true |
| 持久化 | 机器人退出后 players/30/...json 里出现 cuberite:m1/child / root 的 criteria 与时间戳 |
| 客户端 UI | 〔待验证〕1.12.2 客户端重连后：树出现、grant 时 toast + 公告；root 已达成时不重复公告 |

## 8. 已知偏差 / 后续

- 入服发全树而非原版可见集合；客户端过滤行为需三节点对照（M1 的 demo 数据已包含 root→child→grandchild）。
- 没有触发器：criteria 目前只能由 API/命令授予；M3/M4 接触发器。
- rewards.loot 不支持。
- GetAdvancementProgress 未导出到 Lua（M2 若需要再导）。
- 公告的玩家名组件按观察记录实现（insertion/click/hover/show_entity）；未在客户端逐字核对〔待核〕。
