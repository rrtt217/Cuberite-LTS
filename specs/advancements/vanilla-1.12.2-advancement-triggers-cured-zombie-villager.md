# 1.12.2 进度触发器：cured_zombie_villager spec

> **状态**：实现中（feature/advancements-triggers-cured-zombie-villager，叠在 feature/advancements-triggers-bred-animals 之上）。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:cured_zombie_villager` 小节。

## 1. 原版行为（文档）

玩家治愈僵尸村民后触发。条件（都是 entity predicate）：

- `villager`：治愈后生成的村民；
- `zombie`：治愈完成前一刻的僵尸村民。

## 2. 实现

- `cZombieVillager` 新增 `m_CuringPlayerName`：`OnRightClicked()` 用金苹果 + 虚弱开始转化时记录玩家名。
- 转化完成（`m_ConversionTime == 0`）时：
  - 播 cure 音效；
  - 先生成村民并用 `DoWithEntityByID` 拿到实体；
  - 用 `cRoot::Get()->FindAndDoWithPlayer(m_CuringPlayerName, ...)` 找到治愈玩家（离线则不触发）；
  - 调用 `TriggerCuredZombieVillager(player, *this, Villager)`；
  - 最后 `Destroy()`。
- trigger 白名单加入 `minecraft:cured_zombie_villager`；加载期只接受 `villager` / `zombie`，其他键 warning 并跳过；匹配复用 `MatchEntityPredicateValue`。
- demo：已移除（原 cuberite 测试数据由 1.12.2 原版数据取代，见 vanilla-1.12.2-advancement-delivery.md）。

## 3. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 数据文件已加入；启动加载数应为 28（服务端重启后确认） |
| 触发授予 | 〔待核〕两次实机尝试均未观察到转化完成：第一次 6000 tick 期间僵尸村民消失（疑似被已驯服的狼击杀）；第二次临时 100 tick 测试中僵尸村民未被客户端追踪/喂食未命中。需要在和平难度、清空附近怪物、重新生成僵尸村民并确认虚弱后重试 |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 4. 已知偏差 / 后续

- 治愈玩家必须在线；离线时不会补触发（vanilla 行为〔待核〕）。
- 村民实体用 `DoWithEntityByID` 立即查找；若生成尚未加入世界，`villager` 条件不匹配（无条件 demo 不受影响）。
- 转化期间僵尸村民被杀死则不触发（与 vanilla 一致）。
- `villager`/`zombie` 谓词只支持 type/player。
