# S14 状态效果的实体元数据（药水颜色 / ambient / 隐藏粒子）

分支：`feature/effects-potion-metadata` ｜ 级别：P2 ｜ 前置：**S13** ｜ 编排见 [roadmap](vanilla-1.12.2-status-effects-roadmap.md)

## 1. 问题

客户端**看不到别人身上的效果**（自己的 HUD 图标是客户端从效果表渲染的；别人身上的旋涡粒子由 Living 元数据的药水字段驱动）。本仓库的 metadata writer 从不写这些字段：

- 1.8：`cProtocol_1_8_0::WriteEntityMetadata`（[Protocol_1_8.cpp:3199 起](../src/Protocol/Protocol_1_8.cpp)）只写 flags / 名称 / 活着 / 血量等；`IsInvisible()` 只映射到 flags 的 `0x20` 位（[Protocol_1_8.cpp:3219-3222](../src/Protocol/Protocol_1_8.cpp)）。
- 1.9+：`cProtocol_1_9_0::WriteEntityMetadata`（[Protocol_1_9.cpp:1557 起](../src/Protocol/Protocol_1_9.cpp)）对 living 只写 flags + 名称(2) + 血量(6) + 皮肤(12/13)（[Protocol_1_9.cpp:1620-1642](../src/Protocol/Protocol_1_9.cpp)）。
- 后果：洞穴蜘蛛中毒的怪、被信标照的玩家、喝过药水的对手——**身上没有任何粒子**；凋灵玫瑰/滞留药水的视觉链路也缺一段。
- 另一处：`SendEntityEffect` 的 flags/隐藏粒子恒为 0/false（[Protocol_1_8.cpp:506-516](../src/Protocol/Protocol_1_8.cpp)）→ 信标效果的「环境（蓝框）」与 `/effect` 的 `hideParticles` 无法表达。

## 2. 行为规格与已核实的索引

来源：[wikivg 镜像 Entity metadata rev 4095（2017-04，对应 1.11/1.12）](https://wikivg.booky.dev/index.php?title=Entity_metadata&oldid=4095)，Living 段**已逐行核对**：

| Index | 类型 | 含义 |
|---|---|---|
| 6 | Byte | 手部状态（1.9+） |
| 7 | Float | 血量 |
| **8** | **VarInt** | **药水颜色（无效果为 0）** |
| **9** | **Boolean** | **ambient：粒子量降到常规的 1/5** |
| 10 | VarInt | 身上箭数 |

1.8 与 1.9+ 的索引空间**不同**（1.8 的 living 字段位于更高的索引）；**镜像的 `Entities` 页历史最晚只到 2017-11 的重命名，取不到 1.8 时代的 living 表** → 1.8 索引标 `[needs-check]`，动手前需另找允许来源（wiki.vg `Protocol` 1.8 时代修订的 metadata 节只给了包结构 [rev 965 §Entity Metadata](https://wikivg.booky.dev/index.php?title=Protocol&oldid=965)，未展开字段）。

**混合颜色算法**（1.9+ 只有颜色、没有「效果条数」字段）：允许来源未给出合成公式 → `[needs-check]`。
本分支采用可复现的推导并写进注释：**按各效果官方色按 amplifier 加权平均**（逐效果色值取自各效果页信息框，例：[Absorption](https://minecraft.wiki/w/Absorption) `#2552A5`、[Health Boost](https://minecraft.wiki/w/Health_Boost) `#F87D23`；注意 `Status_effect` 汇总页**不含色值**，已实测确认）；无任何效果时写 0（规格明列「or 0 if there is no effect」）。

## 3. 改动

1. 新增 `src/Entities/EffectColorRules.h`：`eType → 0xRRGGBB` 表 + `BlendEffectColor(const tEffectMap &) -> UInt32` + `IsAnyEffectAmbient(...)`（纯函数，可测）。
   表里每个色值旁边注明来源页；无法核实的条目留 `[needs-check]` 注释而不是猜。
2. 在两个 writer 的 living 分支追加字段：
   - 1.9+：写 index 8（VarInt 颜色）与 index 9（Bool ambient），仅当 `a_Entity.IsPawn()`；
   - 1.8：待索引核实后同样补写（**若索引无法核实，本分支只对 1.9+ 落地，1.8 记为已知偏差**）。
3. 变化时机：效果增删已经会重发 metadata（`OnActivate`/`OnDeactivate` 路径，见 [EntityEffect.cpp 的 invisibility 处理](../src/Entities/EntityEffect.cpp)），复用即可；**不要**每 tick 重发。
4. `SendEntityEffect` 增加 `a_bIsAmbient` / `a_bHideParticles` 形参（默认 false），由 `cEntityEffect` 提供（信标 = ambient）→ **改到导出签名，AGENTS 门 4/6 适用**（`AllToLua.pkg` + APIDump + `CheckBindingsDependencies.lua`）。若要把范围压到最小，可只在协议层内部按「效果来源标记」推断，不改 C++ 导出 —— 实现时二选一并说明。

## 4. 测试

`tests/Entities/EffectColorRulesTest.cpp`：
- 单效果 → 该效果的官方色；无效果 → 0；
- 两效果混合 → 结果落在两色分量区间内（不声称等于某个具体值，避免锁住未核实公式）；
- ambient 判定：全非 ambient → false；含一个 ambient → true；
- 色表完整性：23 个 `eType` 都有非 0 色值（防漏）。

**包级验证**：`Tools/ProtoProxy` 对比「有/无效果」的 living spawn 包，确认字段偏移与截断（`0xFF` 结束符）正确；无 oracle → 手工验证记录。

## 5. 已知偏差

- 1.8 客户端可能仍看不到他人粒子（索引待核）。
- 1.9+ 无「效果条数」字段，故粒子密度不随效果数量变化（vanilla 由颜色 + 条数共同决定，本分支无法完全复现）→ 记为可观测差异。
- 玩家血量在 1.9+ 写的是 index 6（[Protocol_1_9.cpp:1631-1633](../src/Protocol/Protocol_1_9.cpp)），而 rev 4095 的表里 6 是「手部状态」、血量在 7 —— **疑似既有索引错位**，本分支需先判明再落 8/9；若确认错位，它是独立缺陷，应拆成独立分支先行（AGENTS §3 前置依赖条款）。
