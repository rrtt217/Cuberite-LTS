# 1.12.2 /advancement 命令 spec（M2）

> **状态**：实现中（feature/advancements-command，叠在 feature/advancements-triggers-combat 之上；Core 对应分支 feature/core-advancement-command）。
> **来源**：[Commands（2017-10-01 修订）](https://minecraft.wiki/w/Commands?oldid=1143335) 的 `/advancement` 小节；advancement 数据/可见性见 [core spec](vanilla-1.12.2-advancements-core.md) 与 [observation](vanilla-1.12.2-advancements-observation.md)。

## 1. 语法（文档）

    advancement grant  <player> only|until|from|through <advancement> [criterion]
    advancement grant  <player> everything
    advancement revoke <player> only|until|from|through <advancement> [criterion]
    advancement revoke <player> everything
    advancement test   <player> <advancement> [criterion]

- `only`：只操作指定的 advancement。
- `until`：指定 advancement 加上它的祖先链（直到 root）。
- `from`：指定 advancement 加上它的全部后代。
- `through`：`until` + `from`。
- `everything`：当前加载的全部 advancement。
- 可选 `criterion`：只操作该 criterion；缺省时操作整个 advancement（所有 criterion）。
- `test`：只报告玩家是否拥有该 advancement / criterion，不修改。
- 权限：vanilla op 等级 2（本实现用 `core.advancement`，默认推荐 admins 组）。

## 2. 实现

- C++（父仓库）新增 3 个 cPlayer 方法：
  - `IsValidAdvancement(ID)`：ID 是否在加载表里；
  - `GrantAdvancementTree(ID, Mode, Criterion)` / `RevokeAdvancementTree(ID, Mode, Criterion)`：
    - Mode 为字符串 `only` / `until` / `from` / `through` / `everything`；
    - `everything` 忽略 ID（传空串），取 `GetAllIDs()`；
    - 其他模式要求 ID 有效，遍历用 manager 的 `GetParent()` / `GetChildren()`；
    - 逐个调用已有的 `GrantAdvancement` / `GrantAdvancementCriterion`（或 Revoke 版本），返回变化数；ID/模式无效返回 -1。
- Core（fork `rrtt217/Core` 分支 `feature/core-advancement-command`）：
  - `cmd_advancement.lua`：解析子命令/模式/criterion，`ResolveTargets` + `ApplyToTargets` 处理目标（含选择器），逐目标调用 C++ 方法；
  - `Info.lua`：`/advancement` 命令条目（`core.advancement`）、console `advancement` 条目、权限描述；
  - `tests/test_advancement_command.lua`：38 条断言（解析、模式透传、criterion、unknown、usage、空目标、console 返回）。
- 反馈文案（本实现，vanilla 原文〔待核〕）：
  - `Granted <advancement> to <player>` / `Revoked <advancement> from <player>`；
  - `<player> has <advancement>` / `<player> does not have <advancement>`；
  - `Unknown advancement <id>`、`No change for <player>`、以及 usage。
  - `everything` 显示 `all advancements`。
- 授予走 M1 的同一路径，因此会发进度更新并按数据里的 display 决定是否公告/toast；vanilla 命令授予是否公告〔待核〕。

## 3. 已知偏差

- 精确反馈措辞、以及 `grant` 是否触发公告/toast，未用原版实测核对〔待核：在 vanilla 1.12.2 控制台执行 `advancement grant @p only minecraft:story/root` 观察输出与客户端表现〕。
- `everything` 不接受 criterion（vanilla 语法本身没有该形式）。
- 目标选择器语义完全复用 Core 的选择器层（C1），跨世界目标按该层规则延迟一 tick。
- `test` 只支持单个 advancement；不支持 `test ... everything`（vanilla 语法也没有）。
- `until`/`from`/`through`/`everything` 的反馈每个目标只输出一行（描述输入的 advancement 或 "all advancements"），不会逐个列出被连带修改的后代；M2 先保持简洁，vanilla 是否逐条输出〔待核〕。

## 4. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| Core 单元测试 | `lua tests/test_advancement_command.lua` → 38 passed, 0 failed；`luacheck` 0 warnings |
| 实机（console，TestBot） | test 未拥有 → grant only → test 拥有 → revoke only → test 未拥有；`revoke from cuberite:m3/stick` 后 stick/five_sticks 都消失，`grant from` 后两者都恢复；`revoke everything` → diamond 消失，`grant everything` → diamond 恢复；criterion 形式 `only cuberite:m3/stick has_stick` 的 revoke/test/grant 全部正确；未知 ID → `Unknown advancement cuberite:nope`；非法模式 → usage |
| APIDump | 3 个新方法已写入 APIDesc；`NewlyUndocumented.lua` 中没有这三个名字（仍是基线那 6 个既有符号）；`DuplicateDocs.txt` 不存在；`apiCheckFailed.flag` 仍为基线文件（与 M1/C6 的既有状态一致） |
