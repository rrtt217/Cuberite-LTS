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

- 入服：1.12–1.12.2 玩家在 SendStatistics 之后收到一次 reset=true 的更新，内容是**当前可见集合** + 每个条目的全部 criteria 进度（管理器 BuildJoinUpdate / ComputeVisible）。可见性按观察记录推导：某个 advancement 完成时，它自身 + 父/子方向 2 步内的节点 + 到 root 的祖先链进入可见集合；未授予 root 的页签不会被下发（客户端不会按进度过滤，实测它会显示收到的全部节点与 root 页签）。
- 授予/移除：reset=false；映射段只放**新变为可见**的定义，进度段放被改动的条目 + 新可见条目，移除段放不再可见的条目。
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
| 客户端 UI（fresh） | 已验：客户端重连后日志 `Loaded 0 advancements`，进度界面显示 "There doesn't seem to be anything here..."（无页签）——未授予 root 不再下发；截图 build/tmp/m1-fresh.png |
| 客户端 UI（root 授予） | 客户端日志 `Loaded 3 advancements`（root/child/grandchild）；树在已打开的进度界面里渲染。只有一个页签时客户端不画页签行（重开后仍然没有）；授予第二个 root 后同一界面立即出现两个页签（钻石 + 罗盘），说明页签行会随新 root 更新，单页签不显示是客户端行为〔待核：与原版单一页签对照〕 |
| 客户端 UI（second_root 授予） | 授予后客户端 `Loaded 4 advancements`，同一打开的界面出现钻石（M1 Root）+ 罗盘（second_root）两个页签；罗盘页签在未授予前不存在（可见性过滤正确）。截图 build/tmp/m1-second-root.png |
| 客户端 UI（child 授予） | 聊天公告已渲染：`rrtt217 has made the advancement [M1 Child]`（task 帧，`chat.type.advancement.task` + 标题被客户端翻译）；客户端日志 `Loaded 3 advancements` |
| 客户端 UI（grandchild 授予） | 聊天公告已渲染：`rrtt217 has reached the goal [M1 Grandchild]`（goal 帧 → `chat.type.advancement.goal`，客户端翻译为 "has reached the goal"）；toast 未在界面打开时截到〔待验证〕 |

## 8. 已知偏差 / 后续

- 可见性规则是"完成节点 + 2 步邻域 + 祖先"的近似：原版对更深的补发（depth ≥ 3）没有抓包样本，M3 触发器落地后再用实机对照。
- M1 的 demo 数据（第二个未授予 root、root→child→grandchild）用于验证：fresh 玩家应看到空页签、root 授予后出现 3 节点、第二个 root 不应出页签。
- 客户端在进度界面已打开时收到新增页签的增量包，只会更新数据（日志 `Loaded 3 advancements`），不会重建当前屏幕的页签列表；vanilla 是否也如此〔待核〕，实机验证时需要关闭重开界面。
- toast 的实机确认受 MCP 覆盖层（右上角）影响；可关闭界面后再授予一次并截图，或让人类看屏幕确认。
- task 与 goal 两个公告键都已在 1.12.2 客户端实测可译（"has made the advancement" / "has reached the goal"）；challenge 键仍〔待核〕。
- 没有触发器：criteria 目前只能由 API/命令授予；M3/M4 接触发器。
- rewards.loot 不支持。
- GetAdvancementProgress 未导出到 Lua（M2 若需要再导）。
- 公告的玩家名组件按观察记录实现（insertion/click/hover/show_entity）；未在客户端逐字核对〔待核〕。
